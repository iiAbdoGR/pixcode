#include "AuditView.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include "../localization/LocalizationManager.h"

namespace pixcode::desktop {

AuditView::AuditView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent)
    : QWidget(parent), m_appContext(std::move(appContext)) {
    setupUi();
    updateTranslations();

    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, &AuditView::updateTranslations);
}

void AuditView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(16);

    auto* headerLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("headerTitle");

    m_refreshButton = new QPushButton(this);
    m_refreshButton->setObjectName("secondaryButton");
    connect(m_refreshButton, &QPushButton::clicked, this, &AuditView::refresh);

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_refreshButton);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(m_table);
}

void AuditView::updateTranslations() {
    m_titleLabel->setText(trText("audit.title"));
    m_refreshButton->setText(LocalizationManager::instance().isRtl() ? "تحديث السجل" : "Refresh");

    QStringList headers = {
        trText("audit.col.time"),
        trText("audit.col.actor"),
        trText("audit.col.action"),
        trText("audit.col.type"),
        trText("audit.col.summary")
    };
    m_table->setHorizontalHeaderLabels(headers);
    refresh();
}

void AuditView::refresh() {
    m_table->setRowCount(0);
    const auto logs = m_appContext->auditRepository()->findRecent(100, 0);

    for (const auto& log : logs) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        m_table->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(log.occurredAtUtc)));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(log.actorUsername)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(log.action)));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(log.entityType)));
        m_table->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(log.summary)));
    }
}

} // namespace pixcode::desktop

