#include "HexDump.h"
#include "Configuration.h"
#include "Bridge.h"
#include "StringUtil.h"
#include <QMessageBox>

HexDump::HexDump(QWidget* parent)
    : AbstractTableView(parent)
{
    SelectionData_t data;
    memset(&data, 0, sizeof(SelectionData_t));
    mSelection = data;

    mGuiState = HexDump::NoState;

    setRowCount(0);

    mMemPage = new MemoryPage(0, 0);
    mForceColumn = -1;

    clearDescriptors();

    backgroundColor = ConfigColor("HexDumpBackgroundColor");
    textColor = ConfigColor("HexDumpTextColor");
    selectionColor = ConfigColor("HexDumpSelectionColor");

    mRvaDisplayEnabled = false;
    mSyncAddrExpression = "";
    mNonprintReplace = QChar('.'); //QChar(0x25CA);
    mNullReplace = QChar('.'); //QChar(0x2022);

    historyClear();

    // Slots
    connect(Bridge::getBridge(), SIGNAL(updateDump()), this, SLOT(updateDumpSlot()));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChanged(DBGSTATE)));
    setupCopyMenu();

    Initialize();
}

HexDump::~HexDump()
{
    delete mMemPage;
}

void HexDump::updateColors()
{
    AbstractTableView::updateColors();

    backgroundColor = ConfigColor("HexDumpBackgroundColor");
    textColor = ConfigColor("HexDumpTextColor");
    selectionColor = ConfigColor("HexDumpSelectionColor");

    mModifiedBytesColor = ConfigColor("HexDumpModifiedBytesColor");
    mModifiedBytesBackgroundColor = ConfigColor("HexDumpModifiedBytesBackgroundColor");
    mRestoredBytesColor = ConfigColor("HexDumpRestoredBytesColor");
    mRestoredBytesBackgroundColor = ConfigColor("HexDumpRestoredBytesBackgroundColor");
    mByte00Color = ConfigColor("HexDumpByte00Color");
    mByte00BackgroundColor = ConfigColor("HexDumpByte00BackgroundColor");
    mByte7FColor = ConfigColor("HexDumpByte7FColor");
    mByte7FBackgroundColor = ConfigColor("HexDumpByte7FBackgroundColor");
    mByteFFColor = ConfigColor("HexDumpByteFFColor");
    mByteFFBackgroundColor = ConfigColor("HexDumpByteFFBackgroundColor");
    mByteIsPrintColor = ConfigColor("HexDumpByteIsPrintColor");
    mByteIsPrintBackgroundColor = ConfigColor("HexDumpByteIsPrintBackgroundColor");

    mUserModuleCodePointerHighlightColor = ConfigColor("HexDumpUserModuleCodePointerHighlightColor");
    mUserModuleDataPointerHighlightColor = ConfigColor("HexDumpUserModuleDataPointerHighlightColor");
    mSystemModuleCodePointerHighlightColor = ConfigColor("HexDumpSystemModuleCodePointerHighlightColor");
    mSystemModuleDataPointerHighlightColor = ConfigColor("HexDumpSystemModuleDataPointerHighlightColor");
    mUnknownCodePointerHighlightColor = ConfigColor("HexDumpUnknownCodePointerHighlightColor");
    mUnknownDataPointerHighlightColor = ConfigColor("HexDumpUnknownDataPointerHighlightColor");

    reloadData();
}

void HexDump::updateFonts()
{
    duint setting;
    if(BridgeSettingGetUint("Gui", "NonprintReplaceCharacter", &setting))
        mNonprintReplace = QChar(uint(setting));
    if(BridgeSettingGetUint("Gui", "NullReplaceCharacter", &setting))
        mNullReplace = QChar(uint(setting));
    setFont(ConfigFont("HexDump"));
    invalidateCachedFont();
}

void HexDump::updateShortcuts()
{
    AbstractTableView::updateShortcuts();
    mCopyAddress->setShortcut(ConfigShortcut("ActionCopyAddress"));
    mCopyRva->setShortcut(ConfigShortcut("ActionCopyRva"));
    mCopySelection->setShortcut(ConfigShortcut("ActionCopy"));
}

void HexDump::updateDumpSlot()
{
    if(mSyncAddrExpression.length() && DbgFunctions()->ValFromString)
    {
        duint syncAddr;
        if(DbgFunctions()->ValFromString(mSyncAddrExpression.toUtf8().constData(), &syncAddr)
                && DbgMemIsValidReadPtr(syncAddr))
        {
            printDumpAt(syncAddr, false, false, true);
        }
    }
    reloadData();
}

void HexDump::copySelectionSlot()
{
    Bridge::CopyToClipboard(makeCopyText());
}

