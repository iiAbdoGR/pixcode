#include "AttendanceView.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTimeEdit>
#include <QtCore/QTime>
#include "../localization/LocalizationManager.h"
#include "../dialogs/ScheduleSessionDialog.h"

namespace pixcode::desktop {

AttendanceView::AttendanceView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent)
    : QWidget(parent), m_appContext(std::move(appContext)) {

    setupUi();
    updateTranslations();

    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, &AttendanceView::updateTranslations);
}

void AttendanceView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(16);

    // Header
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("headerTitle");
    mainLayout->addWidget(m_titleLabel);

    // Controls bar
    auto* controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(12);

    auto* groupLbl = new QLabel(trText("attendance.select_group"), this);
    m_groupCombo = new QComboBox(this);
    m_groupCombo->setMinimumWidth(200);

    auto* sessionLbl = new QLabel(trText("attendance.select_session"), this);
    m_sessionCombo = new QComboBox(this);
    m_sessionCombo->setMinimumWidth(220);

    m_addSessionBtn = new QPushButton(this);
    m_addSessionBtn->setObjectName("secondaryButton");
    connect(m_addSessionBtn, &QPushButton::clicked, this, &AttendanceView::onAddSessionClicked);

    m_markAllPresentBtn = new QPushButton(this);
    m_markAllPresentBtn->setObjectName("secondaryButton");
    connect(m_markAllPresentBtn, &QPushButton::clicked, this, &AttendanceView::onMarkAllPresentClicked);

    m_saveAttendanceBtn = new QPushButton(this);
    m_saveAttendanceBtn->setObjectName("primaryButton");
    connect(m_saveAttendanceBtn, &QPushButton::clicked, this, &AttendanceView::onSaveAttendanceClicked);

    controlsLayout->addWidget(groupLbl);
    controlsLayout->addWidget(m_groupCombo);
    controlsLayout->addWidget(m_addSessionBtn);
    controlsLayout->addSpacing(16);
    controlsLayout->addWidget(sessionLbl);
    controlsLayout->addWidget(m_sessionCombo);
    controlsLayout->addStretch();
    controlsLayout->addWidget(m_markAllPresentBtn);
    controlsLayout->addWidget(m_saveAttendanceBtn);

    mainLayout->addLayout(controlsLayout);

    // Stats bar
    m_statsLabel = new QLabel(this);
    m_statsLabel->setStyleSheet("background-color: #2E3A44; color: #FFFFFF; padding: 10px; border-radius: 6px; font-weight: bold; font-size: 13px;");
    mainLayout->addWidget(m_statsLabel);

    // Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    mainLayout->addWidget(m_table);

    connect(m_groupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AttendanceView::onGroupChanged);
    connect(m_sessionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AttendanceView::onSessionChanged);
}

void AttendanceView::updateTranslations() {
    m_titleLabel->setText(trText("attendance.title"));
    m_addSessionBtn->setText(trText("attendance.schedule_session"));
    m_markAllPresentBtn->setText(trText("attendance.mark_all_present"));
    m_saveAttendanceBtn->setText(trText("attendance.save"));

    QStringList headers = {
        trText("attendance.col.student_name"),
        trText("attendance.col.phone"),
        trText("attendance.col.status"),
        trText("attendance.col.arrival_time"),
        trText("attendance.col.notes")
    };
    m_table->setHorizontalHeaderLabels(headers);

    refresh();
}

void AttendanceView::refresh() {
    loadGroups();
}

void AttendanceView::loadGroups() {
    m_groupCombo->blockSignals(true);
    m_groupCombo->clear();

    application::GroupFilter filter;
    auto groups = m_appContext->groupRepository()->findAll(filter);

    for (const auto& g : groups) {
        m_groupCombo->addItem(QString::fromStdString(g.name), static_cast<qint64>(g.id));
    }
    m_groupCombo->blockSignals(false);

    if (m_groupCombo->count() > 0) {
        m_currentGroupId = m_groupCombo->currentData().toLongLong();
        loadSessionsForGroup(m_currentGroupId);
    } else {
        m_sessionCombo->clear();
        m_table->setRowCount(0);
        m_statsLabel->setText(LocalizationManager::instance().isRtl() ? "لا توجد مجموعات متاحة" : "No groups available");
    }
}

