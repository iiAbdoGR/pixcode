#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class DashboardView : public QWidget {
    Q_OBJECT

public:
    explicit DashboardView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

    void refresh();

private:
    void setupUi();
    void updateTranslations();

    std::shared_ptr<ApplicationContext> m_appContext;

    QLabel* m_welcomeLabel;
    QLabel* m_roleBadge;
    QLabel* m_dbHealthLabel;
    QLabel* m_userCountLabel;
    QLabel* m_adminCountLabel;
};

} // namespace pixcode::desktop

