#include "mdichild.h"
#include <QRandomGenerator>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>

MdiChild::MdiChild(QWidget *parent)
    : QWidget(parent)
{
    // set background color
    setAutoFillBackground(true);
    QPalette p = palette();
    p.setColor(QPalette::Window, QColor("#171717"));
    setPalette(p);

    // Create a layout for the child window
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    setAttribute(Qt::WA_StyledBackground, false);

    // Create a label with some content
    contentLabel = new QLabel(this);
    contentLabel->setStyleSheet("color: #A3A3A3;");
    contentLabel->setAlignment(Qt::AlignCenter);
    contentLabel->setText("MDI Child Window Content\n\n"
                         "This is a sample MDI child window.\n"
                         "You can add more windows using the 'Add Window' button.\n"
                         "You can load a UI plugin by clicking on it");
    
    // Add the label to the layout
    layout->addWidget(contentLabel);
    
    // Set the layout for this widget
    setLayout(layout);
    
    // Set minimum size
    setMinimumSize(300, 200);
}

MdiChild::~MdiChild()
{
} 

void MdiChild::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    const QColor backgroundColor("#171717");
    const QColor borderColor("#434343");
    const qreal radius = 16.0;
    const qreal penWidth = 2.0;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRectF rect = this->rect().adjusted(penWidth / 2.0, penWidth / 2.0,
                                        -penWidth / 2.0, -penWidth / 2.0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(backgroundColor);
    painter.drawRoundedRect(rect, radius, radius);

    QPen pen(borderColor, penWidth);
    pen.setCosmetic(true);
    pen.setStyle(Qt::DashLine);
    pen.setDashPattern({4.0, 4.0});
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect, radius, radius);
}