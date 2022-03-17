#include "testhelper.h"

void TestHelper::timeRange_valid()
{
    QVERIFY(TimeRange(8,0, 8,30).isValid());
    // Inverted order of range but ok
    QVERIFY(TimeRange(12,0, 8,30).isValid());
    // Error: Invalid time
    QVERIFY(!TimeRange(-1, 2, 3, 0).isValid());
    QVERIFY(!TimeRange(QTime(), QTime(8, 0)).isValid());
    QVERIFY(!TimeRange(QTime(8, 0), QTime()).isValid());
}

void TestHelper::timeRange_inverted()
{
    QVERIFY(TimeRange(8,30, 8,0).isInverted());
    QVERIFY(!TimeRange(8,0, 8,30).isInverted());
    // Error: Invalid time
    QVERIFY(!TimeRange(QTime(), QTime(8, 0)).isInverted());
}

void TestHelper::timeRange_intersects()
{
    QVERIFY(TimeRange(8,0, 8,30).intersects(TimeRange(8,30, 9,0)));
    QVERIFY(TimeRange(8,0, 8,30).intersects(TimeRange(8,15, 9,0)));
    QVERIFY(TimeRange(8,0, 8,30).intersects(TimeRange(7,15, 8,0)));
    QVERIFY(TimeRange(8,0, 8,30).intersects(TimeRange(7,15, 8,15)));
    QVERIFY(!TimeRange(8,0, 8,30).intersects(TimeRange(9,30, 10,0)));
    // Error: Invalid time ranges
    QVERIFY(!TimeRange().intersects(TimeRange(9,30, 10,0)));
    QVERIFY(!TimeRange(8,0, 8,30).intersects(TimeRange()));
}

void TestHelper::timeRange_contains()
{
    QVERIFY(TimeRange(8,0, 8,30).contains(TimeRange(8,0, 8,10)));
    QVERIFY(TimeRange(8,0, 8,30).contains(TimeRange(8,10, 8,30)));
    QVERIFY(TimeRange(8,0, 8,30).contains(TimeRange(8,0, 8,30)));
    // Intersects but not contains at all
    QVERIFY(!TimeRange(8,0, 8,30).contains(TimeRange(8,15, 9,30)));
    QVERIFY(!TimeRange(8,0, 8,30).contains(TimeRange(7,15, 8,30)));
    // Error: Invalid time ranges
    QVERIFY(!TimeRange(8,0, 8,30).contains(TimeRange()));
    QVERIFY(!TimeRange().contains(TimeRange(7,15, 8,30)));
}

void TestHelper::timeRange_unite()
{
    auto r1 = TimeRange::unite(TimeRange(8,0, 8,30), TimeRange(8,30, 9,30));
    QCOMPARE(r1.size(), 1);
    QCOMPARE(r1[0], TimeRange(8,0, 9,30));

    auto r2 = TimeRange::unite(TimeRange(8,0, 8,30), TimeRange(8,10, 9,30));
    QCOMPARE(r2.size(), 1);
    QCOMPARE(r2[0], TimeRange(8,0, 9,30));

    auto r3 = TimeRange::unite(TimeRange(8,0, 8,30), TimeRange(8,10, 8,20));
    QCOMPARE(r3.size(), 1);
    QCOMPARE(r3[0], TimeRange(8,0, 8,30));

    // Inverted order of getUnion parameters
    auto r3_ = TimeRange::unite(TimeRange(8,10, 8,20), TimeRange(8,0, 8,30));
    QCOMPARE(r3_.size(), 1);
    QCOMPARE(r3_[0], TimeRange(8,0, 8,30));

    auto r4 = TimeRange::unite(TimeRange(8,0, 8,30), TimeRange(8,45, 9,30));
    QCOMPARE(r4.size(), 2);
    QCOMPARE(r4[0], TimeRange(8,0, 8,30));
    QCOMPARE(r4[1], TimeRange(8,45, 9,30));

    auto r5 = TimeRange::unite(TimeRange(8,0, 8,30), TimeRange(8,0, 8,30));
    QCOMPARE(r5.size(), 1);
    QCOMPARE(r5[0], TimeRange(8,0, 8,30));

    auto r6 = TimeRange::unite(TimeRange(), TimeRange(8,0, 8,30));
    QCOMPARE(r6.size(), 1);
    QCOMPARE(r6[0], TimeRange(8,0, 8,30));

    auto r7 = TimeRange::unite(TimeRange(8,0, 8,30), TimeRange());
    QCOMPARE(r7.size(), 1);
    QCOMPARE(r7[0], TimeRange(8,0, 8,30));

    auto r8 = TimeRange::unite(TimeRange(), TimeRange());
    QCOMPARE(r8.size(), 0);
}

