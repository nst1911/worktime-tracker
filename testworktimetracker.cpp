#include "testworktimetracker.h"

WorktimeTracker TestWorktimeTracker::example(const QSqlDatabase &db)
{
    WorktimeTracker wt(db);

    wt.insertSchedule("testschedule1", QTime(5, 25), QTime(9, 49));

    wt.insertRecord(QDate(2022, 01, 18), QTime(9, 0), QTime(17, 0));

    for (int i = 1; i < 60; i++)
        wt.insertRecord(QDate(2022, 01, 18).addDays(i), QTime(8, 0), QTime(17, 0));

    wt.insertRecord(QDate(2022, 01, 18).addDays(60), QTime(8, 0), QTime(18, 0));

    wt.insertLeavePass(QTime(10, 0), QTime(10, 15), QDate(2022, 01, 19), "lp0");
    wt.insertLeavePass(QTime(10, 25), QTime(10, 40), QDate(2022, 01, 19), "lp1");
    wt.insertLeavePass(QTime(10, 55), QTime(10, 45), QDate(2022, 01, 19), "lpf");
    wt.insertLeavePass(QTime(8, 0), QTime(9, 35), QDate(2022, 02, 20), "lp2");
    wt.insertLeavePass(QTime(16, 20), QTime(17, 00), QDate(2022, 03, 15), "lp3");

    return wt;
}

void TestWorktimeTracker::unite()
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

void TestWorktimeTracker::unite_list()
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

void TestWorktimeTracker::insertSchedule()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    QVERIFY(wt.insertSchedule("test1", QTime(9, 0), QTime(11, 0)));
    QVERIFY(!wt.insertSchedule("test1", QTime(9, 0), QTime(11, 0)));    // Error: The same as previous
    QVERIFY(!wt.insertSchedule("test2", QTime(12, 0), QTime(11, 0)));   // Error: begin > end
    QVERIFY(!wt.insertSchedule("test3", QTime(), QTime(10, 0)));        // Error: invalid begin
    QVERIFY(!wt.insertSchedule("test3", QTime(10, 0), QTime()));        // Error: invalid end
    QVERIFY(!wt.insertSchedule("test4", QTime(10, 0), QTime(10, 0)));   // Error: begin == end
    QVERIFY(wt.insertSchedule("test5", QTime(9, 30), QTime(11, 30)));

    QSqlQuery q("SELECT * FROM schedule", db);

    q.next();
    QCOMPARE(q.value(0).toString(), wt.defaultSchedule().name);
    QCOMPARE(q.value(1).toTime(), wt.defaultSchedule().begin);
    QCOMPARE(q.value(2).toTime(), wt.defaultSchedule().end);

    q.next();
    QCOMPARE(q.value(0).toString(), QString("test1"));
    QCOMPARE(q.value(1).toTime(), QTime(9, 0));
    QCOMPARE(q.value(2).toTime(), QTime(11, 0));

    q.next();
    QCOMPARE(q.value(0).toString(), QString("test5"));
    QCOMPARE(q.value(1).toTime(), QTime(9, 30));
    QCOMPARE(q.value(2).toTime(), QTime(11, 30));

    clear(&db);
}

void TestWorktimeTracker::getRecord()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    wt.insertRecord(QDate(1996, 11, 26));
    wt.insertRecord(QDate(1996, 11, 29));

    auto r1 = wt.getRecord(QDate(1996, 11, 26));
    auto r2 = wt.getRecord(QDate(1996, 11, 29));
    auto r3 = wt.getRecord(QDate());            // Error: Invalid date
    auto r4 = wt.getRecord(QDate(1996, 2, 2));  // Error: Date does not exist in table

    QVERIFY(r1.isValid());
    QCOMPARE(r1.date, QDate(1996, 11, 26));
    QCOMPARE(r1.schedule.name, wt.defaultSchedule().name);
    QCOMPARE(r1.arrivalTime, wt.defaultSchedule().begin);
    QCOMPARE(r1.leavingTime, wt.defaultSchedule().end);

    QVERIFY(r2.isValid());
    QCOMPARE(r2.date, QDate(1996, 11, 29));
    QCOMPARE(r2.schedule.name, wt.defaultSchedule().name);
    QCOMPARE(r2.arrivalTime, wt.defaultSchedule().begin);
    QCOMPARE(r2.leavingTime, wt.defaultSchedule().end);

    QVERIFY(!r3.isValid());
    QVERIFY(!r4.isValid());
}