void HexDump::printDumpAt(dsint parVA, bool select, bool repaint, bool updateTableOffset)
{
    duint wSize;
    auto wBase = DbgMemFindBaseAddr(parVA, &wSize); //get memory base
    unsigned char test;
    if(!wBase || !wSize || !DbgMemRead(wBase, &test, sizeof(test)))
        return;
    dsint wRVA = parVA - wBase; //calculate rva
    int wBytePerRowCount = getBytePerRowCount(); //get the number of bytes per row
    dsint wRowCount;

    // Byte offset used to be aligned on the given RVA
    mByteOffset = (int)((dsint)wRVA % (dsint)wBytePerRowCount);
    mByteOffset = mByteOffset > 0 ? wBytePerRowCount - mByteOffset : 0;

    // Compute row count
    wRowCount = wSize / wBytePerRowCount;
    wRowCount += mByteOffset > 0 ? 1 : 0;

    if(mRvaDisplayEnabled && mMemPage->getBase() != mRvaDisplayPageBase)
        mRvaDisplayEnabled = false;

    setRowCount(wRowCount); //set the number of rows

    mMemPage->setAttributes(wBase, wSize);  // Set base and size (Useful when memory page changed)

    if(updateTableOffset)
    {
        setTableOffset(-1); //make sure the requested address is always first
        setTableOffset((wRVA + mByteOffset) / wBytePerRowCount); //change the displayed offset
    }

    if(select)
    {
        setSingleSelection(wRVA);
        dsint wEndingAddress = wRVA + getSizeOf(mDescriptor.at(0).data.itemSize) - 1;
        expandSelectionUpTo(wEndingAddress);
    }

    if(repaint)
        reloadData();
}

void HexDump::printDumpAt(dsint parVA)
{
    printDumpAt(parVA, true);
}

duint HexDump::rvaToVa(dsint rva)
{
    return mMemPage->va(rva);
}

duint HexDump::getTableOffsetRva()
{
    return getTableOffset() * getBytePerRowCount() - mByteOffset;
}

QString HexDump::makeAddrText(duint va)
{
    char label[MAX_LABEL_SIZE] = "";
    QString addrText = "";
    dsint cur_addr = va;
    if(mRvaDisplayEnabled) //RVA display
    {
        dsint rva = cur_addr - mRvaDisplayBase;
        if(rva == 0)
        {
#ifdef _WIN64
            addrText = "$ ==>            ";
#else
            addrText = "$ ==>    ";
#endif //_WIN64
        }
        else if(rva > 0)
        {
#ifdef _WIN64
            addrText = "$+" + QString("%1").arg(rva, -15, 16, QChar(' ')).toUpper();
#else
            addrText = "$+" + QString("%1").arg(rva, -7, 16, QChar(' ')).toUpper();
#endif //_WIN64
        }
        else if(rva < 0)
        {
#ifdef _WIN64
            addrText = "$-" + QString("%1").arg(-rva, -15, 16, QChar(' ')).toUpper();
#else
            addrText = "$-" + QString("%1").arg(-rva, -7, 16, QChar(' ')).toUpper();
#endif //_WIN64
        }
    }
    addrText += ToPtrString(cur_addr);
    if(DbgGetLabelAt(cur_addr, SEG_DEFAULT, label)) //has label
    {
        char module[MAX_MODULE_SIZE] = "";
        if(DbgGetModuleAt(cur_addr, module) && !QString(label).startsWith("JMP.&"))
            addrText += " <" + QString(module) + "." + QString(label) + ">";
        else
            addrText += " <" + QString(label) + ">";
    }
    else
        *label = 0;
    return std::move(addrText);
}

QString HexDump::makeCopyText()
{
    dsint deltaRowBase = getSelectionStart() % getBytePerRowCount() + mByteOffset;
    if(deltaRowBase >= getBytePerRowCount())
        deltaRowBase -= getBytePerRowCount();
    auto curRow = getSelectionStart() - deltaRowBase;
    QString result;
    while(curRow <= getSelectionEnd())
    {
        for(int col = 0; col < getColumnCount(); col++)
        {
            if(col)
                result += " ";
            RichTextPainter::List richText;
            getColumnRichText(col, curRow, richText);
            QString colText;
            for(auto & r : richText)
                colText += r.text;
            if(col + 1 == getColumnCount())
                result += colText;
            else
                result += colText.leftJustified(getColumnWidth(col) / getCharWidth(), QChar(' '), true);
        }
        curRow += getBytePerRowCount();
        result += "\n";
    }
    return std::move(result);
}

void HexDump::addVaToHistory(dsint parVa)
{
    //truncate everything right from the current VA
    if(mVaHistory.size() && mCurrentVa < mVaHistory.size() - 1) //mCurrentVa is not the last
        mVaHistory.erase(mVaHistory.begin() + mCurrentVa + 1, mVaHistory.end());

    //do not have 2x the same va in a row
    if(!mVaHistory.size() || mVaHistory.last() != parVa)
    {
        mCurrentVa++;
        mVaHistory.push_back(parVa);
    }
}

bool HexDump::historyHasPrev()
{
    if(!mCurrentVa || !mVaHistory.size()) //we are at the earliest history entry
        return false;
    return true;
}

