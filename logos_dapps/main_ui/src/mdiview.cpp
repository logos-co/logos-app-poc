#include "mdiview.h"
#include "mdichild.h"
#include <QTabBar>
#include <QApplication>
#include <QDebug>

MdiView::MdiView(QWidget *parent)
    : QWidget(parent), windowCounter(0)
{
    setupUi();
    
    // Add an initial MDI window
    addMdiWindow();
}

MdiView::~MdiView()
{
}

void MdiView::setupUi()
{
    // Create main layout
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Create toolbar
    toolBar = new QToolBar(this);
    
    // Add button
    addButton = new QPushButton(tr("Add Window"), this);
    connect(addButton, &QPushButton::clicked, this, &MdiView::addMdiWindow);
    toolBar->addWidget(addButton);
    
    // Toggle button
    toggleButton = new QPushButton(tr("Toggle View Mode"), this);
    connect(toggleButton, &QPushButton::clicked, this, &MdiView::toggleViewMode);
    toolBar->addWidget(toggleButton);
    
    // Add toolbar to layout
    mainLayout->addWidget(toolBar);
    
    // Create MDI area
    mdiArea = new QMdiArea(this);
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setViewMode(QMdiArea::SubWindowView); // Start in windowed mode
    
    // Use system background color for MDI area
    QPalette palette = QApplication::palette();
    mdiArea->setBackground(palette.color(QPalette::Window));
    
    // Enable tab close buttons
    mdiArea->setTabsClosable(true);
    
    // Connect the tabCloseRequested signal to handle tab closing
    connect(mdiArea, &QMdiArea::subWindowActivated, this, &MdiView::updateTabCloseButtons);
    
    // Add MDI area to layout
    mainLayout->addWidget(mdiArea);
    
    // Set the layout for this widget
    setLayout(mainLayout);
}

void MdiView::addMdiWindow()
{
    // Create a new MDI child window
    MdiChild *child = new MdiChild;
    windowCounter++;
    child->setWindowTitle(tr("MDI Window %1").arg(windowCounter));
    
    // Add the child to the MDI area
    QMdiSubWindow *subWindow = mdiArea->addSubWindow(child);
    subWindow->setMinimumSize(200, 200);
    subWindow->show();
    
    // Connect the close event of the subwindow
    connect(subWindow, &QMdiSubWindow::windowStateChanged, this, &MdiView::updateTabCloseButtons);
}

void MdiView::toggleViewMode()
{
    // Toggle between tabbed and windowed mode
    if (mdiArea->viewMode() == QMdiArea::SubWindowView) {
        mdiArea->setViewMode(QMdiArea::TabbedView);
        toggleButton->setText(tr("Switch to Windowed"));
        
        // Make sure tab close buttons are visible
        updateTabCloseButtons();
    } else {
        mdiArea->setViewMode(QMdiArea::SubWindowView);
        toggleButton->setText(tr("Switch to Tabbed"));
    }
}

void MdiView::updateTabCloseButtons()
{
    // This function ensures that tab close buttons are visible
    // It needs to be called when switching to tabbed mode and when windows change
    if (mdiArea->viewMode() == QMdiArea::TabbedView) {
        // Find the internal QTabBar
        QTabBar* tabBar = mdiArea->findChild<QTabBar*>();
        if (tabBar) {
            tabBar->setTabsClosable(true);
            
            // Disconnect any existing connections to avoid duplicates
            disconnect(tabBar, &QTabBar::tabCloseRequested, nullptr, nullptr);
            
            // Connect the tabCloseRequested signal
            connect(tabBar, &QTabBar::tabCloseRequested, [this](int index) {
                // Get the list of subwindows
                QList<QMdiSubWindow*> windows = mdiArea->subWindowList();
                
                // Make sure the index is valid
                if (index >= 0 && index < windows.size()) {
                    // Close the window at the specified index
                    windows.at(index)->close();
                }
            });
        }
    }
}

QMdiSubWindow* MdiView::addPluginWindow(QWidget* pluginWidget, const QString& title)
{
    if (!pluginWidget) {
        qDebug() << "Cannot add null plugin widget to MDI area";
        return nullptr;
    }
    
    // Create a new MDI sub-window
    QMdiSubWindow *subWindow = new QMdiSubWindow();
    subWindow->setWidget(pluginWidget);
    subWindow->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->setWindowTitle(title);
    
    // Add the sub-window to the MDI area
    mdiArea->addSubWindow(subWindow);
    
    // Show the sub-window
    subWindow->show();
    
    // Store the mapping between plugin widget and MDI window
    m_pluginWindows[pluginWidget] = subWindow;
    
    // Update tab close buttons if in tabbed mode
    updateTabCloseButtons();
    
    return subWindow;
}

void MdiView::removePluginWindow(QWidget* pluginWidget)
{
    if (!pluginWidget || !m_pluginWindows.contains(pluginWidget)) {
        return;
    }
    
    QMdiSubWindow* subWindow = m_pluginWindows[pluginWidget];
    if (subWindow) {
        // Detach the plugin widget from the sub-window to prevent it from being deleted
        subWindow->setWidget(nullptr);
        
        // Close and delete the sub-window
        subWindow->close();
        
        // Remove from the map
        m_pluginWindows.remove(pluginWidget);
    }
} 