void TestWorktimeTracker::getRecords()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    auto d = QDate(1996, 11, 26);

    for (int i = 0; i < 5; ++i)
        wt.insertRecord(d.addDays(i));

    auto r1 = wt.getRecords(QDate(1996, 11, 26), QDate(1996, 11, 29));
    auto r2 = wt.getRecords(QDate(1996, 11, 20), QDate(1996, 11, 28));
    auto r3 = wt.getRecords(QDate(1996, 11, 29), QDate(1996, 12, 31));
    auto r4 = wt.getRecords(QDate(1996, 11, 26), QDate(1996, 11, 26));
    // Dates don't exist in table
    auto r5 = wt.getRecords(QDate(2000, 01, 01), QDate(3000, 01, 01));
    // Invalid dates
    auto r6 = wt.getRecords(QDate(), QDate(3000, 01, 01));
    auto r7 = wt.getRecords(QDate(2000, 01, 01), QDate());

    QVERIFY(r1.size() == 4);
    for (int i = 0; i < 4; ++i)
        QVERIFY(r1[i].isValid() && r1[i].date == d.addDays(i));

    QVERIFY(r2.size() == 3);
    for (int i = 0; i < 3; ++i)
        QVERIFY(r2[i].isValid() && r2[i].date == d.addDays(i));

    QVERIFY(r3.size() == 2);
    QVERIFY(r3[0].isValid() && r3[0].date == QDate(1996, 11, 29));
    QVERIFY(r3[1].isValid() && r3[1].date == QDate(1996, 11, 30));

    QVERIFY(r4.size() == 1);
    QVERIFY(r4[0].isValid() && r4[0].date == d);

    QVERIFY(r5.isEmpty());
    QVERIFY(r6.isEmpty());
    QVERIFY(r7.isEmpty());
}

void TestWorktimeTracker::insertRecord()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    wt.insertSchedule("test", QTime(10, 0), QTime(12, 0));

    QVERIFY(wt.insertRecord(QDate(1996, 11, 26)));
    QVERIFY(!wt.insertRecord(QDate(1996, 11, 26))); // Error: The same as previous
    QVERIFY(wt.insertRecord(QDate(2222, 01, 01), QTime(10, 0), QTime(15, 0)));
    QVERIFY(wt.insertRecord(QDate(2222, 01, 02), QTime(10, 0), QTime(15, 0), "test"));
    QVERIFY(!wt.insertRecord(QDate(), QTime(10, 0), QTime(11, 0))); // Error: Invalid date
    QVERIFY(!wt.insertRecord(QDate(1996, 11, 27), QTime(), QTime(15, 0))); // Error: Invalid begin
    QVERIFY(!wt.insertRecord(QDate(1996, 11, 28), QTime(15, 0), QTime())); // Error: Invalid end
    QVERIFY(!wt.insertRecord(QDate(1996, 11, 29), QTime(15, 0), QTime(15, 0))); // Error: begin == end
    QVERIFY(!wt.insertRecord(QDate(1996, 11, 29), QTime(16, 0), QTime(15, 0))); // Error: begin > end
    QVERIFY(!wt.insertRecord(QDate(1995, 4, 26), QTime(0, 0), QTime(5, 0), "tt")); // Error: Invalid schedule

    QSqlQuery q("SELECT * FROM worktime", db);

    q.next();
    QCOMPARE(q.value(0).toDate(), QDate(1996, 11, 26));
    QCOMPARE(q.value(1).toString(), wt.defaultSchedule().name);
    QCOMPARE(q.value(2).toTime(), wt.defaultSchedule().begin);
    QCOMPARE(q.value(3).toTime(), wt.defaultSchedule().end);

    q.next();
    QCOMPARE(q.value(0).toDate(), QDate(2222, 01, 01));
    QCOMPARE(q.value(1).toString(), wt.defaultSchedule().name);
    QCOMPARE(q.value(2).toTime(), QTime(10, 0));
    QCOMPARE(q.value(3).toTime(), QTime(15, 0));

    q.next();
    QCOMPARE(q.value(0).toDate(), QDate(2222, 01, 02));
    QCOMPARE(q.value(1).toString(), "test");
    QCOMPARE(q.value(2).toTime(), QTime(10, 0));
    QCOMPARE(q.value(3).toTime(), QTime(15, 0));

    clear(&db);
}

