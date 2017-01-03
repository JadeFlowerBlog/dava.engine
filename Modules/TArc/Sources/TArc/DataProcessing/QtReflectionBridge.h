#pragma once

#include "TArc/DataProcessing/DataWrapper.h"
#include "TArc/DataProcessing/DataListener.h"
#include "Functional/Signal.h"
#include "Base/BaseTypes.h"

#include <QObject>
#include <QPointer>
#include <QMetaType>

namespace DAVA
{
namespace TArc
{
class QtReflectionBridge;
class QtReflected final : public QObject, private DataListener
{
public:
    QtReflected() = default;
    QtReflected(const QtReflected& other);
    QtReflected& operator=(const QtReflected& other);

    const QMetaObject* metaObject() const override;
    int qt_metacall(QMetaObject::Call c, int id, void** argv) override;
    void Init();

    Signal<> metaObjectCreated;

private:
    friend class QtReflectionBridge;
    QtReflected(QtReflectionBridge* reflectionBridge, DataWrapper&& wrapper, QObject* parent);

    void OnDataChanged(const DataWrapper& wrapper, const Vector<Any>& fields) override;
    void CreateMetaObject();

    void FirePropertySignal(const String& propertyName);
    void FirePropertySignal(int signalId);
    void CallMethod(int id, void** argv);

private:
    QPointer<QtReflectionBridge> reflectionBridge;
    DataWrapper wrapper;
    QMetaObject* qtMetaObject = nullptr;
};

class QtReflectionBridge final : public QObject
{
public:
    QtReflectionBridge();
    ~QtReflectionBridge();
    QtReflected* CreateQtReflected(DataWrapper&& wrapper, QObject* parent);

private:
    QVariant Convert(const Any& value) const;
    Any Convert(const QVariant& value) const;
    friend class QtReflected;
    UnorderedMap<const ReflectedType*, QMetaObject*> metaObjects;

    UnorderedMap<const Type*, QVariant (*)(const Any&)> anyToQVariant;
    UnorderedMap<int, Any (*)(const QVariant&)> qvariantToAny;
};
} // namespace TArc
} // namespace DAVA
