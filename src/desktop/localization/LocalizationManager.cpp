#include "LocalizationManager.h"
#include <QtWidgets/QApplication>

namespace pixcode::desktop {

LocalizationManager::LocalizationManager()
    : m_currentLanguage("ar") {
    initDictionary();
}

LocalizationManager& LocalizationManager::instance() {
    static LocalizationManager s_instance;
    return s_instance;
}

void LocalizationManager::setLanguage(const QString& languageCode) {
    if (m_currentLanguage == languageCode) return;

    m_currentLanguage = languageCode;

    if (m_currentLanguage == "ar") {
        qApp->setLayoutDirection(Qt::RightToLeft);
    } else {
        qApp->setLayoutDirection(Qt::LeftToRight);
    }

    emit languageChanged(m_currentLanguage);
}

QString LocalizationManager::translate(const QString& key) const {
    const std::string lang = m_currentLanguage.toStdString();
    const std::string k = key.toStdString();

    auto langIt = m_dictionary.find(lang);
    if (langIt != m_dictionary.end()) {
        auto keyIt = langIt->second.find(k);
        if (keyIt != langIt->second.end()) {
            return keyIt->second;
        }
    }

    // Fallback to English if not in current
    if (lang != "en") {
        auto enIt = m_dictionary.find("en");
        if (enIt != m_dictionary.end()) {
            auto keyIt = enIt->second.find(k);
            if (keyIt != enIt->second.end()) {
                return keyIt->second;
            }
        }
    }

    return key;
}

void LocalizationManager::initDictionary() {
    // English
    auto& en = m_dictionary["en"];
    en["app.title"] = "Pix Code Academy Management System";
    en["app.tagline"] = "Desktop Academy Administration";
    en["login.title"] = "Sign In to Pix Code";
    en["login.subtitle"] = "Enter your credentials to access the management portal";
    en["login.username"] = "Username";
    en["login.password"] = "Password";
    en["login.submit"] = "Sign In";
    en["login.language"] = "العربية";
    en["login.locked"] = "Account temporarily locked. Please try again in %1 minutes.";
    en["login.error.empty"] = "Please enter both username and password.";

    en["wizard.title"] = "Welcome to Pix Code Academy";
    en["wizard.subtitle"] = "First-Run Initial Setup: Create the System Administrator Account";
    en["wizard.admin_user"] = "Administrator Username";
    en["wizard.admin_pass"] = "Password (min 8 chars with upper, lower & numbers)";
    en["wizard.admin_confirm"] = "Confirm Password";
    en["wizard.select_lang"] = "Default Academy Language";
    en["wizard.submit"] = "Initialize System";

    en["nav.dashboard"] = "Dashboard";
    en["nav.registration"] = "Registration";
    en["nav.students"] = "Students";
    en["nav.courses"] = "Courses & Groups";
    en["nav.attendance"] = "Attendance";
    en["nav.finance"] = "Finance";
    en["nav.payroll"] = "Payroll";
    en["nav.users"] = "Users & Roles";
    en["nav.audit"] = "Audit Logs";
    en["nav.backup"] = "Settings & Backup";

    en["header.logout"] = "Sign Out";
    en["header.autolock"] = "Auto-Lock: %1s";
    en["header.switch_lang"] = "العربية";

    en["users.title"] = "System User Management";
    en["users.col.username"] = "Username";
    en["users.col.role"] = "Role";
    en["users.col.status"] = "Status";
    en["users.col.failed"] = "Failed Logins";
    en["users.col.locked"] = "Lockout Status";
    en["users.col.last_login"] = "Last Login (UTC)";
    en["users.status.active"] = "Active";
    en["users.status.disabled"] = "Disabled";
    en["users.status.locked"] = "Locked Out";

    en["audit.title"] = "System Security & Audit Trail";
    en["audit.col.time"] = "Occurred At (UTC)";
    en["audit.col.actor"] = "User / Actor";
    en["audit.col.action"] = "Action";
    en["audit.col.type"] = "Entity Type";
    en["audit.col.summary"] = "Summary";

    // Arabic
    auto& ar = m_dictionary["ar"];
    ar["app.title"] = "نظام إدارة أكاديمية بيكس كود";
    ar["app.tagline"] = "نظام الإدارة المكتبي الموحد";
    ar["login.title"] = "تسجيل الدخول إلى بيكس كود";
    ar["login.subtitle"] = "أدخل بيانات الاعتماد الخاصة بك للوصول إلى لوحة التحكم";
    ar["login.username"] = "اسم المستخدم";
    ar["login.password"] = "كلمة المرور";
    ar["login.submit"] = "تسجيل الدخول";
    ar["login.language"] = "English";
    ar["login.locked"] = "تم قفل الحساب مؤقتًا. يُرجى المحاولة بعد %1 دقيقة.";
    ar["login.error.empty"] = "يُرجى إدخال اسم المستخدم وكلمة المرور.";

    ar["wizard.title"] = "مرحبًا بك في أكاديمية بيكس كود";
    ar["wizard.subtitle"] = "الإعداد لأول مرة: إنشاء حساب مدير النظام (SuperAdmin)";
    ar["wizard.admin_user"] = "اسم مستخدم المدير";
    ar["wizard.admin_pass"] = "كلمة المرور (8 أحرف على الأقل تحتوي حروف كبيرة وصغيرة وأرقام)";
    ar["wizard.admin_confirm"] = "تأكيد كلمة المرور";
    ar["wizard.select_lang"] = "لغة النظام الافتراضية";
    ar["wizard.submit"] = "تهيئة النظام والبدء";

    ar["nav.dashboard"] = "لوحة التحكم";
    ar["nav.registration"] = "التسجيل السريع";
    ar["nav.students"] = "الطلاب";
    ar["nav.courses"] = "المجموعات والدورات";
    ar["nav.attendance"] = "تسجيل الحضور";
    ar["nav.finance"] = "المالية والمقبوضات";
    ar["nav.payroll"] = "مستحقات المدربين";
    ar["nav.users"] = "المستخدمون والصلاحيات";
    ar["nav.audit"] = "سجل التدقيق والأمان";
    ar["nav.backup"] = "الإعدادات والنسخ الاحتياطي";

    ar["header.logout"] = "تسجيل الخروج";
    ar["header.autolock"] = "القفل التلقائي: %1ث";
    ar["header.switch_lang"] = "English";

    ar["users.title"] = "إدارة مستخدمي النظام والصلاحيات";
    ar["users.col.username"] = "اسم المستخدم";
    ar["users.col.role"] = "الدور الوظيفي";
    ar["users.col.status"] = "الحالة";
    ar["users.col.failed"] = "محاولات فاشلة";
    ar["users.col.locked"] = "حالة القفل";
    ar["users.col.last_login"] = "آخر تسجيل دخول (UTC)";
    ar["users.status.active"] = "نشط";
    ar["users.status.disabled"] = "معطل";
    ar["users.status.locked"] = "مقفل مؤقتًا";

    ar["audit.title"] = "سجل العمليات والرقابة الأمنية (Audit Trail)";
    ar["audit.col.time"] = "التوقيت (UTC)";
    ar["audit.col.actor"] = "المستخدم المنفذ";
    ar["audit.col.action"] = "العملية";
    ar["audit.col.type"] = "نوع الكيان";
    ar["audit.col.summary"] = "ملخص البيان";
}

} // namespace pixcode::desktop

