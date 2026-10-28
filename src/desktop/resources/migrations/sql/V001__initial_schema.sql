-- Migration V001: Initial Schema & RBAC Foundations

-- 1. Schema Migrations Ledger
CREATE TABLE IF NOT EXISTS schema_migrations (
    version INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    applied_at_utc TEXT NOT NULL,
    checksum TEXT NOT NULL
);

-- 2. Users Table
CREATE TABLE IF NOT EXISTS users (
    id TEXT PRIMARY KEY,
    username TEXT NOT NULL UNIQUE COLLATE NOCASE,
    password_hash TEXT NOT NULL,
    is_active INTEGER NOT NULL DEFAULT 1,
    must_change_password INTEGER NOT NULL DEFAULT 0,
    failed_login_count INTEGER NOT NULL DEFAULT 0,
    lockout_until_utc TEXT NULL,
    preferred_language TEXT NOT NULL DEFAULT 'ar',
    created_at_utc TEXT NOT NULL,
    updated_at_utc TEXT NOT NULL,
    last_login_at_utc TEXT NULL
);

-- 3. Roles Table
CREATE TABLE IF NOT EXISTS roles (
    id TEXT PRIMARY KEY,
    code TEXT NOT NULL UNIQUE,
    display_key TEXT NOT NULL,
    is_system_role INTEGER NOT NULL DEFAULT 0
);

-- 4. Permissions Table
CREATE TABLE IF NOT EXISTS permissions (
    id TEXT PRIMARY KEY,
    code TEXT NOT NULL UNIQUE,
    display_key TEXT NOT NULL,
    description_key TEXT NOT NULL
);

-- 5. User Roles Junction Table
CREATE TABLE IF NOT EXISTS user_roles (
    user_id TEXT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    role_id TEXT NOT NULL REFERENCES roles(id) ON DELETE RESTRICT,
    PRIMARY KEY (user_id, role_id)
);

-- 6. Role Permissions Junction Table
CREATE TABLE IF NOT EXISTS role_permissions (
    role_id TEXT NOT NULL REFERENCES roles(id) ON DELETE CASCADE,
    permission_id TEXT NOT NULL REFERENCES permissions(id) ON DELETE RESTRICT,
    PRIMARY KEY (role_id, permission_id)
);

-- 7. Audit Logs Table
CREATE TABLE IF NOT EXISTS audit_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    occurred_at_utc TEXT NOT NULL,
    actor_user_id TEXT NULL REFERENCES users(id) ON DELETE SET NULL,
    action TEXT NOT NULL,
    entity_type TEXT NOT NULL,
    entity_id TEXT NOT NULL,
    summary TEXT NOT NULL,
    correlation_id TEXT NULL,
    changes_json TEXT NULL
);

-- 8. App Settings Table
CREATE TABLE IF NOT EXISTS app_settings (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL,
    updated_at_utc TEXT NOT NULL,
    updated_by_user_id TEXT NULL REFERENCES users(id) ON DELETE SET NULL
);

-- 9. Backup Records Table
CREATE TABLE IF NOT EXISTS backup_records (
    id TEXT PRIMARY KEY,
    backup_path TEXT NOT NULL,
    created_at_utc TEXT NOT NULL,
    size_bytes INTEGER NOT NULL,
    sha256_checksum TEXT NOT NULL,
    status TEXT NOT NULL
);

-- Indexes for optimal lookup performance
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username COLLATE NOCASE);
CREATE INDEX IF NOT EXISTS idx_audit_logs_lookup ON audit_logs(entity_type, entity_id, occurred_at_utc);
CREATE INDEX IF NOT EXISTS idx_audit_logs_actor ON audit_logs(actor_user_id, occurred_at_utc);

