#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class InstructorPayoutDialog : public QDialog {
    Q_OBJECT

public:
    explicit InstructorPayoutDialog(std::shared_ptr<ApplicationContext> appContext,
                                   std::int64_t instructorId,
                                   QWidget* parent = nullptr);

signals:
    void payoutRecorded(const std::string& receiptNumber);

private slots:
    void onSaveClicked();

private:
    void setupUi();

    std::shared_ptr<ApplicationContext> m_appContext;
    std::int64_t m_instructorId;

    QLabel* m_infoLabel;
    QDoubleSpinBox* m_amountSpin;
    QComboBox* m_methodCombo;
    QLineEdit* m_refEdit;
    QTextEdit* m_notesEdit;
    QPushButton* m_saveBtn;
    QPushButton* m_cancelBtn;
};

} // namespace pixcode::desktop

