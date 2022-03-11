#ifndef WORKTIMETRACKER_H
#define WORKTIMETRACKER_H

#include <QSqlDatabase>
#include <QDateTime>
#include "timehelper.h"

class WorktimeTracker
{
public:
    struct Schedule;
    struct LeavePass;
    struct Worktime;

    WorktimeTracker(const QSqlDatabase& db);

    TimeSpan getSummary(const QDate& from, const QDate& to) const;
    TimeSpan getSummary(int month, int year = -1);

    bool insertDate(const QDate& date,
                    const QTime& arrival,
                    const QTime& leaving,
                    const QString& schedule = "default");
    bool insertDate(const QDate& date);


    Schedule getScheduleBeforeDate(const QDate& date) const;
    Schedule getSchedule(const QString& type) const;
    bool setSchedule(const QString& schedule, const QDate& from, const QDate& to = QDate());
    bool insertSchedule(const QString& schedule, const QTime& begin, const QTime& end);

    bool setArrivalTime(const QTime& time = QTime(), const QDate& from = QDate(), const QDate& to = QDate());
    bool setLeavingTime(const QTime& time = QTime(), const QDate& from = QDate(), const QDate& to = QDate());

    QList<LeavePass> getLeavePassList(const QDate& date) const;
    bool insertLeavePass(const QTime& from, const QTime& to, const QDate& date = QDate(), const QString& comment = QString());
    bool setLeavePassBegin(const QTime& time, const QDate& date = QDate(), int id = 0);
    bool setLeavePassEnd(const QTime& time, const QDate& date = QDate(), int id = 0);
    bool setLeavePassComment(const QString& comment, const QDate& date = QDate(), int id = 0);

private:
    QSqlDatabase m_db;

    void initWorktimeTable();
    void initLeavepassTable();
    void initScheduleTable();

    inline QString dateToString(const QDate& date) const {
        return date.toString(Qt::ISODate);
    }
    inline QString timeToString(const QTime& time) const {
        return time.toString(Qt::ISODate);
    }

    inline QDate stringToDate(const QString& str) const {
        return QDate::fromString(str, Qt::ISODate);
    }
    inline QTime stringToTime(const QString& str) const {
        return QTime::fromString(str, Qt::ISODate);
    }

    bool execQueryVerbosely(QSqlQuery* q, const QString& cmd = QString()) const;

    bool updateColumnData(const QString &table, const QString& column, const QDate &from, const QDate& to, const QString &data) const;
    bool updateLeavePassData(const QString& column, const QDate& date, int id, const QString& data) const;

    void test();
};

struct WorktimeTracker::Schedule
{
    QString name;
    QTime begin;
    QTime end;

    bool isValid() const;
    QString toString() const;
};

struct WorktimeTracker::LeavePass
{
    QDate date;
    int   id;
    QTime from;
    QTime to;
    QString comment;

    bool isValid() const;
    QString toString() const;
};

struct WorktimeTracker::Worktime
{
    QDate date;
    Schedule schedule;
    QTime arrivalTime;
    QTime leavingTime;

    bool isValid() const;
    QString toString() const;
};

#endif // WORKTIMETRACKER_H
