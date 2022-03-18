#ifndef TESTHELPER_H
#define TESTHELPER_H

#include <QObject>
#include <QtTest/QTest>
#include "helper.h"

class TestHelper : public QObject
{
    Q_OBJECT

private slots:
    void timeRange_valid();
    void timeRange_inverted();
    void timeRange_intersects();
    void timeRange_contains();
    void timeRange_unite();
    void timeRange_unite_list();
    void timeRange_subtract();
    void timeRange_subtract_list();
    void timeRange_equalOperator();
    void timeSpan_hoursMinutes();
};

#endif // TESTHELPER_H
