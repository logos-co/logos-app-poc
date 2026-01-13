#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QStringList>
#include <QMap>
#include <QSet>
#include "logos_api.h"
#include "logos_api_client.h"

class PackageManagerBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList packages READ packages NOTIFY packagesChanged)
    Q_PROPERTY(QStringList categories READ categories NOTIFY categoriesChanged)
    Q_PROPERTY(int selectedCategoryIndex READ selectedCategoryIndex WRITE setSelectedCategoryIndex NOTIFY selectedCategoryIndexChanged)
    Q_PROPERTY(QString detailsHtml READ detailsHtml NOTIFY detailsHtmlChanged)
    Q_PROPERTY(bool hasSelectedPackages READ hasSelectedPackages NOTIFY hasSelectedPackagesChanged)
    Q_PROPERTY(bool isInstalling READ isInstalling NOTIFY isInstallingChanged)

public:
    explicit PackageManagerBackend(LogosAPI* logosAPI = nullptr, QObject* parent = nullptr);
    ~PackageManagerBackend() = default;

    QVariantList packages() const;
    QStringList categories() const;
    int selectedCategoryIndex() const;
    void setSelectedCategoryIndex(int index);
    QString detailsHtml() const;
    bool hasSelectedPackages() const;
    bool isInstalling() const;

public slots:
    void reload();
    void install();
    void testPluginCall();
    void selectPackage(int index);
    void togglePackage(int index, bool checked);

signals:
    void packagesChanged();
    void categoriesChanged();
    void selectedCategoryIndexChanged();
    void detailsHtmlChanged();
    void hasSelectedPackagesChanged();
    void isInstallingChanged();
    void packagesInstalled();
    void packageInstalled(const QString& packageName);

private:
    struct PackageInfo {
        QString name;
        QString moduleName;
        QString installedVersion;
        QString latestVersion;
        QString description;
        QStringList files;
        QString category;
        QString type;
        QStringList dependencies;
        bool isLoaded;
        bool isSelected;
    };

    void scanPackagesFolder();
    void clearPackageList();
    void addFallbackPackages();
    void updateFilteredPackages();
    void selectDependencies(const QString& packageName, QSet<QString>& processedPackages);
    void updateHasSelectedPackages();
    void subscribeToInstallationEvents();
    void onPackageInstallationFinished(const QString& packageName, bool success, const QString& error);
    void installNextPackage();

    QMap<QString, PackageInfo> m_allPackages;
    QVariantList m_filteredPackages;
    QStringList m_categories;
    int m_selectedCategoryIndex;
    QString m_detailsHtml;
    bool m_hasSelectedPackages;
    bool m_isProcessingDependencies;
    LogosAPI* m_logosAPI;
    
    bool m_isInstalling;
    QStringList m_pendingPackages;
    QStringList m_successfulPackages;
    QStringList m_failedPackages;
};
