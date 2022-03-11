#include "worktimetracker.h"
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QSqlRecord>

WorktimeTracker::WorktimeTracker(const QSqlDatabase &db)
    : m_db(db)
{
    Q_ASSERT(m_db.isOpen());

    QSqlQuery query(m_db);

    initScheduleTable();
    initLeavepassTable();
    initWorktimeTable();

    test();
}

TimeSpan WorktimeTracker::getSummary(const QDate &from, const QDate &to) const
{
//    if (!from.isValid() || !to.isValid())
//        return TimeSpan();

//    QSqlQuery query(m_db);
//    query.prepare("SELECT * FROM worktime WHERE Date BETWEEN date(:from) AND date(:to)");
//    query.bindValue(":from", dateToString(from));
//    query.bindValue(":to", dateToString(to));

//    if (!execQueryVerbosely(&query))
//        return TimeSpan();

//    TimeSpan ts;

//    while (query.next())
//    {
//        auto date = stringToDate(query.value("Date").toString());

//        auto schedule = getSchedule(query.value("Schedule").toString());
//        if (!schedule.isValid())
//            return TimeSpan();

//        auto arrivalTime = stringToTime(query.value("ArrivalTime").toString());
//        auto leavingTime = stringToTime(query.value("LeavingTime").toString());

//        auto leavePasses = getLeavePassList(date);
//        for (auto leavePass : leavePasses)
//            ts.seconds -= leavePass.to.secsTo(leavePass.from);

//        if (leavePasses.isEmpty() ? true : arrivalTime < schedule.begin)
//            ts.seconds += arrivalTime.secsTo(schedule.begin);

//        if (leavePasses.isEmpty() ? true : schedule.end < leavingTime)
//            ts.seconds += schedule.end.secsTo(leavingTime);
//    }

//    return ts;
}

TimeSpan WorktimeTracker::getSummary(int month, int year)
{
    if (month < 1 || month > 12)
        return TimeSpan();

    int _year = year < 0 ? QDate::currentDate().year() : year;

    QDate monthStart = QDate(_year, month, 1);
    QDate monthEnd   = QDate(_year, month, monthStart.daysInMonth());

    return getSummary(monthStart, monthEnd);
}

bool WorktimeTracker::insertDate(const QDate &date, const QTime &arrival, const QTime &leaving, const QString &schedule)
{
    // TODO: arrival must be lower than leaving

    if (schedule.isEmpty() || !date.isValid())
        return false;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO worktime VALUES (:d, :schedule, :arrival, :leaving)");
    query.bindValue(":d", dateToString(date));
    query.bindValue(":schedule", schedule);
    query.bindValue(":arrival", timeToString(arrival));
    query.bindValue(":leaving", timeToString(leaving));

    return execQueryVerbosely(&query);
}

bool WorktimeTracker::insertDate(const QDate &date)
{
    Schedule schedule = getScheduleBeforeDate(date);

    if (schedule.isValid())
        return insertDate(date, QTime(), QTime(), schedule.name);

    return insertDate(date, QTime(), QTime());
}

bool WorktimeTracker::setSchedule(const QString &schedule, const QDate &from, const QDate &to)
{
    if (schedule.isEmpty())
        return false;

    auto _from  = from.isValid() ? from : QDate::currentDate();
    auto _to    = to.isValid() ? to : _from;

    return updateColumnData("worktime", "Schedule", _from, _to, schedule);
}

bool WorktimeTracker::insertSchedule(const QString &schedule, const QTime &begin, const QTime &end)
{
    // TODO: begin must be lower than end

    if (schedule.isEmpty() || !begin.isValid() || !end.isValid())
        return false;

    QSqlQuery query(m_db);

    query.prepare("INSERT INTO schedule VALUES(:schedule,:begin,:end)");
    query.bindValue(":schedule", schedule);
    query.bindValue(":begin", timeToString(begin));
    query.bindValue(":end", timeToString(end));
    return execQueryVerbosely(&query);
}

bool WorktimeTracker::setArrivalTime(const QTime &time, const QDate &from, const QDate &to)
{
    // TODO: time must be lower than LeavingTime

    auto _time = time.isValid() ? time : QTime::currentTime();
    auto _from = from.isValid() ? from : QDate::currentDate();
    auto _to   = to.isValid() ? to : _from;
    return updateColumnData("worktime", "ArrivalTime", _from, _to, timeToString(_time));
}

