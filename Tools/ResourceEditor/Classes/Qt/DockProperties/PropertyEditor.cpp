/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/


#include "DAVAEngine.h"
#include "Entity/Component.h"
#include "Main/mainwindow.h"

#include <QPushButton>
#include <QFile>
#include <QTextStream>
#include <QDebug>

#include "DockProperties/PropertyEditor.h"
#include "MaterialEditor/MaterialEditor.h"
#include "Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataIntrospection.h"
#include "Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataDavaVariant.h"
#include "Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataDavaKeyedArchive.h"
#include "Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataKeyedArchiveMember.h"
#include "Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataInspColl.h"
#include "Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataInspMember.h"
#include "Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataInspDynamic.h"
#include "Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataMetaObject.h"
#include "Tools/QtPropertyEditor/QtPropertyDataValidator/HeightmapValidator.h"
#include "Tools/QtPropertyEditor/QtPropertyDataValidator/TexturePathValidator.h"
#include "Tools/QtPropertyEditor/QtPropertyDataValidator/ScenePathValidator.h"
#include "Commands2/MetaObjModifyCommand.h"
#include "Commands2/InspMemberModifyCommand.h"
#include "Commands2/ConvertToShadowCommand.h"
#include "Commands2/DeleteRenderBatchCommand.h"
#include "Commands2/CloneLastBatchCommand.h"
#include "Commands2/AddComponentCommand.h"
#include "Commands2/RemoveComponentCommand.h"
#include "Commands2/RebuildTangentSpaceCommand.h"
#include "Qt/Settings/SettingsManager.h"
#include "Project/ProjectManager.h"

#include "PropertyEditorStateHelper.h"
#include "Qt/Project/ProjectManager.h"

#include "ActionComponentEditor.h"
#include "SoundComponentEditor/SoundComponentEditor.h"

#include "Scene3D/Components/Controller/SnapToLandscapeControllerComponent.h"

#include "Scene/System/PathSystem.h"

#include "Deprecated/SceneValidator.h"

#include "Tools/PathDescriptor/PathDescriptor.h"
#include "Tools/LazyUpdater/LazyUpdater.h"
#include "QtTools/WidgetHelpers/SharedIcon.h"

namespace PropertyEditorDetails
{
void ExecuteCommands(DAVA::Vector<Command2 *> && commands, const DAVA::String & commandName, SceneEditor2 * scene)
{
    const bool usebatch = commands.size() > 1;
    if (usebatch)
    {
        scene->BeginBatch(commandName);
    }

    for (Command2 * cmd : commands)
    {
        scene->Exec(cmd);
    }

    if (usebatch)
    {
        scene->EndBatch();
    }
}
}

PropertyEditor::PropertyEditor(QWidget *parent /* = 0 */, bool connectToSceneSignals /*= true*/)
	: QtPropertyEditor(parent)
	, viewMode(VIEW_NORMAL)
	, treeStateHelper(this, curModel)
{
	Function<void()> fn(this, &PropertyEditor::ResetProperties);
	propertiesUpdater = new LazyUpdater(fn, this);

	if(connectToSceneSignals)
	{
		QObject::connect(SceneSignals::Instance(), SIGNAL(Activated(SceneEditor2 *)), this, SLOT(sceneActivated(SceneEditor2 *)));
		QObject::connect(SceneSignals::Instance(), SIGNAL(Deactivated(SceneEditor2 *)), this, SLOT(sceneDeactivated(SceneEditor2 *)));
		QObject::connect(SceneSignals::Instance(), SIGNAL(CommandExecuted(SceneEditor2 *, const Command2*, bool)), this, SLOT(CommandExecuted(SceneEditor2 *, const Command2*, bool )));
		QObject::connect(SceneSignals::Instance(), SIGNAL(SelectionChanged(SceneEditor2 *, const EntityGroup *, const EntityGroup *)), this, SLOT(sceneSelectionChanged(SceneEditor2 *, const EntityGroup *, const EntityGroup *)));
	}
	posSaver.Attach(this, "DocPropetyEditor");

	DAVA::VariantType v = posSaver.LoadValue("splitPos");
	if(v.GetType() == DAVA::VariantType::TYPE_INT32)
	{
        header()->resizeSection( 0, v.AsInt32() );
    }

    Ui::MainWindow* mainUi = QtMainWindow::Instance()->GetUI();
    connect( mainUi->actionAddActionComponent, SIGNAL( triggered() ), SLOT( OnAddActionComponent() ) );
    connect(mainUi->actionAddQualitySettingsComponent, SIGNAL(triggered()), SLOT(OnAddModelTypeComponent()));
    connect(mainUi->actionAddStaticOcclusionComponent, SIGNAL(triggered()), SLOT(OnAddStaticOcclusionComponent()));
    connect(mainUi->actionAddSoundComponent, SIGNAL(triggered()), this, SLOT(OnAddSoundComponent()));
    connect(mainUi->actionAddWaveComponent, SIGNAL(triggered()), SLOT(OnAddWaveComponent()));
    connect(mainUi->actionAddSkeletonComponent, SIGNAL(triggered()), SLOT(OnAddSkeletonComponent()));
    connect(mainUi->actionAddPathComponent, SIGNAL(triggered()), SLOT(OnAddPathComponent()));
    connect(mainUi->actionAddRotationComponent, SIGNAL(triggered()), SLOT(OnAddRotationControllerComponent()));
    connect(mainUi->actionAddSnapToLandscapeComponent, SIGNAL(triggered()), SLOT(OnAddSnapToLandscapeControllerComponent()));
    connect(mainUi->actionAddWASDComponent, SIGNAL(triggered()), SLOT(OnAddWASDControllerComponent()));

	SetUpdateTimeout(5000);
	SetEditTracking(true);
	setMouseTracking(true);

	LoadScheme("~doc:/PropEditorDefault.scheme");
}

PropertyEditor::~PropertyEditor()
{
	DAVA::VariantType v(header()->sectionSize(0));
	posSaver.SaveValue("splitPos", v);

    ClearCurrentNodes();
}

