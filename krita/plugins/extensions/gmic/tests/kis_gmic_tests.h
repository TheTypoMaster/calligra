/*
 *  Copyright (c) 2013 Lukáš Tvrdý <lukast.dev@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
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

#ifndef _KIS_GMIC_TESTS_H_
#define _KIS_GMIC_TESTS_H_

#include <QtTest>
#include <QImage>
#include <gmic.h>

// #define RUN_FILTERS

class KisGmicBlacklister;

class KisGmicFilterSetting;
class Component;

class KisGmicTests : public QObject
{
    Q_OBJECT

private:
    bool filterWithGmic(KisGmicFilterSetting * gmicFilterSetting, const QString &fileName, gmic_list<float> &images);

    QString filePathify(const QString &filterName);
    bool isAlreadyThere(QString fileName);

private:
    Component * m_root;
    QImage m_qimage;
    gmic_list<float> m_images;
    gmic_image<float> m_gmicImage;
    QString m_blacklistFilePath;
    KisGmicBlacklister * m_blacklister;

private slots:
    void initTestCase();
    void cleanupTestCase();

#ifdef RUN_FILTERS
    void testColorizeFilter();
#endif
    /**
     * This test case takes all filters parsed from gmic definition @file gmic_def.gmic
     * and counts them. The count is compared to GMIC_FILTER_COUNT
     *
     * If you define RUN_FILTERS in compilation, it will also try to run all filters on specified image.
     * This is off by default, because it takes longer time and it is important to run it like this only sometimes (e.g. when gmic is updated).
     * It is used for finding filters that might crash Krita due to unsupported feature required from gmic
     *
     */
    void testAllFilters();
    void testBlacklister();
    void testGatherLayers();

    void testConvertGrayScaleGmic();
    void testConvertGrayScaleAlphaGmic();
    void testConvertRGBgmic();
    void testConvertRGBAgmic();

    void testFilterOnlySelection();


};

#endif
