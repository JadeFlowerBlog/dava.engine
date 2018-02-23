#include "Scene3D/Systems/RenderUpdateSystem.h"

#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Scene3D/Components/ReflectionComponent.h"
#include "Scene3D/Components/SpeedTreeComponent.h"
#include "Scene3D/Components/MeshComponent.h"


#include "Render/Highlevel/Camera.h"
#include "Render/Highlevel/Frustum.h"
#include "Render/Highlevel/Landscape.h"
#include "Render/Highlevel/RenderBatch.h"
#include "Render/Highlevel/RenderLayer.h"
#include "Render/Highlevel/RenderPass.h"
#include "Render/Highlevel/RenderSystem.h"
#include "Render/Highlevel/ReflectionRenderer.h"
#include "Render/Highlevel/Mesh.h"
#include "Render/Highlevel/SpeedTreeObject.h"

#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Components/SingleComponents/TransformSingleComponent.h"
#include "Scene3D/Components/SwitchComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Lod/LodComponent.h"
#include "Scene3D/Scene.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(RenderUpdateSystem)
{
    ReflectionRegistrator<RenderUpdateSystem>::Begin()[M::Tags("base")]
    .ConstructorByPointer<Scene*>()
    .Method("Process", &RenderUpdateSystem::Process)[M::SystemProcess(SP::Group::ENGINE_END, SP::Type::NORMAL, 7.0f)]
    .End();
}

RenderUpdateSystem::RenderUpdateSystem(Scene* scene)
    : SceneSystem(scene, ComponentUtils::MakeMask<TransformComponent>())
{
    renderSystem = scene->GetRenderSystem();
}

void RenderUpdateSystem::AddEntity(Entity* entity)
{
    for (RenderObject* object : GetRenderObjects(entity))
        RegisterRenderObject(entity, object);
}

void RenderUpdateSystem::RemoveEntity(Entity* entity)
{
    for (RenderObject* object : GetRenderObjects(entity))
        UnregisterRenderObject(entity, object);
}

void RenderUpdateSystem::RegisterComponent(Entity* entity, Component* component)
{
    RegisterRenderObject(entity, GetRenderObject(component));
}

void RenderUpdateSystem::UnregisterComponent(Entity* entity, Component* component)
{
    UnregisterRenderObject(entity, GetRenderObject(component));
}

void RenderUpdateSystem::RegisterRenderObject(Entity* entity, RenderObject* object)
{
    if (object == nullptr)
        return;

    DVASSERT(renderObjects.find(object) == renderObjects.end());

    const Matrix4* worldTransformPointer = entity->GetComponent<TransformComponent>()->GetWorldTransformPtr();
    object->SetWorldTransformPtr(worldTransformPointer);
    UpdateActiveIndexes(entity, object);
    renderObjects.insert(object);
    GetScene()->GetRenderSystem()->RenderPermanent(object);
}

void RenderUpdateSystem::UnregisterRenderObject(Entity* entity, RenderObject* object)
{
    if (object == nullptr)
        return;

    auto found = renderObjects.find(object);
    if (found != renderObjects.end())
    {
        GetScene()->GetRenderSystem()->RemoveFromRender(object);
        renderObjects.erase(object);
    }
}

void RenderUpdateSystem::PrepareForRemove()
{
    RenderSystem* renderSystem = GetScene()->GetRenderSystem();
    for (RenderObject* renderObject : renderObjects)
    {
        renderSystem->RemoveFromRender(renderObject);
    }
    renderObjects.clear();
}

void RenderUpdateSystem::Process(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_RENDER_UPDATE_SYSTEM);

    TransformSingleComponent* tsc = GetScene()->GetSingletonComponent<TransformSingleComponent>();

    for (auto& pair : GetRenderObjects(tsc->worldTransformChanged))
        UpdateRenderObject(pair.first, pair.second);

    RenderSystem* renderSystem = GetScene()->GetRenderSystem();
    renderSystem->SetMainCamera(GetScene()->GetCurrentCamera());
    renderSystem->SetDrawCamera(GetScene()->GetDrawCamera());

    GetScene()->GetRenderSystem()->Update(timeElapsed);
}

void RenderUpdateSystem::UpdateRenderObject(Entity* entity, RenderObject* object)
{
    if (object == nullptr)
        return;

    // Update new transform pointer, and mark that transform is changed
    const Matrix4* worldTransformPointer = entity->GetComponent<TransformComponent>()->GetWorldTransformPtr();
    object->SetWorldTransformPtr(worldTransformPointer);
    entity->GetScene()->renderSystem->MarkForUpdate(object);

    Matrix4 inverseWorldTransform = Matrix4::IDENTITY;
    worldTransformPointer->GetInverse(inverseWorldTransform);
    object->SetInverseTransform(inverseWorldTransform);
}

void RenderUpdateSystem::UpdateActiveIndexes(Entity* entity, RenderObject* object)
{
    Entity* parent;

    // search effective lod index
    parent = entity;
    while (nullptr != parent)
    {
        LodComponent* lc = GetLodComponent(parent);
        if (nullptr != lc)
        {
            object->SetLodIndex(lc->GetCurrentLod());
            break;
        }

        parent = parent->GetParent();
    }

    // search effective switch index
    parent = entity;
    while (nullptr != parent)
    {
        SwitchComponent* sc = GetSwitchComponent(parent);
        if (nullptr != sc)
        {
            object->SetSwitchIndex(sc->GetSwitchIndex());
            break;
        }

        parent = parent->GetParent();
    }
}
};
