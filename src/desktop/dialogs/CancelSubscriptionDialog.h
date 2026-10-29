#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <memory>
#include "../app/ApplicationContext.h"
#include "application/dto/EnrollmentDtos.h"

namespace pixcode::desktop {

class CancelSubscriptionDialog : public QDialog {
    Q_OBJECT

public:
    explicit CancelSubscriptionDialog(std::shared_ptr<ApplicationContext> appContext,
                                     std::int64_t enrollmentId,
                                     QWidget* parent = nullptr);

signals:
    void subscriptionCancelled(const application::EnrollmentDto& enrollment);

private slots:
    void onConfirmClicked();

private:
    void setupUi();
    void loadData();

    std::shared_ptr<ApplicationContext> m_appContext;
    std::int64_t m_enrollmentId;

    QLabel* m_infoLabel;
    QTextEdit* m_reasonEdit;
    QPushButton* m_confirmButton;
    QPushButton* m_closeButton;
};

} // namespace pixcode::desktop

