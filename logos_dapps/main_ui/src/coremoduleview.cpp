#include "coremoduleview.h"
#include "logos_api.h"
#include "logos_api_client.h"
#include <QFont>
#include <memory>
#include <QStringList>
#include <QCoreApplication>
#include <QDebug>
#include <QPushButton>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QFileDialog>
#include <QMessageBox>
#include "logos_api_client.h"
#include "pluginmethodsview.h"
#include <QJsonArray>
#include <QJsonObject>
#include "logos_sdk.h"

CoreModuleView::CoreModuleView(QWidget *parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_titleLabel(nullptr)
    , m_subtitleLabel(nullptr)
    , m_pluginList(nullptr)
    , m_updateTimer(nullptr)
    , m_stackedWidget(nullptr)
    , m_pluginsListWidget(nullptr)
    , m_currentMethodsView(nullptr)
{
    setupUi();

    // Initial update of plugin list
    updatePluginList();
}

CoreModuleView::~CoreModuleView()
{
    // Qt automatically cleans up child widgets
}

void CoreModuleView::setupUi()
{
    // Set the background color for the entire view
    setStyleSheet("QWidget#coreModuleView { background-color: #1e1e1e; }");
    setObjectName("coreModuleView");

    // Create main layout
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);

    // Create stacked widget
    m_stackedWidget = new QStackedWidget(this);
    m_layout->addWidget(m_stackedWidget);

    // Create plugins list widget
    m_pluginsListWidget = new QWidget(this);
    QVBoxLayout* pluginsLayout = new QVBoxLayout(m_pluginsListWidget);
    pluginsLayout->setSpacing(20);
    pluginsLayout->setContentsMargins(40, 40, 40, 40);

    // Create and style the title
    m_titleLabel = new QLabel("Core Modules", m_pluginsListWidget);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setStyleSheet("color: #ffffff;");

    // Add a subtitle
    m_subtitleLabel = new QLabel("All available plugins in the system", m_pluginsListWidget);
    m_subtitleLabel->setStyleSheet("color: #a0a0a0; font-size: 14px; margin-bottom: 20px;");

    pluginsLayout->addWidget(m_titleLabel);
    pluginsLayout->addWidget(m_subtitleLabel);
   
    // Add the "Add Plugin" button
    QPushButton* addPluginButton = new QPushButton("Install Plugin", m_pluginsListWidget);
    addPluginButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: #ffffff;"
        "   border-radius: 4px;"
        "   padding: 8px 16px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   margin-bottom: 20px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3d8b40;"
        "}"
    );
    addPluginButton->setMinimumHeight(40);
    connect(addPluginButton, &QPushButton::clicked, this, &CoreModuleView::onAddPluginClicked);
    pluginsLayout->addWidget(addPluginButton);

    // Create the plugin list
    createPluginList();

    // Add plugins list widget to stacked widget
    m_stackedWidget->addWidget(m_pluginsListWidget);
}

void CoreModuleView::createPluginList()
{
    // Get the plugins layout
    QVBoxLayout* pluginsLayout = qobject_cast<QVBoxLayout*>(m_pluginsListWidget->layout());
    if (!pluginsLayout) {
        return;
    }

    // Create a container widget for the list with padding
    QWidget* listContainer = new QWidget(m_pluginsListWidget);
    listContainer->setObjectName("listContainer");
    listContainer->setStyleSheet(
        "QWidget#listContainer {"
        "   background-color: #2d2d2d;"
        "   border-radius: 8px;"
        "   padding: 20px;"
        "   border: 1px solid #3d3d3d;"
        "}"
        "QPushButton {"
        "   background-color: #4b4b4b;"
        "   color: #ffffff;"
        "   border-radius: 4px;"
        "   padding: 6px 12px;"
        "   font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #5b5b5b;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3b3b3b;"
        "}"
    );
    QVBoxLayout* containerLayout = new QVBoxLayout(listContainer);
    containerLayout->setContentsMargins(20, 20, 20, 20);

    // Create the list widget
    m_pluginList = new QListWidget(listContainer);
    m_pluginList->setMinimumHeight(300);
    m_pluginList->setFrameShape(QFrame::NoFrame); // Remove the default border
    m_pluginList->setStyleSheet(
        "QListWidget {"
        "   background-color: transparent;"
        "   border: none;"
        "   margin: 16px 0px;"
        "}"
        "QListWidget::item {"
        "   color: #e0e0e0;"
        "   margin: 0px 0px;"
        "   padding: 0px 0px;"
        "   border-radius: 6px;"
        "   background-color: #363636;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: #404040;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: #404040;"
        "   color: #ffffff;"
        "}"
    );

    containerLayout->addWidget(m_pluginList);
    pluginsLayout->addWidget(listContainer);
}