void TestHelper::timeRange_unite_list()
{
    auto l1 = QList<TimeRange>();
    auto r1 = TimeRange::unite(l1);
    QCOMPARE(r1.size(), 0);


    auto l2 = QList<TimeRange>({TimeRange(8,0, 8,30)});
    auto r2 = TimeRange::unite(l2);
    QCOMPARE(r2.size(), 1);
    QCOMPARE(r2[0], TimeRange(8,0, 8,30));


    auto l3 = QList<TimeRange>({TimeRange(8,0, 8,30), TimeRange(8,20, 8,40)});
    auto r3 = TimeRange::unite(l3);
    QCOMPARE(r3.size(), 1);
    QCOMPARE(r3[0], TimeRange(8,0, 8,40));


    auto l4 = QList<TimeRange>({ TimeRange(8,20, 8,40),
                                 TimeRange(8,0, 8,30),
                                 TimeRange(8,35, 8,50) });
    auto r4 = TimeRange::unite(l4);

    QCOMPARE(r4.size(), 1);
    QCOMPARE(r4[0], TimeRange(8,0, 8,50));


    auto l5 = QList<TimeRange>({ TimeRange(8,0, 8,30),
                                 TimeRange(8,10, 8,20),
                                 TimeRange(8,35, 8,50) });
    auto r5 = TimeRange::unite(l5);
    QCOMPARE(r5.size(), 2);
    QCOMPARE(r5[0], TimeRange(8,0, 8,30));
    QCOMPARE(r5[1], TimeRange(8,35, 8,50));


    auto l6 = QList<TimeRange>({ TimeRange(8,0, 8,30),
                                 TimeRange(8,31, 8,35),
                                 TimeRange(8,37, 8,50) });
    auto r6 = TimeRange::unite(l6);
    QCOMPARE(r6, l6);


    auto l7 = QList<TimeRange>({ TimeRange(8,0, 8,30),
                                 TimeRange(),
                                 TimeRange(8,25, 8,50) });
    auto r7 = TimeRange::unite(l7);
    QCOMPARE(r7.size(), 1);
    QCOMPARE(r7[0], TimeRange(8,0, 8,50));
}

void TestHelper::timeRange_equalOperator()
{
    QVERIFY(TimeRange(8,30, 9,30) == TimeRange(8,30, 9,30));
    QVERIFY(TimeRange(8,30, 9,30) == TimeRange(QTime(8, 30), QTime(9, 30)));
    QVERIFY(TimeRange() == TimeRange());
    QVERIFY(TimeRange() == TimeRange(QTime(), QTime()));
    QVERIFY(!(TimeRange(8,30, 9,30) == TimeRange(9,30, 10,30)));
    QVERIFY(!(TimeRange(8,30, 9,30) == TimeRange()));
}

void TestHelper::timeSpan_hoursMinutes()
{
    TimeSpan t1(QTime(10,0), QTime(11, 30));
    QCOMPARE(t1.hours(), 1);
    QCOMPARE(t1.minutes(), 30);

    TimeSpan t2(QTime(11, 30), QTime(10, 0));
    QCOMPARE(t2.hours(), -1);
    QCOMPARE(t2.minutes(), -30);
}
