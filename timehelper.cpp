#include "timehelper.h"

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
    return begin.isValid() && end.isValid();
}

QString TimeRange::toString() const
{
    return QString("%1 - %2").arg(begin.toString()).arg(end.toString());
}

QList<TimeRange> TimeRange::getUnion(const TimeRange &r1, const TimeRange &r2)
{
    if (!r1.isValid() || !r2.isValid())
        return QList<TimeRange>();

    if (r1.begin >= r1.end || r2.begin >= r2.end)
        return QList<TimeRange>();

    if (r1.end < r2.begin) // Non-Overlapping
        return QList<TimeRange>({r1, r2});

    TimeRange r;
    r.begin = r1.begin;
    r.end = std::max({r1.begin, r1.end, r2.begin, r2.end});

    return QList<TimeRange>({r});
}
