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

#ifndef KSBLAUNCHER_H
#define KSBLAUNCHER_H

#include <QObject>
#include <QProcess>
#include <QDBusConnection>

class QProcess;

class WorldProxy;
class SandboxProxy;

class Launcher : public QObject
{
    Q_OBJECT
public:
    enum Flag {
        NoFlags          = 0,
        NoKDEInit        = 1 << 0
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    explicit Launcher(QObject *parent = 0);
    ~Launcher();

    bool connectToWorld();
    bool connectToSandbox();
    bool setupServices();
    bool launchApp(const QStringList &args, const Flags &flags);
    void cleanup();

    QString errorText() const;

    int retVal() const;

Q_SIGNALS:
    void quit();

private Q_SLOTS:
    void onAppFinished(int code, QProcess::ExitStatus status);
    void onAppError(QProcess::ProcessError error);

private:
    bool error(const QString &error = QString());
    void stopProcess(QProcess *process);

private:
    QProcess *mKdeInitProcess;
    QProcess *mAppProcess;

    QString mSandboxBusAddress;
    Q_PID mSandboxBusPID;

    WorldProxy *mWorldProxy;
    SandboxProxy *mSandboxProxy;

    QString mErrorText;

    int mAppRetVal;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Launcher::Flags)

#endif // KSBLAUNCHER_H

