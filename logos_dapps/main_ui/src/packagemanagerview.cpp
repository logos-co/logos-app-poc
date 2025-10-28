#include "packagemanagerview.h"
#include "mainwindow.h"
#include <QFont>
#include <QHeaderView>
#include <QIcon>
#include <QCheckBox>
#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QMetaObject>
#include <algorithm>
#include <QSizePolicy>
#include <QDir>
#include <QFile>
#include <QTimer>
#include "logos_sdk.h"

PackageManagerView::PackageManagerView(QWidget *parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_splitter(nullptr)
    , m_sidebarWidget(nullptr)
    , m_sidebarLayout(nullptr)
    , m_categoryList(nullptr)
    , m_contentWidget(nullptr)
    , m_contentLayout(nullptr)
    , m_packageTable(nullptr)
    , m_buttonLayout(nullptr)
    , m_reloadButton(nullptr)
    , m_applyButton(nullptr)
    , m_detailsTextEdit(nullptr)
    , m_mainWindow(nullptr)
    , m_logosAPI(nullptr)
{
    // Create own LogosAPI instance if none provided
    // m_logosAPI = new LogosAPI("core", this);
    
    setupUi();
}

PackageManagerView::PackageManagerView(LogosAPI* logosAPI, QWidget *parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_splitter(nullptr)
    , m_sidebarWidget(nullptr)
    , m_sidebarLayout(nullptr)
    , m_categoryList(nullptr)
    , m_contentWidget(nullptr)
    , m_contentLayout(nullptr)
    , m_packageTable(nullptr)
    , m_buttonLayout(nullptr)
    , m_reloadButton(nullptr)
    , m_applyButton(nullptr)
    , m_detailsTextEdit(nullptr)
    , m_mainWindow(nullptr)
    , m_logosAPI(logosAPI)
{
    setupUi();
}

PackageManagerView::~PackageManagerView()
{
    // LogosAPI will be automatically deleted as it's a child object
}

void PackageManagerView::setupUi()
{
    // Set proper size policy for the widget to make it resizable
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Create main layout
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(20);
    m_layout->setContentsMargins(40, 40, 40, 40);
    
    // Create and style the title
    QLabel* titleLabel = new QLabel("Package Manager", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #ffffff;");
    
    // Add a subtitle
    QLabel* subtitleLabel = new QLabel("Manage plugins and packages", this);
    subtitleLabel->setStyleSheet("color: #a0a0a0; font-size: 14px; margin-bottom: 20px;");
    
    // Create button layout
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(10);
    
    // Create buttons with icons
    m_reloadButton = new QPushButton(QIcon(":/icons/modules.png"), "Reload");
    m_applyButton = new QPushButton(QIcon(":/icons/modules.png"), "Install");
    m_applyButton->setEnabled(false);
    
    // Connect reload button
    connect(m_reloadButton, &QPushButton::clicked, this, &PackageManagerView::onReloadClicked);
    
    // Connect apply button
    connect(m_applyButton, &QPushButton::clicked, this, &PackageManagerView::onApplyClicked);
    
    // Add buttons to layout
    m_buttonLayout->addWidget(m_reloadButton);
    m_buttonLayout->addWidget(m_applyButton);
    m_buttonLayout->addStretch();
    
    // Create splitter for sidebar and package content
    m_splitter = new QSplitter(Qt::Horizontal);
    
    // Create and setup sidebar
    createCategorySidebar();
    
    // Create content widget (contains table and details)
    m_contentWidget = new QWidget();
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setSpacing(10);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create the package table
    createPackageTable();
    
    // Create package details section
    createPackageDetails();
    
    // Add table and details to content layout
    m_contentLayout->addWidget(m_packageTable);
    m_contentLayout->addWidget(m_detailsTextEdit);
    
    // Set initial sizing for the table and details
    m_packageTable->setMinimumHeight(300);
    // Set the splitter to be properly resizable
    m_splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Add widgets to splitter
    m_splitter->addWidget(m_sidebarWidget);
    m_splitter->addWidget(m_contentWidget);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    
    // Add widgets to main layout
    m_layout->addWidget(titleLabel);
    m_layout->addWidget(subtitleLabel);
    m_layout->addLayout(m_buttonLayout);
    m_layout->addWidget(m_splitter, 1); // Give the splitter a stretch factor of 1
    
    // Set the details text to a simple instruction
    m_detailsTextEdit->setText("Select a package to view its details.");
    
    // Scan packages folder
    scanPackagesFolder();
    
    // Select "All" category by default
    if (m_categoryList->count() > 0) {
        m_categoryList->setCurrentRow(0);
    }
}

void PackageManagerView::createCategorySidebar()
{
    // Create sidebar widget
    m_sidebarWidget = new QWidget();
    m_sidebarWidget->setMinimumWidth(180);
    m_sidebarWidget->setMaximumWidth(250);
    m_sidebarWidget->setStyleSheet("background-color: #2D2D2D;");
    
    // Create sidebar layout
    m_sidebarLayout = new QVBoxLayout(m_sidebarWidget);
    m_sidebarLayout->setSpacing(0);
    m_sidebarLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create category list
    m_categoryList = new QListWidget();
    m_categoryList->setStyleSheet(
        "QListWidget { "
        "  background-color: #2D2D2D; "
        "  color: #ffffff; "
        "  border: none; "
        "}"
        "QListWidget::item { "
        "  padding: 10px 15px; "
        "  border-radius: 3px; "
        "  margin: 3px 5px; "
        "}"
        "QListWidget::item:selected { "
        "  background-color: #3D3D3D; "
        "  color: #ffffff; "
        "}"
        "QListWidget::item:hover:!selected { "
        "  background-color: #353535; "
        "}"
    );
    
    // Add "All" category item by default
    m_categoryList->addItem("All");
    
    // We'll add the rest of the categories dynamically during scanPackagesFolder()
    
    // Connect signals
    connect(m_categoryList, &QListWidget::currentRowChanged, this, &PackageManagerView::onCategorySelected);
    
    // Add to sidebar layout
    m_sidebarLayout->addWidget(m_categoryList);
}

void PackageManagerView::createPackageTable()
{
    m_packageTable = new QTableWidget();
    m_packageTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_packageTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_packageTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_packageTable->setShowGrid(false);
    m_packageTable->setAlternatingRowColors(true);
    m_packageTable->setStyleSheet("QTableWidget { background-color: #333333; color: #ffffff; border: 1px solid #000000; }"
                                 "QTableWidget::item:alternate { background-color: #2a2a2a; }"
                                 "QTableWidget::item:selected { background-color: #4A90E2; color: #ffffff; }"
                                 "QTableWidget::item { border-bottom: 1px solid #444444; }");

    m_packageTable->setRowCount(0);
    m_packageTable->verticalHeader()->setVisible(false);

    // Set columns
    m_packageTable->setColumnCount(6);
    QStringList headers;
    headers << "S" << "Package" << "Installed Ver" << "Latest Version" << "Type" << "Description";
    m_packageTable->setHorizontalHeaderLabels(headers);

    // Configure header
    QHeaderView* header = m_packageTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setDefaultSectionSize(150);
    header->setSectionsClickable(true);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(5, QHeaderView::Stretch);

    header->setStyleSheet("QHeaderView::section { background-color: #333333; color: #a0a0a0; font-weight: bold; }");

    QHeaderView* verticalHeader = m_packageTable->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(30);
    verticalHeader->setMinimumWidth(30);

    // Set column widths
    m_packageTable->setColumnWidth(0, 30);   // S column (checkbox)
    m_packageTable->setColumnWidth(2, 150);  // Installed Version
    m_packageTable->setColumnWidth(3, 150);  // Latest Version
    m_packageTable->setColumnWidth(4, 100);  // Type

    // Connect package selection signal
    connect(m_packageTable, &QTableWidget::cellClicked, this, &PackageManagerView::onPackageSelected);

    // Connect checkbox state change signal - use itemChanged signal which sends QTableWidgetItem*
    connect(m_packageTable, &QTableWidget::itemChanged, this, &PackageManagerView::onPackageCheckStateChanged);
}

void PackageManagerView::createPackageDetails()
{
    // Create details text edit
    m_detailsTextEdit = new QTextEdit();
    m_detailsTextEdit->setReadOnly(true);
    m_detailsTextEdit->setStyleSheet("QTextEdit { background-color: #333333; color: #ffffff; border: 1px solid #444444; }");

    // Set a proper size policy to allow resizing
    m_detailsTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void PackageManagerView::addPackage(const QString& name, const QString& installedVersion, 
                                 const QString& latestVersion, const QString& type,
                                 const QString& description, bool checked)
{
    int row = m_packageTable->rowCount();
    m_packageTable->insertRow(row);

    // Set row number (1-based for display)
    m_packageTable->setVerticalHeaderItem(row, new QTableWidgetItem(QString::number(row + 1)));

    // Create checkbox item for the S column
    QTableWidgetItem* checkItem = new QTableWidgetItem();
    checkItem->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    m_packageTable->setItem(row, 0, checkItem);

    // Add other columns
    m_packageTable->setItem(row, 1, new QTableWidgetItem(name));
    m_packageTable->setItem(row, 2, new QTableWidgetItem(installedVersion));
    m_packageTable->setItem(row, 3, new QTableWidgetItem(latestVersion));
    m_packageTable->setItem(row, 4, new QTableWidgetItem(type));
    m_packageTable->setItem(row, 5, new QTableWidgetItem(description));

    // Update Install button enabled state
    updateInstallButtonState();
}

void PackageManagerView::onCategorySelected(int row)
{
    QString category = m_categoryList->item(row)->text();
    
    // Clear the table
    m_packageTable->setRowCount(0);
    
    qDebug("Category selected: %s", qPrintable(category));
    
    // If "All" is selected, show all packages
    if (category.compare("All", Qt::CaseInsensitive) == 0) {
        for (const auto& info : m_packages) {
            addPackage(info.name, info.installedVersion, info.latestVersion, info.type,
                      info.description, info.isLoaded);
        }
    } else {
        // Show only packages in the selected category (case insensitive comparison)
        for (const auto& info : m_packages) {
            if (info.category.compare(category, Qt::CaseInsensitive) == 0) {
                addPackage(info.name, info.installedVersion, info.latestVersion, info.type,
                          info.description, info.isLoaded);
            }
        }
    }
}

void PackageManagerView::onPackageSelected(int row, int column)
{
    // Get package information
    QString packageName = m_packageTable->item(row, 1)->text();
    QString installedVer = m_packageTable->item(row, 2)->text();
    QString latestVer = m_packageTable->item(row, 3)->text();
    QString description = m_packageTable->item(row, 5)->text();
    
    // Check if this is one of our scanned packages
    if (m_packages.contains(packageName)) {
        const PackageInfo& info = m_packages[packageName];
        
        // Get additional metadata if available
        QPluginLoader loader(info.path);
        QJsonObject metadata = loader.metaData();
        QJsonObject metaDataObj = metadata.value("MetaData").toObject();
        
        // Format detailed information
        QString detailText = QString("<h2>%1</h2>").arg(packageName);
        detailText += QString("<p><b>Description:</b> %1</p>").arg(description);
        detailText += QString("<p><b>Installed Version:</b> %1</p>").arg(installedVer);
        detailText += QString("<p><b>Latest Version:</b> %1</p>").arg(latestVer);
        
        if (!info.path.isEmpty()) {
            detailText += QString("<p><b>Path:</b> %1</p>").arg(info.path);
        }
        
        // Add author information if available
        QString author = metaDataObj.value("author").toString();
        if (!author.isEmpty()) {
            detailText += QString("<p><b>Author:</b> %1</p>").arg(author);
        }
        
        // Add type information if available
        QString type = metaDataObj.value("type").toString();
        if (!type.isEmpty()) {
            detailText += QString("<p><b>Type:</b> %1</p>").arg(type);
        }
        
        // Add dependencies information
        if (!info.dependencies.isEmpty()) {
            detailText += "<p><b>Dependencies:</b></p><ul>";
            for (const QString& dependency : info.dependencies) {
                detailText += QString("<li>%1</li>").arg(dependency);
            }
            detailText += "</ul>";
        } else {
            detailText += "<p><b>Dependencies:</b> None</p>";
        }
        
        // Add capabilities information if available
        QJsonArray capabilities = metaDataObj.value("capabilities").toArray();
        if (!capabilities.isEmpty()) {
            detailText += "<p><b>Capabilities:</b></p><ul>";
            for (const QJsonValue& capability : capabilities) {
                detailText += QString("<li>%1</li>").arg(capability.toString());
            }
            detailText += "</ul>";
        }
        
        // Set the details text
        m_detailsTextEdit->setHtml(detailText);
    } else {
        // Format detailed information for default packages
        QString detailText = QString("<h2>%1</h2>").arg(packageName);
        detailText += QString("<p><b>Description:</b> %1</p>").arg(description);
        detailText += QString("<p><b>Installed Version:</b> %1</p>").arg(installedVer);
        detailText += QString("<p><b>Latest Version:</b> %1</p>").arg(latestVer);
        
        // For 0ad, show some additional information
        if (packageName == "0ad") {
            detailText += QString("<p>0 A.D. (pronounced \"zero eye-dee\") is a free, open-source, cross-platform "
                                "real-time strategy (RTS) game of ancient warfare. In short, it is a historically-based "
                                "war/economy game that allows players to relive or rewrite the history of Western "
                                "civilizations, focusing on the years between 500 B.C. and 500 A.D.</p>");
            
            detailText += QString("<p><a href='#'>Visit Homepage</a> | <a href='#'>View Changelog</a></p>");
        }
        
        // Set the details text
        m_detailsTextEdit->setHtml(detailText);
    }
}

void PackageManagerView::scanPackagesFolder()
{
    clearPackageList();

    QJsonArray packagesArray;
    if (m_logosAPI && m_logosAPI->getClient("package_manager")->isConnected()) {
        LogosModules logos(m_logosAPI);
        packagesArray = logos.package_manager.getPackages();
        qDebug() << "LogosAPI: Retrieved" << packagesArray.size() << "packages from package_manager";
    } else {
        qDebug() << "LogosAPI not connected, cannot get packages from package_manager";
        addFallbackPackages();
        return;
    }

    qDebug() << "================================================";
    qDebug() << "packagesArray:" << packagesArray;
    if (packagesArray.isEmpty()) {
        addFallbackPackages();
        return;
    }

    // Set to store unique categories
    QSet<QString> categories;

    // Process each package from the array
    int loadedCount = 0;
    for (const QJsonValue& value : packagesArray) {
        QJsonObject obj = value.toObject();
        QString name = obj.value("name").toString();
        QString installedVersion = obj.value("installedVersion").toString();
        QString latestVersion = obj.value("latestVersion").toString();
        QString description = obj.value("description").toString();
        QString type = obj.value("type").toString();
        QString category = obj.value("category").toString();
        categories.insert(category);

        // Store package info
        PackageInfo info;
        info.name = name;
        info.installedVersion = installedVersion;
        info.latestVersion = latestVersion;
        info.description = description;
        info.path = obj.value("path").toString();
        info.isLoaded = false;
        info.category = category;
        info.type = type;
        // Dependencies
        QStringList dependencies;
        QJsonArray depsArray = obj.value("dependencies").toArray();
        for (const QJsonValue& dep : depsArray) {
            dependencies.append(dep.toString());
        }
        info.dependencies = dependencies;
        m_packages[name] = info;
        addPackage(name, installedVersion, latestVersion, type, description);
        loadedCount++;
    }

    // Add the categories to the sidebar
    QStringList sortedCategories;
    for (const QString& category : categories) {
        sortedCategories.append(category);
    }
    std::sort(sortedCategories.begin(), sortedCategories.end());
    for (const QString& category : sortedCategories) {
        if (!category.isEmpty()) {
            QString capitalizedCategory = category;
            capitalizedCategory[0] = capitalizedCategory[0].toUpper();
            m_categoryList->addItem(capitalizedCategory);
        }
    }

    qDebug("Loaded %d packages from package_manager", loadedCount);

    if (loadedCount == 0) {
        addFallbackPackages();
    }

    m_isProcessingDependencies = false;
}

void PackageManagerView::clearPackageList()
{
    // Clear the package table
    m_packageTable->setRowCount(0);
    
    // Clear package map
    m_packages.clear();
    
    // Clear category list except for "All"
    while (m_categoryList->count() > 1) {
        delete m_categoryList->takeItem(1);
    }
}

void PackageManagerView::onReloadClicked()
{
    // Scan packages folder again
    scanPackagesFolder();
}

void PackageManagerView::addFallbackPackages()
{
    // Add some demo packages for UI demonstration with dependencies
    addPackage("0ad", "0.0.23.1-4ubuntu3", "0.0.23.1-4ubuntu3", "Game", "Real-time strategy game of ancient warfare", true);
    addPackage("0ad-data", "0.0.23.1-1", "0.0.23.1-1", "Data", "Real-time strategy game of ancient warfare");
    addPackage("0ad-data-common", "0.0.23.1-1", "0.0.23.1-1", "Data", "Real-time strategy game of ancient warfare");
    addPackage("0install", "2.15.1-1", "2.15.1-1", "System", "cross-distribution packaging system");
    addPackage("0install-core", "2.15.1-1", "2.15.1-1", "System", "cross-distribution packaging system");
    addPackage("0xffff", "0.8-1", "0.8-1", "Utility", "Open Free Fiasco Firmware");
    addPackage("2048-qt", "0.1.6-2build1", "0.1.6-2build1", "Game", "mathematics based puzzle game");
}

void PackageManagerView::onApplyClicked()
{
    QList<QString> selectedPackages = getSelectedPackages();

    if (selectedPackages.isEmpty()) {
        m_detailsTextEdit->setText("No packages selected. Select at least one package to install.");
        return;
    }

    // Process each selected package
    QStringList successfulPlugins;
    QStringList failedPlugins;

    for (const QString& packageName : selectedPackages) {
        if (!m_packages.contains(packageName)) {
            failedPlugins << packageName + " (package not found)";
            continue;
        }

        const PackageInfo& info = m_packages[packageName];
        QString filePath = info.path;

        // TEMPORARY CHANGE: Special handling for UI plugins - copy directly to ./plugins directory
        // for demonstration purposes, this should be done through the core
        if (info.type.compare("UI", Qt::CaseInsensitive) == 0) {
            // Get the application directory path and create the plugins directory path
            QString appDir = QCoreApplication::applicationDirPath();
            QString pluginsDir = appDir + "/../plugins";

            // Make sure the plugins directory exists
            QDir dir;
            if (!dir.exists(pluginsDir)) {
                dir.mkpath(pluginsDir);
            }

            // Generate the destination file path
            QFileInfo fileInfo(filePath);
            QString destFilePath = pluginsDir + "/" + fileInfo.fileName();

            // Copy the file
            bool copySuccess = QFile::copy(filePath, destFilePath);
            if (copySuccess) {
                // Set permissions to make it writeable
                QFile::setPermissions(destFilePath, 
                    QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | 
                    QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup | 
                    QFile::ReadOther | QFile::WriteOther | QFile::ExeOther);

                successfulPlugins << packageName + " (UI plugin copied to plugins directory)";
            } else {
                failedPlugins << packageName + " (failed to copy UI plugin)";
            }

            continue;
        }

        // // Regular installation process for non-UI plugins
        // bool installSuccess = false;
        // QMetaObject::invokeMethod(
        //     packageManagerPlugin,
        //     "installPlugin",
        //     Qt::DirectConnection,
        //     Q_RETURN_ARG(bool, installSuccess),
        //     Q_ARG(QString, filePath)
        // );

        // Use new API to install plugin via package_manager wrapper
        bool installSuccess = false;
        if (m_logosAPI && m_logosAPI->getClient("package_manager")->isConnected()) {
            LogosModules logos(m_logosAPI);
            installSuccess = logos.package_manager.installPlugin(filePath);
        } else {
            qDebug() << "LogosAPI not connected, cannot install plugin:" << packageName;
        }

        if (!installSuccess) {
            failedPlugins << packageName + " (installation failed)";
        }

        // Then process the plugin to load it
        //QString pluginName;
        //bool success = QMetaObject::invokeMethod(
        //    coreManagerPlugin,
        //    "processPlugin",
        //    Qt::DirectConnection,
        //    Q_RETURN_ARG(QString, pluginName),
        //    Q_ARG(QString, filePath)
        //);

        //if (success) {
        //    successfulPlugins << packageName + " (" + pluginName + ")";
        //} else {
        //    failedPlugins << packageName + " (processing failed)";
        //}
    }

    // Display the results
    QString resultText = "<h3>Installation Results</h3>";

    if (!successfulPlugins.isEmpty()) {
        resultText += "<p><b>Successfully installed:</b></p><ul>";
        for (const QString& plugin : successfulPlugins) {
            resultText += "<li>" + plugin + "</li>";
        }
        resultText += "</ul>";
    }

    if (!failedPlugins.isEmpty()) {
        resultText += "<p><b>Failed to install:</b></p><ul>";
        for (const QString& plugin : failedPlugins) {
            resultText += "<li>" + plugin + "</li>";
        }
        resultText += "</ul>";
    }

    // If we have a reference to the main window, refresh the core modules view
    if (m_mainWindow && !successfulPlugins.isEmpty()) {
        m_mainWindow->refreshCoreModuleView();
        m_mainWindow->refreshModulesView();
        resultText += "<p><b>Module views refreshed.</b></p>";
    }

    m_detailsTextEdit->setHtml(resultText);

    // Refresh the package list to show updated status
    scanPackagesFolder();

    // call scanPackagesFolder again after 5 seconds
    QTimer::singleShot(5000, this, &PackageManagerView::scanPackagesFolder);
}

QList<QString> PackageManagerView::getSelectedPackages()
{
    QList<QString> selectedPackages;

    // Check all rows in the package table
    for (int row = 0; row < m_packageTable->rowCount(); ++row) {
        QTableWidgetItem* checkItem = m_packageTable->item(row, 0);
        if (checkItem && checkItem->checkState() == Qt::Checked) {
            QTableWidgetItem* nameItem = m_packageTable->item(row, 1);
            if (nameItem) {
                selectedPackages << nameItem->text();
            }
        }
    }

    return selectedPackages;
}

void PackageManagerView::updateInstallButtonState()
{
    // Check if any package is selected
    bool anySelected = false;

    for (int row = 0; row < m_packageTable->rowCount(); ++row) {
        QTableWidgetItem* checkItem = m_packageTable->item(row, 0);
        if (checkItem && checkItem->checkState() == Qt::Checked) {
            anySelected = true;
            break;
        }
    }

    // Enable the Install button only if at least one package is selected
    m_applyButton->setEnabled(anySelected);
}

// Helper method to find a package row by name
int PackageManagerView::findPackageRow(const QString& packageName)
{
    for (int row = 0; row < m_packageTable->rowCount(); ++row) {
        QTableWidgetItem* nameItem = m_packageTable->item(row, 1);
        if (nameItem && nameItem->text() == packageName) {
            return row;
        }
    }
    return -1;
}

// Helper method to select or deselect a package
void PackageManagerView::selectPackage(const QString& packageName, bool select)
{
    int row = findPackageRow(packageName);
    if (row != -1) {
        QTableWidgetItem* checkItem = m_packageTable->item(row, 0);
        if (checkItem) {
            checkItem->setCheckState(select ? Qt::Checked : Qt::Unchecked);
        }
    }
}

// Recursive method to select all dependencies of a package
void PackageManagerView::selectDependencies(const QString& packageName, QSet<QString>& processedPackages)
{
    // Prevent infinite recursion with circular dependencies
    if (processedPackages.contains(packageName)) {
        return;
    }
    
    // Mark this package as processed
    processedPackages.insert(packageName);
    
    // Check if this package exists and has dependencies
    if (!m_packages.contains(packageName)) {
        return;
    }
    
    const PackageInfo& info = m_packages[packageName];
    
    // Select this package
    selectPackage(packageName, true);
    
    // Recursively select all dependencies
    for (const QString& dependency : info.dependencies) {
        selectDependencies(dependency, processedPackages);
    }
}

// Handle checkbox state changes
void PackageManagerView::onPackageCheckStateChanged(QTableWidgetItem* item)
{
    // Only process if it's the checkbox column (0) and we're not already processing dependencies
    if (!item || item->column() != 0 || m_isProcessingDependencies) {
        return;
    }
    
    int row = item->row();
    QTableWidgetItem* nameItem = m_packageTable->item(row, 1);
    
    if (!nameItem) {
        return;
    }
    
    bool isChecked = (item->checkState() == Qt::Checked);
    QString packageName = nameItem->text();
    
    // If the package is being checked, select its dependencies
    if (isChecked) {
        m_isProcessingDependencies = true;
        
        QSet<QString> processedPackages;
        selectDependencies(packageName, processedPackages);
        
        m_isProcessingDependencies = false;
    }
    
    // Update the install button state
    updateInstallButtonState();
} 
