// CubeRendererSceneVisitor.h
#pragma once

#include <memory>                         
#include <ituGL/scene/SceneVisitor.h>
#include <ituGL/scene/SceneModel.h>

class Renderer;
class Camera;                             
class SceneCamera;
class SceneLight;
class SceneModel;

class CubeRendererSceneVisitor : public SceneVisitor
{
public:
    CubeRendererSceneVisitor(Renderer& renderer,
        std::shared_ptr<Camera> captureCam,
        SceneModel* toSkip);

    void VisitCamera(SceneCamera& sceneCamera) override;
    void VisitLight(SceneLight& sceneLight)   override;
    void VisitModel(SceneModel& sceneModel)   override;

private:
    Renderer& m_renderer;
    std::shared_ptr<Camera>    m_captureCam; 
    SceneModel* m_skipModel;
};
