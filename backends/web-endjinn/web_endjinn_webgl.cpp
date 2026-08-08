#include "web_endjinn_webgl.h"

#include "../pc-endjinn/pc_endjinn_pvr.h"
#include <enDjinn/enj_web_render.h>

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
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <utility>
#include <vector>

extern "C" void pc_endjinn_input_shutdown(void);

namespace {

using pc_endjinn_pvr::QueuedPrimitive;

struct WebVertex {
  float position[3];
  uint8_t color[4];
  float uv[2];
};

static_assert(sizeof(WebVertex) == 24u);

struct DrawBatch {
  pvr_list_t list{};
  uint32_t first_vertex{};
  uint32_t vertex_count{};
  bool depth_write{};
  bool textured{};
  pvr_ptr_t texture{};
  uint32_t texture_format{};
  uint32_t texture_width{};
  uint32_t texture_height{};
  pvr_filter_mode_t texture_filter{};
  bool modifier{};
  bool modifier_volume{};
  uint32_t modifier_mode{};
};

struct FrameDrawData {
  std::vector<WebVertex> vertices;
  std::vector<DrawBatch> batches;
};

struct SortablePrimitive {
  const QueuedPrimitive *primitive{};
  float depth{};
};

struct WebTexture {
  GLuint id{};
  uint64_t texture_revision{};
  uint64_t palette_revision{};
};

struct CustomRenderPass {
  enj_web_render_pass_phase_t phase{ENJ_WEB_RENDER_PASS_BACKGROUND};
  enj_web_render_pass_callback_t callback{};
  std::vector<uint8_t> data;
};

struct GenericDrawState {
  bool depth_test{};
  bool depth_write{};
  bool color_write{};
  bool stencil_test{};
  bool blend{};
  bool punch_through{};
  GLenum stencil_func{GL_ALWAYS};
  GLuint stencil_func_mask{0xffu};
  GLuint stencil_write_mask{0xffu};
  GLenum stencil_depth_pass{GL_KEEP};
  GLenum blend_source{GL_SRC_ALPHA};
  GLenum blend_destination{GL_ONE_MINUS_SRC_ALPHA};
};

vid_mode_t g_video_mode{640, 480};
SDL_Window *g_window = nullptr;
SDL_GLContext g_context = nullptr;
GLuint g_program = 0;
GLuint g_vertex_buffer = 0;
GLuint g_vertex_array = 0;
GLuint g_white_texture = 0;
GLint g_position = -1;
GLint g_color = -1;
GLint g_uv = -1;
GLint g_sampler = -1;
GLint g_textured = -1;
GLint g_punch_through = -1;
float g_bg_color[3]{};
bool g_fsaa = false;
bool g_translucent_autosort = true;
bool g_ready = false;
std::unordered_map<uint64_t, WebTexture> g_textures;
FrameDrawData g_frame;
std::array<std::vector<SortablePrimitive>, 5> g_frame_lists;
std::vector<CustomRenderPass> g_custom_render_passes;
GLuint g_bound_texture = 0;
GenericDrawState g_generic_draw_state;
bool g_generic_draw_state_valid = false;

constexpr const char *vertex_shader = R"(#version 300 es
in vec3 a_position;
in vec4 a_color;
in vec2 a_uv;
out vec4 v_color;
out vec2 v_uv;
void main() {
  gl_Position = vec4(a_position, 1.0);
  v_color = a_color;
  v_uv = a_uv;
}
)";

constexpr const char *fragment_shader = R"(#version 300 es
precision mediump float;
in vec4 v_color;
in vec2 v_uv;
uniform sampler2D u_texture;
uniform bool u_textured;
uniform bool u_punch_through;
out vec4 fragment_color;
void main() {
  vec4 color = (u_textured ? texture(u_texture, v_uv) : vec4(1.0)) * v_color;
  if (u_punch_through && color.a < 0.5) discard;
  fragment_color = color;
}
)";

float clamp01(float value) { return std::min(std::max(value, 0.0f), 1.0f); }

float pvr_depth(float z) { return clamp01(z * 0.25f); }

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
  return true;
}

