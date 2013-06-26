/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2013  Luke Wolf <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef STEPSTEPSTACK_H
#define STEPSTEPSTACK_H

#include <QtCore/QObject>
#include <QtCore/QStack>

class StepStepBase;
class StepStepStack_p;
class StepStepStack :  QObject
{

  Q_OBJECT
public:
    StepStepStack();
    StepStepStack(const StepStepStack& other);
    ~StepStepStack();
    StepStepStack& operator=(const StepStepStack& other);
    bool operator==(const StepStepStack& other);

    StepStepBase at(int i);
    StepStepBase pop();
    StepStepBase top();

    void serialize(QString Filename);
    void deserialize(QString Filename);

    void insertAt(int i, StepStepBase step);
    void removeAt(int i);

    void push(StepStepBase* step);

private:
    StepStepStack_p* d;
};

#endif // STEPSTEPSTACK_H
