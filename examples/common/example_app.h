/**
 * Shared boilerplate for Monstro example applications.
 *
 * Provides GLFW window creation, Vulkan/VulkEase rendering setup,
 * Monstro context initialization, input forwarding, and the main loop.
 *
 * Usage:
 *   1. Include this header.
 *   2. Define your example's state struct and three functions:
 *        void example_init(ExampleApp* app, void* user_data);
 *        void example_update(ExampleApp* app, void* user_data, float dt);
 *        void example_shutdown(ExampleApp* app, void* user_data);
 *   3. Call example_app_run() from main().
 */
#ifndef MONSTRO_EXAMPLE_APP_H
#define MONSTRO_EXAMPLE_APP_H

#include <GLFW/glfw3.h>
#include <vulkease.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#endif

#include <monstro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ExampleApp
{
   GLFWwindow *window;
   VEContext *context;
   VEDevice *device;
   VESwapchain *swapchain;
   VERenderTarget renderTarget;
   VETextureIndex colorTexture;

   VEShader *vertexShader;
   VEShader *fragmentShader;
   VEGraphicsPipeline *pipeline;
   VESamplerIndex sampler;
   VETextureIndex whiteTexture;

   VEBufferAddress vertexBuffer;
   VEBufferAddress indexBuffer;
   size_t vertexBufferCapacity;
   size_t indexBufferCapacity;

   MonstroContext *ui;
   MonstroFont *font;
   VETextureIndex fontAtlasTexture;
   uint64_t fontAtlasTextureId;

   void *user_data;
};

typedef void (*ExampleInitFn)(ExampleApp *app, void *user_data);
typedef void (*ExampleUpdateFn)(ExampleApp *app, void *user_data, float dt);
typedef void (*ExampleShutdownFn)(ExampleApp *app, void *user_data);

struct ExampleAppDesc
{
   const char *title;
   int width;
   int height;
   const char *font_path;
   float font_size;
   void *user_data;
   ExampleInitFn init;
   ExampleUpdateFn update;
   ExampleShutdownFn shutdown;
};

/* -- input mapping -------------------------------------------------------- */

static MonstroKey ea_glfw_key_to_monstro(int key)
{
   switch (key)
   {
   case GLFW_KEY_TAB:
      return MONSTRO_KEY_TAB;
   case GLFW_KEY_LEFT:
      return MONSTRO_KEY_LEFT_ARROW;
   case GLFW_KEY_RIGHT:
      return MONSTRO_KEY_RIGHT_ARROW;
   case GLFW_KEY_UP:
      return MONSTRO_KEY_UP_ARROW;
   case GLFW_KEY_DOWN:
      return MONSTRO_KEY_DOWN_ARROW;
   case GLFW_KEY_PAGE_UP:
      return MONSTRO_KEY_PAGE_UP;
   case GLFW_KEY_PAGE_DOWN:
      return MONSTRO_KEY_PAGE_DOWN;
   case GLFW_KEY_HOME:
      return MONSTRO_KEY_HOME;
   case GLFW_KEY_END:
      return MONSTRO_KEY_END;
   case GLFW_KEY_INSERT:
      return MONSTRO_KEY_INSERT;
   case GLFW_KEY_DELETE:
      return MONSTRO_KEY_DELETE;
   case GLFW_KEY_BACKSPACE:
      return MONSTRO_KEY_BACKSPACE;
   case GLFW_KEY_SPACE:
      return MONSTRO_KEY_SPACE;
   case GLFW_KEY_ENTER:
      return MONSTRO_KEY_ENTER;
   case GLFW_KEY_ESCAPE:
      return MONSTRO_KEY_ESCAPE;
   case GLFW_KEY_LEFT_CONTROL:
      return MONSTRO_KEY_LEFT_CTRL;
   case GLFW_KEY_RIGHT_CONTROL:
      return MONSTRO_KEY_RIGHT_CTRL;
   case GLFW_KEY_LEFT_SHIFT:
      return MONSTRO_KEY_LEFT_SHIFT;
   case GLFW_KEY_RIGHT_SHIFT:
      return MONSTRO_KEY_RIGHT_SHIFT;
   case GLFW_KEY_LEFT_ALT:
      return MONSTRO_KEY_LEFT_ALT;
   case GLFW_KEY_RIGHT_ALT:
      return MONSTRO_KEY_RIGHT_ALT;
   case GLFW_KEY_LEFT_SUPER:
      return MONSTRO_KEY_LEFT_SUPER;
   case GLFW_KEY_RIGHT_SUPER:
      return MONSTRO_KEY_RIGHT_SUPER;
   case GLFW_KEY_A:
      return MONSTRO_KEY_A;
   case GLFW_KEY_C:
      return MONSTRO_KEY_C;
   case GLFW_KEY_V:
      return MONSTRO_KEY_V;
   case GLFW_KEY_X:
      return MONSTRO_KEY_X;
   case GLFW_KEY_Y:
      return MONSTRO_KEY_Y;
   case GLFW_KEY_Z:
      return MONSTRO_KEY_Z;
   default:
      return MONSTRO_KEY_COUNT;
   }
}