void TestWorktimeTracker::getScheduleBeforeDate()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    wt.insertSchedule("test", QTime(10, 0), QTime(12, 0));

    wt.insertRecord(QDate::currentDate(), QTime(8, 0), QTime(15, 0), "test");
    wt.insertRecord(QDate::currentDate().addDays(1));

    auto sch1 = wt.getScheduleBeforeDate(QDate::currentDate().addDays(1));
    QCOMPARE(sch1.name, "test");
    QCOMPARE(sch1.begin, QTime(10, 0));
    QCOMPARE(sch1.end, QTime(12, 0));

    auto sch2 = wt.getScheduleBeforeDate(QDate::currentDate()); // Error: There are no dates before the first date
    QVERIFY(!sch2.isValid());

    clear(&db);
}

void TestWorktimeTracker::getSchedule()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    wt.insertSchedule("test", QTime(10, 0), QTime(12, 0));

    auto sch1 = wt.getSchedule("test");
    QCOMPARE(sch1.name, "test");
    QCOMPARE(sch1.begin, QTime(10, 0));
    QCOMPARE(sch1.end, QTime(12, 0));

    auto sch2 = wt.getSchedule("default");
    QCOMPARE(sch2.name, wt.defaultSchedule().name);
    QCOMPARE(sch2.begin, wt.defaultSchedule().begin);
    QCOMPARE(sch2.end, wt.defaultSchedule().end);

    auto sch3 = wt.getSchedule("abcdefgh");
    QVERIFY(!sch3.isValid());

    clear(&db);
}

void TestWorktimeTracker::setSchedule()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    wt.insertSchedule("test", QTime(10, 0), QTime(12, 0));

    for (int i = 0; i < 4; ++i)
        wt.insertRecord(QDate::currentDate().addDays(i));

    QVERIFY(wt.setSchedule("test", QDate::currentDate(), QDate::currentDate().addDays(2)));

    QList<WorktimeTracker::Record> records;

    for (int i = 0; i < 4; ++i)
        records.append(wt.getRecord(QDate::currentDate().addDays(i)));

    QCOMPARE(records[0].schedule.name, "test");
    QCOMPARE(records[1].schedule.name, "test");
    QCOMPARE(records[2].schedule.name, "test");
    QCOMPARE(records[3].schedule.name, wt.defaultSchedule().name);

    clear(&db);
}

void TestWorktimeTracker::setArrivalTime()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    auto d = QDate::currentDate();

    for (int i = 0; i < 3; ++i)
        wt.insertRecord(d.addDays(i));

    QVERIFY(wt.setArrivalTime(QTime(10, 0), d));
    QCOMPARE(wt.getRecord(d).arrivalTime, QTime(10,0));

    // Error: arrival time > leaving time
    QVERIFY(!wt.setArrivalTime(QTime(18, 0), d));

    QVERIFY(wt.setArrivalTime(QTime(11, 0), d, d.addDays(2)));
    QCOMPARE(wt.getRecord(d).arrivalTime, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(1)).arrivalTime, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(2)).arrivalTime, QTime(11,0));

    // Should work since setArrivalTime swaps dates
    QVERIFY(wt.setArrivalTime(QTime(11, 0), d.addDays(2), d));
    QCOMPARE(wt.getRecord(d).arrivalTime, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(1)).arrivalTime, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(2)).arrivalTime, QTime(11,0));

    // If time is invalid, current time is set.
    // Record time and next current time always differ, so 100 ms delay would be ok

    // WARNING! IF CURRENT TIME WERE LATER THAN LEAVING TIME,
    // setArrivalTime() WOULD RETURN FALSE

    QVERIFY(wt.setArrivalTime(QTime(), d));
    QVERIFY(qAbs(wt.getRecord(d).arrivalTime.secsTo(QTime::currentTime())) < 100);

    // If date is invalid, current date is set
    QVERIFY(wt.setArrivalTime(QTime(10, 0), QDate()));
    QCOMPARE(wt.getRecord(d).arrivalTime, QTime(10, 0));

    // Error: date does not exist in table
    QVERIFY(!wt.setArrivalTime(QTime(10, 0), QDate(1996, 11, 26)));

    clear(&db);
}

