#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

class CounterWidget : public QWidget {
    Q_OBJECT

public:
    explicit CounterWidget(QWidget* parent = nullptr);

private slots:
    void increment();

private:
    int count = 0;
    QLabel* countLabel;
    QPushButton* incrementButton;
}; 