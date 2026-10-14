#include "web_endjinn_webgl.h"
#include "web_endjinn_frame.h"

#include "../host-common/host_limits.h"
#include "../host-common/host_pvr.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#else
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>

extern "C" void enj_host_input_shutdown(void);

namespace {

using web_endjinn::DrawBatch;
using web_endjinn::FrameDrawData;
using web_endjinn::WebModifierTriangle;
using web_endjinn::WebVertex;
using enj_host_pvr::QueuedPrimitive;

struct WebTexture {
  GLuint id{};
  uint64_t texture_revision{};
  uint64_t palette_revision{};
  uint32_t texture_filter{UINT32_MAX};
  bool has_mips{};
};

struct GenericDrawState {
  bool depth_test{};
  bool depth_write{};
  bool color_write{};
  bool stencil_test{};
  bool blend{};
  bool punch_through{};
  GLenum depth_func{GL_GEQUAL};
  GLenum stencil_func{GL_ALWAYS};
  GLint stencil_reference{};
  GLuint stencil_func_mask{0xffu};
  GLuint stencil_write_mask{0xffu};
  GLenum stencil_depth_pass{GL_KEEP};
  GLenum blend_source{GL_SRC_ALPHA};
  GLenum blend_destination{GL_ONE_MINUS_SRC_ALPHA};
};

enum class DrawMode {
  Standard,
  ModifierXor,
  ModifierOr,
  ModifierInclude,
  ModifierExclude,
  ModifierClearCurrent,
  OpaqueModifierReceiver,
};

vid_mode_t g_video_mode{640, 480};
SDL_Window *g_window = nullptr;
SDL_GLContext g_context = nullptr;
GLuint g_program = 0;
GLuint g_vertex_buffer = 0;
GLuint g_vertex_array = 0;
GLuint g_white_texture = 0;
GLuint g_modifier_texture = 0;
GLint g_position = -1;
GLint g_color = -1;
GLint g_uv = -1;
GLint g_sampler = -1;
GLint g_textured = -1;
GLint g_punch_through = -1;
GLint g_modifier_sampler = -1;
GLint g_modifier_count = -1;
GLint g_modifier_texture_width = -1;
GLint g_modifier_area = -1;
float g_bg_color[3]{};
bool g_fsaa = false;
bool g_translucent_autosort = true;
bool g_ready = false;
std::unordered_map<uint64_t, WebTexture> g_textures;
GLuint g_bound_texture = 0;
GenericDrawState g_generic_draw_state;
bool g_generic_draw_state_valid = false;

constexpr const char *vertex_shader = R"(#version 300 es
in vec3 a_position;
in vec4 a_color;
in vec2 a_uv;
out vec4 v_color;
out vec2 v_uv;
out vec3 v_position;
void main() {
  gl_Position = vec4(a_position, 1.0);
  v_color = a_color;
  v_uv = a_uv;
  v_position = a_position;
}
)";

constexpr const char *fragment_shader = R"(#version 300 es
precision highp float;
precision highp int;
in vec4 v_color;
in vec2 v_uv;
in vec3 v_position;
uniform sampler2D u_texture;
uniform sampler2D u_modifier_events;
uniform bool u_textured;
uniform bool u_punch_through;
uniform int u_modifier_count;
uniform int u_modifier_texture_width;
uniform int u_modifier_area;
out vec4 fragment_color;

vec4 modifier_event_texel(int linear_index) {
  return texelFetch(u_modifier_events,
                    ivec2(linear_index % u_modifier_texture_width,
                          linear_index / u_modifier_texture_width), 0);
}

float cross2(vec2 a, vec2 b) {
  return a.x * b.y - a.y * b.x;
}

bool top_left_edge(vec2 edge) {
  /* WebGL NDC Y is the inverse of the submitted framebuffer Y. */
  return edge.y > 0.0 || (edge.y == 0.0 && edge.x < 0.0);
}

