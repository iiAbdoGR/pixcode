-- Migration V003: Staff, Instructor Payroll, Teaching Sessions & Attendance Management

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

