#include "HandlesView.h"
#include "Bridge.h"
#include "VersionHelpers.h"
#include "StdTable.h"
#include "LabeledSplitter.h"
#include "StringUtil.h"
#include "ReferenceView.h"
#include "MainWindow.h"
#include "MessagesBreakpoints.h"
#include <QVBoxLayout>

HandlesView::HandlesView(QWidget* parent) : QWidget(parent)
{

    mHandlesTable = new SearchListView(true, this, true);
    mHandlesTable->mList->setWindowTitle("Handles");
    mHandlesTable->mSearchStartCol = 0;
    int wCharWidth = mHandlesTable->mList->getCharWidth();
    // Setup handles list
    mHandlesTable->mList->setDrawDebugOnly(true);
    mHandlesTable->mList->addColumnAt(8 + 16 * wCharWidth, tr("Type"), false);
    mHandlesTable->mList->addColumnAt(8 + 8 * wCharWidth, tr("Type number"), false);
    mHandlesTable->mList->addColumnAt(8 + sizeof(duint) * 2 * wCharWidth, tr("Handle"), false);
    mHandlesTable->mList->addColumnAt(8 + 16 * wCharWidth, tr("Access"), false);
    mHandlesTable->mList->addColumnAt(8 + wCharWidth * 20, tr("Name"), false);
    mHandlesTable->mList->loadColumnFromConfig("Handle");
    // Setup search list
    mHandlesTable->mSearchList->addColumnAt(8 + 16 * wCharWidth, tr("Type"), false);
    mHandlesTable->mSearchList->addColumnAt(8 + 8 * wCharWidth, tr("Type number"), false);
    mHandlesTable->mSearchList->addColumnAt(8 + sizeof(duint) * 2 * wCharWidth, tr("Handle"), false);
    mHandlesTable->mSearchList->addColumnAt(8 + 16 * wCharWidth, tr("Access"), false);
    mHandlesTable->mSearchList->addColumnAt(8 + wCharWidth * 20, tr("Name"), false);
    mHandlesTable->mSearchList->loadColumnFromConfig("Handle");

    mWindowsTable = new SearchListView(true, this, true);
    mWindowsTable->mList->setWindowTitle("Windows");
    mWindowsTable->mSearchStartCol = 0;
    // Setup windows list
    mWindowsTable->mList->setDrawDebugOnly(true);
    mWindowsTable->mList->addColumnAt(8 + sizeof(duint) * 2 * wCharWidth, tr("Proc"), false);
    mWindowsTable->mList->addColumnAt(8 + 8 * wCharWidth, tr("Handle"), false);
    mWindowsTable->mList->addColumnAt(8 + 120 * wCharWidth, tr("Title"), false);
    mWindowsTable->mList->addColumnAt(8 + 40 * wCharWidth, tr("Class"), false);
    mWindowsTable->mList->addColumnAt(8 + 8 * wCharWidth, tr("Thread"), false);
    mWindowsTable->mList->addColumnAt(8 + 16 * wCharWidth, tr("Style"), false);
    mWindowsTable->mList->addColumnAt(8 + 16 * wCharWidth, tr("StyleEx"), false);
    mWindowsTable->mList->addColumnAt(8 + 8 * wCharWidth, tr("Parent"), false);
    mWindowsTable->mList->addColumnAt(8 + 20 * wCharWidth, tr("Size"), false);
    mWindowsTable->mList->addColumnAt(8 + 6 * wCharWidth, tr("Enable"), false);
    mWindowsTable->mList->loadColumnFromConfig("Window");
    // Setup search list
    mWindowsTable->mSearchList->addColumnAt(8 + sizeof(duint) * 2 * wCharWidth, tr("Proc"), false);
    mWindowsTable->mSearchList->addColumnAt(8 + 8 * wCharWidth, tr("Handle"), false);
    mWindowsTable->mSearchList->addColumnAt(8 + 120 * wCharWidth, tr("Title"), false);
    mWindowsTable->mSearchList->addColumnAt(8 + 40 * wCharWidth, tr("Class"), false);
    mWindowsTable->mSearchList->addColumnAt(8 + 8 * wCharWidth, tr("Thread"), false);
    mWindowsTable->mSearchList->addColumnAt(8 + 16 * wCharWidth, tr("Style"), false);
    mWindowsTable->mSearchList->addColumnAt(8 + 16 * wCharWidth, tr("StyleEx"), false);
    mWindowsTable->mSearchList->addColumnAt(8 + 8 * wCharWidth, tr("Parent"), false);
    mWindowsTable->mSearchList->addColumnAt(8 + 20 * wCharWidth, tr("Size"), false);
    mWindowsTable->mSearchList->addColumnAt(8 + 6 * wCharWidth, tr("Enable"), false);
    mWindowsTable->mSearchList->loadColumnFromConfig("Window");

    mTcpConnectionsTable = new SearchListView(true, this, true);
    mTcpConnectionsTable->setWindowTitle("TcpConnections");
    mHandlesTable->mSearchStartCol = 0;
    // create tcp list
    mTcpConnectionsTable->mList->setDrawDebugOnly(true);
    mTcpConnectionsTable->mList->addColumnAt(8 + 64 * wCharWidth, tr("Remote address"), false);
    mTcpConnectionsTable->mList->addColumnAt(8 + 64 * wCharWidth, tr("Local address"), false);
    mTcpConnectionsTable->mList->addColumnAt(8 + 8 * wCharWidth, tr("State"), false);
    mTcpConnectionsTable->mList->loadColumnFromConfig("TcpConnection");
    // create search list
    mTcpConnectionsTable->mSearchList->addColumnAt(8 + 64 * wCharWidth, tr("Remote address"), false);
    mTcpConnectionsTable->mSearchList->addColumnAt(8 + 64 * wCharWidth, tr("Local address"), false);
    mTcpConnectionsTable->mSearchList->addColumnAt(8 + 8 * wCharWidth, tr("State"), false);
    mTcpConnectionsTable->mSearchList->loadColumnFromConfig("TcpConnection");
    /*
        mHeapsTable = new ReferenceView(this);
        mHeapsTable->setWindowTitle("Heaps");
        //mHeapsTable->setContextMenuPolicy(Qt::CustomContextMenu);
        mHeapsTable->addColumnAt(sizeof(duint) * 2, tr("Address"));
        mHeapsTable->addColumnAt(sizeof(duint) * 2, tr("Size"));
        mHeapsTable->addColumnAt(20, tr("Flags"));
        mHeapsTable->addColumnAt(50, tr("Comments"));
    */
    mPrivilegesTable = new StdTable(this);
    mPrivilegesTable->setWindowTitle("Privileges");
    mPrivilegesTable->setDrawDebugOnly(true);
    mPrivilegesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mPrivilegesTable->addColumnAt(8 + 32 * wCharWidth, tr("Privilege"), false);
    mPrivilegesTable->addColumnAt(8 + 16 * wCharWidth, tr("State"), false);
    mPrivilegesTable->loadColumnFromConfig("Privilege");

    // Splitter
    mSplitter = new LabeledSplitter(this);
    mSplitter->addWidget(mHandlesTable, tr("Handles"));
    mSplitter->addWidget(mWindowsTable, tr("Windows"));
    //mSplitter->addWidget(mHeapsTable, tr("Heaps"));
    mSplitter->addWidget(mTcpConnectionsTable, tr("TCP Connections"));
    mSplitter->addWidget(mPrivilegesTable, tr("Privileges"));
    mSplitter->collapseLowerTabs();

    // Layout
    mVertLayout = new QVBoxLayout;
    mVertLayout->setSpacing(0);
    mVertLayout->setContentsMargins(0, 0, 0, 0);
    mVertLayout->addWidget(mSplitter);
    this->setLayout(mVertLayout);
    mSplitter->loadFromConfig("HandlesViewSplitter");

    // Create the action list for the right click context menu
    mActionRefresh = new QAction(DIcon("arrow-restart.png"), tr("&Refresh"), this);
    connect(mActionRefresh, SIGNAL(triggered()), this, SLOT(reloadData()));
    addAction(mActionRefresh);
    mActionCloseHandle = new QAction(DIcon("disable.png"), tr("Close handle"), this);
    connect(mActionCloseHandle, SIGNAL(triggered()), this, SLOT(closeHandleSlot()));
    mActionDisablePrivilege = new QAction(DIcon("disable.png"), tr("Disable Privilege: "), this);
    connect(mActionDisablePrivilege, SIGNAL(triggered()), this, SLOT(disablePrivilegeSlot()));
    mActionEnablePrivilege = new QAction(DIcon("enable.png"), tr("Enable Privilege: "), this);
    connect(mActionEnablePrivilege, SIGNAL(triggered()), this, SLOT(enablePrivilegeSlot()));
    mActionDisableAllPrivileges = new QAction(DIcon("disable.png"), tr("Disable all privileges"), this);
    connect(mActionDisableAllPrivileges, SIGNAL(triggered()), this, SLOT(disableAllPrivilegesSlot()));
    mActionEnableAllPrivileges = new QAction(DIcon("enable.png"), tr("Enable all privileges"), this);
    connect(mActionEnableAllPrivileges, SIGNAL(triggered()), this, SLOT(enableAllPrivilegesSlot()));
    mActionEnableWindow = new QAction(DIcon("enable.png"), tr("Enable window"), this);
    connect(mActionEnableWindow, SIGNAL(triggered()), this, SLOT(enableWindowSlot()));
    mActionDisableWindow = new QAction(DIcon("disable.png"), tr("Disable window"), this);
    connect(mActionDisableWindow, SIGNAL(triggered()), this, SLOT(disableWindowSlot()));
    mActionFollowProc = new QAction(DIcon(ArchValue("processor32.png", "processor64.png")), tr("Follow Proc in Disassembler"), this);
    connect(mActionFollowProc, SIGNAL(triggered()), this, SLOT(followInDisasmSlot()));
    mActionToggleProcBP = new QAction(DIcon("breakpoint_toggle.png"), tr("Toggle Breakpoint in Proc"), this);
    connect(mActionToggleProcBP, SIGNAL(triggered()), this, SLOT(toggleBPSlot()));
    mActionMessageProcBP = new QAction(DIcon("breakpoint_execute.png"), tr("Message Breakpoint"), this);
    connect(mActionMessageProcBP, SIGNAL(triggered()), this, SLOT(messagesBPSlot()));

    connect(mHandlesTable, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(handlesTableContextMenuSlot(QMenu*)));
    connect(mWindowsTable, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(windowsTableContextMenuSlot(QMenu*)));
    connect(mTcpConnectionsTable, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(tcpConnectionsTableContextMenuSlot(QMenu*)));
    connect(mPrivilegesTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(privilegesTableContextMenuSlot(const QPoint &)));
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcuts()));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChanged(DBGSTATE)));

    if(!IsWindowsVistaOrGreater())
    {
        mTcpConnectionsTable->mList->setRowCount(1);
        mTcpConnectionsTable->mList->setCellContent(0, 0, tr("TCP Connection enumeration is only available on Windows Vista or greater."));
        mTcpConnectionsTable->mList->reloadData();
    }
    reloadData();
    refreshShortcuts();
}

