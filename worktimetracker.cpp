#include "worktimetracker.h"
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QSqlRecord>

WorktimeTracker::WorktimeTracker(const QSqlDatabase &db, const QTime &scheduleBegin, const QTime &scheduleEnd, const QTime &lunchBegin, const QTime &lunchEnd)
    : m_db(db)
{
    // TODO: Using Q_ASSERT for checking db and time is not safe. It'd be better to hide constructor
    // in private/protected area and create WorktimeTracker instances via static method like
    // WorktimeTracker::create()

    Q_ASSERT(m_db.isOpen());

    // Use temp schedule object to check if time arguments are valid
    Schedule temp = { DEFAULT_SCHEDULE_NAME,
                      scheduleBegin,
                      scheduleEnd,
                      lunchBegin,
                      lunchEnd };

    Q_ASSERT(temp.isValid());

    m_defaultSchedule = temp;

    initScheduleTable();
    initLeavepassTable();
    initWorktimeTable();
}

TimeSpan WorktimeTracker::getSummary(const QDate &from, const QDate &to) const
{
    if (!from.isValid())
        return TimeSpan();

    auto _from = from;
    auto _to   = to.isValid() ? to : _from; // if 'to' is invalid, to = from

    if (_from > _to)
        qSwap(_from, _to);

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM worktime WHERE Date BETWEEN date(:from) AND date(:to)");
    query.bindValue(":from", dateToString(_from));
    query.bindValue(":to", dateToString(_to));

    if (!execQueryVerbosely(&query))
        return TimeSpan();

    TimeSpan ts;

    while (query.next())
    {
        auto date = query.value("Date").toDate();

        qDebug() << date;

        auto schedule = getSchedule(query.value("Schedule").toString());
        if (!schedule.isValid())
            return TimeSpan();

        QList<TimeRange> debtList;
        QList<TimeRange> overtimeList;

        // Fill debt and overtime lists by calculating arrival/leaving time

        // TODO: exclude lunch time

        auto checkIn = query.value("CheckIn").toTime();

        if (checkIn > schedule.begin)
            debtList.append(TimeRange(schedule.begin, checkIn));
        else
            overtimeList.append(TimeRange(checkIn, schedule.begin));

        auto checkOut = query.value("CheckOut").toTime();

        if (schedule.end > checkOut)
            debtList.append(TimeRange(checkOut, schedule.end));
        else
            overtimeList.append(TimeRange(schedule.end, checkOut));

        // Fill debt and overtime lists by calculating leave passes

        auto leavePasses = getLeavePassList(date);
        for (auto leavePass : leavePasses)
            debtList.append(TimeRange(leavePass.from, leavePass.to));

        // Time ranges can overlap each other so they have to
        // be merged by unite()
        debtList = TimeRange::unite(debtList);
        overtimeList = TimeRange::unite(overtimeList);

        for (auto debt : debtList)
            ts.seconds -= TimeSpan(debt).seconds;

        for (auto overtime : overtimeList)
            ts.seconds += TimeSpan(overtime).seconds;
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
    r.checkIn = query.value("CheckIn").toTime();
    r.checkOut = query.value("CheckOut").toTime();

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
        r.checkIn = query.value("CheckIn").toTime();
        r.checkOut = query.value("CheckOut").toTime();
        records.append(r);
    }

    return records;
}

bool WorktimeTracker::insertRecord(const QDate &date, const QTime &checkIn, const QTime &checkOut, const QString &schedule)
{
    if (schedule.isEmpty() || !date.isValid())
        return false;

    // If there is no schedule with this name

    auto s = getSchedule(schedule);

    if (schedule != defaultSchedule().name && !getSchedule(schedule).isValid())
        return false;

    if (!TimeRange::valid(checkIn, checkOut) || TimeRange::inverted(checkIn, checkOut))
        return false;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO worktime VALUES (:d, :schedule, :arrival, :leaving)");
    query.bindValue(":d", dateToString(date));
    query.bindValue(":schedule", schedule);
    query.bindValue(":arrival", timeToString(checkIn));
    query.bindValue(":leaving", timeToString(checkOut));

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

bool WorktimeTracker::insertSchedule(const QString &name, const QTime &begin, const QTime &end, const QTime &lunchBegin, const QTime &lunchEnd)
{
    if (name.isEmpty())
        return false;

    TimeRange schedule(begin, end);
    TimeRange lunch(lunchBegin, lunchEnd);

    if (!schedule.isValid() || schedule.isInverted())
        return false;

    if (!lunch.isValid() || lunch.isInverted() || !schedule.contains(lunch))
        return false;

    QSqlQuery query(m_db);

    query.prepare("INSERT INTO schedule VALUES(:schedule,:begin,:end,:lunchBegin,:lunchEnd)");
    query.bindValue(":schedule", name);
    query.bindValue(":begin", timeToString(begin));
    query.bindValue(":end", timeToString(end));
    query.bindValue(":lunchBegin", timeToString(lunchBegin));
    query.bindValue(":lunchEnd", timeToString(lunchEnd));
    return execQueryVerbosely(&query);
}

bool WorktimeTracker::setCheckIn(const QTime &time, const QDate &from, const QDate &to)
{
    if (!time.isValid())
        return false;

    auto _from = from.isValid() ? from : QDate::currentDate();
    auto _to   = to.isValid() ? to : _from;

    return updateColumnData("worktime",
                            "CheckIn",
                            _from,
                            _to,
                            timeToString(time),
                            "CheckOut >= time(:data)");
}

bool WorktimeTracker::setCheckOut(const QTime &time, const QDate &from, const QDate &to)
{
    if (!time.isValid())
        return false;

    auto _from = from.isValid() ? from : QDate::currentDate();
    auto _to   = to.isValid() ? to : _from;

    return updateColumnData("worktime",
                            "CheckOut",
                            _from,
                            _to,
                            timeToString(time),
                            "CheckIn <= time(:data)");
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
        lp.from    = stringToTime(query.value("Begin").toString());
        lp.to      = stringToTime(query.value("End").toString());
        lp.comment = query.value("Comment").toString();
        leavePassList.append(lp);
    }

    return leavePassList;
}

