#include "worktimetracker.h"
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QSqlRecord>

WorktimeTracker::WorktimeTracker(const QSqlDatabase &db, const QTime &scheduleBegin, const QTime &scheduleEnd)
    : m_db(db)
{
    // TODO: Using Q_ASSERT for checking db and time is not safe. It'd be better to hide constructor
    // in private/protected area and create WorktimeTracker instances via static method like
    // WorktimeTracker::create()

    Q_ASSERT(m_db.isOpen() && scheduleBegin.isValid() && scheduleEnd.isValid());

    m_defaultSchedule = { DEFAULT_SCHEDULE_NAME, scheduleBegin, scheduleEnd };

    initScheduleTable();
    initLeavepassTable();
    initWorktimeTable();
}

TimeSpan WorktimeTracker::getSummary(const QDate &from, const QDate &to) const
{
    if (!from.isValid() || !to.isValid())
        return TimeSpan();

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM worktime WHERE Date BETWEEN date(:from) AND date(:to)");
    query.bindValue(":from", dateToString(from));
    query.bindValue(":to", dateToString(to));

    if (!execQueryVerbosely(&query))
        return TimeSpan();

    TimeSpan ts;

    while (query.next())
    {
//        auto date = query.value("Date").toDate();

//        auto schedule = getSchedule(query.value("Schedule").toString());
//        if (!schedule.isValid())
//            return TimeSpan();

//        QList<TimeRange> debtList, bonusList;

//        auto arrivalTime = query.value("ArrivalTime").toTime();
//        auto leavingTime = query.value("LeavingTime").toTime();

//        if (arrivalTime > schedule.begin)
//            debtList.append(TimeRange(schedule.begin, arrivalTime));
//        else
//            bonusList.append(TimeRange(arrivalTime, schedule.begin));

//        if (schedule.end > leavingTime)
//            debtList.append(TimeRange(leavingTime, schedule.end));
//        else
//            bonusList.append(TimeRange(schedule.end, leavingTime));

//        auto leavePasses = getLeavePassList(date);
//        for (auto leavePass : leavePasses)
//            debtList.append(TimeRange(leavePass.from, leavePass.to));

//        for (auto debt : debtList)

    }

    return ts;
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

WorktimeTracker::Record WorktimeTracker::getRecord(const QDate &date) const
{
    if (!date.isValid())
        return Record();

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM worktime WHERE Date = date(:d)");
    query.bindValue(":d", dateToString(date));

    if (!execQueryVerbosely(&query))
        return Record();

    if (!query.next())
        return Record();

    Record r;
    r.schedule = getSchedule(query.value("Schedule").toString());
    r.date = query.value("Date").toDate();
    r.arrivalTime = query.value("ArrivalTime").toTime();
    r.leavingTime = query.value("LeavingTime").toTime();

    return r;
}

QList<WorktimeTracker::Record> WorktimeTracker::getRecords(const QDate &from, const QDate &to) const
{
    if (!from.isValid() || !to.isValid())
        return QList<Record>();

    if (from == to)
        return QList<Record>({getRecord(from)});

    QDate _from = qMin(from, to);
    QDate _to   = qMax(from, to);

    QSqlQuery query(m_db);
    query.prepare(QString("SELECT * FROM worktime WHERE Date BETWEEN date(:from) AND date(:to)"));
    query.bindValue(":from", dateToString(_from));
    query.bindValue(":to", dateToString(_to));

    if (!execQueryVerbosely(&query))
        return QList<Record>();

    QList<Record> records;

    while (query.next())
    {
        Record r;
        r.schedule = getSchedule(query.value("Schedule").toString());
        r.date = query.value("Date").toDate();
        r.arrivalTime = query.value("ArrivalTime").toTime();
        r.leavingTime = query.value("LeavingTime").toTime();
        records.append(r);
    }

    return records;
}

bool WorktimeTracker::insertRecord(const QDate &date, const QTime &arrival, const QTime &leaving, const QString &schedule)
{
    if (schedule.isEmpty() || !date.isValid())
        return false;

    // If there is no schedule with this name
    if (schedule != defaultSchedule().name && !getSchedule(schedule).isValid())
        return false;

    if (!TimeRange::valid(arrival, leaving) || TimeRange::inverted(arrival, leaving))
        return false;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO worktime VALUES (:d, :schedule, :arrival, :leaving)");
    query.bindValue(":d", dateToString(date));
    query.bindValue(":schedule", schedule);
    query.bindValue(":arrival", timeToString(arrival));
    query.bindValue(":leaving", timeToString(leaving));

    return execQueryVerbosely(&query);
}

bool WorktimeTracker::insertRecord(const QDate &date)
{
    Schedule schedule = getScheduleBeforeDate(date);
    if (!schedule.isValid())
        schedule = defaultSchedule();

    return insertRecord(date, schedule.begin, schedule.end, schedule.name);
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

    if (schedule.isEmpty() || !TimeRange::valid(begin, end) || TimeRange::inverted(begin, end))
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
    auto _time = time.isValid() ? time : QTime::currentTime();
    auto _from = from.isValid() ? from : QDate::currentDate();
    auto _to   = to.isValid() ? to : _from;

    return updateColumnData("worktime",
                            "ArrivalTime",
                            _from,
                            _to,
                            timeToString(_time),
                            "LeavingTime >= time(:data)");
}

bool WorktimeTracker::setLeavingTime(const QTime &time, const QDate &from, const QDate &to)
{
    // TODO: time must be higher than ArrivalTime

    auto _time = time.isValid() ? time : QTime::currentTime();
    auto _from = from.isValid() ? from : QDate::currentDate();
    auto _to   = to.isValid() ? to : _from;

    return updateColumnData("worktime",
                            "LeavingTime",
                            _from,
                            _to,
                            timeToString(_time),
                            "ArrivalTime <= time(:data)");
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
    // TODO: return false if there's already a leave pass with the same time

    // insertLeavePass(QTime(8,0), QTime(8,10)); -> return true
    // insertLeavePass(QTime(8,0), QTime(8,10)); -> return false

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
    return updateLeavePassData("TimeFrom", _date, id, timeToString(_time), "TimeTo >= time(:data)");
}

bool WorktimeTracker::setLeavePassEnd(const QTime &time, const QDate &date, int id)
{
    // TODO: time must be higher than TimeFrom

    auto _time = time.isValid() ? time : QTime::currentTime();
    auto _date = date.isValid() ? date : QDate::currentDate();
    return updateLeavePassData("TimeTo", _date, id, timeToString(_time), "TimeFrom <= time(:data)");
}

bool WorktimeTracker::setLeavePassComment(const QString &comment, const QDate &date, int id)
{
    auto _date = date.isValid() ? date : QDate::currentDate();
    return updateLeavePassData("Comment", _date, id, comment);
}

WorktimeTracker::Schedule WorktimeTracker::defaultSchedule() const
{
    return m_defaultSchedule;
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

    query.prepare("INSERT INTO schedule VALUES(:name,:begin,:end)");
    query.bindValue(":name", m_defaultSchedule.name);
    query.bindValue(":begin", timeToString(m_defaultSchedule.begin));
    query.bindValue(":end", timeToString(m_defaultSchedule.end));
    execQueryVerbosely(&query);

//    query.prepare("INSERT INTO schedule VALUES('floating_first_hour',:begin,:end)");
//    query.bindValue(":begin", timeToString(scheduleBegin));
//    query.bindValue(":end", timeToString(scheduleEnd));
    //    execQueryVerbosely(&query);
}

bool WorktimeTracker::updateColumnData(const QString &table, const QString &column, const QDate &from, const QDate &to, const QString& data, const QString &additionalCondition) const
{
    if (!from.isValid() && !to.isValid())
        return false;

    QSqlQuery query(m_db);

    // Swap if from > to
    QDate _from = qMin(from, to);
    QDate _to   = qMax(from, to);

    QString queryText = "UPDATE %1 SET %2=:data WHERE Date BETWEEN date(:from) AND date(:to)";
    if (!additionalCondition.isEmpty()) queryText += " AND %3";

    query.prepare(queryText.arg(table).arg(column).arg(additionalCondition));
    query.bindValue(":from", dateToString(_from));
    query.bindValue(":to", dateToString(_to));
    query.bindValue(":data", data);

    if (!execQueryVerbosely(&query))
        return false;

    query.exec("SELECT changes()");

    return query.next() ? query.value(0).toBool() : false;
}

bool WorktimeTracker::updateLeavePassData(const QString &column, const QDate &date, int id, const QString &data, const QString &additionalCondition) const
{
    if (!date.isValid())
        return false;

    QSqlQuery query(m_db);

    QString queryText = "UPDATE leavepass SET %2=:data WHERE Date = date(:d) AND Id = :id";
    if (!additionalCondition.isEmpty()) queryText += " AND %3";

    query.prepare(queryText.arg(column).arg(additionalCondition));
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

bool WorktimeTracker::Record::isValid() const
{
    return date.isValid() && schedule.isValid() && arrivalTime.isValid() && leavingTime.isValid();
}

QString WorktimeTracker::Record::toString() const
{
    return QString("worktime date=%1, schedule={%2}, arrivalTime=%3, leavingTime=%4")
            .arg(date.toString())
            .arg(schedule.toString())
            .arg(arrivalTime.toString())
            .arg(leavingTime.toString());
}