void HandlesView::reloadData()
{
    if(DbgIsDebugging())
    {
        enumHandles();
        enumWindows();
        enumTcpConnections();
        //enumHeaps();
        enumPrivileges();
    }
    else
    {
        mHandlesTable->mList->setRowCount(0);
        mHandlesTable->mList->reloadData();
        mWindowsTable->mList->setRowCount(0);
        mWindowsTable->mList->reloadData();
        mTcpConnectionsTable->mList->setRowCount(0);
        mTcpConnectionsTable->mList->reloadData();

        //mHeapsTable->setRowCount(0);
        //mHeapsTable->reloadData();
        mPrivilegesTable->setRowCount(0);
        mPrivilegesTable->reloadData();
    }
}

void HandlesView::refreshShortcuts()
{
    mActionRefresh->setShortcut(ConfigShortcut("ActionRefresh"));
}

void HandlesView::dbgStateChanged(DBGSTATE state)
{
    if(state == stopped)
        reloadData();
}

void HandlesView::handlesTableContextMenuSlot(QMenu* wMenu)
{
    if(!DbgIsDebugging())
        return;
    StdTable & table = *mHandlesTable->mCurList;
    QMenu wCopyMenu(tr("&Copy"), this);
    wCopyMenu.setIcon(DIcon("copy.png"));

    wMenu->addAction(mActionRefresh);
    if(table.getRowCount())
    {
        wMenu->addAction(mActionCloseHandle);

        table.setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu->addSeparator();
            wMenu->addMenu(&wCopyMenu);
        }
    }
}

