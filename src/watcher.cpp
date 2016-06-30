/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXDE-Qt - a lightweight, Qt based, desktop toolset
 *
 * Authors:
 *   Christian Surlykke <christian@surlykke.dk>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include <QDebug>
#include "../config/powermanagementsettings.h"
#include "watcher.h"
#include "x11helper.h"

Watcher::Watcher(QObject *parent) :
    QObject(parent),
    mScreenSaver(this)
{
    connect(&mScreenSaver, SIGNAL(done()), &mLoop, SLOT(quit()));
}

Watcher::~Watcher()
{
}

void Watcher::doAction(int action)
{
    // if (action == -1) { }
    if (action == -2 && !X11Helper::isScreenSaverLocked())
    {
        mScreenSaver.lockScreen();
        mLoop.exec();
    }
    else if (action >= 0)
        mPower.doAction((LXQt::Power::Action) action);

    emit done();
}
