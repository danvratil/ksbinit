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

#include "worldproxy.h"
#include "sandboxproxy.h"

#include <QLoggingCategory>

#include <QDBusConnectionInterface>
#include <QDBusInterface>

Q_LOGGING_CATEGORY(WorldProxyDebug, "WorldProxy")

WorldProxy::WorldProxy(QObject* parent)
    : QObject(parent)
    , mWorldConn(Q_NULLPTR)
    , mSandbox(Q_NULLPTR)
{

}

WorldProxy::~WorldProxy()
{
}

bool WorldProxy::isValid() const
{
    return mError.isEmpty();
}

QString WorldProxy::error() const
{
    return mError;
}


QDBusConnection* WorldProxy::connection() const
{
    return mWorldConn;
}


bool WorldProxy::connectToBus()
{
    mWorldConn = new QDBusConnection(QDBusConnection::sessionBus());
    if (!mWorldConn->isConnected()) {
        mError = mWorldConn->lastError().message();
        delete mWorldConn;
        mWorldConn = Q_NULLPTR;
        return false;
    }
    return true;
}

void WorldProxy::connectToSandbox(SandboxProxy *sandboxProxy)
{
    mSandbox = sandboxProxy;
}

bool WorldProxy::hasService(const QString &service)
{
    return mWorldConn->interface()->isServiceRegistered(service);
}

bool WorldProxy::requestService(const QString &service)
{
    qCDebug(WorldProxyDebug) << "Requesting service" << service;
    QDBusReply<void> reply = mWorldConn->interface()->startService(service);
    if (reply.isValid()) {
        qCDebug(WorldProxyDebug) << "Service" << service << "started succesfully";
        return true;
    } else {
        qCDebug(WorldProxyDebug) << "Failed to start service" << service << ":" << reply.error().message();
        return false;
    }
}

QString WorldProxy::serviceOwner(const QString &serviceName)
{
    return mWorldConn->interface()->serviceOwner(serviceName);
}


QDBusMessage WorldProxy::forwardMessage(const QDBusMessage &message)
{
    return mWorldConn->call(message, QDBus::Block);
}

QString WorldProxy::introspect(const QString &service, const QString &path)
{
    const QDBusMessage msg = QDBusMessage::createMethodCall(service, path,
                                                            QStringLiteral("org.freedesktop.DBus.Introspectable"),
                                                            QStringLiteral("Introspect"));
    QDBusMessage reply = mWorldConn->call(msg, QDBus::Block);
    if (reply.type() == QDBusMessage::ReplyMessage) {
        if (reply.signature() == QLatin1String("s")) {
            return reply.arguments().at(0).toString();
        }
    }

    qCWarning(WorldProxyDebug) << "Error when introspecting service" << service << ", path" << path << ":" << reply.arguments();
    return QString();
}
