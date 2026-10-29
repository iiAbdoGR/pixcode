#include "CancelSubscriptionDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"
#include "../styles/Theme.h"

namespace pixcode::desktop {

CancelSubscriptionDialog::CancelSubscriptionDialog(std::shared_ptr<ApplicationContext> appContext,
                                                   std::int64_t enrollmentId,
                                                   QWidget* parent)
    : QDialog(parent),
      m_appContext(std::move(appContext)),
      m_enrollmentId(enrollmentId) {
    setupUi();
    loadData();
}

void CancelSubscriptionDialog::setupUi() {
    setWindowTitle(LocalizationManager::instance().get("dialog.cancel_subscription.title", "Cancel Subscription"));
    setMinimumWidth(450);
    setStyleSheet(QString("QDialog { background-color: %1; color: %2; }")
        .arg(Theme::Background)
        .arg(Theme::Primary));

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(16);
    rootLayout->setContentsMargins(24, 24, 24, 24);

    auto* titleLabel = new QLabel(LocalizationManager::instance().get("dialog.cancel_subscription.heading", "Confirm Subscription Cancellation"), this);
    QFont f = titleLabel->font();
    f.setBold(true);
    f.setPointSize(12);
    titleLabel->setFont(f);
    titleLabel->setStyleSheet(QString("color: %1;").arg(Theme::Primary));
    rootLayout->addWidget(titleLabel);

    auto* warnLabel = new QLabel(
        LocalizationManager::instance().get("dialog.cancel_subscription.warning",
        "Warning: Cancelling releases the reserved seat in the group while preserving all historical payment records intact."),
        this);
    warnLabel->setWordWrap(true);
    warnLabel->setStyleSheet("color: #C0392B; font-weight: bold; padding: 8px; background-color: #FDEDEC; border-radius: 4px;");
    rootLayout->addWidget(warnLabel);

    m_infoLabel = new QLabel(this);
    m_infoLabel->setStyleSheet(QString("background-color: white; border: 1px solid %1; border-radius: 4px; padding: 12px; color: %2;")
        .arg(Theme::Border)
        .arg(Theme::Primary));
    rootLayout->addWidget(m_infoLabel);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    m_reasonEdit = new QTextEdit(this);
    m_reasonEdit->setPlaceholderText(LocalizationManager::instance().get("dialog.cancel_subscription.reason_ph", "State the reason for cancellation..."));
    m_reasonEdit->setMaximumHeight(80);
    m_reasonEdit->setStyleSheet(QString("QTextEdit { background-color: white; border: 1px solid %1; border-radius: 4px; padding: 6px; }")
        .arg(Theme::Border));
    formLayout->addRow(LocalizationManager::instance().get("dialog.cancel_subscription.reason", "Reason:"), m_reasonEdit);

    rootLayout->addLayout(formLayout);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_closeButton = new QPushButton(LocalizationManager::instance().get("common.cancel", "Cancel"), this);
    m_closeButton->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid %2; padding: 8px 16px; border-radius: 4px; }")
        .arg(Theme::Surface)
        .arg(Theme::Border));
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_closeButton);

    m_confirmButton = new QPushButton(LocalizationManager::instance().get("dialog.cancel_subscription.confirm_btn", "Cancel Subscription"), this);
    m_confirmButton->setStyleSheet("QPushButton { background-color: #E74C3C; color: white; font-weight: bold; padding: 8px 16px; border-radius: 4px; border: none; } QPushButton:hover { background-color: #C0392B; }");
    connect(m_confirmButton, &QPushButton::clicked, this, &CancelSubscriptionDialog::onConfirmClicked);
    btnLayout->addWidget(m_confirmButton);

    rootLayout->addLayout(btnLayout);
}

void CancelSubscriptionDialog::loadData() {
    auto res = m_appContext->enrollmentUseCases()->getEnrollment(m_enrollmentId);
    if (!res.isOk()) {
        QMessageBox::critical(this, LocalizationManager::instance().get("common.error", "Error"),
                              QString::fromStdString(res.error().message()));
        reject();
        return;
    }

    const auto& dto = res.value();
    QString info = QString("<b>%1:</b> %2 (%3)<br><b>%4:</b> %5 - %6<br><b>%7:</b> %8 EGP | <b>%9:</b> %10 EGP")
        .arg(LocalizationManager::instance().get("student.label", "Student"))
        .arg(QString::fromStdString(dto.studentName))
        .arg(QString::fromStdString(dto.studentCode))
        .arg(LocalizationManager::instance().get("courses.label", "Course / Group"))
        .arg(QString::fromStdString(dto.courseName))
        .arg(QString::fromStdString(dto.groupName))
        .arg(LocalizationManager::instance().get("finance.net_fee", "Net Fee"))
        .arg(dto.netFeePiastres / 100.0, 0, 'f', 2)
        .arg(LocalizationManager::instance().get("finance.paid", "Paid"))
        .arg(dto.paidPiastres / 100.0, 0, 'f', 2);

    m_infoLabel->setText(info);
}

void CancelSubscriptionDialog::onConfirmClicked() {
    std::string reason = m_reasonEdit->toPlainText().trimmed().toStdString();
    if (reason.empty()) {
        QMessageBox::warning(this, LocalizationManager::instance().get("common.warning", "Warning"),
                             LocalizationManager::instance().get("dialog.cancel_subscription.reason_req", "Please enter a cancellation reason."));
        return;
    }

    std::string actorId;
    if (m_appContext->authContext()->currentUser().has_value()) {
        actorId = m_appContext->authContext()->currentUser()->id();
    }

    application::CancelSubscriptionCommand cmd{
        .enrollmentId = m_enrollmentId,
        .reason = reason,
        .cancelledByUserId = actorId
    };

    auto res = m_appContext->enrollmentUseCases()->cancelEnrollment(cmd);
    if (!res.isOk()) {
        QMessageBox::critical(this, LocalizationManager::instance().get("common.error", "Error"),
                              QString::fromStdString(res.error().message()));
        return;
    }

    emit subscriptionCancelled(res.value());
    accept();
}

} // namespace pixcode::desktop