void HandlesView::windowsTableContextMenuSlot(QMenu* wMenu)
{
    if(!DbgIsDebugging())
        return;
    StdTable & table = *mWindowsTable->mCurList;
    QMenu wCopyMenu(tr("Copy"), this);
    wCopyMenu.setIcon(DIcon("copy.png"));
    wMenu->addAction(mActionRefresh);

    if(table.getRowCount())
    {
        if(table.getCellContent(table.getInitialSelection(), 9) == tr("Enabled"))
        {
            mActionDisableWindow->setText(tr("Disable window"));
            wMenu->addAction(mActionDisableWindow);
        }
        else
        {
            mActionEnableWindow->setText(tr("Enable window"));
            wMenu->addAction(mActionEnableWindow);
        }

        wMenu->addAction(mActionFollowProc);
        wMenu->addAction(mActionToggleProcBP);
        wMenu->addAction(mActionMessageProcBP);
        wMenu->addSeparator();
        table.setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu->addSeparator();
            wMenu->addMenu(&wCopyMenu);
        }
    }
}

void HandlesView::tcpConnectionsTableContextMenuSlot(QMenu* wMenu)
{
    if(!DbgIsDebugging())
        return;
    StdTable & table = *mTcpConnectionsTable->mCurList;
    QMenu wCopyMenu(tr("&Copy"), this);
    wCopyMenu.setIcon(DIcon("copy.png"));

    wMenu->addAction(mActionRefresh);
    if(table.getRowCount())
    {
        table.setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu->addSeparator();
            wMenu->addMenu(&wCopyMenu);
        }
    }
}

