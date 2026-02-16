#include "mdiview.h"
#include "mdichild.h"
#include <QApplication>
#include <QDebug>
#include <QColor>
#include <QTimer>
#include <QEvent>
#include <QWheelEvent>
#include <QScroller>
#include <QScrollerProperties>
#include <QEasingCurve>

MdiView::MdiView(QWidget *parent)
    : QWidget(parent)
    , windowCounter(0)
    , m_mdiAddBtn(nullptr)
    , m_emptyPlaceholder(nullptr)
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
    
    mdiArea = new QMdiArea(this);
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setViewMode(QMdiArea::SubWindowView);
    
    // TODO: this should probably be a qml file and should then use Logos.Theme instead
    mdiArea->setBackground(QColor("#171717"));
    
    mdiArea->setTabsClosable(true);
    
    connect(mdiArea, &QMdiArea::subWindowActivated, this, &MdiView::updateTabCloseButtons);
    
    mainLayout->addWidget(mdiArea);
    
    setLayout(mainLayout);
    mdiArea->setViewMode(QMdiArea::TabbedView);

    m_emptyPlaceholder = new MdiChild(mdiArea);
    m_emptyPlaceholder->setVisible(false);
    m_emptyPlaceholder->setGeometry(mdiArea->rect());
    mdiArea->installEventFilter(this);

    // Ensure tab bar styling applies after the tab bar is created
    QTimer::singleShot(0, this, &MdiView::updateTabCloseButtons);
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
        if (m_mdiAddBtn) {
            m_mdiAddBtn->setVisible(false);
        }
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
                    QTimer::singleShot(0, this, &MdiView::updateTabCloseButtons);
                }
            });

            customizeTabBarStyle(tabBar);
            ensureMdiAddButton(tabBar);

            QTimer::singleShot(0, this, [this, tabBar]() {
                insetTabBarGeometry(tabBar, 24);
                repositionMdiAddButton();
            });
        }
    }

    updateEmptyPlaceholder();
}

void MdiView::insetTabBarGeometry(QTabBar *tabBar, int insetPx)
{
    if (!tabBar) return;
    QWidget *p = tabBar->parentWidget();
    if (!p) return;

    QRect g = tabBar->geometry();
    const bool hasWindows = !mdiArea->subWindowList().isEmpty();
    if (!hasWindows) {
        // Reserve space on the left for the add button.
        const int addButtonReservedSpace = 42 + 10;
        const int leftInset = insetPx + addButtonReservedSpace;
        const int rightInset = 24;
        tabBar->setGeometry(leftInset, g.y(), p->width() - leftInset - rightInset, g.height());
        return;
    }

    // With windows: reserve right side for 10px gap + add button (42px) + right margin (24px).
    const int addButtonWidth = 42;
    const int gapBeforeAddButton = 10;
    const int rightMargin = 24;
    const int rightInset = gapBeforeAddButton + addButtonWidth + rightMargin;
    tabBar->setGeometry(insetPx, g.y(), p->width() - insetPx - rightInset, g.height());
}