WebVertex make_vertex(float x, float y, float z, uint32_t argb, float u,
                      float v) {
  const float half_width = g_video_mode.width * (g_fsaa ? 1.0f : 0.5f);
  const float half_height = g_video_mode.height * 0.5f;
  return {{x / half_width - 1.0f, 1.0f - y / half_height,
           pvr_depth(z)},
          {static_cast<uint8_t>((argb >> 16u) & 0xffu),
           static_cast<uint8_t>((argb >> 8u) & 0xffu),
           static_cast<uint8_t>(argb & 0xffu),
           static_cast<uint8_t>((argb >> 24u) & 0xffu)},
          {u, v}};
}

bool triangle_is_culled(const QueuedPrimitive &primitive, uint32_t a,
                        uint32_t b, uint32_t c) {
  if (primitive.culling != PVR_CULLING_CCW &&
      primitive.culling != PVR_CULLING_CW) {
    return false;
  }
  const float area =
      (primitive.x[b] - primitive.x[a]) *
          (primitive.y[c] - primitive.y[a]) -
      (primitive.y[b] - primitive.y[a]) *
          (primitive.x[c] - primitive.x[a]);
  return primitive.culling == PVR_CULLING_CCW ? area < 0.0f : area > 0.0f;
}

void emit_primitive(std::vector<WebVertex> &vertices,
                    const QueuedPrimitive &primitive) {
  const auto emit = [&](uint32_t index) {
    vertices.push_back(make_vertex(
        primitive.x[index], primitive.y[index], primitive.z[index],
        primitive.color[index], primitive.u[index], primitive.v[index]));
  };
  if (primitive.count == 3u) {
    if (!triangle_is_culled(primitive, 0u, 1u, 2u)) {
      emit(0u);
      emit(1u);
      emit(2u);
    }
  } else if (primitive.count == 4u) {
    if (!triangle_is_culled(primitive, 0u, 1u, 2u)) {
      emit(0u);
      emit(1u);
      emit(2u);
    }
    if (!triangle_is_culled(primitive, 0u, 2u, 3u)) {
      emit(0u);
      emit(2u);
      emit(3u);
    }
  }
}

float average_z(const QueuedPrimitive &primitive) {
  float total = 0.0f;
  for (uint32_t i = 0; i < primitive.count; i++) {
    total += primitive.z[i];
  }
  return primitive.count == 0 ? 0.0f : total / primitive.count;
}