-- SEED ROLES
INSERT OR IGNORE INTO roles (id, code, display_key, is_system_role) VALUES
('role_superadmin', 'SuperAdmin', 'role.superadmin', 1),
('role_admin', 'Admin', 'role.admin', 1),
('role_accountant', 'Accountant', 'role.accountant', 1),
('role_secretary', 'Secretary', 'role.secretary', 1),
('role_instructor', 'Instructor', 'role.instructor', 1);

-- SEED PERMISSIONS
INSERT OR IGNORE INTO permissions (id, code, display_key, description_key) VALUES
-- System
('p_sys_settings_read', 'system.settings.read', 'perm.sys.settings.read', 'desc.sys.settings.read'),
('p_sys_settings_upd', 'system.settings.update', 'perm.sys.settings.update', 'desc.sys.settings.update'),
('p_sys_users_manage', 'system.users.manage', 'perm.sys.users.manage', 'desc.sys.users.manage'),
('p_sys_audit_read', 'system.audit.read', 'perm.sys.audit.read', 'desc.sys.audit.read'),
('p_sys_backup_manage', 'system.backup.manage', 'perm.sys.backup.manage', 'desc.sys.backup.manage'),
-- Courses & Groups
('p_courses_read', 'courses.read', 'perm.courses.read', 'desc.courses.read'),
('p_courses_manage', 'courses.manage', 'perm.courses.manage', 'desc.courses.manage'),
('p_groups_read_all', 'groups.read.all', 'perm.groups.read.all', 'desc.groups.read.all'),
('p_groups_read_assigned', 'groups.read.assigned', 'perm.groups.read.assigned', 'desc.groups.read.assigned'),
('p_groups_manage', 'groups.manage', 'perm.groups.manage', 'desc.groups.manage'),
-- Students
('p_students_read', 'students.read', 'perm.students.read', 'desc.students.read'),
('p_students_create', 'students.create', 'perm.students.create', 'desc.students.create'),
('p_students_update', 'students.update', 'perm.students.update', 'desc.students.update'),
('p_students_sensitive', 'students.read_sensitive', 'perm.students.read_sensitive', 'desc.students.read_sensitive'),
-- Attendance
('p_att_student_mark', 'attendance.student.mark', 'perm.att.student.mark', 'desc.att.student.mark'),
('p_att_student_edit_past', 'attendance.student.edit_past', 'perm.att.student.edit_past', 'desc.att.student.edit_past'),
('p_att_inst_confirm', 'attendance.instructor.confirm', 'perm.att.inst.confirm', 'desc.att.inst.confirm'),
('p_att_inst_approve', 'attendance.instructor.approve', 'perm.att.inst.approve', 'desc.att.inst.approve'),
-- Finance
('p_fin_pay_read', 'finance.payments.read', 'perm.fin.pay.read', 'desc.fin.pay.read'),
('p_fin_pay_collect', 'finance.payments.collect', 'perm.fin.pay.collect', 'desc.fin.pay.collect'),
('p_fin_pay_reverse', 'finance.payments.reverse', 'perm.fin.pay.reverse', 'desc.fin.pay.reverse'),
('p_fin_disc_apply', 'finance.discounts.apply', 'perm.fin.disc.apply', 'desc.fin.disc.apply'),
('p_fin_disc_override', 'finance.discounts.override_limit', 'perm.fin.disc.override', 'desc.fin.disc.override'),
('p_fin_inst_manage', 'finance.installments.manage', 'perm.fin.inst.manage', 'desc.fin.inst.manage'),
('p_fin_overdue_read', 'finance.overdue.read', 'perm.fin.overdue.read', 'desc.fin.overdue.read'),
('p_fin_exp_read', 'finance.expenses.read', 'perm.fin.exp.read', 'desc.fin.exp.read'),
('p_fin_exp_manage', 'finance.expenses.manage', 'perm.fin.exp.manage', 'desc.fin.exp.manage'),
('p_fin_dash_read', 'finance.dashboard.read', 'perm.fin.dash.read', 'desc.fin.dash.read'),
('p_fin_rep_read', 'finance.reports.read', 'perm.fin.rep.read', 'desc.fin.rep.read'),
-- Payroll
('p_pay_read_all', 'payroll.read', 'perm.pay.read', 'desc.pay.read'),
('p_pay_calc_appr', 'payroll.calculate_approve', 'perm.pay.calc', 'desc.pay.calc'),
('p_pay_due_summary', 'payroll.read_due_summary', 'perm.pay.due_sum', 'desc.pay.due_sum'),
('p_pay_inst_detail', 'payroll.read_instructor_detail', 'perm.pay.inst_det', 'desc.pay.inst_det'),
('p_pay_inst_pay', 'payroll.pay_instructor', 'perm.pay.inst_pay', 'desc.pay.inst_pay'),
('p_inst_self_read', 'instructor.self.read', 'perm.inst.self.read', 'desc.inst.self.read');