bool triangle_crosses_receiver(int index) {
  vec3 av = modifier_event_texel(index * 4).xyz;
  vec3 bv = modifier_event_texel(index * 4 + 1).xyz;
  vec3 cv = modifier_event_texel(index * 4 + 2).xyz;
  vec2 a = av.xy;
  vec2 b = bv.xy;
  vec2 c = cv.xy;
  float az = av.z;
  float bz = bv.z;
  float cz = cv.z;
  vec2 p = v_position.xy;
  float denominator = cross2(b - a, c - a);
  if (abs(denominator) < 0.0000001) return false;
  if (denominator < 0.0) {
    vec2 swap_position = b;
    b = c;
    c = swap_position;
    float swap_depth = bz;
    bz = cz;
    cz = swap_depth;
    denominator = -denominator;
  }
  vec2 edge0 = b - a;
  vec2 edge1 = c - b;
  vec2 edge2 = a - c;
  float coverage0 = cross2(edge0, p - a);
  float coverage1 = cross2(edge1, p - b);
  float coverage2 = cross2(edge2, p - c);
  const float edge_epsilon = 0.0000001;
  if (coverage0 < -edge_epsilon ||
      (abs(coverage0) <= edge_epsilon && !top_left_edge(edge0)) ||
      coverage1 < -edge_epsilon ||
      (abs(coverage1) <= edge_epsilon && !top_left_edge(edge1)) ||
      coverage2 < -edge_epsilon ||
      (abs(coverage2) <= edge_epsilon && !top_left_edge(edge2))) {
    return false;
  }
  float wa = cross2(b - p, c - p) / denominator;
  float wb = cross2(c - p, a - p) / denominator;
  float wc = 1.0 - wa - wb;
  float modifier_depth = wa * az + wb * bz + wc * cz;
  return modifier_depth > v_position.z;
}

bool area_one_at_receiver() {
  bool current = false;
  bool summary = false;
  bool pending = false;
  for (int i = 0; i < u_modifier_count; i++) {
    vec4 state = modifier_event_texel(i * 4 + 3);
    if (triangle_crosses_receiver(i)) {
      current = state.y > 0.5 ? true : !current;
    }
    pending = true;
    if (state.x > 0.5) {
      summary = int(state.z + 0.5) == 2 ? summary && !current
                                       : summary || current;
      current = false;
      pending = false;
    }
  }
  if (pending) summary = summary || current;
  return summary;
}

void main() {
  if (u_modifier_area != 0) {
    bool area_one = area_one_at_receiver();
    if ((u_modifier_area == 1 && area_one) ||
        (u_modifier_area == 2 && !area_one)) discard;
  }
  vec4 color = (u_textured ? texture(u_texture, v_uv) : vec4(1.0)) * v_color;
  if (u_punch_through && color.a < 0.5) discard;
  fragment_color = color;
}
)";

float clamp01(float value) { return std::min(std::max(value, 0.0f), 1.0f); }

GLuint compile_shader(GLenum type, const char *source) {
  const GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  GLint ok = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (ok == GL_TRUE) {
    return shader;
  }
  char message[1024]{};
  glGetShaderInfoLog(shader, sizeof(message), nullptr, message);
  std::fprintf(stderr, "web-enDjinn: shader compile failed: %s\n", message);
  glDeleteShader(shader);
  return 0;
}

