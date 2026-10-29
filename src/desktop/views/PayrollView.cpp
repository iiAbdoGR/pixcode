#include "PayrollView.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QFrame>
#include "../localization/LocalizationManager.h"
#include "../dialogs/InstructorPayoutDialog.h"
#include "../styles/Theme.h"

namespace pixcode::desktop {

PayrollView::PayrollView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent)
    : QWidget(parent), m_appContext(std::move(appContext)) {

    setupUi();
    updateTranslations();

    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, &PayrollView::updateTranslations);
}

static QFrame* createCard(const QString& title, QLabel*& valueLabel, const QString& color) {
    auto* frame = new QFrame();
    frame->setStyleSheet(QString("background-color: #FFFFFF; border: 1px solid #CBD2D9; border-radius: 8px; padding: 12px;"));
    auto* layout = new QVBoxLayout(frame);
    layout->setSpacing(4);
    layout->setContentsMargins(12, 12, 12, 12);

    auto* lblTitle = new QLabel(title);
    lblTitle->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 600;").arg(Theme::TextSecondary));

    valueLabel = new QLabel("0");
    valueLabel->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: bold;").arg(color));

    layout->addWidget(lblTitle);
    layout->addWidget(valueLabel);
    return frame;
}

void PayrollView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(16);

    // 1. Top bar: Title + Instructor selector + actions
    auto* topLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("headerTitle");

    auto* selLbl = new QLabel(trText("payroll.select_instructor"), this);
    m_instructorCombo = new QComboBox(this);
    m_instructorCombo->setMinimumWidth(220);

    m_rateLabel = new QLabel(this);
    m_rateLabel->setStyleSheet("font-weight: bold; color: #2E3A44;");

    m_updateRateBtn = new QPushButton(trText("payroll.update_rate"), this);
    m_updateRateBtn->setObjectName("secondaryButton");
    connect(m_updateRateBtn, &QPushButton::clicked, this, &PayrollView::onUpdateRateClicked);

    m_approveSessionBtn = new QPushButton(trText("payroll.approve_session"), this);
    m_approveSessionBtn->setObjectName("secondaryButton");
    connect(m_approveSessionBtn, &QPushButton::clicked, this, &PayrollView::onApproveSessionClicked);

    m_issuePayoutBtn = new QPushButton(trText("payroll.record_payout"), this);
    m_issuePayoutBtn->setObjectName("primaryButton");
    connect(m_issuePayoutBtn, &QPushButton::clicked, this, &PayrollView::onIssuePayoutClicked);

    topLayout->addWidget(m_titleLabel);
    topLayout->addSpacing(16);
    topLayout->addWidget(selLbl);
    topLayout->addWidget(m_instructorCombo);
    topLayout->addWidget(m_rateLabel);
    topLayout->addWidget(m_updateRateBtn);
    topLayout->addStretch();
    topLayout->addWidget(m_approveSessionBtn);
    topLayout->addWidget(m_issuePayoutBtn);

    mainLayout->addLayout(topLayout);

    // 2. KPI Cards Row
    auto* cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(12);

    cardsLayout->addWidget(createCard(trText("payroll.card.sessions"), m_sessionsCountLabel, Theme::Primary));
    cardsLayout->addWidget(createCard(trText("payroll.card.hours"), m_hoursLabel, Theme::Secondary));
    cardsLayout->addWidget(createCard(trText("payroll.card.earned"), m_earnedLabel, Theme::Success));
    cardsLayout->addWidget(createCard(trText("payroll.card.paid"), m_paidLabel, Theme::Slate));
    cardsLayout->addWidget(createCard(trText("payroll.card.due"), m_dueLabel, Theme::Danger));
    cardsLayout->addWidget(createCard("حالة السداد", m_settlementStatusLabel, Theme::Primary));

    mainLayout->addLayout(cardsLayout);

    // 3. Tab Widget: Sessions, Settled Archive & Payouts
    m_tabWidget = new QTabWidget(this);

    // Tab 1: Active & Payable Sessions
    m_sessionsTable = new QTableWidget(this);
    m_sessionsTable->setColumnCount(8);
    m_sessionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_sessionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_sessionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_sessionsTable->horizontalHeader()->setStretchLastSection(true);
    m_sessionsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(m_sessionsTable, &QTableWidget::itemSelectionChanged, this, &PayrollView::onSessionSelectionChanged);
    m_tabWidget->addTab(m_sessionsTable, "الحصص الجارية والمستحقة");

    // Tab 2: Settled / Archived Sessions
    m_archivedSessionsTable = new QTableWidget(this);
    m_archivedSessionsTable->setColumnCount(8);
    m_archivedSessionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_archivedSessionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_archivedSessionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_archivedSessionsTable->horizontalHeader()->setStretchLastSection(true);
    m_archivedSessionsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tabWidget->addTab(m_archivedSessionsTable, "أرشيف الحصص المسددة");

    // Tab 3: Payouts
    m_payoutsTable = new QTableWidget(this);
    m_payoutsTable->setColumnCount(6);
    m_payoutsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_payoutsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_payoutsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_payoutsTable->horizontalHeader()->setStretchLastSection(true);
    m_payoutsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tabWidget->addTab(m_payoutsTable, trText("payroll.tab.payouts"));

    mainLayout->addWidget(m_tabWidget);

    connect(m_instructorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PayrollView::onInstructorChanged);
}