-- GRANT PERMISSIONS TO ROLES
-- Admin (Operations by default)
INSERT OR IGNORE INTO role_permissions (role_id, permission_id) VALUES
('role_admin', 'p_sys_settings_read'),
('role_admin', 'p_sys_audit_read'),
('role_admin', 'p_sys_backup_manage'),
('role_admin', 'p_courses_read'),
('role_admin', 'p_courses_manage'),
('role_admin', 'p_groups_read_all'),
('role_admin', 'p_groups_manage'),
('role_admin', 'p_students_read'),
('role_admin', 'p_students_create'),
('role_admin', 'p_students_update'),
('role_admin', 'p_students_sensitive'),
('role_admin', 'p_att_student_mark'),
('role_admin', 'p_att_student_edit_past'),
('role_admin', 'p_att_inst_approve'),
('role_admin', 'p_fin_disc_apply'),
('role_admin', 'p_fin_disc_override');

-- Accountant (Finance, Expenses, Payroll, Reports)
INSERT OR IGNORE INTO role_permissions (role_id, permission_id) VALUES
('role_accountant', 'p_courses_read'),
('role_accountant', 'p_groups_read_all'),
('role_accountant', 'p_students_read'),
('role_accountant', 'p_fin_pay_read'),
('role_accountant', 'p_fin_pay_collect'),
('role_accountant', 'p_fin_pay_reverse'),
('role_accountant', 'p_fin_disc_apply'),
('role_accountant', 'p_fin_inst_manage'),
('role_accountant', 'p_fin_overdue_read'),
('role_accountant', 'p_fin_exp_read'),
('role_accountant', 'p_fin_exp_manage'),
('role_accountant', 'p_fin_dash_read'),
('role_accountant', 'p_fin_rep_read'),
('role_accountant', 'p_pay_read_all'),
('role_accountant', 'p_pay_calc_appr'),
('role_accountant', 'p_pay_due_summary'),
('role_accountant', 'p_pay_inst_detail'),
('role_accountant', 'p_pay_inst_pay');

-- Secretary (Registration, Groups, Student collection, Overdue follow-up)
INSERT OR IGNORE INTO role_permissions (role_id, permission_id) VALUES
('role_secretary', 'p_courses_read'),
('role_secretary', 'p_groups_read_all'),
('role_secretary', 'p_students_read'),
('role_secretary', 'p_students_create'),
('role_secretary', 'p_students_update'),
('role_secretary', 'p_att_student_mark'),
('role_secretary', 'p_fin_pay_read'),
('role_secretary', 'p_fin_pay_collect'),
('role_secretary', 'p_fin_disc_apply'),
('role_secretary', 'p_fin_inst_manage'),
('role_secretary', 'p_fin_overdue_read');

-- Instructor (Own groups and hours)
INSERT OR IGNORE INTO role_permissions (role_id, permission_id) VALUES
('role_instructor', 'p_courses_read'),
('role_instructor', 'p_groups_read_assigned'),
('role_instructor', 'p_att_student_mark'),
('role_instructor', 'p_att_inst_confirm'),
('role_instructor', 'p_inst_self_read');

