#include "testworktimetracker.h"

WorktimeTracker TestWorktimeTracker::example(const QSqlDatabase &db)
{
    WorktimeTracker wt(db);

    wt.insertSchedule("testschedule1", QTime(5, 25), QTime(9, 49), QTime(6,0), QTime(7,0));

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

void TestWorktimeTracker::insertSchedule()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    QVERIFY(wt.insertSchedule("test1", QTime(9, 0), QTime(11, 0), QTime(10, 0), QTime(10, 30)));
    QVERIFY(wt.insertSchedule("test1_1", QTime(9, 30), QTime(11, 30), QTime(10, 30), QTime(11, 00)));
    // Error: The same as previous
    QVERIFY(!wt.insertSchedule("test1", QTime(9, 0), QTime(11, 0), QTime(10, 0), QTime(10, 30)));
    // Error: begin > end
    QVERIFY(!wt.insertSchedule("test2", QTime(12, 0), QTime(11, 0), QTime(11, 30), QTime(11, 40)));
    // Error: invalid begin
    QVERIFY(!wt.insertSchedule("test3", QTime(), QTime(10, 0), QTime(9, 30), QTime(9, 40)));
    // Error: invalid end
    QVERIFY(!wt.insertSchedule("test3", QTime(10, 0), QTime(), QTime(9, 30), QTime(9, 40)));
    // Error: begin == end
    QVERIFY(!wt.insertSchedule("test4", QTime(10, 0), QTime(10, 0), QTime(10, 0), QTime(10, 0)));
    // Error: invalid lunchBegin
    QVERIFY(!wt.insertSchedule("test5", QTime(9, 30), QTime(11, 30), QTime(), QTime(10, 0)));
    // Error: invalid lunchEnd
    QVERIFY(!wt.insertSchedule("test6", QTime(9, 30), QTime(11, 30), QTime(9, 30), QTime()));
    // Error: lunch time is out of schedule time boundaries
    QVERIFY(!wt.insertSchedule("test7", QTime(9, 30), QTime(11, 30), QTime(12, 30), QTime(13, 0)));
    QVERIFY(!wt.insertSchedule("test8", QTime(9, 30), QTime(11, 30), QTime(8, 30), QTime(10, 0)));

    QSqlQuery q("SELECT * FROM schedule", db);

    q.next();

    QCOMPARE(q.value(0).toString(), wt.defaultSchedule().name);
    QCOMPARE(q.value(1).toTime(), wt.defaultSchedule().begin);
    QCOMPARE(q.value(2).toTime(), wt.defaultSchedule().end);
    QCOMPARE(q.value(3).toTime(), wt.defaultSchedule().lunchTimeBegin);
    QCOMPARE(q.value(4).toTime(), wt.defaultSchedule().lunchTimeEnd);

    q.next();
    QCOMPARE(q.value(0).toString(), QString("test1"));
    QCOMPARE(q.value(1).toTime(), QTime(9, 0));
    QCOMPARE(q.value(2).toTime(), QTime(11, 0));
    QCOMPARE(q.value(3).toTime(), QTime(10, 0));
    QCOMPARE(q.value(4).toTime(), QTime(10, 30));

    q.next();
    QCOMPARE(q.value(0).toString(), QString("test1_1"));
    QCOMPARE(q.value(1).toTime(), QTime(9, 30));
    QCOMPARE(q.value(2).toTime(), QTime(11, 30));
    QCOMPARE(q.value(3).toTime(), QTime(10, 30));
    QCOMPARE(q.value(4).toTime(), QTime(11, 0));

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
    QCOMPARE(r1.checkIn, wt.defaultSchedule().begin);
    QCOMPARE(r1.checkOut, wt.defaultSchedule().end);

    QVERIFY(r2.isValid());
    QCOMPARE(r2.date, QDate(1996, 11, 29));
    QCOMPARE(r2.schedule.name, wt.defaultSchedule().name);
    QCOMPARE(r2.checkIn, wt.defaultSchedule().begin);
    QCOMPARE(r2.checkOut, wt.defaultSchedule().end);

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

    wt.insertSchedule("test", QTime(10, 0), QTime(12, 0), QTime(10, 30), QTime(11, 0));

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

    wt.insertSchedule("test", QTime(10, 0), QTime(12, 0), QTime(10, 30), QTime(11, 0));

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

    wt.insertSchedule("test", QTime(10, 0), QTime(12, 0), QTime(10, 30), QTime(11, 0));

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

    wt.insertSchedule("test", QTime(10, 0), QTime(12, 0), QTime(10, 30), QTime(11, 0));

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

void TestWorktimeTracker::setCheckIn()
{
    // TODO: test it with custom schedules

    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    auto d = QDate::currentDate();

    for (int i = 0; i < 3; ++i)
        wt.insertRecord(d.addDays(i));

    QVERIFY(wt.setCheckIn(QTime(10, 0), d));
    QCOMPARE(wt.getRecord(d).checkIn, QTime(10,0));

    // Error: arrival time > leaving time
    QVERIFY(!wt.setCheckIn(QTime(18, 0), d));

    QVERIFY(wt.setCheckIn(QTime(11, 0), d, d.addDays(2)));
    QCOMPARE(wt.getRecord(d).checkIn, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(1)).checkIn, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(2)).checkIn, QTime(11,0));

    // Should work since setCheckIn swaps dates
    QVERIFY(wt.setCheckIn(QTime(11, 0), d.addDays(2), d));
    QCOMPARE(wt.getRecord(d).checkIn, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(1)).checkIn, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(2)).checkIn, QTime(11,0));

    // Invalid time
    QVERIFY(!wt.setCheckIn(QTime(), d));

    // If date is invalid, current date is set
    QVERIFY(wt.setCheckIn(QTime(10, 0), QDate()));
    QCOMPARE(wt.getRecord(d).checkIn, QTime(10, 0));

    // Error: date does not exist in table
    QVERIFY(!wt.setCheckIn(QTime(10, 0), QDate(1996, 11, 26)));

    clear(&db);
}

