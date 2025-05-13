#pragma once

#include <ituGL/application/Application.h>

#include <ituGL/scene/Scene.h>
#include <ituGL/renderer/Renderer.h>
#include <ituGL/camera/CameraController.h>
#include <ituGL/utils/DearImGui.h>
#include <ituGL/shader/ShaderUniformCollection.h>
#include <ituGL/shader/Material.h>
#include <ituGL/asset/ModelLoader.h>
#include <ituGL/renderer/SkyboxRenderPass.h>
#include <ituGL/scene/SceneModel.h>

class TextureCubemapObject;
class Material;

class SceneViewerApplication : public Application
{
public:
    SceneViewerApplication();

protected:
    void Initialize() override;
    void Update() override;
    void Render() override;
    void Cleanup() override;

private:
    void InitializeCamera();
    void InitializeLights();
    void InitializeMaterial();
    void InitializeModels();
    void InitializeRenderer();

    void RenderGUI();

    std::shared_ptr<Material> InitMaterial(Shader& fragmentShader, Shader& vertexShader, ShaderUniformCollection::NameSet& filterUniforms);
    ModelLoader MakeLoader(std::shared_ptr<Material> mat);

    std::shared_ptr<Texture2DObject> CreateNoiseTexture(unsigned int width, unsigned int height);

    std::shared_ptr<ShaderProgram> MakeProgram(Shader& fragmentShader, Shader& vertexShader);

    void SetUniformsForMat(std::shared_ptr<Material> mat);

private:
    // Helper object for debug GUI
    DearImGui m_imGui;

    // Camera controller
    CameraController m_cameraController;

    // Global scene
    Scene m_scene;

    // Renderer
    Renderer m_renderer;

    // Skybox texture
    std::shared_ptr<TextureCubemapObject> m_skyboxTexture;

    SkyboxRenderPass* m_skyboxPass;

    // Default material
    std::shared_ptr<Material> m_defaultMaterial;

    // My invis material
    std::shared_ptr<Material> m_invisMaterial;

    std::shared_ptr<Texture2DObject> m_noiseMap;

    std::vector<std::shared_ptr<SceneModel>> m_refractiveObjects;

    bool m_outline = false;

    bool m_flicker = false;

    bool m_refract = false;

    float m_outlineStr = 3.0f;

    float m_maxVisDist = 5.0f;

    float m_flickerSpeed = 10.0f;

    float m_flickerSize = 5.0f;
    
    float m_flickerChaos = 20.0f;

    float m_flickerThreshold = 0.9f;

    float m_IOR = 0.95f;

    std::shared_ptr<TextureCubemapObject> GenerateSceneCubemap(unsigned int size, const glm::vec3& center);

    std::shared_ptr<TextureCubemapObject> m_objectCubemap;

    float m_skyboxMaxLod = false;
};
