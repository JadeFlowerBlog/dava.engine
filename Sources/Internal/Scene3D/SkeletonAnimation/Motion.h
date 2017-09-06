#pragma once

#include "Base/BaseMath.h"
#include "Base/BaseTypes.h"
#include "Base/FastNameMap.h"
#include "Reflection/Reflection.h"
#include "Scene3D/SkeletonAnimation/SkeletonPose.h"

namespace DAVA
{
class BlendTree;
class SkeletonComponent;
class YamlNode;
class Motion
{
    Motion() = default;

public:
    enum eMotionBlend
    {
        BLEND_OVERRIDE,
        BLEND_ADD,
        BLEND_DIFF,
        BLEND_LERP,

        BLEND_COUNT
    };
    enum eTransitionType : uint8
    {
        TRANSITION_TYPE_CROSS_FADE,
        TRANSITION_TYPE_FROZEN_FADE,
        TRANSITION_TYPE_BLENDTREE,

        TRANSITION_TYPE_COUNT
    };
    enum eTransitionFunc : uint8
    {
        TRANSITION_FUNC_LINEAR,
        TRANSITION_FUNC_CURVE,

        TRANSITION_FUNC_COUNT
    };
    enum eTransitionSync : uint8
    {
        TRANSITION_SYNC_IMMIDIATE,
        TRANSITION_SYNC_WAIT_END,
        TRANSITION_SYNC_WAIT_PHASE_END,

        TRANSITION_SYNC_COUNT
    };
    enum eTransitionInterrupt : uint8
    {
        TRANSITION_INTERRUPT_PHASE_INVERSE,

        TRANSITION_INTERRUPT_COUNT
    };

    ~Motion();

    static Motion* LoadFromYaml(const YamlNode* motionNode);

    const FastName& GetName() const;
    eMotionBlend GetBlendMode() const;
    const SkeletonPose& GetCurrentSkeletonPose() const;

    void BindSkeleton(const SkeletonComponent* skeleton);
    void Update(float32 dTime, Vector<std::pair<FastName, FastName>>* outEndedPhases = nullptr /*[state-id, phase-id]*/);

    const Vector<FastName>& GetParameterIDs() const;
    bool BindParameter(const FastName& parameterID, const float32* param);
    bool UnbindParameter(const FastName& parameterID);
    void UnbindParameters();

    const Vector<FastName>& GetStateIDs() const;
    bool RequestState(const FastName& stateID);

protected:
    struct State;
    class Transition
    {
    public:
        void Update(float32 dTime, SkeletonPose* outPose);
        void Reset(State* srcState, State* dstState);

        bool IsComplete() const;

    protected:
        eTransitionType type = TRANSITION_TYPE_COUNT;
        eTransitionFunc func = TRANSITION_FUNC_COUNT;
        eTransitionSync sync = TRANSITION_SYNC_COUNT;
        eTransitionInterrupt interruption = TRANSITION_INTERRUPT_PHASE_INVERSE;
        float32 duration = 0.f;
        FastName waitPhaseID;
        bool syncPhases = false;

        //runtime
        State* srcState = nullptr;
        State* dstState = nullptr;
        float32 transitionPhase = 0.f;
        bool started = false;

        friend class Motion;
    };
    struct State
    {
        bool Update(float32 dTime); //return 'true' if current phase of state is ended
        void EvaluatePose(SkeletonPose* outPose) const;

        FastName id;
        BlendTree* blendTree = nullptr;
        FastNameMap<Transition*> transitions;

        uint32 currPhaseIndex = 0u;
        uint32 prevPhaseIndex = 0u;
        float32 phase = 0.f;
        Vector<const float32*> boundParams;

        //TODO: *Skinning* move it to BlendTree ?
        Vector<FastName> phaseNames;
    };

    FastName name;
    eMotionBlend blendMode = BLEND_COUNT;

    Vector<State> states;
    FastNameMap<State*> statesMap;
    State* currentState = nullptr;

    Vector<Transition> transitions;
    Transition* currentTransition = nullptr;

    Vector<FastName> statesIDs;
    Vector<FastName> parameterIDs;

    SkeletonPose currentPose;

    //////////////////////////////////////////////////////////////////////////
    //temporary for debug
    const FastName& GetStateID() const
    {
        static const FastName invalidID = FastName("#invalid-state");
        return (currentState != nullptr) ? currentState->id : invalidID;
    }
    void SetStateID(const FastName& id)
    {
        RequestState(id);
    }
    //////////////////////////////////////////////////////////////////////////

    DAVA_REFLECTION(Motion);
};

inline const FastName& Motion::GetName() const
{
    return name;
}

inline Motion::eMotionBlend Motion::GetBlendMode() const
{
    return blendMode;
}

inline const SkeletonPose& Motion::GetCurrentSkeletonPose() const
{
    return currentPose;
}

inline const Vector<FastName>& Motion::GetParameterIDs() const
{
    return parameterIDs;
}

} //ns