/* -- GLFW callbacks ------------------------------------------------------- */

static void ea_error_cb(int error, const char *desc) { fprintf(stderr, "GLFW Error %d: %s\n", error, desc); }

static void ea_fb_size_cb(GLFWwindow *w, int width, int height)
{
   auto *app = (ExampleApp *)glfwGetWindowUserPointer(w);
   if (!app || width <= 0 || height <= 0)
      return;
   if (app->swapchain)
      veResizeSwapchain(app->swapchain, (uint32_t)width, (uint32_t)height);
   if (app->colorTexture != VE_INVALID_TEXTURE_INDEX)
   {
      veResizeRenderTarget(app->device, &app->renderTarget, (uint32_t)width, (uint32_t)height);
      app->colorTexture = app->renderTarget.attachments[0].texture;
   }
   if (app->ui)
      monstro_set_display_size(app->ui, (float)width, (float)height);
}

static void ea_key_cb(GLFWwindow *w, int key, int /*scancode*/, int action, int mods)
{
   auto *app = (ExampleApp *)glfwGetWindowUserPointer(w);
   if (!app || !app->ui)
      return;
   MonstroKey mk = ea_glfw_key_to_monstro(key);
   if (mk != MONSTRO_KEY_COUNT)
      monstro_set_key(app->ui, mk, action != GLFW_RELEASE);
   MonstroModFlags mf = MONSTRO_MOD_NONE;
   if (mods & GLFW_MOD_CONTROL)
      mf = (MonstroModFlags)(mf | MONSTRO_MOD_CTRL);
   if (mods & GLFW_MOD_SHIFT)
      mf = (MonstroModFlags)(mf | MONSTRO_MOD_SHIFT);
   if (mods & GLFW_MOD_ALT)
      mf = (MonstroModFlags)(mf | MONSTRO_MOD_ALT);
   if (mods & GLFW_MOD_SUPER)
      mf = (MonstroModFlags)(mf | MONSTRO_MOD_SUPER);
   monstro_set_key_mods(app->ui, mf);
}

