/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * http://lxqt.org
 *
 * Copyright: 2016 LXQt team
 * Authors:
 *   Paulo Lieuthier <paulolieuthier@gmail.com>
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

#include "lockscreenwatcher.h"
#include <LXQt/ScreenSaver>
#include <QDBusReply>
#include <QDebug>
#include <unistd.h>

/*
 * Logind
 */

LogindProvider::LogindProvider(QObject *parent) :
    mInterface(QStringLiteral("org.freedesktop.login1"),
            QStringLiteral("/org/freedesktop/login1"),
            QStringLiteral("org.freedesktop.login1.Manager"),
            QDBusConnection::systemBus())
{
    connect(&mInterface, SIGNAL(PrepareForSleep(bool)),
            this, SIGNAL(aboutToSleep(bool)));
}

LogindProvider::~LogindProvider()
{
    release();
}

bool LogindProvider::isActive()
{
    return mInterface.isValid();
}

bool LogindProvider::inhibit()
{
    QDBusReply<QDBusUnixFileDescriptor> reply = mInterface.call(
            QStringLiteral("Inhibit"),
            QStringLiteral("sleep"),
            QStringLiteral("LXQt Power Management"),
            QStringLiteral("Start screen locker before sleep."),
            QStringLiteral("delay"));
    if (!reply.isValid())
    {
        qDebug() << "LockScreenWatcher: " << reply.error();
        return false;
    }
    else
        mFileDescriptor = reply.value();

    return true;
}

void LogindProvider::release()
{
    if (mFileDescriptor.isValid())
    {
        ::close(mFileDescriptor.fileDescriptor());
        mFileDescriptor.setFileDescriptor(-1);
    }
}

/*
 * ConsoleKit
 */

ConsoleKitProvider::ConsoleKitProvider(QObject *parent) :
    mMethodInhibitNotPresent(false),
    mInterface(QStringLiteral("org.freedesktop.ConsoleKit"),
            QStringLiteral("/org/freedesktop/ConsoleKit/Manager"),
            QStringLiteral("org.freedesktop.ConsoleKit.Manager"),
            QDBusConnection::systemBus())

{
    connect(&mInterface, SIGNAL(PrepareForSleep(bool)),
            this, SIGNAL(aboutToSleep(bool)));

    // validate interface, look for melhod Inhibit
    if (mInterface.isValid())
    {
        QDBusReply<QString> reply = QDBusInterface(
                QStringLiteral("org.freedesktop.ConsoleKit"),
                QStringLiteral("/org/freedesktop/ConsoleKit/Manager"),
                QStringLiteral("org.freedesktop.DBus.Introspectable"),
                QDBusConnection::systemBus())
            .call(QStringLiteral("Introspect"));
        mMethodInhibitNotPresent = reply.value().contains("Inhibit");
    }
}

ConsoleKitProvider::~ConsoleKitProvider()
{
    release();
}

bool ConsoleKitProvider::isActive()
{
    return mInterface.isValid() && !mMethodInhibitNotPresent;
}

bool ConsoleKitProvider::inhibit()
{
    QDBusReply<QDBusUnixFileDescriptor> reply = mInterface.call(
            QStringLiteral("Inhibit"),
            QStringLiteral("sleep"),
            QStringLiteral("LXQt Power Management"),
            QStringLiteral("Start screen locker before sleep."),
            QStringLiteral("delay"));
    if (!reply.isValid())
    {
        qDebug() << "LockScreenWatcher: " << reply.error();
        return false;
    }
    else
        mFileDescriptor = reply.value();

    return true;
}

void ConsoleKitProvider::release()
{
    if (mFileDescriptor.isValid())
    {
        ::close(mFileDescriptor.fileDescriptor());
        mFileDescriptor.setFileDescriptor(-1);
    }
}

/*
 * Watcher
 */

LockScreenWatcher::LockScreenWatcher(QObject *parent)
{
    mProvider = new LogindProvider(this);
    if (!mProvider->isActive())
    {
        mProvider->deleteLater();
        mProvider = nullptr;
    }

    mProvider = new ConsoleKitProvider(this);
    if (mProvider->isActive())
    {
        mProvider->deleteLater();
        mProvider = nullptr;
    }

    if (mProvider)
    {
        connect(mProvider, &LockScreenProvider::aboutToSleep,
                this, &LockScreenWatcher::aboutToSleep);
        mProvider->inhibit();
    }
}

LockScreenWatcher::~LockScreenWatcher()
{
}

void LockScreenWatcher::aboutToSleep(bool beforeSleep)
{
    if (beforeSleep)
    {
        // FIXME: don't use hardcoded value
        doAction(-2);
    }
}
