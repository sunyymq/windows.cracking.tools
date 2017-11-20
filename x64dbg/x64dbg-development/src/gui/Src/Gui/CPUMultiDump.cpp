#include "CPUMultiDump.h"
#include "CPUDump.h"
#include "WatchView.h"
#include "LocalVarsView.h"
#include "StructWidget.h"
#include "Bridge.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QTabBar>
#include "FlickerThread.h"

CPUMultiDump::CPUMultiDump(CPUDisassembly* disas, int nbCpuDumpTabs, QWidget* parent)
    : MHTabWidget(parent, true)
{
    setWindowTitle("CPUMultiDump");
    mMaxCPUDumpTabs = nbCpuDumpTabs;
    mInitAllDumpTabs = false;

    for(uint i = 0; i < mMaxCPUDumpTabs; i++)
    {
        CPUDump* cpuDump = new CPUDump(disas, this);
        cpuDump->loadColumnFromConfig(QString("CPUDump%1").arg(i + 1)); //TODO: needs a workaround because the columns change
        connect(cpuDump, SIGNAL(displayReferencesWidget()), this, SLOT(displayReferencesWidgetSlot()));
        auto nativeTitle = QString("Dump ") + QString::number(i + 1);
        this->addTabEx(cpuDump, DIcon("dump.png"), tr("Dump ") + QString::number(i + 1), nativeTitle);
        cpuDump->setWindowTitle(nativeTitle);
    }

    mCurrentCPUDump = dynamic_cast<CPUDump*>(currentWidget());

    mWatch = new WatchView(this);

    //mMaxCPUDumpTabs++;
    auto nativeTitle = QString("Watch 1");
    this->addTabEx(mWatch, DIcon("animal-dog.png"), tr("Watch ") + QString::number(1), nativeTitle);
    mWatch->setWindowTitle(nativeTitle);
    mWatch->loadColumnFromConfig("Watch1");

    mLocalVars = new LocalVarsView(this);
    this->addTabEx(mLocalVars, DIcon("localvars.png"), tr("Locals"), "Locals");

    mStructWidget = new StructWidget(this);
    this->addTabEx(mStructWidget, mStructWidget->windowIcon(), mStructWidget->windowTitle(), "Struct");

    connect(this, SIGNAL(currentChanged(int)), this, SLOT(updateCurrentTabSlot(int)));
    connect(tabBar(), SIGNAL(OnDoubleClickTabIndex(int)), this, SLOT(openChangeTabTitleDialogSlot(int)));

    connect(Bridge::getBridge(), SIGNAL(dumpAt(dsint)), this, SLOT(printDumpAtSlot(dsint)));
    connect(Bridge::getBridge(), SIGNAL(dumpAtN(duint, int)), this, SLOT(printDumpAtNSlot(duint, int)));
    connect(Bridge::getBridge(), SIGNAL(selectionDumpGet(SELECTIONDATA*)), this, SLOT(selectionGetSlot(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(selectionDumpSet(const SELECTIONDATA*)), this, SLOT(selectionSetSlot(const SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChangedSlot(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(focusDump()), this, SLOT(focusCurrentDumpSlot()));
    connect(Bridge::getBridge(), SIGNAL(getDumpAttention()), this, SLOT(getDumpAttention()));

    connect(mCurrentCPUDump, SIGNAL(selectionUpdated()), mCurrentCPUDump, SLOT(selectionUpdatedSlot()));
}

CPUDump* CPUMultiDump::getCurrentCPUDump()
{
    return mCurrentCPUDump;
}

// Only get tab names for all dump tabs!
void CPUMultiDump::getTabNames(QList<QString> & names)
{
    names.clear();
    int i;
    int index;
    // placeholders
    for(i = 0; i < getMaxCPUTabs(); i++)
        names.push_back(QString("Dump %1").arg(i + 1));
    // enumerate all tabs
    for(i = 0; i < QTabWidget::count(); i++)
    {
        if(!getNativeName(i).startsWith("Dump "))
            continue;
        index = getNativeName(i).mid(5).toInt() - 1;
        if(index < getMaxCPUTabs())
            names[index] = this->tabBar()->tabText(i);
    }
    // enumerate all detached windows
    for(i = 0; i < windows().count(); i++)
    {
        QString nativeName = dynamic_cast<MHDetachedWindow*>(windows()[i]->parent())->mNativeName;
        if(nativeName.startsWith("Dump "))
        {
            index = nativeName.mid(5).toInt() - 1;
            if(index < getMaxCPUTabs())
                names[index] = dynamic_cast<MHDetachedWindow*>(windows()[i]->parent())->windowTitle();
        }
    }
}

int CPUMultiDump::getMaxCPUTabs()
{
    return mMaxCPUDumpTabs;
}

int CPUMultiDump::GetDumpWindowIndex(int dump)
{
    QString dumpNativeName = QString("Dump ") + QString::number(dump);
    for(int i = 0; i < count(); i++)
    {
        if(getNativeName(i) == dumpNativeName)
            return i;
    }
    return 2147483647;
}

int CPUMultiDump::GetWatchWindowIndex()
{
    QString watchNativeName = QString("Watch 1");
    for(int i = 0; i < count(); i++)
    {
        if(getNativeName(i) == watchNativeName)
            return i;
    }
    return 2147483647;
}

void CPUMultiDump::SwitchToDumpWindow()
{
    if(!mCurrentCPUDump)
        setCurrentIndex(GetDumpWindowIndex(1));
}

void CPUMultiDump::SwitchToWatchWindow()
{
    if(mCurrentCPUDump)
        setCurrentIndex(GetWatchWindowIndex());
}

void CPUMultiDump::updateCurrentTabSlot(int tabIndex)
{
    CPUDump* t = qobject_cast<CPUDump*>(widget(tabIndex));
    mCurrentCPUDump = t;
}

void CPUMultiDump::printDumpAtSlot(dsint parVa)
{
    if(mInitAllDumpTabs)
    {
        CPUDump* cpuDump = NULL;
        for(int i = 0; i < count(); i++)
        {
            if(!getNativeName(i).startsWith("Dump "))
                continue;
            cpuDump = qobject_cast<CPUDump*>(widget(i));
            if(cpuDump)
            {
                cpuDump->historyClear();
                cpuDump->addVaToHistory(parVa);
                cpuDump->printDumpAt(parVa);
            }
        }

        mInitAllDumpTabs = false;
    }
    else
    {
        SwitchToDumpWindow();
        mCurrentCPUDump->printDumpAt(parVa);
        mCurrentCPUDump->addVaToHistory(parVa);
    }
}

void CPUMultiDump::printDumpAtNSlot(duint parVa, int index)
{
    int tabindex = GetDumpWindowIndex(index);
    if(tabindex == 2147483647)
        return;
    CPUDump* current = qobject_cast<CPUDump*>(widget(tabindex));
    if(!current)
        return;
    setCurrentIndex(tabindex);
    current->printDumpAt(parVa);
    current->addVaToHistory(parVa);
}

void CPUMultiDump::selectionGetSlot(SELECTIONDATA* selectionData)
{
    SwitchToDumpWindow();
    mCurrentCPUDump->selectionGet(selectionData);
}

void CPUMultiDump::selectionSetSlot(const SELECTIONDATA* selectionData)
{
    SwitchToDumpWindow();
    mCurrentCPUDump->selectionSet(selectionData);
}

void CPUMultiDump::dbgStateChangedSlot(DBGSTATE dbgState)
{
    if(dbgState == initialized)
        mInitAllDumpTabs = true;
}

void CPUMultiDump::openChangeTabTitleDialogSlot(int tabIndex)
{
    bool bUserPressedOk;
    QString sCurrentTabName = tabBar()->tabText(tabIndex);

    QString sNewTabName = QInputDialog::getText(this, tr("Change Tab %1 Name").arg(tabIndex + 1), tr("Tab Name"), QLineEdit::Normal, sCurrentTabName, &bUserPressedOk, Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
    if(bUserPressedOk)
    {
        if(sNewTabName.length() != 0)
            tabBar()->setTabText(tabIndex, sNewTabName);
    }
}

void CPUMultiDump::displayReferencesWidgetSlot()
{
    emit displayReferencesWidget();
}

void CPUMultiDump::focusCurrentDumpSlot()
{
    SwitchToDumpWindow();
    mCurrentCPUDump->setFocus();
}

void CPUMultiDump::getDumpAttention()
{
    FlickerThread* thread = new FlickerThread(mCurrentCPUDump, this);
    thread->setProperties(3, 1);
    connect(thread, SIGNAL(setStyleSheet(QString)), mCurrentCPUDump, SLOT(setStyleSheet(QString)));
    thread->start();
}