void PayrollView::updateTranslations() {
    m_titleLabel->setText(trText("payroll.title"));
    m_updateRateBtn->setText(trText("payroll.update_rate"));
    m_approveSessionBtn->setText(trText("payroll.approve_session"));
    m_issuePayoutBtn->setText(trText("payroll.record_payout"));

    m_tabWidget->setTabText(0, "الحصص الجارية والمستحقة");
    m_tabWidget->setTabText(1, "أرشيف الحصص المسددة");
    m_tabWidget->setTabText(2, trText("payroll.tab.payouts"));

    QStringList sessionHeaders = {
        trText("payroll.col.session_date"),
        trText("payroll.col.group"),
        trText("payroll.col.duration"),
        trText("payroll.col.session_status"),
        trText("payroll.col.approval"),
        "حالة الصرف",
        trText("payroll.col.earned"),
        trText("common.notes")
    };
    m_sessionsTable->setHorizontalHeaderLabels(sessionHeaders);
    m_archivedSessionsTable->setHorizontalHeaderLabels(sessionHeaders);

    QStringList payoutHeaders = {
        trText("payroll.col.payout_receipt"),
        trText("payroll.col.payout_amount"),
        trText("payroll.col.payout_method"),
        trText("payroll.col.payout_date"),
        "المرجع",
        trText("common.notes")
    };
    m_payoutsTable->setHorizontalHeaderLabels(payoutHeaders);

    refresh();
}

void PayrollView::refresh() {
    loadInstructors();
}

void PayrollView::loadInstructors() {
    m_instructorCombo->blockSignals(true);
    m_instructorCombo->clear();

    application::InstructorFilter filter;
    filter.status = domain::InstructorStatus::Active;
    auto instructors = m_appContext->instructorRepository()->findAll(filter);

    for (const auto& inst : instructors) {
        m_instructorCombo->addItem(QString::fromStdString(inst.fullName), static_cast<qint64>(inst.id));
    }
    m_instructorCombo->blockSignals(false);

    if (m_instructorCombo->count() > 0) {
        m_currentInstructorId = m_instructorCombo->currentData().toLongLong();
        loadInstructorData(m_currentInstructorId);
    } else {
        m_rateLabel->setText("");
        m_sessionsCountLabel->setText("0");
        m_hoursLabel->setText("0");
        m_earnedLabel->setText("0");
        m_paidLabel->setText("0");
        m_dueLabel->setText("0");
        m_sessionsTable->setRowCount(0);
        m_payoutsTable->setRowCount(0);
    }
}

void PayrollView::onInstructorChanged(int index) {
    if (index < 0) return;
    m_currentInstructorId = m_instructorCombo->itemData(index).toLongLong();
    loadInstructorData(m_currentInstructorId);
}