static void ea_char_cb(GLFWwindow *w, unsigned int cp)
{
   auto *app = (ExampleApp *)glfwGetWindowUserPointer(w);
   if (!app || !app->ui)
      return;
   char u[5] = {};
   if (cp < 0x80)
   {
      u[0] = (char)cp;
   }
   else if (cp < 0x800)
   {
      u[0] = (char)(0xC0 | (cp >> 6));
      u[1] = (char)(0x80 | (cp & 0x3F));
   }
   else if (cp < 0x10000)
   {
      u[0] = (char)(0xE0 | (cp >> 12));
      u[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
      u[2] = (char)(0x80 | (cp & 0x3F));
   }
   else
   {
      u[0] = (char)(0xF0 | (cp >> 18));
      u[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
      u[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
      u[3] = (char)(0x80 | (cp & 0x3F));
   }
   monstro_add_input_text(app->ui, u);
}

static void ea_cursor_cb(GLFWwindow *w, double x, double y)
{
   auto *app = (ExampleApp *)glfwGetWindowUserPointer(w);
   if (app && app->ui)
      monstro_set_mouse_pos(app->ui, (float)x, (float)y);
}

static void ea_mouse_btn_cb(GLFWwindow *w, int button, int action, int /*mods*/)
{
   auto *app = (ExampleApp *)glfwGetWindowUserPointer(w);
   if (!app || !app->ui)
      return;
   MonstroMouseButton mb = MONSTRO_MOUSE_BUTTON_LEFT;
   if (button == GLFW_MOUSE_BUTTON_RIGHT)
      mb = MONSTRO_MOUSE_BUTTON_RIGHT;
   else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
      mb = MONSTRO_MOUSE_BUTTON_MIDDLE;
   monstro_set_mouse_button(app->ui, mb, action == GLFW_PRESS);
}

static void ea_scroll_cb(GLFWwindow *w, double xo, double yo)
{
   auto *app = (ExampleApp *)glfwGetWindowUserPointer(w);
   if (app && app->ui)
      monstro_set_mouse_wheel(app->ui, (float)xo, (float)yo);
}

/* -- rendering helpers ---------------------------------------------------- */

enum EATextureType : uint32_t
{
   EA_TEX_RGBA = 0,
   EA_TEX_FONT = 1,
   EA_TEX_NONE = 2
};

struct EAPushConstants
{
   uint64_t vertexBufferAddress;
   uint64_t indexBufferAddress;
   float screenWidth;
   float screenHeight;
   uint32_t textureIndex;
   uint32_t samplerIndex;
   uint32_t indexOffset;
   uint32_t textureType;
};

static bool ea_upload_buffers(ExampleApp *app, MonstroDrawBuffer *db)
{
   const MonstroVertex *verts = nullptr;
   size_t vbs = 0;
   monstro_draw_buffer_get_vertex_buffer(db, &verts, &vbs);
   const uint32_t *idx = nullptr;
   uint32_t ic = 0;
   monstro_draw_buffer_get_index_buffer(db, &idx, &ic);
   size_t ibs = ic * sizeof(uint32_t);
   if (vbs == 0 || ibs == 0)
      return true;

   if (vbs > app->vertexBufferCapacity)
   {
      if (app->vertexBuffer != VE_INVALID_ADDRESS)
         veDestroyBuffer(app->device, app->vertexBuffer);
      size_t nc = vbs * 2;
      VEBufferDesc d = {};
      d.size = nc;
      d.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
      d.debugName = "EAVertexBuf";
      if (veCreateBuffer(app->device, &d, &app->vertexBuffer) != VE_SUCCESS)
         return false;
      app->vertexBufferCapacity = nc;
   }
   if (ibs > app->indexBufferCapacity)
   {
      if (app->indexBuffer != VE_INVALID_ADDRESS)
         veDestroyBuffer(app->device, app->indexBuffer);
      size_t nc = ibs * 2;
      VEBufferDesc d = {};
      d.size = nc;
      d.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
      d.debugName = "EAIndexBuf";
      if (veCreateBuffer(app->device, &d, &app->indexBuffer) != VE_SUCCESS)
         return false;
      app->indexBufferCapacity = nc;
   }
   if (verts && vbs > 0)
      veUpdateBuffer(app->device, app->vertexBuffer, verts, vbs, 0);
   if (idx && ibs > 0)
      veUpdateBuffer(app->device, app->indexBuffer, idx, ibs, 0);
   return true;
}

static void ea_render_ui(ExampleApp *app, VECommandBuffer *cmd, uint32_t w, uint32_t h)
{
   const MonstroDrawCommand *dcs = nullptr;
   uint32_t dc_count = 0;
   monstro_context_get_draw_commands(app->ui, &dcs, &dc_count);
   if (dc_count == 0 || app->vertexBuffer == VE_INVALID_ADDRESS)
      return;
   veApplyGraphicsState(cmd, app->pipeline, nullptr, nullptr, nullptr);
   for (uint32_t i = 0; i < dc_count; i++)
   {
      const MonstroDrawCommand *dc = &dcs[i];
      if (dc->custom_draw)
      {
         float cr[4] = {dc->clip_rect[0], dc->clip_rect[1], dc->clip_rect[2], dc->clip_rect[3]};
         dc->custom_draw(dc->user_data, cr, dc->layer);
         continue;
      }
      int32_t sx = (int32_t)dc->clip_rect[0], sy = (int32_t)dc->clip_rect[1];
      uint32_t sw = (uint32_t)dc->clip_rect[2], sh = (uint32_t)dc->clip_rect[3];
      if (sx < 0)
      {
         sw = (sw > (uint32_t)(-sx)) ? sw + (uint32_t)sx : 0;
         sx = 0;
      }
      if (sy < 0)
      {
         sh = (sh > (uint32_t)(-sy)) ? sh + (uint32_t)sy : 0;
         sy = 0;
      }
      if ((uint32_t)sx + sw > w)
         sw = w - (uint32_t)sx;
      if ((uint32_t)sy + sh > h)
         sh = h - (uint32_t)sy;
      veSetScissor(cmd, sx, sy, sw, sh);

      VETextureIndex tex = app->whiteTexture;
      EATextureType tt = EA_TEX_NONE;
      if (dc->texture_id == app->fontAtlasTextureId && app->fontAtlasTextureId != 0)
      {
         tex = app->fontAtlasTexture;
         tt = EA_TEX_FONT;
      }
      else if (dc->texture_id != 0)
      {
         tex = (VETextureIndex)dc->texture_id;
         tt = EA_TEX_RGBA;
      }

      EAPushConstants pc = {};
      pc.vertexBufferAddress = app->vertexBuffer;
      pc.indexBufferAddress = app->indexBuffer;
      pc.screenWidth = (float)w;
      pc.screenHeight = (float)h;
      pc.textureIndex = tex;
      pc.samplerIndex = app->sampler;
      pc.indexOffset = dc->index_offset;
      pc.textureType = tt;
      vePushConstants(cmd, &pc, sizeof(pc), 0);
      veDraw(cmd, dc->index_count, 1, 0, 0);
   }
}

/* -- main entry point ----------------------------------------------------- */

static int example_app_run(const ExampleAppDesc &desc)
{
   ExampleApp app = {};
   app.fontAtlasTexture = VE_INVALID_TEXTURE_INDEX;
   app.vertexBuffer = VE_INVALID_ADDRESS;
   app.indexBuffer = VE_INVALID_ADDRESS;
   app.user_data = desc.user_data;

   glfwSetErrorCallback(ea_error_cb);
   if (!glfwInit())
   {
      fprintf(stderr, "Failed to init GLFW\n");
      return 1;
   }

   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
   int win_w = desc.width > 0 ? desc.width : 1280;
   int win_h = desc.height > 0 ? desc.height : 800;
   app.window = glfwCreateWindow(win_w, win_h, desc.title ? desc.title : "Monstro Example", nullptr, nullptr);
   if (!app.window)
   {
      glfwTerminate();
      return 1;
   }

   glfwSetWindowUserPointer(app.window, &app);
   glfwSetFramebufferSizeCallback(app.window, ea_fb_size_cb);
   glfwSetKeyCallback(app.window, ea_key_cb);
   glfwSetCharCallback(app.window, ea_char_cb);
   glfwSetCursorPosCallback(app.window, ea_cursor_cb);
   glfwSetMouseButtonCallback(app.window, ea_mouse_btn_cb);
   glfwSetScrollCallback(app.window, ea_scroll_cb);

   if (veCreateContext(desc.title, nullptr, 0, &app.context) != VE_SUCCESS)
   {
      glfwTerminate();
      return 1;
   }
   if (veCreateDevice(app.context, VK_NULL_HANDLE, nullptr, 0, &app.device) != VE_SUCCESS)
   {
      veDestroyContext(app.context);
      glfwTerminate();
      return 1;
   }

   VESwapchainDesc sd = {};
   sd.width = (uint32_t)win_w;
   sd.height = (uint32_t)win_h;
   sd.colorFormat = VK_FORMAT_B8G8R8A8_SRGB;
#ifdef DEBUG
   sd.vsync = true;
#endif
   sd.debugName = "ExampleSwapchain";
#if defined(_WIN32)
   sd.surface.type = VE_SURFACE_TYPE_WIN32;
   sd.surface.win32.hwnd = glfwGetWin32Window(app.window);
#elif defined(__linux__)
   sd.surface.type = VE_SURFACE_TYPE_XLIB;
   sd.surface.xlib.display = (void *)glfwGetX11Display();
   sd.surface.xlib.window = glfwGetX11Window(app.window);
#elif defined(__APPLE__)
   sd.surface.type = VE_SURFACE_TYPE_COCOA;
   sd.surface.cocoa.window = glfwGetCocoaWindow(app.window);
#endif
   if (veCreateSwapchain(app.device, &sd, &app.swapchain) != VE_SUCCESS)
   {
      veDestroyDevice(app.device);
      veDestroyContext(app.context);
      glfwTerminate();
      return 1;
   }

   int fw, fh;
   glfwGetFramebufferSize(app.window, &fw, &fh);
   VEColor cc = {0.1f, 0.1f, 0.12f, 1.0f};
   app.renderTarget =
       veCreateSimpleRenderTarget(app.device, (uint32_t)fw, (uint32_t)fh, veGetSwapchainFormat(app.swapchain),
                                  VK_FORMAT_UNDEFINED, cc, 1.0f, &app.colorTexture, nullptr);

   if (veLoadShaderFromFile(app.device, "data/shaders/ui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main", "EAVert",
                            &app.vertexShader) != VE_SUCCESS)
   {
      fprintf(stderr, "Failed to load vertex shader\n");
      return 1;
   }
   if (veLoadShaderFromFile(app.device, "data/shaders/ui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main", "EAFrag",
                            &app.fragmentShader) != VE_SUCCESS)
   {
      fprintf(stderr, "Failed to load fragment shader\n");
      return 1;
   }

   VEGraphicsPipelineDesc pd = veDefaultGraphicsPipelineDesc();
   pd.vertexShader = app.vertexShader;
   pd.fragmentShader = app.fragmentShader;
   pd.debugName = "EAPipeline";
   pd.vertexBindingCount = 0;
   pd.vertexAttributeCount = 0;
   pd.depthTestEnable = false;
   pd.depthWriteEnable = false;
   pd.cullMode = VK_CULL_MODE_NONE;
   pd.blendAttachments[0].blendEnable = true;
   pd.blendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
   pd.blendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
   pd.blendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;
   pd.blendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
   pd.blendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
   pd.blendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
   pd.blendAttachments[0].colorWriteMask =
       VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   pd.blendAttachmentCount = 1;
   if (veCreateGraphicsPipeline(app.device, &pd, &app.pipeline) != VE_SUCCESS)
   {
      fprintf(stderr, "Failed to create pipeline\n");
      return 1;
   }

   veCreateLinearSampler(app.device, &app.sampler);

   VkImageUsageFlags wtu = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT;
   veCreateTexture2D(app.device, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, wtu, "EAWhiteTex", &app.whiteTexture);
   uint32_t wp = 0xFFFFFFFF;
   veHostWriteTextureRegion(app.device, app.whiteTexture, &wp, sizeof(wp), nullptr);

   app.ui = monstro_context_create();
   glfwGetFramebufferSize(app.window, &fw, &fh);
   monstro_set_display_size(app.ui, (float)fw, (float)fh);

   const char *fp = desc.font_path ? desc.font_path : "data/fonts/freeSans.ttf";
   float fs = desc.font_size > 0 ? desc.font_size : 16.0f;
   MonstroFontManager *fm = monstro_context_get_font_manager(app.ui);
   MonstroTextureData ad = {};
   app.font = monstro_font_load_ttf_from_file(fm, fp, fs, 32, 96, &ad);
   if (app.font)
   {
      VkFormat af = (ad.format == MONSTRO_TEXTURE_FORMAT_R8) ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
      veCreateTexture2D(app.device, ad.width, ad.height, af, wtu, "EAFontAtlas", &app.fontAtlasTexture);
      veHostWriteTextureRegion(app.device, app.fontAtlasTexture, ad.data, ad.data_size, nullptr);
      app.fontAtlasTextureId = (uint64_t)app.fontAtlasTexture;
      monstro_font_set_atlas_texture(app.font, app.fontAtlasTextureId);
      monstro_font_add(fm, app.font, "default", fs);
      monstro_context_set_default_font(app.ui, app.font);
   }

   if (desc.init)
      desc.init(&app, desc.user_data);

   double lastTime = glfwGetTime();
   VEFrameTimingInfo timing = {};
   while (!glfwWindowShouldClose(app.window))
   {
      veBeginFrame(app.device);
      glfwPollEvents();
      double now = glfwGetTime();
      float dt = (float)(now - lastTime);
      lastTime = now;

      monstro_begin_frame(app.ui, dt);
      if (desc.update)
         desc.update(&app, desc.user_data, dt);
      monstro_end_frame(app.ui);

      MonstroDrawBuffer *db = monstro_context_get_draw_buffer(app.ui);
      ea_upload_buffers(&app, db);

      VECommandBuffer *cmd = nullptr;
      if (veBeginCommandBuffer(app.device, &cmd) == VE_SUCCESS && cmd)
      {
         glfwGetFramebufferSize(app.window, &fw, &fh);
         veBeginRendering(cmd, &app.renderTarget);
         ea_render_ui(&app, cmd, (uint32_t)fw, (uint32_t)fh);
         veEndRendering(cmd);
         veBlitTextureToSwapchain(cmd, app.colorTexture, app.swapchain, VK_FILTER_LINEAR);
         vePresentImage(app.swapchain, cmd, true);
      }
      veEndFrame(app.device, &timing);

      if (timing.frameNumber % 1000 == 0)
      {
         printf("Frame %llu: %.2f ms (%.1f FPS)\n", (unsigned long long)timing.frameNumber, timing.frameTimeMs,
                timing.fps);
      }
   }

   veDeviceWaitIdle(app.device);
   if (desc.shutdown)
      desc.shutdown(&app, desc.user_data);

   if (app.ui)
      monstro_context_destroy(app.ui);
   if (app.vertexBuffer != VE_INVALID_ADDRESS)
      veDestroyBuffer(app.device, app.vertexBuffer);
   if (app.indexBuffer != VE_INVALID_ADDRESS)
      veDestroyBuffer(app.device, app.indexBuffer);
   if (app.whiteTexture != VE_INVALID_TEXTURE_INDEX)
      veDestroyTexture(app.device, app.whiteTexture);
   if (app.fontAtlasTexture != VE_INVALID_TEXTURE_INDEX)
      veDestroyTexture(app.device, app.fontAtlasTexture);
   if (app.pipeline)
      veDestroyGraphicsPipeline(app.pipeline);
   if (app.vertexShader)
      veDestroyShader(app.vertexShader);
   if (app.fragmentShader)
      veDestroyShader(app.fragmentShader);
   if (app.swapchain)
      veDestroySwapchain(app.swapchain);
   if (app.device)
      veDestroyDevice(app.device);
   if (app.context)
      veDestroyContext(app.context);
   if (app.window)
      glfwDestroyWindow(app.window);
   glfwTerminate();
   return 0;
}

#endif
