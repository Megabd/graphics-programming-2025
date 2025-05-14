// CubeRendererSceneVisitor.h
#pragma once

#include <memory>                         // ← for std::shared_ptr
#include <ituGL/scene/SceneVisitor.h>
#include <ituGL/scene/SceneModel.h>

class Renderer;
class Camera;                             // ← forward‐declare Camera itself
class SceneCamera;
class SceneLight;
class SceneModel;

class CubeRendererSceneVisitor : public SceneVisitor
{
public:
    // declaration only – no initializer list here!
    CubeRendererSceneVisitor(Renderer& renderer,
        std::shared_ptr<Camera> captureCam,
        SceneModel* toSkip);

    void VisitCamera(SceneCamera& sceneCamera) override;
    void VisitLight(SceneLight& sceneLight)   override;
    void VisitModel(SceneModel& sceneModel)   override;

private:
    Renderer& m_renderer;
    std::shared_ptr<Camera>    m_captureCam;  // store it so we can use it
    SceneModel* m_skipModel;
};