bool create_program() {
  const GLuint vertex = compile_shader(GL_VERTEX_SHADER, vertex_shader);
  const GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);
  if (vertex == 0 || fragment == 0) {
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return false;
  }
  g_program = glCreateProgram();
  glAttachShader(g_program, vertex);
  glAttachShader(g_program, fragment);
  glLinkProgram(g_program);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  GLint ok = GL_FALSE;
  glGetProgramiv(g_program, GL_LINK_STATUS, &ok);
  if (ok != GL_TRUE) {
    char message[1024]{};
    glGetProgramInfoLog(g_program, sizeof(message), nullptr, message);
    std::fprintf(stderr, "web-enDjinn: shader link failed: %s\n", message);
    glDeleteProgram(g_program);
    g_program = 0;
    return false;
  }
  g_position = glGetAttribLocation(g_program, "a_position");
  g_color = glGetAttribLocation(g_program, "a_color");
  g_uv = glGetAttribLocation(g_program, "a_uv");
  g_sampler = glGetUniformLocation(g_program, "u_texture");
  g_textured = glGetUniformLocation(g_program, "u_textured");
  g_punch_through = glGetUniformLocation(g_program, "u_punch_through");
  g_modifier_sampler = glGetUniformLocation(g_program, "u_modifier_events");
  g_modifier_count = glGetUniformLocation(g_program, "u_modifier_count");
  g_modifier_texture_width =
      glGetUniformLocation(g_program, "u_modifier_texture_width");
  g_modifier_area = glGetUniformLocation(g_program, "u_modifier_area");
  return true;
}

uint64_t texture_key(const DrawBatch &batch) {
  return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(batch.texture)) |
         (static_cast<uint64_t>(batch.texture_format) << 32u);
}

void bind_texture_2d(GLuint texture) {
  if (g_bound_texture != texture) {
    glBindTexture(GL_TEXTURE_2D, texture);
    g_bound_texture = texture;
  }
}

GLuint texture_for(const DrawBatch &batch) {
  if (!batch.textured) {
    return g_white_texture;
  }
  const uint64_t key = texture_key(batch);
  WebTexture &cached = g_textures[key];
  const uint64_t texture_revision =
      enj_host_pvr::texture_revision(batch.texture);
  const uint64_t palette_revision =
      enj_host_pvr::palette_revision(batch.texture_format);
  if (cached.id != 0 && cached.texture_revision == texture_revision &&
      cached.palette_revision == palette_revision) {
    if (cached.texture_filter !=
        static_cast<uint32_t>(batch.texture_filter)) {
      bind_texture_2d(cached.id);
      const bool nearest = batch.texture_filter == PVR_FILTER_NEAREST;
      const bool trilinear = batch.texture_filter == PVR_FILTER_TRILINEAR1 ||
                             batch.texture_filter == PVR_FILTER_TRILINEAR2;
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                      nearest ? GL_NEAREST : GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                      trilinear && cached.has_mips
                          ? GL_LINEAR_MIPMAP_LINEAR
                          : (nearest ? GL_NEAREST : GL_LINEAR));
      cached.texture_filter = static_cast<uint32_t>(batch.texture_filter);
    }
    return cached.id;
  }

  QueuedPrimitive description{};
  description.textured = true;
  description.texture = batch.texture;
  description.texture_format = batch.texture_format;
  description.texture_width = batch.texture_width;
  description.texture_height = batch.texture_height;
  description.texture_filter = batch.texture_filter;
  enj_host_pvr::DecodedTexture decoded;
  if (!enj_host_pvr::decode_texture(description, decoded)) {
    return g_white_texture;
  }
  if (cached.id == 0) {
    glGenTextures(1, &cached.id);
  }
  bind_texture_2d(cached.id);
  if (decoded.indexed) {
    const auto palette = enj_host_pvr::palette_rgba();
    for (size_t level = 0; level < decoded.mips.size(); level++) {
      const auto &mip = decoded.mips[level];
      std::vector<uint32_t> rgba(mip.pixels.size());
      for (size_t i = 0; i < rgba.size(); i++) {
        const size_t color = decoded.palette_base + mip.pixels[i];
        rgba[i] = color < palette.size() ? palette[color] : 0xffffffffu;
      }
      glTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), GL_RGBA,
                   static_cast<GLsizei>(mip.width),
                   static_cast<GLsizei>(mip.height), 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, rgba.data());
    }
  } else {
    for (size_t level = 0; level < decoded.mips.size(); level++) {
      const auto &mip = decoded.mips[level];
      glTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), GL_RGBA,
                   static_cast<GLsizei>(mip.width),
                   static_cast<GLsizei>(mip.height), 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, mip.pixels.data());
    }
  }
  const bool nearest = batch.texture_filter == PVR_FILTER_NEAREST;
  const bool trilinear = batch.texture_filter == PVR_FILTER_TRILINEAR1 ||
                         batch.texture_filter == PVR_FILTER_TRILINEAR2;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  nearest ? GL_NEAREST : GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  trilinear && decoded.mips.size() > 1
                      ? GL_LINEAR_MIPMAP_LINEAR
                      : (nearest ? GL_NEAREST : GL_LINEAR));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  cached.texture_revision = texture_revision;
  cached.palette_revision = palette_revision;
  cached.texture_filter = static_cast<uint32_t>(batch.texture_filter);
  cached.has_mips = decoded.mips.size() > 1u;
  return cached.id;
}

