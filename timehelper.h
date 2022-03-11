#ifndef TIMEHELPER_H
#define TIMEHELPER_H

#include <QString>
#include <QTime>

struct TimeSpan
{
    qint64 seconds = 0;

    int hours() const;
    int minutes() const;
    QString toString() const;
};

struct TimeRange
{
    QTime begin;
    QTime end;

    bool isValid() const;
    QString toString() const;

    static QList<TimeRange> getUnion(const TimeRange& r1, const TimeRange& r2);
};


#endif // TIMEHELPER_H
