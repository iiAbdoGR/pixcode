#include "MoneyTests.h"
#include "domain/value_objects/Money.h"

void MoneyTests::testBasicArithmetic() {
    auto m1 = pixcode::domain::Money::fromEgp(100);
    auto m2 = pixcode::domain::Money::fromPiasters(550);

    auto sum = m1 + m2;
    QCOMPARE(sum.piasters(), 10550);
    QCOMPARE(sum.wholeEgp(), 105);
    QCOMPARE(sum.fractionalPiasters(), 50);

    auto diff = m1 - m2;
    QCOMPARE(diff.piasters(), 9450);
    QCOMPARE(diff.format(), "94.50 EGP");
}

void MoneyTests::testInstallmentRemainderPolicy() {
    auto total = pixcode::domain::Money::fromEgp(1000);
    auto parts = total.splitEqually(3);

    QCOMPARE(static_cast<int>(parts.size()), 3);
    QCOMPARE(parts[0].piasters(), 33334);
    QCOMPARE(parts[1].piasters(), 33333);
    QCOMPARE(parts[2].piasters(), 33333);

    auto sum = parts[0] + parts[1] + parts[2];
    QCOMPARE(sum.piasters(), total.piasters());
}
