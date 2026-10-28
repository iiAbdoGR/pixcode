#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QLabel>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class FinanceView : public QWidget {
    Q_OBJECT

public:
    explicit FinanceView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

    void refresh();

private slots:
    void onSearchChanged(const QString& text);
    void onViewReceiptClicked();
    void onRowDoubleClicked(int row, int column);

private:
    void setupUi();
    void updateTranslations();
    void loadFinancials();

    std::shared_ptr<ApplicationContext> m_appContext;

    QLabel* m_titleLabel;
    QLabel* m_totalCollectedLabel;
    QLabel* m_totalReceivablesLabel;

    QLineEdit* m_searchEdit;
    QPushButton* m_viewReceiptBtn;
    QPushButton* m_refreshBtn;

    QTableWidget* m_table;
};

} // namespace pixcode::desktop

