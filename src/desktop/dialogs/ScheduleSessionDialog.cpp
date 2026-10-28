#include "ScheduleSessionDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"

namespace pixcode::desktop {

ScheduleSessionDialog::ScheduleSessionDialog(std::shared_ptr<ApplicationContext> appContext,
                                             std::int64_t groupId,
                                             QWidget* parent)
    : QDialog(parent),
      m_appContext(std::move(appContext)),
      m_groupId(groupId) {

    setupUi();
}

void ScheduleSessionDialog::setupUi() {
    bool isRtl = LocalizationManager::instance().isRtl();
    setWindowTitle(isRtl ? "جدولة حصة جديدة للمجموعة" : "Schedule New Group Session");
    setMinimumWidth(380);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(14);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);

    m_startTimeEdit = new QTimeEdit(QTime::currentTime(), this);
    m_endTimeEdit = new QTimeEdit(QTime::currentTime().addSecs(3600), this);

    m_durationSpin = new QSpinBox(this);
    m_durationSpin->setRange(15, 600);
    m_durationSpin->setValue(60);
    m_durationSpin->setSuffix(isRtl ? " دقيقة" : " mins");

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMaximumHeight(70);
    m_notesEdit->setPlaceholderText(isRtl ? "موضوع الحصة أو ملاحظات" : "Session topic or notes");

    formLayout->addRow(isRtl ? "تاريخ الحصة:" : "Session Date:", m_dateEdit);
    formLayout->addRow(isRtl ? "وقت البدء:" : "Start Time:", m_startTimeEdit);
    formLayout->addRow(isRtl ? "وقت الانتهاء:" : "End Time:", m_endTimeEdit);
    formLayout->addRow(isRtl ? "المدة المخططة:" : "Planned Duration:", m_durationSpin);
    formLayout->addRow(isRtl ? "ملاحظات:" : "Notes:", m_notesEdit);

    mainLayout->addLayout(formLayout);

    auto* btnLayout = new QHBoxLayout();
    m_saveBtn = new QPushButton(isRtl ? "جدولة الحصة" : "Schedule Session", this);
    m_saveBtn->setObjectName("primaryButton");
    m_cancelBtn = new QPushButton(isRtl ? "إلغاء" : "Cancel", this);
    m_cancelBtn->setObjectName("secondaryButton");

    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_saveBtn, &QPushButton::clicked, this, &ScheduleSessionDialog::onSaveClicked);
}

void ScheduleSessionDialog::onSaveClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();

    application::CreateSessionCommand cmd;
    cmd.groupId = m_groupId;
    auto groupOpt = m_appContext->groupRepository()->findById(m_groupId);
    if (groupOpt.has_value()) {
        cmd.instructorId = groupOpt->instructorId;
    }
    cmd.sessionDate = m_dateEdit->date().toString(Qt::ISODate).toStdString();
    cmd.scheduledStartTime = m_startTimeEdit->time().toString("HH:mm").toStdString();
    cmd.scheduledEndTime = m_endTimeEdit->time().toString("HH:mm").toStdString();
    cmd.plannedDurationMinutes = m_durationSpin->value();
    cmd.notes = m_notesEdit->toPlainText().trimmed().toStdString();

    auto res = m_appContext->payrollUseCases()->createSession(cmd);
    if (!res.isOk()) {
        QMessageBox::critical(this, isRtl ? "خطأ" : "Error", QString::fromStdString(res.error().message()));
        return;
    }

    emit sessionScheduled(res.value().id);
    accept();
}

} // namespace pixcode::desktop
