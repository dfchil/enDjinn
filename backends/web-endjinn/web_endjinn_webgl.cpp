#include "web_endjinn_webgl.h"

#include "../pc-endjinn/pc_endjinn_pvr.h"

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
#include <vector>

extern "C" void pc_endjinn_input_shutdown(void);

namespace {

using pc_endjinn_pvr::QueuedPrimitive;

struct WebVertex {
  float position[3];
  float color[4];
  float uv[2];
};

struct DrawBatch {
  pvr_list_t list{};
  uint32_t first_vertex{};
  uint32_t vertex_count{};
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

struct WebTexture {
  GLuint id{};
  uint64_t texture_revision{};
  uint64_t palette_revision{};
};

vid_mode_t g_video_mode{640, 480};
SDL_Window *g_window = nullptr;
SDL_GLContext g_context = nullptr;
GLuint g_program = 0;
GLuint g_vertex_buffer = 0;
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
          {((argb >> 16u) & 0xffu) / 255.0f,
           ((argb >> 8u) & 0xffu) / 255.0f,
           (argb & 0xffu) / 255.0f,
           ((argb >> 24u) & 0xffu) / 255.0f},
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

FrameDrawData build_frame() {
  const auto &queued = pc_endjinn_pvr::primitives();
  FrameDrawData frame;
  frame.vertices.reserve(queued.size() * 6u);

  const auto append_list = [&](pvr_list_t list, bool sort,
                               bool modifier_volume) {
    std::vector<const QueuedPrimitive *> primitives;
    for (const QueuedPrimitive &primitive : queued) {
      if (primitive.list == list &&
          primitive.modifier_volume == modifier_volume) {
        primitives.push_back(&primitive);
      }
    }
    if (sort) {
      std::stable_sort(primitives.begin(), primitives.end(),
                       [](const QueuedPrimitive *a,
                          const QueuedPrimitive *b) {
                         return average_z(*a) < average_z(*b);
                       });
    }
    for (const QueuedPrimitive *primitive : primitives) {
      const bool same = !frame.batches.empty() &&
                        frame.batches.back().list == list &&
                        frame.batches.back().textured == primitive->textured &&
                        frame.batches.back().texture == primitive->texture &&
                        frame.batches.back().texture_format ==
                            primitive->texture_format &&
                        frame.batches.back().texture_filter ==
                            primitive->texture_filter &&
                        frame.batches.back().modifier == primitive->modifier &&
                        frame.batches.back().modifier_volume ==
                            primitive->modifier_volume &&
                        frame.batches.back().modifier_mode ==
                            primitive->modifier_mode;
      if (!same) {
        frame.batches.push_back(
            {list,
             static_cast<uint32_t>(frame.vertices.size()),
             0u,
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
      const uint32_t before = static_cast<uint32_t>(frame.vertices.size());
      emit_primitive(frame.vertices, *primitive);
      frame.batches.back().vertex_count +=
          static_cast<uint32_t>(frame.vertices.size()) - before;
    }
  };

  append_list(PVR_LIST_OP_MOD, false, true);
  append_list(PVR_LIST_TR_MOD, false, true);
  append_list(PVR_LIST_OP_POLY, false, false);
  append_list(PVR_LIST_PT_POLY, false, false);
  append_list(PVR_LIST_TR_POLY, g_translucent_autosort, false);
  return frame;
}

uint64_t texture_key(const DrawBatch &batch) {
  return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(batch.texture)) |
         (static_cast<uint64_t>(batch.texture_format) << 32u);
}

GLuint texture_for(const DrawBatch &batch) {
  if (!batch.textured) {
    return g_white_texture;
  }
  const uint64_t key = texture_key(batch);
  WebTexture &cached = g_textures[key];
  const uint64_t texture_revision =
      pc_endjinn_pvr::texture_revision(batch.texture);
  const uint64_t palette_revision = pc_endjinn_pvr::palette_revision();
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
  glBindTexture(GL_TEXTURE_2D, cached.id);
  const auto palette = pc_endjinn_pvr::palette_rgba();
  for (size_t level = 0; level < decoded.mips.size(); level++) {
    const auto &mip = decoded.mips[level];
    if (decoded.indexed) {
      std::vector<uint32_t> rgba(mip.pixels.size());
      for (size_t i = 0; i < rgba.size(); i++) {
        const size_t color = decoded.palette_base + mip.pixels[i];
        rgba[i] = color < palette.size() ? palette[color] : 0xffffffffu;
      }
      glTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), GL_RGBA,
                   static_cast<GLsizei>(mip.width),
                   static_cast<GLsizei>(mip.height), 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, rgba.data());
    } else {
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
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDisable(GL_STENCIL_TEST);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_GEQUAL);
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  glUniform1i(g_punch_through, batch.list == PVR_LIST_PT_POLY);

  if (batch.modifier_volume) {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0xff);
    glStencilMask(0xff);
    glStencilOp(GL_KEEP, GL_KEEP,
                batch.modifier_mode == PVR_MODIFIER_EXCLUDE_LAST_POLY
                    ? GL_ZERO
                    : GL_REPLACE);
  } else if (batch.modifier) {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 1, 0xff);
    glStencilMask(0x00);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  } else if (batch.list == PVR_LIST_TR_POLY) {
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
  glViewport((width - viewport_width) / 2, (height - viewport_height) / 2,
             viewport_width, viewport_height);
  glClearColor(g_bg_color[0], g_bg_color[1], g_bg_color[2], 1.0f);
#ifdef __EMSCRIPTEN__
  glClearDepthf(0.0f);
#else
  glClearDepth(0.0);
#endif
  glDepthMask(GL_TRUE);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  if (frame.vertices.empty()) {
    SDL_GL_SwapWindow(g_window);
    return;
  }

  glUseProgram(g_program);
  glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(frame.vertices.size() *
                                       sizeof(WebVertex)),
               frame.vertices.data(), GL_STREAM_DRAW);
  glEnableVertexAttribArray(static_cast<GLuint>(g_position));
  glEnableVertexAttribArray(static_cast<GLuint>(g_color));
  glEnableVertexAttribArray(static_cast<GLuint>(g_uv));
  glVertexAttribPointer(static_cast<GLuint>(g_position), 3, GL_FLOAT, GL_FALSE,
                        sizeof(WebVertex),
                        reinterpret_cast<void *>(offsetof(WebVertex, position)));
  glVertexAttribPointer(static_cast<GLuint>(g_color), 4, GL_FLOAT, GL_FALSE,
                        sizeof(WebVertex),
                        reinterpret_cast<void *>(offsetof(WebVertex, color)));
  glVertexAttribPointer(static_cast<GLuint>(g_uv), 2, GL_FLOAT, GL_FALSE,
                        sizeof(WebVertex),
                        reinterpret_cast<void *>(offsetof(WebVertex, uv)));
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(g_sampler, 0);

  for (const DrawBatch &batch : frame.batches) {
    if (batch.vertex_count == 0) {
      continue;
    }
    set_draw_state(batch);
    glBindTexture(GL_TEXTURE_2D, texture_for(batch));
    glUniform1i(g_textured, batch.textured);
    glDrawArrays(GL_TRIANGLES, static_cast<GLint>(batch.first_vertex),
                 static_cast<GLsizei>(batch.vertex_count));
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
  glGenTextures(1, &g_white_texture);
  glBindTexture(GL_TEXTURE_2D, g_white_texture);
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
    glDeleteBuffers(1, &g_vertex_buffer);
    glDeleteProgram(g_program);
  }
  g_textures.clear();
  g_ready = false;
  g_white_texture = 0;
  g_vertex_buffer = 0;
  g_program = 0;
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

void pvr_scene_begin() { pc_endjinn_pvr::scene_begin(); }
void pvr_scene_finish() { draw_frame(build_frame()); }

}  // namespace web_endjinn
