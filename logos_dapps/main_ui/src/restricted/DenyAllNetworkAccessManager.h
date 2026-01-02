#pragma once

#include <QNetworkAccessManager>
#include <QNetworkRequest>

class QIODevice;

class DenyAllNetworkAccessManager : public QNetworkAccessManager {
public:
    using QNetworkAccessManager::QNetworkAccessManager;

protected:
    QNetworkReply* createRequest(Operation op,
                                 const QNetworkRequest& request,
                                 QIODevice* outgoingData = nullptr) override;
};
