/*
 * Copyright 2015  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef PROXYOWNER_H
#define PROXYOWNER_H

#include <QDBusVirtualObject>
#include <QDBusConnection>

class WorldProxy;
class SandboxProxy;

class ProxyOwner : public QDBusVirtualObject
{
    Q_OBJECT
public:
    ProxyOwner(const QString &worldOwner,
              const QDBusConnection &sandboxConn,
              WorldProxy *world,
              SandboxProxy *sandbox);
    ~ProxyOwner();

    void addService(const QString &worldService);
    QDBusConnection connection() const;

    bool handleMessage(const QDBusMessage &sandboxMessage, const QDBusConnection &sandboxConn);
    QString introspect(const QString &path) const;

private:
    bool registerObjectsAndSignals(const QString &service, const QString &path);
    void connectToWorldSignal(const QString &service, const QString &path,
                              const QString &interface, const QString &member);

    QString mWorldOwner;
    QDBusConnection mSandboxConn;

    QStringList mServiceNames;

    WorldProxy *mWorld;
    SandboxProxy *mSandbox;
};

#endif // PROXYOWNER_H
