#include "PaymentDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"
#include "domain/value_objects/Money.h"

namespace pixcode::desktop {

PaymentDialog::PaymentDialog(std::shared_ptr<ApplicationContext> appContext,
                             std::int64_t enrollmentId,
                             QWidget* parent)
    : QDialog(parent),
      m_appContext(std::move(appContext)),
      m_enrollmentId(enrollmentId) {

    setupUi();
    loadEnrollmentData();
}

void PaymentDialog::setupUi() {
    bool isRtl = LocalizationManager::instance().isRtl();
    setWindowTitle(isRtl ? "تسجيل دفعة / إيصال قبض" : "Record Payment / Issue Receipt");
    setMinimumWidth(480);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    auto* infoLayout = new QFormLayout();
    infoLayout->setSpacing(8);

    m_studentInfoLabel = new QLabel(this);
    m_studentInfoLabel->setStyleSheet("font-weight: 600; color: #1F2933;");

    m_groupInfoLabel = new QLabel(this);
    m_netFeeLabel = new QLabel(this);
    m_alreadyPaidLabel = new QLabel(this);
    m_balanceLabel = new QLabel(this);
    m_balanceLabel->setStyleSheet("font-weight: bold; color: #D32F2F;");

    infoLayout->addRow(isRtl ? "الطالب:" : "Student:", m_studentInfoLabel);
    infoLayout->addRow(isRtl ? "المجموعة:" : "Group:", m_groupInfoLabel);
    infoLayout->addRow(isRtl ? "صافي الرسوم:" : "Net Fee:", m_netFeeLabel);
    infoLayout->addRow(isRtl ? "المدفوع سابقاً:" : "Already Paid:", m_alreadyPaidLabel);
    infoLayout->addRow(isRtl ? "المتبقي المستحق:" : "Remaining Balance:", m_balanceLabel);

    mainLayout->addLayout(infoLayout);

    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #CBD2D9;");
    mainLayout->addWidget(sep);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    m_amountEdit = new QLineEdit(this);
    m_amountEdit->setPlaceholderText(isRtl ? "المبلغ بالجنيه (مثال: 500)" : "Amount in EGP (e.g. 500)");

    m_methodCombo = new QComboBox(this);
    m_methodCombo->addItem(isRtl ? "نقدي (كاش)" : "Cash", "cash");
    m_methodCombo->addItem(isRtl ? "تحويل بنكي / فودافون كاش" : "Bank Transfer", "bank_transfer");
    m_methodCombo->addItem(isRtl ? "أخرى" : "Other", "other");

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMaximumHeight(70);
    m_notesEdit->setPlaceholderText(isRtl ? "ملاحظات الدفع أو المرجع" : "Payment reference or notes");

    formLayout->addRow(isRtl ? "المبلغ المدفوع (ج.م) *:" : "Amount (EGP) *:", m_amountEdit);
    formLayout->addRow(isRtl ? "طريقة الدفع:" : "Payment Method:", m_methodCombo);
    formLayout->addRow(isRtl ? "ملاحظات:" : "Notes:", m_notesEdit);

    mainLayout->addLayout(formLayout);

    auto* buttonsLayout = new QHBoxLayout();
    m_cancelButton = new QPushButton(isRtl ? "إلغاء" : "Cancel", this);
    m_cancelButton->setObjectName("secondaryButton");
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    m_saveButton = new QPushButton(isRtl ? "تأكيد الدفع وإصدار الإيصال" : "Confirm & Issue Receipt", this);
    m_saveButton->setObjectName("primaryButton");
    connect(m_saveButton, &QPushButton::clicked, this, &PaymentDialog::onRecordClicked);

    buttonsLayout->addStretch();
    buttonsLayout->addWidget(m_cancelButton);
    buttonsLayout->addWidget(m_saveButton);

    mainLayout->addLayout(buttonsLayout);
}

void PaymentDialog::loadEnrollmentData() {
    auto res = m_appContext->enrollmentUseCases()->getEnrollment(m_enrollmentId);
    if (res.isOk()) {
        const auto& e = res.value();
        m_studentInfoLabel->setText(QString::fromStdString(e.studentName) + " (" + QString::fromStdString(e.studentCode) + ")");
        m_groupInfoLabel->setText(QString::fromStdString(e.groupName) + " - " + QString::fromStdString(e.courseName));

        domain::Money netFee(e.netFeePiastres);
        domain::Money paid(e.paidPiastres);
        domain::Money balance(e.balancePiastres);

        m_netFeeLabel->setText(QString::fromStdString(netFee.format()));
        m_alreadyPaidLabel->setText(QString::fromStdString(paid.format()));
        m_balanceLabel->setText(QString::fromStdString(balance.format()));

        m_currentBalancePiastres = e.balancePiastres;

        // Default the amount field to the remaining balance in whole EGP
        m_amountEdit->setText(QString::number(balance.wholeEgp()));
        m_amountEdit->selectAll();
    }
}

void PaymentDialog::onRecordClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    bool ok = false;
    double egpAmount = m_amountEdit->text().trimmed().toDouble(&ok);

    if (!ok || egpAmount <= 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Validation Error",
                             isRtl ? "يرجى إدخال مبلغ صحيح أكبر من الصفر." : "Please enter a valid positive amount.");
        m_amountEdit->setFocus();
        return;
    }

    std::int64_t amountPiastres = static_cast<std::int64_t>(egpAmount * 100.0 + 0.5);

    if (amountPiastres > m_currentBalancePiastres) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Validation Error",
                             isRtl ? "المبلغ المدفوع يتجاوز الرصيد المستحق المتبقي." : "Amount exceeds the remaining balance.");
        m_amountEdit->setFocus();
        return;
    }

    std::optional<std::string> userId;
    if (m_appContext->authContext()->currentUser().has_value()) {
        userId = m_appContext->authContext()->currentUser()->id();
    }

    application::RecordPaymentCommand cmd{
        .enrollmentId = m_enrollmentId,
        .amountPiastres = amountPiastres,
        .paymentMethod = m_methodCombo->currentData().toString().toStdString(),
        .notes = m_notesEdit->toPlainText().trimmed().toStdString(),
        .recordedByUserId = userId
    };

    auto res = m_appContext->enrollmentUseCases()->recordPayment(cmd);
    if (res.isOk()) {
        const auto& p = res.value();
        QMessageBox::information(this, isRtl ? "تم بنجاح" : "Success",
            QString(isRtl ? "تم تسجيل الدفعة بنجاح!\nرقم الإيصال: %1\nالمبلغ: %2 ج.م"
                          : "Payment recorded successfully!\nReceipt Number: %1\nAmount: %2 EGP")
                .arg(QString::fromStdString(p.receiptNumber))
                .arg(egpAmount, 0, 'f', 2));

        emit paymentRecorded(p);
        accept();
    } else {
        QMessageBox::critical(this, isRtl ? "خطأ" : "Error",
                              QString::fromStdString(res.error().message()));
    }
}

} // namespace pixcode::desktop

