#ifndef __QUICKED_QT_MODEL_PACKAGE_COMMAND_EXECUTOR_H__
#define __QUICKED_QT_MODEL_PACKAGE_COMMAND_EXECUTOR_H__

#include "Model/PackageCommandExecutor.h"

#include <QString>

class Document;
class PackageBaseNode;
class QUndoStack;
class QUndoCommand;

class QtModelPackageCommandExecutor : public PackageCommandExecutor
{
public:
    QtModelPackageCommandExecutor(Document *_document);
    virtual ~QtModelPackageCommandExecutor();
    
    void AddImportedPackageIntoPackage(PackageControlsNode *importedPackageControls, PackageNode *package) override;
    
public:
    void ChangeProperty(ControlNode *node, BaseProperty *property, const DAVA::VariantType &value) override;
    void ResetProperty(ControlNode *node, BaseProperty *property) override;
private:
    void ChangeDefaultProperties(const DAVA::Vector<ControlNode *> &node, BaseProperty *property, const DAVA::VariantType &value);

public:
    void AddComponent(ControlNode *node, DAVA::uint32 componentType, DAVA::uint32 componentIndex) override;
    void RemoveComponent(ControlNode *node, DAVA::uint32 componentType, DAVA::uint32 componentIndex) override;

    void InsertControl(ControlNode *control, ControlsContainerNode *dest, DAVA::int32 destIndex) override;
    void CopyControls(const DAVA::Vector<ControlNode*> &nodes, ControlsContainerNode *dest, DAVA::int32 destIndex) override;
    void MoveControls(const DAVA::Vector<ControlNode*> &nodes, ControlsContainerNode *dest, DAVA::int32 destIndex) override;
    void RemoveControls(const DAVA::Vector<ControlNode*> &nodes) override;

    bool Paste(PackageNode *root, ControlsContainerNode *dest, DAVA::int32 destIndex, const DAVA::String &data);

private:
    void InsertControlImpl(ControlNode *control, ControlsContainerNode *dest, DAVA::int32 destIndex);
    void RemoveControlImpl(ControlNode *node);
    void AddComponentImpl(ControlNode *node, DAVA::uint32 componentType, DAVA::uint32 componentIndex);
    void RemoveComponentImpl(ControlNode *node, DAVA::uint32 componentType, DAVA::uint32 componentIndex);
    bool IsNodeInHierarchy(const PackageBaseNode *node) const;
    
private:
    QUndoStack *GetUndoStack();
    void PushCommand(QUndoCommand *cmd);
    void BeginMacro(const QString &name);
    void EndMacro();
    
private:
    Document *document;
};

#endif // __QUICKED_QT_MODEL_PACKAGE_COMMAND_EXECUTOR_H__