void HandlesView::privilegesTableContextMenuSlot(const QPoint & pos)
{
    if(!DbgIsDebugging())
        return;
    StdTable & table = *mPrivilegesTable;
    QMenu wMenu;
    bool isValid = (table.getRowCount() != 0 && table.getCellContent(table.getInitialSelection(), 1) != tr("Unknown"));
    wMenu.addAction(mActionRefresh);
    if(isValid)
    {
        if(table.getCellContent(table.getInitialSelection(), 1) == tr("Enabled"))
        {
            mActionDisablePrivilege->setText(tr("Disable Privilege: ") + table.getCellContent(table.getInitialSelection(), 0));
            wMenu.addAction(mActionDisablePrivilege);
        }
        else
        {
            mActionEnablePrivilege->setText(tr("Enable Privilege: ") + table.getCellContent(table.getInitialSelection(), 0));
            wMenu.addAction(mActionEnablePrivilege);
        }
    }
    wMenu.addAction(mActionDisableAllPrivileges);
    wMenu.addAction(mActionEnableAllPrivileges);

    QMenu wCopyMenu(tr("&Copy"), this);
    wCopyMenu.setIcon(DIcon("copy.png"));
    table.setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu.addSeparator();
        wMenu.addMenu(&wCopyMenu);
    }
    wMenu.exec(table.mapToGlobal(pos));
}

void HandlesView::closeHandleSlot()
{
    DbgCmdExecDirect(QString("handleclose %1").arg(mHandlesTable->mCurList->getCellContent(mHandlesTable->mCurList->getInitialSelection(), 2)).toUtf8().constData());
    enumHandles();
}

void HandlesView::enablePrivilegeSlot()
{
    DbgCmdExecDirect(QString("EnablePrivilege \"%1\"").arg(mPrivilegesTable->getCellContent(mPrivilegesTable->getInitialSelection(), 0)).toUtf8().constData());
    enumPrivileges();
}

void HandlesView::disablePrivilegeSlot()
{
    if(!DbgIsDebugging())
        return;
    DbgCmdExecDirect(QString("DisablePrivilege \"%1\"").arg(mPrivilegesTable->getCellContent(mPrivilegesTable->getInitialSelection(), 0)).toUtf8().constData());
    enumPrivileges();
}

