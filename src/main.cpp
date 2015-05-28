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

#include <QCoreApplication>
#include <QCommandLineParser>

#include <QDebug>

#include "launcher.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("ksbinit"));
    app.setApplicationVersion(QStringLiteral("0.1"));
    app.setOrganizationDomain(QStringLiteral("kde.org"));
    app.setOrganizationName(QStringLiteral("KDE"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("KDE Sandbox Init - a helper utility to improve integration "
                                                    "of sandboxed KDE application with the outside environment."));

    QCommandLineOption nokdeinit(QStringLiteral("no-kdeinit"),
                                 QStringLiteral("Launch the application directly instead of using kdeinit5"));
    parser.addOption(nokdeinit);
    parser.addPositionalArgument(QStringLiteral("app"),
                                 QStringLiteral("Application to launch"),
                                 QStringLiteral("APP [ARGS]"));
    parser.addHelpOption();
    parser.addVersionOption();

    QStringList args = app.arguments();

    parser.parse(args);

    if (parser.isSet(QStringLiteral("help")) && parser.positionalArguments().isEmpty()) {
        parser.showHelp();
        // exit 0
    }
    if (parser.isSet(QStringLiteral("version")) && parser.positionalArguments().isEmpty()) {
        parser.showVersion();
        // exit 0
    }

    // Remove our name from the args list, this leaves name of the app to launch
    // and its arguments
    args.removeFirst();

    Launcher::Flags flags = Launcher::NoFlags;
    if (parser.isSet(nokdeinit)) {
        args.removeFirst();
        flags |= Launcher::NoKDEInit;
    }

    Launcher launcher;
    QObject::connect(&launcher, &Launcher::quit,
                     &app, &QCoreApplication::quit);

    // Connects to the outside bus via DBUS_SESSION_BUS_ADDRESS
    if (!launcher.connectToWorld()) {
        qFatal("Failed to establish connection with the outside session bus: %s", qPrintable(launcher.errorText()));
    }

    // Launches new DBus session inside sandbox and connects to it too
    if (!launcher.connectToSandbox()) {
        qFatal("Failed to establish connection with the sandbox session bus: %s", qPrintable(launcher.errorText()));
    }

    // Create bunch of default KDE DBus services on the sandbox bus. We will
    // relay calls to them to the actual session bus
    if (!launcher.setupServices()) {
        qFatal("Failed to setup DBus services: %s", qPrintable(launcher.errorText()));
    }

    // Now launch the application itself
    if (!launcher.launchApp(args, flags)) {
        qFatal("Failed to start application: %s", qPrintable(launcher.errorText()));
    }

    const int thisRet = app.exec();

    // Clean up while QApp stil exists
    launcher.cleanup();

    return (launcher.retVal() == -1 ? thisRet : launcher.retVal());
}
