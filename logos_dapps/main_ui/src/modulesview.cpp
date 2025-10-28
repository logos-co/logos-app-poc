#include "modulesview.h"
#include <QPluginLoader>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include <QLabel>
#include <QFont>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpacerItem>
#include "mainwindow.h"

ModulesView::ModulesView(QWidget *parent, MainWindow* mainWindow)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
    , m_layout(nullptr)
    , m_buttonLayout(nullptr)
{
    // Get the MDI view from the main window if available
    if (m_mainWindow) {
        m_mdiView = m_mainWindow->getMdiView();
    } else {
        m_mdiView = nullptr;
    }
    
    setupUi();
    setupPluginButtons(m_buttonLayout);
}

ModulesView::~ModulesView()
{
    // Unload all components
    QStringList components = m_loadedComponents.keys();
    for (const QString& name : components) {
        onUnloadComponent(name);
    }
}

void ModulesView::setupUi()
{
    // Create main layout
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(20);
    m_layout->setContentsMargins(40, 40, 40, 40);
    
    // Create and style the title
    QLabel* titleLabel = new QLabel("UI Modules", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #ffffff;");
    
    // Add a subtitle
    QLabel* subtitleLabel = new QLabel("Available UI plugins in the system", this);
    subtitleLabel->setStyleSheet("color: #a0a0a0; font-size: 14px; margin-bottom: 20px;");
    
    m_layout->addWidget(titleLabel);
    m_layout->addWidget(subtitleLabel);

    // Create button layout
    m_buttonLayout = new QVBoxLayout();
    m_layout->addLayout(m_buttonLayout);
}

void ModulesView::setupPluginButtons(QVBoxLayout* buttonLayout)
{
    qDebug() << "Setting up plugin buttons...";
    
    if (!buttonLayout) {
        qDebug() << "Button layout is null, cannot set up plugin buttons";
        return;
    }
    
    try {
        // Find and create buttons for available plugins
        QStringList plugins = findAvailablePlugins();
        
        if (plugins.isEmpty()) {
            QLabel* noPluginsLabel = new QLabel("No UI plugins found in the plugins directory.", this);
            buttonLayout->addWidget(noPluginsLabel);
            qDebug() << "No UI plugins found.";
            return;
        }
        
        qDebug() << "Found" << plugins.size() << "UI plugins:" << plugins;
        
        for (const QString& plugin : plugins) {
            QFrame* pluginFrame = new QFrame(this);
            pluginFrame->setObjectName("pluginFrame");
            pluginFrame->setStyleSheet(
                "QFrame#pluginFrame {"
                "   background-color: #2d2d2d;"
                "   border-radius: 8px;"
                "   padding: 12px;"
                "   border: 1px solid #3d3d3d;"
                "   margin-bottom: 8px;"
                "}"
            );
            QVBoxLayout* pluginLayout = new QVBoxLayout(pluginFrame);
            pluginLayout->setSpacing(12);
            
            // Create header layout
            QHBoxLayout* headerLayout = new QHBoxLayout();
            
            // Plugin name with bold font
            QLabel* pluginLabel = new QLabel(plugin, this);
            QFont pluginFont = pluginLabel->font();
            pluginFont.setBold(true);
            pluginLabel->setFont(pluginFont);
            pluginLabel->setStyleSheet("color: #ffffff; font-size: 16px;");
            pluginLabel->setMinimumWidth(100);
            
            // Load and unload buttons
            QPushButton* loadButton = nullptr;
            QPushButton* unloadButton = nullptr;
            
            if (plugin != "main_ui") {
                loadButton = new QPushButton("Load", this);
                loadButton->setStyleSheet(
                    "QPushButton {"
                    "   background-color: #4CAF50;"
                    "   color: white;"
                    "   border: none;"
                    "   padding: 8px 16px;"
                    "   border-radius: 4px;"
                    "}"
                    "QPushButton:hover {"
                    "   background-color: #45a049;"
                    "}"
                    "QPushButton:disabled {"
                    "   background-color: #cccccc;"
                    "}"
                );
                
                unloadButton = new QPushButton("Unload", this);
                unloadButton->setStyleSheet(
                    "QPushButton {"
                    "   background-color: #f44336;"
                    "   color: white;"
                    "   border: none;"
                    "   padding: 8px 16px;"
                    "   border-radius: 4px;"
                    "}"
                    "QPushButton:hover {"
                    "   background-color: #da190b;"
                    "}"
                    "QPushButton:disabled {"
                    "   background-color: #cccccc;"
                    "}"
                );
                
                // Store buttons for later use
                m_loadButtons[plugin] = loadButton;
                m_unloadButtons[plugin] = unloadButton;
                
                // Initially hide the unload button
                unloadButton->hide();
            }
            
            // Add widgets to header layout
            headerLayout->addWidget(pluginLabel);
            headerLayout->addStretch();

            if (plugin != "main_ui") {
                headerLayout->addWidget(loadButton);
                headerLayout->addWidget(unloadButton);
            }
            
            // Add header layout to plugin layout
            pluginLayout->addLayout(headerLayout);
            
            // Add plugin frame to main layout
            buttonLayout->addWidget(pluginFrame);
            
            // Connect buttons to slots using lambda functions to pass the plugin name
            if (plugin != "main_ui") {
                connect(loadButton, &QPushButton::clicked, this, [this, plugin]() {
                    onLoadComponent(plugin);
                });
                connect(unloadButton, &QPushButton::clicked, this, [this, plugin]() {
                    onUnloadComponent(plugin);
                });
                
                // Set initial button states
                updateButtonStates(plugin, true);
            }
        }
        
        qDebug() << "Plugin buttons setup complete.";
    } catch (const std::exception& e) {
        qDebug() << "Exception during plugin button setup:" << e.what();
    } catch (...) {
        qDebug() << "Unknown exception during plugin button setup";
    }
}

QStringList ModulesView::findAvailablePlugins()
{
    QStringList plugins;
    
    // Get the plugins directory path
    QString pluginsPath = QCoreApplication::applicationDirPath() + "/../plugins";
    QDir pluginsDir(pluginsPath);
    
    qDebug() << "Looking for plugins in:" << pluginsPath;
    
    if (!pluginsDir.exists()) {
        qWarning() << "Plugins directory does not exist:" << pluginsPath;
        return plugins;
    }
    
    // Get all files in the plugins directory
    QStringList entries = pluginsDir.entryList(QDir::Files);
    qDebug() << "All files in plugins directory:" << entries;
    
    // Determine platform-specific library extension
    QString libExtension;
    #if defined(Q_OS_MAC)
        libExtension = ".dylib";
        int extLength = 6; // Length of ".dylib"
    #elif defined(Q_OS_WIN)
        libExtension = ".dll";
        int extLength = 4; // Length of ".dll"
    #else // Linux/Unix
        libExtension = ".so";
        int extLength = 3; // Length of ".so"
    #endif
    
    qDebug() << "Using plugin extension:" << libExtension;
    
    // Add all library files (they're all valid plugins)
    for (const QString& entry : entries) {
        if (entry.endsWith(libExtension)) {
            // Skip system libraries (typically start with lib)
            if (entry.startsWith("lib")) {
                qDebug() << "Skipping system library:" << entry;
                continue;
            }
            
            // Get clean plugin name
            QString pluginName = entry;
            pluginName.chop(extLength); // Remove extension
            plugins.append(pluginName);
            qDebug() << "Found plugin:" << pluginName;
        }
    }
    
    qDebug() << "Total plugins found:" << plugins.size();
    return plugins;
}

QString ModulesView::getPluginPath(const QString& name)
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
    
    return QCoreApplication::applicationDirPath() + "/../plugins/" + name + libExtension;
}