bool HexDump::historyHasNext()
{
    int size = mVaHistory.size();
    if(!size || mCurrentVa >= mVaHistory.size() - 1) //we are at the newest history entry
        return false;
    return true;
}

void HexDump::historyPrev()
{
    if(!historyHasPrev())
        return;
    mCurrentVa--;
    printDumpAt(mVaHistory.at(mCurrentVa));
}

void HexDump::historyNext()
{
    if(!historyHasNext())
        return;
    mCurrentVa++;
    printDumpAt(mVaHistory.at(mCurrentVa));
}

void HexDump::historyClear()
{
    mCurrentVa = -1;
    mVaHistory.clear();
}

void HexDump::setupCopyMenu()
{
    // Copy -> Data
    mCopySelection = new QAction(DIcon("copy_selection.png"), tr("&Selected lines"), this);
    connect(mCopySelection, SIGNAL(triggered(bool)), this, SLOT(copySelectionSlot()));
    mCopySelection->setShortcutContext(Qt::WidgetShortcut);
    addAction(mCopySelection);

    // Copy -> Address
    mCopyAddress = new QAction(DIcon("copy_address.png"), tr("&Address"), this);
    connect(mCopyAddress, SIGNAL(triggered()), this, SLOT(copyAddressSlot()));
    mCopyAddress->setShortcutContext(Qt::WidgetShortcut);
    addAction(mCopyAddress);

    // Copy -> RVA
    mCopyRva = new QAction(DIcon("copy_address.png"), "&RVA", this);
    connect(mCopyRva, SIGNAL(triggered()), this, SLOT(copyRvaSlot()));
    mCopyRva->setShortcutContext(Qt::WidgetShortcut);
    addAction(mCopyRva);
}

void HexDump::copyAddressSlot()
{
    QString addrText = ToPtrString(rvaToVa(getInitialSelection()));
    Bridge::CopyToClipboard(addrText);
}

void HexDump::copyRvaSlot()
{
    duint addr = rvaToVa(getInitialSelection());
    duint base = DbgFunctions()->ModBaseFromAddr(addr);
    if(base)
    {
        QString addrText = ToHexString(addr - base);
        Bridge::CopyToClipboard(addrText);
    }
    else
        QMessageBox::warning(this, tr("Error!"), tr("Selection not in a module..."));
}

void HexDump::mouseMoveEvent(QMouseEvent* event)
{
    bool wAccept = true;

    int x = event->x();
    int y = event->y();

    if(mGuiState == HexDump::MultiRowsSelectionState)
    {
        //qDebug() << "State = MultiRowsSelectionState";

        if((transY(y) >= 0) && y <= this->height())
        {
            int wColIndex = getColumnIndexFromX(x);

            if(mForceColumn != -1)
            {
                wColIndex = mForceColumn;
                x = getColumnPosition(mForceColumn) + 1;
            }

            if(wColIndex > 0) // No selection for first column (addresses)
            {
                dsint wStartingAddress = getItemStartingAddress(x, y);
                dsint dataSize = getSizeOf(mDescriptor.at(wColIndex - 1).data.itemSize) - 1;
                dsint wEndingAddress = wStartingAddress + dataSize;

                if(wEndingAddress < (dsint)mMemPage->getSize())
                {
                    if(wStartingAddress < getInitialSelection())
                    {
                        expandSelectionUpTo(wStartingAddress);
                        mSelection.toIndex += dataSize;
                        emit selectionUpdated();
                    }
                    else
                        expandSelectionUpTo(wEndingAddress);

                    mGuiState = HexDump::MultiRowsSelectionState;

                    updateViewport();
                }
            }
            else
            {
                dsint wStartingAddress = getItemStartingAddress(getColumnPosition(1) + 1, y);
                dsint dataSize = getSizeOf(mDescriptor.at(0).data.itemSize) * mDescriptor.at(0).itemCount - 1;
                dsint wEndingAddress = wStartingAddress + dataSize;

                if(wEndingAddress < (dsint)mMemPage->getSize())
                {
                    if(wStartingAddress < getInitialSelection())
                    {
                        expandSelectionUpTo(wStartingAddress);
                        mSelection.toIndex += dataSize;
                        emit selectionUpdated();
                    }
                    else
                        expandSelectionUpTo(wEndingAddress);

                    mGuiState = HexDump::MultiRowsSelectionState;

                    updateViewport();
                }
            }

            wAccept = true;
        }
        else if(y > this->height() && mGuiState == HexDump::MultiRowsSelectionState)
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        }
        else if(transY(y) < 0 && mGuiState == HexDump::MultiRowsSelectionState)
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        }
    }

    if(wAccept == true)
        AbstractTableView::mouseMoveEvent(event);
}

