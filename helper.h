#ifndef HELPER_H
#define HELPER_H

#include <QString>
#include <QTime>
#include <QSqlQuery>

struct TimeRange
{
    TimeRange();
    TimeRange(const QTime& _begin, const QTime& _end);
    TimeRange(int beginHour, int beginMinutes, int endHour, int endMinutes);

    QTime begin;
    QTime end;

    QString toString() const;

    bool isValid() const;
    bool isInverted() const;

    bool intersects(const TimeRange& range) const;

    static bool inverted(const QTime& begin, const QTime& end);
    static bool inverted(const TimeRange& range);

    static bool valid(const QTime& begin, const QTime& end);
    static bool valid(const TimeRange& range);

    static QList<TimeRange> unite(const TimeRange& r1, const TimeRange& r2);
    static QList<TimeRange> unite(const QList<TimeRange>& ranges);
};

inline bool operator==(const TimeRange& r1, const TimeRange& r2) {
    return (r1.begin.msecsSinceStartOfDay() == r2.begin.msecsSinceStartOfDay() &&
            r1.end.msecsSinceStartOfDay() == r2.end.msecsSinceStartOfDay());
}

inline uint qHash(const TimeRange& r, uint seed) {
    return qHash(r.begin, seed) ^ qHash(r.end, seed);
}

struct TimeSpan
{
    TimeSpan();
    TimeSpan(const TimeRange &range);
    TimeSpan(const QTime& begin, const QTime& end);
    TimeSpan(qint64 seconds);

    qint64 seconds = 0;

    int hours() const;
    int minutes() const;
    QString toString() const;
};

bool execQueryVerbosely(QSqlQuery* q, const QString& cmd = QString());

#endif // HELPER_H
