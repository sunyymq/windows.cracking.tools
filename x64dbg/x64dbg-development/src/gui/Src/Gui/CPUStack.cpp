#include "CPUStack.h"
#include "CPUDump.h"
#include <QClipboard>
#include "Configuration.h"
#include "Bridge.h"
#include "HexEditDialog.h"
#include "WordEditDialog.h"
#include "CPUMultiDump.h"
#include "GotoDialog.h"

CPUStack::CPUStack(CPUMultiDump* multiDump, QWidget* parent) : HexDump(parent)
{
    setWindowTitle("Stack");
    setShowHeader(false);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;
    bStackFrozen = false;
    mMultiDump = multiDump;

    mForceColumn = 1;

    wColDesc.isData = true; //void*
    wColDesc.itemCount = 1;
    wColDesc.separator = 0;
#ifdef _WIN64
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = HexQword;
#else
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = HexDword;
#endif
    appendDescriptor(10 + charwidth * 2 * sizeof(duint), "void*", false, wColDesc);

    wColDesc.isData = false; //comments
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(2000, tr("Comments"), false, wColDesc);

    setupContextMenu();

    mGoto = 0;

    // Slots
    connect(Bridge::getBridge(), SIGNAL(stackDumpAt(duint, duint)), this, SLOT(stackDumpAt(duint, duint)));
    connect(Bridge::getBridge(), SIGNAL(selectionStackGet(SELECTIONDATA*)), this, SLOT(selectionGet(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(selectionStackSet(const SELECTIONDATA*)), this, SLOT(selectionSet(const SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChangedSlot(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(focusStack()), this, SLOT(setFocus()));
    connect(Bridge::getBridge(), SIGNAL(updateDump()), this, SLOT(updateSlot()));

    connect(this, SIGNAL(selectionUpdated()), this, SLOT(selectionUpdatedSlot()));

    Initialize();
}

void CPUStack::updateColors()
{
    HexDump::updateColors();

    backgroundColor = ConfigColor("StackBackgroundColor");
    textColor = ConfigColor("StackTextColor");
    selectionColor = ConfigColor("StackSelectionColor");
    mStackReturnToColor = ConfigColor("StackReturnToColor");
    mStackSEHChainColor = ConfigColor("StackSEHChainColor");
    mUserStackFrameColor = ConfigColor("StackFrameColor");
    mSystemStackFrameColor = ConfigColor("StackFrameSystemColor");
}

void CPUStack::updateFonts()
{
    setFont(ConfigFont("Stack"));
    invalidateCachedFont();
}

void CPUStack::setupContextMenu()
{
    mMenuBuilder = new MenuBuilder(this, [](QMenu*)
    {
        return DbgIsDebugging();
    });

    //Push
    mMenuBuilder->addAction(makeShortcutAction(DIcon("arrow-small-down.png"), ArchValue(tr("P&ush DWORD..."), tr("P&ush QWORD...")), SLOT(pushSlot()), "ActionPush"));

    //Pop
    mMenuBuilder->addAction(makeShortcutAction(DIcon("arrow-small-up.png"), ArchValue(tr("P&op DWORD"), tr("P&op QWORD")), SLOT(popSlot()), "ActionPop"));

    //Realign
    mMenuBuilder->addAction(makeAction(DIcon("align-stack-pointer.png"), tr("Align Stack Pointer"), SLOT(realignSlot())), [this](QMenu*)
    {
        return (mCsp & (sizeof(duint) - 1)) != 0;
    });

    // Modify
    mMenuBuilder->addAction(makeAction(DIcon("modify.png"), tr("Modify"), SLOT(modifySlot())));

    auto binaryMenu = new MenuBuilder(this);

    //Binary->Edit
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_edit.png"), tr("&Edit"), SLOT(binaryEditSlot()), "ActionBinaryEdit"));

    //Binary->Fill
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_fill.png"), tr("&Fill..."), SLOT(binaryFillSlot()), "ActionBinaryFill"));

    //Binary->Separator
    binaryMenu->addSeparator();

    //Binary->Copy
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_copy.png"), tr("&Copy"), SLOT(binaryCopySlot()), "ActionBinaryCopy"));

    //Binary->Paste
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_paste.png"), tr("&Paste"), SLOT(binaryPasteSlot()), "ActionBinaryPaste"));

    //Binary->Paste (Ignore Size)
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_paste_ignoresize.png"), tr("Paste (&Ignore Size)"), SLOT(binaryPasteIgnoreSizeSlot()), "ActionBinaryPasteIgnoreSize"));

    mMenuBuilder->addMenu(makeMenu(DIcon("binary.png"), tr("B&inary")), binaryMenu);

    auto copyMenu = new MenuBuilder(this);
    copyMenu->addAction(mCopySelection);
    copyMenu->addAction(mCopyAddress);
    copyMenu->addAction(mCopyRva, [this](QMenu*)
    {
        return DbgFunctions()->ModBaseFromAddr(rvaToVa(getInitialSelection())) != 0;
    });
    mMenuBuilder->addMenu(makeMenu(DIcon("copy.png"), tr("&Copy")), copyMenu);

    //Breakpoint (hardware access) menu
    auto hardwareAccessMenu = makeMenu(DIcon("breakpoint_access.png"), tr("Hardware, Access"));
    hardwareAccessMenu->addAction(makeAction(DIcon("breakpoint_byte.png"), tr("&Byte"), SLOT(hardwareAccess1Slot())));
    hardwareAccessMenu->addAction(makeAction(DIcon("breakpoint_word.png"), tr("&Word"), SLOT(hardwareAccess2Slot())));
    hardwareAccessMenu->addAction(makeAction(DIcon("breakpoint_dword.png"), tr("&Dword"), SLOT(hardwareAccess4Slot())));
#ifdef _WIN64
    hardwareAccessMenu->addAction(makeAction(DIcon("breakpoint_qword.png"), tr("&Qword"), SLOT(hardwareAccess8Slot())));
#endif //_WIN64

    //Breakpoint (hardware write) menu
    auto hardwareWriteMenu = makeMenu(DIcon("breakpoint_write.png"), tr("Hardware, Write"));
    hardwareWriteMenu->addAction(makeAction(DIcon("breakpoint_byte.png"), tr("&Byte"), SLOT(hardwareWrite1Slot())));
    hardwareWriteMenu->addAction(makeAction(DIcon("breakpoint_word.png"), tr("&Word"), SLOT(hardwareWrite2Slot())));
    hardwareWriteMenu->addAction(makeAction(DIcon("breakpoint_dword.png"), tr("&Dword"), SLOT(hardwareWrite4Slot())));
#ifdef _WIN64
    hardwareWriteMenu->addAction(makeAction(DIcon("breakpoint_qword.png"), tr("&Qword"), SLOT(hardwareAccess8Slot())));
#endif //_WIN64

    //Breakpoint (remove hardware)
    auto hardwareRemove = makeAction(DIcon("breakpoint_remove.png"), tr("Remove &Hardware"), SLOT(hardwareRemoveSlot()));

    //Breakpoint (memory access) menu
    auto memoryAccessMenu = makeMenu(DIcon("breakpoint_memory_access.png"), tr("Memory, Access"));
    memoryAccessMenu->addAction(makeAction(DIcon("breakpoint_memory_singleshoot.png"), tr("&Singleshoot"), SLOT(memoryAccessSingleshootSlot())));
    memoryAccessMenu->addAction(makeAction(DIcon("breakpoint_memory_restore_on_hit.png"), tr("&Restore on hit"), SLOT(memoryAccessRestoreSlot())));

    //Breakpoint (memory write) menu
    auto memoryWriteMenu = makeMenu(DIcon("breakpoint_memory_write.png"), tr("Memory, Write"));
    memoryWriteMenu->addAction(makeAction(DIcon("breakpoint_memory_singleshoot.png"), tr("&Singleshoot"), SLOT(memoryWriteSingleshootSlot())));
    memoryWriteMenu->addAction(makeAction(DIcon("breakpoint_memory_restore_on_hit.png"), tr("&Restore on hit"), SLOT(memoryWriteRestoreSlot())));

    //Breakpoint (remove memory) menu
    auto memoryRemove = makeAction(DIcon("breakpoint_remove.png"), tr("Remove &Memory"), SLOT(memoryRemoveSlot()));

    //Breakpoint menu
    auto breakpointMenu = new MenuBuilder(this);

    //Breakpoint menu
    breakpointMenu->addBuilder(new MenuBuilder(this, [ = ](QMenu * menu)
    {
        duint selectedAddr = rvaToVa(getInitialSelection());
        if(DbgGetBpxTypeAt(selectedAddr) & bp_hardware) //hardware breakpoint set
        {
            menu->addAction(hardwareRemove);
        }
        else //memory breakpoint not set
        {
            menu->addMenu(hardwareAccessMenu);
            menu->addMenu(hardwareWriteMenu);
        }

        menu->addSeparator();

        if(DbgGetBpxTypeAt(selectedAddr) & bp_memory) //memory breakpoint set
        {
            menu->addAction(memoryRemove);
        }
        else //memory breakpoint not set
        {
            menu->addMenu(memoryAccessMenu);
            menu->addMenu(memoryWriteMenu);
        }
        return true;
    }));
    mMenuBuilder->addMenu(makeMenu(DIcon("breakpoint.png"), tr("Brea&kpoint")), breakpointMenu);

    // Restore Selection
    mMenuBuilder->addAction(makeShortcutAction(DIcon("eraser.png"), tr("&Restore selection"), SLOT(undoSelectionSlot()), "ActionUndoSelection"), [this](QMenu*)
    {
        dsint start = rvaToVa(getSelectionStart());
        dsint end = rvaToVa(getSelectionEnd());
        return DbgFunctions()->PatchInRange(start, end);
    });

    //Find Pattern
    mMenuBuilder->addAction(makeShortcutAction(DIcon("search-for.png"), tr("&Find Pattern..."), SLOT(findPattern()), "ActionFindPattern"));

    //Follow CSP
    mMenuBuilder->addAction(makeShortcutAction(DIcon("neworigin.png"), ArchValue(tr("Follow E&SP"), tr("Follow R&SP")), SLOT(gotoCspSlot()), "ActionGotoOrigin"));
    mMenuBuilder->addAction(makeAction(DIcon("cbp.png"), ArchValue(tr("Follow E&BP"), tr("Follow R&BP")), SLOT(gotoCbpSlot())), [this](QMenu*)
    {
        return DbgMemIsValidReadPtr(DbgValFromString("cbp"));
    });

    auto gotoMenu = new MenuBuilder(this);

    //Go to Expression
    gotoMenu->addAction(makeShortcutAction(DIcon("geolocation-goto.png"), tr("Go to &Expression"), SLOT(gotoExpressionSlot()), "ActionGotoExpression"));

    //Go to Base of Stack Frame
    gotoMenu->addAction(makeShortcutAction(DIcon("neworigin.png"), tr("Go to Base of Stack Frame"), SLOT(gotoFrameBaseSlot()), "ActionGotoBaseOfStackFrame"));

    //Go to Previous Frame
    gotoMenu->addAction(makeShortcutAction(DIcon("previous.png"), tr("Go to Previous Stack Frame"), SLOT(gotoPreviousFrameSlot()), "ActionGotoPrevStackFrame"));

    //Go to Next Frame
    gotoMenu->addAction(makeShortcutAction(DIcon("next.png"), tr("Go to Next Stack Frame"), SLOT(gotoNextFrameSlot()), "ActionGotoNextStackFrame"));

    //Go to Previous
    gotoMenu->addAction(makeShortcutAction(DIcon("previous.png"), tr("Go to Previous"), SLOT(gotoPreviousSlot()), "ActionGotoPrevious"), [this](QMenu*)
    {
        return historyHasPrev();
    });

    //Go to Next
    gotoMenu->addAction(makeShortcutAction(DIcon("next.png"), tr("Go to Next"), SLOT(gotoNextSlot()), "ActionGotoNext"), [this](QMenu*)
    {
        return historyHasNext();
    });

    mMenuBuilder->addMenu(makeMenu(DIcon("goto.png"), tr("&Go to")), gotoMenu);

    //Freeze the stack
    mMenuBuilder->addAction(mFreezeStack = makeShortcutAction(DIcon("freeze.png"), tr("Freeze the stack"), SLOT(freezeStackSlot()), "ActionFreezeStack"));
    mFreezeStack->setCheckable(true);

    //Follow in Memory Map
    mMenuBuilder->addAction(makeShortcutAction(DIcon("memmap_find_address_page.png"), ArchValue(tr("Follow DWORD in Memory Map"), tr("Follow QWORD in Memory Map")), SLOT(followInMemoryMapSlot()), "ActionFollowMemMap"), [this](QMenu*)
    {
        duint ptr;
        return DbgMemRead(rvaToVa(getInitialSelection()), (unsigned char*)&ptr, sizeof(ptr)) && DbgMemIsValidReadPtr(ptr);
    });

    auto followStackName = ArchValue(tr("Follow DWORD in &Stack"), tr("Follow QWORD in &Stack"));
    mFollowStack = makeAction(DIcon("stack.png"), followStackName, SLOT(followStackSlot()));
    mFollowStack->setShortcutContext(Qt::WidgetShortcut);
    mFollowStack->setShortcut(QKeySequence("enter"));
    mMenuBuilder->addAction(mFollowStack, [this](QMenu*)
    {
        duint ptr;
        if(!DbgMemRead(rvaToVa(getInitialSelection()), (unsigned char*)&ptr, sizeof(ptr)))
            return false;
        duint stackBegin = mMemPage->getBase();
        duint stackEnd = stackBegin + mMemPage->getSize();
        return ptr >= stackBegin && ptr < stackEnd;
    });

    //Follow in Disassembler
    auto disasmIcon = DIcon(ArchValue("processor32.png", "processor64.png"));
    mFollowDisasm = makeAction(disasmIcon, ArchValue(tr("&Follow DWORD in Disassembler"), tr("&Follow QWORD in Disassembler")), SLOT(followDisasmSlot()));
    mFollowDisasm->setShortcutContext(Qt::WidgetShortcut);
    mFollowDisasm->setShortcut(QKeySequence("enter"));
    mMenuBuilder->addAction(mFollowDisasm, [this](QMenu*)
    {
        duint ptr;
        return DbgMemRead(rvaToVa(getInitialSelection()), (unsigned char*)&ptr, sizeof(ptr)) && DbgMemIsValidReadPtr(ptr);
    });

    //Follow in Dump
    mMenuBuilder->addAction(makeAction(DIcon("dump.png"), tr("Follow in Dump"), SLOT(followInDumpSlot())));

    //Follow PTR in Dump
    auto followDumpName = ArchValue(tr("Follow DWORD in &Dump"), tr("Follow QWORD in &Dump"));
    mMenuBuilder->addAction(makeAction(DIcon("dump.png"), followDumpName, SLOT(followDumpPtrSlot())), [this](QMenu*)
    {
        duint ptr;
        return DbgMemRead(rvaToVa(getInitialSelection()), (unsigned char*)&ptr, sizeof(ptr)) && DbgMemIsValidReadPtr(ptr);
    });

    //Follow in Dump N menu
    auto followDumpNMenu = new MenuBuilder(this, [this](QMenu*)
    {
        duint ptr;
        return DbgMemRead(rvaToVa(getInitialSelection()), (unsigned char*)&ptr, sizeof(ptr)) && DbgMemIsValidReadPtr(ptr);
    });
    int maxDumps = mMultiDump->getMaxCPUTabs();
    for(int i = 0; i < maxDumps; i++)
    {
        auto action = makeAction(tr("Dump %1").arg(i + 1), SLOT(followinDumpNSlot()));
        followDumpNMenu->addAction(action);
        mFollowInDumpActions.push_back(action);
    }
    mMenuBuilder->addMenu(makeMenu(DIcon("dump.png"), followDumpName.replace("&", "")), followDumpNMenu);

    //Watch data
    auto watchDataName = ArchValue(tr("&Watch DWORD"), tr("&Watch QWORD"));
    mMenuBuilder->addAction(makeAction(DIcon("animal-dog.png"), watchDataName,  SLOT(watchDataSlot())));

    mPluginMenu = new QMenu(this);
    Bridge::getBridge()->emitMenuAddToList(this, mPluginMenu, GUI_STACK_MENU);

    mMenuBuilder->addSeparator();
    mMenuBuilder->addBuilder(new MenuBuilder(this, [this](QMenu * menu)
    {
        menu->addActions(mPluginMenu->actions());
        return true;
    }));

    mMenuBuilder->loadFromConfig();
}

void CPUStack::updateFreezeStackAction()
{
    if(bStackFrozen)
        mFreezeStack->setText(tr("Unfreeze the stack"));
    else
        mFreezeStack->setText(tr("Freeze the stack"));
    mFreezeStack->setChecked(bStackFrozen);
}

void CPUStack::getColumnRichText(int col, dsint rva, RichTextPainter::List & richText)
{
    // Compute VA
    duint wVa = rvaToVa(rva);

    bool wActiveStack = (wVa >= mCsp); //inactive stack

    STACK_COMMENT comment;
    RichTextPainter::CustomRichText_t curData;
    curData.highlight = false;
    curData.flags = RichTextPainter::FlagColor;
    curData.textColor = textColor;

    if(col && mDescriptor.at(col - 1).isData == true) //paint stack data
    {
        HexDump::getColumnRichText(col, rva, richText);
        if(!wActiveStack)
        {
            QColor inactiveColor = ConfigColor("StackInactiveTextColor");
            for(int i = 0; i < int(richText.size()); i++)
            {
                richText[i].flags = RichTextPainter::FlagColor;
                richText[i].textColor = inactiveColor;
            }
        }
    }
    else if(col && DbgStackCommentGet(wVa, &comment)) //paint stack comments
    {
        if(wActiveStack)
        {
            if(*comment.color)
            {
                if(comment.color[0] == '!')
                {
                    if(strcmp(comment.color, "!sehclr") == 0)
                        curData.textColor = mStackSEHChainColor;
                    else if(strcmp(comment.color, "!rtnclr") == 0)
                        curData.textColor = mStackReturnToColor;
                    else
                        curData.textColor = textColor;
                }
                else
                    curData.textColor = QColor(QString(comment.color));
            }
            else
                curData.textColor = textColor;
        }
        else
            curData.textColor = ConfigColor("StackInactiveTextColor");
        curData.text = comment.comment;
        richText.push_back(curData);
    }
    else
        HexDump::getColumnRichText(col, rva, richText);
}

QString CPUStack::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    // Compute RVA
    int wBytePerRowCount = getBytePerRowCount();
    dsint wRva = (rowBase + rowOffset) * wBytePerRowCount - mByteOffset;
    duint wVa = rvaToVa(wRva);

    bool wIsSelected = isSelected(wRva);
    if(wIsSelected) //highlight if selected
        painter->fillRect(QRect(x, y, w, h), QBrush(selectionColor));

    if(col == 0) // paint stack address
    {
        QColor background;
        if(DbgGetLabelAt(wVa, SEG_DEFAULT, nullptr)) //label
        {
            if(wVa == mCsp) //CSP
            {
                background = ConfigColor("StackCspBackgroundColor");
                painter->setPen(QPen(ConfigColor("StackCspColor")));
            }
            else //no CSP
            {
                background = ConfigColor("StackLabelBackgroundColor");
                painter->setPen(ConfigColor("StackLabelColor"));
            }
        }
        else //no label
        {
            if(wVa == mCsp) //CSP
            {
                background = ConfigColor("StackCspBackgroundColor");
                painter->setPen(QPen(ConfigColor("StackCspColor")));
            }
            else if(wIsSelected) //selected normal address
            {
                background = ConfigColor("StackSelectedAddressBackgroundColor");
                painter->setPen(QPen(ConfigColor("StackSelectedAddressColor"))); //black address (DisassemblySelectedAddressColor)
            }
            else //normal address
            {
                background = ConfigColor("StackAddressBackgroundColor");
                painter->setPen(QPen(ConfigColor("StackAddressColor")));
            }
        }
        if(background.alpha())
            painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background when defined
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, makeAddrText(wVa));
        return QString();
    }
    else if(col == 1) // paint stack data
    {
        if(mCallstack.size())
        {
            int stackFrameBitfield = 0; // 0:none, 1:top of stack frame, 2:bottom of stack frame, 4:middle of stack frame
            int party = 0;
            if(wVa >= mCallstack[0].addr)
            {
                for(size_t i = 0; i < mCallstack.size() - 1; i++)
                {
                    if(wVa >= mCallstack[i].addr && wVa < mCallstack[i + 1].addr)
                    {
                        stackFrameBitfield |= (mCallstack[i].addr == wVa) ? 1 : 0;
                        stackFrameBitfield |= (mCallstack[i + 1].addr == wVa + sizeof(duint)) ? 2 : 0;
                        if(stackFrameBitfield == 0)
                            stackFrameBitfield = 4;
                        party = mCallstack[i].party;
                        break;
                    }
                }
                // draw stack frame
                if(stackFrameBitfield == 0)
                    return HexDump::paintContent(painter, rowBase, rowOffset, 1, x, y, w, h);
                else
                {
                    int height = getRowHeight();
                    int halfHeight = height / 2;
                    int width = 5;
                    int offset = 2;
                    auto result = HexDump::paintContent(painter, rowBase, rowOffset, 1, x + (width - 2), y, w - (width - 2), h);
                    if(party == 0)
                        painter->setPen(QPen(mUserStackFrameColor, 2));
                    else
                        painter->setPen(QPen(mSystemStackFrameColor, 2));
                    if((stackFrameBitfield & 1) != 0)
                    {
                        painter->drawLine(x + width, y + halfHeight / 2, x + offset, y + halfHeight / 2);
                        painter->drawLine(x + offset, y + halfHeight / 2, x + offset, y + halfHeight);
                    }
                    else
                        painter->drawLine(x + offset, y, x + offset, y + halfHeight);
                    if((stackFrameBitfield & 2) != 0)
                    {
                        painter->drawLine(x + width, y + height / 4 * 3, x + offset, y + height / 4 * 3);
                        painter->drawLine(x + offset, y + height / 4 * 3, x + offset, y + halfHeight);
                    }
                    else
                        painter->drawLine(x + offset, y + height, x + offset, y + halfHeight);
                    return result;
                }
            }
            else
                return HexDump::paintContent(painter, rowBase, rowOffset, 1, x, y, w, h);
        }
        else
            return HexDump::paintContent(painter, rowBase, rowOffset, 1, x, y, w, h);
    }
    else
        return HexDump::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
}

void CPUStack::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu wMenu(this); //create context menu
    mMenuBuilder->build(&wMenu);
    wMenu.exec(event->globalPos());
}

void CPUStack::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->button() != Qt::LeftButton || !DbgIsDebugging())
        return;
    switch(getColumnIndexFromX(event->x()))
    {
    case 0: //address
    {
        //very ugly way to calculate the base of the current row (no clue why it works)
        dsint deltaRowBase = getInitialSelection() % getBytePerRowCount() + mByteOffset;
        if(deltaRowBase >= getBytePerRowCount())
            deltaRowBase -= getBytePerRowCount();
        dsint mSelectedVa = rvaToVa(getInitialSelection() - deltaRowBase);
        if(mRvaDisplayEnabled && mSelectedVa == mRvaDisplayBase)
            mRvaDisplayEnabled = false;
        else
        {
            mRvaDisplayEnabled = true;
            mRvaDisplayBase = mSelectedVa;
            mRvaDisplayPageBase = mMemPage->getBase();
        }
        reloadData();
    }
    break;

    default:
    {
        modifySlot();
    }
    break;
    }
}

