#include "PackageManagerBackend.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPluginLoader>
#include <QTimer>
#include <QStandardPaths>
#include <algorithm>
#include "logos_sdk.h"

PackageManagerBackend::PackageManagerBackend(LogosAPI* logosAPI, QObject* parent)
    : QObject(parent)
    , m_selectedCategoryIndex(0)
    , m_detailsHtml("Select a package to view its details.")
    , m_hasSelectedPackages(false)
    , m_isProcessingDependencies(false)
    , m_logosAPI(logosAPI)
    , m_isInstalling(false)
{
    m_categories << "All";
    subscribeToInstallationEvents();
    scanPackagesFolder();
}

QVariantList PackageManagerBackend::packages() const
{
    return m_filteredPackages;
}

QStringList PackageManagerBackend::categories() const
{
    return m_categories;
}

int PackageManagerBackend::selectedCategoryIndex() const
{
    return m_selectedCategoryIndex;
}

void PackageManagerBackend::setSelectedCategoryIndex(int index)
{
    if (m_selectedCategoryIndex != index && index >= 0 && index < m_categories.size()) {
        m_selectedCategoryIndex = index;
        emit selectedCategoryIndexChanged();
        updateFilteredPackages();
    }
}

QString PackageManagerBackend::detailsHtml() const
{
    return m_detailsHtml;
}

bool PackageManagerBackend::hasSelectedPackages() const
{
    return m_hasSelectedPackages;
}

bool PackageManagerBackend::isInstalling() const
{
    return m_isInstalling;
}

void PackageManagerBackend::reload()
{
    scanPackagesFolder();
}

void PackageManagerBackend::subscribeToInstallationEvents()
{
    if (!m_logosAPI) {
        return;
    }
    
    LogosAPIClient* client = m_logosAPI->getClient("package_manager");
    if (!client || !client->isConnected()) {
        return;
    }
    
    LogosModules logos(m_logosAPI);
    logos.package_manager.on("packageInstallationFinished", [this](const QVariantList& data) {
        if (data.size() < 3) {
            return;
        }
        QString packageName = data[0].toString();
        bool success = data[1].toBool();
        QString error = data[2].toString();
        
        QTimer::singleShot(0, this, [this, packageName, success, error]() {
            onPackageInstallationFinished(packageName, success, error);
        });
    });
}

void PackageManagerBackend::onPackageInstallationFinished(const QString& packageName, bool success, const QString& error)
{
    if (success) {
        m_successfulPackages << packageName;
        emit packageInstalled(packageName);
    } else {
        QString failureReason = error.isEmpty() ? "installation failed" : error;
        m_failedPackages << packageName + " (" + failureReason + ")";
    }
    
    installNextPackage();
}

void PackageManagerBackend::installNextPackage()
{
    if (m_pendingPackages.isEmpty()) {
        m_isInstalling = false;
        emit isInstallingChanged();
        
        QString resultText = "<h3>Installation Results</h3>";

        if (!m_successfulPackages.isEmpty()) {
            resultText += "<p><b>Successfully installed:</b></p><ul>";
            for (const QString& plugin : m_successfulPackages) {
                resultText += "<li>" + plugin + "</li>";
            }
            resultText += "</ul>";
        }

        if (!m_failedPackages.isEmpty()) {
            resultText += "<p><b>Failed to install:</b></p><ul>";
            for (const QString& plugin : m_failedPackages) {
                resultText += "<li>" + plugin + "</li>";
            }
            resultText += "</ul>";
        }

        if (!m_successfulPackages.isEmpty()) {
            emit packagesInstalled();
        }

        m_detailsHtml = resultText;
        emit detailsHtmlChanged();

        scanPackagesFolder();
        return;
    }
    
    QString packageName = m_pendingPackages.takeFirst();
    
    if (!m_allPackages.contains(packageName)) {
        m_failedPackages << packageName + " (package not found)";
        installNextPackage();
        return;
    }
    
    if (!m_logosAPI || !m_logosAPI->getClient("package_manager")->isConnected()) {
        m_failedPackages << packageName + " (package_manager not connected)";
        installNextPackage();
        return;
    }
    
    m_detailsHtml = QString("<h3>Installing...</h3><p>Installing package: <b>%1</b></p><p>%2 package(s) remaining in queue.</p>")
        .arg(packageName)
        .arg(m_pendingPackages.size());
    emit detailsHtmlChanged();
    
    LogosModules logos(m_logosAPI);
    
    // Determine package type to route to correct directory
    QString packageType = m_allPackages[packageName].type;
    bool isUiPlugin = (packageType.compare("ui", Qt::CaseInsensitive) == 0);
    
    QDir appDir(QCoreApplication::applicationDirPath());
    appDir.cdUp();
    
    QString installDir;
    if (isUiPlugin) {
        // UI plugins go to /plugins directory
#ifdef LOGOS_DISTRIBUTED_BUILD
        // For distributed builds (DMG/AppImage), use Application Support
        installDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/plugins";
#else
        // For development builds (nix), use bundled plugins directory
        installDir = QDir::cleanPath(appDir.absolutePath() + "/plugins");
#endif
    } else {
        // Core modules go to /modules directory
#ifdef LOGOS_DISTRIBUTED_BUILD
        // For distributed builds (DMG/AppImage), use Application Support
        installDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/modules";
#else
        // For development builds (nix), use bundled modules directory
        installDir = QDir::cleanPath(appDir.absolutePath() + "/modules");
#endif
    }
    
    logos.package_manager.installPackageAsync(packageName, installDir);
}