void ModulesView::onLoadComponent(const QString& name)
{
    qDebug() << "Attempting to load plugin:" << name;
    
    if (m_loadedComponents.contains(name)) {
        qDebug() << "Plugin" << name << "is already loaded";
        return;
    }
    
    try {
        QString pluginPath = getPluginPath(name);
        qDebug() << "Loading plugin from:" << pluginPath;
        
        QPluginLoader loader(pluginPath);
        if (!loader.load()) {
            qDebug() << "Failed to load plugin:" << name << "-" << loader.errorString();
            return;
        }
        
        QObject* plugin = loader.instance();
        if (!plugin) {
            qDebug() << "Failed to get plugin instance:" << name << "-" << loader.errorString();
            return;
        }
        
        IComponent* component = qobject_cast<IComponent*>(plugin);
        if (!component) {
            qDebug() << "Failed to cast plugin to IComponent:" << name;
            loader.unload();
            return;
        }
        
        // Create the component widget
        QWidget* componentWidget = nullptr;
        try {
            LogosAPI* logosAPI = nullptr;
            if (m_mainWindow) {
                logosAPI = m_mainWindow->getLogosAPI();
            }
            componentWidget = component->createWidget(logosAPI);
            if (!componentWidget) {
                qDebug() << "Component returned null widget:" << name;
                loader.unload();
                return;
            }
        } catch (const std::exception& e) {
            qDebug() << "Exception creating widget for" << name << ":" << e.what();
            loader.unload();
            return;
        } catch (...) {
            qDebug() << "Unknown exception creating widget for" << name;
            loader.unload();
            return;
        }
        
        // Store the component and widget
        m_loadedComponents[name] = component;
        m_componentWidgets[name] = componentWidget;
        
        // Add the component widget to the MDI view if available
        if (m_mdiView) {
            qDebug() << "Adding plugin" << name << "to MDI view";
            m_mdiView->addPluginWindow(componentWidget, name);
            
            // Switch to the Apps tab (index 0)
            if (m_mainWindow) {
                QVector<SidebarButton*> buttons = m_mainWindow->findChildren<SidebarButton*>();
                if (!buttons.isEmpty()) {
                    buttons[0]->click();
                }
            }
        }
        
        // Update button states
        updateButtonStates(name, false);
        
        qDebug() << "Successfully loaded plugin:" << name;
    } catch (const std::exception& e) {
        qDebug() << "Exception during load of" << name << ":" << e.what();
    } catch (...) {
        qDebug() << "Unknown exception during load of" << name;
    }
}

