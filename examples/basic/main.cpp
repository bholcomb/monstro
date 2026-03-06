/**
 * Monstro UI Basic Example
 * Demonstrates basic Monstro UI setup with GLFW window and VulkEase rendering
 * 
 * Based on VulkEase examples (01_triangle, 03_cube)
 */

#include <vulkease.h>
#include <GLFW/glfw3.h>

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
#include <monstro/font.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

// Shader paths (compiled SPIR-V)
#define UI_VERTEX_SHADER_PATH "data/basic/shaders/ui.vert.spv"
#define UI_FRAGMENT_SHADER_PATH "data/basic/shaders/ui.frag.spv"

// Window settings
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE "Monstro UI Example"

#ifdef DEBUG
static bool g_vsync = true;
#else
static bool g_vsync = false;
#endif

// Application state
struct App {
    GLFWwindow* window;
    VEContext* context;
    VEDevice* device;
    VESwapchain* swapchain;
    
    // Render target
    VERenderTarget renderTarget;
    VETextureIndex colorTexture;
    
    // UI Renderer resources
    VEShader* vertexShader;
    VEShader* fragmentShader;
    VEGraphicsPipeline* pipeline;
    VESamplerIndex sampler;
    VETextureIndex whiteTexture;
    
    // Dynamic buffers (recreated as needed)
    VEBufferAddress vertexBuffer;
    VEBufferAddress indexBuffer;
    size_t vertexBufferCapacity;
    size_t indexBufferCapacity;
    
    // Monstro UI
    MonstroContext* ui;
    
    // Font resources
    MonstroFont* font;
    VETextureIndex fontAtlasTexture;
    uint64_t fontAtlasTextureId;  // Monstro texture ID (mapped to fontAtlasTexture)
    
    // Demo state
    int buttonClicks;
    bool checkboxValue;
    float sliderValue;
};

static App* g_app = nullptr;

// GLFW key to Monstro key mapping
static MonstroKey glfw_key_to_monstro(int key) {
    switch (key) {
        case GLFW_KEY_TAB: return MONSTRO_KEY_TAB;
        case GLFW_KEY_LEFT: return MONSTRO_KEY_LEFT_ARROW;
        case GLFW_KEY_RIGHT: return MONSTRO_KEY_RIGHT_ARROW;
        case GLFW_KEY_UP: return MONSTRO_KEY_UP_ARROW;
        case GLFW_KEY_DOWN: return MONSTRO_KEY_DOWN_ARROW;
        case GLFW_KEY_PAGE_UP: return MONSTRO_KEY_PAGE_UP;
        case GLFW_KEY_PAGE_DOWN: return MONSTRO_KEY_PAGE_DOWN;
        case GLFW_KEY_HOME: return MONSTRO_KEY_HOME;
        case GLFW_KEY_END: return MONSTRO_KEY_END;
        case GLFW_KEY_INSERT: return MONSTRO_KEY_INSERT;
        case GLFW_KEY_DELETE: return MONSTRO_KEY_DELETE;
        case GLFW_KEY_BACKSPACE: return MONSTRO_KEY_BACKSPACE;
        case GLFW_KEY_SPACE: return MONSTRO_KEY_SPACE;
        case GLFW_KEY_ENTER: return MONSTRO_KEY_ENTER;
        case GLFW_KEY_ESCAPE: return MONSTRO_KEY_ESCAPE;
        case GLFW_KEY_LEFT_CONTROL: return MONSTRO_KEY_LEFT_CTRL;
        case GLFW_KEY_RIGHT_CONTROL: return MONSTRO_KEY_RIGHT_CTRL;
        case GLFW_KEY_LEFT_SHIFT: return MONSTRO_KEY_LEFT_SHIFT;
        case GLFW_KEY_RIGHT_SHIFT: return MONSTRO_KEY_RIGHT_SHIFT;
        case GLFW_KEY_LEFT_ALT: return MONSTRO_KEY_LEFT_ALT;
        case GLFW_KEY_RIGHT_ALT: return MONSTRO_KEY_RIGHT_ALT;
        case GLFW_KEY_LEFT_SUPER: return MONSTRO_KEY_LEFT_SUPER;
        case GLFW_KEY_RIGHT_SUPER: return MONSTRO_KEY_RIGHT_SUPER;
        case GLFW_KEY_A: return MONSTRO_KEY_A;
        case GLFW_KEY_C: return MONSTRO_KEY_C;
        case GLFW_KEY_V: return MONSTRO_KEY_V;
        case GLFW_KEY_X: return MONSTRO_KEY_X;
        case GLFW_KEY_Y: return MONSTRO_KEY_Y;
        case GLFW_KEY_Z: return MONSTRO_KEY_Z;
        default: return MONSTRO_KEY_COUNT;
    }
}