void CPUStack::stackDumpAt(duint addr, duint csp)
{
    if(DbgMemIsValidReadPtr(addr))
        addVaToHistory(addr);
    mCsp = csp;

    // Get the callstack
    DBGCALLSTACK callstack;
    memset(&callstack, 0, sizeof(DBGCALLSTACK));
    DbgFunctions()->GetCallStack(&callstack);
    mCallstack.resize(callstack.total);
    if(mCallstack.size())
    {
        // callstack data highest >> lowest
        std::qsort(callstack.entries, callstack.total, sizeof(DBGCALLSTACKENTRY), [](const void* a, const void* b)
        {
            auto p = (const DBGCALLSTACKENTRY*)a;
            auto q = (const DBGCALLSTACKENTRY*)b;
            if(p->addr < q->addr)
                return -1;
            else
                return 1;
        });
        for(size_t i = 0; i < mCallstack.size(); i++)
        {
            mCallstack[i].addr = callstack.entries[i].addr;
            mCallstack[i].party = DbgFunctions()->ModGetParty(callstack.entries[i].to);
        }
        BridgeFree(callstack.entries);
    }

    printDumpAt(addr);
}

void CPUStack::updateSlot()
{
    if(!DbgIsDebugging())
        return;
    // Get the callstack
    DBGCALLSTACK callstack;
    memset(&callstack, 0, sizeof(DBGCALLSTACK));
    DbgFunctions()->GetCallStack(&callstack);
    mCallstack.resize(callstack.total);
    if(mCallstack.size())
    {
        // callstack data highest >> lowest
        std::qsort(callstack.entries, callstack.total, sizeof(DBGCALLSTACKENTRY), [](const void* a, const void* b)
        {
            auto p = (const DBGCALLSTACKENTRY*)a;
            auto q = (const DBGCALLSTACKENTRY*)b;
            if(p->addr < q->addr)
                return -1;
            else
                return 1;
        });
        for(size_t i = 0; i < mCallstack.size(); i++)
        {
            mCallstack[i].addr = callstack.entries[i].addr;
            mCallstack[i].party = DbgFunctions()->ModGetParty(callstack.entries[i].to);
        }
        BridgeFree(callstack.entries);
    }
}

