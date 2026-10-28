#include "SqliteUserRepository.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <stdexcept>

namespace pixcode::infrastructure {

SqliteUserRepository::SqliteUserRepository(std::shared_ptr<DatabaseManager> dbManager)
    : m_dbManager(std::move(dbManager)) {}

domain::User SqliteUserRepository::mapRowToUser(const QSqlQuery& query) {
    const std::string id = query.value("id").toString().toStdString();
    const std::string username = query.value("username").toString().toStdString();
    const std::string passwordHash = query.value("password_hash").toString().toStdString();
    const bool isActive = query.value("is_active").toBool();
    const bool mustChangePassword = query.value("must_change_password").toBool();
    const int failedLoginCount = query.value("failed_login_count").toInt();

    std::optional<std::string> lockoutUntilUtc;
    if (!query.value("lockout_until_utc").isNull()) {
        lockoutUntilUtc = query.value("lockout_until_utc").toString().toStdString();
    }

    const std::string preferredLanguage = query.value("preferred_language").toString().toStdString();
    const std::string createdAtUtc = query.value("created_at_utc").toString().toStdString();
    const std::string updatedAtUtc = query.value("updated_at_utc").toString().toStdString();

    std::optional<std::string> lastLoginAtUtc;
    if (!query.value("last_login_at_utc").isNull()) {
        lastLoginAtUtc = query.value("last_login_at_utc").toString().toStdString();
    }

    domain::User user(id, username, passwordHash, isActive, mustChangePassword,
                      failedLoginCount, lockoutUntilUtc, preferredLanguage,
                      createdAtUtc, updatedAtUtc, lastLoginAtUtc);

    populateUserRoles(user);
    return user;
}

void SqliteUserRepository::populateUserRoles(domain::User& user) {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);
    query.prepare(
        "SELECT r.id, r.code, r.display_key, r.is_system_role "
        "FROM roles r "
        "JOIN user_roles ur ON r.id = ur.role_id "
        "WHERE ur.user_id = :user_id;"
    );
    query.bindValue(":user_id", QString::fromStdString(user.id()));

    if (!query.exec()) return;

    while (query.next()) {
        const std::string roleId = query.value("id").toString().toStdString();
        const std::string code = query.value("code").toString().toStdString();
        const std::string displayKey = query.value("display_key").toString().toStdString();
        const bool isSystem = query.value("is_system_role").toBool();

        domain::Role role(roleId, code, displayKey, isSystem);

        // Fetch permissions for this role
        QSqlQuery permQuery(db);
        permQuery.prepare(
            "SELECT p.id, p.code, p.display_key, p.description_key "
            "FROM permissions p "
            "JOIN role_permissions rp ON p.id = rp.permission_id "
            "WHERE rp.role_id = :role_id;"
        );
        permQuery.bindValue(":role_id", QString::fromStdString(roleId));
        if (permQuery.exec()) {
            while (permQuery.next()) {
                role.addPermission(domain::Permission(
                    permQuery.value("id").toString().toStdString(),
                    permQuery.value("code").toString().toStdString(),
                    permQuery.value("display_key").toString().toStdString(),
                    permQuery.value("description_key").toString().toStdString()
                ));
            }
        }

        user.addRole(std::move(role));
    }
}

std::optional<domain::User> SqliteUserRepository::findById(const std::string& id) {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM users WHERE id = :id;");
    query.bindValue(":id", QString::fromStdString(id));

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return mapRowToUser(query);
}

std::optional<domain::User> SqliteUserRepository::findByUsername(const std::string& username) {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM users WHERE username = :username COLLATE NOCASE;");
    query.bindValue(":username", QString::fromStdString(username));

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return mapRowToUser(query);
}

std::vector<domain::User> SqliteUserRepository::findAll() {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);
    std::vector<domain::User> users;

    if (!query.exec("SELECT * FROM users ORDER BY username COLLATE NOCASE ASC;")) {
        return users;
    }

    while (query.next()) {
        users.push_back(mapRowToUser(query));
    }

    return users;
}