void TestWorktimeTracker::setLeavingTime()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    auto d = QDate::currentDate();

    for (int i = 0; i < 3; ++i)
        wt.insertRecord(d.addDays(i));

    QVERIFY(wt.setLeavingTime(QTime(10, 0), d));
    QCOMPARE(wt.getRecord(d).leavingTime, QTime(10,0));

    // Error: arrival time > leaving time
    QVERIFY(!wt.setLeavingTime(QTime(7, 0), d));

    QVERIFY(wt.setLeavingTime(QTime(11, 0), d, d.addDays(2)));
    QCOMPARE(wt.getRecord(d).leavingTime, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(1)).leavingTime, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(2)).leavingTime, QTime(11,0));

    // Should work since setArrivalTime swaps dates
    QVERIFY(wt.setLeavingTime(QTime(11, 0), d.addDays(2), d));
    QCOMPARE(wt.getRecord(d).leavingTime, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(1)).leavingTime, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(2)).leavingTime, QTime(11,0));

    // If time is invalid, current time is set.
    // Record time and next current time always differ, so 100 ms delay would be ok

    // WARNING! IF CURRENT TIME WERE EARLIER THAN ARRIVING TIME,
    // setLeavingTime() WOULD RETURN FALSE

    QVERIFY(wt.setLeavingTime(QTime(), d));
    QVERIFY(qAbs(wt.getRecord(d).leavingTime.secsTo(QTime::currentTime())) < 100);

    // If date is invalid, current date is set
    QVERIFY(wt.setLeavingTime(QTime(10, 0), QDate()));
    QCOMPARE(wt.getRecord(d).leavingTime, QTime(10, 0));

    // Error: date does not exist in table
    QVERIFY(!wt.setLeavingTime(QTime(10, 0), QDate(1996, 11, 26)));

    clear(&db);
}

void TestWorktimeTracker::insertLeavePass()
{
    // TODO: don't insert same leave passes

    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    auto d = QDate(1996, 11, 26);

    QVERIFY(wt.insertLeavePass(QTime(10, 0), QTime(12, 0)));
    // Error: Same leave passes
//    QVERIFY(!wt.insertLeavePass(QTime(10, 0), QTime(12, 0)));
    QVERIFY(wt.insertLeavePass(QTime(11, 0), QTime(11, 30), d));
    QVERIFY(wt.insertLeavePass(QTime(9, 0), QTime(9, 1), d, "abc"));
    // Error: Invalid time
    QVERIFY(!wt.insertLeavePass(QTime(), QTime(10, 10), d.addDays(2)));
    QVERIFY(!wt.insertLeavePass(QTime(10, 10), QTime(), d.addDays(2)));

    QSqlQuery q("SELECT * FROM leavepass", db);

    q.next();
    QCOMPARE(q.value(0).toDate(), QDate::currentDate());
    QCOMPARE(q.value(1).toInt(), 0);
    QCOMPARE(q.value(2).toTime(), QTime(10, 0));
    QCOMPARE(q.value(3).toTime(), QTime(12, 0));
    QCOMPARE(q.value(4).toString(), QString());

    q.next();
    QCOMPARE(q.value(0).toDate(), d);
    QCOMPARE(q.value(1).toInt(), 0);
    QCOMPARE(q.value(2).toTime(), QTime(11, 0));
    QCOMPARE(q.value(3).toTime(), QTime(11, 30));
    QCOMPARE(q.value(4).toString(), QString());

    q.next();
    QCOMPARE(q.value(0).toDate(), d);
    QCOMPARE(q.value(1).toInt(), 1);
    QCOMPARE(q.value(2).toTime(), QTime(9, 0));
    QCOMPARE(q.value(3).toTime(), QTime(9, 1));
    QCOMPARE(q.value(4).toString(), QString("abc"));

    clear(&db);
}