void CPUStack::gotoCspSlot()
{
    DbgCmdExec("sdump csp");
}

void CPUStack::gotoCbpSlot()
{
    DbgCmdExec("sdump cbp");
}

int CPUStack::getCurrentFrame(const std::vector<CPUStack::CPUCallStack> & mCallstack, duint wVA)
{
    if(mCallstack.size())
        for(size_t i = 0; i < mCallstack.size() - 1; i++)
            if(wVA >= mCallstack[i].addr && wVA < mCallstack[i + 1].addr)
                return int(i);
    return -1;
}

void CPUStack::gotoFrameBaseSlot()
{
    int frame = getCurrentFrame(mCallstack, rvaToVa(getInitialSelection()));
    if(frame != -1)
        DbgCmdExec(QString("sdump \"%1\"").arg(ToPtrString(mCallstack[frame].addr)).toUtf8().constData());
}

void CPUStack::gotoNextFrameSlot()
{
    int frame = getCurrentFrame(mCallstack, rvaToVa(getInitialSelection()));
    if(frame != -1 && frame + 1 < int(mCallstack.size()))
        DbgCmdExec(QString("sdump \"%1\"").arg(ToPtrString(mCallstack[frame + 1].addr)).toUtf8().constData());
}

void CPUStack::gotoPreviousFrameSlot()
{
    int frame = getCurrentFrame(mCallstack, rvaToVa(getInitialSelection()));
    if(frame != -1 && frame > 0)
        DbgCmdExec(QString("sdump \"%1\"").arg(ToPtrString(mCallstack[frame - 1].addr)).toUtf8().constData());
}

