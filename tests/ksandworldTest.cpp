#include "ksandworldTest.h"

void ksandworldTest::initTestCase()
{}

void ksandworldTest::init()
{}

void ksandworldTest::cleanup()
{}

void ksandworldTest::cleanupTestCase()
{}

void ksandworldTest::someTest()
{
    QCOMPARE(1,2);
}

QTEST_MAIN(ksandworldTest)

#include "ksandworldTest.moc"