void AttendanceView::onGroupChanged(int index) {
    if (index < 0) return;
    m_currentGroupId = m_groupCombo->itemData(index).toLongLong();
    loadSessionsForGroup(m_currentGroupId);
}

void AttendanceView::loadSessionsForGroup(std::int64_t groupId) {
    m_sessionCombo->blockSignals(true);
    m_sessionCombo->clear();

    application::SessionFilter filter;
    filter.groupId = groupId;
    auto sessions = m_appContext->sessionRepository()->findAll(filter);

    for (const auto& s : sessions) {
        QString text = QString("%1 (%2 - %3) - [%4]")
            .arg(QString::fromStdString(s.sessionDate))
            .arg(QString::fromStdString(s.scheduledStartTime))
            .arg(QString::fromStdString(s.scheduledEndTime))
            .arg(QString::fromStdString(s.statusToString()));
        m_sessionCombo->addItem(text, static_cast<qint64>(s.id));
    }
    m_sessionCombo->blockSignals(false);

    if (m_sessionCombo->count() > 0) {
        m_currentSessionId = m_sessionCombo->currentData().toLongLong();
        loadAttendanceForSession(m_currentSessionId);
    } else {
        m_table->setRowCount(0);
        m_statsLabel->setText(LocalizationManager::instance().isRtl() 
            ? "لا توجد حصص مسجلة لهذه المجموعة. اضغط على (+ إضافة موعد حصة) لجدولة أول حصة."
            : "No sessions for this group. Click (+ Schedule Session) to add one.");
    }
}

void AttendanceView::onSessionChanged(int index) {
    if (index < 0) return;
    m_currentSessionId = m_sessionCombo->itemData(index).toLongLong();
    loadAttendanceForSession(m_currentSessionId);
}

void AttendanceView::onAddSessionClicked() {
    if (m_currentGroupId <= 0) return;
    ScheduleSessionDialog dlg(m_appContext, m_currentGroupId, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadSessionsForGroup(m_currentGroupId);
    }
}

void AttendanceView::loadAttendanceForSession(std::int64_t sessionId) {
    m_table->setRowCount(0);
    bool isRtl = LocalizationManager::instance().isRtl();

    auto existingRes = m_appContext->attendanceUseCases()->getSessionAttendance(sessionId);
    if (existingRes.isOk() && !existingRes.value().empty()) {
        const auto& list = existingRes.value();
        for (const auto& item : list) {
            int row = m_table->rowCount();
            m_table->insertRow(row);

            auto* nameItem = new QTableWidgetItem(QString::fromStdString(item.studentName));
            nameItem->setData(Qt::UserRole, static_cast<qint64>(item.studentId));
            m_table->setItem(row, 0, nameItem);

            m_table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(item.studentCode)));

            auto* statusCombo = new QComboBox(m_table);
            statusCombo->addItem(trText("attendance.status.present"), "present");
            statusCombo->addItem(trText("attendance.status.absent"), "absent");
            statusCombo->addItem(trText("attendance.status.late"), "late");
            statusCombo->addItem(trText("attendance.status.excused"), "excused");

            int idx = statusCombo->findData(QString::fromStdString(item.status));
            if (idx >= 0) statusCombo->setCurrentIndex(idx);
            m_table->setCellWidget(row, 2, statusCombo);

            auto* arrivalEdit = new QLineEdit(m_table);
            arrivalEdit->setText(QString::fromStdString(item.arrivalTime));
            arrivalEdit->setPlaceholderText("HH:MM");
            m_table->setCellWidget(row, 3, arrivalEdit);

            auto* notesEdit = new QLineEdit(m_table);
            notesEdit->setText(QString::fromStdString(item.notes));
            m_table->setCellWidget(row, 4, notesEdit);
        }
    } else {
        // Load active enrollments for this group as fresh roster
        auto enrollments = m_appContext->enrollmentRepository()->findByGroup(m_currentGroupId);
        for (const auto& e : enrollments) {
            if (e.status != domain::EnrollmentStatus::Active) continue;
            auto studentOpt = m_appContext->studentRepository()->findById(e.studentId);
            if (!studentOpt.has_value()) continue;

            int row = m_table->rowCount();
            m_table->insertRow(row);

            auto* nameItem = new QTableWidgetItem(QString::fromStdString(studentOpt->fullName));
            nameItem->setData(Qt::UserRole, static_cast<qint64>(studentOpt->id));
            m_table->setItem(row, 0, nameItem);

            m_table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(studentOpt->phone)));

            auto* statusCombo = new QComboBox(m_table);
            statusCombo->addItem(trText("attendance.status.present"), "present");
            statusCombo->addItem(trText("attendance.status.absent"), "absent");
            statusCombo->addItem(trText("attendance.status.late"), "late");
            statusCombo->addItem(trText("attendance.status.excused"), "excused");
            statusCombo->setCurrentIndex(0); // default present
            m_table->setCellWidget(row, 2, statusCombo);

            auto* arrivalEdit = new QLineEdit(m_table);
            arrivalEdit->setPlaceholderText("HH:MM");
            m_table->setCellWidget(row, 3, arrivalEdit);

            auto* notesEdit = new QLineEdit(m_table);
            m_table->setCellWidget(row, 4, notesEdit);
        }
    }

    auto statsRes = m_appContext->attendanceUseCases()->getSessionStats(sessionId);
    if (statsRes.isOk()) {
        updateStatsLabel(statsRes.value());
    }
}

