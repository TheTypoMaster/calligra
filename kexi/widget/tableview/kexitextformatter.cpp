/* This file is part of the KDE project
   Copyright (C) 2007-2012 Jarosław Staniek <staniek@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "kexitextformatter.h"
#include <widget/utils/kexidatetimeformatter.h>

#include <KDb>

#include <QLocale>

//! @internal
class KexiTextFormatter::Private
{
public:
    Private() : field(0), dateFormatter(0), timeFormatter(0) {
    }

    ~Private() {
        delete dateFormatter;
        delete timeFormatter;
    }

    KDbField* field;
    KexiDateFormatter *dateFormatter;
    KexiTimeFormatter *timeFormatter;
};

KexiTextFormatter::KexiTextFormatter()
        : d(new Private)
{
}

KexiTextFormatter::~KexiTextFormatter()
{
    delete d;
}

void KexiTextFormatter::setField(KDbField* field)
{
    d->field = field;
    if (!d->field)
        return;
    const KDbField::Type t = d->field->type();
    switch (t) {
    case KDbField::Date:
    case KDbField::DateTime:
        d->dateFormatter = new KexiDateFormatter();
        break;
    default:
        delete d->dateFormatter;
        d->dateFormatter = 0;
        break;
    }
    switch (t) {
    case KDbField::Time:
    case KDbField::DateTime:
        d->timeFormatter = new KexiTimeFormatter();
        break;
    default:
        delete d->timeFormatter;
        d->timeFormatter = 0;
        break;
    }
}

//! toString() implementation for Text type
static QString toStringForTextType(const QVariant& value, const QString& add,
                                   KDbField *field,
                                   bool *lengthExceeded)
{
    const QString str(value.toString());
    if (lengthExceeded) {
        if (field && field->maxLength() > 0) {
            *lengthExceeded = (str.length() + add.length()) > field->maxLength();
        }
        else {
            *lengthExceeded = false;
        }
    }
    return str + add;
}

QString KexiTextFormatter::toString(const QVariant& value, const QString& add,
                                    bool *lengthExceeded) const
{
    //cases, in order of expected frequency
    if (!d->field || d->field->type() == KDbField::Text) {
        return toStringForTextType(value, add, d->field, lengthExceeded);
    }

    if (lengthExceeded) {
        *lengthExceeded = false;
    }
    if (d->field->isIntegerType()) {
        if (value.toInt() == 0)
            return add; //eat 0
    }
    else if (d->field->isFPNumericType()) {
//! @todo precision!
//! @todo support 'g' format
        if (value.toDouble() == 0.0)
            return add.isEmpty() ? "0" : add; //eat 0
        return KDb::formatNumberForVisibleDecimalPlaces(
            value.toDouble(), d->field->visibleDecimalPlaces()) + add;
    }

    switch (d->field->type()) {
    case KDbField::Boolean: {
    //! @todo temporary solution for booleans!
        const bool boolValue = value.isNull() ? QVariant(add).toBool() : value.toBool();
        return boolValue ? "1" : "0";
    }
    case KDbField::Date:
        return d->dateFormatter->toString(
                   value.toString().isEmpty() ? QDate() : value.toDate());
    case KDbField::Time:
        return d->timeFormatter->toString(
                   value.toString().isEmpty()
                   ? QTime(99, 0, 0) //hack to avoid converting null variant to valid QTime(0,0,0)
                   : value.toTime());
    case KDbField::DateTime:
        if (value.toString().isEmpty())
            return add;
        return KexiDateTimeFormatter::toString(
                   *d->dateFormatter, *d->timeFormatter, value.toDateTime());
    case KDbField::BigInteger:
        if (value.toLongLong() == 0)
            return add; //eat 0
        break;
    default:
        break;
    }
    //default: text
    return toStringForTextType(value, add, d->field, lengthExceeded);
}

QVariant KexiTextFormatter::fromString(const QString& text) const
{
    if (!d->field)
        return QVariant();

    switch (d->field->type()) {
    case KDbField::Text:
    case KDbField::LongText:
        return text;
    case KDbField::Byte:
    case KDbField::ShortInteger:
        return text.toShort();
//! @todo uint, etc?
    case KDbField::Integer:
        return text.toInt();
    case KDbField::BigInteger:
        return text.toLongLong();
    case KDbField::Boolean:
//! @todo temporary solution for booleans!
        return text == "1";
    case KDbField::Date:
        return d->dateFormatter->stringToVariant(text);
    case KDbField::Time:
        return d->timeFormatter->stringToVariant(text);
    case KDbField::DateTime:
        return KexiDateTimeFormatter::fromString(
                   *d->dateFormatter, *d->timeFormatter, text);
    case KDbField::Float:
    case KDbField::Double: {
        // replace custom decimal symbol with '.' as required by to{Float|Double}()
        QString fixedText(text);
        QLocale locale;
        fixedText.replace(locale.decimalPoint(), '.');
        if (d->field->type() == KDbField::Double)
            return fixedText.toDouble();
        return fixedText.toFloat();
    }
    default:
        break;
    }
    return text;
//! @todo more data types!
}

bool KexiTextFormatter::valueIsEmpty(const QString& text) const
{
    if (text.isEmpty())
        return true;

    if (d->field) {
        switch (d->field->type()) {
        case KDbField::Date:
            return d->dateFormatter->isEmpty(text);
        case KDbField::Time:
            return d->timeFormatter->isEmpty(text);
        case KDbField::DateTime:
            return KexiDateTimeFormatter::isEmpty(*d->dateFormatter, *d->timeFormatter, text);
        default:
            break;
        }
    }

//! @todo
    return text.isEmpty();
}

bool KexiTextFormatter::valueIsValid(const QString& text) const
{
    if (!d->field)
        return true;
//! @todo fix for fields with "required" property = true
    if (valueIsEmpty(text)/*ok?*/)
        return true;

    switch (d->field->type()) {
    case KDbField::Date:
        return d->dateFormatter->stringToVariant(text).isValid();
    case KDbField::Time:
        return d->timeFormatter->stringToVariant(text).isValid();
    case KDbField::DateTime:
        return KexiDateTimeFormatter::isValid(*d->dateFormatter, *d->timeFormatter, text);
    default:
        break;
    }

//! @todo
    return true;
}

QString KexiTextFormatter::inputMask() const
{
    switch (d->field->type()) {
    case KDbField::Date:
//! @todo use KDateWidget?
        return d->dateFormatter->inputMask();
    case KDbField::Time:
//! @todo use KTimeWidget?
        return d->timeFormatter->inputMask();
    case KDbField::DateTime:
        return KexiDateTimeFormatter::inputMask(*d->dateFormatter, *d->timeFormatter);
    default:
        break;
    }

    return QString();
}

bool KexiTextFormatter::lengthExceeded(const QString& text) const
{
    return d->field && d->field->type() == KDbField::Text && d->field->maxLength() > 0
            && text.length() > d->field->maxLength();
}