void PropertyEditor::SetEntities(const EntityGroup *selected)
{
    ClearCurrentNodes();
    SCOPE_EXIT
    {
        ResetProperties();
        SaveScheme("~doc:/PropEditorDefault.scheme");
    };

    if (selected == nullptr || selected->IsEmpty())
        return;

    curNodes.reserve(selected->Size());
    for (const EntityGroup::EntityMap::value_type & mapNode : selected->GetContent())
    {
        DAVA::Entity* node = SafeRetain(mapNode.first);
        curNodes << node;
        // ensure that custom properties exist
        // this call will create them if they are not created yet
        GetOrCreateCustomProperties(node);
    }
}

void PropertyEditor::SetViewMode(eViewMode mode)
{
	if(viewMode != mode)
	{
		viewMode = mode;
        ResetProperties();
	}
}

PropertyEditor::eViewMode PropertyEditor::GetViewMode() const
{
	return viewMode;
}

void PropertyEditor::SetFavoritesEditMode(bool set)
{
	if(favoritesEditMode != set)
	{
		favoritesEditMode = set;
		ResetProperties();
	}
}

bool PropertyEditor::GetFavoritesEditMode() const
{
	return favoritesEditMode;
}

void PropertyEditor::ClearCurrentNodes()
{
    for ( int i = 0; i < curNodes.size(); i++ )
	    SafeRelease(curNodes[i]);
    curNodes.clear();
}

void PropertyEditor::ResetProperties()
{
    // Store the current Property Editor Tree state before switching to the new node.
	// Do not clear the current states map - we are using one storage to share opened
	// Property Editor nodes between the different Scene Nodes.
	treeStateHelper.SaveTreeViewState(false);

	RemovePropertyAll();
	favoriteGroup = NULL;

    const int nNodes = curNodes.size();
    if(nNodes > 0)
	{
		// create data tree, but don't add it to the property editor
		TPropertyPtr root(new QtPropertyData(DAVA::FastName("root")));

		// add info about current entities
        for (int i = 0; i < nNodes; i++)
        {
            DAVA::Entity *node = curNodes.at(i);
            const DAVA::InspInfo * inspInfo = node->GetTypeInfo();
            TPropertyPtr curEntityData(CreateInsp(inspInfo->Name(), node, inspInfo));

            PropEditorUserData* userData = GetUserData(curEntityData.get());
            userData->entity = node;

            root->MergeChild(std::move(curEntityData));

		    // add info about components
            for (int ic = 0; ic < Component::COMPONENT_COUNT; ic++)
            {
                const int nComponents = node->GetComponentCount(ic);
                for ( int cidx = 0; cidx < nComponents; cidx++ )
                {
                    Component *component = node->GetComponent(ic, cidx);
			        if (component)
			        {
                        const DAVA::InspInfo * componentInspInfo = component->GetTypeInfo();
				        TPropertyPtr componentData(CreateInsp(componentInspInfo->Name(), component, componentInspInfo));
                        PropEditorUserData* userData = GetUserData(componentData.get());
                        userData->entity = node;

                        bool isRemovable = true;
                        switch (component->GetType())
                        {
                        case Component::STATIC_OCCLUSION_DEBUG_DRAW_COMPONENT:
                        case Component::DEBUG_RENDER_COMPONENT:
                        case Component::TRANSFORM_COMPONENT:
                        case Component::CUSTOM_PROPERTIES_COMPONENT:    // Disable removing, because custom properties are created automatically
                        case Component::WAYPOINT_COMPONENT:             // disable remove, b/c waypoint entity doesn't make sence without waypoint component
                        case Component::EDGE_COMPONENT:                 // disable remove, b/c edge has to be removed directly from scene only
                            isRemovable = false;
                            break;
                        }

                        if (isRemovable)
                        {
                            QtPropertyToolButton* deleteButton = CreateButton(componentData.get(), SharedIcon(":/QtIcons/remove.png"), "Remove Component");
                            deleteButton->setObjectName("RemoveButton");
                            deleteButton->setEnabled(true);
				            QObject::connect(deleteButton, SIGNAL(clicked()), this, SLOT(OnRemoveComponent()));
                        }

                        if ( i == 0 )
                        {
                            root->ChildAdd(std::move(componentData));
                        }
                        else
                        {
                            root->MergeChild(std::move(componentData));
                        }
			        }
                }
            }
        }

        ApplyFavorite(root.get());
        ApplyModeFilter(root.get());
        ApplyModeFilter(favoriteGroup);
        ApplyCustomExtensions(root.get());

        DAVA::Vector<TPropertyPtr> properies;
        root->ChildrenExtract(properies);
        for (TPropertyPtr & item : properies)
        {
            ApplyStyle(item.get(), QtPropertyEditor::HEADER_STYLE);
        }
        AppendProperties(std::move(properies));
        FinishTreeCreation();
	}
    
	// Restore back the tree view state from the shared storage.
	if (!treeStateHelper.IsTreeStateStorageEmpty())
	{
		treeStateHelper.RestoreTreeViewState();
	}
	else
	{
		// Expand the root elements as default value.
		expandToDepth(0);
	}
}

void PropertyEditor::ApplyModeFilter(QtPropertyData * parent)
{
	if(NULL != parent)
	{
		for(int i = 0; i < parent->ChildCount(); ++i)
		{
			bool toBeRemove = false;
			bool scanChilds = true;
			const TPropertyPtr & data = parent->ChildGet(i);

			// show only editable items and favorites
			if(viewMode == VIEW_NORMAL)
			{
				if(!data->IsEditable())
				{
					toBeRemove = true;
				}
			}
			// show all editable/viewable items
			else if(viewMode == VIEW_ADVANCED)
			{

			}
			// show only favorite items
			else if(viewMode == VIEW_FAVORITES_ONLY)
			{
				QtPropertyData *favorite = NULL;
				PropEditorUserData *userData = GetUserData(data.get());

				if(userData->type == PropEditorUserData::ORIGINAL)
				{
					toBeRemove = true;
					favorite = userData->associatedData;
				}
				else if(userData->type == PropEditorUserData::COPY)
				{
					favorite = data.get();
					scanChilds = false;
				}

				if(favorite != nullptr && favorite->GetUserData() != nullptr)
				{
					// remove from favorite data back link to the original data
					// because original data will be removed from properties
                    static_cast<PropEditorUserData *>(favorite->GetUserData())->associatedData = nullptr;
				}
			}

			if(toBeRemove)
			{
				parent->ChildRemove(data.get());
				i--;
			}
			else if(scanChilds)
			{
				// apply mode to data childs
				ApplyModeFilter(data.get());
			}
		}
	}
}

