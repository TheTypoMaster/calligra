/*
 *  Copyright (c) 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KIS_IMAGE_CONFIG_H_
#define KIS_IMAGE_CONFIG_H_

#include <kconfiggroup.h>
#include "kritaimage_export.h"


class KRITAIMAGE_EXPORT KisImageConfig
{
public:
    KisImageConfig();
    ~KisImageConfig();

    bool enableProgressReporting(bool requestDefault = false) const;
    void setEnableProgressReporting(bool value);

    bool enablePerfLog(bool requestDefault = false) const;
    void setEnablePerfLog(bool value);

    qreal transformMaskOffBoundsReadArea() const;

    int updatePatchHeight() const;
    void setUpdatePatchHeight(int value);
    int updatePatchWidth() const;
    void setUpdatePatchWidth(int value);

    qreal maxCollectAlpha() const;
    qreal maxMergeAlpha() const;
    qreal maxMergeCollectAlpha() const;
    qreal schedulerBalancingRatio() const;
    void setSchedulerBalancingRatio(qreal value);

    int maxSwapSize(bool requestDefault = false) const;
    void setMaxSwapSize(int value);

    int swapSlabSize() const;
    void setSwapSlabSize(int value);

    int swapWindowSize() const;
    void setSwapWindowSize(int value);

    int tilesHardLimit() const; // MiB
    int tilesSoftLimit() const; // MiB
    int poolLimit() const; // MiB

    qreal memoryHardLimitPercent(bool requestDefault = false) const; // % of total RAM
    qreal memorySoftLimitPercent(bool requestDefault = false) const; // % of memoryHardLimitPercent() * (1 - 0.01 * memoryPoolLimitPercent())
    qreal memoryPoolLimitPercent(bool requestDefault = false) const; // % of memoryHardLimitPercent()
    void setMemoryHardLimitPercent(qreal value);
    void setMemorySoftLimitPercent(qreal value);
    void setMemoryPoolLimitPercent(qreal value);

    static int totalRAM(); // MiB

    /**
     * @return a specific directory for the swapfile, if set. If not set, return an
     * empty QString and use the default KDE directory.
     */
    QString swapDir(bool requestDefault = false);
    void setSwapDir(const QString &swapDir);

private:
    Q_DISABLE_COPY(KisImageConfig)

private:
    KConfigGroup m_config;
};


#endif /* KIS_IMAGE_CONFIG_H_ */

