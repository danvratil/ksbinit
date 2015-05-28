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

#include "proxyowner.h"
#include "utils.h"

#include "worldproxy.h"
#include "sandboxproxy.h"

#include <QStringBuilder>
#include <QDBusMessage>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QDebug>

#include <QXmlStreamReader>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(ProxyOwnerDebug, "ProxyOwner");

ProxyOwner::ProxyOwner(const QString &worldOwner,
                       const QDBusConnection &sandboxConn,
                       WorldProxy *world,
                       SandboxProxy *sandbox)
    : QDBusVirtualObject()
    , mWorldOwner(worldOwner)
    , mSandboxConn(sandboxConn)
    , mWorld(world)
    , mSandbox(sandbox)
{
}

ProxyOwner::~ProxyOwner()
{
}

void ProxyOwner::addService(const QString &serviceName)
{
    mServiceNames << serviceName;
    mSandboxConn.registerService(serviceName);

    // Introspect only once
    if (mServiceNames.size() == 1) {
        registerObjectsAndSignals(serviceName, QStringLiteral("/"));
    }
}

QDBusConnection ProxyOwner::connection() const
{
    return mSandboxConn;
}

bool ProxyOwner::handleMessage(const QDBusMessage &sandboxMsg,
                               const QDBusConnection &sandboxConn)
{
    // Map the service from sandbox to world
    QDBusMessage mappedMsg = QDBusMessage::createMethodCall(mWorldOwner,
                                                            sandboxMsg.path(),
                                                            sandboxMsg.interface(),
                                                            sandboxMsg.member());
    mappedMsg.setArguments(sandboxMsg.arguments());
    mappedMsg.setDelayedReply(sandboxMsg.isDelayedReply());
    mappedMsg.setAutoStartService(sandboxMsg.autoStartService());
    qCDebug(ProxyOwnerDebug) << "Relaying message" << sandboxMsg.service()
                                                   << sandboxMsg.path()
                                                   << sandboxMsg.interface()
                                                   << sandboxMsg.member()
                                                   << "to"
                                                   << mappedMsg.service();
    const QDBusMessage reply = mWorld->forwardMessage(mappedMsg);

    QDBusMessage mappedReply;
    if (reply.type() == QDBusMessage::ErrorMessage) {
        mappedReply = sandboxMsg.createError(reply.errorName(), reply.errorMessage());
    } else if (reply.type() == QDBusMessage::ReplyMessage) {
        mappedReply = sandboxMsg.createReply(reply.arguments());
    }

    sandboxConn.send(mappedReply);
    return true;
}

QString ProxyOwner::introspect(const QString &path) const
{
    Q_ASSERT(!mServiceNames.isEmpty());

    QString xml = mWorld->introspect(mServiceNames.at(0), path);
    // We are only interested in the self node (hence <node>)
    const int start = xml.indexOf(QStringLiteral("<node>")) + 6; // strlen("<node>")
    const int end = xml.indexOf(QStringLiteral("</node>"));
    return xml.mid(start, end - start);
}

bool ProxyOwner::registerObjectsAndSignals(const QString &serviceName, const QString &path)
{
    const QString introspected = mWorld->introspect(serviceName, path);
    QXmlStreamReader reader(introspected);
    bool isNonLeaf = false;

    QString interface;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.name() == QLatin1String("interface")) {
            if (reader.tokenType() == QXmlStreamReader::StartElement) {
                interface = reader.attributes().value(QStringLiteral("name")).toString();
            } else {
                interface.clear();
            }
            continue;
        }

        if (reader.tokenType() != QXmlStreamReader::StartElement) {
            continue;
        }

        if (reader.name() == QLatin1String("signal")) {
            Q_ASSERT(!interface.isEmpty());

            const QString signalName = reader.attributes().value(QStringLiteral("name")).toString();
            connectToWorldSignal(serviceName, path, interface, signalName);
            continue;
        }

        if (reader.name() == QLatin1String("node")) {
            const QXmlStreamAttributes attrs = reader.attributes();
            if (attrs.hasAttribute(QStringLiteral("name"))) {
                isNonLeaf = true;
                QString objPath;
                if (path.endsWith(QLatin1Char('/'))) {
                    objPath = path + attrs.value(QStringLiteral("name")).toString();
                } else {
                    objPath = path + QLatin1Char('/') + attrs.value(QStringLiteral("name")).toString();
                }

                // Only register object on this path if it's a leaf object
                if (!registerObjectsAndSignals(serviceName, objPath)) {
                    qCDebug(ProxyOwnerDebug) << "Registering virtual object on service:" << serviceName << ", path: " << objPath;
                    mSandboxConn.registerVirtualObject(objPath, this, QDBusConnection::SingleNode);
                }
            }
            continue;
        }
    }

    return isNonLeaf;
}

void ProxyOwner::connectToWorldSignal(const QString &service, const QString &path,
                                      const QString &interface, const QString &member)
{
    Q_UNUSED(service);
    mWorld->connection()->connect(mWorldOwner, path, interface, member,
                                  mSandbox, SLOT(forwardWorldSignal(QDBusMessage)));
}
