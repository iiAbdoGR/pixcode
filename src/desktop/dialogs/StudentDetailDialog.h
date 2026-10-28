#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QPushButton>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class StudentDetailDialog : public QDialog {
    Q_OBJECT

public:
    explicit StudentDetailDialog(std::shared_ptr<ApplicationContext> appContext,
                                 std::int64_t studentId,
                                 QWidget* parent = nullptr);

signals:
    void studentUpdated();

private slots:
    void onNewEnrollmentClicked();
    void onRecordPaymentClicked();
    void onEditClicked();
    void onToggleArchiveClicked();
    void onRefresh();

private:
    void setupUi();
    void loadData();

    std::shared_ptr<ApplicationContext> m_appContext;
    std::int64_t m_studentId;

    QLabel* m_nameLabel;
    QLabel* m_codeLabel;
    QLabel* m_phoneLabel;
    QLabel* m_guardianLabel;
    QLabel* m_addressLabel;
    QLabel* m_statusLabel;
    QLabel* m_notesLabel;

    QTableWidget* m_enrollmentsTable;
    QPushButton* m_newEnrollmentBtn;
    QPushButton* m_recordPaymentBtn;
    QPushButton* m_editBtn;
    QPushButton* m_toggleArchiveBtn;
    QPushButton* m_closeBtn;
};

} // namespace pixcode::desktop

