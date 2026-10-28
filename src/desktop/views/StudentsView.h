#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class StudentsView : public QWidget {
    Q_OBJECT

public:
    explicit StudentsView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

    void refresh();

private slots:
    void onSearchChanged(const QString& text);
    void onStatusFilterChanged(int index);
    void onAddStudentClicked();
    void onRowDoubleClicked(int row, int column);
    void onViewDetailsClicked();
    void onEditStudentClicked();
    void onArchiveStudentClicked();

private:
    void setupUi();
    void updateTranslations();
    void loadStudents();

    std::shared_ptr<ApplicationContext> m_appContext;

    QLabel* m_titleLabel;
    QLineEdit* m_searchEdit;
    QComboBox* m_statusCombo;
    QPushButton* m_addButton;
    QPushButton* m_detailsButton;
    QPushButton* m_editButton;
    QPushButton* m_archiveButton;
    QPushButton* m_refreshButton;

    QTableWidget* m_table;
    QLabel* m_countLabel;
};

} // namespace pixcode::desktop
