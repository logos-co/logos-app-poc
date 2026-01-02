#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>

class LogosAPI;

class LogosQmlBridge : public QObject {
    Q_OBJECT
public:
    explicit LogosQmlBridge(LogosAPI* api, QObject* parent = nullptr);

    Q_INVOKABLE QString callModule(const QString& module,
                                   const QString& method,
                                   const QVariantList& args = QVariantList());

private:
    QString serializeResult(const QVariant& result) const;

    LogosAPI* m_logosAPI;
};
