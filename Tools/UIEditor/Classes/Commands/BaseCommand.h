//
//  BaseCommand.h
//  UIEditor
//
//  Created by Yuri Coder on 10/22/12.
//
//

#ifndef __UIEditor__BaseCommand__
#define __UIEditor__BaseCommand__

#include <DAVAEngine.h>
#include "BaseMetadata.h"
#include "HierarchyTreeController.h"

namespace DAVA {

// Base Command for all the Commands existing in the UI Editor system.
class BaseCommand: public BaseObject
{
public:
    BaseCommand();
    virtual ~BaseCommand();

    // Execute command.
    virtual void Execute() {};
    
    // Rollback command.
    virtual void Rollback() {};
    
protected:
    // Get the Metadata for the tree node passed.
    BaseMetadata* GetMetadataForTreeNode(HierarchyTreeNode::HIERARCHYTREENODEID treeNodeID);
};
    
}

#endif /* defined(__UIEditor__BaseCommand__) */
