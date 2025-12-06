/*!
    @file           QtWindow.h

    @brief          xxx.

    @author         L. J. Barman

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

*/

#ifndef __QT_WINDOW_H__
#define __QT_WINDOW_H__

#include <QMainWindow>
#include <QMap>
#include <QTranslator>
#include <QString>

#include "Song.h"

class QAction;
class CGLView;
class QCloseEvent;
class QKeyEvent;
class QMenu;
class QWidget;

class CSettings;
class CScore;
class GuiSidePanel;
class GuiTopBar;
class QTextBrowser;

static constexpr int maxRecentFiles() { return 20; }

class QtWindow : public QMainWindow
{
    Q_OBJECT

public:
    QtWindow();
    ~QtWindow() override;

    void init();

    void songEventUpdated(eventBits_t eventBits);
    void loadTutorHtml(const QString & name);
    void setCurrentFile(const QString &fileName);

private slots:
    void open();
    void help();
    void website();
    void about();
    void keyboardShortcuts();
    void openRecentFile();

    void showMidiSetup();

    void showPreferencesDialog();
    void showSongDetailsDialog();
    void showKeyboardSetup();

    void toggleSidePanel();

    void onViewPianoKeyboard();

    void onFullScreenStateAct ();

    void enableFollowTempo();
    void disableFollowTempo();

    void on_rightHand();
    void on_bothHands();
    void on_leftHand();
    void on_playFromStart();

    void on_playPause();
    void on_faster();
    void on_slower();
    void on_nextSong();
    void on_previousSong();
    void on_nextBook();
    void on_previousBook();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void decodeCommandLine();
    int decodeIntegerParam(const QString &arg, int defaultParam);
    bool validateIntegerParam(const QString &arg);
    bool validateIntegerParamWithMessage(const QString &arg);
    void decodeMidiFileArg(const QString &arg);
    QString displayShortCut(const QString &code, const QString &description);
    void addShortcutAction(const QString & key, const char * method);
    void updateRecentFileActions();
    QString strippedName(const QString &fullFileName);
    void refreshTranslate();

    void displayUsage();
    void createActions();
    void createMenus();
    void readSettings();
    void writeSettings();

    CSettings* m_settings;

    GuiSidePanel *m_sidePanel;
    GuiTopBar *m_topBar;
    QTextBrowser *m_tutorWindow;

    QTranslator translator;
    QTranslator translatorMusic;
    QTranslator qtTranslator;

    QMap<QWidget*,QMap<QString,QString>> listWidgetsRetranslateUi;
    QMap<QAction*,QMap<QString,QString>> listActionsRetranslateUi;

    CGLView *m_glWidget;
    QAction *m_openAct;
    QAction *m_exitAct;
    QAction *m_aboutAct;
    QAction *m_shortcutAct;
    QAction *m_songPlayAct;
    QAction *m_setupMidiAct;
    QAction *m_setupKeyboardAct;
    QAction *m_sidePanelStateAct;
    QAction *m_viewPianoKeyboard;
    QAction *m_fullScreenStateAct;
    QAction *m_setupPreferencesAct;
    QAction *m_songDetailsAct;

    QMenu *m_fileMenu;
    QMenu *m_viewMenu;
    QMenu *m_songMenu;
    QMenu *m_setupMenu;
    QMenu *m_helpMenu;

    CSong* m_song;
    CScore* m_score;
    QAction *m_separatorAct;

    QAction *m_recentFileActs[maxRecentFiles()];
};

#endif // __QT_WINDOW_H__
