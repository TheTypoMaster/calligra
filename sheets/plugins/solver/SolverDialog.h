/* This file is part of the KDE project
   Copyright (C) 2006 Stefan Nikolaus <stefan.nikolaus@kdemail.net>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KSPREAD_SOLVER_DIALOG
#define KSPREAD_SOLVER_DIALOG

#include <kdialog.h>

#include "ui_SolverMainWidget.h"
#include "ui_SolverDetailsWidget.h"

class QWidget;

namespace Calligra
{
namespace Sheets
{
class Selection;

namespace Plugins
{

/**
 * \class SolverDialog
 * \author Stefan Nikolaus <stefan.nikolaus@kdemail.net>
 */
class SolverDialog : public KDialog, public Ui::Solver, public Ui::SolverDetails
{
    Q_OBJECT

public:
    /**
     * Constructor.
     * \p view The view pointer. Used to access the cells.
     * \p parent the parent widget
     */
    SolverDialog(Selection* selection, QWidget* parent);

protected Q_SLOTS:
    /**
     * Deletes the dialog.
     * Called after one of button is clicked and the dialog will be closed.
     */
    void finishDialog();
};

} // namespace Plugins
} // namespace Sheets
} // namespace Calligra

#endif
