#define NR_OF_LIGHTS 4
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <random>
#include <math.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <Bits.h>
#include <bitset>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image/stb_image.h>
#include <stb_image/stb_image_write.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ImGUI/imgui.h>
#include <ImGUI/imgui_impl_glfw.h>
#include <ImGUI/imgui_impl_opengl3.h>

#include "src/DEBUG.H"
#include "src/shader.h"
#include "src/camera.h"
#include "src/Model.h"
#include "src/Framebuffer.h"
#include "src/Shadows.h"
#include "src/Bloom.h"
#include "src/Scene.h"
#include "src/Assets.h"
#include "src/MasterRenderer.h"

/*Function declarations*/
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void mouseCallback(GLFWwindow* window, double xPos, double yPos);
void scrollCallback(GLFWwindow* window, double xOffset, double yOffset); 
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* filePath, bool gammaCorrection = false);
unsigned int loadCubemap(std::vector<std::string> faces);
float calculateExposure(unsigned int FBO);
void renderSphere();
void renderQuad();
void renderCube();

const float Pi = 3.14159265359f;

unsigned int wWidth = 1920 * 1 / 2, wHeight = 1080 * 1 / 2;
unsigned int ImGuiWindowWidth = 400, ImGuiWindowHeight = wHeight;
bool fullscreen = false, fKeyPressed = false;

// camera
Camera camera(glm::vec3(-1.0f, 0.0f, 5.0f));
float farClipDist = 100.0f;
float lastX = (float)wWidth / 2;
float lastY = (float)wHeight / 2;
bool firstMouse = true;
bool mouseIsVisible = true, mKeyPressed = false;

// timing
double averageFrameTime = 0.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float exposure = 0.0f;
float bloom = 0.05f;
bool lensDirt = true;

int ssaoKernalSize = 64;
float ssaoRadius = 0.5f;
float ssaoBias = 0.025;

glm::vec4 backgroundColor = { 0.2f, 0.3f, 0.3f, 1.0f };

float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