void PackageManagerBackend::install()
{
    if (m_isInstalling) {
        m_detailsHtml = "<p><b>Installation already in progress.</b> Please wait for it to complete.</p>";
        emit detailsHtmlChanged();
        return;
    }
    
    m_pendingPackages.clear();
    m_successfulPackages.clear();
    m_failedPackages.clear();
    
    for (auto it = m_allPackages.begin(); it != m_allPackages.end(); ++it) {
        if (it.value().isSelected) {
            m_pendingPackages << it.key();
        }
    }

    if (m_pendingPackages.isEmpty()) {
        m_detailsHtml = "No packages selected. Select at least one package to install.";
        emit detailsHtmlChanged();
        return;
    }

    m_isInstalling = true;
    emit isInstallingChanged();
    
    m_detailsHtml = QString("<h3>Starting Installation...</h3><p>%1 package(s) to install.</p>")
        .arg(m_pendingPackages.size());
    emit detailsHtmlChanged();
    
    installNextPackage();
}

void PackageManagerBackend::testPluginCall()
{
    if (m_logosAPI && m_logosAPI->getClient("package_manager")->isConnected()) {
        LogosModules logos(m_logosAPI);
        const QString result = logos.package_manager.testPluginCall("my test string");
        m_detailsHtml = QString("<h3>Test Call Result</h3><p>%1</p>")
                           .arg(result.toHtmlEscaped());
    } else {
        m_detailsHtml = "<p><b>Error:</b> package_manager not connected</p>";
    }
    emit detailsHtmlChanged();
}

void PackageManagerBackend::selectPackage(int index)
{
    if (index < 0 || index >= m_filteredPackages.size()) {
        return;
    }

    QVariantMap pkg = m_filteredPackages[index].toMap();
    QString packageName = pkg["name"].toString();
    QString installedVer = pkg["installedVersion"].toString();
    QString latestVer = pkg["latestVersion"].toString();
    QString description = pkg["description"].toString();
    QString type = pkg["type"].toString();
    QString moduleName = pkg["moduleName"].toString();
    QString category = pkg["category"].toString();

    if (m_allPackages.contains(packageName)) {
        const PackageInfo& info = m_allPackages[packageName];

        QString detailText = QString("<h2>%1</h2>").arg(packageName);
        
        if (!moduleName.isEmpty() && moduleName != packageName) {
            detailText += QString("<p><b>Module Name:</b> %1</p>").arg(moduleName);
        }
        
        detailText += QString("<p><b>Description:</b> %1</p>").arg(description.isEmpty() ? "No description available" : description);
        
        if (!type.isEmpty()) {
            detailText += QString("<p><b>Type:</b> %1</p>").arg(type);
        }
        
        if (!category.isEmpty()) {
            detailText += QString("<p><b>Category:</b> %1</p>").arg(category);
        }
        
        if (!installedVer.isEmpty()) {
            detailText += QString("<p><b>Installed Version:</b> %1</p>").arg(installedVer);
        }
        if (!latestVer.isEmpty()) {
            detailText += QString("<p><b>Latest Version:</b> %1</p>").arg(latestVer);
        }

        if (!info.files.isEmpty()) {
            detailText += "<p><b>Files:</b></p><ul>";
            for (const QString& file : info.files) {
                detailText += QString("<li>%1</li>").arg(file);
            }
            detailText += "</ul>";
        }

        if (!info.dependencies.isEmpty()) {
            detailText += "<p><b>Dependencies:</b></p><ul>";
            for (const QString& dependency : info.dependencies) {
                detailText += QString("<li>%1</li>").arg(dependency);
            }
            detailText += "</ul>";
        } else {
            detailText += "<p><b>Dependencies:</b> None</p>";
        }

        m_detailsHtml = detailText;
    } else {
        QString detailText = QString("<h2>%1</h2>").arg(packageName);
        detailText += QString("<p><b>Description:</b> %1</p>").arg(description);
        detailText += QString("<p><b>Installed Version:</b> %1</p>").arg(installedVer);
        detailText += QString("<p><b>Latest Version:</b> %1</p>").arg(latestVer);

        if (packageName == "0ad") {
            detailText += QString("<p>0 A.D. (pronounced \"zero eye-dee\") is a free, open-source, cross-platform "
                                "real-time strategy (RTS) game of ancient warfare.</p>");
        }

        m_detailsHtml = detailText;
    }

    emit detailsHtmlChanged();
}