void PropertyEditor::ApplyFavorite(QtPropertyData* data)
{
	if(NULL != data)
	{
		if(scheme.contains(data->GetPath()))
		{
			SetFavorite(data, true);
		}

		// go through childs
		for(int i = 0; i < data->ChildCount(); ++i)
		{
			ApplyFavorite(data->ChildGet(i).get());
		}
	}
}

void PropertyEditor::ApplyCustomExtensions(QtPropertyData* data)
{
	if(NULL != data)
	{
		const DAVA::MetaInfo *meta = data->MetaInfo();
        const bool isSingleSelection = data->HasMergedData();

        if(NULL != meta)
		{
			if(DAVA::MetaInfo::Instance<DAVA::ActionComponent>() == meta)
			{
				// Add optional button to edit action component
                QtPropertyToolButton* editActions = CreateButton(data, SharedIcon(":/QtIcons/settings.png"), "Edit action component");
                editActions->setEnabled(isSingleSelection);
				QObject::connect(editActions, SIGNAL(clicked()), this, SLOT(ActionEditComponent()));
			}
            else if(DAVA::MetaInfo::Instance<DAVA::SoundComponent>() == meta)
            {
                QtPropertyToolButton* editSound = CreateButton(data, SharedIcon(":/QtIcons/settings.png"), "Edit sound component");
                editSound->setAutoRaise(true);
                QObject::connect(editSound, SIGNAL(clicked()), this, SLOT(ActionEditSoundComponent()));
            }
            else if(DAVA::MetaInfo::Instance<DAVA::WaveComponent>() == meta)
            {
                QtPropertyToolButton *triggerWave = data->AddButton();
                triggerWave->setIcon(SharedIcon(":/QtIcons/clone.png"));
                triggerWave->setAutoRaise(true);

                QObject::connect(triggerWave, SIGNAL(clicked()), this, SLOT(OnTriggerWaveComponent()));
            }
			else if(DAVA::MetaInfo::Instance<DAVA::RenderObject>() == meta)
			{
                QtPropertyDataIntrospection *introData = dynamic_cast<QtPropertyDataIntrospection *>(data);
                if(NULL != introData)
                {
                    DAVA::RenderObject *renderObject = (DAVA::RenderObject *) introData->object;
                    if(SceneValidator::IsObjectHasDifferentLODsCount(renderObject))
                    {
                        QtPropertyToolButton* cloneBatches = CreateButton(data, SharedIcon(":/QtIcons/clone_batches.png"), "Clone batches for LODs correction");
                        cloneBatches->setEnabled(isSingleSelection);
                        QObject::connect(cloneBatches, SIGNAL(clicked()), this, SLOT(CloneRenderBatchesToFixSwitchLODs()));
                    }
                }
			}
			else if(DAVA::MetaInfo::Instance<DAVA::RenderBatch>() == meta)
			{
                QtPropertyToolButton* deleteButton = CreateButton(data, SharedIcon(":/QtIcons/remove.png"), "Delete RenderBatch");
                deleteButton->setEnabled(isSingleSelection);
				QObject::connect(deleteButton, SIGNAL(clicked()), this, SLOT(DeleteRenderBatch()));

				QtPropertyDataIntrospection *introData = dynamic_cast<QtPropertyDataIntrospection *>(data);
				if(NULL != introData)
				{
					DAVA::RenderBatch *batch = (DAVA::RenderBatch *) introData->object;
                    if (batch != NULL)
                    {
					    DAVA::RenderObject *ro = batch->GetRenderObject();
					    if (ro != NULL && ConvertToShadowCommand::CanConvertBatchToShadow(batch))
					    {
                            QtPropertyToolButton* convertButton = CreateButton(data, SharedIcon(":/QtIcons/shadow.png"), "Convert To ShadowVolume");
                            convertButton->setEnabled(isSingleSelection);
						    connect(convertButton, SIGNAL(clicked()), this, SLOT(ConvertToShadow()));
					    }

                        PolygonGroup* group = batch->GetPolygonGroup();
                        if (group != NULL)
                        {
                            bool isRebuildTsEnabled = true;
                            const int32 requiredVertexFormat = (EVF_TEXCOORD0 | EVF_NORMAL);
                            isRebuildTsEnabled &= (group->GetPrimitiveType() == rhi::PRIMITIVE_TRIANGLELIST);
                            isRebuildTsEnabled &= ((group->GetFormat() & requiredVertexFormat) == requiredVertexFormat);

                            if (isRebuildTsEnabled)
                            {
                                QtPropertyToolButton* rebuildTangentButton = CreateButton(data, SharedIcon(":/QtIcons/external.png"), "Rebuild tangent space");
                                rebuildTangentButton->setEnabled(isSingleSelection);
                                connect(rebuildTangentButton, SIGNAL(clicked()), this, SLOT(RebuildTangentSpace()));
                            }
                        }
                    }
				}
			}
			else if(DAVA::MetaInfo::Instance<DAVA::NMaterial>() == meta)
			{
                QtPropertyToolButton* goToMaterialButton = CreateButton(data, SharedIcon(":/QtIcons/3d.png"), "Edit material");
                goToMaterialButton->setEnabled(isSingleSelection);
				QObject::connect(goToMaterialButton, SIGNAL(clicked()), this, SLOT(ActionEditMaterial()));
			}
            else if(DAVA::MetaInfo::Instance<DAVA::FilePath>() == meta)
			{
				QString dataName(data->GetName().c_str());
                PathDescriptor *pathDescriptor = &PathDescriptor::descriptors[0];

                for (auto descriptor : PathDescriptor::descriptors)
				{
                    if(descriptor.pathName == dataName)
					{
                        pathDescriptor = &descriptor;
						break;
					}
				}


				QtPropertyDataDavaVariant* variantData = static_cast<QtPropertyDataDavaVariant*>(data);
				QString defaultPath = GetDefaultFilePath();
				variantData->SetDefaultOpenDialogPath(defaultPath);
				variantData->SetOpenDialogFilter(pathDescriptor->fileFilter);

				QStringList pathList;
				pathList.append(defaultPath);

				switch(pathDescriptor->pathType)
				{
					case PathDescriptor::PATH_HEIGHTMAP:
						variantData->SetValidator(new HeightMapValidator(pathList));
						break;
					case PathDescriptor::PATH_TEXTURE:
						variantData->SetValidator(new TexturePathValidator(pathList));
						break;
					case PathDescriptor::PATH_IMAGE:
					case PathDescriptor::PATH_TEXTURE_SHEET:
						variantData->SetValidator(new PathValidator(pathList));
						break;
                    case PathDescriptor::PATH_SCENE:
                        variantData->SetValidator(new ScenePathValidator(pathList));
                        break;

					default:
						break;
				}
            }
            else if(DAVA::MetaInfo::Instance<DAVA::QualitySettingsComponent>() == meta)
            {
                DAVA::QualitySettingsSystem* qss = DAVA::QualitySettingsSystem::Instance();
                QtPropertyDataDavaVariant *modelTypeData = (QtPropertyDataDavaVariant *)data->ChildGet(DAVA::FastName("modelType"));
                if(NULL != modelTypeData)
                {
                    modelTypeData->AddAllowedValue(DAVA::VariantType(DAVA::FastName()), "Undefined");
                    for(int i = 0; i < qss->GetOptionsCount(); ++i)
                    {
                        modelTypeData->AddAllowedValue(DAVA::VariantType(qss->GetOptionName(i)));
                    }
                }

                QtPropertyDataDavaVariant *groupData = (QtPropertyDataDavaVariant *)data->ChildGet(DAVA::FastName("requiredGroup"));
                if(NULL != groupData)
                {
                    groupData->AddAllowedValue(DAVA::VariantType(DAVA::FastName()), "Undefined");
                    for(size_t i = 0; i < qss->GetMaterialQualityGroupCount(); ++i)
                    {
                        groupData->AddAllowedValue(DAVA::VariantType(qss->GetMaterialQualityGroupName(i)));
                    }
                }

                DAVA::FastName curGroup = groupData->GetVariantValue().AsFastName();

                QtPropertyDataDavaVariant *requiredQualityData = (QtPropertyDataDavaVariant *)data->ChildGet(DAVA::FastName("requiredQuality"));
                if(NULL != requiredQualityData)
                {
                    requiredQualityData->AddAllowedValue(DAVA::VariantType(DAVA::FastName()), "Undefined");
                    for(size_t i = 0; i < qss->GetMaterialQualityCount(curGroup); ++i)
                    {
                        requiredQualityData->AddAllowedValue(DAVA::VariantType(qss->GetMaterialQualityName(curGroup, i)));
                    }
                }
            }
		}

		// go through childs
		for(int i = 0; i < data->ChildCount(); ++i)
		{
			ApplyCustomExtensions(data->ChildGet(i).get());
		}
	}
}

