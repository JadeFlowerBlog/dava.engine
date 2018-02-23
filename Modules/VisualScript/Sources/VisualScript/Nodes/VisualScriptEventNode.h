#pragma once

#include "VisualScript/VisualScriptNode.h"

#include <Base/FastName.h>

namespace DAVA
{
class VisualScriptEventNode : public VisualScriptNode
{
    DAVA_VIRTUAL_REFLECTION(VisualScriptEventNode, VisualScriptNode);

public:
    VisualScriptEventNode();
    ~VisualScriptEventNode() override = default;

    void SetEventName(const FastName& eventName_);
    const FastName& GetEventName() const;

    void BindReflection(const Reflection& ref_) override;

    void Save(YamlNode* node) const override;
    void Load(const YamlNode* node) override;

protected:
    FastName eventName;
};
}
