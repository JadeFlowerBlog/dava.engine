#pragma once

#include <memory>

#include <QtWidgets/QWidget>

#include "PortType.hpp"
#include "NodeData.hpp"
#include "Serializable.hpp"
#include "NodeGeometry.hpp"
#include "NodeStyle.hpp"
#include "NodePainterDelegate.hpp"
#include "Export.hpp"


class QPointF;
namespace QtNodes
{

enum class NodeValidationState
{
  Valid,
  Warning,
  Error
};

class StyleCollection;

class NODE_EDITOR_PUBLIC NodeDataModel
  : public QObject
  , public Serializable
{
  Q_OBJECT

public:

  NodeDataModel();

  virtual
  ~NodeDataModel() = default;

  /// Caption is used in GUI
  virtual QString
  caption() const = 0;

    virtual QColor
    captionColor() const = 0;

  /// It is possible to hide caption in GUI
  virtual bool
  captionVisible() const { return true; }

  /// Port caption is used in GUI to label individual ports
  virtual QString
  portCaption(PortType, PortIndex) const { return QString(); }
    
  /// It is possible to hide port caption in GUI
  virtual bool
  portCaptionVisible(PortType, PortIndex) const { return false; }

  virtual QList<QString>
  portCaptions(PortType, PortIndex) const { return QList<QString>(); }

  virtual QString
  portHint(PortType, PortIndex) const { return QString(); }
    
  /// Name makes this model unique
  virtual QString
  name() const = 0;

  /// Function creates instances of a model stored in DataModelRegistry
  virtual std::unique_ptr<NodeDataModel>
  clone() const = 0;

    virtual void
    SetCategory(const QString& category) = 0;

public:

  QJsonObject
  save() const override;

public:

  virtual
  unsigned int nPorts(PortType portType) const = 0;

    virtual 
    PortKind 
    GetPortKind(PortType portType, PortIndex portIndex) const = 0;

  virtual
  NodeDataType dataType(PortType portType, PortIndex portIndex) const = 0;

public:

  enum class ConnectionPolicy
  {
    One,
    Many,
  };

//  virtual
//  ConnectionPolicy
//  portOutConnectionPolicy(PortIndex) const
//  {
//    return ConnectionPolicy::Many;
//  }

  virtual
  ConnectionPolicy
  connectionPolicy(PortType portType, PortIndex portIndex) const;

    
  NodeStyle const&
  nodeStyle() const;

  void
  setNodeStyle(NodeStyle const& style);

public:

  /// Triggers the algorithm
  virtual
  void
  setInData(std::shared_ptr<NodeData> nodeData,
            PortIndex port) = 0;

    virtual int canSetInData(PortIndex inPort, std::shared_ptr<NodeData> nodeData) const = 0;
    
  virtual
  std::shared_ptr<NodeData>
  outData(PortIndex port) = 0;

    virtual
    void
    DisconnectInData(PortIndex portIndexIn, std::shared_ptr<NodeData> outNodeData) = 0;

    
  virtual
  QWidget *
  embeddedWidget() = 0;

  virtual
  bool
  resizable() const { return false; }

  virtual
  NodeValidationState
  validationState() const { return NodeValidationState::Valid; }

  virtual
  QString
  validationMessage() const { return QString(""); }

  virtual
  NodePainterDelegate* painterDelegate() const { return nullptr; }
    
    virtual void SetEntryHeight(unsigned int height) = 0;
    virtual void SetSpacing(unsigned int spacing) = 0;

signals:

  void
  dataUpdated(PortIndex index);

  void
  dataInvalidated(PortIndex index);

  void
  computingStarted();

  void
  computingFinished();

private:

  NodeStyle _nodeStyle;
};
}
