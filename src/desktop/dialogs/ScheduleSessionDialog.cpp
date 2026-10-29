#include "ScheduleSessionDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QLabel>
#include "../localization/LocalizationManager.h"
#include "../styles/Theme.h"

namespace pixcode::desktop {

ScheduleSessionDialog::ScheduleSessionDialog(std::shared_ptr<ApplicationContext> appContext,
                                             std::int64_t groupId,
                                             std::optional<std::int64_t> sessionId,
                                             QWidget* parent)
    : QDialog(parent),
      m_appContext(std::move(appContext)),
      m_groupId(groupId),
      m_sessionId(sessionId) {

    setupUi();
    loadData();
}

void ScheduleSessionDialog::setupUi() {
    bool isRtl = LocalizationManager::instance().isRtl();
    setLayoutDirection(isRtl ? Qt::RightToLeft : Qt::LeftToRight);

    const bool isEdit = m_sessionId.has_value();
    setWindowTitle(isEdit
        ? (isRtl ? "تعديل موعد الحصة وإعادة جدولتها" : "Edit & Reschedule Session")
        : (isRtl ? "جدولة حصة جديدة للمجموعة" : "Schedule New Group Session"));
    setMinimumWidth(440);
    setStyleSheet(QString("QDialog { background-color: %1; color: %2; }")
        .arg(Theme::Background)
        .arg(Theme::TextPrimary));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    auto* titleLabel = new QLabel(windowTitle(), this);
    QFont f = titleLabel->font();
    f.setBold(true);
    f.setPointSize(12);
    titleLabel->setFont(f);
    titleLabel->setStyleSheet(QString("color: %1;").arg(Theme::Primary));
    mainLayout->addWidget(titleLabel);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    const QString inputStyle = QString("QLineEdit, QComboBox, QTextEdit, QDateEdit, QTimeEdit, QSpinBox { "
                                       "background-color: white; border: 1px solid %1; border-radius: 4px; padding: 6px; }")
        .arg(Theme::Border);

    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setStyleSheet(inputStyle);

    m_startTimeEdit = new QTimeEdit(QTime(QTime::currentTime().hour(), 0), this);
    m_startTimeEdit->setDisplayFormat("HH:mm");
    m_startTimeEdit->setStyleSheet(inputStyle);

    m_endTimeEdit = new QTimeEdit(m_startTimeEdit->time().addSecs(3600), this);
    m_endTimeEdit->setDisplayFormat("HH:mm");
    m_endTimeEdit->setStyleSheet(inputStyle);

    m_durationSpin = new QSpinBox(this);
    m_durationSpin->setRange(15, 600);
    m_durationSpin->setSingleStep(15);
    m_durationSpin->setValue(60);
    m_durationSpin->setSuffix(isRtl ? " دقيقة" : " mins");
    m_durationSpin->setStyleSheet(inputStyle);

    m_instructorCombo = new QComboBox(this);
    m_instructorCombo->setStyleSheet(inputStyle);
    m_instructorCombo->addItem(isRtl ? "بدون مدرب حالياً" : "None / Unassigned", QVariant::fromValue(static_cast<qint64>(0)));

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMaximumHeight(70);
    m_notesEdit->setStyleSheet(inputStyle);
    m_notesEdit->setPlaceholderText(isRtl ? "موضوع الحصة أو ملاحظات إضافية" : "Session topic or additional notes");

    formLayout->addRow(isRtl ? "تاريخ الحصة *:" : "Session Date *:", m_dateEdit);
    formLayout->addRow(isRtl ? "وقت البدء *:" : "Start Time *:", m_startTimeEdit);
    formLayout->addRow(isRtl ? "وقت الانتهاء *:" : "End Time *:", m_endTimeEdit);
    formLayout->addRow(isRtl ? "المدة المخططة:" : "Planned Duration:", m_durationSpin);
    formLayout->addRow(isRtl ? "المحاضر المسؤول:" : "Assigned Instructor:", m_instructorCombo);
    formLayout->addRow(isRtl ? "ملاحظات:" : "Notes:", m_notesEdit);

    mainLayout->addLayout(formLayout);

    auto* btnLayout = new QHBoxLayout();
    m_cancelBtn = new QPushButton(isRtl ? "إلغاء" : "Cancel", this);
    m_cancelBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid %2; padding: 8px 16px; border-radius: 4px; }")
        .arg(Theme::Sage)
        .arg(Theme::Border));

    m_saveBtn = new QPushButton(isEdit
        ? (isRtl ? "حفظ التعديلات" : "Save Changes")
        : (isRtl ? "جدولة الحصة" : "Schedule Session"), this);
    m_saveBtn->setStyleSheet(QString("QPushButton { background-color: %1; color: white; font-weight: bold; padding: 8px 18px; border-radius: 4px; border: none; } QPushButton:hover { background-color: %2; }")
        .arg(Theme::Secondary)
        .arg(Theme::Primary));

    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_saveBtn, &QPushButton::clicked, this, &ScheduleSessionDialog::onSaveClicked);

    // Sync start time, end time, and duration automatically
    connect(m_startTimeEdit, &QTimeEdit::timeChanged, this, &ScheduleSessionDialog::onStartTimeChanged);
    connect(m_endTimeEdit, &QTimeEdit::timeChanged, this, &ScheduleSessionDialog::onEndTimeChanged);
    connect(m_durationSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ScheduleSessionDialog::onDurationChanged);
}

