#include "FirstRunWizardDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"

namespace pixcode::desktop {

FirstRunWizardDialog::FirstRunWizardDialog(std::shared_ptr<ApplicationContext> appContext, QWidget* parent)
    : QDialog(parent), m_appContext(std::move(appContext)) {
    setupUi();
    updateTranslations();

    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, &FirstRunWizardDialog::updateTranslations);
}

void FirstRunWizardDialog::setupUi() {
    setWindowTitle("Pix Code Academy - Setup");
    setFixedSize(540, 480);
    setModal(true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(20);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("headerTitle");
    m_titleLabel->setWordWrap(true);

    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setStyleSheet("color: #52606D; font-size: 13px;");
    m_subtitleLabel->setWordWrap(true);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(14);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("e.g. admin");

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_confirmPasswordEdit = new QLineEdit(this);
    m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);

    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItem("العربية (Arabic)", "ar");
    m_languageCombo->addItem("English", "en");

    connect(m_languageCombo, &QComboBox::currentIndexChanged, this, [this](int idx) {
        const QString lang = m_languageCombo->itemData(idx).toString();
        LocalizationManager::instance().setLanguage(lang);
    });

    formLayout->addRow("Username:", m_usernameEdit);
    formLayout->addRow("Password:", m_passwordEdit);
    formLayout->addRow("Confirm Password:", m_confirmPasswordEdit);
    formLayout->addRow("Language:", m_languageCombo);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName("errorLabel");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();

    m_submitButton = new QPushButton(this);
    m_submitButton->setMinimumHeight(44);
    connect(m_submitButton, &QPushButton::clicked, this, &FirstRunWizardDialog::onSubmitClicked);

    mainLayout->addWidget(m_titleLabel);
    mainLayout->addWidget(m_subtitleLabel);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_errorLabel);
    mainLayout->addStretch();
    mainLayout->addWidget(m_submitButton);
}

void FirstRunWizardDialog::updateTranslations() {
    m_titleLabel->setText(trText("wizard.title"));
    m_subtitleLabel->setText(trText("wizard.subtitle"));
    m_submitButton->setText(trText("wizard.submit"));
}

void FirstRunWizardDialog::onSubmitClicked() {
    m_errorLabel->hide();

    application::FirstRunSetupCommand cmd{
        .username = m_usernameEdit->text().trimmed().toStdString(),
        .password = m_passwordEdit->text().toStdString(),
        .confirmPassword = m_confirmPasswordEdit->text().toStdString(),
        .preferredLanguage = m_languageCombo->currentData().toString().toStdString()
    };

    auto result = m_appContext->firstRunUseCase()->execute(cmd);

    if (result.isError()) {
        m_errorLabel->setText(QString::fromStdString(result.error().message()));
        m_errorLabel->show();
        return;
    }

    QMessageBox::information(this, "Success",
                             LocalizationManager::instance().isRtl()
                                 ? "تم تهيئة النظام بنجاح وإنشاء حساب المشرف! يمكنك الآن تسجيل الدخول."
                                 : "System initialized successfully! You may now sign in.");
    accept();
}

} // namespace pixcode::desktop