void HandlesView::enableAllPrivilegesSlot()
{
    if(!DbgIsDebugging())
        return;
    for(int i = 0; i < mPrivilegesTable->getRowCount(); i++)
        if(mPrivilegesTable->getCellContent(i, 1) != tr("Unknown"))
            DbgCmdExecDirect(QString("EnablePrivilege \"%1\"").arg(mPrivilegesTable->getCellContent(i, 0)).toUtf8().constData());
    enumPrivileges();
}

void HandlesView::disableAllPrivilegesSlot()
{
    if(!DbgIsDebugging())
        return;
    for(int i = 0; i < mPrivilegesTable->getRowCount(); i++)
        if(mPrivilegesTable->getCellContent(i, 1) != tr("Unknown"))
            DbgCmdExecDirect(QString("DisablePrivilege \"%1\"").arg(mPrivilegesTable->getCellContent(i, 0)).toUtf8().constData());
    enumPrivileges();
}

void HandlesView::enableWindowSlot()
{
    DbgCmdExecDirect(QString("EnableWindow %1").arg(mWindowsTable->mCurList->getCellContent(mWindowsTable->mCurList->getInitialSelection(), 1)).toUtf8().constData());
    enumWindows();
}

void HandlesView::disableWindowSlot()
{
    DbgCmdExecDirect(QString("DisableWindow %1").arg(mWindowsTable->mCurList->getCellContent(mWindowsTable->mCurList->getInitialSelection(), 1)).toUtf8().constData());
    enumWindows();
}

void HandlesView::followInDisasmSlot()
{
    DbgCmdExec(QString("disasm %1").arg(mWindowsTable->mCurList->getCellContent(mWindowsTable->mCurList->getInitialSelection(), 0)).toUtf8().constData());
}

void HandlesView::toggleBPSlot()
{
    StdTable & mCurList = *mWindowsTable->mCurList;

    if(!DbgIsDebugging())
        return;

    if(!mCurList.getRowCount())
        return;
    QString addrText = mCurList.getCellContent(mCurList.getInitialSelection(), 0).toUtf8().constData();
    duint wVA;
    if(!DbgFunctions()->ValFromString(addrText.toUtf8().constData(), &wVA))
        return;
    if(!DbgMemIsValidReadPtr(wVA))
        return;

    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_normal) == bp_normal)
        wCmd = "bc " + ToPtrString(wVA);
    else if(wBpType == bp_none)
        wCmd = "bp " + ToPtrString(wVA);

    DbgCmdExecDirect(wCmd.toUtf8().constData());
}

void HandlesView::messagesBPSlot()
{
    StdTable & mCurList = *mWindowsTable->mCurList;
    MessagesBreakpoints::MsgBreakpointData mbpData;

    if(!mCurList.getRowCount())
        return;

    mbpData.wndHandle = mCurList.getCellContent(mCurList.getInitialSelection(), 1).toUtf8().constData();
    mbpData.procVA = mCurList.getCellContent(mCurList.getInitialSelection(), 0).toUtf8().constData();

    MessagesBreakpoints messagesBPDialog(mbpData, this);
    messagesBPDialog.exec();
}

