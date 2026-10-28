-- Migration V002: Academic and Financial Foundations

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
