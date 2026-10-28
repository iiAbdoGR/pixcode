#include "SqliteRoleRepository.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

namespace pixcode::infrastructure {

SqliteRoleRepository::SqliteRoleRepository(std::shared_ptr<DatabaseManager> dbManager)
    : m_dbManager(std::move(dbManager)) {}

void SqliteRoleRepository::populateRolePermissions(domain::Role& role) {
    auto db = m_dbManager->getConnection();
    QSqlQuery permQuery(db);
    permQuery.prepare(
        "SELECT p.id, p.code, p.display_key, p.description_key "
        "FROM permissions p "
        "JOIN role_permissions rp ON p.id = rp.permission_id "
        "WHERE rp.role_id = :role_id;"
    );
    permQuery.bindValue(":role_id", QString::fromStdString(role.id()));

    if (!permQuery.exec()) return;

    while (permQuery.next()) {
        role.addPermission(domain::Permission(
            permQuery.value("id").toString().toStdString(),
            permQuery.value("code").toString().toStdString(),
            permQuery.value("display_key").toString().toStdString(),
            permQuery.value("description_key").toString().toStdString()
        ));
    }
}

std::optional<domain::Role> SqliteRoleRepository::findById(const std::string& id) {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);
    query.prepare("SELECT id, code, display_key, is_system_role FROM roles WHERE id = :id;");
    query.bindValue(":id", QString::fromStdString(id));

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    domain::Role role(
        query.value("id").toString().toStdString(),
        query.value("code").toString().toStdString(),
        query.value("display_key").toString().toStdString(),
        query.value("is_system_role").toBool()
    );

    populateRolePermissions(role);
    return role;
}

std::optional<domain::Role> SqliteRoleRepository::findByCode(const std::string& code) {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);
    query.prepare("SELECT id, code, display_key, is_system_role FROM roles WHERE code = :code;");
    query.bindValue(":code", QString::fromStdString(code));

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    domain::Role role(
        query.value("id").toString().toStdString(),
        query.value("code").toString().toStdString(),
        query.value("display_key").toString().toStdString(),
        query.value("is_system_role").toBool()
    );

    populateRolePermissions(role);
    return role;
}

std::vector<domain::Role> SqliteRoleRepository::findAll() {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);
    std::vector<domain::Role> roles;

    if (!query.exec("SELECT id, code, display_key, is_system_role FROM roles ORDER BY code ASC;")) {
        return roles;
    }

    while (query.next()) {
        domain::Role role(
            query.value("id").toString().toStdString(),
            query.value("code").toString().toStdString(),
            query.value("display_key").toString().toStdString(),
            query.value("is_system_role").toBool()
        );
        populateRolePermissions(role);
        roles.push_back(std::move(role));
    }

    return roles;
}

std::vector<domain::Role> SqliteRoleRepository::findRolesForUser(const std::string& userId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);
    std::vector<domain::Role> roles;

    query.prepare(
        "SELECT r.id, r.code, r.display_key, r.is_system_role "
        "FROM roles r "
        "JOIN user_roles ur ON r.id = ur.role_id "
        "WHERE ur.user_id = :user_id;"
    );
    query.bindValue(":user_id", QString::fromStdString(userId));

    if (!query.exec()) {
        return roles;
    }

    while (query.next()) {
        domain::Role role(
            query.value("id").toString().toStdString(),
            query.value("code").toString().toStdString(),
            query.value("display_key").toString().toStdString(),
            query.value("is_system_role").toBool()
        );
        populateRolePermissions(role);
        roles.push_back(std::move(role));
    }

    return roles;
}

} // namespace pixcode::infrastructure

