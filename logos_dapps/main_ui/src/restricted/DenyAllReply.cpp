#include "restricted/DenyAllReply.h"

#include <QTimer>
#include <QUrl>

DenyAllReply::DenyAllReply(const QNetworkRequest& request, QObject* parent)
    : QNetworkReply(parent)
{
    setRequest(request);
    setUrl(request.url());
    setOpenMode(QIODevice::ReadOnly);
    setError(QNetworkReply::ContentOperationNotPermittedError,
             QStringLiteral("Network access disabled for this QML engine"));
    QTimer::singleShot(0, this, [this]() {
        emit errorOccurred(error());
        emit finished();
    });
}

void DenyAllReply::abort() {}

bool DenyAllReply::isSequential() const
{
    return true;
}

qint64 DenyAllReply::bytesAvailable() const
{
    return 0;
}

qint64 DenyAllReply::readData(char*, qint64)
{
    return -1;
}

qint64 DenyAllReply::writeData(const char*, qint64)
{
    return -1;
}