void CPUStack::gotoExpressionSlot()
{
    if(!DbgIsDebugging())
        return;
    duint size = 0;
    duint base = DbgMemFindBaseAddr(mCsp, &size);
    if(!mGoto)
        mGoto = new GotoDialog(this);
    mGoto->validRangeStart = base;
    mGoto->validRangeEnd = base + size;
    mGoto->setWindowTitle(tr("Enter expression to follow in Stack..."));
    if(mGoto->exec() == QDialog::Accepted)
    {
        duint value = DbgValFromString(mGoto->expressionText.toUtf8().constData());
        DbgCmdExec(QString().sprintf("sdump %p", value).toUtf8().constData());
    }
}

void CPUStack::gotoPreviousSlot()
{
    historyPrev();
}

void CPUStack::gotoNextSlot()
{
    historyNext();
}

void CPUStack::selectionGet(SELECTIONDATA* selection)
{
    selection->start = rvaToVa(getSelectionStart());
    selection->end = rvaToVa(getSelectionEnd());
    Bridge::getBridge()->setResult(1);
}

void CPUStack::selectionSet(const SELECTIONDATA* selection)
{
    dsint selMin = mMemPage->getBase();
    dsint selMax = selMin + mMemPage->getSize();
    dsint start = selection->start;
    dsint end = selection->end;
    if(start < selMin || start >= selMax || end < selMin || end >= selMax) //selection out of range
    {
        Bridge::getBridge()->setResult(0);
        return;
    }
    setSingleSelection(start - selMin);
    expandSelectionUpTo(end - selMin);
    reloadData();
    Bridge::getBridge()->setResult(1);
}
void CPUStack::selectionUpdatedSlot()
{
    duint selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(duint)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            duint stackBegin = mMemPage->getBase();
            duint stackEnd = stackBegin + mMemPage->getSize();
            if(selectedData >= stackBegin && selectedData < stackEnd) //data is a pointer to stack address
            {
                disconnect(SIGNAL(enterPressedSignal()));
                connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followStackSlot()));
                mFollowDisasm->setShortcut(QKeySequence(""));
                mFollowStack->setShortcut(QKeySequence("enter"));
            }
            else
            {
                disconnect(SIGNAL(enterPressedSignal()));
                connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followDisasmSlot()));
                mFollowStack->setShortcut(QKeySequence(""));
                mFollowDisasm->setShortcut(QKeySequence("enter"));
            }
        }
}

