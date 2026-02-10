#include "LogosQmlBridge.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "logos_api.h"
#include "logos_api_client.h"

LogosQmlBridge::LogosQmlBridge(LogosAPI* api, QObject* parent)
    : QObject(parent)
    , m_logosAPI(api)
{
}

QString LogosQmlBridge::callModule(const QString& module,
                                   const QString& method,
                                   const QVariantList& args)
{
    if (!m_logosAPI) {
        return "{\"error\":\"LogosAPI not available\"}";
    }

    // TODO: restrictions will go here, i.e is this plugin called to call this module and method?
    // will a way to track the origin

    LogosAPIClient* client = m_logosAPI->getClient(module);
    if (!client || !client->isConnected()) {
        return "{\"error\":\"Module not connected\"}";
    }

    QVariant result = client->invokeRemoteMethod(module, method, args);
    if (!result.isValid()) {
        return "{\"error\":\"Invalid response\"}";
    }

    return serializeResult(result);
}

QString LogosQmlBridge::serializeResult(const QVariant& result) const
{
    if (!result.isValid()) {
        return QString();
    }

    QJsonValue jsonValue = QJsonValue::fromVariant(result);
    if (jsonValue.isObject()) {
        QJsonDocument doc(jsonValue.toObject());
        return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    }

    if (jsonValue.isArray()) {
        QJsonDocument doc(jsonValue.toArray());
        return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    }

    if (jsonValue.isString()) {
        return jsonValue.toString();
    }

    if (jsonValue.isDouble()) {
        return QString::number(jsonValue.toDouble(), 'g', 15);
    }

    if (jsonValue.isBool()) {
        return jsonValue.toBool() ? "true" : "false";
    }

    QJsonDocument doc = QJsonDocument::fromVariant(result);
    if (!doc.isNull() && !doc.isEmpty()) {
        return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    }

    return result.toString();
}
