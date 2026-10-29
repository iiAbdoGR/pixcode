#include "LocalizationManager.h"
#include <QtWidgets/QApplication>
#include <QtCore/QSettings>

namespace pixcode::desktop {

LocalizationManager::LocalizationManager()
    : m_currentLanguage("ar") {
    QSettings settings("PixCode", "Academy");
    QString saved = settings.value("app_language", "ar").toString();
    if (!saved.isEmpty() && (saved == "ar" || saved == "en")) {
        m_currentLanguage = saved;
    }
    initDictionary();
}

LocalizationManager& LocalizationManager::instance() {
    static LocalizationManager s_instance;
    return s_instance;
}

void LocalizationManager::setLanguage(const QString& languageCode) {
    if (m_currentLanguage == languageCode) return;

    m_currentLanguage = languageCode;

    QSettings settings("PixCode", "Academy");
    settings.setValue("app_language", m_currentLanguage);

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
    en["nav.instructors"] = "Instructors";
    en["nav.attendance"] = "Attendance";
    en["nav.finance"] = "Finance";
    en["nav.payroll"] = "Instructor Payroll";
    en["nav.users"] = "Staff & Users";
    en["nav.audit"] = "Audit Logs";
    en["nav.backup"] = "Settings & Backup";

    en["header.logout"] = "Sign Out";
    en["header.autolock"] = "Auto-Lock: %1s";
    en["header.switch_lang"] = "العربية";

    // Common actions
    en["common.save"] = "Save";
    en["common.cancel"] = "Cancel";
    en["common.refresh"] = "Refresh";
    en["common.edit"] = "Edit";
    en["common.delete"] = "Delete";
    en["common.search"] = "Search...";
    en["common.all"] = "All";
    en["common.status"] = "Status";
    en["common.notes"] = "Notes";
    en["common.date"] = "Date";
    en["common.actions"] = "Actions";

    // Users / Staff
    en["users.title"] = "Staff Accounts & User Management";
    en["users.add_staff"] = "+ Add Staff Member";
    en["users.edit_staff"] = "Edit Account & Roles";
    en["users.reset_password"] = "Reset Password";
    en["users.col.username"] = "Username";
    en["users.col.role"] = "Roles";
    en["users.col.status"] = "Status";
    en["users.col.failed"] = "Failed Logins";
    en["users.col.locked"] = "Lockout Status";
    en["users.col.last_login"] = "Last Login (UTC)";
    en["users.status.active"] = "Active";
    en["users.status.disabled"] = "Disabled";
    en["users.status.locked"] = "Locked Out";

    // Attendance
    en["attendance.title"] = "Student Attendance Management";
    en["attendance.select_group"] = "Select Group:";
    en["attendance.select_session"] = "Select Session:";
    en["attendance.schedule_session"] = "+ Schedule Session";
    en["attendance.mark_all_present"] = "Mark All Present";
    en["attendance.save"] = "Save Attendance";
    en["attendance.col.student_name"] = "Student Name";
    en["attendance.col.phone"] = "Phone";
    en["attendance.col.status"] = "Attendance Status";
    en["attendance.col.arrival_time"] = "Arrival Time";
    en["attendance.col.notes"] = "Notes";
    en["attendance.status.present"] = "Present";
    en["attendance.status.absent"] = "Absent";
    en["attendance.status.late"] = "Late";
    en["attendance.status.excused"] = "Excused";
    en["attendance.stats.summary"] = "Total: %1 | Present: %2 | Absent: %3 | Late: %4 | Excused: %5 | Rate: %6%";

    // Payroll
    en["payroll.title"] = "Instructor Hourly Compensation & Payroll";
    en["payroll.select_instructor"] = "Select Instructor:";
    en["payroll.hourly_rate"] = "Hourly Rate: %1 EGP";
    en["payroll.update_rate"] = "Update Rate";
    en["payroll.card.sessions"] = "Completed Sessions";
    en["payroll.card.hours"] = "Approved Hours";
    en["payroll.card.earned"] = "Gross Earnings";
    en["payroll.card.paid"] = "Total Paid";
    en["payroll.card.due"] = "Balance Due";
    en["payroll.record_payout"] = "+ Issue Payout";
    en["payroll.approve_session"] = "Approve Session";
    en["payroll.tab.sessions"] = "Teaching Sessions";
    en["payroll.tab.payouts"] = "Payout History";
    en["payroll.col.session_date"] = "Date";
    en["payroll.col.group"] = "Group";
    en["payroll.col.duration"] = "Minutes";
    en["payroll.col.session_status"] = "Status";
    en["payroll.col.approval"] = "Approval";
    en["payroll.col.earned"] = "Earned (EGP)";
    en["payroll.col.payout_receipt"] = "Receipt #";
    en["payroll.col.payout_amount"] = "Amount (EGP)";
    en["payroll.col.payout_method"] = "Method";
    en["payroll.col.payout_date"] = "Date";

    // Audit
    en["audit.title"] = "System Security & Audit Trail";
    en["audit.col.time"] = "Occurred At (UTC)";
    en["audit.col.actor"] = "User / Actor";
    en["audit.col.action"] = "Action";
    en["audit.col.type"] = "Entity Type";
    en["audit.col.summary"] = "Summary";

    // Roles
    en["role.owner"] = "Owner (Full access & financials)";
    en["role.admin"] = "Admin (Operational duties)";

    // Dashboard
    en["dash.welcome_sub"] = "Pix Code Academy Management Hub";
    en["dash.section.operational"] = "Academic Operations";
    en["dash.active_students"] = "Active Students";
    en["dash.active_groups"] = "Open Study Groups";
    en["dash.today_sessions"] = "Today's Sessions";
    en["dash.att_rate"] = "Overall Attendance Rate";
    en["dash.section.financial"] = "Financial Overview & KPIs (Authorized Only)";
    en["dash.total_income"] = "Total Collected Income";
    en["dash.receivables"] = "Outstanding Student Dues";
    en["dash.inst_payables"] = "Instructor Payables (Approved)";
    en["dash.net_cash"] = "Net Operational Cash Balance";

    // Dialogs: Cancel & Restore Subscription
    en["dialog.cancel_subscription.title"] = "Cancel Subscription";
    en["dialog.cancel_subscription.heading"] = "Confirm Subscription Cancellation";
    en["dialog.cancel_subscription.warning"] = "Warning: Cancelling releases the reserved seat in the group while preserving all historical payment records intact.";
    en["dialog.cancel_subscription.reason"] = "Reason:";
    en["dialog.cancel_subscription.reason_ph"] = "State the reason for cancellation...";
    en["dialog.cancel_subscription.reason_req"] = "Please enter a cancellation reason.";
    en["dialog.cancel_subscription.confirm_btn"] = "Cancel Subscription";

    en["dialog.restore_subscription.title"] = "Restore Subscription";
    en["dialog.restore_subscription.heading"] = "Restore Cancelled Subscription";
    en["dialog.restore_subscription.capacity_info"] = "Group Capacity: %1 / %2 active seats filled.";
    en["dialog.restore_subscription.reason"] = "Reason:";
    en["dialog.restore_subscription.reason_ph"] = "State the reason for restoring the subscription...";
    en["dialog.restore_subscription.reason_req"] = "Please enter a restoration reason.";
    en["dialog.restore_subscription.confirm_btn"] = "Restore Subscription";

    // Dialogs: Instructor
    en["dialog.instructor.edit_title"] = "Edit Instructor";
    en["dialog.instructor.create_title"] = "Add Instructor";
    en["dialog.instructor.name"] = "Full Name:";
    en["dialog.instructor.phone"] = "Phone Number:";
    en["dialog.instructor.spec"] = "Specialization:";
    en["dialog.instructor.comp_type"] = "Compensation Mode:";
    en["instructor.comp.hourly"] = "Hourly Rate";
    en["instructor.comp.per_session"] = "Per Session";
    en["instructor.comp.monthly_salary"] = "Monthly Salary";
    en["instructor.comp.fixed_contract"] = "Fixed Contract";
    en["dialog.instructor.base_rate"] = "Rate / Per Session / Hourly (EGP):";
    en["dialog.instructor.monthly_salary"] = "Monthly Salary (EGP):";
    en["dialog.instructor.contract_start"] = "Contract Start Date:";
    en["dialog.instructor.contract_end"] = "Contract End Date:";
    en["dialog.instructor.status"] = "Status:";
    en["dialog.instructor.notes"] = "Notes:";
    en["dialog.instructor.name_req"] = "Instructor name is required.";
    en["payroll.summary.monthly_salary"] = "Monthly Salary: %1 EGP";
    en["payroll.summary.per_session"] = "Per Session: %1 EGP";
    en["payroll.summary.hourly"] = "Hourly Rate: %1 EGP";
    en["payroll.summary.fixed_contract"] = "Fixed Contract: %1 EGP";

    // Dialogs: Staff
    en["dialog.staff.edit_title"] = "Edit User Account";
    en["dialog.staff.create_title"] = "Add User Account";
    en["dialog.staff.username"] = "Username:";
    en["dialog.staff.password"] = "Initial Password:";
    en["dialog.staff.role"] = "System Role:";
    en["dialog.staff.active"] = "Account Active";
    en["dialog.staff.user_short"] = "Username must be at least 3 characters.";
    en["dialog.staff.pass_short"] = "Password must be at least 8 characters.";

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
    ar["nav.instructors"] = "المحاضرون";
    ar["nav.attendance"] = "تسجيل الحضور";
    ar["nav.finance"] = "المالية والمقبوضات";
    ar["nav.payroll"] = "مستحقات المدربين";
    ar["nav.users"] = "الموظفون والمستخدمون";
    ar["nav.audit"] = "سجل التدقيق والأمان";
    ar["nav.backup"] = "الإعدادات والنسخ الاحتياطي";

    ar["header.logout"] = "تسجيل الخروج";
    ar["header.autolock"] = "القفل التلقائي: %1ث";
    ar["header.switch_lang"] = "English";

    // Common actions
    ar["common.save"] = "حفظ";
    ar["common.cancel"] = "إلغاء";
    ar["common.refresh"] = "تحديث";
    ar["common.edit"] = "تعديل";
    ar["common.delete"] = "حذف";
    ar["common.search"] = "بحث...";
    ar["common.all"] = "الكل";
    ar["common.status"] = "الحالة";
    ar["common.notes"] = "ملاحظات";
    ar["common.date"] = "التاريخ";
    ar["common.actions"] = "الإجراءات";

    // Users / Staff
    ar["users.title"] = "إدارة حسابات الموظفين والمستخدمين والصلاحيات";
    ar["users.add_staff"] = "+ إضافة موظف جديد";
    ar["users.edit_staff"] = "تعديل الحساب والأدوار";
    ar["users.reset_password"] = "إعادة تعيين كلمة المرور";
    ar["users.col.username"] = "اسم المستخدم";
    ar["users.col.role"] = "الأدوار والصلاحيات";
    ar["users.col.status"] = "الحالة";
    ar["users.col.failed"] = "محاولات فاشلة";
    ar["users.col.locked"] = "حالة القفل";
    ar["users.col.last_login"] = "آخر تسجيل دخول (UTC)";
    ar["users.status.active"] = "نشط";
    ar["users.status.disabled"] = "معطل";
    ar["users.status.locked"] = "مقفل مؤقتًا";

    // Attendance
    ar["attendance.title"] = "إدارة دفاتر حضور وغياب الطلاب";
    ar["attendance.select_group"] = "اختر المجموعة:";
    ar["attendance.select_session"] = "اختر الحصة / الجلسة:";
    ar["attendance.schedule_session"] = "+ إضافة موعد حصة";
    ar["attendance.mark_all_present"] = "تحديد الكل حاضر";
    ar["attendance.save"] = "حفظ دفتر الحضور";
    ar["attendance.col.student_name"] = "اسم الطالب";
    ar["attendance.col.phone"] = "رقم الهاتف";
    ar["attendance.col.status"] = "حالة الحضور";
    ar["attendance.col.arrival_time"] = "وقت الوصول";
    ar["attendance.col.notes"] = "ملاحظات";
    ar["attendance.status.present"] = "حاضر";
    ar["attendance.status.absent"] = "غائب";
    ar["attendance.status.late"] = "متأخر";
    ar["attendance.status.excused"] = "معذور";
    ar["attendance.stats.summary"] = "الإجمالي: %1 | حاضر: %2 | غائب: %3 | متأخر: %4 | معذور: %5 | نسبة الالتزام: %6%";

    // Payroll
    ar["payroll.title"] = "مستحقات وساعات المدربين وصرف الأجور";
    ar["payroll.select_instructor"] = "اختر المدرب:";
    ar["payroll.hourly_rate"] = "سعر الساعة: %1 ج.م";
    ar["payroll.update_rate"] = "تعديل السعر";
    ar["payroll.card.sessions"] = "الحصص المنفذة";
    ar["payroll.card.hours"] = "الساعات المعتمدة";
    ar["payroll.card.earned"] = "إجمالي المستحقات";
    ar["payroll.card.paid"] = "المدفوع للمدرب";
    ar["payroll.card.due"] = "الرصيد المتبقي له";
    ar["payroll.record_payout"] = "+ صرف دفعة للمدرب";
    ar["payroll.approve_session"] = "اعتماد الحصة";
    ar["payroll.tab.sessions"] = "سجل الحصص التدريسية";
    ar["payroll.tab.payouts"] = "سجل الإيصالات والمدفوعات";
    ar["payroll.col.session_date"] = "التاريخ";
    ar["payroll.col.group"] = "المجموعة";
    ar["payroll.col.duration"] = "الدقائق";
    ar["payroll.col.session_status"] = "حالة الحصة";
    ar["payroll.col.approval"] = "الاعتماد الإداري";
    ar["payroll.col.earned"] = "الأجر (ج.م)";
    ar["payroll.col.payout_receipt"] = "رقم إيصال الصرف";
    ar["payroll.col.payout_amount"] = "المبلغ (ج.م)";
    ar["payroll.col.payout_method"] = "طريقة الدفع";
    ar["payroll.col.payout_date"] = "تاريخ الصرف";

    // Audit
    ar["audit.title"] = "سجل العمليات والرقابة الأمنية (Audit Trail)";
    ar["audit.col.time"] = "التوقيت (UTC)";
    ar["audit.col.actor"] = "المستخدم المنفذ";
    ar["audit.col.action"] = "العملية";
    ar["audit.col.type"] = "نوع الكيان";
    ar["audit.col.summary"] = "ملخص البيان";

    // Roles
    ar["role.owner"] = "مالك الأكاديمية (صلاحيات كاملة ومالية)";
    ar["role.admin"] = "مسؤول النظام (مهام تشغيلية)";

    // Dashboard
    ar["dash.welcome_sub"] = "منظومة إدارة أكاديمية بيكس كود";
    ar["dash.section.operational"] = "المؤشرات والعمليات الأكاديمية";
    ar["dash.active_students"] = "الطلاب النشطون";
    ar["dash.active_groups"] = "المجموعات المفتوحة";
    ar["dash.today_sessions"] = "حصص اليوم المجدولة";
    ar["dash.att_rate"] = "نسبة الحضور الإجمالية";
    ar["dash.section.financial"] = "المؤشرات والتدفقات المالية (للمصرح لهم فقط)";
    ar["dash.total_income"] = "إجمالي المتحصلات المقبوضة";
    ar["dash.receivables"] = "الرسوم والمستحقات المتبقية للطلاب";
    ar["dash.inst_payables"] = "مستحقات المدربين المعتمدة";
    ar["dash.net_cash"] = "صافي الرصيد النقدي الفعلي";

    // Dialogs: Cancel & Restore Subscription
    ar["dialog.cancel_subscription.title"] = "إلغاء اشتراك طالب";
    ar["dialog.cancel_subscription.heading"] = "تأكيد إلغاء اشتراك الطالب";
    ar["dialog.cancel_subscription.warning"] = "تنبيه: يؤدي الإلغاء إلى تفريغ المقعد بالمجموعة مع الحفاظ الكامل على كافة الإيصالات والمدفوعات السابقة بسجل الطالب دون تغيير.";
    ar["dialog.cancel_subscription.reason"] = "سبب الإلغاء:";
    ar["dialog.cancel_subscription.reason_ph"] = "اكتب سبب الإلغاء بالتفصيل...";
    ar["dialog.cancel_subscription.reason_req"] = "يرجى إدخال سبب الإلغاء.";
    ar["dialog.cancel_subscription.confirm_btn"] = "تأكيد الإلغاء";

    ar["dialog.restore_subscription.title"] = "استعادة اشتراك ملغي";
    ar["dialog.restore_subscription.heading"] = "استعادة اشتراك الطالب الملغي";
    ar["dialog.restore_subscription.capacity_info"] = "سعة المجموعة: تم شغل %1 من أصل %2 مقعد.";
    ar["dialog.restore_subscription.reason"] = "سبب الاستعادة:";
    ar["dialog.restore_subscription.reason_ph"] = "اكتب سبب استعادة الاشتراك بالتفصيل...";
    ar["dialog.restore_subscription.reason_req"] = "يرجى إدخال سبب الاستعادة.";
    ar["dialog.restore_subscription.confirm_btn"] = "استعادة الاشتراك";

    // Dialogs: Instructor
    ar["dialog.instructor.edit_title"] = "تعديل بيانات المحاضر";
    ar["dialog.instructor.create_title"] = "إضافة محاضر جديد";
    ar["dialog.instructor.name"] = "اسم المحاضر:";
    ar["dialog.instructor.phone"] = "رقم الهاتف:";
    ar["dialog.instructor.spec"] = "التخصص التدريبي:";
    ar["dialog.instructor.comp_type"] = "نظام الاستحقاق المالي:";
    ar["instructor.comp.hourly"] = "بالساعة التدريبية";
    ar["instructor.comp.per_session"] = "بالحصة المنفذة";
    ar["instructor.comp.monthly_salary"] = "راتب شهري";
    ar["instructor.comp.fixed_contract"] = "عقد شهري ثابت";
    ar["dialog.instructor.base_rate"] = "سعر الحصة / الساعة (ج.م):";
    ar["dialog.instructor.monthly_salary"] = "الراتب الشهري (ج.م):";
    ar["dialog.instructor.contract_start"] = "تاريخ بداية العقد:";
    ar["dialog.instructor.contract_end"] = "تاريخ نهاية العقد:";
    ar["dialog.instructor.status"] = "الحالة:";
    ar["dialog.instructor.notes"] = "ملاحظات:";
    ar["dialog.instructor.name_req"] = "اسم المحاضر مطلوب.";
    ar["payroll.summary.monthly_salary"] = "راتب شهري: %1 ج.م";
    ar["payroll.summary.per_session"] = "سعر الحصة: %1 ج.م";
    ar["payroll.summary.hourly"] = "سعر الساعة: %1 ج.م";
    ar["payroll.summary.fixed_contract"] = "عقد شهري: %1 ج.م";

    // Dialogs: Staff
    ar["dialog.staff.edit_title"] = "تعديل حساب المستخدم";
    ar["dialog.staff.create_title"] = "إضافة مستخدم جديد";
    ar["dialog.staff.username"] = "اسم المستخدم:";
    ar["dialog.staff.password"] = "كلمة المرور الأولية:";
    ar["dialog.staff.role"] = "الدور بالنظام:";
    ar["dialog.staff.active"] = "الحساب نشط";
    ar["dialog.staff.user_short"] = "اسم المستخدم يجب ألا يقل عن 3 أحرف.";
    ar["dialog.staff.pass_short"] = "كلمة المرور يجب ألا تقل عن 8 أحرف.";
}

} // namespace pixcode::desktop