void HexDump::mousePressEvent(QMouseEvent* event)
{
    if(event->buttons() == Qt::MiddleButton) //copy address to clipboard
    {
        if(!DbgIsDebugging())
            return;
        MessageBeep(MB_OK);
        QString addrText = ToPtrString(rvaToVa(getInitialSelection()));
        Bridge::CopyToClipboard(addrText);
        return;
    }
    //qDebug() << "HexDump::mousePressEvent";

    int x = event->x();
    int y = event->y();

    bool wAccept = false;

    if(((event->buttons() & Qt::LeftButton) != 0) && ((event->buttons() & Qt::RightButton) == 0))
    {
        if(getGuiState() == AbstractTableView::NoState)
        {
            if(y > getHeaderHeight() && y <= this->height())
            {
                int wColIndex = getColumnIndexFromX(x);

                if(mForceColumn != -1)
                {
                    wColIndex = mForceColumn;
                    x = getColumnPosition(mForceColumn) + 1;
                }

                if(wColIndex > 0 && mDescriptor.at(wColIndex - 1).isData == true) // No selection for first column (addresses) and no data columns
                {
                    dsint wStartingAddress = getItemStartingAddress(x, y);
                    dsint dataSize = getSizeOf(mDescriptor.at(wColIndex - 1).data.itemSize) - 1;
                    dsint wEndingAddress = wStartingAddress + dataSize;

                    if(wEndingAddress < (dsint)mMemPage->getSize())
                    {
                        bool bUpdateTo = false;
                        if(!(event->modifiers() & Qt::ShiftModifier))
                            setSingleSelection(wStartingAddress);
                        else if(getInitialSelection() > wEndingAddress)
                        {
                            wEndingAddress -= dataSize;
                            bUpdateTo = true;
                        }
                        expandSelectionUpTo(wEndingAddress);
                        if(bUpdateTo)
                        {
                            mSelection.toIndex += dataSize;
                            emit selectionUpdated();
                        }

                        mGuiState = HexDump::MultiRowsSelectionState;

                        updateViewport();
                    }
                }
                else if(wColIndex == 0)
                {
                    dsint wStartingAddress = getItemStartingAddress(getColumnPosition(1) + 1, y);
                    dsint dataSize = getSizeOf(mDescriptor.at(0).data.itemSize) * mDescriptor.at(0).itemCount - 1;
                    dsint wEndingAddress = wStartingAddress + dataSize;

                    if(wEndingAddress < (dsint)mMemPage->getSize())
                    {
                        bool bUpdateTo = false;
                        if(!(event->modifiers() & Qt::ShiftModifier))
                            setSingleSelection(wStartingAddress);
                        else if(getInitialSelection() > wEndingAddress)
                        {
                            wEndingAddress -= dataSize;
                            bUpdateTo = true;
                        }
                        expandSelectionUpTo(wEndingAddress);
                        if(bUpdateTo)
                        {
                            mSelection.toIndex += dataSize;
                            emit selectionUpdated();
                        }

                        mGuiState = HexDump::MultiRowsSelectionState;

                        updateViewport();
                    }
                }

                wAccept = true;
            }
        }
    }

    if(wAccept == false)
        AbstractTableView::mousePressEvent(event);
}

void HexDump::mouseReleaseEvent(QMouseEvent* event)
{
    bool wAccept = true;

    if((event->buttons() & Qt::LeftButton) == 0)
    {
        if(mGuiState == HexDump::MultiRowsSelectionState)
        {
            mGuiState = HexDump::NoState;

            updateViewport();

            wAccept = false;
        }
    }
    if((event->button() & Qt::BackButton) != 0)
    {
        wAccept = true;
        historyPrev();
    }
    else if((event->button() & Qt::ForwardButton) != 0)
    {
        wAccept = true;
        historyNext();
    }

    if(wAccept == true)
        AbstractTableView::mouseReleaseEvent(event);
}

void HexDump::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Up && event->modifiers() == Qt::ControlModifier)
    {
        duint offsetVa = rvaToVa(getTableOffsetRva()) - 1;
        if(DbgMemFindBaseAddr(rvaToVa(getTableOffsetRva()), nullptr) == DbgMemFindBaseAddr(offsetVa, nullptr))
        {
            printDumpAt(offsetVa);
            addVaToHistory(offsetVa);
        }
    }
    else if(event->key() == Qt::Key_Down && event->modifiers() == Qt::ControlModifier)
    {
        duint offsetVa = rvaToVa(getTableOffsetRva()) + 1;
        if(DbgMemFindBaseAddr(rvaToVa(getTableOffsetRva()), nullptr) == DbgMemFindBaseAddr(offsetVa, nullptr))
        {
            printDumpAt(offsetVa);
            addVaToHistory(offsetVa);
        }
    }
    else
        AbstractTableView::keyPressEvent(event);
}

