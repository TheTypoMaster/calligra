/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Dymo <cloudtemple@mksat.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include <map>

#include <klocale.h>

#include <qpainter.h>

#include "canvpagefooter.h"
#include "property.h"

CanvasPageFooter::CanvasPageFooter(int x, int y, int width, int height, QCanvas * canvas):
    CanvasBand(x, y, width, height, canvas)
{
    props["Height"] = *(new PropPtr(new Property(IntegerValue, "Height", i18n("Page footer's height"), "50")));
}

void CanvasPageFooter::draw(QPainter &painter)
{
    painter.drawText(rect(), AlignVCenter | AlignLeft,
        i18n("Page Footer"));
    CanvasBand::draw(painter);
}

QString CanvasPageFooter::getXml()
{
    return "\t<PageFooter PrintFrequency=\"1\"" + CanvasBand::getXml() + "\t</PageFooter>\n\n";
}