bool WorktimeTracker::setLeavingTime(const QTime &time, const QDate &from, const QDate &to)
{
    // TODO: time must be higher than ArrivalTime

    auto _time = time.isValid() ? time : QTime::currentTime();
    auto _from = from.isValid() ? from : QDate::currentDate();
    auto _to   = to.isValid() ? to : _from;
    return updateColumnData("worktime", "LeavingTime", _from, _to, timeToString(_time));
}

QList<WorktimeTracker::LeavePass> WorktimeTracker::getLeavePassList(const QDate &date) const
{
    if (!date.isValid())
        return QList<LeavePass>();

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM leavepass WHERE Date = date(:d)");
    query.bindValue(":d", dateToString(date));
    if (!execQueryVerbosely(&query))
        return QList<LeavePass>();

    QList<LeavePass> leavePassList;
    while (query.next())
    {
        LeavePass lp;
        lp.date    = stringToDate(query.value("Date").toString());
        lp.id      = query.value("Id").toInt();
        lp.from    = stringToTime(query.value("TimeFrom").toString());
        lp.to      = stringToTime(query.value("TimeTo").toString());
        lp.comment = query.value("Comment").toString();
        leavePassList.append(lp);
    }

    return leavePassList;
}

bool WorktimeTracker::insertLeavePass(const QTime &from, const QTime &to, const QDate &date, const QString &comment)
{
    if (!from.isValid() || !to.isValid())
        return false;

    QSqlQuery query(m_db);

    auto _date = date.isValid() ? date : QDate::currentDate();

    query.prepare("SELECT Count(*) FROM leavepass WHERE Date = date(:d) ORDER BY Date DESC");
    query.bindValue(":d", dateToString(_date));
    if (execQueryVerbosely(&query))
    {
        if (query.next())
        {
            bool ok;
            int count = query.value(0).toInt(&ok);
            if (!ok) return false;

            // TODO: Leave pass count is temporary limited with 2 because I don't know how to
            // implement WorkRange::getUnion() of more than 2 ranges :)
            if (count >= 2)
            {
                qDebug() << "Leave pass count limit == 2";
                return false;
            }

            query.prepare("INSERT INTO leavepass VALUES (:d, :id, :begin, :end, :comment)");
            query.bindValue(":d", dateToString(_date));
            query.bindValue(":id", QString::number(count));
            query.bindValue(":begin", timeToString(from));
            query.bindValue(":end", timeToString(to));
            query.bindValue(":comment", comment);

            return execQueryVerbosely(&query);
        }
    }

    return false;
}

bool WorktimeTracker::setLeavePassBegin(const QTime &time, const QDate &date, int id)
{
    // TODO: time must be lower than TimeTo

    auto _time = time.isValid() ? time : QTime::currentTime();
    auto _date = date.isValid() ? date : QDate::currentDate();
    return updateLeavePassData("TimeFrom", _date, id, timeToString(_time));
}

bool WorktimeTracker::setLeavePassEnd(const QTime &time, const QDate &date, int id)
{
    // TODO: time must be higher than TimeFrom

    auto _time = time.isValid() ? time : QTime::currentTime();
    auto _date = date.isValid() ? date : QDate::currentDate();
    return updateLeavePassData("TimeTo", _date, id, timeToString(_time));
}

bool WorktimeTracker::setLeavePassComment(const QString &comment, const QDate &date, int id)
{
    auto _date = date.isValid() ? date : QDate::currentDate();
    return updateLeavePassData("Comment", _date, id, comment);
}

void WorktimeTracker::initWorktimeTable()
{
    QSqlQuery query(m_db);

    execQueryVerbosely(&query, "CREATE TABLE worktime ("
                               "    Date TEXT PRIMARY KEY NOT NULL,"
                               "    Schedule TEXT,"
                               "    ArrivalTime TEXT,"
                               "    LeavingTime TEXT"
                               ")");
}