QString HexDump::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    // Reset byte offset when base address is reached
    if(rowBase == 0 && mByteOffset != 0)
        printDumpAt(mMemPage->getBase(), false, false);

    // Compute RVA
    int wBytePerRowCount = getBytePerRowCount();
    dsint wRva = (rowBase + rowOffset) * wBytePerRowCount - mByteOffset;

    if(col && mDescriptor.at(col - 1).isData)
        printSelected(painter, rowBase, rowOffset, col, x, y, w, h);

    RichTextPainter::List richText;
    getColumnRichText(col, wRva, richText);
    RichTextPainter::paintRichText(painter, x, y, w, h, 4, richText, mFontMetrics);

    return QString();
}

void HexDump::printSelected(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    if((col > 0) && ((col - 1) < mDescriptor.size()))
    {
        ColumnDescriptor_t curDescriptor = mDescriptor.at(col - 1);
        int wBytePerRowCount = getBytePerRowCount();
        dsint wRva = (rowBase + rowOffset) * wBytePerRowCount - mByteOffset;
        int wItemPixWidth = getItemPixelWidth(curDescriptor);
        int wCharWidth = getCharWidth();
        if(wItemPixWidth == wCharWidth)
            x += 4;

        for(int i = 0; i < curDescriptor.itemCount; i++)
        {
            int wSelectionX = x + i * wItemPixWidth;
            if(isSelected(wRva + i * getSizeOf(curDescriptor.data.itemSize)) == true)
            {
                int wSelectionWidth = wItemPixWidth > w - (wSelectionX - x) ? w - (wSelectionX - x) : wItemPixWidth;
                wSelectionWidth = wSelectionWidth < 0 ? 0 : wSelectionWidth;
                painter->setPen(textColor);
                painter->fillRect(QRect(wSelectionX, y, wSelectionWidth, h), QBrush(selectionColor));
            }
            int separator = curDescriptor.separator;
            if(i && separator && !(i % separator))
            {
                painter->setPen(separatorColor);
                painter->drawLine(wSelectionX, y, wSelectionX, y + h);
            }
        }
    }
}

/************************************************************************************
                                Selection Management
************************************************************************************/
void HexDump::expandSelectionUpTo(dsint rva)
{
    if(rva < mSelection.firstSelectedIndex)
    {
        mSelection.fromIndex = rva;
        mSelection.toIndex = mSelection.firstSelectedIndex;
        emit selectionUpdated();
    }
    else if(rva > mSelection.firstSelectedIndex)
    {
        mSelection.fromIndex = mSelection.firstSelectedIndex;
        mSelection.toIndex = rva;
        emit selectionUpdated();
    }
    else if(rva == mSelection.firstSelectedIndex)
    {
        setSingleSelection(rva);
    }
}

void HexDump::setSingleSelection(dsint rva)
{
    mSelection.firstSelectedIndex = rva;
    mSelection.fromIndex = rva;
    mSelection.toIndex = rva;
    emit selectionUpdated();
}

dsint HexDump::getInitialSelection()
{
    return mSelection.firstSelectedIndex;
}

dsint HexDump::getSelectionStart()
{
    return mSelection.fromIndex;
}

dsint HexDump::getSelectionEnd()
{
    return mSelection.toIndex;
}

bool HexDump::isSelected(dsint rva)
{
    if(rva >= mSelection.fromIndex && rva <= mSelection.toIndex)
        return true;
    else
        return false;
}

