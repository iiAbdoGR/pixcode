-- Migration V005: Instructor Contracts & Admin Session Approval Permissions

-- 1. Extend instructors table with contract dates and monthly salary
ALTER TABLE instructors ADD COLUMN contract_start_date TEXT;
ALTER TABLE instructors ADD COLUMN contract_end_date TEXT;
ALTER TABLE instructors ADD COLUMN monthly_salary_piastres INTEGER NOT NULL DEFAULT 0;

-- 2. Grant session approval and payroll calculation approval to Admin role
INSERT OR IGNORE INTO role_permissions (role_id, permission_id) VALUES
('role_admin', 'p_att_inst_approve'),
('role_admin', 'p_pay_calc_appr');