void CPUStack::followDisasmSlot()
{
    duint selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(duint)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            QString addrText = ToPtrString(selectedData);
            DbgCmdExec(QString("disasm " + addrText).toUtf8().constData());
        }
}

void CPUStack::followDumpPtrSlot()
{
    duint selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(duint)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            QString addrText = ToPtrString(selectedData);
            DbgCmdExec(QString("dump " + addrText).toUtf8().constData());
        }
}

void CPUStack::followinDumpNSlot()
{
    duint selectedData = rvaToVa(getInitialSelection());

    if(DbgMemIsValidReadPtr(selectedData))
    {
        for(int i = 0; i < mFollowInDumpActions.length(); i++)
        {
            if(mFollowInDumpActions[i] == sender())
            {
                QString addrText = QString("%1").arg(ToPtrString(selectedData));
                DbgCmdExec(QString("dump [%1], %2").arg(addrText.toUtf8().constData()).arg(i + 1).toUtf8().constData());
            }
        }
    }
}

void CPUStack::followStackSlot()
{
    duint selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(duint)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            QString addrText = ToPtrString(selectedData);
            DbgCmdExec(QString("sdump " + addrText).toUtf8().constData());
        }
}

void CPUStack::watchDataSlot()
{
    DbgCmdExec(QString("AddWatch \"[%1]\", \"uint\"").arg(ToPtrString(rvaToVa(getSelectionStart()))).toUtf8().constData());
}