QtPropertyData* PropertyEditor::CreateInsp(const DAVA::FastName& name, void *object, const DAVA::InspInfo *info)
{
	QtPropertyData *ret = NULL;

	if(NULL != info)
	{
		bool hasMembers = false;
		const InspInfo *baseInfo = info;

		// check if there are any members in introspection
		while(NULL != baseInfo)
		{
			if(baseInfo->MembersCount() > 0)
			{
				hasMembers = true;
				break;
			}
			baseInfo = baseInfo->BaseInfo();
		}

        ret = new QtPropertyDataIntrospection(name, object, info, false);

		// add members is there are some
        // and if we allow to view such introspection type
		if(hasMembers && IsInspViewAllowed(info))
		{
			while(NULL != baseInfo)
			{
				for(int i = 0; i < baseInfo->MembersCount(); ++i)
				{
					const DAVA::InspMember *member = baseInfo->Member(i);

                    TPropertyPtr memberData(CreateInspMember(member->Name(), object, member));
					ret->ChildAdd(std::move(memberData));
				}

				baseInfo = baseInfo->BaseInfo();
			}
		}
    }

	return ret;
}

QtPropertyData* PropertyEditor::CreateInspMember(const DAVA::FastName& name, void *object, const DAVA::InspMember *member)
{
	QtPropertyData* ret = NULL;

	if(NULL != member && (member->Flags() & DAVA::I_VIEW))
	{
		void *momberObject = member->Data(object);
		const DAVA::InspInfo *memberIntrospection = member->Type()->GetIntrospection(momberObject);
		bool isKeyedArchive = (member->Type() == DAVA::MetaInfo::Instance<DAVA::KeyedArchive*>());

		if(NULL != memberIntrospection && !isKeyedArchive)
		{
            ret = CreateInsp(name, momberObject, memberIntrospection);
		}
		else
		{
			if(member->Collection() && !isKeyedArchive)
			{
                ret = CreateInspCollection(name, momberObject, member->Collection());
			}
			else
			{
                ret = QtPropertyDataIntrospection::CreateMemberData(name, object, member);
			}
		}
	}

	return ret;
}

QtPropertyData* PropertyEditor::CreateInspCollection(const DAVA::FastName& name, void *object, const DAVA::InspColl *collection)
{
	QtPropertyData *ret = new QtPropertyDataInspColl(name, object, collection, false);

	if(NULL != collection && collection->Size(object) > 0)
	{
		int index = 0;
		DAVA::MetaInfo *valueType = collection->ItemType();
		DAVA::InspColl::Iterator i = collection->Begin(object);
		while(NULL != i)
		{
            DAVA::FastName childName(std::to_string(index));
			if(NULL != valueType->GetIntrospection())
			{
				void * itemObject = collection->ItemData(i);
				const DAVA::InspInfo *itemInfo = valueType->GetIntrospection(itemObject);

                TPropertyPtr inspData(CreateInsp(childName, itemObject, itemInfo));
				ret->ChildAdd(std::move(inspData));
			}
			else
			{
				if(!valueType->IsPointer())
				{
                    TPropertyPtr childData(new QtPropertyDataMetaObject(childName, collection->ItemPointer(i), valueType));
					ret->ChildAdd(std::move(childData));
				}
				else
				{
                    DAVA::FastName localChildName = childName;
                    if (collection->ItemKeyType() == DAVA::MetaInfo::Instance<DAVA::FastName>())
                    {
                        localChildName = *reinterpret_cast<const DAVA::FastName *>(collection->ItemKeyData(i));
                    }
					QString s;
                    TPropertyPtr childData(new QtPropertyData(localChildName, s.sprintf("[%p] Pointer", collection->ItemData(i))));
					childData->SetEnabled(false);
					ret->ChildAdd(std::move(childData));
				}
			}

			index++;
			i = collection->Next(i);
		}
	}

	return ret;
}