bool upload_modifier_texture(
    const std::vector<WebModifierTriangle> &modifiers,
    GLint *texture_width) {
  if (modifiers.size() >
      enj_host_limits::translucent_modifier_triangles) {
    std::fprintf(stderr,
                 "web-enDjinn: translucent modifier triangle overflow "
                 "(%zu > %zu); frame rejected\n",
                 modifiers.size(),
                 enj_host_limits::translucent_modifier_triangles);
    return false;
  }
  GLint maximum_size = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximum_size);
  if (maximum_size <= 0) {
    return false;
  }
  const size_t texel_count = std::max<size_t>(1u, modifiers.size() * 4u);
  const size_t width = std::min<size_t>(
      static_cast<size_t>(maximum_size), std::max<size_t>(4u, texel_count));
  const size_t height = (texel_count + width - 1u) / width;
  if (height > static_cast<size_t>(maximum_size)) {
    std::fprintf(stderr,
                 "web-enDjinn: translucent modifier triangle overflow "
                 "(%zu events exceed a %dx%d data texture); frame rejected\n",
                 modifiers.size(), maximum_size, maximum_size);
    return false;
  }

  std::vector<float> pixels(width * height * 4u, 0.0f);
  for (size_t i = 0u; i < modifiers.size(); i++) {
    const WebModifierTriangle &event = modifiers[i];
    float *destination = pixels.data() + i * 16u;
    std::memcpy(destination, event.a.data(), sizeof(float) * 4u);
    std::memcpy(destination + 4u, event.b.data(), sizeof(float) * 4u);
    std::memcpy(destination + 8u, event.c.data(), sizeof(float) * 4u);
    std::memcpy(destination + 12u, event.state.data(), sizeof(float) * 4u);
  }

  glActiveTexture(GL_TEXTURE1);
  if (g_modifier_texture == 0u) {
    glGenTextures(1, &g_modifier_texture);
  }
  glBindTexture(GL_TEXTURE_2D, g_modifier_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, static_cast<GLsizei>(width),
               static_cast<GLsizei>(height), 0, GL_RGBA, GL_FLOAT,
               pixels.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glActiveTexture(GL_TEXTURE0);
  g_bound_texture = 0u;
  *texture_width = static_cast<GLint>(width);
  return true;
}