void CPUStack::binaryEditSlot()
{
    HexEditDialog hexEdit(this);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    hexEdit.setWindowTitle(tr("Edit data at %1").arg(ToPtrString(rvaToVa(selStart))));
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    dsint dataSize = hexEdit.mHexEdit->data().size();
    dsint newSize = selSize > dataSize ? selSize : dataSize;
    data = new byte_t[newSize];
    mMemPage->read(data, selStart, newSize);
    QByteArray patched = hexEdit.mHexEdit->applyMaskedData(QByteArray((const char*)data, newSize));
    mMemPage->write(patched.constData(), selStart, patched.size());
    GuiUpdateAllViews();
}

void CPUStack::binaryFillSlot()
{
    HexEditDialog hexEdit(this);
    hexEdit.showKeepSize(false);
    hexEdit.mHexEdit->setOverwriteMode(false);
    dsint selStart = getSelectionStart();
    hexEdit.setWindowTitle(tr("Fill data at %1").arg(ToPtrString(rvaToVa(selStart))));
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    QString pattern = hexEdit.mHexEdit->pattern();
    dsint selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    hexEdit.mHexEdit->fill(0, QString(pattern));
    QByteArray patched(hexEdit.mHexEdit->data());
    mMemPage->write(patched, selStart, patched.size());
    GuiUpdateAllViews();
}

