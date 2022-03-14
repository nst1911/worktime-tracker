#ifndef WORKTIMETRACKERTEST_H
#define WORKTIMETRACKERTEST_H

#include <QObject>
#include <QSqlDatabase>
#include <QtTest/QTest>
#include "worktimetracker.h"

class TestWorktimeTracker : public QObject
{
    Q_OBJECT

public:
    static WorktimeTracker example(const QSqlDatabase& db);

private slots:
    void insertSchedule();
    void insertRecord();
    void getRecord();
    void getRecords();
    void getScheduleBeforeDate();
    void getSchedule();
    void setSchedule();
    void setArrivalTime();
    void setLeavingTime();
    void getLeavePassList();
    void insertLeavePass();
    void setLeavePassBegin();
    void setLeavePassEnd();
    void setLeavePassComment();

private:
    QSqlDatabase createDb() const;
    void clear(QSqlDatabase* db);
};

#endif // WORKTIMETRACKERTEST_H