void CoreModuleView::updatePluginList()
{
    qDebug() << "\n\n----------> Updating plugin list\n\n";
    
    LogosAPI api("core");
    LogosAPIClient* coreManagerClient = api.getClient("core_manager");
    if (!coreManagerClient || !coreManagerClient->isConnected()) {
        qWarning() << "Core manager client is not available";
        return;
    }

    LogosModules logos(&api);
    QJsonArray pluginsArray = logos.core_manager.getKnownPlugins();

    qDebug() << "================================";
    qDebug() << "pluginsArray:" << pluginsArray;

    // Clear the current list
    m_pluginList->clear();

    if (pluginsArray.isEmpty()) {
        qDebug() << "No plugins known";
        QListWidgetItem* item = new QListWidgetItem("No plugins available");
        item->setIcon(QIcon(":/icons/plugin.png"));
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_pluginList->addItem(item);
        return;
    } else {
        qDebug() << "Known plugins:";
        for (const QJsonValue &val : pluginsArray) {
            QJsonObject pluginObj = val.toObject();
            QString pluginName = pluginObj["name"].toString();
            bool isLoaded = pluginObj["loaded"].toBool();
            qDebug() << "  " << pluginName << (isLoaded ? " (loaded)" : " (not loaded)");
        }
    }

    // Add each plugin to the list
    for (const QJsonValue &val : pluginsArray) {
        QJsonObject pluginObj = val.toObject();
        QString pluginName = pluginObj["name"].toString();
        bool isLoaded = pluginObj["loaded"].toBool();

        // Create a widget to hold both the plugin name and the button
        QWidget* itemWidget = new QWidget();
        QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);
        itemLayout->setContentsMargins(10, 10, 10, 10);

        // Add the plugin name
        QLabel* nameLabel = new QLabel(pluginName);
        nameLabel->setStyleSheet("color: #e0e0e0; font-size: 16px;");
        itemLayout->addWidget(nameLabel);

        // Add status indicator
        QLabel* statusLabel = new QLabel(isLoaded ? "(Loaded)" : "(Not Loaded)");
        statusLabel->setStyleSheet(isLoaded ?
                                  "color: #4CAF50; font-size: 14px;" :
                                  "color: #F44336; font-size: 14px;");
        itemLayout->addWidget(statusLabel);

        // Add spacer to push the button to the right
        itemLayout->addStretch();

        if (isLoaded) {
            // Plugin is loaded, show Unload and View Methods buttons
            QPushButton* unloadButton = new QPushButton("Unload Plugin");
            unloadButton->setProperty("pluginName", pluginName);
            unloadButton->setMinimumHeight(30); // Set minimum height for button
            unloadButton->setStyleSheet("background-color: #F44336;"); // Red button for unload
            connect(unloadButton, &QPushButton::clicked, this, &CoreModuleView::onUnloadPluginClicked);
            itemLayout->addWidget(unloadButton);

            QPushButton* viewMethodsButton = new QPushButton("View Methods");
            viewMethodsButton->setProperty("pluginName", pluginName);
            viewMethodsButton->setMinimumHeight(30); // Set minimum height for button
            connect(viewMethodsButton, &QPushButton::clicked, this, &CoreModuleView::onViewMethodsClicked);
            itemLayout->addWidget(viewMethodsButton);
        } else {
            // Plugin is not loaded, show Load button
            QPushButton* loadButton = new QPushButton("Load Plugin");
            loadButton->setProperty("pluginName", pluginName);
            loadButton->setMinimumHeight(30); // Set minimum height for button
            connect(loadButton, &QPushButton::clicked, this, &CoreModuleView::onLoadPluginClicked);
            itemLayout->addWidget(loadButton);
        }

        // Add the widget to the list
        QListWidgetItem* item = new QListWidgetItem();
        itemWidget->setMinimumHeight(50); // Set minimum height for the item
        item->setSizeHint(QSize(itemWidget->sizeHint().width(), 50)); // Force height to be 50 pixels
        m_pluginList->addItem(item);
        m_pluginList->setItemWidget(item, itemWidget);
    }
}