void HexDump::getColumnRichText(int col, dsint rva, RichTextPainter::List & richText)
{
    RichTextPainter::CustomRichText_t curData;
    curData.highlight = false;
    curData.flags = RichTextPainter::FlagAll;
    curData.textColor = textColor;
    curData.textBackground = Qt::transparent;
    curData.highlightColor = Qt::transparent;

    RichTextPainter::CustomRichText_t spaceData;
    spaceData.highlight = false;
    spaceData.flags = RichTextPainter::FlagNone;
    spaceData.highlightColor = Qt::transparent;

    if(!col) //address
    {
        curData.text = makeAddrText(rvaToVa(rva));
        richText.push_back(curData);
    }
    else if(mDescriptor.at(col - 1).isData == true)
    {
        const ColumnDescriptor_t & desc = mDescriptor.at(col - 1);
        int wI;

        int wByteCount = getSizeOf(desc.data.itemSize);
        int wBufferByteCount = desc.itemCount * wByteCount;

        wBufferByteCount = wBufferByteCount > (dsint)(mMemPage->getSize() - rva) ? mMemPage->getSize() - rva : wBufferByteCount;

        byte_t* wData = new byte_t[wBufferByteCount];
        mMemPage->read(wData, rva, wBufferByteCount);

        if(desc.textCodec) //convert the row bytes to unicode
        {
            //This might produce invalid characters in variables-width encodings. This is currently ignored.
            curData.text = desc.textCodec->toUnicode(QByteArray((const char*)wData, wBufferByteCount));
            curData.text.replace('\t', "\\t");
            curData.text.replace('\f', "\\f");
            curData.text.replace('\v', "\\v");
            curData.text.replace('\n', "\\n");
            curData.text.replace('\r', "\\r");
            richText.push_back(curData);
        }
        else
        {
            for(wI = 0; wI < desc.itemCount && (rva + wI) < (dsint)mMemPage->getSize(); wI++)
            {
                curData.text.clear();
                curData.textColor = textColor;
                curData.textBackground = Qt::transparent;
                curData.flags = RichTextPainter::FlagAll;

                int maxLen = getStringMaxLength(desc.data);
                if((rva + wI + wByteCount - 1) < (dsint)mMemPage->getSize())
                {
                    toString(desc.data, rva + wI * wByteCount, wData + wI * wByteCount, curData);
                    if(curData.text.length() < maxLen)
                    {
                        spaceData.text = QString(' ').repeated(maxLen - curData.text.length());
                        richText.push_back(spaceData);
                    }
                    if(wI % sizeof(duint) == 0 && wByteCount == 1 && desc.data.byteMode == HexByte) //pointer underlining
                    {
                        auto ptr = *(duint*)(wData + wI * wByteCount);
                        if(spaceData.highlight = curData.highlight = DbgMemIsValidReadPtr(ptr))
                        {
                            auto codePage = DbgFunctions()->MemIsCodePage(ptr, false);
                            auto modbase = DbgFunctions()->ModBaseFromAddr(ptr);
                            if(modbase)
                            {
                                if(DbgFunctions()->ModGetParty(modbase) == 1) //system
                                    spaceData.highlightColor = curData.highlightColor = codePage ? mSystemModuleCodePointerHighlightColor : mSystemModuleDataPointerHighlightColor;
                                else //user
                                    spaceData.highlightColor = curData.highlightColor = codePage ? mUserModuleCodePointerHighlightColor : mUserModuleDataPointerHighlightColor;
                            }
                            else
                                spaceData.highlightColor = curData.highlightColor = codePage ? mUnknownCodePointerHighlightColor : mUnknownDataPointerHighlightColor;
                        }
                    }
                    richText.push_back(curData);
                    if(maxLen)
                    {
                        spaceData.text = QString(' ');
                        if(wI % sizeof(duint) == sizeof(duint) - 1)
                            spaceData.highlight = false;
                        richText.push_back(spaceData);
                    }
                }
                else
                {
                    curData.text = QString("?").rightJustified(maxLen, ' ');
                    if(maxLen)
                        curData.text.append(' ');
                    richText.push_back(curData);
                }
            }
        }

        delete[] wData;
    }
}

void HexDump::toString(DataDescriptor_t desc, duint rva, byte_t* data, RichTextPainter::CustomRichText_t & richText) //convert data to string
{
    switch(desc.itemSize)
    {
    case Byte:
    {
        byteToString(rva, *((byte_t*)data), desc.byteMode, richText);
    }
    break;

    case Word:
    {
        wordToString(rva, *((uint16*)data), desc.wordMode, richText);
    }
    break;

    case Dword:
    {
        dwordToString(rva, *((uint32*)data), desc.dwordMode, richText);
    }
    break;

    case Qword:
    {
        qwordToString(rva, *((uint64*)data), desc.qwordMode, richText);
    }
    break;

    case Tword:
    {
        twordToString(rva, data, desc.twordMode, richText);
    }
    break;

    default:
    {

    }
    break;
    }

    if(desc.itemSize == Byte) //byte patches are handled in byteToString
        return;

    dsint start = rvaToVa(rva);
    dsint end = start + getSizeOf(desc.itemSize) - 1;
    if(DbgFunctions()->PatchInRange(start, end))
        richText.textColor = ConfigColor("HexDumpModifiedBytesColor");
}

void HexDump::byteToString(duint rva, byte_t byte, ByteViewMode_e mode, RichTextPainter::CustomRichText_t & richText)
{
    QString wStr = "";

    switch(mode)
    {
    case HexByte:
    {
        wStr = ToByteString((unsigned char)byte);
    }
    break;

    case AsciiByte:
    {
        QChar wChar = QChar::fromLatin1((char)byte);

        if(wChar.isPrint())
            wStr = QString(wChar);
        else if(!wChar.unicode())
            wStr = mNullReplace;
        else
            wStr = mNonprintReplace;
    }
    break;

    case SignedDecByte:
    {
        wStr = QString::number((int)((char)byte));
    }
    break;

    case UnsignedDecByte:
    {
        wStr = QString::number((unsigned int)byte);
    }
    break;

    default:
    {

    }
    break;
    }

    richText.text = wStr;

    DBGPATCHINFO patchInfo;
    if(DbgFunctions()->PatchGetEx(rvaToVa(rva), &patchInfo))
    {
        if(byte == patchInfo.newbyte)
        {
            richText.textColor = mModifiedBytesColor;
            richText.textBackground = mModifiedBytesBackgroundColor;
        }
        else
        {
            richText.textColor = mRestoredBytesColor;
            richText.textBackground = mRestoredBytesBackgroundColor;
        }
    }
    else
    {
        switch(byte)
        {
        case 0x00:
            richText.textColor = mByte00Color;
            richText.textBackground = mByte00BackgroundColor;
            break;
        case 0x7F:
            richText.textColor = mByte7FColor;
            richText.textBackground = mByte7FBackgroundColor;
            break;
        case 0xFF:
            richText.textColor = mByteFFColor;
            richText.textBackground = mByteFFBackgroundColor;
            break;
        default:
            if(isprint(byte) || isspace(byte))
            {
                richText.textColor = mByteIsPrintColor;
                richText.textBackground = mByteIsPrintBackgroundColor;
            }
            break;
        }
    }
}