void PayrollView::loadInstructorData(std::int64_t instructorId) {
    auto summaryRes = m_appContext->payrollUseCases()->getInstructorPayrollSummary(instructorId);
    if (!summaryRes.isOk()) return;

    const auto& s = summaryRes.value();

    bool isOwner = m_appContext->authContext()->currentUser().has_value() && m_appContext->authContext()->currentUser()->isOwner();
    m_rateLabel->setVisible(isOwner);
    m_updateRateBtn->setVisible(isOwner);
    m_approveSessionBtn->setVisible(isOwner);

    if (s.compensationType == "monthly_salary") {
        m_rateLabel->setText(trText("payroll.summary.monthly_salary").arg(QString::number(s.monthlySalaryPiastres / 100.0, 'f', 2)));
    } else if (s.compensationType == "per_session") {
        m_rateLabel->setText(trText("payroll.summary.per_session").arg(QString::number(s.currentHourlyRatePiastres / 100.0, 'f', 2)));
    } else if (s.compensationType == "fixed_contract") {
        m_rateLabel->setText(trText("payroll.summary.fixed_contract").arg(QString::number(s.currentHourlyRatePiastres / 100.0, 'f', 2)));
    } else {
        m_rateLabel->setText(trText("payroll.summary.hourly").arg(QString::number(s.currentHourlyRatePiastres / 100.0, 'f', 2)));
    }
    m_sessionsCountLabel->setText(QString::number(s.totalCompletedSessions));
    m_hoursLabel->setText(QString::number(s.totalApprovedPayableMinutes / 60.0, 'f', 1));

    domain::Money earned(s.grossEarnedPiastres);
    m_earnedLabel->setText(QString::fromStdString(earned.format()));

    domain::Money paid(s.totalPaidOutPiastres);
    m_paidLabel->setText(QString::fromStdString(paid.format()));

    domain::Money due(s.outstandingDuesPiastres);
    m_dueLabel->setText(QString::fromStdString(due.format()));

    if (s.settlementStatus == "fully_paid") {
        m_settlementStatusLabel->setText("مسدد بالكامل");
        m_settlementStatusLabel->setStyleSheet("color: #2E7D32; font-size: 16px; font-weight: bold;");
    } else if (s.settlementStatus == "partially_paid") {
        m_settlementStatusLabel->setText("مسدد جزئياً");
        m_settlementStatusLabel->setStyleSheet("color: #0A4174; font-size: 16px; font-weight: bold;");
    } else {
        m_settlementStatusLabel->setText(s.grossEarnedPiastres > 0 ? "بانتظار الصرف" : "لا توجد مستحقات");
        m_settlementStatusLabel->setStyleSheet("color: #ED6C02; font-size: 16px; font-weight: bold;");
    }

    // Load sessions tables (Active / Settled)
    m_sessionsTable->setRowCount(0);
    m_archivedSessionsTable->setRowCount(0);
    application::SessionFilter filter;
    filter.instructorId = instructorId;
    auto sessions = m_appContext->sessionRepository()->findAll(filter);
    for (const auto& sess : sessions) {
        bool isSettled = (sess.settlementStatus == domain::SessionSettlementStatus::FullyPaid);
        QTableWidget* targetTable = isSettled ? m_archivedSessionsTable : m_sessionsTable;

        int r = targetTable->rowCount();
        targetTable->insertRow(r);

        auto* dateItem = new QTableWidgetItem(QString::fromStdString(sess.sessionDate));
        dateItem->setData(Qt::UserRole, static_cast<qint64>(sess.id));
        targetTable->setItem(r, 0, dateItem);

        auto groupOpt = m_appContext->groupRepository()->findById(sess.groupId);
        QString groupName = groupOpt ? QString::fromStdString(groupOpt->name) : QString::number(sess.groupId);
        targetTable->setItem(r, 1, new QTableWidgetItem(groupName));

        targetTable->setItem(r, 2, new QTableWidgetItem(QString("%1m").arg(sess.payableDurationMinutes)));
        targetTable->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(sess.statusToString())));

        auto* appItem = new QTableWidgetItem(sess.approvalStatus == domain::SessionApprovalStatus::Approved ? "معتمدة" : "قيد الاعتماد");
        appItem->setData(Qt::UserRole, QString::fromStdString(sess.approvalStatusToString()));
        if (sess.approvalStatus == domain::SessionApprovalStatus::Approved) {
            appItem->setForeground(QBrush(QColor("#2E7D32")));
        } else {
            appItem->setForeground(QBrush(QColor("#ED6C02")));
        }
        targetTable->setItem(r, 4, appItem);

        QString settlText = "غير مسددة";
        QColor settlColor("#ED6C02");
        if (sess.settlementStatus == domain::SessionSettlementStatus::FullyPaid) {
            settlText = "مسددة بالكامل";
            settlColor = QColor("#2E7D32");
        } else if (sess.settlementStatus == domain::SessionSettlementStatus::PartiallyPaid) {
            settlText = "مسددة جزئياً";
            settlColor = QColor("#0A4174");
        }
        auto* settlItem = new QTableWidgetItem(settlText);
        settlItem->setForeground(QBrush(settlColor));
        targetTable->setItem(r, 5, settlItem);

        domain::Money earnedMoney = sess.earnedAmount;
        targetTable->setItem(r, 6, new QTableWidgetItem(QString::fromStdString(earnedMoney.format())));
        targetTable->setItem(r, 7, new QTableWidgetItem(QString::fromStdString(sess.notes)));
    }
    onSessionSelectionChanged();

    // Load payouts table
    m_payoutsTable->setRowCount(0);
    auto payouts = m_appContext->payoutRepository()->findByInstructor(instructorId);
    for (const auto& p : payouts) {
        int r = m_payoutsTable->rowCount();
        m_payoutsTable->insertRow(r);

        m_payoutsTable->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(p.receiptNumber)));

        domain::Money amt = p.amount;
        m_payoutsTable->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(amt.format())));
        m_payoutsTable->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(p.paymentMethodToString())));
        QString pDate = QString::fromStdString(p.paymentDateUtc);
        if (pDate.length() >= 10) pDate = pDate.left(10);
        m_payoutsTable->setItem(r, 3, new QTableWidgetItem(pDate));
        m_payoutsTable->setItem(r, 4, new QTableWidgetItem(QString::fromStdString(p.referenceNumber.empty() ? "-" : p.referenceNumber)));
        m_payoutsTable->setItem(r, 5, new QTableWidgetItem(QString::fromStdString(p.notes)));
    }
}

