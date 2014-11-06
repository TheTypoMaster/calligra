/*
 *  Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2014 Sven Langkamp <sven.langkamp@gmail.com>
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

#include "kis_painting_assistants_manager.h"
#include "kis_painting_assistants_decoration.h"
#include "kis_image_view.h"
#include "kis_view2.h"

#include <klocale.h>
#include <kguiitem.h>
#include <ktoggleaction.h>
#include <kactioncollection.h>

KisPaintingAssistantsManager::KisPaintingAssistantsManager(KisView2* view) : QObject(view)
    , m_imageView(0)
{

}

KisPaintingAssistantsManager::~KisPaintingAssistantsManager()
{

}

void KisPaintingAssistantsManager::setup(KActionCollection * collection)
{
    m_toggleAssistant = new KToggleAction(i18n("Show Painting Assistants"), this);
    collection->addAction("view_toggle_painting_assistants", m_toggleAssistant);

    m_toggleAssistant->setCheckedState(KGuiItem(i18n("Hide Painting Assistants")));
    updateAction();
}

void KisPaintingAssistantsManager::setView(QPointer<KisImageView> imageView)
{
    if (m_imageView) {
        m_toggleAssistant->disconnect();
        if (decoration()) {
            decoration()->disconnect(this);
        }
    }
    m_imageView = imageView;
    if (m_imageView && decoration()) {
        connect(m_toggleAssistant, SIGNAL(triggered()), decoration(), SLOT(toggleVisibility()));
        connect(decoration(), SIGNAL(assistantChanged()), SLOT(updateAction()));
    }
    updateAction();
}

void KisPaintingAssistantsManager::updateAction()
{
    if (decoration()) {
        m_toggleAssistant->setChecked(decoration()->visible());
        m_toggleAssistant->setEnabled(!decoration()->assistants().isEmpty());
    } else {
        m_toggleAssistant->setEnabled(false);
    }
}

KisPaintingAssistantsDecoration* KisPaintingAssistantsManager::decoration()
{
    if (m_imageView) {
        return m_imageView->canvasBase()->paintingAssistantsDecoration();
    }
    return 0;
}

