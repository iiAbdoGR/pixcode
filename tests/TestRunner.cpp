#include <QtTest/QTest>
#include <iostream>
#include "domain/PasswordHasherTests.h"
#include "domain/UserEntityTests.h"
#include "domain/MoneyTests.h"
#include "domain/AcademicDomainTests.h"
#include "domain/AttendanceAndPayrollTests.h"
#include "application/AuthorizationTests.h"
#include "infrastructure/DatabaseMigrationTests.h"
#include "infrastructure/AuditLogTests.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    int status = 0;

    std::cout << "\n============================================\n";
    std::cout << "      Pix Code Academy Test Suite           \n";
    std::cout << "============================================\n\n";

    {
        PasswordHasherTests t;
        int s = QTest::qExec(&t, argc, argv);
        std::cout << "[Test] PasswordHasherTests exit: " << s << std::endl;
        status |= s;
    }
    {
        UserEntityTests t;
        int s = QTest::qExec(&t, argc, argv);
        std::cout << "[Test] UserEntityTests exit: " << s << std::endl;
        status |= s;
    }
    {
        MoneyTests t;
        int s = QTest::qExec(&t, argc, argv);
        std::cout << "[Test] MoneyTests exit: " << s << std::endl;
        status |= s;
    }
    {
        AcademicDomainTests t;
        int s = QTest::qExec(&t, argc, argv);
        std::cout << "[Test] AcademicDomainTests exit: " << s << std::endl;
        status |= s;
    }
    {
        AttendanceAndPayrollTests t;
        int s = QTest::qExec(&t, argc, argv);
        std::cout << "[Test] AttendanceAndPayrollTests exit: " << s << std::endl;
        status |= s;
    }
    {
        AuthorizationTests t;
        int s = QTest::qExec(&t, argc, argv);
        std::cout << "[Test] AuthorizationTests exit: " << s << std::endl;
        status |= s;
    }
    {
        DatabaseMigrationTests t;
        int s = QTest::qExec(&t, argc, argv);
        std::cout << "[Test] DatabaseMigrationTests exit: " << s << std::endl;
        status |= s;
    }
    {
        AuditLogTests t;
        int s = QTest::qExec(&t, argc, argv);
        std::cout << "[Test] AuditLogTests exit: " << s << std::endl;
        status |= s;
    }

    std::cout << "\nAll Tests Completed. Final Status: " << status << "\n";
    return status;
}
