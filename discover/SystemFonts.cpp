/*
 *   Copyright (C) 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "SystemFonts.h"
#include <QFontDatabase>
#include <QGuiApplication>

SystemFonts::SystemFonts(QObject* parent)
    : QObject(parent)
{
    QGuiApplication::instance()->installEventFilter(this);
}

QFont SystemFonts::fixedFont() const
{
    return QFontDatabase::systemFont(QFontDatabase::FixedFont);
}

QFont SystemFonts::generalFont() const
{
    return QFontDatabase::systemFont(QFontDatabase::GeneralFont);
}

QFont SystemFonts::smallestReadableFont() const
{
    return QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont);
}

QFont SystemFonts::titleFont() const
{
    return QFontDatabase::systemFont(QFontDatabase::TitleFont);
}

bool SystemFonts::eventFilter(QObject* obj, QEvent* ev)
{
    if(ev->type() == QEvent::ApplicationFontChange) {
        emit fontsChanged();
    }
    return QObject::eventFilter(obj, ev);
}