void TestWorktimeTracker::getLeavePassList()
{
    // TODO: leave pass count is limited with 2 yet

    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    // Error: invalid date
    QVERIFY(wt.getLeavePassList(QDate()).isEmpty());

    auto d = QDate(1996, 11, 26);

    wt.insertLeavePass(QTime(10,0), QTime(11,0), d);
    wt.insertLeavePass(QTime(11,0), QTime(12,0), d);
//    wt.insertLeavePass(QTime(13,0), QTime(14,0), d);

    auto list = wt.getLeavePassList(d);
    // QVERIFY(list.size() == 3);
    QVERIFY(list.size() == 2);
    QCOMPARE(list[0].from, QTime(10, 0));
    QCOMPARE(list[0].to, QTime(11, 0));
    QCOMPARE(list[1].from, QTime(11, 0));
    QCOMPARE(list[1].to, QTime(12, 0));
//    QCOMPARE(list[2].from, QTime(13, 0));
//    QCOMPARE(list[2].to, QTime(14, 0));

    clear(&db);
}

void TestWorktimeTracker::setLeavePassBegin()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    auto d = QDate(1996, 11, 26);
    wt.insertLeavePass(QTime(10,0), QTime(11,0), d);
    wt.insertLeavePass(QTime(11,0), QTime(11,30), d);
    wt.insertLeavePass(QTime(15,0), QTime(16,0));

    QVERIFY(wt.setLeavePassBegin(QTime(10,20)));
    QCOMPARE(wt.getLeavePassList(QDate::currentDate())[0].from, QTime(10,20));

    QVERIFY(wt.setLeavePassBegin(QTime(9,15), d));
    QCOMPARE(wt.getLeavePassList(d)[0].from, QTime(9, 15));

    QVERIFY(wt.setLeavePassBegin(QTime(11,25), d, 1));
    QCOMPARE(wt.getLeavePassList(d)[1].from, QTime(11, 25));

    // Error: begin > end
    QVERIFY(!wt.setLeavePassBegin(QTime(17,15), d, 1));

    // Error: Invalid id
    QVERIFY(!wt.setLeavePassBegin(QTime(8,15), d, 2));

    // Error: Date doesn't exists in table
    QVERIFY(!wt.setLeavePassBegin(QTime(10,10), QDate(2222, 11, 11)));
}

void TestWorktimeTracker::setLeavePassEnd()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    auto d = QDate(1996, 11, 26);
    wt.insertLeavePass(QTime(10,0), QTime(11,0), d);
    wt.insertLeavePass(QTime(11,0), QTime(11,30), d);
    wt.insertLeavePass(QTime(15,0), QTime(16,0));

    QVERIFY(wt.setLeavePassEnd(QTime(17,20)));
    QCOMPARE(wt.getLeavePassList(QDate::currentDate())[0].to, QTime(17,20));

    QVERIFY(wt.setLeavePassEnd(QTime(12,15), d));
    QCOMPARE(wt.getLeavePassList(d)[0].to, QTime(12, 15));

    QVERIFY(wt.setLeavePassEnd(QTime(11,25), d, 1));
    QCOMPARE(wt.getLeavePassList(d)[1].to, QTime(11, 25));

    // Error: begin > end
    QVERIFY(!wt.setLeavePassEnd(QTime(6,15), d, 1));

    // Error: Invalid id
    QVERIFY(!wt.setLeavePassEnd(QTime(13,15), d, 2));

    // Error: Date doesn't exists in table
    QVERIFY(!wt.setLeavePassEnd(QTime(10,10), QDate(2222, 11, 11)));
}

void TestWorktimeTracker::setLeavePassComment()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    auto d = QDate(1996, 11, 26);
    wt.insertLeavePass(QTime(10,0), QTime(11,0), d);
    wt.insertLeavePass(QTime(11,0), QTime(12,0), d);

    QVERIFY(wt.setLeavePassComment("comment1", d));
    QCOMPARE(wt.getLeavePassList(d)[0].comment, QString("comment1"));

    QVERIFY(wt.setLeavePassComment("comment2", d, 1));
    QCOMPARE(wt.getLeavePassList(d)[1].comment, QString("comment2"));

    // Error: Invalid id
    QVERIFY(!wt.setLeavePassComment("comment3", d, 2));

    // Error: Date doesn't exists in table
    QVERIFY(!wt.setLeavePassComment("comment4"));
}

QSqlDatabase TestWorktimeTracker::createDb() const
{
    auto db = QSqlDatabase::addDatabase("QSQLITE", ":memory:");
    db.open();
    return db;
}

void TestWorktimeTracker::clear(QSqlDatabase *db)
{
    db->close();
    QSqlDatabase::removeDatabase(":memory:");
}
