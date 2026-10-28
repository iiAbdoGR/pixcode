#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <memory>
#include "../app/ApplicationContext.h"
#include "application/dto/EnrollmentDtos.h"

namespace pixcode::desktop {

class PaymentDialog : public QDialog {
    Q_OBJECT

public:
    explicit PaymentDialog(std::shared_ptr<ApplicationContext> appContext,
                           std::int64_t enrollmentId,
                           QWidget* parent = nullptr);

signals:
    void paymentRecorded(const application::PaymentDto& payment);

private slots:
    void onRecordClicked();

private:
    void setupUi();
    void loadEnrollmentData();

    std::shared_ptr<ApplicationContext> m_appContext;
    std::int64_t m_enrollmentId;

    QLabel* m_studentInfoLabel;
    QLabel* m_groupInfoLabel;
    QLabel* m_netFeeLabel;
    QLabel* m_alreadyPaidLabel;
    QLabel* m_balanceLabel;

    QLineEdit* m_amountEdit;
    QComboBox* m_methodCombo;
    QTextEdit* m_notesEdit;
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;

    std::int64_t m_currentBalancePiastres{0};
};

} // namespace pixcode::desktop

