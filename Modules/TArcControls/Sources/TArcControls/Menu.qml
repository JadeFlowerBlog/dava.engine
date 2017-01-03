import QtQuick 2.6
import QtQuick.Controls 1.4

ContextMenu
{
    id: menu
    objectName: "Menu"
    
    path: "Menu"
    property alias title: menu.path

    property list<QtObject> items
    default property alias itemsProperty : menu.items

    property var actions : []
    property var fullPath: path

    onAboutToShow:
    {
        activate();
    }

    onAboutToHide:
    {
        deactivate();
    }

    Component.onCompleted:
    {
        for (var i=0; i < items.length; i++)
        {
            createAction(items[i], menu);
        }

        if (fullPath == path)
        {
            fullPath = fullPath + "|";
        }
    }

    Component.onDestruction:
    {
        for (var i=0; i < items.length; i++)
        {
            destroyAction(items[i], menu);
        }
    }

    function activate()
    {
        for (var actionKey in actions)
        {
            var action = actions[actionKey];
            if (action == null)
            {
                continue;
            }
            action.active = true;
        }

        for (var i=0; i < items.length; i++)
        {
            if (isMenu(items[i]))
            {
                items[i].activate();
            }
        }
    }

    function deactivate()
    {
        for (var actionKey in actions)
        {
            var action = actions[actionKey];
            if (action == null)
            {
                continue;
            }
            action.active = false;
        }

        for (var i=0; i < items.length; i++)
        {
            if (isMenu(items[i]))
            {
                items[i].deactivate();
            }
        }
    }

    function insertItem(index, object)
    {
        createAction(object, menu);
    }

    function removeItem(object)
    {
        destroyAction(object, menu);
    }

    function isMenuItem(object)
    {
        if(object == null)
        {
            return false;
        }
        var text = object.text;
        var trigger = object.trigger;
        return (typeof text !== "undefined" && typeof trigger !== "undefined");
    }

    function isMenu(object)
    {
        if(object == null)
        {
            return false;
        }
        var path = object.path;
        var items = object.items;
        var actions = object.actions;
        return (typeof path !== "undefined" && typeof items !== "undefined" && typeof items !== "undefined");
    }

    function createAction(object, menu)
    {
        //Automatically convert all MenuItem children into WGActions that we add to
        //the application just before we show the menu, and remove after we hide it
        if (isMenuItem(object))
        {
            var qmlString = "import QtQuick 2.4; import QtQuick.Controls 1.4; import TArcControls 1.0; Action {}";
            var action = Qt.createQmlObject( qmlString, menu );
            action.actionId = Qt.binding( function() { return menu.fullPath + "." + object.text; } );
            action.actionText = Qt.binding( function() { return object.text; } );
            action.actionPath = Qt.binding( function() { return menu.fullPath; } );
            action.checkable = Qt.binding( function() { return object.checkable; } );
            action.checked = Qt.binding( function() { return object.checked; } );
            action.enabled = Qt.binding( function() { return object.enabled; } );
            action.visible = Qt.binding( function() { return object.visible; } );
            action.triggered.connect(object.trigger);
            actions[object.text] = action;
            return;
        }

        if (isMenu(object))
        {
            object.fullPath = Qt.binding( function() { return menu.fullPath + "." + object.path; } )
        }
    }

    function destroyAction(object, menu)
    {
         if (isMenuItem(object))
         {
            var action = actions[object.text];
            action.destroy();
            actions[object.text] = null;
            return;
        }
    }
}
