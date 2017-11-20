#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Imports.h"

class QDragEnterEvent;
class QDropEvent;
class CloseDialog;
class CommandLineEdit;
class MHTabWidget;
class CPUWidget;
class MemoryMapView;
class CallStackView;
class SEHChainView;
class LogView;
class SymbolView;
class BreakpointsView;
class ScriptView;
class ReferenceManager;
class ThreadView;
class PatchDialog;
class CalculatorDialog;
class DebugStatusLabel;
class LogStatusLabel;
class UpdateChecker;
class SourceViewerManager;
class HandlesView;
class MainWindowCloseThread;
class TimeWastedCounter;
class NotesManager;
class SettingsDialog;
class DisassemblerGraphView;
class SimpleTraceDialog;
class MRUList;

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

    void setupCommandBar();
    void setupStatusBar();
    void closeEvent(QCloseEvent* event);
    void setTab(QWidget* widget);
    void loadTabDefaultOrder();
    void loadTabSavedOrder();
    void clearTabWidget();

public slots:
    void saveWindowSettings();
    void loadWindowSettings();
    void executeCommand();
    void execCommandSlot();
    void setFocusToCommandBar();
    void displayMemMapWidget();
    void displayLogWidget();
    void displayScriptWidget();
    void displayAboutWidget();
    void execTocnd();
    void execTicnd();
    void animateIntoSlot();
    void animateOverSlot();
    void animateCommandSlot();
    void openFileSlot();
    void openRecentFileSlot(QString filename);
    void restartDebugging();
    void displayBreakpointWidget();
    void updateWindowTitleSlot(QString filename);
    void runSlot();
    void execTRBit();
    void execTRByte();
    void execTRWord();
    void execTRNone();
    void displayCpuWidget();
    void displaySymbolWidget();
    void displaySourceViewWidget();
    void displayReferencesWidget();
    void displayThreadsWidget();
    void displaySnowmanWidget();
    void displayVariables();
    void displayGraphWidget();
    void displayPreviousTab();
    void displayNextTab();
    void hideTab();
    void openSettings();
    void openAppearance();
    void openCalculator();
    void addRecentFile(QString file);
    void setLastException(unsigned int exceptionCode);
    void findStrings();
    void findModularCalls();
    void addMenuToList(QWidget* parent, QMenu* menu, int hMenu, int hParentMenu = -1);
    void addMenu(int hMenu, QString title);
    void addMenuEntry(int hMenu, QString title);
    void addSeparator(int hMenu);
    void clearMenu(int hMenu);
    void menuEntrySlot();
    void removeMenuEntry(int hEntry);
    void setIconMenuEntry(int hEntry, QIcon icon);
    void setIconMenu(int hMenu, QIcon icon);
    void setCheckedMenuEntry(int hEntry, bool checked);
    void setHotkeyMenuEntry(int hEntry, QString hotkey, QString id);
    void setVisibleMenuEntry(int hEntry, bool visible);
    void setVisibleMenu(int hMenu, bool visible);
    void setNameMenuEntry(int hEntry, QString name);
    void setNameMenu(int hMenu, QString name);
    void runSelection();
    void runExpression();
    void getStrWindow(const QString title, QString* text);
    void patchWindow();
    void displayComments();
    void displayLabels();
    void displayBookmarks();
    void displayFunctions();
    void checkUpdates();
    void crashDump();
    void displayCallstack();
    void displaySEHChain();
    void setGlobalShortcut(QAction* action, const QKeySequence & key);
    void refreshShortcuts();
    void openShortcuts();
    void changeTopmost(bool checked);
    void donate();
    void blog();
    void reportBug();
    void displayAttach();
    void changeCommandLine();
    void displayManual();
    void canClose();
    void addQWidgetTab(QWidget* qWidget, QString nativeName);
    void addQWidgetTab(QWidget* qWidget);
    void showQWidgetTab(QWidget* qWidget);
    void closeQWidgetTab(QWidget* qWidget);
    void executeOnGuiThread(void* cbGuiThread);
    void tabMovedSlot(int from, int to);
    void chkSaveloadTabSavedOrderStateChangedSlot(bool state);
    void dbgStateChangedSlot(DBGSTATE state);
    void displayNotesWidget();
    void displayHandlesWidget();
    void manageFavourites();
    void updateFavouriteTools();
    void clickFavouriteTool();
    void chooseLanguage();
    void setInitializationScript();
    void customizeMenu();
    void addFavouriteItem(int type, const QString & name, const QString & description);
    void setFavouriteItemShortcut(int type, const QString & name, const QString & shortcut);

