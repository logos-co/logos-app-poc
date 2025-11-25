#include "applauncher.h"
#include "mainwindow.h"
#include "modulesview.h"
#include "modulesgenericview.h"
#include <QPluginLoader>
#include <QCoreApplication>
#include <QDebug>
#include <QJsonObject>
#include <QDir>

AppLauncher::AppLauncher(QWidget *parent, MainWindow *mainWindow)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
    , m_modulesView(nullptr)
    , m_layout(nullptr)
{
    setupUi();
    // Don't refresh here - will be called after ModulesView is created
}

AppLauncher::~AppLauncher()
{
}

void AppLauncher::setupUi()
{
    // Create main horizontal layout
    m_layout = new QHBoxLayout(this);
    m_layout->setSpacing(20);
    m_layout->setContentsMargins(20, 10, 20, 10);
    m_layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    
    // Set dark background with border to separate from MDI section
    setStyleSheet(
        "AppLauncher {"
        "   background-color: #2D2D2D;"
        "   border-top: 1px solid #3D3D3D;"
        "   border-bottom: none;"
        "   border-left: none;"
        "   border-right: none;"
        "}"
    );
    
    // Set fixed height for dock-like appearance
    setFixedHeight(90);
}

void AppLauncher::setModulesView(ModulesView* modulesView)
{
    m_modulesView = modulesView;
    refreshAppList();
}

void AppLauncher::refreshAppList()
{
    clearAppList();
    
    if (!m_modulesView) {
        qDebug() << "AppLauncher: ModulesView not available";
        return;
    }
    
    // Get available apps from ModulesView
    QStringList apps = m_modulesView->getAvailableApps();
    
    if (apps.isEmpty()) {
        qDebug() << "AppLauncher: No apps available";
        return;
    }
    
    qDebug() << "AppLauncher: Found" << apps.size() << "apps:" << apps;
    
    // Create app icons
    for (const QString& appName : apps) {
        // Create container widget for icon and dot
        QWidget* container = new QWidget(this);
        QVBoxLayout* containerLayout = new QVBoxLayout(container);
        containerLayout->setSpacing(4);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
        
        // Load app icon
        QPixmap iconPixmap = loadAppIcon(appName);
        if (iconPixmap.isNull()) {
            // Try getting icon from ModulesView
            iconPixmap = m_modulesView->getAppIcon(appName);
        }
        
        // Create icon button
        QPushButton* iconButton = new QPushButton(container);
        iconButton->setFixedSize(64, 64);
        iconButton->setIconSize(QSize(64, 64));
        
        if (!iconPixmap.isNull()) {
            QIcon icon(iconPixmap);
            iconButton->setIcon(icon);
        } else {
            // Fallback: use app name as text
            iconButton->setText(appName.left(4));
        }
        
        // Style the button with hand cursor
        iconButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #3D3D3D;"
            "   border: none;"
            "   border-radius: 12px;"
            "   padding: 8px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #4D4D4D;"
            "}"
            "QPushButton:pressed {"
            "   background-color: #5D5D5D;"
            "}"
        );
        iconButton->setCursor(Qt::PointingHandCursor);
        
        // Create dot indicator
        QLabel* dotIndicator = new QLabel(container);
        dotIndicator->setFixedSize(6, 6);
        dotIndicator->setStyleSheet(
            "QLabel {"
            "   background-color: #A0A0A0;"
            "   border-radius: 3px;"
            "}"
        );
        dotIndicator->hide(); // Hidden by default
        
        // Add to container layout
        containerLayout->addWidget(iconButton);
        containerLayout->addWidget(dotIndicator, 0, Qt::AlignHCenter);
        
        // Store references
        m_appButtons[appName] = iconButton;
        m_dotIndicators[appName] = dotIndicator;
        m_appContainers[appName] = container;
        
        // Connect button click
        connect(iconButton, &QPushButton::clicked, this, [this, appName]() {
            onAppClicked(appName);
        });
        
        // Add container to main layout
        m_layout->addWidget(container);
        
        // Update initial state
        bool isLoaded = m_modulesView->isAppLoaded(appName);
        updateAppState(appName, isLoaded);
    }
}

void AppLauncher::updateAppState(const QString& appName, bool isOpen)
{
    if (m_dotIndicators.contains(appName)) {
        if (isOpen) {
            m_dotIndicators[appName]->show();
        } else {
            m_dotIndicators[appName]->hide();
        }
    }
}

void AppLauncher::onAppClicked(const QString& appName)
{
    qDebug() << "AppLauncher: App clicked:" << appName;
    
    if (!m_modulesView) {
        qDebug() << "AppLauncher: ModulesView not available";
        return;
    }
    
    // Check if app is already loaded
    bool isLoaded = m_modulesView->isAppLoaded(appName);
    
    if (isLoaded) {
        // App is already open - bring to front
        qDebug() << "AppLauncher: App" << appName << "is already loaded, bringing to front";
        
        // Get the widget and activate it in MDI view
        QWidget* appWidget = m_modulesView->getAppWidget(appName);
        if (appWidget && m_mainWindow && m_mainWindow->getMdiView()) {
            // Check if widget still exists and is valid
            MdiView* mdiView = m_mainWindow->getMdiView();
            if (mdiView) {
                mdiView->activatePluginWindow(appWidget);
                
                // Switch to Apps tab (index 0) to show the activated window
                QVector<SidebarButton*> buttons = m_mainWindow->findChildren<SidebarButton*>();
                if (!buttons.isEmpty()) {
                    buttons[0]->click();
                }
            }
        } else {
            // Widget doesn't exist anymore - app was closed, update state
            qDebug() << "AppLauncher: Widget for" << appName << "no longer exists, updating state";
            updateAppState(appName, false);
        }
    } else {
        // Load the app
        qDebug() << "AppLauncher: Loading app:" << appName;
        m_modulesView->onLoadComponent(appName);
    }
}

void AppLauncher::clearAppList()
{
    // Remove all widgets from layout
    QLayoutItem* item;
    while ((item = m_layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->hide();
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    // Clear maps
    m_appButtons.clear();
    m_dotIndicators.clear();
    m_appContainers.clear();
}

QPixmap AppLauncher::loadAppIcon(const QString& appName)
{
    // Determine platform-specific library extension
    QString libExtension;
    #if defined(Q_OS_MAC)
        libExtension = ".dylib";
    #elif defined(Q_OS_WIN)
        libExtension = ".dll";
    #else // Linux/Unix
        libExtension = ".so";
    #endif
    
    QString pluginPath = QCoreApplication::applicationDirPath() + "/../plugins/" + appName + libExtension;
    QPluginLoader loader(pluginPath);
    
    if (loader.load()) {
        // Get embedded metadata from plugin
        QJsonObject metadata = loader.metaData();
        QJsonObject metaDataObj = metadata.value("MetaData").toObject();
        
        // Extract icon path from embedded metadata
        if (metaDataObj.contains("icon")) {
            QString iconPath = metaDataObj.value("icon").toString();
            if (!iconPath.isEmpty() && iconPath.startsWith(":/")) {
                // Icon is embedded in plugin resources
                QPixmap iconPixmap = QPixmap(iconPath);
                if (!iconPixmap.isNull()) {
                    loader.unload();
                    return iconPixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }
            }
        }
        
        loader.unload();
    }
    
    return QPixmap();
}
