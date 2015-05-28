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

#ifndef SANDBOXPROXY_H
#define SANDBOXPROXY_H

#include <QObject>
#include <QDBusConnection>
#include <QVector>

class WorldProxy;
class ProxyOwner;
class QDBusMessage;


class SandboxProxy : public QObject
{
    Q_OBJECT

public:
    explicit SandboxProxy(const QString &dbusAddress,
                          QObject *parent = Q_NULLPTR);
    ~SandboxProxy();

    bool isValid() const;
    QString error() const;

    bool connectToBus();
    void connectToWorld(WorldProxy *worldProxy);

    ProxyOwner* service(const QString &service) const;

public Q_SLOTS:
    void forwardWorldSignal(const QDBusMessage &msg);

private:
    QString mError;

    QString mDBusAddress;

    WorldProxy *mWorld;

    mutable uint mLastConnectionId;
    mutable QHash<QString, ProxyOwner*> mProxyServices;
};

#endif // SANDBOXPROXY_H