void TestWorktimeTracker::setCheckOut()
{
    // TODO: test it with custom schedules

    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    auto d = QDate::currentDate();

    for (int i = 0; i < 3; ++i)
        wt.insertRecord(d.addDays(i));

    QVERIFY(wt.setCheckOut(QTime(10, 0), d));
    QCOMPARE(wt.getRecord(d).checkOut, QTime(10,0));

    // Error: arrival time > leaving time
    QVERIFY(!wt.setCheckOut(QTime(7, 0), d));

    QVERIFY(wt.setCheckOut(QTime(11, 0), d, d.addDays(2)));
    QCOMPARE(wt.getRecord(d).checkOut, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(1)).checkOut, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(2)).checkOut, QTime(11,0));

    // Should work since setCheckIn swaps dates
    QVERIFY(wt.setCheckOut(QTime(11, 0), d.addDays(2), d));
    QCOMPARE(wt.getRecord(d).checkOut, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(1)).checkOut, QTime(11,0));
    QCOMPARE(wt.getRecord(d.addDays(2)).checkOut, QTime(11,0));

    // Invalid time
    QVERIFY(!wt.setCheckOut(QTime(), d));

    // If date is invalid, current date is set
    QVERIFY(wt.setCheckOut(QTime(10, 0), QDate()));
    QCOMPARE(wt.getRecord(d).checkOut, QTime(10, 0));

    // Error: date does not exist in table
    QVERIFY(!wt.setCheckOut(QTime(10, 0), QDate(1996, 11, 26)));

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
    wt.insertLeavePass(QTime(13,0), QTime(14,0), d);

    auto list = wt.getLeavePassList(d);
    QCOMPARE(list.size(), 3);
    QCOMPARE(list[0].from, QTime(10, 0));
    QCOMPARE(list[0].to, QTime(11, 0));
    QCOMPARE(list[1].from, QTime(11, 0));
    QCOMPARE(list[1].to, QTime(12, 0));
    QCOMPARE(list[2].from, QTime(13, 0));
    QCOMPARE(list[2].to, QTime(14, 0));

    clear(&db);
}

void TestWorktimeTracker::setLeavePassBegin()
{
    // TODO: test it with custom schedules

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

    clear(&db);
}

void TestWorktimeTracker::setLeavePassEnd()
{
    // TODO: test it with custom schedules

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

    clear(&db);
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

    clear(&db);
}