//Enum functions
//Enumerate handles and update handles table
void HandlesView::enumHandles()
{
    BridgeList<HANDLEINFO> handles;
    if(DbgFunctions()->EnumHandles(&handles))
    {
        auto count = handles.Count();
        mHandlesTable->mList->setRowCount(count);
        for(auto i = 0; i < count; i++)
        {
            const HANDLEINFO & handle = handles[i];
            char name[MAX_STRING_SIZE] = "";
            char typeName[MAX_STRING_SIZE] = "";
            DbgFunctions()->GetHandleName(handle.Handle, name, sizeof(name), typeName, sizeof(typeName));
            mHandlesTable->mList->setCellContent(i, 0, typeName);
            mHandlesTable->mList->setCellContent(i, 1, ToHexString(handle.TypeNumber));
            mHandlesTable->mList->setCellContent(i, 2, ToHexString(handle.Handle));
            mHandlesTable->mList->setCellContent(i, 3, ToHexString(handle.GrantedAccess));
            mHandlesTable->mList->setCellContent(i, 4, name);
        }
    }
    else
        mHandlesTable->mList->setRowCount(0);
    mHandlesTable->mList->reloadData();
    // refresh values also when in mSearchList
    mHandlesTable->refreshSearchList();
}
//Enumerate windows and update windows table
void HandlesView::enumWindows()
{
    BridgeList<WINDOW_INFO> windows;
    if(DbgFunctions()->EnumWindows(&windows))
    {
        auto count = windows.Count();
        mWindowsTable->mList->setRowCount(count);
        for(auto i = 0; i < count; i++)
        {
            mWindowsTable->mList->setCellContent(i, 0, ToPtrString(windows[i].wndProc));
            mWindowsTable->mList->setCellContent(i, 1, ToHexString(windows[i].handle));
            mWindowsTable->mList->setCellContent(i, 2, QString(windows[i].windowTitle));
            mWindowsTable->mList->setCellContent(i, 3, QString(windows[i].windowClass));
            char threadname[MAX_THREAD_NAME_SIZE];
            if(DbgFunctions()->ThreadGetName(windows[i].threadId, threadname))
                mWindowsTable->mList->setCellContent(i, 4, QString::fromUtf8(threadname));
            else if(Config()->getBool("Gui", "PidInHex"))
                mWindowsTable->mList->setCellContent(i, 4, ToHexString(windows[i].threadId));
            else
                mWindowsTable->mList->setCellContent(i, 4, QString::number(windows[i].threadId));
            //Style
            mWindowsTable->mList->setCellContent(i, 5, ToHexString(windows[i].style));
            //StyleEx
            mWindowsTable->mList->setCellContent(i, 6, ToHexString(windows[i].styleEx));
            mWindowsTable->mList->setCellContent(i, 7, ToHexString(windows[i].parent) + (windows[i].parent == ((duint)GetDesktopWindow()) ? tr(" (Desktop window)") : ""));
            //Size
            QString sizeText = QString("(%1,%2);%3x%4").arg(windows[i].position.left).arg(windows[i].position.top)
                               .arg(windows[i].position.right - windows[i].position.left).arg(windows[i].position.bottom - windows[i].position.top);
            mWindowsTable->mList->setCellContent(i, 8, sizeText);
            mWindowsTable->mList->setCellContent(i, 9, windows[i].enabled != FALSE ? tr("Enabled") : tr("Disabled"));
        }
    }
    else
        mWindowsTable->mList->setRowCount(0);
    mWindowsTable->mList->reloadData();
    // refresh values also when in mSearchList
    mWindowsTable->refreshSearchList();
}

//Enumerate privileges and update privileges table
void HandlesView::enumPrivileges()
{
    mPrivilegesTable->setRowCount(35);
    AppendPrivilege(0, "SeAssignPrimaryTokenPrivilege");
    AppendPrivilege(1, "SeAuditPrivilege");
    AppendPrivilege(2, "SeBackupPrivilege");
    AppendPrivilege(3, "SeChangeNotifyPrivilege");
    AppendPrivilege(4, "SeCreateGlobalPrivilege");
    AppendPrivilege(5, "SeCreatePagefilePrivilege");
    AppendPrivilege(6, "SeCreatePermanentPrivilege");
    AppendPrivilege(7, "SeCreateSymbolicLinkPrivilege");
    AppendPrivilege(8, "SeCreateTokenPrivilege");
    AppendPrivilege(9, "SeDebugPrivilege");
    AppendPrivilege(10, "SeEnableDelegationPrivilege");
    AppendPrivilege(11, "SeImpersonatePrivilege");
    AppendPrivilege(12, "SeIncreaseBasePriorityPrivilege");
    AppendPrivilege(13, "SeIncreaseQuotaPrivilege");
    AppendPrivilege(14, "SeIncreaseWorkingSetPrivilege");
    AppendPrivilege(15, "SeLoadDriverPrivilege");
    AppendPrivilege(16, "SeLockMemoryPrivilege");
    AppendPrivilege(17, "SeMachineAccountPrivilege");
    AppendPrivilege(18, "SeManageVolumePrivilege");
    AppendPrivilege(19, "SeProfileSingleProcessPrivilege");
    AppendPrivilege(20, "SeRelabelPrivilege");
    AppendPrivilege(21, "SeRemoteShutdownPrivilege");
    AppendPrivilege(22, "SeRestorePrivilege");
    AppendPrivilege(23, "SeSecurityPrivilege");
    AppendPrivilege(24, "SeShutdownPrivilege");
    AppendPrivilege(25, "SeSyncAgentPrivilege");
    AppendPrivilege(26, "SeSystemEnvironmentPrivilege");
    AppendPrivilege(27, "SeSystemProfilePrivilege");
    AppendPrivilege(28, "SeSystemtimePrivilege");
    AppendPrivilege(29, "SeTakeOwnershipPrivilege");
    AppendPrivilege(30, "SeTcbPrivilege");
    AppendPrivilege(31, "SeTimeZonePrivilege");
    AppendPrivilege(32, "SeTrustedCredManAccessPrivilege");
    AppendPrivilege(33, "SeUndockPrivilege");
    AppendPrivilege(34, "SeUnsolicitedInputPrivilege");
    mPrivilegesTable->reloadData();
}