int main()
{
    debug::showConsoleWindow();
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //glfwWindowHint(GLFW_SAMPLES, 4);//enable for multisampling
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //Creates a new window object and checks for errors			*OPTIMIZABLE*
    GLFWwindow* window = glfwCreateWindow(wWidth + ImGuiWindowWidth, wHeight, "LearnOpenGL", NULL, NULL);
    if(window == NULL)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    //Makes the window the current context
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //Initializes Glad and checks for errors		*OPTIMIZABLE*
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    stbi_set_flip_vertically_on_load(1);

    glEnable(GL_DEPTH_TEST);//enables the Depth Buffer and depth testing
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    //glEnable(GL_MULTISAMPLE);//enables basic multisampling

    // builds and compiles our shaders
    DShader defaultBlinnPhongShader("ProgramFiles\\Resources\\Shaders\\default\\defaultBlinnPhongShader.V.shader", "ProgramFiles\\Resources\\Shaders\\default\\defaultBlinnPhongShader.F.shader");
    DShader defaultPBRShader("ProgramFiles\\Resources\\Shaders\\default\\defaultPBRShader.V.shader", "ProgramFiles\\Resources\\Shaders\\default\\defaultPBRShader.F.shader");

    Shader irradianceShader("ProgramFiles\\Resources\\Shaders\\cubemap.V.shader", "ProgramFiles\\Resources\\Shaders\\irradianceConvolution.F.shader");
    Shader prefilterShader("ProgramFiles\\Resources\\Shaders\\cubemap.V.shader", "ProgramFiles\\Resources\\Shaders\\prefilter.F.shader");
    Shader brdfShader("ProgramFiles\\Resources\\Shaders\\BRDF.V.shader", "ProgramFiles\\Resources\\Shaders\\BRDF.F.shader");
    Shader backgroundShader("ProgramFiles\\Resources\\Shaders\\background.V.shader", "ProgramFiles\\Resources\\Shaders\\background.F.shader");

    Shader bloomShader("ProgramFiles\\Resources\\Shaders\\Bloom\\finalBloom.V.shader", "ProgramFiles\\Resources\\Shaders\\Bloom\\finalBloom.F.shader");

    Shader GBufferShader("ProgramFiles\\Resources\\Shaders\\GBuffer.V.shader", "ProgramFiles\\Resources\\Shaders\\GBuffer.F.shader");
    Shader PBRFirstPass("ProgramFiles\\Resources\\Shaders\\PBRdeferred.V.shader", "ProgramFiles\\Resources\\Shaders\\PBRFirstPass.F.shader");
    Shader PBRSecondPass("ProgramFiles\\Resources\\Shaders\\PBRdeferred.V.shader", "ProgramFiles\\Resources\\Shaders\\PBRSecondPass.F.shader");

    Shader SSAOShader("ProgramFiles\\Resources\\Shaders\\SSAO\\SSAO.V.shader", "ProgramFiles\\Resources\\Shaders\\SSAO\\SSAO.F.shader");
    Shader SSAOBlurShader("ProgramFiles\\Resources\\Shaders\\SSAO\\SSAO.V.shader", "ProgramFiles\\Resources\\Shaders\\SSAO\\SSAOBlur.F.shader");

    PBRSecondPass.use();
    PBRSecondPass.setInt("irradianceMap", 0);
    PBRSecondPass.setInt("prefilterMap", 1);
    PBRSecondPass.setInt("brdfLUT", 2);
    PBRSecondPass.setInt("LoMap", 3);
    PBRSecondPass.setInt("gMaterialMask", 4);
    PBRSecondPass.setInt("gPosition", 5);
    PBRSecondPass.setInt("gNormalShadow", 6);
    PBRSecondPass.setInt("gAlbedo", 7);
    PBRSecondPass.setInt("gMetalRoughAO", 8);


    backgroundShader.use();
    backgroundShader.setInt("environmentMap", 0);

    //textures
    unsigned int ironAlbedoMap       = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\rustedIron\\albedo.png   ");
    unsigned int ironNormalMap       = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\rustedIron\\normal.png   ");
    unsigned int ironMetallicMap     = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\rustedIron\\metallic.png ");
    unsigned int ironRoughnessMap    = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\rustedIron\\roughness.png");
    unsigned int ironAOMap           = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\rustedIron\\ao.png       ");
                                     
    unsigned int goldAlbedoMap       = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\gold\\albedo.png   ");
    unsigned int goldNormalMap       = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\gold\\normal.png   ");
    unsigned int goldMetallicMap     = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\gold\\metallic.png ");
    unsigned int goldRoughnessMap    = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\gold\\roughness.png");
    unsigned int goldAOMap           = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\gold\\ao.png       ");

    unsigned int grassAlbedoMap      = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\grass\\albedo.png   ");
    unsigned int grassNormalMap      = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\grass\\normal.png   ");
    unsigned int grassMetallicMap    = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\grass\\metallic.png ");
    unsigned int grassRoughnessMap   = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\grass\\roughness.png");
    unsigned int grassAOMap          = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\grass\\ao.png       ");

    unsigned int plasticAlbedoMap    = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\plastic\\albedo.png   ");
    unsigned int plasticNormalMap    = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\plastic\\normal.png   ");
    unsigned int plasticMetallicMap  = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\plastic\\metallic.png ");
    unsigned int plasticRoughnessMap = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\plastic\\roughness.png");
    unsigned int plasticAOMap        = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\plastic\\ao.png       ");

    unsigned int wallAlbedoMap       = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\wall\\albedo.png   ");
    unsigned int wallNormalMap       = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\wall\\normal.png   ");
    unsigned int wallMetallicMap     = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\wall\\metallic.png ");
    unsigned int wallRoughnessMap    = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\wall\\roughness.png");
    unsigned int wallAOMap           = loadTexture("ProgramFiles\\Resources\\Textures\\pbr\\wall\\ao.png       ");

    unsigned int cubeAlbedoMap = loadTexture("ProgramFiles\\Resources\\Textures\\metal.png");
    unsigned int cubeNormalMap = loadTexture("ProgramFiles\\Resources\\Textures\\toy_box_normal.png");
    unsigned int cubeMetallicMap = loadTexture("ProgramFiles\\Resources\\Textures\\noise.png");
    unsigned int cubeRoughnessMap = loadTexture("ProgramFiles\\Resources\\Textures\\noise.png");
    unsigned int cubeAOMap = loadTexture("ProgramFiles\\Resources\\Textures\\noise.png");


    //light properties
    glm::vec3 lightPositions[] = {
        glm::vec3(-10.0f,  10.0f, 10.0f),
        glm::vec3( 10.0f,  10.0f, 10.0f),
        glm::vec3(-10.0f, -10.0f, 10.0f),
        glm::vec3( 10.0f, -10.0f, 10.0f)
    };

    glm::vec3 lightColors[] = {
        glm::vec3(400.0f, 200.0f, 200.0f),
        glm::vec3(200.0f, 200.0f, 400.0f),
        glm::vec3(200.0f, 400.0f, 200.0f),
        glm::vec3(300.0f, 300.0f, 300.0f)
    };

    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    std::vector<glm::vec3> SSAOSamples;
    for(unsigned int i = 0; i < 64; i++)
    {
        glm::vec3 sample = glm::vec3(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);

        SSAOSamples.push_back(sample);
    }

    Framebuffer gBuffer(wWidth, wHeight, 1, true);
    gBuffer.addTextureAttachment(GL_RGB, GL_RGB, GL_NEAREST, GL_NEAREST); //Material Mask texture
    gBuffer.addTextureAttachment(GL_RGBA16F, GL_RGBA, GL_LINEAR, GL_LINEAR); //Normal/transparency texture
    gBuffer.addTextureAttachment(GL_RGBA, GL_RGBA, GL_LINEAR, GL_LINEAR); //Albedo texture
    gBuffer.addTextureAttachment(GL_RGB, GL_RGB, GL_NEAREST, GL_NEAREST); //Metallic/Roughness/Ambient occlusion texture
    gBuffer.addTextureAttachment(GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_NEAREST, GL_NEAREST, GL_DEPTH_ATTACHMENT); //Depth buffer
    if(!gBuffer.checkStatus())
    {
        std::cout << "Error creating gBuffer" << std::endl;
        return -1;
    }

    int randomNoiseTexture = loadTexture("ProgramFiles\\Resources\\Textures\\noise.png");

    //creates framebuffer object
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    
    //loads the skybox cube map
    std::vector<std::string> cubeMapFilePaths;
    cubeMapFilePaths.push_back("ProgramFiles\\Resources\\Textures\\skybox\\right.jpg");
    cubeMapFilePaths.push_back("ProgramFiles\\Resources\\Textures\\skybox\\left.jpg");
    cubeMapFilePaths.push_back("ProgramFiles\\Resources\\Textures\\skybox\\top.jpg");
    cubeMapFilePaths.push_back("ProgramFiles\\Resources\\Textures\\skybox\\bottom.jpg");
    cubeMapFilePaths.push_back("ProgramFiles\\Resources\\Textures\\skybox\\front.jpg");
    cubeMapFilePaths.push_back("ProgramFiles\\Resources\\Textures\\skybox\\back.jpg");
    stbi_set_flip_vertically_on_load(false);
    unsigned int skyboxCubeMap = loadCubemap(cubeMapFilePaths);

    //matrices for capturing data onto the cubemap
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    //creates cubemap for irradiance
    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for(unsigned int i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    //renders (solves the diffuse integral) to create the irradiance map
    irradianceShader.use();
    irradianceShader.setInt("environmentMap", 0);
    irradianceShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubeMap);
    glViewport(0, 0, 32, 32);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    for(unsigned int i = 0; i < 6; i++)
    {
        irradianceShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    unsigned int prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for(unsigned int i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);// enable pre-filter mipmap sampling 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //generates mipmaps for cubemap
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    //run a quasi monte-carlo simulation on the environment lighting to create a prefilter cubemap
    prefilterShader.use();
    prefilterShader.setInt("environmentMap", 0);
    prefilterShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubeMap);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipmapLevels = 5;
    for(unsigned int mip = 0; mip < maxMipmapLevels; mip++)
    {
        unsigned int mipWidth = (unsigned int)(128 * std::pow(0.5, mip));
        unsigned int mipHeight = (unsigned int)(128 * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipmapLevels - 1);
        prefilterShader.setFloat("roughness", roughness);
        for(unsigned int i = 0; i < 6; i++)
        {
            prefilterShader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderCube();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* brdfData = stbi_loadf("brdfLUT.hdr", &width, &height, &nrComponents, 0);

    //generates the 2D look-up texture
    unsigned int brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);

    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RGB, GL_FLOAT, brdfData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    unsigned int mainFBO, mainRBO;
    unsigned int HDRColorBuffer0, HDRColorBuffer1;
    glGenFramebuffers(1, &mainFBO);
    glGenRenderbuffers(1, &mainRBO);

    glGenTextures(1, &HDRColorBuffer0);
    glBindTexture(GL_TEXTURE_2D, HDRColorBuffer0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, wWidth, wHeight, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &HDRColorBuffer1);
    glBindTexture(GL_TEXTURE_2D, HDRColorBuffer1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, wWidth, wHeight, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    glBindFramebuffer(GL_FRAMEBUFFER, mainFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, HDRColorBuffer0, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, mainRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, wWidth, wHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mainRBO);
    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("ERROR MAKING FRAMEBUFFER:: ATTACHMENTS COULDN'T BE SUCCESSFULLY MADE WITH CAPTURE FBO ");
        return -1;
    }


    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glm::mat4 model(1.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 inverseView = glm::inverse(view);
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)wWidth / (float)wHeight, 0.1f, farClipDist);
    glm::mat4 invProjection = glm::inverse(projection);
    //projection = glm::infinitePerspective(glm::radians(camera.Zoom), (float)wWidth / (float)wHeight, 0.1f);
    backgroundShader.use();
    backgroundShader.setMat4("projection", projection);

    int scrWidth, scrHeight;
    glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
    glViewport(0, 0, scrWidth, scrHeight);

    BloomRenderer bloomRenderer;
    bloomRenderer.Init(wWidth, wHeight, "ProgramFiles\\Resources\\Textures\\lensDirt0.jpg");

    bloomShader.use();
    bloomShader.setInt("scene", 0);
    bloomShader.setInt("bloomBlur", 1);
    bloomShader.setInt("lensDirtTexture", 2);
    bloomShader.setFloat("NrOfMips", float(bloomRenderer.m_NrMips));

    ShadowRenderer shadowRenderer;
    shadowRenderer.Init();
    shadowRenderer.createShadowMap(0, 1024, 1024, lightPositions[0], lightColors[0], POINT_LIGHT);
    shadowRenderer.createShadowMap(1, 1024, 1024, lightPositions[1], lightColors[1], POINT_LIGHT);
    shadowRenderer.createShadowMap(2, 1024, 1024, lightPositions[2], lightColors[2], POINT_LIGHT);
    shadowRenderer.createShadowMap(3, 1024, 1024, lightPositions[3], lightColors[3], POINT_LIGHT);

    GBufferShader.use();
    GBufferShader.setInt("gMaterialMask", 0);
    GBufferShader.setInt("gNormal", 1);
    GBufferShader.setInt("gAlbedo", 2);
    GBufferShader.setInt("gMetalRoughAO", 3);
    GBufferShader.setInt("albedoMap", 0);
    GBufferShader.setInt("normalMap", 1);
    GBufferShader.setInt("metallicMap", 2);
    GBufferShader.setInt("roughnessMap", 3);
    GBufferShader.setInt("aoMap", 4);

    PBRFirstPass.use();
    PBRFirstPass.setInt("light.m_CubeShadowMap", 0);
    PBRFirstPass.setInt("gMaterialMask", 1);
    PBRFirstPass.setInt("gNormal", 2);
    PBRFirstPass.setInt("gAlbedo", 3);
    PBRFirstPass.setInt("gMetalRoughAO", 4);
    PBRFirstPass.setInt("gDepth", 5);

    PBRSecondPass.use();
    PBRSecondPass.setInt("irradianceMap", 0);
    PBRSecondPass.setInt("prefilterMap", 1);
    PBRSecondPass.setInt("brdfLUT", 2);
    PBRSecondPass.setInt("LoMap", 3);
    PBRSecondPass.setInt("gMaterialMask", 4);
    PBRSecondPass.setInt("gNormal", 5);
    PBRSecondPass.setInt("gAlbedo", 6);
    PBRSecondPass.setInt("gMetalRoughAO", 7);
    PBRSecondPass.setInt("gDepth", 8);


    backgroundShader.use();
    backgroundShader.setInt("environmentMap", 0);

    SSAOShader.use();
    for(unsigned int i = 0; i < SSAOSamples.size(); i++)
    {
        SSAOShader.setVec3("samples[" + std::to_string(i) + "]", SSAOSamples[i]);
    }
    SSAOShader.setInt("gMaterialMask", 0);
    SSAOShader.setInt("gNormal", 1);
    SSAOShader.setInt("gMetalRoughAO", 2);
    SSAOShader.setInt("gDepth", 3);
    SSAOShader.setInt("noiseTex", 4);

    SSAOBlurShader.use();
    SSAOBlurShader.setInt("occlusionBuffer", 0);

    //glEnable(GL_CULL_FACE);//<--- Enable
    //glCullFace(GL_BACK);   //<--- these for
    //glFrontFace(GL_CCW);   //<--- perfomance
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//Use for debugging certain polygons

    //Main rendering for loop		*OPTIMIZABLE*
    /*-------------------------------------------------------------------------------------------------------------------------*/
    for(long long frameNR = 0; !glfwWindowShouldClose(window); frameNR++)
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;//this tells us how long a frame is
        if(frameNR == 0)
            averageFrameTime = deltaTime;
        averageFrameTime = (averageFrameTime + deltaTime) / 2.0f;
        lastFrame = currentFrame;

        processInput(window);

        model = glm::mat4(1.0f);
        view = camera.GetViewMatrix();
        inverseView = glm::inverse(view);
        projection = glm::perspective(glm::radians(camera.Zoom), (float)wWidth / (float)wHeight, 0.1f, farClipDist);
        invProjection = glm::inverse(projection);
        backgroundShader.use();
        backgroundShader.setMat4("projection", projection);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);//enables the Depth Buffer and depth testing
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);//This clears the color buffer and sets it to be this color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        for(unsigned int i = 0; i < NR_OF_LIGHTS; i++)
        {
            shadowRenderer.use(i);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            model = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0, 0.0, 2.0));
            shadowRenderer.m_CurrentShader->setMat4("model", model);
            renderSphere();
            model = glm::translate(glm::mat4(1.0f), glm::vec3(-3.0, 0.0, 2.0));
            shadowRenderer.m_CurrentShader->setMat4("model", model);
            renderSphere();
            model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0, 0.0, 2.0));
            shadowRenderer.m_CurrentShader->setMat4("model", model);
            renderSphere();
            model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0, 0.0, 2.0));
            shadowRenderer.m_CurrentShader->setMat4("model", model);
            renderSphere();
            model = glm::translate(glm::mat4(1.0f), glm::vec3(3.0, 0.0, 2.0));
            shadowRenderer.m_CurrentShader->setMat4("model", model);
            renderSphere();
            model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0, 4.0));
            model = glm::scale(model, glm::vec3(0.5f));
            shadowRenderer.m_CurrentShader->setMat4("model", model);
            renderCube();
        }

        //geometry buffer pass
        {
            gBuffer.use();
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);//This clears the color buffer and sets it to be this color
            glEnable(GL_DEPTH_TEST);//enables the Depth Buffer and depth testing
            glDepthMask(GL_TRUE);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_BLEND);
            glViewport(0, 0, gBuffer.getWidth(), gBuffer.getHeight());
            GBufferShader.use();
            GBufferShader.setMat4("view", view);
            GBufferShader.setFloat("time", glfwGetTime() * 0.1f);
            GBufferShader.setMat4("projection", projection);

            //rusted iron
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ironAlbedoMap);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, ironNormalMap);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, ironMetallicMap);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, ironRoughnessMap);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, ironAOMap);
            
            model = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0, 0.0, 2.0));
            model = glm::rotate(model, (float)sin(glfwGetTime() * 0.1f), glm::vec3(0.0f, 1.0f, 0.0f));
            GBufferShader.setMat4("model", model);
            GBufferShader.setVec3("material", materialPBR);
            renderSphere();
            
            //gold
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, goldAlbedoMap);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, goldNormalMap);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, goldMetallicMap);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, goldRoughnessMap);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, goldAOMap);
            
            model = glm::translate(glm::mat4(1.0f), glm::vec3(-3.0, 0.0, 2.0));
            model = glm::rotate(model, (float)sin(glfwGetTime() * 0.1f), glm::vec3(0.0f, 1.0f, 0.0f));
            GBufferShader.setMat4("model", model);
            GBufferShader.setVec3("material", materialPBR);
            renderSphere();
            
            //grass
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, grassAlbedoMap);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, grassNormalMap);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, grassMetallicMap);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, grassRoughnessMap);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, grassAOMap);
            
            model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0, 0.0, 2.0));
            model = glm::rotate(model, (float)sin(glfwGetTime() * 0.1f), glm::vec3(0.0f, 1.0f, 0.0f));
            GBufferShader.setMat4("model", model);
            GBufferShader.setVec3("material", materialPBR);
            renderSphere();

            //plastic
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, plasticAlbedoMap);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, plasticNormalMap);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, plasticMetallicMap);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, plasticRoughnessMap);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, plasticAOMap);
            
            model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0, 0.0, 2.0));
            model = glm::rotate(model, (float)sin(glfwGetTime() * 0.1f), glm::vec3(0.0f, 1.0f, 0.0f));
            GBufferShader.setMat4("model", model);
            GBufferShader.setVec3("material", materialBlinnPhong);
            renderSphere();
            
            //wall
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, wallAlbedoMap);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, wallNormalMap);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, wallMetallicMap);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, wallRoughnessMap);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, wallAOMap);
            
            model = glm::translate(glm::mat4(1.0f), glm::vec3(3.0, 0.0, 2.0));
            model = glm::rotate(model, (float)sin(glfwGetTime() * 0.1f), glm::vec3(0.0f, 1.0f, 0.0f));
            GBufferShader.setMat4("model", model);
            GBufferShader.setVec3("material", materialCellShading);
            renderSphere();

            //cube
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cubeAlbedoMap);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, cubeNormalMap);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, cubeMetallicMap);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, cubeRoughnessMap);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, cubeAOMap);

            model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0, 4.0));
            model = glm::scale(model, glm::vec3(0.5f));
            GBufferShader.setMat4("model", model);
            GBufferShader.setVec3("material", materialCellShading);
            renderCube();
        }
        
        //TO DO: add a SSAO (screen space ambient occlusion) pass
        {{
            glBindFramebuffer(GL_FRAMEBUFFER, mainFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, HDRColorBuffer0, 0);
            glDisable(GL_BLEND);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            SSAOShader.use();
            SSAOShader.setInt("kernalSize", ssaoKernalSize);
            SSAOShader.setFloat("radius", ssaoRadius);
            SSAOShader.setFloat("bias", ssaoBias);
            SSAOShader.setFloat("time", glfwGetTime());
            SSAOShader.setVec2("noiseScale", glm::vec2(wWidth / 512, wHeight / 512));
            SSAOShader.setMat4("projection", projection);
            SSAOShader.setMat4("invProjection", invProjection);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gBuffer.m_Textures[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gBuffer.m_Textures[1]);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gBuffer.m_Textures[3]);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, gBuffer.m_Textures[4]);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, randomNoiseTexture);

            renderQuad();

            gBuffer.use();

            SSAOBlurShader.use();
            SSAOBlurShader.setVec2("resolution", glm::vec2(wWidth, wHeight));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, HDRColorBuffer0);

            renderQuad();
        }}


        //PBR double pass
        {
         //first PBR pass
                glBindFramebuffer(GL_FRAMEBUFFER, mainFBO);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, HDRColorBuffer1, 0);
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);//This clears the color buffer and sets it to be this color
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glEnable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_DST_ALPHA, GL_ONE, GL_ONE);

                PBRFirstPass.use();
                PBRFirstPass.setVec3("camPos", camera.Position);
                PBRFirstPass.setMat4("inverseView", inverseView);
                PBRFirstPass.setMat4("invProjection", invProjection);
                for(unsigned int i = 0; i < shadowRenderer.getNrOfShadowMaps(); i++)
                {
                    PBRFirstPass.setMat4("inverseView", inverseView);
                    PBRFirstPass.setFloat("light.m_Resolution", shadowRenderer.shadowMaps[i]->m_Width);
                    PBRFirstPass.setVec3("light.m_Pos", shadowRenderer.shadowMaps[i]->m_Light->m_Pos);
                    PBRFirstPass.setVec3("light.m_Color", lightColors[i]);
                    PBRFirstPass.setFloat("light.m_FarPlane", SHADOW_FAR_PLANE);


                    //bind the shadow maps
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, shadowRenderer.shadowMapTexture(i));
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, gBuffer.m_Textures[0]);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, gBuffer.m_Textures[1]);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, gBuffer.m_Textures[2]);
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_2D, gBuffer.m_Textures[3]);
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, gBuffer.m_Textures[4]);


                    renderQuad();
                }
                glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
                glDisable(GL_BLEND);



                //second PBR pass
                glBindFramebuffer(GL_FRAMEBUFFER, mainFBO);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, HDRColorBuffer0, 0);
                glViewport(0, 0, wWidth, wHeight);
                glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f);//This clears the color buffer and sets it to be this color
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glEnable(GL_BLEND);

                PBRSecondPass.use();
                PBRSecondPass.setVec3("camPos", camera.Position);
                PBRSecondPass.setMat4("invProjection", invProjection);
                PBRSecondPass.setMat4("invView", inverseView);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, HDRColorBuffer1);

                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, gBuffer.m_Textures[0]); //Material Mask texture
                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_2D, gBuffer.m_Textures[1]); //Normal/Shadows texture
                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_2D, gBuffer.m_Textures[2]); //Albedo texture
                glActiveTexture(GL_TEXTURE7);
                glBindTexture(GL_TEXTURE_2D, gBuffer.m_Textures[3]); //Metallic/Roughness/Ambient Occlusion texture
                glActiveTexture(GL_TEXTURE8);
                glBindTexture(GL_TEXTURE_2D, gBuffer.m_Textures[4]); //Depth texture

                renderQuad();
        }

        //TO DO: add a transparency pass that renders everything transparent/translucent on top of the gbuffer and handles screen space reflections/refractions


        for(unsigned int i = 0; i < sizeof(lightPositions) / sizeof(lightPositions[0]); i++)
        {
            lightPositions[i] += glm::vec3(sin(glfwGetTime() * 0.5f) * 0.005f, cos(glfwGetTime() * 0.5f) * 0.005f, sin(glfwGetTime() * 0.25f) * cos(glfwGetTime()) * 0.001f);
            lightColors[i] += glm::vec3(0.5f * sin(glfwGetTime() + 0 / Pi), 0.5f * sin(glfwGetTime() + 1 / Pi), 0.5f * sin(glfwGetTime() + 2 / Pi));
        }
        //camera.Position = lightPositions[0];
        shadowRenderer.updateShadowMap(0, lightPositions[0], lightColors[0]);
        shadowRenderer.updateShadowMap(1, lightPositions[1], lightColors[1]);
        shadowRenderer.updateShadowMap(2, lightPositions[2], lightColors[2]);
        shadowRenderer.updateShadowMap(3, lightPositions[3], lightColors[3]);

        //renders skybox
        backgroundShader.use();
        backgroundShader.setMat4("view", view);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubeMap);
        
        renderCube();
        glDepthFunc(GL_LEQUAL);


        bloomRenderer.RenderBloomTexture(HDRColorBuffer0, 0.0005f);
        
        bloomShader.use();
        bloomShader.setFloat("exposure", exposure);
        bloomShader.setFloat("bloomStrength", bloom);
        bloomShader.setInt("isLensDirt", lensDirt);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, HDRColorBuffer0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bloomRenderer.BloomTexture());
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, bloomRenderer.LensDirtTexture());
        renderQuad();

        //shadowRenderer.debugShadowMap(0);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {   //ImGui Rendering
            ImGui::Begin("Control Window 1");
            ImGui::SetWindowPos(ImVec2(wWidth, 0));
            ImGui::SetWindowSize(ImVec2(ImGuiWindowWidth, ImGuiWindowHeight));
            if(ImGui::TreeNode("Application Performance"))
            {
                ImGui::Text("Application average frame time: %.3fms (%.1f FPS)", deltaTime, 1.0f / deltaTime);
                ImGui::Text("Field of view: %.3f", camera.Zoom);
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("Exposure and Bloom"))
            {
                if(ImGui::Button(std::string("Lens Dirt: ").append(lensDirt ? "Enabled" : "Disabled").c_str()))
                    lensDirt = !lensDirt;

                ImGui::SliderFloat("exposure", &exposure, -Pi, Pi);
                ImGui::SliderFloat("bloom", &bloom, 0.0f, 1.0f);

                if(ImGui::Button("reset exposure"))
                    exposure = 0.0f;
                ImGui::SameLine();
                if(ImGui::Button("reset bloom"))
                    bloom = 0.05f;
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("Lights"))
            {
                ImGui::DragFloat3("Light[0] Position", glm::value_ptr(lightPositions[0]), 0.1f, -50.0f,  50.0f);
                ImGui::DragFloat3("Light[0] Color   ", glm::value_ptr(lightColors   [0]), 0.1f,   0.0f, 100.0f);
                ImGui::DragFloat3("Light[1] Position", glm::value_ptr(lightPositions[1]), 0.1f, -50.0f,  50.0f);
                ImGui::DragFloat3("Light[1] Color   ", glm::value_ptr(lightColors   [1]), 0.1f,   0.0f, 100.0f);
                ImGui::DragFloat3("Light[2] Position", glm::value_ptr(lightPositions[2]), 0.1f, -50.0f,  50.0f);
                ImGui::DragFloat3("Light[2] Color   ", glm::value_ptr(lightColors   [2]), 0.1f,   0.0f, 100.0f);
                ImGui::DragFloat3("Light[3] Position", glm::value_ptr(lightPositions[3]), 0.1f, -50.0f,  50.0f);
                ImGui::DragFloat3("Light[3] Color   ", glm::value_ptr(lightColors   [3]), 0.1f,   0.0f, 100.0f);

                ImGui::TreePop();
            }
            if(ImGui::TreeNode("SSAO"))
            {
                ImGui::DragFloat("ssaoRadius", &ssaoRadius, 0.1f, 0.0f, 5.0f);
                ImGui::DragFloat("ssaoBias", &ssaoBias, 0.1f, 0.0f, 1.0f);
                ImGui::DragInt("kernalSize", &ssaoKernalSize);

                ImGui::TreePop();
            }
            ImGui::Text("Background Color");
            backgroundColor.r = sin(glfwGetTime() + 0.0f * Pi / 3.0f) * 0.5f + 0.5f;
            backgroundColor.g = sin(glfwGetTime() + 1.0f * Pi / 3.0f) * 0.5f + 0.5f;
            backgroundColor.b = sin(glfwGetTime() + 2.0f * Pi / 3.0f) * 0.5f + 0.5f;
            backgroundColor = glm::vec4(0.0f);
            ImGui::ColorPicker3("\0", glm::value_ptr(backgroundColor));
            ImGui::End();
        }


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);//swaps frame buffers
        glfwPollEvents();
    }

    irradianceShader.destroy();
    prefilterShader.destroy();
    brdfShader.destroy();
    backgroundShader.destroy();
    bloomShader.destroy();
    bloomRenderer.Destroy();
    shadowRenderer.Destroy();
    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();//this tells glfw to release any memory that is be using to run the window
    
    printf("Average frame time : %f.5ms\n", averageFrameTime / 1.0f);
    
    return 0;
}