void set_draw_state(const DrawBatch &batch, DrawMode mode) {
  GenericDrawState desired{};
  desired.depth_test = batch.depth_test;
  desired.depth_write = batch.depth_write;
  desired.color_write = true;
  desired.punch_through = batch.alpha_cutout ||
                          batch.list == PVR_LIST_PT_POLY;
  if (mode == DrawMode::ModifierXor || mode == DrawMode::ModifierOr) {
    desired.depth_write = false;
    desired.color_write = false;
    desired.stencil_test = true;
    desired.stencil_func = GL_ALWAYS;
    desired.stencil_func_mask = 0x02u;
    desired.stencil_write_mask = 0x02u;
    desired.stencil_reference = mode == DrawMode::ModifierOr ? 0x02 : 0;
    desired.stencil_depth_pass =
        mode == DrawMode::ModifierOr ? GL_REPLACE : GL_INVERT;
  } else if (mode == DrawMode::ModifierInclude ||
             mode == DrawMode::ModifierExclude) {
    desired.depth_test = false;
    desired.depth_write = false;
    desired.color_write = false;
    desired.stencil_test = true;
    desired.stencil_func = GL_EQUAL;
    desired.stencil_func_mask = 0x02u;
    desired.stencil_write_mask = 0x01u;
    desired.stencil_reference =
        mode == DrawMode::ModifierExclude ? 0x02 : 0x03;
    desired.stencil_depth_pass =
        mode == DrawMode::ModifierExclude ? GL_ZERO : GL_REPLACE;
  } else if (mode == DrawMode::ModifierClearCurrent) {
    desired.depth_test = false;
    desired.depth_write = false;
    desired.color_write = false;
    desired.stencil_test = true;
    desired.stencil_func = GL_ALWAYS;
    desired.stencil_func_mask = 0x02u;
    desired.stencil_write_mask = 0x02u;
    desired.stencil_depth_pass = GL_ZERO;
  } else if (mode == DrawMode::OpaqueModifierReceiver) {
    desired.depth_write = false;
    desired.depth_func = GL_EQUAL;
    desired.stencil_test = true;
    desired.stencil_func = GL_EQUAL;
    desired.stencil_reference = 0x01;
    desired.stencil_func_mask = 0x01u;
    desired.stencil_write_mask = 0u;
  } else if (batch.list == PVR_LIST_TR_POLY) {
    desired.depth_write = false;
    desired.blend = true;
  }

  const bool invalid = !g_generic_draw_state_valid;
  if (invalid ||
      desired.color_write != g_generic_draw_state.color_write) {
    const GLboolean enabled = desired.color_write ? GL_TRUE : GL_FALSE;
    glColorMask(enabled, enabled, enabled, enabled);
  }
  if (invalid || desired.depth_test != g_generic_draw_state.depth_test) {
    desired.depth_test ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
  }
  if (desired.depth_test &&
      (invalid || !g_generic_draw_state.depth_test ||
       desired.depth_func != g_generic_draw_state.depth_func)) {
    glDepthFunc(desired.depth_func);
  }
  if (invalid || desired.depth_write != g_generic_draw_state.depth_write) {
    glDepthMask(desired.depth_write ? GL_TRUE : GL_FALSE);
  }
  if (invalid ||
      desired.stencil_test != g_generic_draw_state.stencil_test) {
    desired.stencil_test ? glEnable(GL_STENCIL_TEST)
                         : glDisable(GL_STENCIL_TEST);
  }
  if (desired.stencil_test &&
      (invalid || !g_generic_draw_state.stencil_test ||
       desired.stencil_func != g_generic_draw_state.stencil_func ||
       desired.stencil_reference !=
           g_generic_draw_state.stencil_reference ||
       desired.stencil_func_mask !=
           g_generic_draw_state.stencil_func_mask)) {
    glStencilFunc(desired.stencil_func, desired.stencil_reference,
                  desired.stencil_func_mask);
  }
  if (desired.stencil_test &&
      (invalid || !g_generic_draw_state.stencil_test ||
       desired.stencil_write_mask !=
           g_generic_draw_state.stencil_write_mask)) {
    glStencilMask(desired.stencil_write_mask);
  }
  if (desired.stencil_test &&
      (invalid || !g_generic_draw_state.stencil_test ||
       desired.stencil_depth_pass !=
           g_generic_draw_state.stencil_depth_pass)) {
    glStencilOp(GL_KEEP, GL_KEEP, desired.stencil_depth_pass);
  }
  if (invalid || desired.blend != g_generic_draw_state.blend) {
    desired.blend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
  }
  if (desired.blend &&
      (invalid || !g_generic_draw_state.blend ||
       desired.blend_source != g_generic_draw_state.blend_source ||
       desired.blend_destination !=
           g_generic_draw_state.blend_destination)) {
    glBlendFunc(desired.blend_source, desired.blend_destination);
  }
  if (invalid ||
      desired.punch_through != g_generic_draw_state.punch_through) {
    glUniform1i(g_punch_through, desired.punch_through);
  }
  g_generic_draw_state = desired;
  g_generic_draw_state_valid = true;
}

