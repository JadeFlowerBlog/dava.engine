#ifndef __COMMAND_CREATE_NODE_H__
#define __COMMAND_CREATE_NODE_H__

#include "Command.h"
#include "../Constants.h"

class CommandCreateNode: public Command
{
public:	
	DAVA_DEPRECATED(CommandCreateNode(ResourceEditor::eNodeType type));// DEPRECATED: use SceneEditorScreenMain
    
protected:
    virtual void Execute();

protected:
    ResourceEditor::eNodeType nodeType;
};




#endif // #ifndef __COMMAND_CREATE_NODE_H__