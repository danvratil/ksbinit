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

#include "launcher.h"
#include "utils.h"
#include "sandboxproxy.h"
#include "worldproxy.h"
#include "stdinreader.h"

#include <QDBusError>
#include <QProcess>
#include <QDebug>
#include <QStringList>

#include <signal.h>

Launcher::Launcher(QObject *parent)
    : QObject(parent)
    , mKdeInitProcess(Q_NULLPTR)
    , mAppProcess(Q_NULLPTR)
    , mSandboxBusPID(-1)
    , mWorldProxy(Q_NULLPTR)
    , mSandboxProxy(Q_NULLPTR)
    , mAppRetVal(-1)
{
}

Launcher::~Launcher()
{
}

QString Launcher::errorText() const
{
    return mErrorText;
}

bool Launcher::error(const QString &error)
{
    mErrorText = error;
    return false;
}

int Launcher::retVal() const
{
    return mAppRetVal;
}


bool Launcher::connectToWorld()
{
    mWorldProxy = new WorldProxy(this);
    if (!mWorldProxy->connectToBus()) {
        error();
        delete mWorldProxy;
        mWorldProxy = Q_NULLPTR;
        return false;
    }

    return true;
}

bool Launcher::connectToSandbox()
{
    QProcess dbusProcess;
    dbusProcess.start(QStringLiteral("dbus-launch"), QIODevice::ReadOnly);
    if (!dbusProcess.waitForFinished(10000)) {
        return error(QStringLiteral("Failed to wait for dbus-launch to finish"));
    }

    dbusProcess.setReadChannel(QProcess::StandardOutput);
    QPair<QByteArray, QByteArray> pair = Utils::parseEnvVar(dbusProcess.readLine().trimmed());
    if (pair.first != "DBUS_SESSION_BUS_ADDRESS") {
        return error(QStringLiteral("Unexpected output from dbus-launch"));
    }
    mSandboxBusAddress = QString::fromLatin1(pair.second);

    pair = Utils::parseEnvVar(dbusProcess.readLine().trimmed());
    if (pair.first != "DBUS_SESSION_BUS_PID") {
        return error(QStringLiteral("Unexpected output from dbus-launch"));
    }
    mSandboxBusPID = pair.second.toLongLong();

    mSandboxProxy = new SandboxProxy(mSandboxBusAddress, this);
    if (!mSandboxProxy->connectToBus()) {
        error(mSandboxProxy->error());
        delete mSandboxProxy;
        mSandboxProxy = Q_NULLPTR;
        return false;
    }

    return true;
}

bool Launcher::setupServices()
{
    mSandboxProxy->connectToWorld(mWorldProxy);
    mWorldProxy->connectToSandbox(mSandboxProxy);

    return true;
}

bool Launcher::launchApp(const QStringList &_args, const Flags &flags)
{
    QStringList args = _args;

    QProcessEnvironment environ = QProcessEnvironment::systemEnvironment();
    environ.insert(QStringLiteral("DBUS_SESSION_BUS_ADDRESS"), mSandboxBusAddress);

    if (!(flags & NoKDEInit)) {
        mKdeInitProcess = new QProcess(this);
        mKdeInitProcess->setProgram(QStringLiteral("kdeinit5"));
        mKdeInitProcess->setProgram(QStringLiteral("kdeinit5"));
        mKdeInitProcess->setProcessEnvironment(environ);
        mKdeInitProcess->start();
        if (!mKdeInitProcess->waitForStarted()) {
            qWarning() << "Failed to start kdeinit5, but continuing...";
        }
        connect(mKdeInitProcess, &QProcess::readyReadStandardError,
                [this]() {
                    qStdErr() << mKdeInitProcess->readAllStandardError();
                });
        connect(mKdeInitProcess, &QProcess::readyReadStandardOutput,
                [this]() {
                    qStdOut() << mKdeInitProcess->readAllStandardOutput();
                });
    }

    mAppProcess = new QProcess(this);
    mAppProcess->setProgram(args.takeFirst());
    mAppProcess->setArguments(args);
    mAppProcess->setProcessEnvironment(environ);
    connect(mAppProcess, static_cast<void(QProcess::*)(int exitCode, QProcess::ExitStatus)>(&QProcess::finished),
            this, &Launcher::onAppFinished);
    connect(mAppProcess, static_cast<void(QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
            this, &Launcher::onAppError);
    // Forward app stderr and stdout to our
    connect(mAppProcess, &QProcess::readyReadStandardError,
            [this]() {
                qStdErr() << mAppProcess->readAllStandardError();
            });
    connect(mAppProcess, &QProcess::readyReadStandardOutput,
            [this]() {
                qStdOut() << mAppProcess->readAllStandardOutput();
            });
    connect(new StdInReader(mAppProcess), &StdInReader::textAvailable,
            [this](const QString &text) {
                mAppProcess->write(text.toUtf8());
            });

    mAppProcess->start(QIODevice::ReadWrite);
    if (!mAppProcess->waitForStarted()) {
        return error(QStringLiteral("Failed to start application"));
    }

    return true;
}

void Launcher::stopProcess(QProcess *process)
{
    if (process->state() == QProcess::Running) {
        process->terminate();
        if (!process->waitForFinished()) {
            process->kill();
            process->waitForFinished();
        }
    }
}

void Launcher::cleanup()
{
    if (mAppProcess) {
        stopProcess(mAppProcess);
        delete mAppProcess;
    }
    if (mKdeInitProcess) {
        stopProcess(mKdeInitProcess);
        delete mKdeInitProcess;
    }

    delete mSandboxProxy;
    delete mWorldProxy;

    if (mSandboxBusPID > 0) {
        qDebug() << "Sending SIGTERM to dbus daemon (PID" << mSandboxBusPID << ")";
        kill(mSandboxBusPID, SIGTERM);
    }
}

void Launcher::onAppError(QProcess::ProcessError error)
{
    switch (error) {
    case QProcess::FailedToStart:
        qWarning() << "Application failed to start:" << mAppProcess->errorString();
        Q_EMIT quit();
        return;
    case QProcess::Crashed:
        qWarning() << "Application crashed!";
        Q_EMIT quit();
        return;
    default:
        return;
    }
}

void Launcher::onAppFinished(int code, QProcess::ExitStatus status)
{
    if (status == QProcess::CrashExit) {
        // Handled by onAppError()
        return;
    }

    qDebug() << "Application terminated with exit code" << code;
    mAppRetVal = code;
    Q_EMIT quit();
}
