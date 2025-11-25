#include "mdiview.h"
#include "mdichild.h"
#include <QTabBar>
#include <QApplication>
#include <QDebug>

MdiView::MdiView(QWidget *parent)
    : QWidget(parent), windowCounter(0)
{
    setupUi();
    addMdiWindow();
}

MdiView::~MdiView()
{
}

void MdiView::setupUi()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    toolBar = new QToolBar(this);
    
    addButton = new QPushButton(tr("Add Window"), this);
    connect(addButton, &QPushButton::clicked, this, &MdiView::addMdiWindow);
    toolBar->addWidget(addButton);
    
    toggleButton = new QPushButton(tr("Toggle View Mode"), this);
    connect(toggleButton, &QPushButton::clicked, this, &MdiView::toggleViewMode);
    toolBar->addWidget(toggleButton);
    
    mainLayout->addWidget(toolBar);
    
    mdiArea = new QMdiArea(this);
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setViewMode(QMdiArea::SubWindowView);
    
    QPalette palette = QApplication::palette();
    mdiArea->setBackground(palette.color(QPalette::Window));
    
    mdiArea->setTabsClosable(true);
    
    connect(mdiArea, &QMdiArea::subWindowActivated, this, &MdiView::updateTabCloseButtons);
    
    mainLayout->addWidget(mdiArea);
    
    setLayout(mainLayout);
}

void MdiView::addMdiWindow()
{
    MdiChild *child = new MdiChild;
    windowCounter++;
    child->setWindowTitle(tr("MDI Window %1").arg(windowCounter));
    
    QMdiSubWindow *subWindow = mdiArea->addSubWindow(child);
    subWindow->setMinimumSize(200, 200);
    subWindow->show();
    
    connect(subWindow, &QMdiSubWindow::windowStateChanged, this, &MdiView::updateTabCloseButtons);
}

void MdiView::toggleViewMode()
{
    if (mdiArea->viewMode() == QMdiArea::SubWindowView) {
        mdiArea->setViewMode(QMdiArea::TabbedView);
        toggleButton->setText(tr("Switch to Windowed"));
        
        updateTabCloseButtons();
    } else {
        mdiArea->setViewMode(QMdiArea::SubWindowView);
        toggleButton->setText(tr("Switch to Tabbed"));
    }
}

void MdiView::updateTabCloseButtons()
{
    if (mdiArea->viewMode() == QMdiArea::TabbedView) {
        QTabBar* tabBar = mdiArea->findChild<QTabBar*>();
        if (tabBar) {
            tabBar->setTabsClosable(true);
            
            disconnect(tabBar, &QTabBar::tabCloseRequested, nullptr, nullptr);
            
            connect(tabBar, &QTabBar::tabCloseRequested, [this](int index) {
                QList<QMdiSubWindow*> windows = mdiArea->subWindowList();
                
                if (index >= 0 && index < windows.size()) {
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
    
    QMdiSubWindow *subWindow = new QMdiSubWindow();
    subWindow->setWidget(pluginWidget);
    subWindow->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->setWindowTitle(title);
    
    subWindow->setMinimumSize(200, 200);
    
    QSize widgetSize = pluginWidget->sizeHint();
    if (widgetSize.isValid() && widgetSize.width() > 0 && widgetSize.height() > 0) {
        subWindow->resize(widgetSize);
    } else {
        subWindow->resize(800, 600);
    }
    
    mdiArea->addSubWindow(subWindow);
    
    subWindow->show();
    
    m_pluginWindows[pluginWidget] = subWindow;
    m_subWindowToWidget[subWindow] = pluginWidget;
    
    connect(subWindow, &QMdiSubWindow::destroyed, this, [this, pluginWidget, subWindow]() {
        if (pluginWidget && m_pluginWindows.contains(pluginWidget)) {
            m_pluginWindows.remove(pluginWidget);
        }
        if (subWindow && m_subWindowToWidget.contains(subWindow)) {
            m_subWindowToWidget.remove(subWindow);
        }
    });
    
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
        subWindow->setWidget(nullptr);
        
        if (m_subWindowToWidget.contains(subWindow)) {
            m_subWindowToWidget.remove(subWindow);
        }
        
        subWindow->close();
        
        m_pluginWindows.remove(pluginWidget);
    }
}

QWidget* MdiView::getWidgetForSubWindow(QMdiSubWindow* subWindow)
{
    if (subWindow && m_subWindowToWidget.contains(subWindow)) {
        return m_subWindowToWidget.value(subWindow);
    }
    return nullptr;
}

void MdiView::activatePluginWindow(QWidget* pluginWidget)
{
    if (!pluginWidget) {
        qDebug() << "MdiView::activatePluginWindow: pluginWidget is null";
        return;
    }
    
    if (!m_pluginWindows.contains(pluginWidget)) {
        qDebug() << "MdiView::activatePluginWindow: pluginWidget not found in map";
        return;
    }
    
    QMdiSubWindow* subWindow = m_pluginWindows[pluginWidget];
    if (subWindow) {
        if (subWindow->widget() == pluginWidget) {
            subWindow->raise();
            subWindow->activateWindow();
            mdiArea->setActiveSubWindow(subWindow);
        } else {
            qDebug() << "MdiView::activatePluginWindow: subwindow widget mismatch";
        }
    } else {
        qDebug() << "MdiView::activatePluginWindow: subWindow is null";
    }
} 
