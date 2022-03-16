#ifndef WORKTIMETRACKER_H
#define WORKTIMETRACKER_H

#include <QSqlDatabase>
#include <QDateTime>
#include "helper.h"

// TODO:
// rename:
// leave pass -> debt
// arrival time -> check in
// leaving time -> check out

class WorktimeTracker
{
public:
    struct Schedule
    {
        QString name;
        QTime   begin, end;
        bool    isValid() const;
        QString toString() const;
    };
    struct LeavePass
    {
        QDate   date;
        int     id;
        QTime   from, to;
        QString comment;
        bool    isValid() const;
        QString toString() const;
    };
    struct Record
    {
        QDate    date;
        Schedule schedule;
        QTime    arrivalTime, leavingTime;
        bool     isValid() const;
        QString  toString() const;
    };

    WorktimeTracker(const QSqlDatabase& db,
                    const QTime& scheduleBegin = QTime(8, 0),
                    const QTime& scheduleEnd = QTime(17, 0));

    TimeSpan getSummary(const QDate& from, const QDate& to = QDate()) const;
    TimeSpan getSummary(int month, int year = -1);

    Record getRecord(const QDate& date) const;
    QList<Record> getRecords(const QDate& from, const QDate& to) const;

    bool insertRecord(const QDate& date,
                      const QTime& arrival,
                      const QTime& leaving,
                      const QString& schedule = DEFAULT_SCHEDULE_NAME);
    bool insertRecord(const QDate& date);


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

    Schedule defaultSchedule() const;

private:
    QSqlDatabase m_db;

    Schedule m_defaultSchedule;
    static constexpr auto DEFAULT_SCHEDULE_NAME = "default";

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

    bool updateColumnData(const QString &table,
                          const QString& column,
                          const QDate &from,
                          const QDate& to,
                          const QString &data,
                          const QString& additionalCondition = QString()) const;

    bool updateLeavePassData(const QString& column,
                             const QDate& date,
                             int id,
                             const QString& data,
                             const QString& additionalCondition = QString()) const;
};

#endif // WORKTIMETRACKER_H