void HexDump::wordToString(duint rva, uint16 word, WordViewMode_e mode, RichTextPainter::CustomRichText_t & richText)
{
    Q_UNUSED(rva);
    QString wStr;

    switch(mode)
    {
    case HexWord:
    {
        wStr = ToWordString((unsigned short)word);
    }
    break;

    case UnicodeWord:
    {
        QChar wChar = QChar::fromLatin1((char)word & 0xFF);
        if(wChar.isPrint() && (word >> 8) == 0)
            wStr = QString(wChar);
        else if(!wChar.unicode())
            wStr = mNullReplace;
        else
            wStr = mNonprintReplace;
    }
    break;

    case SignedDecWord:
    {
        wStr = QString::number((int)((short)word));
    }
    break;

    case UnsignedDecWord:
    {
        wStr = QString::number((unsigned int)word);
    }
    break;

    default:
    {

    }
    break;
    }

    richText.text = wStr;
}

void HexDump::dwordToString(duint rva, uint32 dword, DwordViewMode_e mode, RichTextPainter::CustomRichText_t & richText)
{
    Q_UNUSED(rva);
    QString wStr;

    switch(mode)
    {
    case HexDword:
    {
        wStr = QString("%1").arg((unsigned int)dword, 8, 16, QChar('0')).toUpper();
    }
    break;

    case SignedDecDword:
    {
        wStr = QString::number((int)dword);
    }
    break;

    case UnsignedDecDword:
    {
        wStr = QString::number((unsigned int)dword);
    }
    break;

    case FloatDword:
    {
        wStr = ToFloatString(&dword);
    }
    break;

    default:
    {

    }
    break;
    }

    richText.text = wStr;
}

void HexDump::qwordToString(duint rva, uint64 qword, QwordViewMode_e mode, RichTextPainter::CustomRichText_t & richText)
{
    Q_UNUSED(rva);
    QString wStr;

    switch(mode)
    {
    case HexQword:
    {
        wStr = QString("%1").arg((unsigned long long)qword, 16, 16, QChar('0')).toUpper();
    }
    break;

    case SignedDecQword:
    {
        wStr = QString::number((long long)qword);
    }
    break;

    case UnsignedDecQword:
    {
        wStr = QString::number((unsigned long long)qword);
    }
    break;

    case DoubleQword:
    {
        wStr = ToDoubleString(&qword);
    }
    break;

    default:
    {

    }
    break;
    }

    richText.text = wStr;
}

void HexDump::twordToString(duint rva, void* tword, TwordViewMode_e mode, RichTextPainter::CustomRichText_t & richText)
{
    Q_UNUSED(rva);
    QString wStr;

    switch(mode)
    {
    case FloatTword:
    {
        wStr = ToLongDoubleString(tword);
    }
    break;

    default:
    {

    }
    break;
    }

    richText.text = wStr;
}

int HexDump::getSizeOf(DataSize_e size)
{
    return int(size);
}

int HexDump::getStringMaxLength(DataDescriptor_t desc)
{
    int wLength = 0;

    switch(desc.itemSize)
    {
    case Byte:
    {
        wLength = byteStringMaxLength(desc.byteMode);
    }
    break;

    case Word:
    {
        wLength = wordStringMaxLength(desc.wordMode);
    }
    break;

    case Dword:
    {
        wLength = dwordStringMaxLength(desc.dwordMode);
    }
    break;

    case Qword:
    {
        wLength = qwordStringMaxLength(desc.qwordMode);
    }
    break;

    case Tword:
    {
        wLength = twordStringMaxLength(desc.twordMode);
    }
    break;

    default:
    {

    }
    break;
    }

    return wLength;
}

int HexDump::byteStringMaxLength(ByteViewMode_e mode)
{
    int wLength = 0;

    switch(mode)
    {
    case HexByte:
    {
        wLength = 2;
    }
    break;

    case AsciiByte:
    {
        wLength = 0;
    }
    break;

    case SignedDecByte:
    {
        wLength = 4;
    }
    break;

    case UnsignedDecByte:
    {
        wLength = 3;
    }
    break;

    default:
    {

    }
    break;
    }

    return wLength;
}

