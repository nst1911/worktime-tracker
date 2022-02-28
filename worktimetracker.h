#ifndef WORKTIMETRACKER_H
#define WORKTIMETRACKER_H

#include <QSqlDatabase>
#include <QDateTime>

class WorktimeTracker
{
public:
    WorktimeTracker(const QSqlDatabase& db);

    QTime getTimeSummary(int month);

    bool insertDate(const QDate& date,
                    const QTime& scheduleBegin,
                    const QTime& scheduleEnd,
                    const QTime& arrival,
                    const QTime& leaving);
    bool insertDate(const QDate& date);

    bool setSchedule(const QTime& start, const QTime& end, const QDate& from, const QDate& to = QDate());
    bool setArrivalTime(const QTime& time = QTime(), const QDate& from = QDate(), const QDate& to = QDate());
    bool setLeavingTime(const QTime& time = QTime(), const QDate& from = QDate(), const QDate& to = QDate());

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

    struct Schedule {
        QTime begin;
        QTime end;
        inline bool isValid() const { return begin.isValid() && end.isValid(); }
    };

    Schedule getScheduleBeforeDate(const QDate& date) const;
    Schedule getSchedule(const QString& type) const;

    void test();
};

#endif // WORKTIMETRACKER_H
