-- Migration V006: Payroll Allocations, Salary Cycles and Role Security Consolidation

-- 1. Create table for Monthly Salary Periods
CREATE TABLE IF NOT EXISTS instructor_salary_periods (
    id                     INTEGER PRIMARY KEY AUTOINCREMENT,
    instructor_id          INTEGER NOT NULL REFERENCES instructors(id) ON DELETE RESTRICT,
    period_month_utc       TEXT NOT NULL, -- YYYY-MM
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