QtPropertyData* PropertyEditor::CreateClone(QtPropertyData *original)
{
	QtPropertyDataIntrospection *inspData = dynamic_cast<QtPropertyDataIntrospection *>(original);
	if(NULL != inspData)
	{
        return CreateInsp(original->GetName(), inspData->object, inspData->info);
	}

	QtPropertyDataInspMember *memberData = dynamic_cast<QtPropertyDataInspMember *>(original);
	if(NULL != memberData)
	{
        return CreateInspMember(original->GetName(), memberData->object, memberData->member);
	}

	QtPropertyDataInspDynamic *memberDymanic = dynamic_cast<QtPropertyDataInspDynamic *>(original);
	if(NULL != memberData)
	{
        return CreateInspMember(original->GetName(), memberDymanic->ddata.object, memberDymanic->dynamicInfo->GetMember());
    }

    QtPropertyDataMetaObject *metaData  = dynamic_cast<QtPropertyDataMetaObject *>(original);
	if(NULL != metaData)
	{
        return new QtPropertyDataMetaObject(original->GetName(), metaData->object, metaData->meta);
	}

	QtPropertyDataInspColl *memberCollection = dynamic_cast<QtPropertyDataInspColl *>(original);
	if(NULL != memberCollection)
	{
        return CreateInspCollection(original->GetName(), memberCollection->object, memberCollection->collection);
	}

	QtPropertyDataDavaKeyedArcive *memberArch = dynamic_cast<QtPropertyDataDavaKeyedArcive *>(original);
	if(NULL != memberArch)
	{
        return new QtPropertyDataDavaKeyedArcive(original->GetName(), memberArch->archive);
	}

	QtPropertyKeyedArchiveMember *memberArchMem = dynamic_cast<QtPropertyKeyedArchiveMember *>(original);
	if(NULL != memberArchMem)
	{
        return new QtPropertyKeyedArchiveMember(original->GetName(), memberArchMem->archive, memberArchMem->key);
	}

    QtPropertyData * retValue = new QtPropertyData(original->GetName(), QString(original->GetName().c_str()));
    retValue->SetFlags(original->GetFlags());
    return retValue;
}

void PropertyEditor::sceneActivated(SceneEditor2 *scene)
{
	if(NULL != scene)
	{
        SetEntities(&scene->selectionSystem->GetSelection());
    }
}

void PropertyEditor::sceneDeactivated(SceneEditor2 *scene)
{
	SetEntities(NULL);
}

void PropertyEditor::sceneSelectionChanged(SceneEditor2 *scene, const EntityGroup *selected, const EntityGroup *deselected)
{
    SetEntities(selected);
}

void PropertyEditor::CommandExecuted(SceneEditor2 *scene, const Command2* command, bool redo)
{
	int cmdId = command->GetId();

	switch (cmdId)
	{
	case CMDID_COMPONENT_ADD:
	case CMDID_COMPONENT_REMOVE:
	case CMDID_CONVERT_TO_SHADOW:
	case CMDID_PARTICLE_EMITTER_LOAD_FROM_YAML:
    case CMDID_SOUND_ADD_EVENT:
    case CMDID_SOUND_REMOVE_EVENT:
	case CMDID_DELETE_RENDER_BATCH:
	case CMDID_CLONE_LAST_BATCH:
    case CMDID_EXPAND_PATH:
    case CMDID_COLLAPSE_PATH:
        {
            bool doReset = (command->GetEntity() == NULL);
            for ( int i = 0; !doReset && i < curNodes.size(); i++ )
            {
                if (command->GetEntity() == curNodes.at(i))
                {
                    doReset = true;
                }
            }
            if (doReset)
            {
				propertiesUpdater->Update();
            }
            break;
        }
	default:
		OnUpdateTimeout();
		break;
	}
}

void PropertyEditor::OnItemEdited(const QModelIndex &index) // TODO: fix undo/redo
{
	QtPropertyEditor::OnItemEdited(index);

	SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();
    if (curScene == NULL)
        return ;
	QtPropertyData *propData = GetProperty(index);

	if(NULL != propData)
	{
        bool useBatch = propData->HasMergedData();
        if (useBatch)
        {
            curScene->BeginBatch("");
        }
        
        curScene->Exec(reinterpret_cast<Command2 *>(propData->CreateLastCommand()));
        propData->ForeachMergedItem([&curScene](const TPropertyPtr & item)
        {
            curScene->Exec(reinterpret_cast<Command2 *>(item->CreateLastCommand()));
            return true;
        });
        
        if (useBatch)
        {
            curScene->EndBatch();
        }
	}

    // this code it used to reload QualitySettingComponent field, when some changes made by user
    // because of QualitySettingComponent->requiredQuality directly depends from QualitySettingComponent->requiredGroup
    if(propData->GetName() == DAVA::FastName("requiredGroup"))
    {
        QtPropertyDataDavaVariant *requiredQualityData = (QtPropertyDataDavaVariant *)propData->Parent()->ChildGet(DAVA::FastName("requiredQuality"));
        if(NULL != requiredQualityData)
        {
            requiredQualityData->ClearAllowedValues();

            DAVA::FastName curGroup = ((QtPropertyDataDavaVariant *)propData)->GetVariantValue().AsFastName();
            requiredQualityData->AddAllowedValue(DAVA::VariantType(DAVA::FastName()), "Undefined");
            for(size_t i = 0; i < DAVA::QualitySettingsSystem::Instance()->GetMaterialQualityCount(curGroup); ++i)
            {
                requiredQualityData->AddAllowedValue(DAVA::VariantType(DAVA::QualitySettingsSystem::Instance()->GetMaterialQualityName(curGroup, i)));
            }
        }

        propData->Parent()->EmitDataChanged(QtPropertyData::VALUE_SOURCE_CHANGED);
    }
}

void PropertyEditor::mouseReleaseEvent(QMouseEvent *event)
{
	bool skipEvent = false;
	QModelIndex index = indexAt(event->pos());

	// handle favorite state toggle for item under mouse
	if(favoritesEditMode && index.parent().isValid() && index.column() == 0)
	{
		QRect rect = visualRect(index);
		rect.setX(0);
		rect.setWidth(16);

		if(rect.contains(event->pos()))
		{
			QtPropertyData *data = GetProperty(index);
			if(NULL != data && !IsParentFavorite(data))
			{
				SetFavorite(data, !IsFavorite(data));
				skipEvent = true;
			}
		}
	}

	if(!skipEvent)
	{
		QtPropertyEditor::mouseReleaseEvent(event);
	}
}

