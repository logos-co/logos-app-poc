#include "mdichild.h"
#include <QRandomGenerator>
#include <QApplication>

MdiChild::MdiChild(QWidget *parent)
    : QWidget(parent)
{
    // Create a layout for the child window
    layout = new QVBoxLayout(this);
    
    // Create a label with some content
    contentLabel = new QLabel(this);
    
    // Use system colors instead of terminal-like styling
    QPalette palette = QApplication::palette();
    QString styleSheet = QString("background-color: %1; "
                                "color: %2; "
                                "border: 1px solid %3; "
                                "padding: 10px;")
                                .arg(palette.color(QPalette::Base).name(),
                                     palette.color(QPalette::Text).name(),
                                     palette.color(QPalette::Mid).name());
    contentLabel->setStyleSheet(styleSheet);
    contentLabel->setAlignment(Qt::AlignCenter);
    contentLabel->setText("MDI Child Window Content\n\n"
                         "This is a sample MDI child window.\n"
                         "You can add more windows using the 'Add Window' button.\n"
                         "You can toggle between windowed and tabbed mode using the 'Toggle View Mode' button.");
    
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