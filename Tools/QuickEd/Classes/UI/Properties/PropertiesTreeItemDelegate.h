#ifndef __PROPERTIESTREEITEMDELEGATE_H__
#define __PROPERTIESTREEITEMDELEGATE_H__

#include <QWidget>
#include <QVector2D>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include "Model/ControlProperties/AbstractProperty.h"
#include "FileSystem/VariantType.h"
class AbstractPropertyDelegate;
class QToolButton;

class PropertiesTreeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    PropertiesTreeItemDelegate(QObject* parent = NULL);
    ~PropertiesTreeItemDelegate();

    virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

    virtual AbstractPropertyDelegate* GetCustomItemDelegateForIndex(const QModelIndex& index) const;

    void emitCommitData(QWidget* editor);
    void emitCloseEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint);

protected:
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    QMap<AbstractProperty::ePropertyType, AbstractPropertyDelegate*> propertyItemDelegates;
    QMap<DAVA::VariantType::eVariantType, AbstractPropertyDelegate*> variantTypeItemDelegates;
    QMap<QString, AbstractPropertyDelegate*> propertyNameTypeItemDelegates;
};
class PropertyWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PropertyWidget(QWidget* parent = NULL);
    ~PropertyWidget(){};

    bool event(QEvent* e) override;

    void keyPressEvent(QKeyEvent* e) override;

    void mousePressEvent(QMouseEvent* e) override;

    void mouseReleaseEvent(QMouseEvent* e) override;

    void focusInEvent(QFocusEvent* e) override;

    void focusOutEvent(QFocusEvent* e) override;

public:
    QWidget* editWidget;
};
#endif // __PROPERTIESTREEITEMDELEGATE_H__