void WorktimeTracker::initLeavepassTable()
{
    QSqlQuery query(m_db);

    execQueryVerbosely(&query, "CREATE TABLE leavepass ("
                               "    Date TEXT NOT NULL,"
                               "    Id INT,"
                               "    TimeFrom TEXT,"
                               "    TimeTo TEXT,"
                               "    Comment TEXT,"
                               "    PRIMARY KEY (Date, Id)"
                               ")");
}

void WorktimeTracker::initScheduleTable()
{
    QSqlQuery query(m_db);

    execQueryVerbosely(&query, "CREATE TABLE schedule ("
                               "    Name TEXT PRIMARY KEY NOT NULL,"
                               "    Begin TEXT,"
                               "    End TEXT"
                               ")");

    query.prepare("INSERT INTO schedule VALUES('default',:begin,:end)");
    query.bindValue(":begin", timeToString(QTime(8, 0)));  // 08:00
    query.bindValue(":end", timeToString(QTime(17, 0))); // 17:00
    execQueryVerbosely(&query);

    query.prepare("INSERT INTO schedule VALUES('floating_first_hour',:begin,:end)");
    query.bindValue(":begin", timeToString(QTime(8, 0)));  // 08:00
    query.bindValue(":end", timeToString(QTime(17, 0))); // 17:00
    execQueryVerbosely(&query);
}

bool WorktimeTracker::execQueryVerbosely(QSqlQuery *q, const QString &cmd) const
{
    if (!q)
        return false;

    bool result = (cmd.isEmpty()) ? q->exec() : q->exec(cmd);

    if (q->lastError().isValid())
        qDebug() << "-----\nQuery:" << q->executedQuery()
                 << "\nBounds:" << q->boundValues()
                 << "\nError:" << q->lastError().text()
                 << "\n-----";

    return result;
}

bool WorktimeTracker::updateColumnData(const QString &table, const QString &column, const QDate &from, const QDate &to, const QString& data) const
{
    if (!from.isValid() && !to.isValid())
        return false;

    QSqlQuery query(m_db);

    query.prepare(QString("UPDATE %1 SET %2=:data WHERE Date BETWEEN date(:from) AND date(:to)").arg(table).arg(column));
    query.bindValue(":from", dateToString(from));
    query.bindValue(":to", dateToString(to));
    query.bindValue(":data", data);

    if (!execQueryVerbosely(&query))
        return false;

    query.exec("SELECT changes()");

    return query.next() ? query.value(0).toBool() : false;
}

bool WorktimeTracker::updateLeavePassData(const QString &column, const QDate &date, int id, const QString &data) const
{
    if (!date.isValid())
        return false;

    QSqlQuery query(m_db);

    query.prepare(QString("UPDATE leavepass SET %2=:data WHERE Date = date(:d) AND Id = :id").arg(column));
    query.bindValue(":d", dateToString(date));
    query.bindValue(":id", QString::number(id));
    query.bindValue(":data", data);

    if (!execQueryVerbosely(&query))
        return false;

    query.exec("SELECT changes()");

    return query.next() ? query.value(0).toBool() : false;
}

WorktimeTracker::Schedule WorktimeTracker::getSchedule(const QString &name) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM schedule WHERE Name = :name");
    query.bindValue(":name", name);

    if (!execQueryVerbosely(&query))
        return Schedule();

    if (!query.next())
        return Schedule();

    Schedule s;
    s.name = query.value("Name").toString();
    s.begin = stringToTime(query.value("Begin").toString());
    s.end = stringToTime(query.value("End").toString());
    return s;
}