void PropertyEditor::drawRow(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    // custom draw for favorites edit mode
	QStyleOptionViewItemV4 opt = option;
	if(index.parent().isValid() && favoritesEditMode)
	{
		QtPropertyData *data = GetProperty(index);
		if(NULL != data)
		{
			if(!IsParentFavorite(data))
			{
				if(IsFavorite(data))
				{
                    SharedIcon(":/QtIcons/star.png").paint(painter, opt.rect.x(), opt.rect.y(), 16, opt.rect.height());
                }
                else
				{
                    SharedIcon(":/QtIcons/star_empty.png").paint(painter, opt.rect.x(), opt.rect.y(), 16, opt.rect.height());
                }
            }
		}
	}

	QtPropertyEditor::drawRow(painter, opt, index);
}

void PropertyEditor::ActionEditComponent()
{
	if(curNodes.size() == 1)
	{
        Entity *node = curNodes.at(0);
		ActionComponentEditor editor(this);

		editor.SetComponent((DAVA::ActionComponent*)node->GetComponent(DAVA::Component::ACTION_COMPONENT));
		editor.exec();

        SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();
        curScene->selectionSystem->SetSelection(node);
        if (editor.IsModified())
        {
            curScene->SetChanged(true);
        }
	}	
}

void PropertyEditor::ConvertToShadow()
{
	QtPropertyToolButton *btn = dynamic_cast<QtPropertyToolButton *>(QObject::sender());

	if(NULL != btn)
	{
		QtPropertyDataIntrospection *data = dynamic_cast<QtPropertyDataIntrospection *>(btn->GetPropertyData());
        SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();

        if(NULL != data && NULL != curScene)
        {
            DAVA::Vector<Command2 *> commands;
            commands.push_back(new ConvertToShadowCommand(reinterpret_cast<DAVA::RenderBatch *>(data->object)));
            data->ForeachMergedItem([&commands](const TPropertyPtr & item)
            {
                QtPropertyDataIntrospection * dynamicData = dynamic_cast<QtPropertyDataIntrospection *>(item.get());
                if (dynamicData != nullptr)
                {
                    commands.push_back(new ConvertToShadowCommand(reinterpret_cast<DAVA::RenderBatch *>(dynamicData->object)));
                }
                return true;
            });

            PropertyEditorDetails::ExecuteCommands(std::move(commands), "ConvertToShadow batch", curScene);
		}
	}
}

void PropertyEditor::RebuildTangentSpace()
{
    QtPropertyToolButton *btn = dynamic_cast<QtPropertyToolButton *>(QObject::sender());

    if(NULL != btn)
    {
        QtPropertyDataIntrospection *data = dynamic_cast<QtPropertyDataIntrospection *>(btn->GetPropertyData());
        SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();
        if(NULL != data && NULL != curScene)
        {            
            RenderBatch *batch = (RenderBatch *)data->object;
            curScene->Exec(new RebuildTangentSpaceCommand(batch, true));
        }
    }
}

void PropertyEditor::DeleteRenderBatch()
{
    // Code for removing several render batches
	QtPropertyToolButton *btn = dynamic_cast<QtPropertyToolButton *>(QObject::sender());

	if(NULL != btn)
	{
		QtPropertyDataIntrospection *data = dynamic_cast<QtPropertyDataIntrospection *>(btn->GetPropertyData());
        SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();

		if(NULL != data && NULL != curScene)
		{
            DAVA::Vector<Command2 *> commands;
            commands.push_back(new ConvertToShadowCommand(reinterpret_cast<DAVA::RenderBatch *>(data->object)));
            data->ForeachMergedItem([&commands, this](const TPropertyPtr & item)
            {
                QtPropertyDataIntrospection * dynamicData = dynamic_cast<QtPropertyDataIntrospection *>(item.get());
                if (dynamicData != nullptr)
                {
                    Entity *node = curNodes.at(0);

                    if (node)
                    {
                        DAVA::RenderBatch *batch = reinterpret_cast<DAVA::RenderBatch *>(dynamicData->object);
                        DAVA::RenderObject *ro = batch->GetRenderObject();
                        DVASSERT(ro);

                        DAVA::uint32 count = ro->GetRenderBatchCount();
                        for (DAVA::uint32 i = 0; i < count; ++i)
                        {
                            DAVA::RenderBatch* b = ro->GetRenderBatch(i);
                            if (b == batch)
                            {
                                commands.push_back(new DeleteRenderBatchCommand(node, batch->GetRenderObject(), i));
                                break;
                            }
                        }
                    }
                }

                return true;
            });

            PropertyEditorDetails::ExecuteCommands(std::move(commands), "DeleteRenderBatch", curScene);
        }
    }
}

void PropertyEditor::ActionEditMaterial()
{
	QtPropertyToolButton *btn = dynamic_cast<QtPropertyToolButton *>(QObject::sender());

	if(NULL != btn)
	{
		QtPropertyDataIntrospection *data = dynamic_cast<QtPropertyDataIntrospection *>(btn->GetPropertyData());
		if(NULL != data)
		{
			QtMainWindow::Instance()->OnMaterialEditor();
			MaterialEditor::Instance()->SelectMaterial((DAVA::NMaterial *) data->object);
		}
	}
}

void PropertyEditor::ActionEditSoundComponent()
{
    if(curNodes.size() == 1)
    {
        SceneEditor2* scene = QtMainWindow::Instance()->GetCurrentScene();
        if(!scene) return;

        Entity *node = curNodes.at(0);

        scene->BeginBatch("Edit Sound Component");

        SoundComponentEditor editor(scene, QtMainWindow::Instance());
        editor.SetEditableEntity(node);
        editor.exec();

        scene->EndBatch();

        ResetProperties();
    }	
}

bool PropertyEditor::IsParentFavorite(const QtPropertyData * data) const
{
	bool ret = false;

	QtPropertyData *parent = data->Parent();
	while(NULL != parent)
	{
		if(IsFavorite(parent))
		{
			ret = true;
			break;
		}
		else
		{
			parent = parent->Parent();
		}
	}

	return ret;
}