void draw_frame(const FrameDrawData &frame) {
  if (!g_ready) {
    return;
  }
  GLint modifier_texture_width = 0;
  if (!upload_modifier_texture(frame.translucent_modifiers,
                               &modifier_texture_width)) {
    return;
  }
  int width = 0;
  int height = 0;
#ifdef __EMSCRIPTEN__
  double css_width = 0.0;
  double css_height = 0.0;
  if (emscripten_get_element_css_size(
          "#canvas", &css_width, &css_height) == EMSCRIPTEN_RESULT_SUCCESS) {
    const double pixel_ratio = emscripten_get_device_pixel_ratio();
    const int requested_width =
        std::max(1, static_cast<int>(std::lround(css_width * pixel_ratio)));
    const int requested_height =
        std::max(1, static_cast<int>(std::lround(css_height * pixel_ratio)));
    int current_width = 0;
    int current_height = 0;
    (void)emscripten_get_canvas_element_size(
        "#canvas", &current_width, &current_height);
    if (current_width != requested_width || current_height != requested_height) {
      (void)emscripten_set_canvas_element_size(
          "#canvas", requested_width, requested_height);
    }
  }
  (void)emscripten_get_canvas_element_size("#canvas", &width, &height);
#else
  SDL_GL_GetDrawableSize(g_window, &width, &height);
#endif
  if (width <= 0 || height <= 0) {
    return;
  }
  int viewport_width =
      std::min(width, height * g_video_mode.width / g_video_mode.height);
  int viewport_height =
      std::min(height, width * g_video_mode.height / g_video_mode.width);
#if defined(ENJ_INTEGER_PRESENT_WIDTH) != defined(ENJ_INTEGER_PRESENT_HEIGHT)
#error "define both ENJ_INTEGER_PRESENT_WIDTH and ENJ_INTEGER_PRESENT_HEIGHT"
#elif defined(ENJ_INTEGER_PRESENT_WIDTH)
  static_assert(ENJ_INTEGER_PRESENT_WIDTH > 0,
                "integer presentation width must be positive");
  static_assert(ENJ_INTEGER_PRESENT_HEIGHT > 0,
                "integer presentation height must be positive");
  const int integer_scale = std::min(
      width / ENJ_INTEGER_PRESENT_WIDTH,
      height / ENJ_INTEGER_PRESENT_HEIGHT);
  if (integer_scale > 0) {
    viewport_width = ENJ_INTEGER_PRESENT_WIDTH * integer_scale;
    viewport_height = ENJ_INTEGER_PRESENT_HEIGHT * integer_scale;
  } else if (static_cast<int64_t>(width) * ENJ_INTEGER_PRESENT_HEIGHT >
             static_cast<int64_t>(height) * ENJ_INTEGER_PRESENT_WIDTH) {
    viewport_width = height * ENJ_INTEGER_PRESENT_WIDTH /
        ENJ_INTEGER_PRESENT_HEIGHT;
    viewport_height = height;
  } else {
    viewport_width = width;
    viewport_height = width * ENJ_INTEGER_PRESENT_HEIGHT /
        ENJ_INTEGER_PRESENT_WIDTH;
  }
#endif
  const int viewport_x = (width - viewport_width) / 2;
  const int viewport_y = (height - viewport_height) / 2;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(viewport_x, viewport_y, viewport_width, viewport_height);
  glDisable(GL_SCISSOR_TEST);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
#ifdef __EMSCRIPTEN__
  glClearDepthf(0.0f);
#else
  glClearDepth(0.0);
#endif
  glDepthMask(GL_TRUE);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glEnable(GL_SCISSOR_TEST);
  glScissor(viewport_x, viewport_y, viewport_width, viewport_height);
  glClearColor(g_bg_color[0], g_bg_color[1], g_bg_color[2], 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);

  if (!frame.vertices.empty()) {
    glUseProgram(g_program);
    glBindVertexArray(g_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(frame.vertices.size() *
                                         sizeof(WebVertex)),
                 frame.vertices.data(), GL_STREAM_DRAW);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(g_sampler, 0);
    glUniform1i(g_modifier_sampler, 1);
    glUniform1i(g_modifier_count,
                static_cast<GLint>(frame.translucent_modifiers.size()));
    glUniform1i(g_modifier_texture_width, modifier_texture_width);

    g_generic_draw_state_valid = false;
    int last_textured = -1;
    uint32_t opaque_volume_first_vertex = UINT32_MAX;
    for (const DrawBatch &batch : frame.batches) {
      if (batch.vertex_count == 0) {
        continue;
      }
      const bool opaque_modifier_volume =
          batch.modifier_volume && batch.list == PVR_LIST_OP_MOD;
      if (opaque_modifier_volume &&
          opaque_volume_first_vertex == UINT32_MAX) {
        opaque_volume_first_vertex = batch.first_vertex;
      }
      DrawMode mode = DrawMode::Standard;
      if (opaque_modifier_volume) {
        mode = !batch.modifier_volume_last &&
                       batch.modifier_mode != PVR_MODIFIER_OTHER_POLY
                   ? DrawMode::ModifierOr
                   : DrawMode::ModifierXor;
      } else if (batch.modifier && batch.list == PVR_LIST_OP_POLY) {
        mode = DrawMode::OpaqueModifierReceiver;
      }
      set_draw_state(batch, mode);
      const GLint modifier_area = batch.modifier_receiver &&
                                          batch.list == PVR_LIST_TR_POLY
                                      ? (batch.modifier ? 2 : 1)
                                      : 0;
      glUniform1i(g_modifier_area, modifier_area);
      glActiveTexture(GL_TEXTURE0);
      bind_texture_2d(texture_for(batch));
      if (last_textured != static_cast<int>(batch.textured)) {
        last_textured = static_cast<int>(batch.textured);
        glUniform1i(g_textured, last_textured);
      }
      glDrawArrays(GL_TRIANGLES, static_cast<GLint>(batch.first_vertex),
                   static_cast<GLsizei>(batch.vertex_count));

      if (opaque_modifier_volume && batch.modifier_volume_last) {
        const uint32_t volume_end = batch.first_vertex + batch.vertex_count;
        const uint32_t volume_vertex_count =
            opaque_volume_first_vertex == UINT32_MAX ||
                    volume_end < opaque_volume_first_vertex
                ? 0u
                : volume_end - opaque_volume_first_vertex;
        if (volume_vertex_count != 0u) {
          set_draw_state(
              batch,
              batch.modifier_mode == PVR_MODIFIER_EXCLUDE_LAST_POLY
                  ? DrawMode::ModifierExclude
                  : DrawMode::ModifierInclude);
          glDrawArrays(GL_TRIANGLES,
                       static_cast<GLint>(opaque_volume_first_vertex),
                       static_cast<GLsizei>(volume_vertex_count));
          set_draw_state(batch, DrawMode::ModifierClearCurrent);
          glDrawArrays(GL_TRIANGLES,
                       static_cast<GLint>(opaque_volume_first_vertex),
                       static_cast<GLsizei>(volume_vertex_count));
        }
        opaque_volume_first_vertex = UINT32_MAX;
      }
    }
  }
  glStencilMask(0xff);
  SDL_GL_SwapWindow(g_window);
}