bool WorktimeTracker::insertLeavePass(const QTime &from, const QTime &to, const QDate &date, const QString &comment)
{
    // TODO: constraint from and to with schedule begin and end of this date

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
    if (!time.isValid())
        return false;

    auto _date = date.isValid() ? date : QDate::currentDate();
    return updateLeavePassData("Begin", _date, id, timeToString(time), "End >= time(:data)");
}

bool WorktimeTracker::setLeavePassEnd(const QTime &time, const QDate &date, int id)
{
    if (!time.isValid())
        return false;

    auto _date = date.isValid() ? date : QDate::currentDate();
    return updateLeavePassData("End", _date, id, timeToString(time), "Begin <= time(:data)");
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
                               "    CheckIn TEXT,"
                               "    CheckOut TEXT"
                               ")");
}

void WorktimeTracker::initLeavepassTable()
{
    QSqlQuery query(m_db);

    execQueryVerbosely(&query, "CREATE TABLE leavepass ("
                               "    Date TEXT NOT NULL,"
                               "    Id INT,"
                               "    Begin TEXT,"
                               "    End TEXT,"
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
                               "    End TEXT,"
                               "    LunchTimeBegin TEXT,"
                               "    LunchTimeEnd   TEXT"
                               ")");

    query.prepare("INSERT INTO schedule VALUES(:name,:begin,:end,:lunchBegin,:lunchEnd)");
    query.bindValue(":name", m_defaultSchedule.name);
    query.bindValue(":begin", timeToString(m_defaultSchedule.begin));
    query.bindValue(":end", timeToString(m_defaultSchedule.end));
    query.bindValue(":lunchBegin", timeToString(m_defaultSchedule.lunchTimeBegin));
    query.bindValue(":lunchEnd", timeToString(m_defaultSchedule.lunchTimeEnd));
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
    s.lunchTimeBegin = stringToTime(query.value("LunchTimeBegin").toString());
    s.lunchTimeEnd = stringToTime(query.value("LunchTimeEnd").toString());
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
    TimeRange scheduleTime(begin, end);
    TimeRange lunchTime(lunchTimeBegin, lunchTimeEnd);

    return  !name.isEmpty() &&
            scheduleTime.isValid() && lunchTime.isValid() &&
            !scheduleTime.isInverted() && !lunchTime.isInverted() &&
            scheduleTime.contains(lunchTime);
}

QString WorktimeTracker::Schedule::toString() const
{
    return QString("schedule name=%1, begin=%2, end=%3, lunchTimeBegin=%4, lunchTimeEnd=%5")
            .arg(name)
            .arg(begin.toString())
            .arg(end.toString())
            .arg(lunchTimeBegin.toString())
            .arg(lunchTimeEnd.toString());
}

bool WorktimeTracker::Schedule::operator==(const WorktimeTracker::Schedule &s)
{
    return name == s.name &&
           begin == s.begin && end == s.end &&
           lunchTimeBegin == s.lunchTimeBegin && lunchTimeEnd == s.lunchTimeEnd;
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
    return date.isValid() && schedule.isValid() && checkIn.isValid() && checkOut.isValid();
}

QString WorktimeTracker::Record::toString() const
{
    return QString("worktime date=%1, schedule={%2}, checkIn=%3, checkOut=%4")
            .arg(date.toString())
            .arg(schedule.toString())
            .arg(checkIn.toString())
            .arg(checkOut.toString());
}