void AttendanceView::updateStatsLabel(const application::SessionAttendanceStatsDto& stats) {
    QString summary = trText("attendance.stats.summary")
        .arg(stats.totalStudents)
        .arg(stats.presentCount)
        .arg(stats.absentCount)
        .arg(stats.lateCount)
        .arg(stats.excusedCount)
        .arg(QString::number(stats.attendancePercentage, 'f', 1));
    m_statsLabel->setText(summary);
}

void AttendanceView::onMarkAllPresentClicked() {
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* combo = qobject_cast<QComboBox*>(m_table->cellWidget(r, 2));
        if (combo) {
            int idx = combo->findData("present");
            if (idx >= 0) combo->setCurrentIndex(idx);
        }
    }
}

void AttendanceView::onSaveAttendanceClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    if (m_currentSessionId <= 0 || m_table->rowCount() == 0) {
        QMessageBox::information(this, isRtl ? "تنبيه" : "Notice",
            isRtl ? "لا يوجد طلاب لحفظ حضورهم في هذه الحصة." : "No students to record attendance for.");
        return;
    }

    application::RecordSessionAttendanceCommand batch;
    batch.sessionId = m_currentSessionId;

    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* nameItem = m_table->item(r, 0);
        if (!nameItem) continue;

        std::int64_t studentId = nameItem->data(Qt::UserRole).toLongLong();
        auto* combo = qobject_cast<QComboBox*>(m_table->cellWidget(r, 2));
        auto* arrivalEdit = qobject_cast<QLineEdit*>(m_table->cellWidget(r, 3));
        auto* notesEdit = qobject_cast<QLineEdit*>(m_table->cellWidget(r, 4));

        application::RecordStudentAttendanceItem item;
        item.studentId = studentId;
        item.status = combo ? combo->currentData().toString().toStdString() : "present";
        if (arrivalEdit && !arrivalEdit->text().trimmed().isEmpty()) {
            item.arrivalTime = arrivalEdit->text().trimmed().toStdString();
        }
        if (notesEdit) {
            item.notes = notesEdit->text().trimmed().toStdString();
        }

        batch.items.push_back(item);
    }

    auto res = m_appContext->attendanceUseCases()->recordSessionAttendance(batch);
    if (!res.isOk()) {
        QMessageBox::critical(this, isRtl ? "خطأ" : "Error", QString::fromStdString(res.error().message()));
        return;
    }

    QMessageBox::information(this, isRtl ? "تم الحفظ" : "Success",
        isRtl ? "تم حفظ دفتر الحضور بنجاح." : "Attendance records saved successfully.");

    loadAttendanceForSession(m_currentSessionId);
}

} // namespace pixcode::desktop
