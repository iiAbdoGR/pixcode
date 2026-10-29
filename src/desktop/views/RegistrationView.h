#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class RegistrationView : public QWidget {
    Q_OBJECT

public:
    explicit RegistrationView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

    void refresh();

private slots:
    void onNewEnrollmentClicked();
    void onRecordPaymentClicked();
    void onCancelEnrollmentClicked();
    void onRestoreEnrollmentClicked();
    void onFilterChanged();

private:
    void setupUi();
    void updateTranslations();
    void loadEnrollments();

    std::shared_ptr<ApplicationContext> m_appContext;

    QLabel* m_titleLabel;
    QComboBox* m_statusFilterCombo;
    QPushButton* m_enrollButton;
    QPushButton* m_paymentButton;
    QPushButton* m_cancelEnrollmentButton;
    QPushButton* m_restoreEnrollmentButton;
    QPushButton* m_refreshButton;

    QTableWidget* m_table;
};

} // namespace pixcode::desktop