// GLFW Callbacks
static void errorCallback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    App* app = (App*)glfwGetWindowUserPointer(window);
    if (app && width > 0 && height > 0) {
        if (app->swapchain) {
            veResizeSwapchain(app->swapchain, (uint32_t)width, (uint32_t)height);
        }
        if (app->colorTexture != VE_INVALID_TEXTURE_INDEX) {
            veResizeRenderTarget(app->device, &app->renderTarget, (uint32_t)width, (uint32_t)height);
            app->colorTexture = app->renderTarget.attachments[0].texture;
        }
    }
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    App* app = (App*)glfwGetWindowUserPointer(window);
    if (!app || !app->ui) return;
    
    MonstroKey mkey = glfw_key_to_monstro(key);
    if (mkey != MONSTRO_KEY_COUNT) {
        monstro_set_key(app->ui, mkey, action != GLFW_RELEASE);
    }
    
    MonstroModFlags mod_flags = MONSTRO_MOD_NONE;
    if (mods & GLFW_MOD_CONTROL) mod_flags = (MonstroModFlags)(mod_flags | MONSTRO_MOD_CTRL);
    if (mods & GLFW_MOD_SHIFT) mod_flags = (MonstroModFlags)(mod_flags | MONSTRO_MOD_SHIFT);
    if (mods & GLFW_MOD_ALT) mod_flags = (MonstroModFlags)(mod_flags | MONSTRO_MOD_ALT);
    if (mods & GLFW_MOD_SUPER) mod_flags = (MonstroModFlags)(mod_flags | MONSTRO_MOD_SUPER);
    monstro_set_key_mods(app->ui, mod_flags);
}