bool create_context() {
  if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    std::fprintf(stderr, "web-enDjinn: SDL video init failed: %s\n",
                 SDL_GetError());
    return false;
  }
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  g_window = SDL_CreateWindow("web-enDjinn", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 640, 480,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (g_window == nullptr) {
    std::fprintf(stderr, "web-enDjinn: window creation failed: %s\n",
                 SDL_GetError());
    return false;
  }
  g_context = SDL_GL_CreateContext(g_window);
  if (g_context == nullptr || !create_program()) {
    std::fprintf(stderr, "web-enDjinn: WebGL context creation failed: %s\n",
                 SDL_GetError());
    return false;
  }
  glGenBuffers(1, &g_vertex_buffer);
  glGenVertexArrays(1, &g_vertex_array);
  glBindVertexArray(g_vertex_array);
  glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
  glEnableVertexAttribArray(static_cast<GLuint>(g_position));
  glEnableVertexAttribArray(static_cast<GLuint>(g_color));
  glEnableVertexAttribArray(static_cast<GLuint>(g_uv));
  glVertexAttribPointer(static_cast<GLuint>(g_position), 3, GL_FLOAT, GL_FALSE,
                        sizeof(WebVertex),
                        reinterpret_cast<void *>(offsetof(WebVertex, position)));
  glVertexAttribPointer(static_cast<GLuint>(g_color), 4, GL_UNSIGNED_BYTE,
                        GL_TRUE, sizeof(WebVertex),
                        reinterpret_cast<void *>(offsetof(WebVertex, color)));
  glVertexAttribPointer(static_cast<GLuint>(g_uv), 2, GL_FLOAT, GL_FALSE,
                        sizeof(WebVertex),
                        reinterpret_cast<void *>(offsetof(WebVertex, uv)));
  glGenTextures(1, &g_white_texture);
  bind_texture_2d(g_white_texture);
  const uint32_t white = 0xffffffffu;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, &white);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  g_ready = true;
  return true;
}

}  // namespace

