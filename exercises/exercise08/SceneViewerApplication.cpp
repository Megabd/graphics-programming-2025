#include "SceneViewerApplication.h"

#include <ituGL/asset/TextureCubemapLoader.h>
#include <ituGL/asset/ShaderLoader.h>
#include <ituGL/asset/ModelLoader.h>

#include <ituGL/camera/Camera.h>
#include <ituGL/scene/SceneCamera.h>

#include <ituGL/lighting/DirectionalLight.h>
#include <ituGL/lighting/PointLight.h>
#include <ituGL/scene/SceneLight.h>

#include <ituGL/shader/ShaderUniformCollection.h>
#include <ituGL/shader/Material.h>
#include <ituGL/geometry/Model.h>
#include <ituGL/scene/SceneModel.h>
#include <ituGL/scene/Transform.h>

#include <ituGL/renderer/SkyboxRenderPass.h>
#include <ituGL/renderer/ForwardRenderPass.h>
#include <ituGL/scene/RendererSceneVisitor.h>
#include <ituGL/scene/CubeRendererSceneVisitor.h>

#include <ituGL/scene/ImGuiSceneVisitor.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp> 
#include <iostream>
#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>



SceneViewerApplication::SceneViewerApplication()
    : Application(1024, 1024, "Scene Viewer demo")
    , m_renderer(GetDevice())
{
}

void SceneViewerApplication::Initialize()
{
    Application::Initialize();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize DearImGUI
    m_imGui.Initialize(GetMainWindow());

    m_noiseMap = CreateNoiseTexture(256, 256);

    InitializeCamera();
    InitializeLights();
    InitializeMaterial();
    InitializeModels();
    InitializeRenderer();

}

void SceneViewerApplication::Update()
{
    Application::Update();

    // Update camera controller
    m_cameraController.Update(GetMainWindow(), GetDeltaTime());

    // Add the scene nodes to the renderer
    RendererSceneVisitor rendererSceneVisitor(m_renderer);
    m_scene.AcceptVisitor(rendererSceneVisitor);
}

void SceneViewerApplication::Render()
{
    Application::Render();

    GetDevice().Clear(true, Color(0.5f, 0.5f, 0.5f, 1.0f), true, 1.0f);
    // Render the scene
    m_renderer.Render();

    // Render the debug user interface
    RenderGUI();
}

void SceneViewerApplication::Cleanup()
{
    // Cleanup DearImGUI
    m_imGui.Cleanup();

    Application::Cleanup();
}

