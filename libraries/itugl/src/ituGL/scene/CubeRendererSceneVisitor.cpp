#include <ituGL/scene/CubeRendererSceneVisitor.h>

#include <ituGL/renderer/Renderer.h>
#include <ituGL/scene/SceneCamera.h>
#include <ituGL/scene/SceneLight.h>
#include <ituGL/scene/SceneModel.h>
#include <ituGL/scene/Transform.h>

CubeRendererSceneVisitor::CubeRendererSceneVisitor(
    Renderer& renderer,
    std::shared_ptr<Camera> captureCam,
    SceneModel* toSkip)
    : m_renderer(renderer)
    , m_captureCam(std::move(captureCam))
    , m_skipModel(toSkip)
{
    m_renderer.SetCurrentCamera(*m_captureCam);
}

void CubeRendererSceneVisitor::VisitCamera(SceneCamera& sceneCamera)
{
}

void CubeRendererSceneVisitor::VisitLight(SceneLight& sceneLight)
{
    m_renderer.AddLight(*sceneLight.GetLight());
}

void CubeRendererSceneVisitor::VisitModel(SceneModel& sceneModel)
{
    if (&sceneModel == m_skipModel)  // skip this one
        return;
    assert(sceneModel.GetTransform());
    m_renderer.AddModel(*sceneModel.GetModel(), sceneModel.GetTransform()->GetTransformMatrix());
}