void PackageManagerBackend::togglePackage(int index, bool checked)
{
    if (index < 0 || index >= m_filteredPackages.size() || m_isProcessingDependencies) {
        return;
    }

    QVariantMap pkg = m_filteredPackages[index].toMap();
    QString packageName = pkg["name"].toString();

    if (m_allPackages.contains(packageName)) {
        m_allPackages[packageName].isSelected = checked;

        if (checked) {
            m_isProcessingDependencies = true;
            QSet<QString> processedPackages;
            selectDependencies(packageName, processedPackages);
            m_isProcessingDependencies = false;
        }

        updateFilteredPackages();
        updateHasSelectedPackages();
    }
}

void PackageManagerBackend::scanPackagesFolder()
{
    clearPackageList();

    QJsonArray packagesArray;
    if (m_logosAPI && m_logosAPI->getClient("package_manager")->isConnected()) {
        LogosModules logos(m_logosAPI);
        QDir appDir(QCoreApplication::applicationDirPath());
        appDir.cdUp();
        QString modulesDir = QDir::cleanPath(appDir.absolutePath() + "/modules");
        logos.package_manager.setPluginsDirectory(modulesDir);
        packagesArray = logos.package_manager.getPackages();
        qDebug() << "LogosAPI: Retrieved" << packagesArray.size() << "packages from package_manager";
    } else {
        qDebug() << "LogosAPI not connected, cannot get packages from package_manager";
        addFallbackPackages();
        updateFilteredPackages();
        return;
    }

    if (packagesArray.isEmpty()) {
        addFallbackPackages();
        updateFilteredPackages();
        return;
    }

    QSet<QString> categorySet;

    for (const QJsonValue& value : packagesArray) {
        QJsonObject obj = value.toObject();
        QString name = obj.value("name").toString();
        QString description = obj.value("description").toString();
        QString type = obj.value("type").toString();
        QString moduleName = obj.value("moduleName").toString();
        QString category = obj.value("category").toString();
        
        QStringList files;
        QJsonArray filesArray = obj.value("files").toArray();
        for (const QJsonValue& fileVal : filesArray) {
            files.append(fileVal.toString());
        }

        QStringList dependencies;
        QJsonArray dependenciesArray = obj.value("dependencies").toArray();
        for (const QJsonValue& depVal : dependenciesArray) {
            dependencies.append(depVal.toString());
        }

        bool isInstalled = obj.value("installed").toBool(false);

        QString installedVersion = "";
        QString latestVersion = "";
        
        if (category.isEmpty()) {
            category = "Module";
        }
        categorySet.insert(category);

        if (type.isEmpty()) {
            type = "Plugin";
        }

        PackageInfo info;
        info.name = name;
        info.moduleName = moduleName.isEmpty() ? name : moduleName;
        info.installedVersion = installedVersion;
        info.latestVersion = latestVersion;
        info.description = description;
        info.files = files;
        info.isLoaded = isInstalled;
        info.isSelected = false;
        info.category = category;
        info.type = type;
        info.dependencies = dependencies;
        m_allPackages[name] = info;
    }

    QStringList sortedCategories;
    for (const QString& category : categorySet) {
        if (!category.isEmpty()) {
            QString capitalizedCategory = category;
            capitalizedCategory[0] = capitalizedCategory[0].toUpper();
            sortedCategories.append(capitalizedCategory);
        }
    }
    std::sort(sortedCategories.begin(), sortedCategories.end());
    
    m_categories.clear();
    m_categories << "All";
    m_categories.append(sortedCategories);
    emit categoriesChanged();

    if (m_allPackages.isEmpty()) {
        addFallbackPackages();
    }

    updateFilteredPackages();
    m_isProcessingDependencies = false;
}

