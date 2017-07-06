#include "Scene3D/Components/SkeletonComponent.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
REGISTER_CLASS(SkeletonComponent)

DAVA_VIRTUAL_REFLECTION_IMPL(SkeletonComponent::JointConfig)
{
    ReflectionRegistrator<SkeletonComponent::JointConfig>::Begin()
    .Field("name", &SkeletonComponent::JointConfig::name)[M::DisplayName("Name")]
    .Field("uid", &SkeletonComponent::JointConfig::uid)[M::DisplayName("UID")]
    .Field("bbox", &SkeletonComponent::JointConfig::bbox)[M::DisplayName("Bounding box")]
    .Field("bindTransformInv", &SkeletonComponent::JointConfig::bbox)[M::DisplayName("Bind Transform Inverse")]
    .End();
}

DAVA_VIRTUAL_REFLECTION_IMPL(SkeletonComponent)
{
    ReflectionRegistrator<SkeletonComponent>::Begin()
    .ConstructorByPointer()
    .Field("configJoints", &SkeletonComponent::configJoints)[M::DisplayName("Root Joints")]
    .End();
}

template <>
bool AnyCompare<SkeletonComponent::JointConfig>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2)
{
    return v1.Get<SkeletonComponent::JointConfig>() == v2.Get<SkeletonComponent::JointConfig>();
}

SkeletonComponent::JointConfig::JointConfig(int32 _parentIndex, int32 _targetId, const FastName& _name, const FastName& _uid, const Vector3& _position, const Quaternion& _orientation, float32 _scale, const AABBox3& _bbox, const Matrix4& _invBindPose)
    : parentIndex(_parentIndex)
    , targetId(_targetId)
    , name(_name)
    , uid(_uid)
    , orientation(_orientation)
    , position(_position)
    , scale(_scale)
    , bbox(_bbox)
    , bindTransformInv(_invBindPose)
{
}

bool SkeletonComponent::JointConfig::operator==(const JointConfig& other) const
{
    return parentIndex == other.parentIndex &&
    targetId == other.targetId &&
    name == other.name &&
    uid == other.uid &&
    bindTransformInv == other.bindTransformInv;
    orientation == other.orientation &&
    position == other.position &&
    scale == other.scale &&
    bbox == other.bbox;
}

uint32 SkeletonComponent::GetConfigJointsCount()
{
    return uint16(configJoints.size());
}

const SkeletonComponent::JointConfig& SkeletonComponent::GetConfigJoint(uint32 i)
{
    return configJoints[i];
}

void SkeletonComponent::SetConfigJoints(const Vector<JointConfig>& config)
{
    configJoints = config;
    configUpdated = true;
}

void SkeletonComponent::RebuildFromConfig()
{
    GlobalEventSystem::Instance()->Event(this, EventSystem::SKELETON_CONFIG_CHANGED);
}

const DAVA::FastName& SkeletonComponent::GetJointName(uint16 jointId) const
{
    DVASSERT(jointId < configJoints.size());
    return configJoints[jointId].name;
}

const SkeletonComponent::JointTransform& SkeletonComponent::GetObjectSpaceTransform(uint16 jointId) const
{
    DVASSERT(jointId < objectSpaceTransforms.size());
    return objectSpaceTransforms[jointId];
}

Component* SkeletonComponent::Clone(Entity* toEntity)
{
    SkeletonComponent* newComponent = new SkeletonComponent();
    newComponent->SetEntity(toEntity);
    newComponent->configJoints = configJoints;
    return newComponent;
}
void SkeletonComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);
    archive->SetUInt32("skeletoncomponent.jointsCount", static_cast<uint32>(configJoints.size()));
    ScopedPtr<KeyedArchive> jointsArch(new KeyedArchive());
    for (size_t i = 0, sz = configJoints.size(); i < sz; ++i)
    {
        const JointConfig& joint = configJoints[i];
        ScopedPtr<KeyedArchive> jointArch(new KeyedArchive());
        jointArch->SetFastName("joint.name", joint.name);
        jointArch->SetFastName("joint.uid", joint.uid);
        jointArch->SetInt32("joint.parentIndex", joint.parentIndex);
        jointArch->SetInt32("joint.targetId", joint.targetId);
        jointArch->SetVector3("joint.position", joint.position);
        jointArch->SetVector4("joint.orientation", Vector4(joint.orientation.data));
        jointArch->SetFloat("joint.scale", joint.scale);
        jointArch->SetVector3("joint.bbox.min", joint.bbox.min);
        jointArch->SetVector3("joint.bbox.max", joint.bbox.max);
        jointArch->SetMatrix4("joint.invBindPose", joint.bindTransformInv);

        jointsArch->SetArchive(KeyedArchive::GenKeyFromIndex(static_cast<int32>(i)), jointArch);
    }

    archive->SetArchive("skeletoncomponent.joints", jointsArch);
}
void SkeletonComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Deserialize(archive, serializationContext);

    uint32 configJointsCount = archive->GetUInt32("skeletoncomponent.jointsCount", static_cast<uint32>(configJoints.size()));
    configJoints.resize(configJointsCount);
    KeyedArchive* jointsArch = archive->GetArchive("skeletoncomponent.joints");
    for (uint32 i = 0; i < configJointsCount; ++i)
    {
        JointConfig& joint = configJoints[i];
        KeyedArchive* jointArch = jointsArch->GetArchive(KeyedArchive::GenKeyFromIndex(i));
        joint.name = jointArch->GetFastName("joint.name");
        joint.uid = jointArch->GetFastName("joint.uid");
        joint.parentIndex = jointArch->GetInt32("joint.parentIndex");
        joint.targetId = jointArch->GetInt32("joint.targetId");
        joint.position = jointArch->GetVector3("joint.position");
        joint.orientation = Quaternion(jointArch->GetVector4("joint.orientation").data);
        joint.scale = jointArch->GetFloat("joint.scale");
        joint.bbox.min = jointArch->GetVector3("joint.bbox.min");
        joint.bbox.max = jointArch->GetVector3("joint.bbox.max");
        joint.bindTransformInv = jointArch->GetMatrix4("joint.invBindPose", joint.bindTransformInv);
    }
}

template <>
bool AnyCompare<SkeletonComponent::JointConfig>::IsEqual(const Any& v1, const Any& v2)
{
    return v1.Get<SkeletonComponent::JointConfig>() == v2.Get<SkeletonComponent::JointConfig>();
}
}