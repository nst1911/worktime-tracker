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

QTime WorktimeTracker::getTimeSummary(int month)
{

}

bool WorktimeTracker::insertDate(const QDate &date, const QTime &scheduleBegin, const QTime &scheduleEnd, const QTime &arrival, const QTime &leaving)
{
    if (!date.isValid())
        return false;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO worktime VALUES (:d, :schBegin, :schEnd, :arrival, :leaving)");
    query.bindValue(":d", dateToString(date));
    query.bindValue(":schBegin", timeToString(scheduleBegin));
    query.bindValue(":schEnd", timeToString(scheduleEnd));
    query.bindValue(":arrival", timeToString(arrival));
    query.bindValue(":leaving", timeToString(leaving));

    return execQueryVerbosely(&query);
}

bool WorktimeTracker::insertDate(const QDate &date)
{
    Schedule schedule = getScheduleBeforeDate(date);

    if (!schedule.isValid()) {
        schedule = getSchedule("default");
        if (!schedule.isValid()) return false;
    }

    return insertDate(date, schedule.begin, schedule.end, QTime(), QTime());
}

bool WorktimeTracker::setSchedule(const QTime &start, const QTime &end, const QDate &from, const QDate &to)
{
    if (!start.isValid() || !end.isValid())
        return false;

    auto _from  = from.isValid() ? from : QDate::currentDate();
    auto _to    = to.isValid() ? to : _from;

    bool s1 = updateColumnData("worktime", "ScheduleBeginTime", _from, _to, timeToString(start));
    bool s2 = updateColumnData("worktime", "ScheduleEndTime", _from, _to, timeToString(end));

    return s1 && s2;
}

bool WorktimeTracker::setArrivalTime(const QTime &time, const QDate &from, const QDate &to)
{
    auto _time = time.isValid() ? time : QTime::currentTime();
    auto _from = from.isValid() ? from : QDate::currentDate();
    auto _to   = to.isValid() ? to : _from;
    return updateColumnData("worktime", "ArrivalTime", _from, _to, timeToString(_time));
}

bool WorktimeTracker::setLeavingTime(const QTime &time, const QDate &from, const QDate &to)
{
    auto _time = time.isValid() ? time : QTime::currentTime();
    auto _from = from.isValid() ? from : QDate::currentDate();
    auto _to   = to.isValid() ? to : _from;
    return updateColumnData("worktime", "LeavingTime", _from, _to, timeToString(_time));
}

bool WorktimeTracker::insertLeavePass(const QTime &from, const QTime &to, const QDate &date, const QString &comment)
{
    if (!from.isValid() || !to.isValid())
        return false;

    auto _date = date.isValid() ? date : QDate::currentDate();
    int id = 0;

    QSqlQuery query(m_db);
    query.prepare("SELECT Id FROM leavepass WHERE Date = date(:d) ORDER BY Date DESC LIMIT 1");
    query.bindValue(":d", dateToString(_date));
    if (execQueryVerbosely(&query))
    {
        if (query.next())
        {
            bool ok = false;
            int _id = query.value(0).toInt(&ok);
            if (ok) id = _id + 1;
        }
    }

    query.prepare("INSERT INTO leavepass VALUES (:d, :id, :begin, :end, :comment)");
    query.bindValue(":d", dateToString(_date));
    query.bindValue(":id", QString::number(id));
    query.bindValue(":begin", timeToString(from));
    query.bindValue(":end", timeToString(to));
    query.bindValue(":comment", comment);

    return execQueryVerbosely(&query);
}

bool WorktimeTracker::setLeavePassBegin(const QTime &time, const QDate &date, int id)
{
    auto _time = time.isValid() ? time : QTime::currentTime();
    auto _date = date.isValid() ? date : QDate::currentDate();
    return updateLeavePassData("TimeFrom", _date, id, timeToString(_time));
}

bool WorktimeTracker::setLeavePassEnd(const QTime &time, const QDate &date, int id)
{
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
                               "    Date TEXT PRIMARY KEY,"
                               "    ScheduleBeginTime TEXT,"
                               "    ScheduleEndTime TEXT,"
                               "    ArrivalTime TEXT,"
                               "    LeavingTime TEXT"
                               ")");
}

