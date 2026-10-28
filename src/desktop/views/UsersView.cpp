#include "UsersView.h"
#include <QtCore/QDateTime>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"

namespace pixcode::desktop {

UsersView::UsersView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent)
    : QWidget(parent), m_appContext(std::move(appContext)) {
    setupUi();
    updateTranslations();

    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, &UsersView::updateTranslations);
}

void UsersView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(16);

    auto* headerLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("headerTitle");

    m_refreshButton = new QPushButton(this);
    m_refreshButton->setObjectName("secondaryButton");
    connect(m_refreshButton, &QPushButton::clicked, this, &UsersView::onRefreshClicked);

    m_unlockButton = new QPushButton(this);
    m_unlockButton->setObjectName("secondaryButton");
    connect(m_unlockButton, &QPushButton::clicked, this, &UsersView::onUnlockSelectedUser);

    m_toggleActiveButton = new QPushButton(this);
    m_toggleActiveButton->setObjectName("secondaryButton");
    connect(m_toggleActiveButton, &QPushButton::clicked, this, &UsersView::onToggleUserActive);

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_unlockButton);
    headerLayout->addWidget(m_toggleActiveButton);
    headerLayout->addWidget(m_refreshButton);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(m_table);
}

void UsersView::updateTranslations() {
    m_titleLabel->setText(trText("users.title"));
    m_refreshButton->setText(LocalizationManager::instance().isRtl() ? "تحديث القائمة" : "Refresh");
    m_unlockButton->setText(LocalizationManager::instance().isRtl() ? "إلغاء القفل" : "Unlock User");
    m_toggleActiveButton->setText(LocalizationManager::instance().isRtl() ? "تفعيل/تعطيل" : "Toggle Active");

    QStringList headers = {
        trText("users.col.username"),
        trText("users.col.role"),
        trText("users.col.status"),
        trText("users.col.failed"),
        trText("users.col.locked"),
        trText("users.col.last_login")
    };
    m_table->setHorizontalHeaderLabels(headers);
    refresh();
}

void UsersView::onRefreshClicked() {
    refresh();
}

void UsersView::refresh() {
    m_table->setRowCount(0);
    const auto users = m_appContext->userRepository()->findAll();
    const auto now = m_appContext->databaseManager()->databasePath(); // Dummy, we use current system time

    for (const auto& u : users) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        QString rolesStr;
        for (const auto& r : u.roles()) {
            if (!rolesStr.isEmpty()) rolesStr += ", ";
            rolesStr += QString::fromStdString(r.code());
        }

        const bool isLocked = u.lockoutUntilUtc().has_value() && !u.lockoutUntilUtc()->empty();

        auto* userItem = new QTableWidgetItem(QString::fromStdString(u.username()));
        userItem->setData(Qt::UserRole, QString::fromStdString(u.id()));

        m_table->setItem(row, 0, userItem);
        m_table->setItem(row, 1, new QTableWidgetItem(rolesStr));
        m_table->setItem(row, 2, new QTableWidgetItem(u.isActive() ? trText("users.status.active") : trText("users.status.disabled")));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::number(u.failedLoginCount())));
        m_table->setItem(row, 4, new QTableWidgetItem(isLocked ? trText("users.status.locked") : "-"));
        m_table->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(u.lastLoginAtUtc().value_or("-"))));
    }
}

void UsersView::onUnlockSelectedUser() {
    const int row = m_table->currentRow();
    if (row < 0) return;

    const QString userId = m_table->item(row, 0)->data(Qt::UserRole).toString();
    auto userOpt = m_appContext->userRepository()->findById(userId.toStdString());
    if (!userOpt.has_value()) return;

    auto user = *userOpt;
    user.unlock();
    m_appContext->userRepository()->save(user);

    m_appContext->auditService()->record(
        m_appContext->authContext()->currentUser()->id(),
        "USER_UNLOCKED", "User", user.id(), "Account unlocked by admin"
    );

    refresh();
}

void UsersView::onToggleUserActive() {
    const int row = m_table->currentRow();
    if (row < 0) return;

    const QString userId = m_table->item(row, 0)->data(Qt::UserRole).toString();
    auto userOpt = m_appContext->userRepository()->findById(userId.toStdString());
    if (!userOpt.has_value()) return;

    auto user = *userOpt;
    const bool isLastSuper = m_appContext->userRepository()->countActiveSuperAdmins() <= 1;

    try {
        const auto now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
        user.setActive(!user.isActive(), now, isLastSuper);
        m_appContext->userRepository()->save(user);

        m_appContext->auditService()->record(
            m_appContext->authContext()->currentUser()->id(),
            user.isActive() ? "USER_ENABLED" : "USER_DISABLED",
            "User", user.id(),
            user.isActive() ? "Account enabled by admin" : "Account disabled by admin"
        );

        refresh();
    } catch (const domain::DomainError& e) {
        QMessageBox::warning(this, "Operation Denied", QString::fromStdString(e.message()));
    }
}

} // namespace pixcode::desktop