static void charCallback(GLFWwindow* window, unsigned int codepoint) {
    App* app = (App*)glfwGetWindowUserPointer(window);
    if (!app || !app->ui) return;
    
    char utf8[5] = {0};
    if (codepoint < 0x80) {
        utf8[0] = (char)codepoint;
    } else if (codepoint < 0x800) {
        utf8[0] = (char)(0xC0 | (codepoint >> 6));
        utf8[1] = (char)(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000) {
        utf8[0] = (char)(0xE0 | (codepoint >> 12));
        utf8[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        utf8[2] = (char)(0x80 | (codepoint & 0x3F));
    } else {
        utf8[0] = (char)(0xF0 | (codepoint >> 18));
        utf8[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        utf8[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        utf8[3] = (char)(0x80 | (codepoint & 0x3F));
    }
    
    monstro_add_input_text(app->ui, utf8);
}

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    App* app = (App*)glfwGetWindowUserPointer(window);
    if (!app || !app->ui) return;
    monstro_set_mouse_pos(app->ui, (float)xpos, (float)ypos);
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    (void)mods;
    App* app = (App*)glfwGetWindowUserPointer(window);
    if (!app || !app->ui) return;
    
    MonstroMouseButton mbutton = MONSTRO_MOUSE_BUTTON_LEFT;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) mbutton = MONSTRO_MOUSE_BUTTON_RIGHT;
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE) mbutton = MONSTRO_MOUSE_BUTTON_MIDDLE;
    
    monstro_set_mouse_button(app->ui, mbutton, action == GLFW_PRESS);
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    App* app = (App*)glfwGetWindowUserPointer(window);
    if (!app || !app->ui) return;
    monstro_set_mouse_wheel(app->ui, (float)xoffset, (float)yoffset);
}

// Initialize GLFW and create window
static bool initWindow(App* app) {
    glfwSetErrorCallback(errorCallback);
    
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    app->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (!app->window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }
    
    glfwSetWindowUserPointer(app->window, app);
    glfwSetFramebufferSizeCallback(app->window, framebufferSizeCallback);
    glfwSetKeyCallback(app->window, keyCallback);
    glfwSetCharCallback(app->window, charCallback);
    glfwSetCursorPosCallback(app->window, cursorPosCallback);
    glfwSetMouseButtonCallback(app->window, mouseButtonCallback);
    glfwSetScrollCallback(app->window, scrollCallback);
    
    return true;
}

// Initialize VulkEase
static bool initVulkEase(App* app) {
    // Create context
    if (veCreateContext("Monstro UI Example", nullptr, 0, &app->context) != VE_SUCCESS || !app->context) {
        fprintf(stderr, "Failed to create VulkEase context\n");
        return false;
    }
    
    // Create device
    if (veCreateDevice(app->context, VK_NULL_HANDLE, nullptr, 0, &app->device) != VE_SUCCESS || !app->device) {
        fprintf(stderr, "Failed to create VulkEase device\n");
        return false;
    }
    
    printf("VulkEase initialized\n");
    printf("Device: %s\n", veGetDeviceName(app->device));
    
    // Create swapchain
    VESwapchainDesc swapchainDesc = {};
    swapchainDesc.width = WINDOW_WIDTH;
    swapchainDesc.height = WINDOW_HEIGHT;
    swapchainDesc.colorFormat = VK_FORMAT_B8G8R8A8_SRGB;
    swapchainDesc.vsync = g_vsync;
    swapchainDesc.debugName = "MonstroSwapchain";

#if defined(_WIN32)
    swapchainDesc.surface.type = VE_SURFACE_TYPE_WIN32;
    swapchainDesc.surface.win32.hwnd = glfwGetWin32Window(app->window);
#elif defined(__linux__)
    swapchainDesc.surface.type = VE_SURFACE_TYPE_XLIB;
    swapchainDesc.surface.xlib.display = (void*)glfwGetX11Display();
    swapchainDesc.surface.xlib.window = glfwGetX11Window(app->window);
#elif defined(__APPLE__)
    swapchainDesc.surface.type = VE_SURFACE_TYPE_COCOA;
    swapchainDesc.surface.cocoa.window = glfwGetCocoaWindow(app->window);
#endif

    if (veCreateSwapchain(app->device, &swapchainDesc, &app->swapchain) != VE_SUCCESS || !app->swapchain) {
        fprintf(stderr, "Failed to create swapchain\n");
        return false;
    }
    
    // Create render target (no depth buffer for UI)
    int width, height;
    glfwGetFramebufferSize(app->window, &width, &height);
    
    VEColor clearColor = {0.1f, 0.1f, 0.12f, 1.0f};
    app->renderTarget = veCreateSimpleRenderTarget(app->device,
                                                    (uint32_t)width, (uint32_t)height,
                                                    veGetSwapchainFormat(app->swapchain),
                                                    VK_FORMAT_UNDEFINED,  // No depth buffer
                                                    clearColor, 1.0f,
                                                    &app->colorTexture, nullptr);
    
    if (app->colorTexture == VE_INVALID_TEXTURE_INDEX) {
        fprintf(stderr, "Failed to create render target\n");
        return false;
    }
    
    printf("Render target created: %dx%d\n", width, height);
    return true;
}

// Initialize UI renderer resources
static bool initUIRenderer(App* app) {
    // Load shaders
    if (veLoadShaderFromFile(app->device, UI_VERTEX_SHADER_PATH, VK_SHADER_STAGE_VERTEX_BIT, "main",
                             "MonstroUIVertex", &app->vertexShader) != VE_SUCCESS || !app->vertexShader) {
        fprintf(stderr, "Failed to load UI vertex shader\n");
        return false;
    }
    
    if (veLoadShaderFromFile(app->device, UI_FRAGMENT_SHADER_PATH, VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                             "MonstroUIFragment", &app->fragmentShader) != VE_SUCCESS || !app->fragmentShader) {
        fprintf(stderr, "Failed to load UI fragment shader\n");
        return false;
    }
    
    // Create pipeline with alpha blending for UI
    VEGraphicsPipelineDesc pipelineDesc = veDefaultGraphicsPipelineDesc();
    pipelineDesc.vertexShader = app->vertexShader;
    pipelineDesc.fragmentShader = app->fragmentShader;
    pipelineDesc.debugName = "MonstroUIPipeline";
    
    // UI uses no vertex input (bindless via buffer device address)
    pipelineDesc.vertexBindingCount = 0;
    pipelineDesc.vertexBindings = nullptr;
    pipelineDesc.vertexAttributeCount = 0;
    pipelineDesc.vertexAttributes = nullptr;
    
    // Disable depth for UI
    pipelineDesc.depthTestEnable = false;
    pipelineDesc.depthWriteEnable = false;
    
    // No backface culling
    pipelineDesc.cullMode = VK_CULL_MODE_NONE;
    
    // Enable alpha blending
    pipelineDesc.blendAttachments[0].blendEnable = true;
    pipelineDesc.blendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipelineDesc.blendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineDesc.blendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;
    pipelineDesc.blendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pipelineDesc.blendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineDesc.blendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
    pipelineDesc.blendAttachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                       VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pipelineDesc.blendAttachmentCount = 1;
    
    if (veCreateGraphicsPipeline(app->device, &pipelineDesc, &app->pipeline) != VE_SUCCESS || !app->pipeline) {
        fprintf(stderr, "Failed to create UI pipeline\n");
        return false;
    }
    
    // Create linear sampler
    if (veCreateLinearSampler(app->device, &app->sampler) != VE_SUCCESS || app->sampler == VE_INVALID_SAMPLER_INDEX) {
        fprintf(stderr, "Failed to create sampler\n");
        return false;
    }
    
    // Create white texture (1x1 white pixel) as default
    // Need VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT for veHostWriteTextureRegion
    VkImageUsageFlags whiteTexUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT;
    if (veCreateTexture2D(app->device, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, whiteTexUsage, 
                          "MonstroWhiteTexture", &app->whiteTexture) != VE_SUCCESS ||
        app->whiteTexture == VE_INVALID_TEXTURE_INDEX) {
        fprintf(stderr, "Failed to create white texture\n");
        return false;
    }
    
    uint32_t whitePixel = 0xFFFFFFFF;
    VEResult texWriteResult = veHostWriteTextureRegion(app->device, app->whiteTexture, &whitePixel, sizeof(whitePixel), nullptr);
    if (texWriteResult != VE_SUCCESS) {
        fprintf(stderr, "Warning: veHostWriteTextureRegion failed with code %d\n", texWriteResult);
    }
    
    // Initialize buffer addresses to invalid
    app->vertexBuffer = VE_INVALID_ADDRESS;
    app->indexBuffer = VE_INVALID_ADDRESS;
    app->vertexBufferCapacity = 0;
    app->indexBufferCapacity = 0;
    
    printf("UI renderer initialized\n");
    return true;
}

// Upload UI buffers to GPU
static bool uploadUIBuffers(App* app, MonstroCanvas* canvas) {
    const MonstroVertex* vertices = nullptr;
    size_t vertexByteSize = 0;
    monstro_canvas_get_vertex_buffer(canvas, &vertices, &vertexByteSize);
    
    const uint32_t* indices = nullptr;
    uint32_t indexCount = 0;
    monstro_canvas_get_index_buffer(canvas, &indices, &indexCount);
    size_t indexByteSize = indexCount * sizeof(uint32_t);
    
    if (vertexByteSize == 0 || indexByteSize == 0) {
        return true;
    }
    
    // Recreate vertex buffer if capacity exceeded
    if (vertexByteSize > app->vertexBufferCapacity) {
        if (app->vertexBuffer != VE_INVALID_ADDRESS) {
            veDestroyBuffer(app->device, app->vertexBuffer);
        }
        
        size_t newCapacity = vertexByteSize * 2;  // Double for growth
        VEBufferDesc bufDesc = {};
        bufDesc.size = newCapacity;
        bufDesc.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        bufDesc.persistentlyMapped = false;
        bufDesc.debugName = "MonstroVertexBuffer";
        
        if (veCreateBuffer(app->device, &bufDesc, &app->vertexBuffer) != VE_SUCCESS ||
            app->vertexBuffer == VE_INVALID_ADDRESS) {
            fprintf(stderr, "Failed to create vertex buffer\n");
            return false;
        }
        app->vertexBufferCapacity = newCapacity;
    }
    
    // Recreate index buffer if capacity exceeded
    if (indexByteSize > app->indexBufferCapacity) {
        if (app->indexBuffer != VE_INVALID_ADDRESS) {
            veDestroyBuffer(app->device, app->indexBuffer);
        }
        
        size_t newCapacity = indexByteSize * 2;
        VEBufferDesc bufDesc = {};
        bufDesc.size = newCapacity;
        bufDesc.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        bufDesc.persistentlyMapped = false;
        bufDesc.debugName = "MonstroIndexBuffer";
        
        if (veCreateBuffer(app->device, &bufDesc, &app->indexBuffer) != VE_SUCCESS ||
            app->indexBuffer == VE_INVALID_ADDRESS) {
            fprintf(stderr, "Failed to create index buffer\n");
            return false;
        }
        app->indexBufferCapacity = newCapacity;
    }
    
    // Upload data
    if (vertices && vertexByteSize > 0) {
        veUpdateBuffer(app->device, app->vertexBuffer, vertices, vertexByteSize, 0);
    }
    if (indices && indexByteSize > 0) {
        veUpdateBuffer(app->device, app->indexBuffer, indices, indexByteSize, 0);
    }
    
    return true;
}

// Texture type flags for shader
enum UITextureType : uint32_t {
    UI_TEXTURE_RGBA = 0,      // Standard RGBA texture (multiply with vertex color)
    UI_TEXTURE_FONT_ATLAS = 1 // R8 font atlas (R channel is alpha, use vertex color for RGB)
};

// Push constants structure matching shader
struct UIPushConstants {
    uint64_t vertexBufferAddress;
    uint64_t indexBufferAddress;
    float screenWidth;
    float screenHeight;
    uint32_t textureIndex;
    uint32_t samplerIndex;
    uint32_t indexOffset;
    uint32_t textureType;  // UITextureType
};

// Render UI draw commands
static void renderUI(App* app, VECommandBuffer* cmd, MonstroCanvas* canvas, uint32_t width, uint32_t height) {
    const MonstroDrawCommand* drawCmds = nullptr;
    uint32_t drawCount = 0;
    monstro_canvas_get_draw_commands(canvas, &drawCmds, &drawCount);
    
    if (drawCount == 0 || app->vertexBuffer == VE_INVALID_ADDRESS) {
        return;
    }
    
    // Apply pipeline
    veApplyGraphicsState(cmd, app->pipeline, nullptr, nullptr, nullptr);
    
    // Process each draw command
    for (uint32_t i = 0; i < drawCount; i++) {
        const MonstroDrawCommand* dc = &drawCmds[i];
        
        // Handle custom draw callbacks
        if (dc->custom_draw) {
            float clip[4] = {dc->clip_rect[0], dc->clip_rect[1], dc->clip_rect[2], dc->clip_rect[3]};
            dc->custom_draw(dc->user_data, clip, dc->layer);
            continue;
        }
        
        // Set scissor rect from clip rect
        int32_t sx = (int32_t)dc->clip_rect[0];
        int32_t sy = (int32_t)dc->clip_rect[1];
        uint32_t sw = (uint32_t)dc->clip_rect[2];
        uint32_t sh = (uint32_t)dc->clip_rect[3];
        
        // Clamp to screen bounds
        if (sx < 0) { sw = (sw > (uint32_t)(-sx)) ? sw + sx : 0; sx = 0; }
        if (sy < 0) { sh = (sh > (uint32_t)(-sy)) ? sh + sy : 0; sy = 0; }
        if ((uint32_t)sx + sw > width) { sw = width - sx; }
        if ((uint32_t)sy + sh > height) { sh = height - sy; }
        
        veSetScissor(cmd, sx, sy, sw, sh);
        
        // Look up texture - map Monstro texture_id to VulkEase texture index
        VETextureIndex texture = app->whiteTexture;
        UITextureType texType = UI_TEXTURE_RGBA;
        
        if (dc->texture_id == app->fontAtlasTextureId && app->fontAtlasTextureId != 0) {
            texture = app->fontAtlasTexture;
            texType = UI_TEXTURE_FONT_ATLAS;
        }
        
        // Set push constants
        UIPushConstants pc = {};
        pc.vertexBufferAddress = app->vertexBuffer;
        pc.indexBufferAddress = app->indexBuffer;
        pc.screenWidth = (float)width;
        pc.screenHeight = (float)height;
        pc.textureIndex = texture;
        pc.samplerIndex = app->sampler;
        pc.indexOffset = dc->index_offset;
        pc.textureType = texType;
        
        vePushConstants(cmd, &pc, sizeof(pc), 0);
        
        // Draw (using vertex index from shader gl_VertexIndex)
        veDraw(cmd, dc->index_count, 1, 0, 0);
    }
}

// Update UI state
static void updateUI(App* app, float deltaTime, int width, int height) {
    MonstroContext* ctx = app->ui;
    
    monstro_begin_frame(ctx, (float)width, (float)height, deltaTime);
    
    monstro_set_next_window_pos(ctx, 50.0f, 50.0f, MONSTRO_COND_FIRST_USE_EVER);
    monstro_set_next_window_size(ctx, 350.0f, 350.0f, MONSTRO_COND_FIRST_USE_EVER);
    
    if (monstro_begin_window(ctx, "Monstro Demo", nullptr, MONSTRO_WINDOW_DEFAULT)) {
        monstro_label(ctx, "Hello, Monstro!");
        
        monstro_separator(ctx);
        
        if (monstro_button(ctx, "Click Me!", "Click this button")) {
            app->buttonClicks++;
            printf("Button clicked! Total: %d\n", app->buttonClicks);
        }
        
        if (monstro_checkbox(ctx, "Enable Feature", &app->checkboxValue, "Toggle this feature")) {
            printf("Checkbox toggled: %d\n", app->checkboxValue);
        }
        
        monstro_separator_text(ctx, "Layout Test");
        
        // Test horizontal layout - buttons side by side
        monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
        {
            if (monstro_button(ctx, "Left", nullptr)) {
                printf("Left button clicked\n");
            }
            if (monstro_button(ctx, "Middle", nullptr)) {
                printf("Middle button clicked\n");
            }
            if (monstro_button(ctx, "Right", nullptr)) {
                printf("Right button clicked\n");
            }
        }
        monstro_end_layout(ctx);
        
        // Test percent-based sizing
        monstro_separator_text(ctx, "Percent Width");
        
        float halfWidth = monstro_percent(ctx, 50.0f, MONSTRO_LAYOUT_HORIZONTAL);
        monstro_begin_layout(ctx, MONSTRO_LAYOUT_HORIZONTAL);
        {
            if (monstro_button_sized(ctx, "Left 50%", halfWidth - 4.0f, 25.0f, nullptr)) {
                printf("Left 50%% button clicked\n");
            }
            if (monstro_button_sized(ctx, "Right 50%", halfWidth - 4.0f, 25.0f, nullptr)) {
                printf("Right 50%% button clicked\n");
            }
        }
        monstro_end_layout(ctx);
        
        monstro_separator_text(ctx, "More Widgets");
        
        if (monstro_button(ctx, "Reset Counter", nullptr)) {
            app->buttonClicks = 0;
        }
    }
    monstro_end_window(ctx);
    
    monstro_end_frame(ctx);
}

// Render one frame
static void renderFrame(App* app, float deltaTime) {
    int width, height;
    glfwGetFramebufferSize(app->window, &width, &height);
    if (width == 0 || height == 0) return;
    
    // Update UI
    updateUI(app, deltaTime, width, height);
    
    // Upload UI geometry
    MonstroCanvas* canvas = monstro_context_get_canvas(app->ui);
    
    uploadUIBuffers(app, canvas);
    
    // Begin command buffer
    VECommandBuffer* cmd = nullptr;
    if (veBeginCommandBuffer(app->device, &cmd) != VE_SUCCESS || !cmd) {
        return;
    }
    
    // Begin rendering
    veBeginRendering(cmd, &app->renderTarget);
    
    // Render UI
    renderUI(app, cmd, canvas, (uint32_t)width, (uint32_t)height);
    
    // End rendering
    veEndRendering(cmd);
    
    // Blit to swapchain
    VEResult result = veBlitTextureToSwapchain(cmd, app->colorTexture, app->swapchain, VK_FILTER_LINEAR);
    if (result == VE_ERROR_SWAPCHAIN_OUT_OF_DATE) {
        return;
    }
    
    // Present
    vePresentImage(app->swapchain, cmd, true);
}

// Cleanup
static void cleanup(App* app) {
    if (app->device) {
        veDeviceWaitIdle(app->device);
    }
    
    // Destroy Monstro
    if (app->ui) {
        monstro_context_destroy(app->ui);
    }
    
    // Destroy UI renderer resources
    if (app->vertexBuffer != VE_INVALID_ADDRESS) {
        veDestroyBuffer(app->device, app->vertexBuffer);
    }
    if (app->indexBuffer != VE_INVALID_ADDRESS) {
        veDestroyBuffer(app->device, app->indexBuffer);
    }
    if (app->whiteTexture != VE_INVALID_TEXTURE_INDEX) {
        veDestroyTexture(app->device, app->whiteTexture);
    }
    if (app->fontAtlasTexture != VE_INVALID_TEXTURE_INDEX) {
        veDestroyTexture(app->device, app->fontAtlasTexture);
    }
    if (app->pipeline) {
        veDestroyGraphicsPipeline(app->pipeline);
    }
    if (app->vertexShader) {
        veDestroyShader(app->vertexShader);
    }
    if (app->fragmentShader) {
        veDestroyShader(app->fragmentShader);
    }
    
    // Destroy render target
    if (app->colorTexture != VE_INVALID_TEXTURE_INDEX) {
        veDestroyTexture(app->device, app->colorTexture);
    }
    
    // Destroy VulkEase
    if (app->swapchain) {
        veDestroySwapchain(app->swapchain);
    }
    if (app->device) {
        veDestroyDevice(app->device);
    }
    if (app->context) {
        veDestroyContext(app->context);
    }
    
    // Cleanup GLFW
    if (app->window) {
        glfwDestroyWindow(app->window);
    }
    glfwTerminate();
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    App app = {};
    app.fontAtlasTexture = VE_INVALID_TEXTURE_INDEX;
    app.fontAtlasTextureId = 0;
    app.font = nullptr;
    g_app = &app;
    
    printf("Monstro UI Example\n");
    printf("==================\n\n");
    
    // Initialize
    if (!initWindow(&app)) {
        cleanup(&app);
        return 1;
    }
    
    if (!initVulkEase(&app)) {
        cleanup(&app);
        return 1;
    }
    
    if (!initUIRenderer(&app)) {
        cleanup(&app);
        return 1;
    }
    
    // Create Monstro context
    app.ui = monstro_context_create();
    if (!app.ui) {
        fprintf(stderr, "Failed to create Monstro context\n");
        cleanup(&app);
        return 1;
    }
    
    // Load font
    {
        MonstroFontManager* fontManager = monstro_context_get_font_manager(app.ui);
        MonstroTextureData atlasData = {};
        
        // Load TTF font at 16px, ASCII range (32-127)
        app.font = monstro_font_load_ttf_from_file(
            fontManager,
            "data/fonts/freeSans.ttf",
            16.0f,
            32,   // first code point (space)
            96,   // code point count (space through tilde)
            &atlasData
        );
        
        if (!app.font) {
            fprintf(stderr, "Failed to load font\n");
            cleanup(&app);
            return 1;
        }
        
        printf("Font loaded: atlas %ux%u, format %d\n", 
               atlasData.width, atlasData.height, atlasData.format);
        
        // Create GPU texture for font atlas
        VkFormat atlasFormat = (atlasData.format == MONSTRO_TEXTURE_FORMAT_R8) 
                               ? VK_FORMAT_R8_UNORM 
                               : VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags atlasUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT;
        
        if (veCreateTexture2D(app.device, atlasData.width, atlasData.height, atlasFormat, atlasUsage,
                              "MonstroFontAtlas", &app.fontAtlasTexture) != VE_SUCCESS ||
            app.fontAtlasTexture == VE_INVALID_TEXTURE_INDEX) {
            fprintf(stderr, "Failed to create font atlas texture\n");
            cleanup(&app);
            return 1;
        }
        
        // Upload atlas data
        VEResult uploadResult = veHostWriteTextureRegion(app.device, app.fontAtlasTexture, 
                                                         atlasData.data, atlasData.data_size, nullptr);
        if (uploadResult != VE_SUCCESS) {
            fprintf(stderr, "Failed to upload font atlas (error %d)\n", uploadResult);
        }
        
        // Use VulkEase texture index as Monstro texture ID
        // (simple 1:1 mapping for this example)
        app.fontAtlasTextureId = (uint64_t)app.fontAtlasTexture;
        monstro_font_set_atlas_texture(app.font, app.fontAtlasTextureId);
        
        // Register font with font manager and set as default
        monstro_font_add(fontManager, app.font, "default", 16.0f);
        monstro_context_set_default_font(app.ui, app.font);
        
        printf("Font atlas uploaded: texture index %u\n", app.fontAtlasTexture);
    }
    
    printf("Monstro UI initialized\n");
    printf("Starting render loop...\n\n");
    
    // Main loop
    double lastTime = glfwGetTime();
    VEFrameTimingInfo timing = {};
    
    while (!glfwWindowShouldClose(app.window)) {
        veBeginFrame(app.device);
        
        glfwPollEvents();
        
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;
        
        renderFrame(&app, deltaTime);
        
        veEndFrame(app.device, &timing);
        
        if (timing.frameNumber % 1000 == 0) {
            printf("Frame %llu: %.2f ms (%.1f FPS)\n",
                   (unsigned long long)timing.frameNumber,
                   timing.frameTimeMs,
                   timing.fps);
        }
        
        if (glfwGetKey(app.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            break;
        }
    }
    
    printf("\nShutting down...\n");
    cleanup(&app);
    printf("Done.\n");
    
    return 0;
}
