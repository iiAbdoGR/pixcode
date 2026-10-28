#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class UsersView : public QWidget {
    Q_OBJECT

public:
    explicit UsersView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

    void refresh();

private slots:
    void onRefreshClicked();
    void onAddStaffClicked();
    void onEditStaffClicked();
    void onResetPasswordClicked();
    void onUnlockSelectedUser();
    void onToggleUserActive();

private:
    void setupUi();
    void updateTranslations();

    std::shared_ptr<ApplicationContext> m_appContext;

    QLabel* m_titleLabel;
    QTableWidget* m_table;
    QPushButton* m_addStaffBtn;
    QPushButton* m_editStaffBtn;
    QPushButton* m_resetPassBtn;
    QPushButton* m_unlockButton;
    QPushButton* m_toggleActiveButton;
    QPushButton* m_refreshButton;
};

} // namespace pixcode::desktop