#ifdef __EMSCRIPTEN__
extern "C" EMSCRIPTEN_KEEPALIVE void
web_endjinn_request_fullscreen(int resize_canvas) {
  SDL_SetWindowFullscreen(
      g_window, resize_canvas ? SDL_WINDOW_FULLSCREEN_DESKTOP
                              : SDL_WINDOW_FULLSCREEN);
}
#endif

namespace web_endjinn {

vid_mode_t *video_mode() { return &g_video_mode; }

uint64_t timer_ns_gettime64() {
  const uint64_t counter = SDL_GetPerformanceCounter();
  const uint64_t frequency = SDL_GetPerformanceFrequency();
  return frequency == 0
             ? 0
             : (counter / frequency) * 1000000000ull +
                   ((counter % frequency) * 1000000000ull) / frequency;
}

void vid_set_mode(vid_display_mode_generic_t, vid_pixel_mode_t) {
  g_video_mode = {640, 480};
}

void pvr_init(const pvr_init_params_t *params) {
  g_fsaa = params != nullptr && params->fsaa_enabled != 0;
  g_translucent_autosort =
      params == nullptr || params->autosort_disabled == 0;
  (void)create_context();
}

void pvr_shutdown() {
  if (g_context != nullptr) {
    for (const auto &[key, texture] : g_textures) {
      (void)key;
      glDeleteTextures(1, &texture.id);
    }
    glDeleteTextures(1, &g_white_texture);
    glDeleteTextures(1, &g_modifier_texture);
    glDeleteVertexArrays(1, &g_vertex_array);
    glDeleteBuffers(1, &g_vertex_buffer);
    glDeleteProgram(g_program);
  }
  g_textures.clear();
  g_ready = false;
  g_white_texture = 0;
  g_modifier_texture = 0;
  g_vertex_array = 0;
  g_vertex_buffer = 0;
  g_program = 0;
  g_bound_texture = 0;
  g_generic_draw_state_valid = false;
  if (g_context != nullptr) {
    SDL_GL_DeleteContext(g_context);
    g_context = nullptr;
  }
  if (g_window != nullptr) {
    SDL_DestroyWindow(g_window);
    g_window = nullptr;
  }
  enj_host_input_shutdown();
}

void pvr_set_bg_color(float r, float g, float b) {
  g_bg_color[0] = clamp01(r);
  g_bg_color[1] = clamp01(g);
  g_bg_color[2] = clamp01(b);
}

void pvr_scene_begin() {
  enj_host_pvr::scene_begin();
}
void pvr_scene_finish() {
  draw_frame(build_frame(g_video_mode, g_fsaa, g_translucent_autosort));
}

}  // namespace web_endjinn
