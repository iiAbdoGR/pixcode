#include "InstructorPayoutDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"

namespace pixcode::desktop {

InstructorPayoutDialog::InstructorPayoutDialog(std::shared_ptr<ApplicationContext> appContext,
                                               std::int64_t instructorId,
                                               QWidget* parent)
    : QDialog(parent),
      m_appContext(std::move(appContext)),
      m_instructorId(instructorId) {

    setupUi();
}

void InstructorPayoutDialog::setupUi() {
    bool isRtl = LocalizationManager::instance().isRtl();
    setWindowTitle(isRtl ? "صرف مستحقات / دفعة نقدية للمدرب" : "Issue Instructor Payout");
    setMinimumWidth(440);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(14);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    m_infoLabel = new QLabel(this);
    m_infoLabel->setStyleSheet("background-color: #2E3A44; color: #FFFFFF; padding: 10px; border-radius: 6px; font-weight: bold;");

    auto summaryRes = m_appContext->payrollUseCases()->getInstructorPayrollSummary(m_instructorId);
    if (summaryRes.isOk()) {
        const auto& s = summaryRes.value();
        domain::Money due(s.outstandingDuesPiastres);
        m_infoLabel->setText(QString(isRtl ? "المدرب: %1 | الرصيد المستحق له: %2" : "Instructor: %1 | Outstanding Due: %2")
            .arg(QString::fromStdString(s.instructorName))
            .arg(QString::fromStdString(due.format())));
    }
    mainLayout->addWidget(m_infoLabel);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    m_amountSpin = new QDoubleSpinBox(this);
    m_amountSpin->setRange(1.0, 1000000.0);
    m_amountSpin->setDecimals(2);
    m_amountSpin->setSuffix(isRtl ? " ج.م" : " EGP");

    if (summaryRes.isOk() && summaryRes.value().outstandingDuesPiastres > 0) {
        domain::Money due(summaryRes.value().outstandingDuesPiastres);
        m_amountSpin->setValue(due.piasters() / 100.0);
    }

    m_methodCombo = new QComboBox(this);
    m_methodCombo->addItem(isRtl ? "نقدي (خزينة الأكاديمية)" : "Cash", "cash");
    m_methodCombo->addItem(isRtl ? "تحويل بنكي" : "Bank Transfer", "bank_transfer");
    m_methodCombo->addItem(isRtl ? "محفظة إلكترونية (فودافون كاش / إنستاباي)" : "Mobile Wallet (Vodafone Cash / InstaPay)", "vodafone_cash");
    m_methodCombo->addItem(isRtl ? "شيك بنكي" : "Cheque", "cheque");

    m_refEdit = new QLineEdit(this);
    m_refEdit->setPlaceholderText(isRtl ? "رقم التحويل / رقم المعاملة البنكية أو المحفظة" : "Reference / Transaction ID");

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMaximumHeight(70);
    m_notesEdit->setPlaceholderText(isRtl ? "ملاحظات إضافية على الصرف" : "Payout remarks or notes");

    formLayout->addRow(isRtl ? "المبلغ المراد صرفه *:" : "Payout Amount *:", m_amountSpin);
    formLayout->addRow(isRtl ? "طريقة الصرف:" : "Payment Method:", m_methodCombo);
    formLayout->addRow(isRtl ? "رقم المرجع / العملية:" : "Reference / Txn ID:", m_refEdit);
    formLayout->addRow(isRtl ? "ملاحظات:" : "Notes:", m_notesEdit);

    mainLayout->addLayout(formLayout);

    auto* btnLayout = new QHBoxLayout();
    m_saveBtn = new QPushButton(isRtl ? "تأكيد وصرف الدفعة" : "Confirm & Issue Payout", this);
    m_saveBtn->setObjectName("primaryButton");
    m_cancelBtn = new QPushButton(isRtl ? "إلغاء" : "Cancel", this);
    m_cancelBtn->setObjectName("secondaryButton");

    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_saveBtn, &QPushButton::clicked, this, &InstructorPayoutDialog::onSaveClicked);
}

void InstructorPayoutDialog::onSaveClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    std::int64_t piastres = static_cast<std::int64_t>(m_amountSpin->value() * 100.0 + 0.5);

    if (piastres <= 0) {
        QMessageBox::warning(this, isRtl ? "خطأ" : "Validation Error",
            isRtl ? "يجب أن يكون مبلغ الصرف أكبر من الصفر." : "Payout amount must be greater than zero.");
        return;
    }

    application::RecordInstructorPayoutCommand cmd;
    cmd.instructorId = m_instructorId;
    cmd.amountPiastres = piastres;
    cmd.paymentMethod = m_methodCombo->currentData().toString().toStdString();
    cmd.referenceNumber = m_refEdit->text().trimmed().toStdString();
    cmd.notes = m_notesEdit->toPlainText().trimmed().toStdString();

    auto res = m_appContext->payrollUseCases()->recordPayout(cmd);
    if (!res.isOk()) {
        QMessageBox::critical(this, isRtl ? "فشل العملية" : "Error",
            QString::fromStdString(res.error().message()));
        return;
    }

    const auto& payout = res.value();
    QMessageBox::information(this, isRtl ? "تم الصرف بنجاح" : "Payout Successful",
        QString(isRtl ? "تم تسجيل صرف الدفعة بنجاح.\nرقم إيصال الصرف: <b>%1</b>"
                      : "Payout recorded successfully.\nReceipt Number: <b>%1</b>")
            .arg(QString::fromStdString(payout.receiptNumber)));

    emit payoutRecorded(payout.receiptNumber);
    accept();
}

} // namespace pixcode::desktop
