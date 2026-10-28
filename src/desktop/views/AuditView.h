#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class AuditView : public QWidget {
    Q_OBJECT

public:
    explicit AuditView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

    void refresh();

private:
    void setupUi();
    void updateTranslations();

    std::shared_ptr<ApplicationContext> m_appContext;

    QLabel* m_titleLabel;
    QTableWidget* m_table;
    QPushButton* m_refreshButton;
};

} // namespace pixcode::desktop