void PayrollView::onUpdateRateClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    if (m_currentInstructorId <= 0) return;

    auto instOpt = m_appContext->instructorRepository()->findById(m_currentInstructorId);
    if (!instOpt.has_value()) return;

    QString promptTitle = isRtl ? "تعديل الاستحقاق المالي" : "Update Compensation";
    QString promptLabel = isRtl ? "أدخل القيمة الجديدة بالجنيه (EGP):" : "Enter new value (EGP):";
    double currentVal = (instOpt->compensationType == domain::CompensationType::MonthlySalary)
        ? (instOpt->monthlySalary.piasters() / 100.0)
        : (instOpt->baseRate.piasters() > 0 ? instOpt->baseRate.piasters() / 100.0 : instOpt->hourlyRate.piasters() / 100.0);

    bool ok = false;
    double newRate = QInputDialog::getDouble(
        this,
        promptTitle,
        promptLabel,
        currentVal,
        0.0, 1000000.0, 2, &ok
    );

    if (ok) {
        std::int64_t piastres = static_cast<std::int64_t>(newRate * 100.0 + 0.5);
        if (instOpt->compensationType == domain::CompensationType::MonthlySalary) {
            instOpt->monthlySalary = domain::Money(piastres);
            m_appContext->instructorRepository()->save(*instOpt);
        } else {
            instOpt->baseRate = domain::Money(piastres);
            instOpt->hourlyRate = domain::Money(piastres);
            m_appContext->instructorRepository()->save(*instOpt);
            m_appContext->instructorRepository()->updateHourlyRate(m_currentInstructorId, piastres, "Rate adjusted via management portal");
        }

        loadInstructorData(m_currentInstructorId);
    }
}

void PayrollView::onIssuePayoutClicked() {
    if (m_currentInstructorId <= 0) return;
    InstructorPayoutDialog dlg(m_appContext, m_currentInstructorId, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadInstructorData(m_currentInstructorId);
    }
}

void PayrollView::onSessionSelectionChanged() {
    int row = m_sessionsTable->currentRow();
    if (row < 0) {
        m_approveSessionBtn->setEnabled(false);
        return;
    }
    QString appSt = m_sessionsTable->item(row, 4) ? m_sessionsTable->item(row, 4)->data(Qt::UserRole).toString() : "";
    if (appSt == "approved") {
        m_approveSessionBtn->setEnabled(false);
        m_approveSessionBtn->setToolTip("هذه الحصة معتمدة بالفعل ولا يمكن اعتمادها مرة ثانية.");
    } else {
        m_approveSessionBtn->setEnabled(true);
        m_approveSessionBtn->setToolTip("");
    }
}

void PayrollView::onApproveSessionClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    int row = m_sessionsTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, isRtl ? "تنبيه" : "Notice",
            isRtl ? "يرجى اختيار حصة من الجدول أولاً لاعتمادها." : "Please select a session from the table to approve.");
        return;
    }

    QString currentAppSt = m_sessionsTable->item(row, 4) ? m_sessionsTable->item(row, 4)->data(Qt::UserRole).toString() : "";
    if (currentAppSt == "approved") {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Notice",
            isRtl ? "هذه الحصة معتمدة بالفعل ولا يمكن اعتمادها مرة ثانية." : "This session is already approved and cannot be approved twice.");
        return;
    }

    std::int64_t sessionId = m_sessionsTable->item(row, 0)->data(Qt::UserRole).toLongLong();
    application::ApproveSessionCommand cmd;
    cmd.sessionId = sessionId;
    auto sessOpt = m_appContext->sessionRepository()->findById(sessionId);
    if (sessOpt.has_value()) {
        cmd.approvedPayableMinutes = sessOpt->payableDurationMinutes;
    }
    if (m_appContext->authContext()->currentUser().has_value()) {
        cmd.approvedByUserId = m_appContext->authContext()->currentUser()->id();
    }

    try {
        auto res = m_appContext->payrollUseCases()->approveSession(cmd);
        if (!res.isOk()) {
            QMessageBox::critical(this, isRtl ? "خطأ" : "Error", QString::fromStdString(res.error().message()));
            return;
        }

        QMessageBox::information(this, isRtl ? "تم الاعتماد" : "Success",
            isRtl ? "تم اعتماد الحصة واحتساب الأجر بنجاح." : "Session approved and earnings calculated successfully.");

        loadInstructorData(m_currentInstructorId);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, isRtl ? "خطأ" : "Error", QString::fromStdString(e.what()));
    }
}

} // namespace pixcode::desktop
