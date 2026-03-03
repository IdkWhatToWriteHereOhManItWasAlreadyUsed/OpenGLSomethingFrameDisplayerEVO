#include "Game.h"
#include "videoWorld/VideoWorld.h"

#ifdef BOJE_CHEL
#include <windows.h>
#include <psapi.h>
#endif

#include <libs/stb_image.h>

#include <renderer/Frustum.h>
#include <renderer/Shader.h>
#include <videoWorld/chunk/generation/ChunkGenerator.h>
#include <videoWorld/chunk/rendering/RayCaster.h>
#include <videoWorld/block/BlocksManager.h>
#include <imgui/imgui.h>
#include <memory>
#include "InputManager.h"
#include <iostream>
#include <cmath>
#include "Camera.h"
#include <imgui/imgui_impl_glfw.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui_impl_opengl3.h>
#include <glm/fwd.hpp>
#include <videoWorld/block/BlockMeshGenerator.h>
#include <videoWorld/chunk/rendering/ChunkGraphicalData.h>
#include <renderer/lighting/Lights.h>

#include <filesystem>
namespace fs = std::filesystem;

namespace OpenGLSomethingFrameDisplayerEVO {

Game::Game()
{
    m_batchQueue.reserve(5000);
}

bool Game::Initialise(int width, int height, int videoWidth, int videoHeight)
{
    std::cout << "Entar game init\n";
    m_state.windowHeight = height;
    m_state.windowWidth = width;

    if (!InitWindow())
        return false;

    std::cout << "windowt inited\n";

    if (!InitIMGUI())
        return false;

    std::cout << "imgui inited\n";

    if (!InitBlocksManager())
        return false;

    std::cout << "blocks inited\n";
    
    if (!InitShaders())   
        return false;

    std::cout << "Shaders inited\n";

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSwapInterval(1);

    InitTexture();
    InitGBuffer();
    InitZBuffer();

    float quadVertices[] =
    {
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);

    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    world.Initialise(videoWidth, videoHeight);

    ChunkGenerator::InitializeBlocks();
    BlockMeshGenerator::InitialiseCachesMeshes();

    return true;
}

bool Game::InitIMGUI()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO();

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    return true;
}

bool Game::InitWindow()
{
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
    #if defined(__APPLE__)
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
        glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
    #endif
    glfwInit();
    std::cout << "GLFW inited" << std::endl;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    // macOS?
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(m_state.windowWidth, m_state.windowHeight, "OpenGLSomething", nullptr, nullptr);
     std::cout << "GLFW window created" << std::endl;
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
         std::cout << "GLFW context zamuchen" << std::endl;
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return false;
    }

         std::cout << "GLew inited" << std::endl;

    glfwSwapInterval(1);

    std::cout << "vsync enabled" << std::endl;

    #if defined (__APPLE__)
        glfwGetFramebufferSize(window, 
                &m_state.windowWidth,
                &m_state.windowHeight);
    #endif
    glViewport(0, 0, m_state.windowWidth, m_state.windowHeight);

    InputManager::Initialize(window);
       std::cout << "input manager initted" << std::endl;
    InputManager::SetMouseCursorVisible(false);
    return true;
}

bool Game::InitBlocksManager()
{
    fs::path basePath = fs::current_path();
    
    fs::path blocksJson = basePath / "res" / "blocks" / "blocks.json";
    fs::path geometriesJson = basePath / "res" / "blocks" / "geometries.json";
    
    if (!fs::exists(blocksJson)) {
        std::cout << "Blocks JSON not found: " << blocksJson << std::endl;
        return false;
    }
    if (!fs::exists(geometriesJson)) {
        std::cout << "Geometries JSON not found: " << geometriesJson << std::endl;
        return false;
    }
    
    return BlocksManager::LoadBlocksData(
        blocksJson.string().c_str(), 
        geometriesJson.string().c_str()
    );
}