void SceneViewerApplication::InitializeCamera()
{
    // Create the main camera
    std::shared_ptr<Camera> camera = std::make_shared<Camera>();
    camera->SetViewMatrix(glm::vec3(-1, 1, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    camera->SetPerspectiveProjectionMatrix(1.0f, 1.0f, 0.1f, 100.0f);

    // Create a scene node for the camera
    std::shared_ptr<SceneCamera> sceneCamera = std::make_shared<SceneCamera>("camera", camera);

    // Add the camera node to the scene
    m_scene.AddSceneNode(sceneCamera);

    // Set the camera scene node to be controlled by the camera controller
    m_cameraController.SetCamera(sceneCamera);
}

void SceneViewerApplication::InitializeLights()
{
    // Create a directional light and add it to the scene
    std::shared_ptr<DirectionalLight> directionalLight = std::make_shared<DirectionalLight>();
    directionalLight->SetDirection(glm::vec3(-0.3f, -1.0f, -0.3f)); // It will be normalized inside the function
    directionalLight->SetIntensity(3.0f);
    m_scene.AddSceneNode(std::make_shared<SceneLight>("directional light", directionalLight));

    // Create a point light and add it to the scene
    //std::shared_ptr<PointLight> pointLight = std::make_shared<PointLight>();
    //pointLight->SetPosition(glm::vec3(0, 0, 0));
    //pointLight->SetDistanceAttenuation(glm::vec2(5.0f, 10.0f));
    //m_scene.AddSceneNode(std::make_shared<SceneLight>("point light", pointLight));
}

void SceneViewerApplication::InitializeMaterial()
{
    // Load and build shader
    std::vector<const char*> vertexShaderPaths;
    vertexShaderPaths.push_back("shaders/version330.glsl");
    vertexShaderPaths.push_back("shaders/default.vert");
    Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

    // Default Shader
    std::vector<const char*> fragmentShaderPaths;
    fragmentShaderPaths.push_back("shaders/version330.glsl");
    fragmentShaderPaths.push_back("shaders/utils.glsl");
    fragmentShaderPaths.push_back("shaders/lambert-ggx.glsl");
    fragmentShaderPaths.push_back("shaders/lighting.glsl");
    fragmentShaderPaths.push_back("shaders/default_pbr.frag");
    Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

    // My invis shader
    std::vector<const char*> invisFragmentShaderPaths;
    invisFragmentShaderPaths.push_back("shaders/version330.glsl");
    invisFragmentShaderPaths.push_back("shaders/utils.glsl");
    invisFragmentShaderPaths.push_back("shaders/lambert-ggx.glsl");
    invisFragmentShaderPaths.push_back("shaders/lighting.glsl");
    invisFragmentShaderPaths.push_back("shaders/invis.frag");
    Shader invisShader = ShaderLoader(Shader::FragmentShader).Load(invisFragmentShaderPaths);

    // Filter out uniforms that are not material properties
    ShaderUniformCollection::NameSet filteredUniforms;
    filteredUniforms.insert("CameraPosition");
    filteredUniforms.insert("WorldMatrix");
    filteredUniforms.insert("ViewProjMatrix");
    filteredUniforms.insert("LightIndirect");
    filteredUniforms.insert("LightColor");
    filteredUniforms.insert("LightPosition");
    filteredUniforms.insert("LightDirection");
    filteredUniforms.insert("LightAttenuation");


    // Create reference material
    //assert(shaderProgramPtr);
    //m_defaultMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
    m_defaultMaterial = InitMaterial(fragmentShader, vertexShader, filteredUniforms);

    //assert(invisShaderProgramPtr);
    m_invisMaterial = InitMaterial(invisShader, vertexShader, filteredUniforms);
    m_invisMaterial->SetBlendEquation(Material::BlendEquation::Add);
    m_invisMaterial->SetBlendParams(
        Material::BlendParam::SourceAlpha,
        Material::BlendParam::OneMinusSourceAlpha
    );
    m_invisMaterial->SetDepthWrite(false);
    m_invisMaterial->SetDepthTestFunction(Material::TestFunction::LessEqual);
}

void SceneViewerApplication::InitializeModels()
{
    m_skyboxTexture = TextureCubemapLoader::LoadTextureShared("models/skybox/defaultCubemap.png", TextureObject::FormatRGB, TextureObject::InternalFormatSRGB8);

    m_skyboxTexture->Bind();
    m_skyboxTexture->GetParameter(TextureObject::ParameterFloat::MaxLod, m_skyboxMaxLod);
    TextureCubemapObject::Unbind();

    SetUniformsForMat(m_defaultMaterial);
    SetUniformsForMat(m_invisMaterial);

    // Configure loader
    ModelLoader loader = MakeLoader(m_defaultMaterial);

    // Configure loader
    ModelLoader invisLoader = MakeLoader(m_invisMaterial);

    // Load models

    //std::shared_ptr<Model> cameraModel = loader.LoadShared("models/camera/camera.obj");
    //m_scene.AddSceneNode(std::make_shared<SceneModel>("camera model", cameraModel));

    std::shared_ptr<Model> clockModel = loader.LoadShared("models/alarm_clock/alarm_clock.obj");
    m_scene.AddSceneNode(std::make_shared<SceneModel>("alarm clock", clockModel));


    std::shared_ptr<Model> chestModel = loader.LoadShared("models/treasure_chest/treasure_chest.obj");
    m_scene.AddSceneNode(std::make_shared<SceneModel>("treasure_chest", chestModel));

    std::shared_ptr<Model> guy = invisLoader.LoadShared("models/guy/VampKila.obj");
    auto guyNode = std::make_shared<SceneModel>("guy", guy);
    guyNode->GetTransform()->SetScale(glm::vec3(0.01f, 0.01f, 0.01f));
    m_refractiveObjects.push_back(guyNode);
    m_scene.AddSceneNode(guyNode);



    //std::shared_ptr<Model> teaSetModel = loader.LoadShared("models/tea_set/tea_set.obj");
    //m_scene.AddSceneNode(std::make_shared<SceneModel>("tea set", teaSetModel));

}

void SceneViewerApplication::InitializeRenderer()
{
    m_renderer.AddRenderPass(std::make_unique<SkyboxRenderPass>(m_skyboxTexture));
    m_renderer.AddRenderPass(std::make_unique<ForwardRenderPass>());
}

void SceneViewerApplication::RenderGUI()
{
    m_imGui.BeginFrame();

    // Draw GUI for scene nodes, using the visitor pattern
    ImGuiSceneVisitor imGuiVisitor(m_imGui, "Scene");
    m_scene.AcceptVisitor(imGuiVisitor);

    if (ImGui::Button(m_outline ? "Outline on" : "Outline off")) {
        m_outline = !m_outline; 
    }
    ImGui::SliderFloat("Outline/fresnel Strength", &m_outlineStr, 0.0f, 10.0f);

    if (ImGui::Button(m_flicker ? "Flicker on" : "Flicker off")) {
        m_flicker = !m_flicker; 
    }
    ImGui::SliderFloat("Maximum visible distance", &m_maxVisDist, 1.0f, 40.0f);
    ImGui::SliderFloat("Flicker Frequency", &m_flickerSpeed, 1.0f, 50.0f);
    ImGui::SliderFloat("Flicker Pattern Size", &m_flickerSize, 0.1f, 20.0f);
    ImGui::SliderFloat("Flicker Irregularity", &m_flickerChaos, 0.0f, 50.0f);
    ImGui::SliderFloat("Flicker On/Off Threshold", &m_flickerThreshold, 0.0f, 1.0f);

    if (ImGui::Button(m_refract ? "Refraction on" : "Refraction off")) {
        m_refract = !m_refract;
    }
    ImGui::SliderFloat("Ratio of indices of refraction", &m_IOR, 0.0f, 1.0f);


    if (ImGui::Button("Update Environment Map")) {

        // For each invisible object, generate envioment map without themselves in it, and send to shader.
        for (auto& node : m_refractiveObjects) {
            auto sceneCamNode = m_cameraController.GetCamera();
            auto& cam = *sceneCamNode->GetCamera();
            auto map = GenerateSceneCubemap(1024, cam, node.get());
            m_objectCubemap = map;
            node->GetModel()->GetMaterial(0).SetUniformValue("EnvironmentTexture", map);
        
        }
    }

    // Draw GUI for camera controller
    m_cameraController.DrawGUI(m_imGui);

    m_imGui.EndFrame();
}

std::shared_ptr<ShaderProgram> SceneViewerApplication::MakeProgram(Shader& fragmentShader, Shader& vertexShader)
{
    auto prog = std::make_shared<ShaderProgram>();
    prog->Build(vertexShader, fragmentShader);

    // cache uniform locations
    auto camLoc = prog->GetUniformLocation("CameraPosition");
    auto timeLoc = prog->GetUniformLocation("Time");
    auto outlineLoc = prog->GetUniformLocation("outlineOn");
    auto flickerLoc = prog->GetUniformLocation("flickerOn");
    auto refractionLoc = prog->GetUniformLocation("refractionOn");

    auto outlineStrLoc = prog->GetUniformLocation("outlineStr");

    auto maxVisDistLoc = prog->GetUniformLocation("maxVisDist");
    auto flickerSpeedLoc = prog->GetUniformLocation("flickerSpeed");
    auto flickerSizeLoc = prog->GetUniformLocation("flickerSize");
    auto flickerChaosLoc = prog->GetUniformLocation("flickerChaos");
    auto flickerThresholdLoc = prog->GetUniformLocation("flickerThreshold");

    auto IORLoc = prog->GetUniformLocation("IOR");

    auto worldLoc = prog->GetUniformLocation("WorldMatrix");
    auto vpLoc = prog->GetUniformLocation("ViewProjMatrix");

    m_renderer.RegisterShaderProgram(
        prog,
        [=](auto& shader, const glm::mat4& world, const Camera& cam, bool camChanged)
        {
            if (camChanged) {
                shader.SetUniform(camLoc, cam.ExtractTranslation());
                shader.SetUniform(vpLoc, cam.GetViewProjectionMatrix());
            }
            shader.SetUniform(worldLoc, world);
            float currentTime = static_cast<float>(glfwGetTime());
            shader.SetUniform(timeLoc, currentTime);
            shader.SetUniform(outlineLoc, static_cast<int>(m_outline));
            shader.SetUniform(flickerLoc, static_cast<int>(m_flicker));
            shader.SetUniform(refractionLoc, static_cast<int>(m_refract));
            shader.SetUniform(outlineStrLoc, m_outlineStr);
            shader.SetUniform(maxVisDistLoc, m_maxVisDist);
            shader.SetUniform(flickerSpeedLoc, m_flickerSpeed);
            shader.SetUniform(flickerSizeLoc, m_flickerSize);
            shader.SetUniform(flickerChaosLoc, m_flickerChaos);
            shader.SetUniform(flickerThresholdLoc, m_flickerThreshold);
            shader.SetUniform(IORLoc, m_IOR);
        },
        m_renderer.GetDefaultUpdateLightsFunction(*prog)
    );

    return prog;
}

std::shared_ptr<Material> SceneViewerApplication::InitMaterial(Shader& fragmentShader, Shader& vertexShader, ShaderUniformCollection::NameSet& filterUniforms)
{
    auto program = MakeProgram(fragmentShader, vertexShader);

    auto mat = std::make_shared<Material>(program, filterUniforms);

    return mat;
}

void SceneViewerApplication::SetUniformsForMat(std::shared_ptr<Material> mat) {
    // set the common defaults:
    mat->SetUniformValue("AmbientColor", glm::vec3(0.25f));
    mat->SetUniformValue("EnvironmentTexture", m_skyboxTexture);
    mat->SetUniformValue("EnvironmentMaxLod", m_skyboxMaxLod);
    mat->SetUniformValue("Color", glm::vec3(1.0f));
    mat->SetUniformValue("NoiseTexture", m_noiseMap);
}

ModelLoader SceneViewerApplication::MakeLoader(std::shared_ptr<Material> mat)
{
    ModelLoader loader(mat);
    loader.SetCreateMaterials(true);
    loader.GetTexture2DLoader().SetFlipVertical(true);

    // attributes
    loader.SetMaterialAttribute(VertexAttribute::Semantic::Position, "VertexPosition");
    loader.SetMaterialAttribute(VertexAttribute::Semantic::Normal, "VertexNormal");
    loader.SetMaterialAttribute(VertexAttribute::Semantic::Tangent, "VertexTangent");
    loader.SetMaterialAttribute(VertexAttribute::Semantic::Bitangent, "VertexBitangent");
    loader.SetMaterialAttribute(VertexAttribute::Semantic::TexCoord0, "VertexTexCoord");

    // material → uniform
    loader.SetMaterialProperty(ModelLoader::MaterialProperty::DiffuseColor, "Color");
    loader.SetMaterialProperty(ModelLoader::MaterialProperty::DiffuseTexture, "ColorTexture");
    loader.SetMaterialProperty(ModelLoader::MaterialProperty::NormalTexture, "NormalTexture");
    loader.SetMaterialProperty(ModelLoader::MaterialProperty::SpecularTexture, "SpecularTexture");

    return loader;
}

std::shared_ptr<Texture2DObject> SceneViewerApplication::CreateNoiseTexture(unsigned int width, unsigned int height)
{
    // Create the texture object
    std::shared_ptr<Texture2DObject> noiseTexture = std::make_shared<Texture2DObject>();

    // Create a vector to hold the pixel data
    std::vector<float> pixels(height * width);

    // Generate noise values for each pixel
    for (unsigned int j = 0; j < height; ++j)
    {
        for (unsigned int i = 0; i < width; ++i)
        {
            // Generate Perlin noise based on pixel coordinates
            float x = static_cast<float>(i) / (width - 1);
            float y = static_cast<float>(j) / (height - 1);

            // Use STB's Perlin noise for structured noise
            float perlinValue = stb_perlin_fbm_noise3(x * 5.0f, y * 5.0f, 0.0f, 2.0f, 0.5f, 6);

            // Normalize to [0, 1]
            float noiseValue = 0.5f * (perlinValue + 1.0f);

            // Store the noise value
            pixels[j * width + i] = noiseValue;
        }
    }

    // Bind the texture and set its data
    noiseTexture->Bind();
    noiseTexture->SetImage<float>(0, width, height, TextureObject::FormatR, TextureObject::InternalFormatR16F, pixels);
    noiseTexture->GenerateMipmap();

    return noiseTexture;
}

std::shared_ptr<TextureCubemapObject> SceneViewerApplication::GenerateSceneCubemap(
    unsigned int size,
    const Camera& cam,
    SceneModel* skipNode
)
{
    // Get cam pos
    glm::vec3 eyePos = cam.ExtractTranslation();

    // Make and allocate empty cubemap
    auto cubemap = std::make_shared<TextureCubemapObject>();
    cubemap->Bind();

    // Allocate each of the 6 faces
    for (GLuint face = 0; face < 6; ++face) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
            0,                                     
            GL_RGB,                             
            size, size,                            
            0,                                     
            GL_RGB,                                
            GL_UNSIGNED_BYTE,
            nullptr                               
        );
    }

    // Explain more here
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);


    //create a framebuffer and a renderbuffer. Explain more here
    GLuint fbo, rbo;
    glGenFramebuffers(1, &fbo);         
    glGenRenderbuffers(1, &rbo);            

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // Make 90 degree projection and a view for each face.
    glm::mat4 captureProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

    std::array<glm::mat4, 6> captureViews = {
        glm::lookAt(eyePos, eyePos + glm::vec3(1,  0,  0), glm::vec3(0,-1, 0)), // +X or Right
        glm::lookAt(eyePos, eyePos + glm::vec3(-1,  0,  0), glm::vec3(0,-1, 0)), // –X or Left
        glm::lookAt(eyePos, eyePos + glm::vec3(0,  1,  0), glm::vec3(0, 0, 1)), // +Y or Up
        glm::lookAt(eyePos, eyePos + glm::vec3(0, -1,  0), glm::vec3(0, 0,-1)), // –Y or Down
        glm::lookAt(eyePos, eyePos + glm::vec3(0,  0,  1), glm::vec3(0,-1, 0)), // +Z or forward
        glm::lookAt(eyePos, eyePos + glm::vec3(0,  0, -1), glm::vec3(0,-1, 0))  // –Z or backward

    };

    // Camera we render for cube faces from
    auto captureCam = std::make_shared<Camera>();

    // Render into each of the 6 faces of the cubemap
    for (GLuint i = 0; i < 6; ++i) {
        //Attach the i-th face of our cubemap as the target
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            static_cast<GLuint>(cubemap->GetHandle2()),
            0
        );

        // Make sure our viewport matches the cubemap face size, and clear old pixels/depth
        glViewport(0, 0, size, size);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // upload the view+proj to our capture camera to ready it for pictures
        captureCam->SetProjectionMatrix(captureProj);
        captureCam->SetViewMatrix(captureViews[i]);

        // Render from capture cam pov
        m_renderer.SetCurrentCamera(*captureCam);

        // Visit all nodes in the scene and add it the renderer for drawing, except skipnode (the model itself)
        CubeRendererSceneVisitor visitor(m_renderer, captureCam, skipNode);
        m_scene.AcceptVisitor(visitor);

        // Draw on cube face
        m_renderer.Render();
    };
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    cubemap->Bind();
    cubemap->GenerateMipmap();
    int w, h;
    glfwGetFramebufferSize(GetMainWindow().GetInternalWindow(), &w, &h);
    glViewport(0, 0, w, h);

    return cubemap;
}