void WorktimeTracker::test()
{
    QSqlQuery query(m_db);

    insertSchedule("testschedule1", QTime(5, 25), QTime(9, 49));

    insertDate(QDate(2022, 01, 18), QTime(8, 0), QTime(17, 0));

    for (int i = 1; i < 60; i++)
        insertDate(QDate(2022, 01, 18).addDays(i), QTime(8, 0), QTime(17, 0));

    insertDate(QDate(2022, 01, 18).addDays(60), QTime(8, 0), QTime(17, 0));

    // ---------- Test summary --------------------

    qDebug() << "1" << insertLeavePass(QTime(10, 0), QTime(10, 15), QDate(2022, 01, 19), "lp0")
             << insertLeavePass(QTime(10, 25), QTime(10, 40), QDate(2022, 01, 19), "lp1")
             << insertLeavePass(QTime(10, 55), QTime(10, 45), QDate(2022, 01, 19), "lpf")
             << insertLeavePass(QTime(8, 0), QTime(9, 35), QDate(2022, 02, 20), "lp2")
             << insertLeavePass(QTime(16, 20), QTime(17, 00), QDate(2022, 03, 15), "lp3");

    TimeSpan ts1 = getSummary(1);
    TimeSpan ts2 = getSummary(2);
    TimeSpan ts3 = getSummary(3);/*getSummary(QDate(2022, 01, 18), QDate(2022, 03, 19));*/

    qDebug() << "2" << ts1.hours() << ts1.minutes();
    qDebug() << "3" << ts2.hours() << ts2.minutes();
    qDebug() << "4" << ts3.hours() << ts3.minutes();

    QTime t1(8, 00);
    QTime t2(8, 25);
    QTime t3(8, 50);
    QTime t4(8, 55);

    auto u = TimeRange::getUnion({t1, t2}, {t3, t4});
    for (auto r : u) qDebug() << r.toString();


    // ---------- Test functions --------------------

//    auto s1 = getScheduleBeforeDate(QDate(2022, 01, 19));
//    auto s2 = getScheduleBeforeDate(QDate(2022, 01, 20));

//    qDebug() << "1" << s1.begin << s1.end << s2.begin << s2.end;

//    qDebug() << "2" << insertDate(QDate::currentDate()) << insertDate(QDate(1996, 11, 26));

//    qDebug() << "3" << setArrivalTime() << setArrivalTime(QTime(15, 00), QDate(2022, 01, 20)) << setArrivalTime(QTime(15, 20), QDate(2055, 11, 23));
//    qDebug() << "4" << setLeavingTime() << setLeavingTime(QTime(15, 00), QDate(2022, 01, 20));

//    qDebug() << "5" << setSchedule("testschedule1", QDate(2022, 01, 20), QDate(2022, 01, 25));

//    qDebug() << "6" << insertLeavePass(QTime(10, 0), QTime(10, 30))
//             << insertLeavePass(QTime(10, 40), QTime(10, 50))
//             << insertLeavePass(QTime(10, 40), QTime(11, 40), QDate(1996, 11, 26), "comment");

//    qDebug() << "7" << setLeavePassBegin(QTime(15, 35))
//             << setLeavePassEnd(QTime(12, 0), QDate::currentDate(), 1)
//             << setLeavePassComment("no comments");



}

WorktimeTracker::Schedule WorktimeTracker::getScheduleBeforeDate(const QDate &date) const
{
    if (!date.isValid())
        return Schedule();

    QSqlQuery query(m_db);
    query.prepare("SELECT Schedule FROM worktime WHERE Date < date(:d) ORDER BY Date DESC LIMIT 1");
    query.bindValue(":d", dateToString(date));
    if (!execQueryVerbosely(&query))
        return Schedule();

    if (!query.next())
        return Schedule();

    return getSchedule(query.value(0).toString());
}

bool WorktimeTracker::Schedule::isValid() const
{
    return !name.isEmpty() && begin.isValid() && end.isValid();
}

QString WorktimeTracker::Schedule::toString() const
{
    return QString("schedule name=%1, begin=%2, end=%3").arg(name).arg(begin.toString()).arg(end.toString());
}

bool WorktimeTracker::LeavePass::isValid() const
{
    return date.isValid() && from.isValid() && to.isValid();
}

QString WorktimeTracker::LeavePass::toString() const
{
    return QString("leave pass date=%1, id=%2, from=%3, to=%4, comment=%5")
            .arg(date.toString())
            .arg(id)
            .arg(from.toString())
            .arg(to.toString())
            .arg(comment);
}

bool WorktimeTracker::Worktime::isValid() const
{
    return date.isValid() && schedule.isValid() && arrivalTime.isValid() && leavingTime.isValid();
}

QString WorktimeTracker::Worktime::toString() const
{
    return QString("worktime date=%1, schedule={%2}, arrivalTime=%3, leavingTime=%4")
            .arg(date.toString())
            .arg(schedule.toString())
            .arg(arrivalTime.toString())
            .arg(leavingTime.toString());
}