void TestWorktimeTracker::getSummary()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    auto scheduleBegin = wt.defaultSchedule().begin;
    auto scheduleEnd   = wt.defaultSchedule().end;

    constexpr int hourInSec = 60*60;

    auto d = QDate(1996, 01, 01);
    for (int i = 0; i < 100; ++i)
        wt.insertRecord(d.addDays(i));

    // -1 hour in January
    wt.setCheckIn(scheduleBegin.addSecs(hourInSec), QDate(1996, 1, 1));
    QCOMPARE(wt.getSummary(1, 1996).seconds, -hourInSec);

    // +1 hour in February
    wt.setCheckOut(scheduleEnd.addSecs(hourInSec / 2.0), QDate(1996, 2, 15));
    wt.setCheckOut(scheduleEnd.addSecs(hourInSec / 2.0), QDate(1996, 2, 16));
    QCOMPARE(wt.getSummary(2, 1996).seconds, hourInSec);

    // +0.5 hour at 1996/2/15
    QCOMPARE(wt.getSummary(QDate(1996, 2, 15), QDate(1996, 2, 15)).seconds, hourInSec / 2.0);
    QCOMPARE(wt.getSummary(QDate(1996, 2, 15)).seconds, hourInSec / 2.0);

    // -0.5 hour in March
    wt.setCheckIn(scheduleBegin.addSecs(hourInSec/2.0), QDate(1996, 3, 8));
    QCOMPARE(wt.getSummary(3, 1996).seconds, -hourInSec / 2.0);

    // +0.5 hour since 1996/02/10 to 1996/03/10
    QCOMPARE(wt.getSummary(QDate(1996, 2, 10), QDate(1996, 3, 10)).seconds, hourInSec / 2.0);

    // +0.5 hour since 1996/02/10 to 1996/03/10 (inverted order)
    QCOMPARE(wt.getSummary(QDate(1996, 3, 10), QDate(1996, 2, 10)).seconds, hourInSec / 2.0);

    // 0 since 1996/02/16 to 1996/02/19
    QCOMPARE(wt.getSummary(QDate(1996, 2, 16), QDate(1996, 3, 19)).seconds, 0);

    // Custom schedule
    wt.insertSchedule("custom", QTime(10, 0), QTime(12, 0), QTime(10, 30), QTime(11, 0));
    wt.setSchedule("custom", QDate(1996, 3, 3), QDate(1996, 3, 4));
    wt.setCheckIn(QTime(10, 30), QDate(1996, 3, 3));
    wt.setCheckOut(QTime(12, 0), QDate(1996, 3, 3));
    wt.setCheckIn(QTime(10, 0), QDate(1996, 3, 4));
    wt.setCheckOut(QTime(11, 30), QDate(1996, 3, 4));
    QCOMPARE(wt.getSummary(3, 1996).seconds, -1.5*hourInSec);

    // May of current year (+1 hour)
    auto dCurrentYear = QDate(QDate::currentDate().year(), 05, 01);
    wt.insertRecord(dCurrentYear);
    wt.setCheckIn(scheduleBegin.addSecs(-hourInSec), dCurrentYear);
    QCOMPARE(wt.getSummary(5).seconds, hourInSec);

    clear(&db);
}

void TestWorktimeTracker::getSummary_leavepass()
{
    QSqlDatabase db = createDb();
    WorktimeTracker wt(db);

    auto scheduleBegin = wt.defaultSchedule().begin;
    auto scheduleEnd   = wt.defaultSchedule().end;

    constexpr int hourInSec = 60*60;

    auto d = QDate(1996, 01, 01);
    for (int i = 0; i < 100; ++i)
        wt.insertRecord(d.addDays(i));

    wt.setCheckIn(scheduleBegin.addSecs(hourInSec), QDate(1996, 1, 1));
    wt.setCheckOut(scheduleEnd.addSecs(hourInSec), QDate(1996, 1, 1));
    wt.insertLeavePass(QTime(8, 30), QTime(8, 40), QDate(1996, 1, 1));
    wt.insertLeavePass(QTime(8, 38), QTime(9, 00), QDate(1996, 1, 1));
    wt.insertLeavePass(QTime(15, 30), QTime(17, 00), QDate(1996, 1, 1));
    QCOMPARE(wt.getSummary(1, 1996).seconds, -1.5*hourInSec);

    // Custom schedule
    wt.insertSchedule("custom", QTime(10, 0), QTime(12, 0), QTime(10, 30), QTime(11, 0));
    wt.setSchedule("custom", QDate(1996, 3, 3));
    wt.setCheckIn(QTime(10, 0), QDate(1996, 3, 3));
    wt.setCheckOut(QTime(12, 0), QDate(1996, 3, 3));
    wt.insertLeavePass(QTime(10, 30), QTime(11,0), QDate(1996, 3, 3));
    QCOMPARE(wt.getSummary(3, 1996).seconds, -0.5*hourInSec);

    clear(&db);
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