/*Function definitions*/
//Function that is called when resizing the window      *OPTIMIZABLE*
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);//this changes the viewport to be the same as the window size
    //this sets the window width and height to be the rescaled ones
    wWidth = width;
    wHeight = height;
}
//Function that is called for mouse input       *OPTIMIZABLE*
void mouseCallback(GLFWwindow* window, double xPos, double yPos)
{
    if(!mouseIsVisible)
    {
        // tell GLFW to capture our mouse
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        if(firstMouse)
        {
            lastX = xPos;
            lastY = yPos;
            firstMouse = false;
        }

        float xOffset = xPos - lastX;
        float yOffset = lastY - yPos;

        lastX = xPos;
        lastY = yPos;
        
        camera.ProcessMouseMovement(xOffset, yOffset);
    }
    else
    {
        // tell GLFW to capture our mouse
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    }
}
//Function that is called for mouse scroll input
void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    camera.ProcessMouseScroll(yOffset);
}
//Function for reading inputs from the keyboard		*OPTIMIZABLE*
void processInput(GLFWwindow* window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)//if esc is pressed than this will close the program
        glfwSetWindowShouldClose(window, true);

    if(glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !mKeyPressed)
    {
        mouseIsVisible = !mouseIsVisible;
        mKeyPressed = true;
    }
    if(glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE)
        mKeyPressed = false;

    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)//forward
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)//backward
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)//left
        camera.ProcessKeyboard(LEFT, deltaTime);
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)//right
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)//Up
        camera.ProcessKeyboard(UP, deltaTime);
    if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)//Down
        camera.ProcessKeyboard(DOWN, deltaTime);

    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.MovementSpeed = 8.0f;//faster
    else
        camera.MovementSpeed = 4.0f;//slower

    if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)//if f is pressed it maximizes or unmaximizes the window
    {
        if(!fullscreen && !fKeyPressed)
        {
            glfwMaximizeWindow(window);
            fullscreen = true;
        }
        else if(fullscreen && !fKeyPressed)
        {
            glfwRestoreWindow(window);
            fullscreen = false;
        }
        fKeyPressed = true;
    }
    if(glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE)
        fKeyPressed = false;
}