private:
    Ui::MainWindow* ui;
    CloseDialog* mCloseDialog;
    CommandLineEdit* mCmdLineEdit;
    MHTabWidget* mTabWidget;
    CPUWidget* mCpuWidget;
    MemoryMapView* mMemMapView;
    CallStackView* mCallStackView;
    SEHChainView* mSEHChainView;
    LogView* mLogView;
    SymbolView* mSymbolView;
    SourceViewerManager* mSourceViewManager;
    BreakpointsView* mBreakpointsView;
    ScriptView* mScriptView;
    ReferenceManager* mReferenceManager;
    ThreadView* mThreadView;
    PatchDialog* mPatchDialog;
    CalculatorDialog* mCalculatorDialog;
    QWidget* mSnowmanView;
    HandlesView* mHandlesView;
    NotesManager* mNotesManager;
    DisassemblerGraphView* mGraphView;
    SimpleTraceDialog* mSimpleTraceDialog;

    DebugStatusLabel* mStatusLabel;
    LogStatusLabel* mLastLogLabel;

    UpdateChecker* mUpdateChecker;
    TimeWastedCounter* mTimeWastedCounter;

    QString mWindowMainTitle;

    MRUList* mMRUList;
    unsigned int lastException;

    QAction* actionManageFavourites;

    void updateMRUMenu();
    void setupLanguagesMenu();
    void onMenuCustomized();
    void setupMenuCustomization();
    QAction* makeCommandAction(QAction* action, const QString & command);

    //lists for menu customization
    QList<QAction*> mFileMenuStrings;
    QList<QAction*> mViewMenuStrings;
    QList<QAction*> mDebugMenuStrings;
    //"Plugins" menu cannot be customized for item hiding.
    //"Favourites" menu cannot be customized for item hiding.
    QList<QAction*> mOptionsMenuStrings;
    QList<QAction*> mHelpMenuStrings;

    //menu api
    struct MenuEntryInfo
    {
        QAction* mAction;
        int hEntry;
        int hParentMenu;
        QString hotkey;
        QString hotkeyId;
        bool hotkeyGlobal;
    };

    struct MenuInfo
    {
    public:
        MenuInfo(QWidget* parent, QMenu* mMenu, int hMenu, int hParentMenu, bool globalMenu)
            : parent(parent), mMenu(mMenu), hMenu(hMenu), hParentMenu(hParentMenu), globalMenu(globalMenu)
        {
        }

        QWidget* parent;
        QMenu* mMenu;
        int hMenu;
        int hParentMenu;
        bool globalMenu;
    };

    QList<MenuEntryInfo> mEntryList;
    int hEntryNext;
    QList<MenuInfo> mMenuList;
    int hMenuNext;

    void initMenuApi();
    const MenuInfo* findMenu(int hMenu);
    QString nestedMenuDescription(const MenuInfo* menu);
    QString nestedMenuEntryDescription(const MenuEntryInfo & entry);

    bool bCanClose;
    MainWindowCloseThread* mCloseThread;

    struct WidgetInfo
    {
    public:
        WidgetInfo(QWidget* widget, QString nativeName)
        {
            this->widget = widget;
            this->nativeName = nativeName;
        }

        QWidget* widget;
        QString nativeName;
    };

    QList<WidgetInfo> mWidgetList;

protected:
    void dragEnterEvent(QDragEnterEvent* pEvent);
    void dropEvent(QDropEvent* pEvent);

public:
    static QString windowTitle;

private slots:
    void on_actionFaq_triggered();
    void on_actionReloadStylesheet_triggered();
    void on_actionImportSettings_triggered();
    void on_actionImportdatabase_triggered();
    void on_actionExportdatabase_triggered();
    void on_actionRestartAdmin_triggered();
    void on_actionPlugins_triggered();
};

#endif // MAINWINDOW_H
