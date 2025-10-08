#include "dashboardview.h"
#include <QFont>

DashboardView::DashboardView(QWidget *parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_title(nullptr)
    , m_contentLabel(nullptr)
{
    setupUi();
}

DashboardView::~DashboardView()
{
    // Qt automatically cleans up child widgets
}

void DashboardView::setupUi()
{
    // Set the background color for the entire dashboard
    setStyleSheet("QWidget#dashboardView { background-color: #1e1e1e; }");
    setObjectName("dashboardView");

    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(20);
    m_layout->setContentsMargins(40, 40, 40, 40);
    m_layout->setAlignment(Qt::AlignCenter);

    // Create and style the title
    m_title = new QLabel("Dashboard", this);
    QFont titleFont = m_title->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    m_title->setStyleSheet("color: #ffffff;");
    m_title->setAlignment(Qt::AlignCenter);

    // Add content label
    m_contentLabel = new QLabel("Dashboard", this);
    m_contentLabel->setStyleSheet("color: #a0a0a0; font-size: 18px;");
    m_contentLabel->setAlignment(Qt::AlignCenter);

    m_layout->addWidget(m_title);
    m_layout->addWidget(m_contentLabel);
} 