void ModulesView::onUnloadComponent(const QString& name)
{
    qDebug() << "Attempting to unload plugin:" << name;
    
    if (!m_loadedComponents.contains(name)) {
        qDebug() << "Plugin" << name << "is not loaded, nothing to unload";
        return;
    }
    
    try {
        // Get the component and widget
        IComponent* component = m_loadedComponents.value(name);
        QWidget* componentWidget = m_componentWidgets.value(name);
        
        if (!component) {
            qDebug() << "Component for" << name << "is null, removing from loaded components";
            m_loadedComponents.remove(name);
            
            if (componentWidget) {
                qDebug() << "Removing component widget for" << name;
                
                // Remove from MDI view if available
                if (m_mdiView) {
                    m_mdiView->removePluginWindow(componentWidget);
                }
                
                componentWidget->hide();
                componentWidget->deleteLater();
                m_componentWidgets.remove(name);
            }
            
            // Update button states
            updateButtonStates(name);
            return;
        }
        
        if (!componentWidget) {
            qDebug() << "Component widget for" << name << "is null, only removing component";
            m_loadedComponents.remove(name);
            
            // Update button states
            updateButtonStates(name);
            return;
        }
        
        // Remove from MDI view if available
        if (m_mdiView) {
            qDebug() << "Removing plugin" << name << "from MDI view";
            m_mdiView->removePluginWindow(componentWidget);
        }
        
        // Destroy the component widget
        qDebug() << "Destroying component widget for" << name;
        try {
            // Disconnect any signals/slots
            componentWidget->disconnect();
            
            // Call the component's destroyWidget method
            component->destroyWidget(componentWidget);
        } catch (const std::exception& e) {
            qDebug() << "Exception while destroying widget for" << name << ":" << e.what();
        } catch (...) {
            qDebug() << "Unknown exception while destroying widget for" << name;
        }
        
        // Remove from maps
        qDebug() << "Removing" << name << "from loaded components and widgets maps";
        m_loadedComponents.remove(name);
        m_componentWidgets.remove(name);
        
        // Update button states
        updateButtonStates(name);
        
        qDebug() << "Successfully unloaded plugin:" << name;
    } catch (const std::exception& e) {
        qDebug() << "Exception during unload of" << name << ":" << e.what();
    } catch (...) {
        qDebug() << "Unknown exception during unload of" << name;
    }
}

void ModulesView::updateButtonStates(const QString& name, bool isEnabled)
{
    // Update button states
    if (m_loadButtons.contains(name)) {
        if (isEnabled) {
            m_loadButtons[name]->show();
        } else {
            m_loadButtons[name]->hide();
        }
    }
    if (m_unloadButtons.contains(name)) {
        if (isEnabled) {
            m_unloadButtons[name]->hide();
        } else {
            m_unloadButtons[name]->show();
        }
    }
}

void ModulesView::refreshPluginList()
{
    qDebug() << "Refreshing UI modules plugin list";
    
    // Clear the current list
    clearPluginList();
    
    // Re-setup the plugin buttons
    setupPluginButtons(m_buttonLayout);
}

void ModulesView::clearPluginList()
{
    qDebug() << "Clearing UI modules plugin list";
    
    // Clear the button layout
    if (m_buttonLayout) {
        // Remove all widgets from the button layout
        QLayoutItem* item;
        while ((item = m_buttonLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                item->widget()->hide();
                item->widget()->deleteLater();
            }
            delete item;
        }
    }
    
    // Clear button maps
    m_loadButtons.clear();
    m_unloadButtons.clear();
} 