const FrameDrawData &build_frame() {
  const auto &queued = pc_endjinn_pvr::primitives();
  g_frame.vertices.clear();
  g_frame.batches.clear();
  g_frame.vertices.reserve(queued.size() * 6u);
  for (auto &list : g_frame_lists) {
    list.clear();
  }

  for (const QueuedPrimitive &primitive : queued) {
    size_t list_index = g_frame_lists.size();
    if (primitive.modifier_volume) {
      if (primitive.list == PVR_LIST_OP_MOD) {
        list_index = 0u;
      } else if (primitive.list == PVR_LIST_TR_MOD) {
        list_index = 1u;
      }
    } else if (primitive.list == PVR_LIST_OP_POLY) {
      list_index = 2u;
    } else if (primitive.list == PVR_LIST_PT_POLY) {
      list_index = 3u;
    } else if (primitive.list == PVR_LIST_TR_POLY) {
      list_index = 4u;
    }
    if (list_index < g_frame_lists.size()) {
      g_frame_lists[list_index].push_back(
          {&primitive, list_index == 4u ? average_z(primitive) : 0.0f});
    }
  }
  if (g_translucent_autosort) {
    std::stable_sort(
        g_frame_lists[4].begin(), g_frame_lists[4].end(),
        [](const SortablePrimitive &a, const SortablePrimitive &b) {
          return a.depth < b.depth;
        });
  }

  const auto append_list = [&](pvr_list_t list, size_t list_index) {
    for (const SortablePrimitive &sortable : g_frame_lists[list_index]) {
      const QueuedPrimitive *primitive = sortable.primitive;
      const bool same = !g_frame.batches.empty() &&
                        g_frame.batches.back().list == list &&
                        g_frame.batches.back().depth_write ==
                            primitive->depth_write &&
                        g_frame.batches.back().textured == primitive->textured &&
                        g_frame.batches.back().texture == primitive->texture &&
                        g_frame.batches.back().texture_format ==
                            primitive->texture_format &&
                        g_frame.batches.back().texture_filter ==
                            primitive->texture_filter &&
                        g_frame.batches.back().modifier == primitive->modifier &&
                        g_frame.batches.back().modifier_volume ==
                            primitive->modifier_volume &&
                        g_frame.batches.back().modifier_mode ==
                            primitive->modifier_mode;
      if (!same) {
        g_frame.batches.push_back(
            {list,
             static_cast<uint32_t>(g_frame.vertices.size()),
             0u,
             primitive->depth_write,
             primitive->textured,
             primitive->texture,
             primitive->texture_format,
             primitive->texture_width,
             primitive->texture_height,
             primitive->texture_filter,
             primitive->modifier,
             primitive->modifier_volume,
             primitive->modifier_mode});
      }
      const uint32_t before =
          static_cast<uint32_t>(g_frame.vertices.size());
      emit_primitive(g_frame.vertices, *primitive);
      g_frame.batches.back().vertex_count +=
          static_cast<uint32_t>(g_frame.vertices.size()) - before;
    }
  };

  append_list(PVR_LIST_OP_MOD, 0u);
  append_list(PVR_LIST_TR_MOD, 1u);
  append_list(PVR_LIST_OP_POLY, 2u);
  append_list(PVR_LIST_PT_POLY, 3u);
  append_list(PVR_LIST_TR_POLY, 4u);
  return g_frame;
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
      pc_endjinn_pvr::texture_revision(batch.texture);
  const uint64_t palette_revision =
      pc_endjinn_pvr::palette_revision(batch.texture_format);
  if (cached.id != 0 && cached.texture_revision == texture_revision &&
      cached.palette_revision == palette_revision) {
    return cached.id;
  }

  QueuedPrimitive description{};
  description.textured = true;
  description.texture = batch.texture;
  description.texture_format = batch.texture_format;
  description.texture_width = batch.texture_width;
  description.texture_height = batch.texture_height;
  description.texture_filter = batch.texture_filter;
  pc_endjinn_pvr::DecodedTexture decoded;
  if (!pc_endjinn_pvr::decode_texture(description, decoded)) {
    return g_white_texture;
  }
  if (cached.id == 0) {
    glGenTextures(1, &cached.id);
  }
  bind_texture_2d(cached.id);
  if (decoded.indexed) {
    const auto palette = pc_endjinn_pvr::palette_rgba();
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
  return cached.id;
}

void set_draw_state(const DrawBatch &batch) {
  GenericDrawState desired{};
  desired.depth_test = true;
  desired.depth_write = batch.depth_write;
  desired.color_write = true;
  desired.punch_through = batch.list == PVR_LIST_PT_POLY;
  if (batch.modifier_volume) {
    desired.depth_test = false;
    desired.depth_write = false;
    desired.color_write = false;
    desired.stencil_test = true;
    desired.stencil_func = GL_ALWAYS;
    desired.stencil_func_mask = 0xffu;
    desired.stencil_write_mask = 0xffu;
    desired.stencil_depth_pass =
        batch.modifier_mode == PVR_MODIFIER_EXCLUDE_LAST_POLY
            ? GL_ZERO
            : GL_REPLACE;
  } else if (batch.modifier) {
    desired.depth_test = false;
    desired.depth_write = false;
    desired.stencil_test = true;
    desired.blend = true;
    desired.stencil_func = GL_EQUAL;
    desired.stencil_func_mask = 0xffu;
    desired.stencil_write_mask = 0x00u;
    desired.stencil_depth_pass = GL_KEEP;
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
  if (invalid) {
    glDepthFunc(GL_GEQUAL);
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
       desired.stencil_func_mask !=
           g_generic_draw_state.stencil_func_mask)) {
    glStencilFunc(desired.stencil_func, 1, desired.stencil_func_mask);
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

void run_custom_render_passes(
    enj_web_render_pass_phase_t phase,
    const enj_web_render_pass_context_t &context) {
  for (const CustomRenderPass &pass : g_custom_render_passes) {
    if (pass.phase != phase || pass.callback == nullptr) {
      continue;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(context.viewport_x, context.viewport_y, context.viewport_width,
               context.viewport_height);
    pass.callback(&context,
                  pass.data.empty() ? nullptr : pass.data.data());
  }
}

void draw_frame(const FrameDrawData &frame) {
  if (!g_ready) {
    return;
  }
  int width = 0;
  int height = 0;
#ifdef __EMSCRIPTEN__
  (void)emscripten_get_canvas_element_size("#canvas", &width, &height);
#else
  SDL_GL_GetDrawableSize(g_window, &width, &height);
#endif
  const int viewport_width =
      std::min(width, height * g_video_mode.width / g_video_mode.height);
  const int viewport_height =
      std::min(height, width * g_video_mode.height / g_video_mode.width);
  const int viewport_x = (width - viewport_width) / 2;
  const int viewport_y = (height - viewport_height) / 2;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(viewport_x, viewport_y, viewport_width, viewport_height);
  glClearColor(g_bg_color[0], g_bg_color[1], g_bg_color[2], 1.0f);
#ifdef __EMSCRIPTEN__
  glClearDepthf(0.0f);
#else
  glClearDepth(0.0);
#endif
  glDepthMask(GL_TRUE);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  const enj_web_render_pass_context_t pass_context = {
      viewport_x,
      viewport_y,
      viewport_width,
      viewport_height,
      g_video_mode.width * (g_fsaa ? 2 : 1),
      g_video_mode.height,
  };
  run_custom_render_passes(ENJ_WEB_RENDER_PASS_BACKGROUND, pass_context);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(viewport_x, viewport_y, viewport_width, viewport_height);
  g_bound_texture = 0;

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

    g_generic_draw_state_valid = false;
    int last_textured = -1;
    for (const DrawBatch &batch : frame.batches) {
      if (batch.vertex_count == 0) {
        continue;
      }
      set_draw_state(batch);
      bind_texture_2d(texture_for(batch));
      if (last_textured != static_cast<int>(batch.textured)) {
        last_textured = static_cast<int>(batch.textured);
        glUniform1i(g_textured, last_textured);
      }
      glDrawArrays(GL_TRIANGLES, static_cast<GLint>(batch.first_vertex),
                   static_cast<GLsizei>(batch.vertex_count));
    }
  }
  glStencilMask(0xff);
  run_custom_render_passes(ENJ_WEB_RENDER_PASS_FOREGROUND, pass_context);
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

extern "C" void
enj_web_render_pass_submit(enj_web_render_pass_callback_t callback,
                           const void *data, uint32_t data_size) {
  enj_web_render_pass_submit_at(ENJ_WEB_RENDER_PASS_BACKGROUND, callback, data,
                                data_size);
}

extern "C" void
enj_web_render_pass_submit_at(enj_web_render_pass_phase_t phase,
                              enj_web_render_pass_callback_t callback,
                              const void *data, uint32_t data_size) {
  if (callback == nullptr) {
    return;
  }
  CustomRenderPass pass{};
  pass.phase = phase;
  pass.callback = callback;
  if (data != nullptr && data_size != 0u) {
    const auto *bytes = static_cast<const uint8_t *>(data);
    pass.data.assign(bytes, bytes + data_size);
  }
  g_custom_render_passes.push_back(std::move(pass));
}

extern "C" void enj_web_texture_bind(const enj_web_texture_t *texture) {
  DrawBatch batch{};
  if (texture != nullptr) {
    batch.textured = texture->textured != 0u;
    batch.texture = texture->texture;
    batch.texture_format = texture->texture_format;
    batch.texture_width = texture->texture_width;
    batch.texture_height = texture->texture_height;
    batch.texture_filter =
        static_cast<pvr_filter_mode_t>(texture->texture_filter);
  }
  const GLuint resolved = texture_for(batch);
  glBindTexture(GL_TEXTURE_2D, resolved);
  g_bound_texture = resolved;
}

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
    glDeleteVertexArrays(1, &g_vertex_array);
    glDeleteBuffers(1, &g_vertex_buffer);
    glDeleteProgram(g_program);
  }
  g_textures.clear();
  g_custom_render_passes.clear();
  g_ready = false;
  g_white_texture = 0;
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
  pc_endjinn_input_shutdown();
}

void pvr_set_bg_color(float r, float g, float b) {
  g_bg_color[0] = clamp01(r);
  g_bg_color[1] = clamp01(g);
  g_bg_color[2] = clamp01(b);
}

void pvr_scene_begin() {
  g_custom_render_passes.clear();
  pc_endjinn_pvr::scene_begin();
}
void pvr_scene_finish() { draw_frame(build_frame()); }

}  // namespace web_endjinn
