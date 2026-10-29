#pragma once

#include <string_view>

namespace pixcode::infrastructure {

inline constexpr std::string_view V001_InitialSchemaSql = R"SQL(-- Migration V001: Initial Schema & RBAC Foundations

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
('p_sys_settings_read', 'system.settings.read', 'perm.sys.settings.read', 'desc.sys.settings.read'),
('p_sys_settings_upd', 'system.settings.update', 'perm.sys.settings.update', 'desc.sys.settings.update'),
('p_sys_users_manage', 'system.users.manage', 'perm.sys.users.manage', 'desc.sys.users.manage'),
('p_sys_audit_read', 'system.audit.read', 'perm.sys.audit.read', 'desc.sys.audit.read'),
('p_sys_backup_manage', 'system.backup.manage', 'perm.sys.backup.manage', 'desc.sys.backup.manage'),
('p_courses_read', 'courses.read', 'perm.courses.read', 'desc.courses.read'),
('p_courses_manage', 'courses.manage', 'perm.courses.manage', 'desc.courses.manage'),
('p_groups_read_all', 'groups.read.all', 'perm.groups.read.all', 'desc.groups.read.all'),
('p_groups_read_assigned', 'groups.read.assigned', 'perm.groups.read.assigned', 'desc.groups.read.assigned'),
('p_groups_manage', 'groups.manage', 'perm.groups.manage', 'desc.groups.manage'),
('p_students_read', 'students.read', 'perm.students.read', 'desc.students.read'),
('p_students_create', 'students.create', 'perm.students.create', 'desc.students.create'),
('p_students_update', 'students.update', 'perm.students.update', 'desc.students.update'),
('p_students_sensitive', 'students.read_sensitive', 'perm.students.read_sensitive', 'desc.students.read_sensitive'),
('p_att_student_mark', 'attendance.student.mark', 'perm.att.student.mark', 'desc.att.student.mark'),
('p_att_student_edit_past', 'attendance.student.edit_past', 'perm.att.student.edit_past', 'desc.att.student.edit_past'),
('p_att_inst_confirm', 'attendance.instructor.confirm', 'perm.att.inst.confirm', 'desc.att.inst.confirm'),
('p_att_inst_approve', 'attendance.instructor.approve', 'perm.att.inst.approve', 'desc.att.inst.approve'),
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
('p_pay_read_all', 'payroll.read', 'perm.pay.read', 'desc.pay.read'),
('p_pay_calc_appr', 'payroll.calculate_approve', 'perm.pay.calc', 'desc.pay.calc'),
('p_pay_due_summary', 'payroll.read_due_summary', 'perm.pay.due_sum', 'desc.pay.due_sum'),
('p_pay_inst_detail', 'payroll.read_instructor_detail', 'perm.pay.inst_det', 'desc.pay.inst_det'),
('p_pay_inst_pay', 'payroll.pay_instructor', 'perm.pay.inst_pay', 'desc.pay.inst_pay'),
('p_inst_self_read', 'instructor.self.read', 'perm.inst.self.read', 'desc.inst.self.read');

-- GRANT PERMISSIONS TO ROLES
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

INSERT OR IGNORE INTO role_permissions (role_id, permission_id) VALUES
('role_instructor', 'p_courses_read'),
('role_instructor', 'p_groups_read_assigned'),
('role_instructor', 'p_att_student_mark'),
('role_instructor', 'p_att_inst_confirm'),
('role_instructor', 'p_inst_self_read');
)SQL";


inline constexpr std::string_view V002_AcademicSchemaSql = R"SQL(-- Migration V002: Academic and Financial Foundations