void CoreModuleView::onLoadPluginClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) {
        return;
    }

    QString pluginName = button->property("pluginName").toString();
    if (pluginName.isEmpty()) {
        qDebug() << "Plugin name not found in button properties";
        return;
    }

    qDebug() << "Loading plugin:" << pluginName;

    LogosAPI api("core");
    LogosAPIClient* coreManagerClient = api.getClient("core_manager");
    if (!coreManagerClient || !coreManagerClient->isConnected()) {
        qWarning() << "Core manager client is not available";
        return;
    }

    LogosModules logos(&api);
    bool success = logos.core_manager.loadPlugin(pluginName);
    if (success) {
        qDebug() << "Successfully loaded plugin:" << pluginName;
        // Update the UI to reflect the loaded plugin
        updatePluginList();
    } else {
        qDebug() << "Failed to load plugin:" << pluginName;
    }
}

void CoreModuleView::onUnloadPluginClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) {
        return;
    }

    QString pluginName = button->property("pluginName").toString();
    if (pluginName.isEmpty()) {
        qDebug() << "Plugin name not found in button properties";
        return;
    }

    qDebug() << "Unloading plugin:" << pluginName;

    LogosAPI api("core");
    LogosAPIClient* coreManagerClient = api.getClient("core_manager");
    if (!coreManagerClient || !coreManagerClient->isConnected()) {
        qWarning() << "Core manager client is not available";
        return;
    }

    LogosModules logos(&api);
    bool success = logos.core_manager.unloadPlugin(pluginName);
    if (success) {
        qDebug() << "Successfully unloaded plugin:" << pluginName;
        // Update the UI to reflect the unloaded plugin
        updatePluginList();
    } else {
        qDebug() << "Failed to unload plugin:" << pluginName;
    }
}

void CoreModuleView::onViewMethodsClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) {
        return;
    }

    QString pluginName = button->property("pluginName").toString();
    if (pluginName.isEmpty()) {
        qDebug() << "Plugin name not found in button properties";
        return;
    }

    // Create the plugin methods view
    m_currentMethodsView = new PluginMethodsView(pluginName, this);
    connect(m_currentMethodsView, &PluginMethodsView::backClicked, this, &CoreModuleView::onBackToPluginList);

    // Add it to the stacked widget
    m_stackedWidget->addWidget(m_currentMethodsView);

    // Show the methods view
    m_stackedWidget->setCurrentWidget(m_currentMethodsView);
}

void CoreModuleView::onBackToPluginList()
{
    // Switch back to the plugins list view
    m_stackedWidget->setCurrentWidget(m_pluginsListWidget);

    // If there is a current methods view, remove and delete it
    if (m_currentMethodsView) {
        m_stackedWidget->removeWidget(m_currentMethodsView);
        delete m_currentMethodsView;
        m_currentMethodsView = nullptr;
    }
}

void CoreModuleView::onAddPluginClicked()
{
    qDebug() << "Install Plugin button clicked";

    // Open file dialog to select a plugin file
    QString pluginFilter;

#ifdef Q_OS_WIN
    pluginFilter = "Plugins (*.dll)";
#elif defined(Q_OS_MAC)
    pluginFilter = "Plugins (*.dylib)";
#else
    pluginFilter = "Plugins (*.so)";
#endif

    // Use the application's current directory instead of home directory
    QString startDir = QCoreApplication::applicationDirPath();

    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select Plugin File",
        startDir,
        pluginFilter
    );

    if (filePath.isEmpty()) {
        qDebug() << "No file selected";
        return;
    }

    qDebug() << "Selected plugin file:" << filePath;

    // Use new API via LogosModules wrapper
    LogosAPI api("core");
    LogosModules logos(&api);
    bool success = logos.package_manager.installPlugin(filePath);
    if (!success) {
        QMessageBox::warning(this, "Warning", "Failed to install plugin file.");
        return;
    }

    QMessageBox::information(this, "Success", "Plugin installed successfully!");

    // Refresh the plugin list
    updatePluginList();
}