int HexDump::wordStringMaxLength(WordViewMode_e mode)
{
    int wLength = 0;

    switch(mode)
    {
    case HexWord:
    {
        wLength = 4;
    }
    break;

    case UnicodeWord:
    {
        wLength = 0;
    }
    break;

    case SignedDecWord:
    {
        wLength = 6;
    }
    break;

    case UnsignedDecWord:
    {
        wLength = 5;
    }
    break;

    default:
    {

    }
    break;
    }

    return wLength;
}

int HexDump::dwordStringMaxLength(DwordViewMode_e mode)
{
    int wLength = 0;

    switch(mode)
    {
    case HexDword:
    {
        wLength = 8;
    }
    break;

    case SignedDecDword:
    {
        wLength = 11;
    }
    break;

    case UnsignedDecDword:
    {
        wLength = 10;
    }
    break;

    case FloatDword:
    {
        wLength = 13;
    }
    break;

    default:
    {

    }
    break;
    }

    return wLength;
}

int HexDump::qwordStringMaxLength(QwordViewMode_e mode)
{
    int wLength = 0;

    switch(mode)
    {
    case HexQword:
    {
        wLength = 16;
    }
    break;

    case SignedDecQword:
    {
        wLength = 20;
    }
    break;

    case UnsignedDecQword:
    {
        wLength = 20;
    }
    break;

    case DoubleQword:
    {
        wLength = 23;
    }
    break;

    default:
    {

    }
    break;
    }

    return wLength;
}

int HexDump::twordStringMaxLength(TwordViewMode_e mode)
{
    int wLength = 0;

    switch(mode)
    {
    case FloatTword:
    {
        wLength = 29;
    }
    break;

    default:
    {

    }
    break;
    }

    return wLength;
}

int HexDump::getItemIndexFromX(int x)
{
    int wColIndex = getColumnIndexFromX(x);

    if(wColIndex > 0)
    {
        int wColStartingPos = getColumnPosition(wColIndex);
        int wRelativeX = x - wColStartingPos;

        int wItemPixWidth = getItemPixelWidth(mDescriptor.at(wColIndex - 1));
        int wCharWidth = getCharWidth();
        if(wItemPixWidth == wCharWidth)
            wRelativeX -= 4;

        int wItemIndex = wRelativeX / wItemPixWidth;

        wItemIndex = wItemIndex < 0 ? 0 : wItemIndex;
        wItemIndex = wItemIndex > (mDescriptor.at(wColIndex - 1).itemCount - 1) ? (mDescriptor.at(wColIndex - 1).itemCount - 1) : wItemIndex;

        return wItemIndex;
    }
    else
    {
        return 0;
    }
}

dsint HexDump::getItemStartingAddress(int x, int y)
{
    int wRowOffset = getIndexOffsetFromY(transY(y));
    int wItemIndex = getItemIndexFromX(x);
    int wColIndex = getColumnIndexFromX(x);
    dsint wStartingAddress = 0;

    if(wColIndex > 0)
    {
        wColIndex -= 1;
        wStartingAddress = (getTableOffset() + wRowOffset) * (mDescriptor.at(wColIndex).itemCount * getSizeOf(mDescriptor.at(wColIndex).data.itemSize)) + wItemIndex * getSizeOf(mDescriptor.at(wColIndex).data.itemSize) - mByteOffset;
    }

    return wStartingAddress;
}

int HexDump::getBytePerRowCount()
{
    return mDescriptor.at(0).itemCount * getSizeOf(mDescriptor.at(0).data.itemSize);
}

int HexDump::getItemPixelWidth(ColumnDescriptor_t desc)
{
    int wCharWidth = getCharWidth();
    int wItemPixWidth = getStringMaxLength(desc.data) * wCharWidth + wCharWidth;

    return wItemPixWidth;
}

void HexDump::appendDescriptor(int width, QString title, bool clickable, ColumnDescriptor_t descriptor)
{
    addColumnAt(width, title, clickable);
    mDescriptor.append(descriptor);
}

//Clears the descriptors, append a new descriptor and fix the tableOffset (use this instead of clearDescriptors()
void HexDump::appendResetDescriptor(int width, QString title, bool clickable, ColumnDescriptor_t descriptor)
{
    mAllowPainting = false;
    if(mDescriptor.size())
    {
        dsint wRVA = getTableOffset() * getBytePerRowCount() - mByteOffset;
        clearDescriptors();
        appendDescriptor(width, title, clickable, descriptor);
        printDumpAt(rvaToVa(wRVA), true, false);
    }
    else
        appendDescriptor(width, title, clickable, descriptor);
    mAllowPainting = true;
}

void HexDump::clearDescriptors()
{
    deleteAllColumns();
    mDescriptor.clear();
    int charwidth = getCharWidth();
    addColumnAt(8 + charwidth * 2 * sizeof(duint), tr("Address"), false); //address
}

void HexDump::debugStateChanged(DBGSTATE state)
{
    if(state == stopped)
    {
        mMemPage->setAttributes(0, 0);
        setRowCount(0);
        reloadData();
    }
}
