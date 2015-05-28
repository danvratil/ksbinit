#ifndef ksandworldTEST_H
#define ksandworldTEST_H

#include <QtCore/QObject>
#include <QtTest/QTest>

class ksandworldTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();
    void someTest();
};

#endif // ksandworldTEST_H
