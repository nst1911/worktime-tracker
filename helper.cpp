#include "helper.h"
#include <QSqlError>
#include <QDebug>
#include <set>
#include <algorithm>

TimeSpan::TimeSpan()
{

}

TimeSpan::TimeSpan(const TimeRange& range)
    : TimeSpan(range.begin, range.end)
{

}

TimeSpan::TimeSpan(const QTime &begin, const QTime &end)
    : TimeSpan((end.msecsSinceStartOfDay() - begin.msecsSinceStartOfDay()) / 1000)
{

}

TimeSpan::TimeSpan(qint64 _seconds)
    : seconds(_seconds)
{

}

int TimeSpan::hours() const
{
    int h = qAbs(seconds) / 3600;
    return seconds < 0 ? -h : h;
}

int TimeSpan::minutes() const
{
    int m = (qAbs(seconds) % 3600) / 60;
    return seconds < 0 ? -m : m;
}

QString TimeSpan::toString() const
{
    return QString("%1%2:%3").arg(seconds < 0 ? "-" : "").arg(qAbs(hours())).arg(qAbs(minutes()));
}

bool TimeRange::isValid() const
{
    return valid(*this);
}

bool TimeRange::isInverted() const
{
    return inverted(*this);
}

bool TimeRange::intersects(const TimeRange &range) const
{
    return intersects(*this, range);
}

bool TimeRange::contains(const TimeRange &range) const
{
    return contains(*this, range);
}

QList<TimeRange> TimeRange::unite(const TimeRange &range)
{
    return unite(*this, range);
}

QList<TimeRange> TimeRange::subtract(const TimeRange &range)
{
    return subtract(*this, range);
}

bool TimeRange::inverted(const QTime &begin, const QTime &end)
{
    return begin > end;
}

bool TimeRange::inverted(const TimeRange &range)
{
    return inverted(range.begin, range.end);
}

bool TimeRange::valid(const QTime &begin, const QTime &end)
{
    return begin.isValid() && end.isValid() && begin < end;
}

bool TimeRange::valid(const TimeRange &range)
{
    return valid(range.begin, range.end);
}

bool TimeRange::intersects(const TimeRange &t1, const TimeRange &t2)
{
    return t1.isValid() && t2.isValid() ? (t1.end >= t2.begin && t1.begin <= t2.end) : false;
}

bool TimeRange::contains(const TimeRange &t1, const TimeRange &t2)
{
    return t1.isValid() && t2.isValid()
           ? intersects(t1, t2) && (t2.begin >= t1.begin && t2.end <= t1.end)
           : false;
}

TimeRange::TimeRange()
{

}

TimeRange::TimeRange(const QTime &_begin, const QTime &_end)
    : begin(_begin), end(_end)
{

}

TimeRange::TimeRange(int beginHour, int beginMinutes, int endHour, int endMinutes)
    : begin(QTime(beginHour, beginMinutes)), end(QTime(endHour, endMinutes))
{

}

QString TimeRange::toString() const
{
    return QString("%1 - %2").arg(begin.toString()).arg(end.toString());
}

QList<TimeRange> TimeRange::unite(const TimeRange &r1, const TimeRange &r2)
{
    if (!r1.isValid())
        return r2.isValid() ? QList<TimeRange>({r2}) : QList<TimeRange>();

    if (!r2.isValid())
        return r1.isValid() ? QList<TimeRange>({r1}) : QList<TimeRange>();

    if (!r1.intersects(r2))
        return QList<TimeRange>({r1, r2});

    // r1 has to start earlier than r2
    // swap them if need
    auto _r1 = (qMin(r1.begin, r2.begin) == r1.begin) ? r1 : r2;
    auto _r2 = (_r1 == r1) ? r2 : r1;

    TimeRange r;
    r.begin = _r1.begin;
    r.end   = std::max({_r1.begin, _r1.end, _r2.begin, _r2.end});

    return QList<TimeRange>({r});
}

QList<TimeRange> TimeRange::unite(const QList<TimeRange> &ranges)
{
    if (ranges.size() <= 1)
        return ranges;

    if (ranges.size() == 2)
        return unite(ranges[0], ranges[1]);

    QList<TimeRange> sorted = ranges;
    std::sort(sorted.begin(),
              sorted.end(),
              [](const TimeRange& r1, const TimeRange& r2) {
                  return r1.begin < r2.begin;
              }
    );

    QList<TimeRange> united;

    for (auto range : sorted)
    {
        if (!range.isValid())
            continue;

        if (united.isEmpty()) {
            united.append(range);
            continue;
        }

        if (united.last().end >= range.begin)
            united.last().end = qMax(united.last().end, range.end);
        else
            united.append(range);
    }

    return united;
}

QList<TimeRange> TimeRange::subtract(const TimeRange &r1, const TimeRange &r2)
{
    // TODO: make algorithm more compact and simple

    if (!r1.isValid())
        return r2.isValid() ? QList<TimeRange>({r2}) : QList<TimeRange>();

    if (!r2.isValid())
        return r1.isValid() ? QList<TimeRange>({r1}) : QList<TimeRange>();

    Q_ASSERT(r1.begin <= r2.begin);

    // subtraction of congruent ranges returns empty (invalid) range
    if (r1 == r2)
        return QList<TimeRange>({TimeRange()});

    // if ranges are non-intersected
    if (!r1.intersects(r1, r2))
        return QList<TimeRange>({r1, r2});

    // Example: [8:00,9:00]-[8:00,8:30]=[8:30,9:00]
    if (r1.begin == r2.begin) {
        auto minmax = std::minmax(r1.end, r2.end);
        return QList<TimeRange>({TimeRange(minmax.first, minmax.second)});
    }

    // Example: [8:00,9:00]-[8:30,9:00]=[8:00,8:30]
    if (r1.end == r2.end) {
        auto minmax = std::minmax(r1.begin, r2.begin);
        return QList<TimeRange>({TimeRange(minmax.first, minmax.second)});
    }

    // Example: [8:00,9:00]-[9:00,9:30]=[8:00,8:59] and [9:01,9:30]
    if (r1.end == r2.begin)
        return subtract(TimeRange(r1.begin, r1.end.addMSecs(-MIN_TIME_UNIT_MSECS)),
                        TimeRange(r2.begin.addMSecs(MIN_TIME_UNIT_MSECS), r2.end));

    auto timePoints = QList<QTime>({r1.begin, r2.begin, r1.end, r2.end});
    std::sort(timePoints.begin(), timePoints.end());

    QList<TimeRange> result;

    if (timePoints[0] == r1.begin)
        result.append({timePoints[0], timePoints[1]});

    if (timePoints[3] == r1.end)
        result.append({timePoints[2], timePoints[3]});

    return result;
}

QList<TimeRange> TimeRange::subtract(const QList<TimeRange> &ranges)
{
    if (ranges.size() <= 1)
        return ranges;

    // Split ranges to range1 (the first element) and range2 (all the rest)
    auto ranges1 = ranges.mid(0, 1);
    auto ranges2 = ranges.mid(1);

    for (auto r2 : ranges2)
    {
        if (!r2.isValid())
            continue;
        QList<TimeRange> temp;
        for (auto r1 : ranges1)
            temp.append(subtract(r1, r2));
        ranges1 = temp;
    }

    return ranges1;
}

bool execQueryVerbosely(QSqlQuery *q, const QString &cmd)
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
