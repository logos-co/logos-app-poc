#include "CounterWidget.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

CounterWidget::CounterWidget(QWidget* parent) : QWidget(parent) {
    // Create UI elements
    countLabel = new QLabel("0", this);
    incrementButton = new QPushButton("Increment me", this);

    // Set up layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(countLabel);
    layout->addWidget(incrementButton);

    // Connect button click to increment slot
    connect(incrementButton, &QPushButton::clicked, this, &CounterWidget::increment);
}

void CounterWidget::increment() {
    count++;
    countLabel->setText(QString::number(count));
} 