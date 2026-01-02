#include "restricted/DenyAllNetworkAccessManager.h"

#include "restricted/DenyAllReply.h"

QNetworkReply* DenyAllNetworkAccessManager::createRequest(Operation op,
                                                          const QNetworkRequest& request,
                                                          QIODevice* outgoingData)
{
    Q_UNUSED(op);
    Q_UNUSED(outgoingData);
    return new DenyAllReply(request, this);
}