void HandlesView::AppendPrivilege(int row, const char* PrivilegeString)
{
    DbgCmdExecDirect(QString("GetPrivilegeState \"%1\"").arg(PrivilegeString).toUtf8().constData());
    mPrivilegesTable->setCellContent(row, 0, QString(PrivilegeString));
    switch(DbgValFromString("$result"))
    {
    default:
        mPrivilegesTable->setCellContent(row, 1, tr("Unknown"));
        break;
    case 1:
        mPrivilegesTable->setCellContent(row, 1, tr("Disabled"));
        break;
    case 2:
    case 3:
        mPrivilegesTable->setCellContent(row, 1, tr("Enabled"));
        break;
    }
}
//Enumerate TCP connections and update TCP connections table
void HandlesView::enumTcpConnections()
{
    BridgeList<TCPCONNECTIONINFO> connections;
    if(DbgFunctions()->EnumTcpConnections(&connections))
    {
        auto count = connections.Count();
        mTcpConnectionsTable->mList->setRowCount(count);
        for(auto i = 0; i < count; i++)
        {
            const TCPCONNECTIONINFO & connection = connections[i];
            auto remoteText = QString("%1:%2").arg(connection.RemoteAddress).arg(connection.RemotePort);
            mTcpConnectionsTable->mList->setCellContent(i, 0, remoteText);
            auto localText = QString("%1:%2").arg(connection.LocalAddress).arg(connection.LocalPort);
            mTcpConnectionsTable->mList->setCellContent(i, 1, localText);
            mTcpConnectionsTable->mList->setCellContent(i, 2, connection.StateText);
        }
    }
    else
        mTcpConnectionsTable->mList->setRowCount(0);
    mTcpConnectionsTable->mList->reloadData();
    // refresh values also when in mSearchList
    mTcpConnectionsTable->refreshSearchList();
}
/*
//Enumerate Heaps and update Heaps table
void HandlesView::enumHeaps()
{
    BridgeList<HEAPINFO> heaps;
    if(DbgFunctions()->EnumHeaps(&heaps))
    {
        auto count = heaps.Count();
        mHeapsTable->setRowCount(count);
        for(auto i = 0; i < count; i++)
        {
            const HEAPINFO & heap = heaps[i];
            mHeapsTable->setCellContent(i, 0, ToPtrString(heap.addr));
            mHeapsTable->setCellContent(i, 1, ToHexString(heap.size));
            QString flagsText;
            if(heap.flags == 0)
                flagsText = " |"; //Always leave 2 characters to be removed
            if(heap.flags & 1)
                flagsText = "LF32_FIXED |";
            if(heap.flags & 2)
                flagsText += "LF32_FREE |";
            if(heap.flags & 4)
                flagsText += "LF32_MOVABLE |";
            if(heap.flags & (~7))
                flagsText += ToHexString(heap.flags & (~7)) + " |";
            flagsText.chop(2); //Remove last 2 characters: " |"
            mHeapsTable->setCellContent(i, 2, flagsText);
            QString comment;
            char commentUtf8[MAX_COMMENT_SIZE];
            if(DbgGetCommentAt(heap.addr, commentUtf8))
                comment = QString::fromUtf8(commentUtf8);
            else
            {
                if(DbgGetStringAt(heap.addr, commentUtf8))
                    comment = QString::fromUtf8(commentUtf8);
            }
            mHeapsTable->setCellContent(i, 3, comment);
        }
    }
    else
        mHeapsTable->setRowCount(0);
    mHeapsTable->reloadData();
}
*/
