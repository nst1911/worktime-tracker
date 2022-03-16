#include "helper.h"
#include <QSqlError>
#include <QDebug>
#include <set>
#include <algorithm>

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
    return end >= range.begin/* || begin <= range.end*/;
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
    qSort(sorted.begin(),
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
