#include "restricted/DenyAllNAMFactory.h"

#include "restricted/DenyAllNetworkAccessManager.h"

QNetworkAccessManager* DenyAllNAMFactory::create(QObject* parent)
{
    return new DenyAllNetworkAccessManager(parent);
}
