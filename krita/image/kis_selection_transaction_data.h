/*
 *  Copyright (c) 2005 C. Boemann <cbo@boemann.dk>
 *  Copyright (c) 2008 Sven Langkamp <sven.langkamp@gmail.com>
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

#ifndef KIS_SELECTION_TRANSACTION_DATA_H_
#define KIS_SELECTION_TRANSACTION_DATA_H_

#include "kis_transaction_data.h"

#include "kis_selection.h"

/**
 * KisSelectionTransactionData records changes to the selection for the undo stack. There
 * are two selections in Krita: the global selection and the per-layer selection mask.
 * A particular action only works with one of these selections (in the future, we may
 * want to merge the global and local selection).
 *
 * KisSelectionTransactionData remembers which selection was changed.
 */
class KRITAIMAGE_EXPORT KisSelectionTransactionData : public KisTransactionData
{

public:
    KisSelectionTransactionData(const QString& name, KisImageWSP image, KisSelectionSP selection, KUndo2Command* parent = 0);
    virtual ~KisSelectionTransactionData();

public:
    void redo();
    void undo();

private:
    KisImageWSP m_image;
    KisSelectionSP m_selection;
};

#endif /* KIS_SELECTION_TRANSACTION_DATA_H_ */