void CPUStack::binaryCopySlot()
{
    HexEditDialog hexEdit(this);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    Bridge::CopyToClipboard(hexEdit.mHexEdit->pattern(true));
}

void CPUStack::binaryPasteSlot()
{
    HexEditDialog hexEdit(this);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    QClipboard* clipboard = QApplication::clipboard();
    hexEdit.mHexEdit->setData(clipboard->text());

    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    QByteArray patched = hexEdit.mHexEdit->applyMaskedData(QByteArray((const char*)data, selSize));
    if(patched.size() < selSize)
        selSize = patched.size();
    mMemPage->write(patched.constData(), selStart, selSize);
    GuiUpdateAllViews();
}

void CPUStack::binaryPasteIgnoreSizeSlot()
{
    HexEditDialog hexEdit(this);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    QClipboard* clipboard = QApplication::clipboard();
    hexEdit.mHexEdit->setData(clipboard->text());

    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    QByteArray patched = hexEdit.mHexEdit->applyMaskedData(QByteArray((const char*)data, selSize));
    delete [] data;
    mMemPage->write(patched.constData(), selStart, patched.size());
    GuiUpdateAllViews();
}

// Copied from "CPUDump.cpp".
void CPUStack::hardwareAccess1Slot()
{
    QString addr_text = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("bphws " + addr_text + ", r, 1").toUtf8().constData());
}

void CPUStack::hardwareAccess2Slot()
{
    QString addr_text = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("bphws " + addr_text + ", r, 2").toUtf8().constData());
}

void CPUStack::hardwareAccess4Slot()
{
    QString addr_text = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("bphws " + addr_text + ", r, 4").toUtf8().constData());
}

void CPUStack::hardwareAccess8Slot()
{
    QString addr_text = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("bphws " + addr_text + ", r, 8").toUtf8().constData());
}

