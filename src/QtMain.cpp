/****************************************************************************

    Copyright (c)   2008-2013, L. J. Barman, all rights reserved

    This file is part of the PianoBooster application

    PianoBooster is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PianoBooster is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PianoBooster.  If not, see <http://www.gnu.org/licenses/>.

****************************************************************************/

#include <QApplication>

#include <cstdio>
#include <cstdlib>

#include "QtWindow.h"
#include "version.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("PianoBooster"));
    app.setOrganizationDomain(QStringLiteral("https://github.com/pianobooster/PianoBooster"));
    app.setApplicationName(QStringLiteral("Piano Booster"));
    app.setApplicationVersion(QStringLiteral(PB_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("pianobooster"));

    const auto args = app.arguments();
    if (args.contains(QStringLiteral("--version"))) {
        std::printf("pianobooster " PB_VERSION "\n");
        return EXIT_SUCCESS;
    }

    QtWindow mainWindow;
    mainWindow.show();

    const auto exitCode = app.exec();
    closeLogs();
    return exitCode;
}
