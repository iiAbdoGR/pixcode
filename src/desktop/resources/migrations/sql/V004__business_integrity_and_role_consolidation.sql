-- Migration V004: Business Integrity, Role Consolidation (Owner & Admin Only) & Subscription Lifecycle

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

