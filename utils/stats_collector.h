/*
 * Copyright (C) 2019 ~ 2019 Union Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@uniontech.com>
 *
 * Maintainer: zccrs <zhangjide@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef STATS_COLLECTOR_H
#define STATS_COLLECTOR_H

#include <QList>
#include <QMap>
#include <QIcon>
#include <QTimer>
#include <QHash>
#include <QThread>
#include <QUrl>
#include "rapl_read.h"

#include "system_stat.h"
#include "process_stat.h"
#include "process_entry.h"
#include "desktop_entry_stat.h"
#include "find_window_title.h"
#include "network_traffic_filter.h"

struct net_io {
    qulonglong  sentBytes;
    qulonglong  recvBytes;
    qreal       sentBps;
    qreal       recvBps;
};
using NetIO = QSharedPointer<struct net_io>;

enum FilterType { OnlyGUI, OnlyMe, AllProcess };

class StatsCollector : public QObject
{
    Q_OBJECT

public:
    static StatsCollector* instance();

Q_SIGNALS:
    void cpuStatInfoUpdated(qreal cpuPercent, const QList<double> cpuPercents, cpu_usage separatorUsage, QList<cpu_usage> cpuUsageList);
    void memStatInfoUpdated(qulonglong usedMemory,
                            qulonglong totalMemory,
                            qulonglong usedSwap,
                            qulonglong totalSwap,
                            mem_stat memStat);
    void diskStatInfoUpdated(qreal readBps, qreal writeBps);
    void networkStatInfoUpdated(qulonglong totalRecvBytes,
                                qulonglong totalSentBytes,
                                qreal recvBps,
                                qreal sentBps);

    void processSummaryUpdated(int napps, int nprocs);
    void processListUpdated(const QList<ProcessEntry> procList);
    void uptimeInfoUpdated(qulonglong uptime);
    void loadAvgInfoUpdated(qreal loadAvg1, qreal loadAvg5, qreal loadAvg15);
    void tempInfoUpdated(QList<TempInfo> infoList);
    void powerInfoUpdated(QList<PowerConsumption> infoList);

    void initialSysInfoLoaded();    // simply put here to help defer initialize some of the widgets

public Q_SLOTS:
    void start();
    void updateStatus();
    void setFilterType(FilterType type);

private:
    explicit StatsCollector(QObject *parent = nullptr);
    qreal calcProcCPUStats(ProcStat &prev,
                           ProcStat &cur,
                           CPUStat &prevCPU,        // prev total cpu stat
                           CPUStat &curCPU,         // cur total cpu stat
                           qulonglong interval,     // interval of time in 1/100th of a second
                           unsigned long hz);
    QPair<qreal, qreal> calcProcDiskIOStats(ProcStat &prev,
                                            ProcStat &cur,
                                            qulonglong interval);
#if 0
    QPair<qreal, qreal> calcProcNetIOStats(ProcStat &prev,
                                           ProcStat &cur,
                                           qulonglong interval);
#endif
    void mergeSubProcNetIO(pid_t ppid, NetIO &sum);

private:
    static StatsCollector *instancePtr;
    QThread m_workerThread;
    enum StatIndex { kLastStat = 0, kCurrentStat = 1, kStatCount = kCurrentStat + 1 };

    FilterType m_filterType {AllProcess};
    uid_t m_euid;

    int m_napps {};     // current running gui apps)
    int m_nprocs {};    // current running total procs

    qreal       m_interval {2.};
    QTimer     *m_timer {nullptr};

    struct stat_context m_statCtx {};

    time_t m_btime {};

    // helper utility to help find all gui apps
    QScopedPointer<FindWindowTitle> m_wm            {};
    QMap<pid_t, xcb_window_t>   m_trayPIDToWndMap   {};
    QList<pid_t>                m_guiPIDList        {};
    QList<pid_t>                m_appList           {};

    QMap<pid_t, NetIO>          m_procNetIO         {};

    qulonglong  m_uptime[kStatCount]         {{}, {}};
    CPUStat     m_cpuStat[kStatCount]        {{}, {}};
    CPUStatMap  m_cpuStatMap[kStatCount]     {{}, {}};
    NetIFStat   m_ifStat[kStatCount]         {{}, {}};
    DiskIOStat  m_ioStat[kStatCount]         {{}, {}};

    QMap<pid_t, ProcStat>       m_procMap[kStatCount]   {};
    QMap<ino_t, pid_t>          m_inodeToPID            {};
    QMap<pid_t, ProcessEntry>   m_procEntryMap          {};

    QMap<pid_t, pid_t>      m_pidCtoPMapping {};
    QMultiMap<pid_t, pid_t> m_pidPtoCMapping {};

    // pid <==> pid launched from
    QMap<pid_t, pid_t>      m_gioPIDMapping     {};
    // (reverse)
    QMultiMap<pid_t, pid_t> m_gioRevPIDMapping  {};
    // pid <==> launched desktop file
    QMap<pid_t, QString>    m_gioDesktopMapping {};

    QMap<uid_t, QString>    m_uidCache              {};
    QMap<gid_t, QString>    m_gidCache              {};

    DesktopEntryCache       m_desktopEntryCache     {};
    QThread                 m_cacheThread           {};
    DesktopEntryStat       *m_desktopEntryStat      {};
    QList<QString>          m_shellList             {};
    QList<QString>          m_scriptingList         {};
    QList<QByteArray>       m_envPathList           {};

    QIcon m_defaultIcon {};

    friend void readProcStatsCallback(ProcStat &ps, void *context);
    friend void setProcDisplayNameAndIcon(StatsCollector &ctx, ProcessEntry &proc, const ProcStat &ps);
    friend class SystemMonitor;
};

void readProcStatsCallback(ProcStat &ps, void *context);
void setProcDisplayNameAndIcon(StatsCollector &ctx, ProcessEntry &proc, const ProcStat &ps);

Q_DECLARE_METATYPE(cpu_usage)
Q_DECLARE_METATYPE(mem_stat)
Q_DECLARE_METATYPE(TempInfo)

#endif // STATS_COLLECTOR_H