//function used to load texture
unsigned int loadTexture(const char* path, bool gammaCorrection)
{
    unsigned int textureID;//declares a texture ID and glGenerates it
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if(data)
    {
        GLenum internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
        GLenum dataFormat = GL_RGBA;
        if(nrComponents == 1)
            internalFormat = dataFormat = GL_RED;
        else if(nrComponents == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        //this makes the textures repeat if texCoords are larger than 1 or less than 0 and tells openGL to use linear upscaling
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);//this frees the image from system memory since it should be in VRAM by now
    }
    else
    {   //this is if the image wasn't read for any reason
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

//function that is used to load cubemap from vector array of filepaths to textures. 
//Order: +X(Right), -X(Left), +Y(Up), -Y(Down), +Z(Back), -Z(Front)
unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for(unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, NULL);
        if(data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed loading at path: " << faces[i] << "\n";
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

//function used to calculate exposure value using brightness of pixels in the FBO
float calculateExposure(unsigned int FBO, int w = wWidth, int h = wHeight, float defaultExposure = 0.0f)
{
    ////needs a bit of work with pixel data and memory stuff
    //int NrComponents = 4;
    //char** pixelData = (char **) malloc(w * h * NrComponents * sizeof(float));
    //
    //if(pixelData == nullptr)
    //    return defaultExposure;
    //glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    //glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, pixelData);
    //
    //double brightness = 0.0f;
    //for(int i = 0; i < w; i++)
    //{
    //    for(int j = 0; j < h; j++)
    //    {
    //        for(int component = 0; component < NrComponents; component++)
    //        {
    //            unsigned int tempValue = (int)pixelData[i][j * NrComponents];
    //            if(component == NrComponents - 1)
    //                continue;
    //            brightness += ((char*)&tempValue)[component] ;
    //        }
    //    }
    //}
    return exposure;
}

unsigned int cubeVAO = 0, cubeVBO = 0;
void renderCube()
{
    if(cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,  // bottom-left        
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f // top-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);

        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    //glDepthFunc(GL_LEQUAL);
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int quadVAO = 0, quadVBO = 0;
void renderQuad()
{
    if(quadVAO == 0)//if the VAO hasn't already been created then set up the VAO and VBO to be ready for rendering
    {
        float quadVertices[] = {
            // positions            // texture Coords
            -1.0f,  1.0f, 0.0f,     0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f,     0.0f, 0.0f,
             1.0f,  1.0f, 0.0f,     1.0f, 1.0f,
             1.0f, -1.0f, 0.0f,     1.0f, 0.0f,
        };

        //configures the quad VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    //renders quad
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
    if(sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        for(unsigned int x = 0; x <= X_SEGMENTS; x++)
        {
            for(unsigned int y = 0; y <= Y_SEGMENTS; y++)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * Pi) * std::sin(ySegment * Pi);
                float yPos = std::cos(ySegment * Pi);
                float zPos = std::sin(xSegment * 2.0f * Pi) * std::sin(ySegment * Pi);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for(unsigned int y = 0; y < Y_SEGMENTS; y++)
        {
            if(!oddRow)
            {
                for(unsigned int x = 0; x <= X_SEGMENTS; x++)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for(unsigned int x = X_SEGMENTS; x > 0; x--)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow != oddRow;
        }
        indexCount = indices.size();

        std::vector<float> data;
        for(unsigned int i = 0; i < positions.size(); i++)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if(normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if(uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}