bool PropertyEditor::IsFavorite(QtPropertyData *data) const
{
	bool ret = false;

	if(NULL != data)
	{
		PropEditorUserData *userData = GetUserData(data);
		ret = userData->isFavorite;
	}

	return ret;
}

void PropertyEditor::SetFavorite(QtPropertyData *data, bool favorite)
{
	if(NULL == favoriteGroup)
	{
		favoriteGroup = GetProperty(InsertHeader("Favorites", 0));
	}

	if(NULL != data)
	{
		PropEditorUserData *userData = GetUserData(data);

		switch(userData->type)
		{
			case PropEditorUserData::ORIGINAL:
				if(userData->isFavorite != favorite)
				{
					// it is in favorite now, so we are going to remove it from favorites
					if(!favorite)
					{
						DVASSERT(NULL != userData->associatedData);

						QtPropertyData *favorite = userData->associatedData;
						favoriteGroup->ChildRemove(favorite);
						userData->associatedData = NULL;
						userData->isFavorite = false;

						scheme.remove(data->GetPath());
						AddFavoriteChilds(data);
					}
					// new item should be added to favorites list
					else
					{
						DVASSERT(NULL == userData->associatedData);

						// check if it hasn't parent, that is already favorite
						bool canBeAdded = true;
						QtPropertyData *parent = data;
						while(NULL != parent)
						{
							if(GetUserData(parent)->isFavorite)
							{
								canBeAdded = false;
								break;
							}

							parent = parent->Parent();
						}

						if(canBeAdded)
						{
                            TPropertyPtr favorite(CreateClone(data));
                            data->ForeachMergedItem([&favorite, this](const TPropertyPtr & item)
                            {
                                TPropertyPtr mergedClone(CreateClone(item.get()));
                                favorite->Merge(std::move(mergedClone));
                                return true;
                            });

							userData->associatedData = favorite.get();
							userData->isFavorite = true;

							ApplyCustomExtensions(favorite.get());
                            favoriteGroup->MergeChild(std::move(favorite));


							// create user data for added favorite, that will have COPY type,
							// and associatedData will point to the original property
							PropEditorUserData *favUserData = new PropEditorUserData(PropEditorUserData::COPY, data, true);
							favorite->SetUserData(favUserData);

							favUserData->realPath = data->GetPath();
							scheme.insert(data->GetPath());
							RemFavoriteChilds(data);
						}
					}

					data->EmitDataChanged(QtPropertyData::VALUE_SET);
				}
				break;

			case PropEditorUserData::COPY:
				if(userData->isFavorite != favorite)
				{
					// copy of the original data can only be removed
					DVASSERT(!favorite);
					
					// copy of the original data should always have a pointer to the original property data
					QtPropertyData *original = userData->associatedData;
					if(NULL != original)
					{
						PropEditorUserData *originalUserData = GetUserData(original);
						originalUserData->associatedData = NULL;
						originalUserData->isFavorite = false;

						AddFavoriteChilds(original);
					}

					scheme.remove(userData->realPath);
					favoriteGroup->ChildRemove(data);
				}
				break;

			default:
				DVASSERT(false && "Unknown userData type");
				break;
		}
	}

	// if there is no favorite items - remove favorite group
	if(favoriteGroup->ChildCount() == 0)
	{
		RemoveProperty(favoriteGroup);
		favoriteGroup = NULL;
	}
}

void PropertyEditor::AddFavoriteChilds(QtPropertyData *data)
{
	if(NULL != data)
	{
		// go through childs
		for(int i = 0; i < data->ChildCount(); ++i)
		{
            const TPropertyPtr & child = data->ChildGet(i);
			if(scheme.contains(child->GetPath()))
			{
				SetFavorite(child.get(), true);
			}
			else
			{
				AddFavoriteChilds(child.get());
			}
		}
	}
}

void PropertyEditor::RemFavoriteChilds(QtPropertyData *data)
{
	if(NULL != data)
	{
		if(GetUserData(data)->type == PropEditorUserData::ORIGINAL)
		{
			// go through childs
			for(int i = 0; i < data->ChildCount(); ++i)
			{
                const TPropertyPtr & child = data->ChildGet(i);
				PropEditorUserData *userData = GetUserData(child.get());
				if(NULL != userData->associatedData)
				{
					favoriteGroup->ChildRemove(userData->associatedData);

					userData->associatedData = NULL;
					userData->isFavorite = false;
				}
				else
				{
					RemFavoriteChilds(child.get());
				}
			}
		}
	}
}

PropEditorUserData* PropertyEditor::GetUserData(QtPropertyData * data) const
{
    PropEditorUserData *userData = static_cast<PropEditorUserData*>(data->GetUserData());
	if(NULL == userData)
	{
		userData = new PropEditorUserData(PropEditorUserData::ORIGINAL);
		data->SetUserData(userData);
	}

	return userData;
}

void PropertyEditor::LoadScheme(const DAVA::FilePath &path)
{
	// first, we open the file
	QFile file(path.GetAbsolutePathname().c_str());
	if(file.open(QIODevice::ReadOnly))
	{
		scheme.clear();

		QTextStream qin(&file);
		while(!qin.atEnd())
		{
			scheme.insert(qin.readLine());
		}

 		file.close();
	}
}

void PropertyEditor::SaveScheme(const DAVA::FilePath &path)
{
	// first, we open the file
	QFile file(path.GetAbsolutePathname().c_str());
	if(file.open(QIODevice::WriteOnly))
	{
		QTextStream qout(&file);
		foreach(const QString &value, scheme)
		{
			qout << value << endl;
		}
		file.close();
	}
}

bool PropertyEditor::IsInspViewAllowed(const DAVA::InspInfo *info) const
{
	bool ret = true;

	if(info->Type() == DAVA::MetaInfo::Instance<DAVA::NMaterial>())
	{
		// Don't show properties for NMaterial.
        // They should be edited in materialEditor.
		ret = false;
	}

	return ret;
}

QtPropertyToolButton * PropertyEditor::CreateButton( QtPropertyData * data, const QIcon & icon, const QString & tooltip )
{
	DVASSERT(data);

	QtPropertyToolButton *button = data->AddButton();
	button->setIcon(icon);
	button->setToolTip(tooltip);
	button->setIconSize(QSize(12, 12));
	button->setAutoRaise(true);

	return button;
}