void CPUStack::hardwareWrite1Slot()
{
    QString addr_text = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("bphws " + addr_text + ", w, 1").toUtf8().constData());
}

void CPUStack::hardwareWrite2Slot()
{
    QString addr_text = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("bphws " + addr_text + ", w, 2").toUtf8().constData());
}

void CPUStack::hardwareWrite4Slot()
{
    QString addr_text = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("bphws " + addr_text + ", w, 4").toUtf8().constData());
}

void CPUStack::hardwareWrite8Slot()
{
    QString addr_text = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("bphws " + addr_text + ", w, 8").toUtf8().constData());
}

void CPUStack::hardwareRemoveSlot()
{
    QString addr_text = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("bphwc " + addr_text).toUtf8().constData());
}

void CPUStack::memoryAccessSingleshootSlot()
{
    QString addr_text = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("bpm " + addr_text + ", 0, a").toUtf8().constData());
}

void CPUStack::memoryAccessRestoreSlot()
{
    QString addr_text = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("bpm " + addr_text + ", 1, a").toUtf8().constData());
}

void CPUStack::memoryWriteSingleshootSlot()
{
    QString addr_text = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("bpm " + addr_text + ", 0, w").toUtf8().constData());
}

void CPUStack::memoryWriteRestoreSlot()
{
    QString addr_text = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("bpm " + addr_text + ", 1, w").toUtf8().constData());
}

void CPUStack::memoryRemoveSlot()
{
    QString addr_text = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("bpmc " + addr_text).toUtf8().constData());
}

void CPUStack::findPattern()
{
    HexEditDialog hexEdit(this);
    hexEdit.showEntireBlock(true);
    hexEdit.mHexEdit->setOverwriteMode(false);
    hexEdit.setWindowTitle(tr("Find Pattern..."));
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    dsint addr = rvaToVa(getSelectionStart());
    if(hexEdit.entireBlock())
        addr = DbgMemFindBaseAddr(addr, 0);
    QString addrText = ToPtrString(addr);
    DbgCmdExec(QString("findall " + addrText + ", " + hexEdit.mHexEdit->pattern() + ", &data&").toUtf8().constData());
    emit displayReferencesWidget();
}

void CPUStack::undoSelectionSlot()
{
    dsint start = rvaToVa(getSelectionStart());
    dsint end = rvaToVa(getSelectionEnd());
    if(!DbgFunctions()->PatchInRange(start, end)) //nothing patched in selected range
        return;
    DbgFunctions()->PatchRestoreRange(start, end);
    reloadData();
}

void CPUStack::modifySlot()
{
    dsint addr = getInitialSelection();
    WordEditDialog wEditDialog(this);
    dsint value = 0;
    mMemPage->read(&value, addr, sizeof(dsint));
    wEditDialog.setup(tr("Modify"), value, sizeof(dsint));
    if(wEditDialog.exec() != QDialog::Accepted)
        return;
    value = wEditDialog.getVal();
    mMemPage->write(&value, addr, sizeof(dsint));
    GuiUpdateAllViews();
}

void CPUStack::pushSlot()
{
    WordEditDialog wEditDialog(this);
    dsint value = 0;
    wEditDialog.setup(ArchValue(tr("Push DWORD"), tr("Push QWORD")), value, sizeof(dsint));
    if(wEditDialog.exec() != QDialog::Accepted)
        return;
    value = wEditDialog.getVal();
    mCsp -= sizeof(dsint);
    DbgValToString("csp", mCsp);
    DbgMemWrite(mCsp, (const unsigned char*)&value, sizeof(dsint));
    GuiUpdateAllViews();
}

void CPUStack::popSlot()
{
    mCsp += sizeof(dsint);
    DbgValToString("csp", mCsp);
    GuiUpdateAllViews();
}

void CPUStack::realignSlot()
{
#ifdef _WIN64
    mCsp &= ~0x7;
#else //x86
    mCsp &= ~0x3;
#endif //_WIN64
    DbgValToString("csp", mCsp);
    GuiUpdateAllViews();
}

void CPUStack::freezeStackSlot()
{
    if(bStackFrozen)
        DbgCmdExec(QString("setfreezestack 0").toUtf8().constData());
    else
        DbgCmdExec(QString("setfreezestack 1").toUtf8().constData());

    bStackFrozen = !bStackFrozen;

    updateFreezeStackAction();
    if(!bStackFrozen)
        gotoCspSlot();
}

void CPUStack::dbgStateChangedSlot(DBGSTATE state)
{
    if(state == initialized)
    {
        bStackFrozen = false;
        updateFreezeStackAction();
    }
}

void CPUStack::followInMemoryMapSlot()
{
    DbgCmdExec(QString("memmapdump [%1]").arg(ToHexString(rvaToVa(getInitialSelection()))).toUtf8().constData());
}

void CPUStack::followInDumpSlot()
{
    DbgCmdExec(QString("dump %1").arg(ToHexString(rvaToVa(getInitialSelection()))).toUtf8().constData());
}
