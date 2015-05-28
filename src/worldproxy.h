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

#ifndef WORLDPROXY_H
#define WORLDPROXY_H

#include <QObject>
#include <QDBusConnection>

class SandboxProxy;
class QDBusMessage;

class WorldProxy : public QObject
{
    Q_OBJECT

public:
    explicit WorldProxy(QObject *parent = Q_NULLPTR);
    ~WorldProxy();

    bool isValid() const;
    QString error() const;

    bool connectToBus();
    void connectToSandbox(SandboxProxy *sandboxProxy);

    bool hasService(const QString &service);
    bool requestService(const QString &service);

    QString serviceOwner(const QString &serviceName);

    // forward msg from sandbox to world
    QDBusMessage forwardMessage(const QDBusMessage &message);

    QDBusConnection *connection() const;

    QString introspect(const QString &service, const QString &path);

private:
    QString mError;

    QDBusConnection *mWorldConn;
    SandboxProxy *mSandbox;
};

#endif // WORLDPROXY_H