CREATE TABLE IF NOT EXISTS students (
    id                   INTEGER PRIMARY KEY AUTOINCREMENT,
    student_code         TEXT NOT NULL UNIQUE,
    full_name            TEXT NOT NULL,
    phone                TEXT,
    guardian_name        TEXT,
    guardian_phone       TEXT,
    address              TEXT,
    notes                TEXT,
    status               TEXT NOT NULL DEFAULT 'active' CHECK(status IN ('active','archived')),
    registered_at_utc    TEXT NOT NULL,
    updated_at_utc       TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_students_code   ON students(student_code);
CREATE INDEX IF NOT EXISTS idx_students_name   ON students(full_name);
CREATE INDEX IF NOT EXISTS idx_students_phone  ON students(phone);
CREATE INDEX IF NOT EXISTS idx_students_status ON students(status);

CREATE TABLE IF NOT EXISTS courses (
    id                   INTEGER PRIMARY KEY AUTOINCREMENT,
    name                 TEXT NOT NULL,
    description          TEXT,
    duration_weeks       INTEGER NOT NULL DEFAULT 0,
    default_fee_piastres INTEGER NOT NULL DEFAULT 0,
    status               TEXT NOT NULL DEFAULT 'active' CHECK(status IN ('active','inactive')),
    created_at_utc       TEXT NOT NULL,
    updated_at_utc       TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_courses_status ON courses(status);

CREATE TABLE IF NOT EXISTS instructors (
    id                   INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id              TEXT REFERENCES users(id) ON DELETE SET NULL,
    full_name            TEXT NOT NULL,
    phone                TEXT,
    notes                TEXT,
    status               TEXT NOT NULL DEFAULT 'active' CHECK(status IN ('active','inactive')),
    created_at_utc       TEXT NOT NULL,
    updated_at_utc       TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_instructors_status ON instructors(status);

CREATE TABLE IF NOT EXISTS groups (
    id                   INTEGER PRIMARY KEY AUTOINCREMENT,
    course_id            INTEGER NOT NULL REFERENCES courses(id) ON DELETE RESTRICT,
    instructor_id        INTEGER REFERENCES instructors(id) ON DELETE SET NULL,
    name                 TEXT NOT NULL,
    start_date           TEXT,
    end_date             TEXT,
    schedule_description TEXT,
    capacity             INTEGER NOT NULL DEFAULT 0,
    fee_piastres         INTEGER NOT NULL DEFAULT 0,
    status               TEXT NOT NULL DEFAULT 'open' CHECK(status IN ('open','full','closed','cancelled')),
    created_at_utc       TEXT NOT NULL,
    updated_at_utc       TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_groups_course     ON groups(course_id);
CREATE INDEX IF NOT EXISTS idx_groups_instructor ON groups(instructor_id);
CREATE INDEX IF NOT EXISTS idx_groups_status     ON groups(status);

CREATE TABLE IF NOT EXISTS enrollments (
    id                   INTEGER PRIMARY KEY AUTOINCREMENT,
    student_id           INTEGER NOT NULL REFERENCES students(id) ON DELETE RESTRICT,
    group_id             INTEGER NOT NULL REFERENCES groups(id) ON DELETE RESTRICT,
    agreed_fee_piastres  INTEGER NOT NULL DEFAULT 0,
    discount_piastres    INTEGER NOT NULL DEFAULT 0,
    net_fee_piastres     INTEGER NOT NULL DEFAULT 0,
    status               TEXT NOT NULL DEFAULT 'active' CHECK(status IN ('active','cancelled','transferred')),
    enrolled_at_utc      TEXT NOT NULL,
    cancelled_at_utc     TEXT,
    notes                TEXT,
    UNIQUE(student_id, group_id)
);

CREATE INDEX IF NOT EXISTS idx_enrollments_student ON enrollments(student_id);
CREATE INDEX IF NOT EXISTS idx_enrollments_group   ON enrollments(group_id);
CREATE INDEX IF NOT EXISTS idx_enrollments_status  ON enrollments(status);

CREATE TABLE IF NOT EXISTS receipt_counters (
    year    INTEGER PRIMARY KEY,
    last_no INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS payments (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    enrollment_id   INTEGER NOT NULL REFERENCES enrollments(id) ON DELETE RESTRICT,
    amount_piastres INTEGER NOT NULL CHECK(amount_piastres > 0),
    payment_method  TEXT NOT NULL DEFAULT 'cash' CHECK(payment_method IN ('cash','bank_transfer','other')),
    paid_at_utc     TEXT NOT NULL,
    receipt_number  TEXT NOT NULL UNIQUE,
    notes           TEXT,
    recorded_by     TEXT REFERENCES users(id) ON DELETE SET NULL,
    created_at_utc  TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_payments_enrollment ON payments(enrollment_id);
CREATE INDEX IF NOT EXISTS idx_payments_receipt    ON payments(receipt_number);
)SQL";

inline constexpr std::string_view V003_AttendanceAndPayrollSql = R"SQL(-- Migration V003: Staff, Instructor Payroll, Teaching Sessions & Attendance Management

-- 1. Extend instructors table with specialization and hourly rate
ALTER TABLE instructors ADD COLUMN specialization TEXT;
ALTER TABLE instructors ADD COLUMN hourly_rate_piastres INTEGER NOT NULL DEFAULT 0;

-- 2. Instructor Rate History (preserves historical compensation without retroactive alteration)
CREATE TABLE IF NOT EXISTS instructor_rate_history (
    id                   INTEGER PRIMARY KEY AUTOINCREMENT,
    instructor_id        INTEGER NOT NULL REFERENCES instructors(id) ON DELETE CASCADE,
    hourly_rate_piastres INTEGER NOT NULL CHECK(hourly_rate_piastres >= 0),
    effective_from_utc   TEXT NOT NULL,
    created_at_utc       TEXT NOT NULL,
    notes                TEXT
);

CREATE INDEX IF NOT EXISTS idx_rate_hist_inst ON instructor_rate_history(instructor_id);

-- 3. Teaching Sessions
CREATE TABLE IF NOT EXISTS sessions (
    id                          INTEGER PRIMARY KEY AUTOINCREMENT,
    group_id                    INTEGER NOT NULL REFERENCES groups(id) ON DELETE RESTRICT,
    instructor_id               INTEGER REFERENCES instructors(id) ON DELETE SET NULL,
    session_date                TEXT NOT NULL,          -- YYYY-MM-DD
    scheduled_start_time        TEXT,                   -- HH:MM
    scheduled_end_time          TEXT,                   -- HH:MM
    actual_start_time           TEXT,                   -- HH:MM
    actual_end_time             TEXT,                   -- HH:MM
    planned_duration_minutes    INTEGER NOT NULL DEFAULT 60,
    actual_duration_minutes     INTEGER NOT NULL DEFAULT 60,
    payable_duration_minutes    INTEGER NOT NULL DEFAULT 60,
    status                      TEXT NOT NULL DEFAULT 'scheduled' CHECK(status IN ('scheduled','completed','cancelled','rescheduled')),
    approval_status             TEXT NOT NULL DEFAULT 'pending' CHECK(approval_status IN ('pending','approved','rejected')),
    approved_by                 TEXT REFERENCES users(id) ON DELETE SET NULL,
    approved_at_utc             TEXT,
    hourly_rate_snapshot_piastres INTEGER NOT NULL DEFAULT 0,
    earned_amount_piastres      INTEGER NOT NULL DEFAULT 0,
    notes                       TEXT,
    created_at_utc              TEXT NOT NULL,
    updated_at_utc              TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_sessions_group  ON sessions(group_id);
CREATE INDEX IF NOT EXISTS idx_sessions_inst   ON sessions(instructor_id);
CREATE INDEX IF NOT EXISTS idx_sessions_date   ON sessions(session_date);
CREATE INDEX IF NOT EXISTS idx_sessions_status ON sessions(status);

-- 4. Session Student Attendance
CREATE TABLE IF NOT EXISTS session_attendance (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id     INTEGER NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,
    student_id     INTEGER NOT NULL REFERENCES students(id) ON DELETE RESTRICT,
    status         TEXT NOT NULL DEFAULT 'present' CHECK(status IN ('present','absent','late','excused')),
    arrival_time   TEXT,
    notes          TEXT,
    marked_by      TEXT REFERENCES users(id) ON DELETE SET NULL,
    marked_at_utc  TEXT NOT NULL,
    updated_at_utc TEXT NOT NULL,
    UNIQUE(session_id, student_id)
);

CREATE INDEX IF NOT EXISTS idx_att_session ON session_attendance(session_id);
CREATE INDEX IF NOT EXISTS idx_att_student ON session_attendance(student_id);
CREATE INDEX IF NOT EXISTS idx_att_status  ON session_attendance(status);

-- 5. Payout Counters (Atomic per-year payout receipts)
CREATE TABLE IF NOT EXISTS payout_counters (
    year    INTEGER PRIMARY KEY,
    last_no INTEGER NOT NULL DEFAULT 0
);

-- 6. Instructor Payouts
CREATE TABLE IF NOT EXISTS instructor_payouts (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    instructor_id    INTEGER NOT NULL REFERENCES instructors(id) ON DELETE RESTRICT,
    amount_piastres  INTEGER NOT NULL CHECK(amount_piastres > 0),
    payment_date_utc TEXT NOT NULL,
    payment_method   TEXT NOT NULL DEFAULT 'cash' CHECK(payment_method IN ('cash','bank_transfer','other')),
    reference_number TEXT,
    receipt_number   TEXT NOT NULL UNIQUE,
    notes            TEXT,
    created_by       TEXT REFERENCES users(id) ON DELETE SET NULL,
    approved_by      TEXT REFERENCES users(id) ON DELETE SET NULL,
    created_at_utc   TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_payouts_inst ON instructor_payouts(instructor_id);
CREATE INDEX IF NOT EXISTS idx_payouts_rcp  ON instructor_payouts(receipt_number);
)SQL";

inline constexpr std::string_view V004_BusinessIntegrityAndRoleConsolidationSql = R"SQL(-- Migration V004: Business Integrity, Role Consolidation (Owner & Admin Only) & Subscription Lifecycle

-- 1. Consolidate Roles to Owner and Admin Only
INSERT OR IGNORE INTO roles (id, code, display_key, is_system_role) VALUES
('role_owner', 'Owner', 'role.owner', 1),
('role_admin', 'Admin', 'role.admin', 1);

-- Re-map existing users with legacy roles
UPDATE user_roles SET role_id = 'role_owner' WHERE role_id = 'role_superadmin';
UPDATE user_roles SET role_id = 'role_admin' WHERE role_id IN ('role_accountant', 'role_secretary', 'role_instructor');

-- Delete legacy roles from role_permissions, user_roles, and roles
DELETE FROM role_permissions WHERE role_id IN ('role_accountant', 'role_secretary', 'role_instructor', 'role_superadmin');
DELETE FROM roles WHERE id IN ('role_accountant', 'role_secretary', 'role_instructor', 'role_superadmin');

-- Ensure permissions for Owner (all) and Admin (operational + permitted finance/payroll operations)
INSERT OR IGNORE INTO role_permissions (role_id, permission_id)
SELECT 'role_owner', id FROM permissions;

INSERT OR IGNORE INTO role_permissions (role_id, permission_id)
SELECT 'role_admin', id FROM permissions WHERE code IN (
    'students.read', 'students.create', 'students.update',
    'courses.read', 'groups.read.all', 'groups.read.assigned', 'groups.manage',
    'attendance.student.read', 'attendance.student.mark',
    'finance.payments.read', 'finance.payments.collect',
    'payroll.read', 'payroll.pay_instructor'
);

-- 2. Restructure enrollments table with audit metadata and partial unique index
DROP TABLE IF EXISTS enrollments_new;
CREATE TABLE IF NOT EXISTS enrollments_new (
    id                   INTEGER PRIMARY KEY AUTOINCREMENT,
    student_id           INTEGER NOT NULL REFERENCES students(id) ON DELETE RESTRICT,
    group_id             INTEGER NOT NULL REFERENCES groups(id) ON DELETE RESTRICT,
    agreed_fee_piastres  INTEGER NOT NULL DEFAULT 0,
    discount_piastres    INTEGER NOT NULL DEFAULT 0,
    net_fee_piastres     INTEGER NOT NULL DEFAULT 0,
    status               TEXT NOT NULL DEFAULT 'active' CHECK(status IN ('active','cancelled','transferred','completed')),
    enrolled_at_utc      TEXT NOT NULL,
    cancelled_at_utc     TEXT,
    cancellation_reason  TEXT,
    cancelled_by_user_id TEXT REFERENCES users(id) ON DELETE SET NULL,
    restored_at_utc      TEXT,
    restoration_reason   TEXT,
    restored_by_user_id  TEXT REFERENCES users(id) ON DELETE SET NULL,
    notes                TEXT
);

INSERT INTO enrollments_new (
    id, student_id, group_id, agreed_fee_piastres, discount_piastres, net_fee_piastres,
    status, enrolled_at_utc, cancelled_at_utc, notes
)
SELECT
    id, student_id, group_id, agreed_fee_piastres, discount_piastres, net_fee_piastres,
    status, enrolled_at_utc, cancelled_at_utc, notes
FROM enrollments;

DROP TABLE enrollments;
ALTER TABLE enrollments_new RENAME TO enrollments;

CREATE INDEX IF NOT EXISTS idx_enrollments_student ON enrollments(student_id);
CREATE INDEX IF NOT EXISTS idx_enrollments_group   ON enrollments(group_id);
CREATE INDEX IF NOT EXISTS idx_enrollments_status  ON enrollments(status);

-- Partial index strictly preventing duplicate ACTIVE enrollments for same student in same group
-- while permitting historical subscriptions (cancelled, completed, transferred)
CREATE UNIQUE INDEX IF NOT EXISTS idx_unique_active_enrollment 
ON enrollments(student_id, group_id) 
WHERE status = 'active';

-- 3. Extend instructors with compensation type and decouple from user logins
ALTER TABLE instructors ADD COLUMN compensation_type TEXT NOT NULL DEFAULT 'hourly';
ALTER TABLE instructors ADD COLUMN base_rate_piastres INTEGER NOT NULL DEFAULT 0;

ALTER TABLE instructor_rate_history ADD COLUMN compensation_type TEXT NOT NULL DEFAULT 'hourly';
)SQL";

inline constexpr std::string_view V005_InstructorContractsAndAdminApprovalSql = R"SQL(-- Migration V005: Instructor Contracts & Admin Session Approval Permissions

-- 1. Extend instructors table with contract dates and monthly salary
ALTER TABLE instructors ADD COLUMN contract_start_date TEXT;
ALTER TABLE instructors ADD COLUMN contract_end_date TEXT;
ALTER TABLE instructors ADD COLUMN monthly_salary_piastres INTEGER NOT NULL DEFAULT 0;

-- 2. Grant session approval and payroll calculation approval to Admin role
INSERT OR IGNORE INTO role_permissions (role_id, permission_id) VALUES
('role_admin', 'p_att_inst_approve'),
('role_admin', 'p_pay_calc_appr');
)SQL";

inline constexpr std::string_view V006_PayrollAllocationsAndRoleSecuritySql = R"SQL(-- Migration V006: Payroll Allocations, Salary Cycles and Role Security Consolidation

-- 1. Create table for Monthly Salary Periods
CREATE TABLE IF NOT EXISTS instructor_salary_periods (
    id                     INTEGER PRIMARY KEY AUTOINCREMENT,
    instructor_id          INTEGER NOT NULL REFERENCES instructors(id) ON DELETE RESTRICT,
    period_month_utc       TEXT NOT NULL,
    salary_amount_piastres INTEGER NOT NULL CHECK(salary_amount_piastres > 0),
    status                 TEXT NOT NULL DEFAULT 'pending' CHECK(status IN ('pending', 'approved', 'paid')),
    approved_at_utc        TEXT,
    approved_by_user_id    TEXT REFERENCES users(id) ON DELETE SET NULL,
    created_at_utc         TEXT NOT NULL,
    UNIQUE(instructor_id, period_month_utc)
);
CREATE INDEX IF NOT EXISTS idx_salary_periods_inst ON instructor_salary_periods(instructor_id);

-- 2. Add settlement_status to sessions
ALTER TABLE sessions ADD COLUMN settlement_status TEXT NOT NULL DEFAULT 'unpaid';
CREATE INDEX IF NOT EXISTS idx_sessions_settlement ON sessions(settlement_status);

-- 3. Create table for Payout Allocations
CREATE TABLE IF NOT EXISTS instructor_payout_allocations (
    id                        INTEGER PRIMARY KEY AUTOINCREMENT,
    payout_id                 INTEGER NOT NULL REFERENCES instructor_payouts(id) ON DELETE RESTRICT,
    session_id                INTEGER REFERENCES sessions(id) ON DELETE RESTRICT,
    salary_period_id          INTEGER REFERENCES instructor_salary_periods(id) ON DELETE RESTRICT,
    allocated_amount_piastres INTEGER NOT NULL CHECK(allocated_amount_piastres > 0),
    created_at_utc            TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_allocations_payout  ON instructor_payout_allocations(payout_id);
CREATE INDEX IF NOT EXISTS idx_allocations_session ON instructor_payout_allocations(session_id);

-- 4. Mark existing fully-paid historical sessions
UPDATE sessions 
SET settlement_status = 'fully_paid' 
WHERE approval_status = 'approved' 
  AND instructor_id IN (
      SELECT instructor_id 
      FROM instructor_payouts 
      GROUP BY instructor_id 
      HAVING SUM(amount_piastres) >= (
          SELECT COALESCE(SUM(earned_amount_piastres), 0) 
          FROM sessions s2 
          WHERE s2.instructor_id = instructor_payouts.instructor_id 
            AND s2.approval_status = 'approved'
      )
  );

-- 5. Revoke general finance and liability approval permissions from Admin role
DELETE FROM role_permissions 
WHERE role_id = 'role_admin' 
  AND permission_id IN (
      'p_fin_pay_read', 'p_fin_pay_collect', 'p_fin_pay_reverse',
      'p_fin_disc_apply', 'p_fin_disc_override', 'p_fin_inst_manage',
      'p_fin_overdue_read', 'p_fin_exp_read', 'p_fin_exp_manage',
      'p_fin_dash_read', 'p_fin_rep_read',
      'p_pay_calc_appr', 'p_att_inst_approve'
  );

-- Ensure Admin has restricted instructor payment execution
INSERT OR IGNORE INTO role_permissions (role_id, permission_id) VALUES
('role_admin', 'p_pay_inst_pay'),
('role_admin', 'p_pay_read_all');

-- Ensure Owner has all permissions
INSERT OR IGNORE INTO role_permissions (role_id, permission_id)
SELECT 'role_owner', id FROM permissions;
)SQL";
} // namespace pixcode::infrastructure