void ScheduleSessionDialog::loadData() {
    // 1. Populate instructors
    application::InstructorFilter iFilter;
    auto instructors = m_appContext->instructorRepository()->findAll(iFilter);
    for (const auto& inst : instructors) {
        m_instructorCombo->addItem(QString::fromStdString(inst.fullName), QVariant::fromValue(static_cast<qint64>(inst.id)));
    }

    // Default instructor from group
    auto groupOpt = m_appContext->groupRepository()->findById(m_groupId);
    if (groupOpt.has_value() && groupOpt->instructorId.has_value()) {
        int idx = m_instructorCombo->findData(QVariant::fromValue(static_cast<qint64>(*groupOpt->instructorId)));
        if (idx >= 0) m_instructorCombo->setCurrentIndex(idx);
    }

    // 2. If editing existing session
    if (m_sessionId.has_value()) {
        auto sessOpt = m_appContext->sessionRepository()->findById(*m_sessionId);
        if (sessOpt.has_value()) {
            const auto& s = *sessOpt;
            m_dateEdit->setDate(QDate::fromString(QString::fromStdString(s.sessionDate), Qt::ISODate));

            m_syncingTime = true;
            if (!s.scheduledStartTime.empty()) {
                m_startTimeEdit->setTime(QTime::fromString(QString::fromStdString(s.scheduledStartTime), "HH:mm"));
            }
            if (!s.scheduledEndTime.empty()) {
                m_endTimeEdit->setTime(QTime::fromString(QString::fromStdString(s.scheduledEndTime), "HH:mm"));
            }
            if (s.plannedDurationMinutes > 0) {
                m_durationSpin->setValue(s.plannedDurationMinutes);
            }
            m_syncingTime = false;

            if (s.instructorId.has_value()) {
                int idx = m_instructorCombo->findData(QVariant::fromValue(static_cast<qint64>(*s.instructorId)));
                if (idx >= 0) m_instructorCombo->setCurrentIndex(idx);
            }

            m_notesEdit->setPlainText(QString::fromStdString(s.notes));
        }
    }
}

void ScheduleSessionDialog::onStartTimeChanged(const QTime& time) {
    if (m_syncingTime) return;
    m_syncingTime = true;
    m_endTimeEdit->setTime(time.addSecs(m_durationSpin->value() * 60));
    m_syncingTime = false;
}

void ScheduleSessionDialog::onEndTimeChanged(const QTime& time) {
    if (m_syncingTime) return;
    m_syncingTime = true;
    int diffMins = m_startTimeEdit->time().secsTo(time) / 60;
    if (diffMins > 0) {
        m_durationSpin->setValue(diffMins);
    }
    m_syncingTime = false;
}

void ScheduleSessionDialog::onDurationChanged(int minutes) {
    if (m_syncingTime) return;
    m_syncingTime = true;
    m_endTimeEdit->setTime(m_startTimeEdit->time().addSecs(minutes * 60));
    m_syncingTime = false;
}

void ScheduleSessionDialog::onSaveClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();

    QTime start = m_startTimeEdit->time();
    QTime end = m_endTimeEdit->time();

    if (start >= end) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Time Range Error",
                             isRtl ? "وقت الانتهاء يجب أن يكون بعد وقت البدء."
                                   : "Scheduled end time must be later than start time.");
        return;
    }

    if (m_durationSpin->value() <= 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Validation Error",
                             isRtl ? "مدة الحصة يجب أن تكون أكبر من 0 دقيقة."
                                   : "Session duration must be greater than zero.");
        return;
    }

    std::optional<std::int64_t> instId = std::nullopt;
    std::int64_t selectedInst = m_instructorCombo->currentData().toLongLong();
    if (selectedInst > 0) {
        instId = selectedInst;
    }

    try {
        if (m_sessionId.has_value()) {
            application::UpdateSessionCommand cmd;
            cmd.id = *m_sessionId;
            cmd.instructorId = instId;
            cmd.sessionDate = m_dateEdit->date().toString(Qt::ISODate).toStdString();
            cmd.scheduledStartTime = start.toString("HH:mm").toStdString();
            cmd.scheduledEndTime = end.toString("HH:mm").toStdString();
            cmd.plannedDurationMinutes = m_durationSpin->value();
            cmd.notes = m_notesEdit->toPlainText().trimmed().toStdString();

            auto res = m_appContext->payrollUseCases()->updateSession(cmd);
            if (!res.isOk()) {
                QMessageBox::critical(this, isRtl ? "خطأ" : "Error", QString::fromStdString(res.error().message()));
                return;
            }

            emit sessionScheduled(res.value().id);
            accept();
        } else {
            application::CreateSessionCommand cmd;
            cmd.groupId = m_groupId;
            cmd.instructorId = instId;
            cmd.sessionDate = m_dateEdit->date().toString(Qt::ISODate).toStdString();
            cmd.scheduledStartTime = start.toString("HH:mm").toStdString();
            cmd.scheduledEndTime = end.toString("HH:mm").toStdString();
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
    } catch (const std::exception& e) {
        QMessageBox::critical(this, isRtl ? "خطأ" : "Error", QString::fromStdString(e.what()));
        return;
    }
}

} // namespace pixcode::desktop
