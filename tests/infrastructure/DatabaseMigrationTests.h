#pragma once
#include <QtTest/QTest>

class DatabaseMigrationTests : public QObject {
    Q_OBJECT

private slots:
    void testPragmasAndMigrations();
    void testMigrationV004WithExistingDataAndForeignKeys();
};
