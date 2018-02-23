#include "Scene3D/Components/VisualScriptComponent.h"
#include "VisualScript/VisualScript.h"

#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"
#include "FileSystem/KeyedArchive.h"
#include "Engine/Engine.h"

namespace DAVA
{
REGISTER_CLASS(VisualScriptComponent)
DAVA_VIRTUAL_REFLECTION_IMPL(VisualScriptComponent)
{
    ReflectionRegistrator<VisualScriptComponent>::Begin()
    .ConstructorByPointer()
    .Field("scriptfilepath", &VisualScriptComponent::GetScriptFilepath, &VisualScriptComponent::SetScriptFilepath)[M::DisplayName("Script Filepath")]
    .End();
}

VisualScriptComponent::VisualScriptComponent()
{
}

VisualScriptComponent::~VisualScriptComponent()
{
    SafeDelete(script);
}

void VisualScriptComponent::SetScriptFilepath(const FilePath& scriptFilepath_)
{
    if (scriptFilepath != scriptFilepath_)
    {
        scriptFilepath = scriptFilepath_;

        SafeDelete(script);
        script = new VisualScript();
        script->Load(scriptFilepath);
    }
}

Component* VisualScriptComponent::Clone(Entity* toEntity)
{
    VisualScriptComponent* visualScriptComponent = new VisualScriptComponent();
    visualScriptComponent->SetEntity(toEntity);
    visualScriptComponent->SetScriptFilepath(scriptFilepath);
    return visualScriptComponent;
}

void VisualScriptComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);

    if (nullptr != archive)
    {
        String relativePathname = scriptFilepath.GetRelativePathname(serializationContext->GetScenePath());
        archive->SetString("vsc.scriptfilepath", relativePathname);
    }
}

void VisualScriptComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    if (nullptr != archive)
    {
        String relativePathname = archive->GetString("vsc.scriptfilepath", "");
        if (!relativePathname.empty())
        {
            FilePath filepath = serializationContext->GetScenePath() + relativePathname;
            SetScriptFilepath(filepath);
        }
    }

    Component::Deserialize(archive, serializationContext);
}

VisualScript* VisualScriptComponent::GetScript()
{
    return script;
}
}