int SqliteUserRepository::countActiveSuperAdmins() {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);
    const QString sql =
        "SELECT COUNT(DISTINCT u.id) "
        "FROM users u "
        "JOIN user_roles ur ON u.id = ur.user_id "
        "JOIN roles r ON ur.role_id = r.id "
        "WHERE r.code = 'SuperAdmin' AND u.is_active = 1;";

    if (query.exec(sql) && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int SqliteUserRepository::countTotalUsers() {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);
    if (query.exec("SELECT COUNT(*) FROM users;") && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

void SqliteUserRepository::save(const domain::User& user) {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);

    query.prepare(
        "INSERT INTO users (id, username, password_hash, is_active, must_change_password, "
        "                   failed_login_count, lockout_until_utc, preferred_language, "
        "                   created_at_utc, updated_at_utc, last_login_at_utc) "
        "VALUES (:id, :username, :password_hash, :is_active, :must_change_password, "
        "        :failed_login_count, :lockout_until_utc, :preferred_language, "
        "        :created_at_utc, :updated_at_utc, :last_login_at_utc) "
        "ON CONFLICT(id) DO UPDATE SET "
        "  username = excluded.username, "
        "  password_hash = excluded.password_hash, "
        "  is_active = excluded.is_active, "
        "  must_change_password = excluded.must_change_password, "
        "  failed_login_count = excluded.failed_login_count, "
        "  lockout_until_utc = excluded.lockout_until_utc, "
        "  preferred_language = excluded.preferred_language, "
        "  updated_at_utc = excluded.updated_at_utc, "
        "  last_login_at_utc = excluded.last_login_at_utc;"
    );

    query.bindValue(":id", QString::fromStdString(user.id()));
    query.bindValue(":username", QString::fromStdString(user.username()));
    query.bindValue(":password_hash", QString::fromStdString(user.passwordHash()));
    query.bindValue(":is_active", user.isActive() ? 1 : 0);
    query.bindValue(":must_change_password", user.mustChangePassword() ? 1 : 0);
    query.bindValue(":failed_login_count", user.failedLoginCount());

    if (user.lockoutUntilUtc().has_value()) {
        query.bindValue(":lockout_until_utc", QString::fromStdString(*user.lockoutUntilUtc()));
    } else {
        query.bindValue(":lockout_until_utc", QVariant(QMetaType(QMetaType::QString)));
    }

    query.bindValue(":preferred_language", QString::fromStdString(user.preferredLanguage()));
    query.bindValue(":created_at_utc", QString::fromStdString(user.createdAtUtc()));
    query.bindValue(":updated_at_utc", QString::fromStdString(user.updatedAtUtc()));

    if (user.lastLoginAtUtc().has_value()) {
        query.bindValue(":last_login_at_utc", QString::fromStdString(*user.lastLoginAtUtc()));
    } else {
        query.bindValue(":last_login_at_utc", QVariant(QMetaType(QMetaType::QString)));
    }

    if (!query.exec()) {
        throw std::runtime_error("Failed to save user: " + query.lastError().text().toStdString());
    }
}

void SqliteUserRepository::assignRole(const std::string& userId, const std::string& roleId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);
    query.prepare("INSERT OR IGNORE INTO user_roles (user_id, role_id) VALUES (:user_id, :role_id);");
    query.bindValue(":user_id", QString::fromStdString(userId));
    query.bindValue(":role_id", QString::fromStdString(roleId));

    if (!query.exec()) {
        throw std::runtime_error("Failed to assign role: " + query.lastError().text().toStdString());
    }
}

void SqliteUserRepository::removeRole(const std::string& userId, const std::string& roleId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);
    query.prepare("DELETE FROM user_roles WHERE user_id = :user_id AND role_id = :role_id;");
    query.bindValue(":user_id", QString::fromStdString(userId));
    query.bindValue(":role_id", QString::fromStdString(roleId));

    if (!query.exec()) {
        throw std::runtime_error("Failed to remove role: " + query.lastError().text().toStdString());
    }
}

} // namespace pixcode::infrastructure