bool Game::InitShaders()
{
    try
    {
        fs::path basePath = fs::current_path();
        
        fs::path defaultVert = basePath / "res" / "shaders" / "default.vs.glsl";
        fs::path defaultFrag = basePath / "res" / "shaders" / "default.frag.glsl";
        
        if (!fs::exists(defaultVert)) {
            std::cout << "Shader not found: " << defaultVert << std::endl;
            return false;
        }
        if (!fs::exists(defaultFrag)) {
            std::cout << "Shader not found: " << defaultFrag << std::endl;
            return false;
        }
        
        defaultShader = std::make_unique<Shader>(
            defaultVert.string().c_str(),
            defaultFrag.string().c_str()
        );
        
        fs::path hilightVert = basePath / "res" / "shaders" / "hilight.vs.glsl";
        fs::path hilightFrag = basePath / "res" / "shaders" / "hilight.frag.glsl";
        
        if (!fs::exists(hilightVert) || !fs::exists(hilightFrag)) {
            std::cout << "Hilight shaders not found" << std::endl;
            return false;
        }
        
        hilightShader = std::make_unique<Shader>(
            hilightVert.string().c_str(),
            hilightFrag.string().c_str()
        );
        
        fs::path zPrepassVert = basePath / "res" / "shaders" / "zPrepass.vs.glsl";
        fs::path zPrepassFrag = basePath / "res" / "shaders" / "zPrepass.frag.glsl";
        
        if (!fs::exists(zPrepassVert) || !fs::exists(zPrepassFrag)) {
            std::cout << "ZPrepass shaders not found" << std::endl;
            return false;
        }
        
        zPrepassShader = std::make_unique<Shader>(
            zPrepassVert.string().c_str(),
            zPrepassFrag.string().c_str()
        );
        
        fs::path geometryVert = basePath / "res" / "shaders" / "gPass.vs.glsl";
        fs::path geometryFrag = basePath / "res" / "shaders" / "gPass.frag.glsl";
        
        if (!fs::exists(geometryVert) || !fs::exists(geometryFrag)) {
            std::cout << "Geometry shaders not found" << std::endl;
            return false;
        }
        
        geometryShader = std::make_unique<Shader>(
            geometryVert.string().c_str(),
            geometryFrag.string().c_str()
        );
        
        fs::path lightingVert = basePath / "res" / "shaders" / "lightPass.vs.glsl";
        fs::path lightingFrag = basePath / "res" / "shaders" / "lightPass.frag.glsl";
        
        if (!fs::exists(lightingVert) || !fs::exists(lightingFrag)) {
            std::cout << "Lighting shaders not found" << std::endl;
            return false;
        }
        
        lightingShader = std::make_unique<Shader>(
            lightingVert.string().c_str(),
            lightingFrag.string().c_str()
        );

        return true;
    }
    catch (const std::exception& e)
    {
        std::cout << "Failed to initialize shaders: " << e.what() << std::endl;
        return false;
    }
}

void Game::InitTexture()
{
    fs::path basePath = fs::current_path();
    fs::path texturePath = basePath / "res" / "textures" / "blocks.png";
    
    if (!fs::exists(texturePath)) {
        std::cerr << "Texture file not found: " << texturePath << std::endl;
        return;
    }
    
    int width, height, channels;
    unsigned char* image = stbi_load(texturePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (image == nullptr) {
        std::cerr << "Failed to load texture: " << texturePath << std::endl;
        std::cerr << "STBI error: " << stbi_failure_reason() << std::endl;
        return;
    }

    std::cout << "Loaded texture: " << texturePath 
              << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if (channels == 4) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(image);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_blocksTextureAtlas = textureID;
}

void Game::InitGBuffer()
{
    glGenFramebuffers(1, &gBuffer.FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);

    glGenTextures(static_cast<int>(GBufferTexture::Count), gBuffer.textures);

    glBindTexture(GL_TEXTURE_2D, gBuffer.textures[static_cast<int>(GBufferTexture::Positions)]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_state.windowWidth, m_state.windowHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        gBuffer.textures[static_cast<int>(GBufferTexture::Positions)], 0);

    glBindTexture(GL_TEXTURE_2D, gBuffer.textures[static_cast<int>(GBufferTexture::Normal)]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_state.windowWidth, m_state.windowHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
        gBuffer.textures[static_cast<int>(GBufferTexture::Normal)], 0);

    glBindTexture(GL_TEXTURE_2D, gBuffer.textures[static_cast<int>(GBufferTexture::AlbedoSpec)]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_state.windowWidth, m_state.windowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
        gBuffer.textures[static_cast<int>(GBufferTexture::AlbedoSpec)], 0);

    glGenTextures(1, &gBuffer.depthTexture);
    glBindTexture(GL_TEXTURE_2D, gBuffer.depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_state.windowWidth, m_state.windowHeight, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
        gBuffer.depthTexture, 0);

    GLuint attachments[3] =
    {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2
    };
    glDrawBuffers(3, attachments);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "G-Buffer FBO error: " << status << std::endl;
        switch (status) {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            std::cout << "Incomplete attachment" << std::endl; break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            std::cout << "Missing attachment" << std::endl; break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            std::cout << "Unsupported format" << std::endl; break;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Game::InitZBuffer()
{
    glGenFramebuffers(1, &zBuffer.FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, zBuffer.FBO);

    glGenTextures(1, &zBuffer.depthTexture);
    glBindTexture(GL_TEXTURE_2D, zBuffer.depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_state.windowWidth, m_state.windowHeight, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
        zBuffer.depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Can not initialise z-prepass" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


}