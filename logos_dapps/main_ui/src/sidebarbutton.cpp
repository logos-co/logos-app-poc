#include "sidebarbutton.h"
#include <QPainter>
#include <QPaintEvent>
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QFont>

SidebarButton::SidebarButton(const QString &text, const QIcon &icon, QWidget *parent)
    : QPushButton(parent), m_active(false)
{
    setText(text);
    setIcon(icon);
    setIconSize(QSize(28, 28));
    setCheckable(true);
    setFlat(true);
    setCursor(Qt::PointingHandCursor);
    
    // Set fixed size for a more compact button
    setFixedWidth(80);
    setFixedHeight(70);
    
    // Style the button with modern colors
    QString styleSheet = QString(
        "SidebarButton {"
        "    border: none;"
        "    color: #CCCCCC;"
        "    font-size: 11px;"
        "    padding-top: 8px;"
        "    padding-bottom: 8px;"
        "    font-weight: normal;"
        "}"
        "SidebarButton:hover {"
        "    background-color: #3D3D3D;"
        "}"
        "SidebarButton:checked {"
        "    color: #FFFFFF;"
        "    font-weight: bold;"
        "}"
    );
    
    setStyleSheet(styleSheet);
}

void SidebarButton::setActive(bool active)
{
    m_active = active;
    setChecked(active);
    update();  // Trigger a repaint
}

bool SidebarButton::isActive() const
{
    return m_active;
}

void SidebarButton::paintEvent(QPaintEvent *event)
{
    // Draw the button background
    QStyleOption opt;
    opt.initFrom(this);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
    
    // Draw the icon centered at the top
    QRect iconRect(0, 12, width(), height() - 32);
    QIcon icon = this->icon();
    if (!icon.isNull()) {
        QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
        if (m_active)
            mode = QIcon::Active;
        else if (isChecked())
            mode = QIcon::Selected;
        icon.paint(&painter, iconRect, Qt::AlignCenter, mode, isChecked() ? QIcon::On : QIcon::Off);
    }
    
    // Draw the text centered at the bottom
    QFont textFont = painter.font();
    textFont.setPointSize(9);
    if (m_active) {
        textFont.setBold(true);
    }
    painter.setFont(textFont);
    
    if (m_active) {
        painter.setPen(QColor("#FFFFFF"));
    } else {
        painter.setPen(QColor("#CCCCCC"));
    }
    
    QRect textRect(0, height() - 25, width(), 20);
    painter.drawText(textRect, Qt::AlignCenter, text());
    
    // Draw the active indicator if needed
    if (m_active) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#4A90E2"));
        
        // Draw a vertical bar on the left side
        painter.drawRect(0, 0, 3, height());
    }
}

QSize SidebarButton::sizeHint() const
{
    return QSize(80, 70);
} 