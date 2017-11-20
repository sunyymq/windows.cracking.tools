#ifndef SYMBOLVIEW_H
#define SYMBOLVIEW_H

#include <QWidget>
#include "Bridge.h"

class QMenu;
class SearchListView;
class QVBoxLayout;

namespace Ui
{
    class SymbolView;
}

class SymbolView : public QWidget
{
    Q_OBJECT

public:
    explicit SymbolView(QWidget* parent = 0);
    ~SymbolView();
    void setupContextMenu();
    void saveWindowSettings();
    void loadWindowSettings();

private slots:
    void updateStyle();
    void addMsgToSymbolLogSlot(QString msg);
    void clearSymbolLogSlot();
    void moduleSelectionChanged(int index);
    void updateSymbolList(int module_count, SYMBOLMODULEINFO* modules);
    void symbolFollow();
    void symbolFollowDump();
    void symbolFollowImport();
    void enterPressedSlot();
    void symbolContextMenu(QMenu* wMenu);
    void symbolRefreshCurrent();
    void moduleContextMenu(QMenu* wMenu);
    void moduleFollow();
    void moduleEntryFollow();
    void moduleDownloadSymbols();
    void moduleDownloadAllSymbols();
    void moduleCopyPath();
    void moduleBrowse();
    void moduleYara();
    void moduleYaraFile();
    void moduleSetUser();
    void moduleSetSystem();
    void moduleSetParty();
    void moduleFollowMemMap();
    void toggleBreakpoint();
    void toggleBookmark();
    void refreshShortcutsSlot();
    void moduleEntropy();
    void emptySearchResultSlot();
    void selectionGetSlot(SELECTIONDATA* selection);
    void moduleLoad();
    void moduleFree();

signals:
    void showReferences();

private:
    Ui::SymbolView* ui;
    QVBoxLayout* mMainLayout;
    QVBoxLayout* mSymbolLayout;
    QWidget* mSymbolPlaceHolder;
    SearchListView* mSearchListView;
    SearchListView* mModuleList;
    QMap<QString, duint> mModuleBaseList;
    QAction* mFollowSymbolAction;
    QAction* mFollowSymbolDumpAction;
    QAction* mFollowSymbolImportAction;
    QAction* mToggleBreakpoint;
    QAction* mToggleBookmark;
    QAction* mFollowModuleAction;
    QAction* mFollowModuleEntryAction;
    QAction* mDownloadSymbolsAction;
    QAction* mDownloadAllSymbolsAction;
    QAction* mCopyPathAction;
    QAction* mYaraAction;
    QAction* mYaraFileAction;
    QAction* mEntropyAction;
    QAction* mModSetUserAction;
    QAction* mModSetSystemAction;
    QAction* mModSetPartyAction;
    QAction* mBrowseInExplorer;
    QAction* mFollowInMemMap;
    QAction* mLoadLib;
    QAction* mFreeLib;

    static void cbSymbolEnum(SYMBOLINFO* symbol, void* user);
};

#endif // SYMBOLVIEW_H