void WorktimeTracker::initLeavepassTable()
{
    QSqlQuery query(m_db);

    execQueryVerbosely(&query, "CREATE TABLE leavepass ("
                               "    Date TEXT,"
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
                               "    Type TEXT PRIMARY KEY,"
                               "    Begin TEXT,"
                               "    End TEXT"
                               ")");

    query.prepare("INSERT INTO schedule VALUES('default',:begin,:end)");
    query.bindValue(":begin", timeToString(QTime(5, 0)));  // 08:00
    query.bindValue(":end", timeToString(QTime(17, 0))); // 17:00
    execQueryVerbosely(&query);
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

WorktimeTracker::Schedule WorktimeTracker::getSchedule(const QString &type) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT Begin, End FROM schedule WHERE Type = :type");
    query.bindValue(":type", type);

    if (!execQueryVerbosely(&query))
        return Schedule();

    if (!query.next())
        return Schedule();

    Schedule s;
    s.begin = stringToTime(query.value(0).toString());
    s.end = stringToTime(query.value(1).toString());
    return s;
}

void WorktimeTracker::test()
{
    QSqlQuery query(m_db);

    QDate date(QDate::fromString("18.01.2022", "dd.MM.yyyy"));

    query.exec(QString("INSERT INTO worktime VALUES("
                       "    '%1',"
                       "    '07:00:00',"
                       "    '15:00:00',"
                       "    '08:00:00',"
                       "    '18:00:00'"
                       ")").arg(date.toString(Qt::ISODate)));

    for (int i = 0; i < 60; i++)
    {
        query.exec(QString("INSERT INTO worktime VALUES("
                           "    '%1',"
                           "    '08:00:00',"
                           "    '17:00:00',"
                           "    '08:00:00',"
                           "    '17:00:00'"
                           ")").arg(date.addDays(i).toString(Qt::ISODate)));
    }

    query.exec(QString("INSERT INTO worktime VALUES("
                       "    '%1',"
                       "    '08:00:00',"
                       "    '17:00:00',"
                       "    '09:00:00',"
                       "    '17:00:00'"
                       ")").arg(date.addDays(60).toString(Qt::ISODate)));


    auto s1 = getScheduleBeforeDate(QDate(2022, 01, 19));
    auto s2 = getScheduleBeforeDate(QDate(2022, 01, 20));

    qDebug() << "1" << s1.begin << s1.end << s2.begin << s2.end;

    qDebug() << "2" << insertDate(QDate::currentDate()) << insertDate(QDate(1996, 11, 26));

    qDebug() << "3" << setArrivalTime() << setArrivalTime(QTime(15, 00), QDate(2022, 01, 20)) << setArrivalTime(QTime(15, 20), QDate(2055, 11, 23));
    qDebug() << "4" << setLeavingTime() << setLeavingTime(QTime(15, 00), QDate(2022, 01, 20));

    qDebug() << "5" << setSchedule(QTime(10, 00), QTime(11, 00), QDate(2022, 01, 20), QDate(2022, 01, 25));

    qDebug() << "6" << insertLeavePass(QTime(10, 0), QTime(10, 30))
             << insertLeavePass(QTime(10, 40), QTime(10, 50))
             << insertLeavePass(QTime(10, 40), QTime(11, 40), QDate(1996, 11, 26), "comment");

    qDebug() << "7" << setLeavePassBegin(QTime(15, 35))
             << setLeavePassEnd(QTime(12, 0), QDate::currentDate(), 1)
             << setLeavePassComment("no comments");

}

WorktimeTracker::Schedule WorktimeTracker::getScheduleBeforeDate(const QDate &date) const
{
    if (!date.isValid())
        return Schedule();

    QSqlQuery query(m_db);
    query.prepare("SELECT ScheduleBeginTime, ScheduleEndTime FROM worktime WHERE Date < date(:d) ORDER BY Date DESC LIMIT 1");
    query.bindValue(":d", dateToString(date));
    if (!execQueryVerbosely(&query))
        return Schedule();

    if (!query.next())
        return Schedule();

    Schedule schedule;
    schedule.begin = stringToTime(query.value(0).toString());
    schedule.end = stringToTime(query.value(1).toString());

    return schedule;
}

bool WorktimeTracker::execQueryVerbosely(QSqlQuery *q, const QString &cmd) const
{
    if (!q)
        return false;

    bool result = (cmd.isEmpty()) ? q->exec() : q->exec(cmd);

    if (q->lastError().isValid())
        qDebug() << "Query:" << q->executedQuery() << "\nError:" << q->lastError().text();

    return result;
}
