#include "LoginDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include "../localization/LocalizationManager.h"

namespace pixcode::desktop {

LoginDialog::LoginDialog(std::shared_ptr<ApplicationContext> appContext, QWidget* parent)
    : QDialog(parent), m_appContext(std::move(appContext)) {
    setupUi();
    updateTranslations();

    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, &LoginDialog::updateTranslations);
}

void LoginDialog::setupUi() {
    setWindowTitle("Pix Code Academy - Login");
    setFixedSize(460, 480);
    setModal(true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(36, 32, 36, 32);
    mainLayout->setSpacing(16);

    // Header Language switcher
    auto* topRow = new QHBoxLayout();
    m_brandLabel = new QLabel("PIX CODE", this);
    m_brandLabel->setStyleSheet("font-weight: 800; font-size: 16px; color: #2E3A44; letter-spacing: 2px;");

    m_langButton = new QPushButton(this);
    m_langButton->setObjectName("secondaryButton");
    m_langButton->setFixedHeight(30);
    connect(m_langButton, &QPushButton::clicked, this, &LoginDialog::onToggleLanguageClicked);

    topRow->addWidget(m_brandLabel);
    topRow->addStretch();
    topRow->addWidget(m_langButton);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("headerTitle");
    m_titleLabel->setWordWrap(true);

    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setStyleSheet("color: #52606D; font-size: 13px;");
    m_subtitleLabel->setWordWrap(true);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("Username");

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("Password");

    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    connect(m_usernameEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);

    formLayout->addRow("Username:", m_usernameEdit);
    formLayout->addRow("Password:", m_passwordEdit);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName("errorLabel");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();

    m_loginButton = new QPushButton(this);
    m_loginButton->setMinimumHeight(44);
    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);

    mainLayout->addLayout(topRow);
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addWidget(m_subtitleLabel);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_errorLabel);
    mainLayout->addStretch();
    mainLayout->addWidget(m_loginButton);
}

void LoginDialog::updateTranslations() {
    m_titleLabel->setText(trText("login.title"));
    m_subtitleLabel->setText(trText("login.subtitle"));
    m_loginButton->setText(trText("login.submit"));
    m_langButton->setText(trText("login.language"));
}

void LoginDialog::onToggleLanguageClicked() {
    auto& loc = LocalizationManager::instance();
    if (loc.currentLanguage() == "ar") {
        loc.setLanguage("en");
    } else {
        loc.setLanguage("ar");
    }
}

void LoginDialog::onLoginClicked() {
    m_errorLabel->hide();

    const QString user = m_usernameEdit->text().trimmed();
    const QString pass = m_passwordEdit->text();

    if (user.isEmpty() || pass.isEmpty()) {
        m_errorLabel->setText(trText("login.error.empty"));
        m_errorLabel->show();
        return;
    }

    application::LoginCommand cmd{
        .username = user.toStdString(),
        .password = pass.toStdString()
    };

    m_loginButton->setEnabled(false);
    m_loginButton->setText("Verifying...");

    auto result = m_appContext->authenticateUseCase()->execute(cmd);

    m_loginButton->setEnabled(true);
    m_loginButton->setText(trText("login.submit"));

    if (result.isError()) {
        m_errorLabel->setText(QString::fromStdString(result.error().message()));
        m_errorLabel->show();
        return;
    }

    m_appContext->login(result.value());
    accept();
}

} // namespace pixcode::desktop