void MdiView::customizeTabBarStyle(QTabBar* tabBar)
{
    if (!tabBar) return;

    tabBar->setDocumentMode(true);
    tabBar->setDrawBase(false);
    tabBar->setAutoFillBackground(false);
    tabBar->setElideMode(Qt::ElideRight);
    tabBar->setUsesScrollButtons(false);
    tabBar->setExpanding(false);
    tabBar->setFixedHeight(44);
    tabBar->setIconSize(QSize(20, 20));
    QScroller::grabGesture(tabBar, QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(tabBar, QScroller::TouchGesture);
    QScrollerProperties props = QScroller::scroller(tabBar)->scrollerProperties();
    props.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    props.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    props.setScrollMetric(QScrollerProperties::ScrollingCurve, QEasingCurve::OutCubic);
    QScroller::scroller(tabBar)->setScrollerProperties(props);

    tabBar->setStyleSheet(QStringLiteral(R"(
        QTabBar {
            background: #171717;
            border: none;
            qproperty-drawBase: false;
        }
    
        QTabBar::tab {
            background: #262626;
            color: #A4A4A4;
    
            padding: 0px 24px 0px 32px;
            margin-right: 10px;
            margin-left: 0px;
    
            margin-top: 9px;
            margin-bottom: 4px;
    
            border-top-left-radius: 10px;
            border-top-right-radius: 10px;
            height: 35px;
            min-width: 120px;
        }
    
        QTabBar::tab:!selected {
            background: rgba(38, 38, 38, 0.6);
            color: #626262;
        }

        QTabBar::tab:hover { 
            background: #262626; 
        }
    
        QTabBar::close-button {
            image: url(:/icons/close.png);
            width: 14px;
            height: 14px;
            margin-left: 16px;
            margin-top: 10px;
        }
        QTabBar::close-button:hover {
            background: #262626; 
            border-radius: 7px;
        }

    )"));
}

void MdiView::ensureMdiAddButton(QTabBar* tabBar)
{
    if (!tabBar) {
        return;
    }

    // Create add button - parent it to the tab bar's parent so it's not clipped
    if (!m_mdiAddBtn) {
        m_mdiAddBtn = new QToolButton(tabBar->parentWidget());
        m_mdiAddBtn->setIcon(QIcon(":/icons/add-button.png"));
        m_mdiAddBtn->setIconSize(QSize(24, 24));
        m_mdiAddBtn->setAutoRaise(true);
        m_mdiAddBtn->setCursor(Qt::PointingHandCursor);
        m_mdiAddBtn->setFixedSize(42, 35);
        m_mdiAddBtn->setStyleSheet(QStringLiteral(R"(
            QToolButton {
                background: #2A2A2A;
                color: #FFFFFF;
                border-top-left-radius: 14px;
                border-top-right-radius: 14px;
                padding-top: 9px;
                padding-bottom: 2px;
                padding-left: 0px;
                padding-right: 0px;
            }
            QToolButton:hover {
                background: #262626;
            }
            QToolButton:pressed {
                background: #262626;
            }
        )"));
        connect(m_mdiAddBtn, &QToolButton::clicked, this, &MdiView::addMdiWindow);
        tabBar->installEventFilter(this);
    }

    m_mdiAddBtn->setVisible(true);
    repositionMdiAddButton();
}


void MdiView::repositionMdiAddButton()
{
    QTabBar* tabBar = mdiArea->findChild<QTabBar*>();
    if (!tabBar || !m_mdiAddBtn)
        return;

    QWidget* parent = tabBar->parentWidget();
    if (!parent) return;

    const int leftMargin = 24;
    const int rightMargin = 24;
    int x = leftMargin;

    const bool hasWindows = !mdiArea->subWindowList().isEmpty();
    if (hasWindows) {
        const int stickyX = parent->width() - m_mdiAddBtn->width() - rightMargin;
        x = stickyX;
        // Tab style already has margin-right: 10px, so position add button at last tab's right edge for 10px gap.
        if (tabBar->count() > 0) {
            const QRect lastRect = tabBar->tabRect(tabBar->count() - 1);
            if (lastRect.isValid()) {
                const int desiredX = tabBar->x() + lastRect.right();
                if (desiredX + m_mdiAddBtn->width() <= stickyX)
                    x = desiredX;
            }
        }
    }

    const int y = tabBar->geometry().bottom() - m_mdiAddBtn->height();
    m_mdiAddBtn->move(x, y);
    m_mdiAddBtn->raise();
}

void MdiView::updateEmptyPlaceholder()
{
    if (!m_emptyPlaceholder || !mdiArea)
        return;

    const bool empty = mdiArea->subWindowList().isEmpty();
    m_emptyPlaceholder->setVisible(empty);
    if (empty) {
        int top = 0;
        if (QTabBar* bar = mdiArea->findChild<QTabBar*>())
            top = bar->mapTo(mdiArea, QPoint(0, bar->height())).y();
        m_emptyPlaceholder->setGeometry(0, top, mdiArea->width(), mdiArea->height() - top);
        m_emptyPlaceholder->raise();
    }
}

bool MdiView::eventFilter(QObject* watched, QEvent* event)
{
    if (m_emptyPlaceholder && mdiArea && watched == mdiArea) {
        if (event->type() == QEvent::Resize || event->type() == QEvent::Show) {
            updateEmptyPlaceholder();
        }
    }

    QTabBar* tabBar = mdiArea->findChild<QTabBar*>();
    if (tabBar && watched == tabBar) {
        if (event->type() == QEvent::Resize || event->type() == QEvent::Show) {
            repositionMdiAddButton();
        } else if (event->type() == QEvent::Wheel && tabBar->count() > 1) {
            auto *wheelEvent = static_cast<QWheelEvent*>(event);
            int delta = 0;
            if (!wheelEvent->pixelDelta().isNull())
                delta = wheelEvent->pixelDelta().x();
            else if (!wheelEvent->angleDelta().isNull())
                delta = wheelEvent->angleDelta().x() / 2;
            if (delta != 0) {
                const int next = qBound(0, tabBar->currentIndex() + (delta > 0 ? -1 : 1), tabBar->count() - 1);
                if (next != tabBar->currentIndex()) {
                    tabBar->setCurrentIndex(next);
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(watched, event);
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
    if (subWindow->windowIcon().isNull()) {
        QIcon icon = pluginWidget ? pluginWidget->windowIcon() : QIcon();
        if (icon.isNull()) {
            icon = QApplication::windowIcon();
        }
        if (!icon.isNull()) {
            subWindow->setWindowIcon(icon);
        }
    }
    
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
        if (!subWindow->windowTitle().isEmpty()) {
            emit pluginWindowClosed(subWindow->windowTitle());
        }
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
