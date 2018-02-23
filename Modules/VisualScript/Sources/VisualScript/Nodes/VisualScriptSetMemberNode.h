#pragma once

#include "VisualScript/VisualScriptNode.h"
#include <Base/FastName.h>

namespace DAVA
{
class ValueWrapper;
class YamlNode;

class VisualScriptSetMemberNode : public VisualScriptNode
{
    DAVA_VIRTUAL_REFLECTION(VisualScriptSetMemberNode, VisualScriptNode);

public:
    VisualScriptSetMemberNode();
    VisualScriptSetMemberNode(const FastName& className, const FastName& fieldName);
    ~VisualScriptSetMemberNode() override = default;

    void SetClassName(const FastName& className);
    const FastName& GetClassName() const;

    void SetFieldName(const FastName& fieldName);
    const FastName& GetFieldName() const;

    void Save(YamlNode* node) const override;
    void Load(const YamlNode* node) override;

    const ValueWrapper* GetValueWrapper() const;

public:
    void InitPins();
    void InitNodeWithValueWrapper(const ValueWrapper* wrapper);

    FastName className;
    FastName fieldName;

    const ValueWrapper* valueWrapper = nullptr;
};
}
