#include "UsersView.h"
#include <QtCore/QDateTime>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"
#include "../dialogs/StaffFormDialog.h"
#include "../dialogs/ResetPasswordDialog.h"

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

    m_addStaffBtn = new QPushButton(this);
    m_addStaffBtn->setObjectName("primaryButton");
    connect(m_addStaffBtn, &QPushButton::clicked, this, &UsersView::onAddStaffClicked);

    m_editStaffBtn = new QPushButton(this);
    m_editStaffBtn->setObjectName("secondaryButton");
    connect(m_editStaffBtn, &QPushButton::clicked, this, &UsersView::onEditStaffClicked);

    m_resetPassBtn = new QPushButton(this);
    m_resetPassBtn->setObjectName("secondaryButton");
    connect(m_resetPassBtn, &QPushButton::clicked, this, &UsersView::onResetPasswordClicked);

    m_unlockButton = new QPushButton(this);
    m_unlockButton->setObjectName("secondaryButton");
    connect(m_unlockButton, &QPushButton::clicked, this, &UsersView::onUnlockSelectedUser);

    m_toggleActiveButton = new QPushButton(this);
    m_toggleActiveButton->setObjectName("secondaryButton");
    connect(m_toggleActiveButton, &QPushButton::clicked, this, &UsersView::onToggleUserActive);

    m_refreshButton = new QPushButton(this);
    m_refreshButton->setObjectName("secondaryButton");
    connect(m_refreshButton, &QPushButton::clicked, this, &UsersView::onRefreshClicked);

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_addStaffBtn);
    headerLayout->addWidget(m_editStaffBtn);
    headerLayout->addWidget(m_resetPassBtn);
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
    bool isRtl = LocalizationManager::instance().isRtl();
    m_titleLabel->setText(trText("users.title"));
    m_addStaffBtn->setText(trText("users.add_staff"));
    m_editStaffBtn->setText(trText("users.edit_staff"));
    m_resetPassBtn->setText(trText("users.reset_password"));
    m_refreshButton->setText(isRtl ? "تحديث" : "Refresh");
    m_unlockButton->setText(isRtl ? "إلغاء القفل" : "Unlock User");
    m_toggleActiveButton->setText(isRtl ? "تفعيل/تعطيل" : "Toggle Active");

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

void UsersView::onAddStaffClicked() {
    StaffFormDialog dlg(m_appContext, std::nullopt, this);
    connect(&dlg, &StaffFormDialog::staffSaved, this, &UsersView::refresh);
    dlg.exec();
}

void UsersView::onEditStaffClicked() {
    const int row = m_table->currentRow();
    if (row < 0) {
        bool isRtl = LocalizationManager::instance().isRtl();
        QMessageBox::information(this, isRtl ? "تنبيه" : "Notice",
            isRtl ? "يرجى اختيار موظف من الجدول أولاً." : "Please select a staff member first.");
        return;
    }

    const QString userId = m_table->item(row, 0)->data(Qt::UserRole).toString();
    StaffFormDialog dlg(m_appContext, userId.toStdString(), this);
    connect(&dlg, &StaffFormDialog::staffSaved, this, &UsersView::refresh);
    dlg.exec();
}

void UsersView::onResetPasswordClicked() {
    const int row = m_table->currentRow();
    if (row < 0) {
        bool isRtl = LocalizationManager::instance().isRtl();
        QMessageBox::information(this, isRtl ? "تنبيه" : "Notice",
            isRtl ? "يرجى اختيار موظف من الجدول أولاً." : "Please select a staff member first.");
        return;
    }

    const QString userId = m_table->item(row, 0)->data(Qt::UserRole).toString();
    const QString username = m_table->item(row, 0)->text();
    ResetPasswordDialog dlg(m_appContext, userId.toStdString(), username.toStdString(), this);
    dlg.exec();
}

void UsersView::refresh() {
    m_table->setRowCount(0);
    const auto users = m_appContext->userRepository()->findAll();

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