void PropertyEditor::CloneRenderBatchesToFixSwitchLODs()
{
    QtPropertyToolButton *btn = dynamic_cast<QtPropertyToolButton *>(QObject::sender());

    if(NULL != btn)
    {
        QtPropertyDataIntrospection *data = dynamic_cast<QtPropertyDataIntrospection *>(btn->GetPropertyData());
        if(NULL != data)
        {
            DAVA::RenderObject *renderObject = (DAVA::RenderObject *)data->object;

            SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();
            if(curScene && renderObject)
            {
                curScene->Exec(new CloneLastBatchCommand(renderObject));
            }
        }
    }
}

void PropertyEditor::OnAddComponent(Component::eType type)
{
    SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();
    int size = curNodes.size();
    if(size > 0)
    {
        curScene->BeginBatch(Format("Add Component: %d", type));

        for(int i = 0; i < size; ++i)
        {
            Component *c = Component::CreateByType(type);
            curScene->Exec(new AddComponentCommand(curNodes.at(i), c));
        }

        curScene->EndBatch();
    }
}

void PropertyEditor::OnAddComponent(DAVA::Component *component)
{
    DVASSERT(component);
    if(!component) return;
    
    SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();
    int size = curNodes.size();
    if(size > 0)
    {
        curScene->BeginBatch(Format("Add Component: %d", component->GetType()));
        
        for(int i = 0; i < size; ++i)
        {
            Entity* node = curNodes.at(i);
            
            if (node->GetComponentCount(component->GetType()) == 0)
            {
                Component *c = component->Clone(node);
                curScene->Exec(new AddComponentCommand(curNodes.at(i), c));
            }
        }
        
        curScene->EndBatch();
    }
}

void PropertyEditor::OnAddActionComponent()
{
    OnAddComponent(Component::ACTION_COMPONENT);
}

void PropertyEditor::OnAddStaticOcclusionComponent()
{
    OnAddComponent(Component::STATIC_OCCLUSION_COMPONENT);
}

void PropertyEditor::OnAddSoundComponent()
{
    OnAddComponent(Component::SOUND_COMPONENT);
}

void PropertyEditor::OnAddWaveComponent()
{
    OnAddComponent(Component::WAVE_COMPONENT);
}

void PropertyEditor::OnAddModelTypeComponent()
{
    OnAddComponent(Component::QUALITY_SETTINGS_COMPONENT);
}

void PropertyEditor::OnAddSkeletonComponent()
{
    OnAddComponent(Component::SKELETON_COMPONENT);
}

void PropertyEditor::OnAddPathComponent()
{
    SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();
    if (curNodes.size() > 0)
    {
        curScene->BeginBatch(Format("Add Component: %d", Component::PATH_COMPONENT));

        for (Entity* node : curNodes)
        {
            DVASSERT(node);
            if (node->GetComponentCount(Component::PATH_COMPONENT) == 0 
             && node->GetComponentCount(Component::WAYPOINT_COMPONENT) == 0)
            {
                PathComponent* pathComponent = curScene->pathSystem->CreatePathComponent();
                curScene->Exec(new AddComponentCommand(node, pathComponent));
            }
        }

        curScene->EndBatch();
    }
}

void PropertyEditor::OnAddRotationControllerComponent()
{
    OnAddComponent(Component::ROTATION_CONTROLLER_COMPONENT);
}

void PropertyEditor::OnAddSnapToLandscapeControllerComponent()
{
    SnapToLandscapeControllerComponent *snapComponent = static_cast<SnapToLandscapeControllerComponent *> (Component::CreateByType(Component::SNAP_TO_LANDSCAPE_CONTROLLER_COMPONENT));

    float32 height = SettingsManager::Instance()->GetValue(Settings::Scene_CameraHeightOnLandscape).AsFloat();
    snapComponent->SetHeightOnLandscape(height);

    OnAddComponent(snapComponent);
    
    SafeDelete(snapComponent);
}

void PropertyEditor::OnAddWASDControllerComponent()
{
    OnAddComponent(Component::WASD_CONTROLLER_COMPONENT);
}

void PropertyEditor::OnRemoveComponent()
{
	QtPropertyToolButton *btn = dynamic_cast<QtPropertyToolButton *>(QObject::sender());

	if(NULL != btn)
	{
		QtPropertyDataIntrospection *data = dynamic_cast<QtPropertyDataIntrospection *>(btn->GetPropertyData());
        SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();

		if(NULL != data && NULL != curScene)
		{
            DAVA::Vector<Command2 *> commands;
            commands.push_back(new ConvertToShadowCommand(reinterpret_cast<DAVA::RenderBatch *>(data->object)));
            data->ForeachMergedItem([&commands, this](const TPropertyPtr & item)
            {
                QtPropertyDataIntrospection * dynamicData = dynamic_cast<QtPropertyDataIntrospection *>(item.get());
                if (dynamicData != nullptr)
                {
                    Component *component = (Component *)dynamicData->object;
                    PropEditorUserData* userData = GetUserData(dynamicData);
                    DVASSERT(userData);
                    Entity *node = userData->entity;

                    commands.push_back(new RemoveComponentCommand(node, component));
                }

                return true;
            });

            PropertyEditorDetails::ExecuteCommands(std::move(commands), "Remove Component", curScene);
		}
	}
}

void PropertyEditor::OnTriggerWaveComponent()
{
    for(int i = 0; i < curNodes.size(); ++i)
    {
        WaveComponent * component = GetWaveComponent(curNodes.at(i));
        if (component)
        {
            component->Trigger();
        }
    }
}

QString PropertyEditor::GetDefaultFilePath()
{
    QString defaultPath = ProjectManager::Instance()->GetProjectPath().GetAbsolutePathname().c_str();
    FilePath dataSourcePath = ProjectManager::Instance()->GetDataSourcePath();
    if (FileSystem::Instance()->Exists(dataSourcePath))
    {
        defaultPath = dataSourcePath.GetAbsolutePathname().c_str();
    }
    SceneEditor2* editor = QtMainWindow::Instance()->GetCurrentScene();
    if (NULL != editor && FileSystem::Instance()->Exists(editor->GetScenePath()))
    {
        DAVA::String scenePath = editor->GetScenePath().GetDirectory().GetAbsolutePathname();
        if (String::npos != scenePath.find(dataSourcePath.GetAbsolutePathname()))
        {
            defaultPath = scenePath.c_str();
        }
    }

	return defaultPath;
}
