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

#include "sandboxproxy.h"
#include "worldproxy.h"
#include "proxyowner.h"

#include <QLoggingCategory>
#include <QDBusConnectionInterface>

Q_LOGGING_CATEGORY(SandboxProxyDebug, "SandboxProxy")

static QStringList sPassthroughServices = {
        QStringLiteral("org.kde.kded5"),
        QStringLiteral("org.kde.kiod5"),
        QStringLiteral("org.kde.kssld5"),
        QStringLiteral("org.kde.kglobalaccel"),
        QStringLiteral("org.kde.keyboard"),
        QStringLiteral("org.kde.kuiserver"),
        QStringLiteral("org.kde.ksmserver"),
        QStringLiteral("org.kde.JobViewServer"),
        QStringLiteral("org.kde.StatusNotifierWatcher")
        // KWallet?
        //QStringLiteral("org.kde.kwalletd5")
};

SandboxProxy::SandboxProxy(const QString &dbusAddress, QObject *parent)
    : QObject(parent)
    , mDBusAddress(dbusAddress)
    , mWorld(Q_NULLPTR)
    , mLastConnectionId(0)
{
}

SandboxProxy::~SandboxProxy()
{
}

bool SandboxProxy::isValid() const
{
    return mError.isEmpty();
}

QString SandboxProxy::error() const
{
    return mError;
}

bool SandboxProxy::connectToBus()
{
    QDBusConnection sandboxConn = QDBusConnection::connectToBus(mDBusAddress,
                                                                QStringLiteral("ksandboxproxy-bus"));
    if (!sandboxConn.isConnected()) {
        mError = sandboxConn.lastError().message();
        return false;
    }

    return true;
}


ProxyOwner* SandboxProxy::service(const QString &serviceName) const
{
    if (!mWorld->hasService(serviceName) && !mWorld->requestService(serviceName)) {
        return Q_NULLPTR;
    }

    const QString owner = mWorld->serviceOwner(serviceName);
    qCDebug(SandboxProxyDebug) << "Service:" << serviceName << ", world owner:" << owner;
    ProxyOwner *service = mProxyServices.value(owner);
    if (!service) {
        const QString name = QString::fromLatin1("ksandboxproxy-bus-%1").arg(++mLastConnectionId);
        service = new ProxyOwner(owner,
                                 QDBusConnection::connectToBus(mDBusAddress, name),
                                 mWorld, const_cast<SandboxProxy*>(this));
        mProxyServices.insert(owner, service);
    }
    return service;
}


void SandboxProxy::connectToWorld(WorldProxy *worldProxy)
{
    mWorld = worldProxy;

    for (const QString &serviceName : sPassthroughServices) {
        ProxyOwner *proxyService = service(serviceName);
        if (proxyService) {
            proxyService->addService(serviceName);
        }
    }
}


void SandboxProxy::forwardWorldSignal(const QDBusMessage &signal)
{
    QDBusMessage mappedSignal = QDBusMessage::createSignal(signal.path(), signal.interface(), signal.member());
    mappedSignal.setArguments(signal.arguments());
    mappedSignal.setAutoStartService(signal.autoStartService());
    mappedSignal.setDelayedReply(signal.isDelayedReply());

    const QString worldOwner = mWorld->serviceOwner(signal.service());
    qDebug() << "Forwarding signal from" << worldOwner << signal.path() << signal.interface() << signal.member();
    ProxyOwner *proxy = mProxyServices.value(worldOwner);
    proxy->connection().send(signal);
}