void PackageManagerBackend::clearPackageList()
{
    m_allPackages.clear();
    m_filteredPackages.clear();
    m_categories.clear();
    m_categories << "All";
    m_selectedCategoryIndex = 0;
    emit categoriesChanged();
    emit selectedCategoryIndexChanged();
}

void PackageManagerBackend::addFallbackPackages()
{
    auto addPkg = [this](const QString& name, const QString& installedVer, 
                         const QString& latestVer, const QString& type, 
                         const QString& desc, bool selected = false) {
        PackageInfo info;
        info.name = name;
        info.moduleName = name;
        info.installedVersion = installedVer;
        info.latestVersion = latestVer;
        info.type = type;
        info.description = desc;
        info.isLoaded = selected;
        info.isSelected = selected;
        info.category = type;
        info.dependencies = QStringList();
        m_allPackages[name] = info;
    };

    addPkg("0ad", "0.0.23.1-4ubuntu3", "0.0.23.1-4ubuntu3", "Game", "Real-time strategy game of ancient warfare", true);
    addPkg("0ad-data", "0.0.23.1-1", "0.0.23.1-1", "Data", "Real-time strategy game of ancient warfare");
    addPkg("0ad-data-common", "0.0.23.1-1", "0.0.23.1-1", "Data", "Real-time strategy game of ancient warfare");
    addPkg("0install", "2.15.1-1", "2.15.1-1", "System", "cross-distribution packaging system");
    addPkg("0install-core", "2.15.1-1", "2.15.1-1", "System", "cross-distribution packaging system");
    addPkg("0xffff", "0.8-1", "0.8-1", "Utility", "Open Free Fiasco Firmware");
    addPkg("2048-qt", "0.1.6-2build1", "0.1.6-2build1", "Game", "mathematics based puzzle game");

    QSet<QString> categorySet;
    for (auto it = m_allPackages.begin(); it != m_allPackages.end(); ++it) {
        if (!it.value().category.isEmpty()) {
            categorySet.insert(it.value().category);
        }
    }
    
    QStringList sortedCategories;
    for (const QString& cat : categorySet) {
        sortedCategories.append(cat);
    }
    std::sort(sortedCategories.begin(), sortedCategories.end());
    
    m_categories.clear();
    m_categories << "All";
    m_categories.append(sortedCategories);
    emit categoriesChanged();
}

void PackageManagerBackend::updateFilteredPackages()
{
    m_filteredPackages.clear();

    QString selectedCategory = m_categories.value(m_selectedCategoryIndex, "All");
    bool showAll = (selectedCategory.compare("All", Qt::CaseInsensitive) == 0);

    for (auto it = m_allPackages.begin(); it != m_allPackages.end(); ++it) {
        const PackageInfo& info = it.value();
        
        if (showAll || info.category.compare(selectedCategory, Qt::CaseInsensitive) == 0) {
            QVariantMap pkg;
            pkg["name"] = info.name;
            pkg["moduleName"] = info.moduleName;
            pkg["installedVersion"] = info.installedVersion;
            pkg["latestVersion"] = info.latestVersion;
            pkg["type"] = info.type;
            pkg["description"] = info.description;
            pkg["category"] = info.category;
            pkg["isSelected"] = info.isSelected;
            m_filteredPackages.append(pkg);
        }
    }

    emit packagesChanged();
}

void PackageManagerBackend::selectDependencies(const QString& packageName, QSet<QString>& processedPackages)
{
    if (processedPackages.contains(packageName)) {
        return;
    }

    processedPackages.insert(packageName);

    if (!m_allPackages.contains(packageName)) {
        return;
    }

    PackageInfo& info = m_allPackages[packageName];
    info.isSelected = true;

    for (const QString& dependency : info.dependencies) {
        selectDependencies(dependency, processedPackages);
    }
}

void PackageManagerBackend::updateHasSelectedPackages()
{
    bool anySelected = false;
    for (auto it = m_allPackages.begin(); it != m_allPackages.end(); ++it) {
        if (it.value().isSelected) {
            anySelected = true;
            break;
        }
    }

    if (m_hasSelectedPackages != anySelected) {
        m_hasSelectedPackages = anySelected;
        emit hasSelectedPackagesChanged();
    }
}
