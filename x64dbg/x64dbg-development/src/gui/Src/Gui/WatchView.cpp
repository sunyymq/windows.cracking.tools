#include "Bridge.h"
#include "WatchView.h"
#include "CPUMultiDump.h"
#include "WordEditDialog.h"
#include "MiscUtil.h"

WatchView::WatchView(CPUMultiDump* parent) : StdTable(parent)
{
    int charWidth = getCharWidth();
    addColumnAt(8 + charWidth * 12, tr("Name"), false);
    addColumnAt(8 + charWidth * 20, tr("Expression"), false);
    addColumnAt(8 + charWidth * sizeof(duint) * 2, tr("Value"), false);
    addColumnAt(8 + charWidth * 8, tr("Type"), false);
    addColumnAt(150, tr("Watchdog Mode"), false);
    addColumnAt(30, tr("ID"), false);

    connect(Bridge::getBridge(), SIGNAL(updateWatch()), this, SLOT(updateWatch()));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));

    updateColors();
    setupContextMenu();
    setDrawDebugOnly(true);
}

void WatchView::updateWatch()
{
    if(!DbgIsDebugging())
    {
        setRowCount(0);
        return;
    }
    BridgeList<WATCHINFO> WatchList;
    DbgGetWatchList(&WatchList);
    setRowCount(WatchList.Count());
    for(int i = 0; i < WatchList.Count(); i++)
    {
        setCellContent(i, 0, QString(WatchList[i].WatchName));
        setCellContent(i, 1, QString(WatchList[i].Expression));
        switch(WatchList[i].varType)
        {
        case WATCHVARTYPE::TYPE_UINT:
            setCellContent(i, 3, "UINT");
            setCellContent(i, 2, ToPtrString(WatchList[i].value));
            break;
        case WATCHVARTYPE::TYPE_INT:
            setCellContent(i, 3, "INT");
            setCellContent(i, 2, QString::number((dsint)WatchList[i].value));
            break;
        case WATCHVARTYPE::TYPE_FLOAT:
            setCellContent(i, 3, "FLOAT");
            setCellContent(i, 2, QString::number(*(float*)&WatchList[i].value));
            break;
        case WATCHVARTYPE::TYPE_ASCII:
            setCellContent(i, 3, "ASCII");
            {
                char buffer[128];
                // zero the buffer
                memset(buffer, 0, sizeof(buffer));
                if(DbgMemRead(WatchList[i].value, (unsigned char*)buffer, sizeof(buffer) - 1))
                {
                    // convert the ASCII string to QString
                    QString text = QString::fromLocal8Bit(buffer);
                    if(strlen(buffer) == sizeof(buffer) - 1)
                        text.append("...");
                    // remove CRLF
                    text.replace(QChar('\x13'), "\\r");
                    text.replace(QChar('\x10'), "\\n");
                    setCellContent(i, 2, text);
                }
                else
                    setCellContent(i, 2, tr("%1 is not readable.").arg(ToPtrString(WatchList[i].value)));
            }
            break;
        case WATCHVARTYPE::TYPE_UNICODE:
            setCellContent(i, 3, "UNICODE");
            {
                unsigned short buffer[128];
                // zero the buffer
                memset(buffer, 0, sizeof(buffer));
                if(DbgMemRead(WatchList[i].value, (unsigned char*)buffer, sizeof(buffer) - sizeof(unsigned short)))
                {
                    QString text = QString::fromUtf16(buffer);
                    size_t size = text.size();
                    // Check if the last character is an incomplete UTF-16 surrogate.
                    if(text.at(text.size() - 1).isHighSurrogate())
                        text.chop(text.size() - 1); // Delete the incomplete surrogate.
                    // Check if something is truncated.
                    if(size == sizeof(buffer) / sizeof(unsigned short) - 1)
                        text.append("...");
                    // remove CRLF
                    text.replace(QChar('\x13'), "\\r");
                    text.replace(QChar('\x10'), "\\n");
                    setCellContent(i, 2, text);
                }
                else
                    setCellContent(i, 2, tr("%1 is not readable.").arg(ToPtrString(WatchList[i].value)));
            }
            break;
        case WATCHVARTYPE::TYPE_INVALID:
        default:
            setCellContent(i, 3, "INVALID");
            setCellContent(i, 2, "");
            break;
        }
        switch(WatchList[i].watchdogMode)
        {
        case WATCHDOGMODE::MODE_DISABLED:
        default:
            setCellContent(i, 4, tr("Disabled"));
            break;
        case WATCHDOGMODE::MODE_CHANGED:
            setCellContent(i, 4, tr("Changed"));
            break;
        case WATCHDOGMODE::MODE_ISTRUE:
            setCellContent(i, 4, tr("Is true"));
            break;
        case WATCHDOGMODE::MODE_ISFALSE:
            setCellContent(i, 4, tr("Is false"));
            break;
        case WATCHDOGMODE::MODE_UNCHANGED:
            setCellContent(i, 4, tr("Not changed"));
            break;
        }
        setCellContent(i, 5, QString::number(WatchList[i].id));
    }
    reloadData();
}

void WatchView::updateColors()
{
    mWatchTriggeredColor = QPen(ConfigColor("WatchTriggeredColor"));
    mWatchTriggeredBackgroundColor = QBrush(ConfigColor("WatchTriggeredBackgroundColor"));
    StdTable::updateColors();
}

void WatchView::setupContextMenu()
{
    mMenu = new MenuBuilder(this, [](QMenu*)
    {
        return DbgIsDebugging();
    });
    const auto & nonEmptyFunc = [this](QMenu*)
    {
        return getRowCount() != 0;
    };
    mMenu->addAction(makeAction(tr("&Add..."), SLOT(addWatchSlot())));
    mMenu->addAction(makeAction(tr("&Delete"), SLOT(delWatchSlot())), nonEmptyFunc);
    mMenu->addAction(makeAction(DIcon("labels.png"), tr("Rename"), SLOT(renameWatchSlot())), nonEmptyFunc);
    mMenu->addAction(makeAction(DIcon("modify.png"), tr("&Edit..."), SLOT(editWatchSlot())), nonEmptyFunc);
    mMenu->addAction(makeAction(DIcon("modify.png"), tr("&Modify..."), SLOT(modifyWatchSlot())), nonEmptyFunc);
    MenuBuilder* watchdogBuilder = new MenuBuilder(this, nonEmptyFunc);
    QMenu* watchdogMenu = new QMenu(tr("Watchdog"), this);
    watchdogMenu->setIcon(DIcon("animal-dog.png"));
    watchdogBuilder->addAction(makeAction(DIcon("disable.png"), tr("Disabled"), SLOT(watchdogDisableSlot())));
    watchdogBuilder->addSeparator();
    watchdogBuilder->addAction(makeAction(DIcon("arrow-restart.png"), tr("Changed"), SLOT(watchdogChangedSlot())));
    watchdogBuilder->addAction(makeAction(DIcon("control-pause.png"), tr("Not changed"), SLOT(watchdogUnchangedSlot())));
    watchdogBuilder->addAction(makeAction(DIcon("treat_selection_as_tbyte.png"), tr("Is true"), SLOT(watchdogIsTrueSlot()))); // TODO: better icon
    watchdogBuilder->addAction(makeAction(DIcon("treat_selection_as_fword.png"), tr("Is false"), SLOT(watchdogIsFalseSlot())));
    mMenu->addMenu(watchdogMenu, watchdogBuilder);
    mMenu->addSeparator();
    MenuBuilder* copyMenu = new MenuBuilder(this);
    setupCopyMenu(copyMenu);
    mMenu->addMenu(makeMenu(DIcon("copy.png"), tr("&Copy")), copyMenu);
    mMenu->loadFromConfig();
}

QString WatchView::getSelectedId()
{
    return getCellContent(getInitialSelection(), 5);
}

QString WatchView::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    QString ret = StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
    const dsint row = rowBase + rowOffset;
    if(row != getInitialSelection() && DbgFunctions()->WatchIsWatchdogTriggered(getCellContent(row, 5).toUInt()))
    {
        painter->fillRect(QRect(x, y, w, h), mWatchTriggeredBackgroundColor);
        painter->setPen(mWatchTriggeredColor); //white text
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, ret);
        return QString();
    }
    else
        return ret;
}

//SLOTS

void WatchView::contextMenuSlot(const QPoint & pos)
{
    QMenu wMenu(this);
    mMenu->build(&wMenu);
    wMenu.exec(mapToGlobal(pos));
}

void WatchView::addWatchSlot()
{
    QString name;
    if(SimpleInputBox(this, tr("Enter the expression to watch"), "", name, tr("Example: [EAX]")))
        DbgCmdExecDirect(QString("AddWatch ").append(name).toUtf8().constData());
    updateWatch();
}

void WatchView::delWatchSlot()
{
    DbgCmdExecDirect(QString("DelWatch ").append(getSelectedId()).toUtf8().constData());
    updateWatch();
}

void WatchView::renameWatchSlot()
{
    QString name;
    QString originalName = getCellContent(getInitialSelection(), 0);
    if(SimpleInputBox(this, tr("Enter the name of the watch variable"), originalName, name, originalName))
        DbgCmdExecDirect(QString("SetWatchName ").append(getSelectedId() + "," + name).toUtf8().constData());
    updateWatch();
}

void WatchView::modifyWatchSlot()
{
    BridgeList<WATCHINFO> WatchList;
    DbgGetWatchList(&WatchList);
    auto sel = getInitialSelection();
    if(sel > WatchList.Count())
        return;
    WordEditDialog modifyDialog(this);
    modifyDialog.setup(tr("Modify \"%1\"").arg(QString(WatchList[sel].WatchName)), WatchList[sel].value, sizeof(duint));
    if(modifyDialog.exec() == QDialog::Accepted)
    {
        if(!DbgValToString(WatchList[sel].Expression, modifyDialog.getVal()))
            SimpleErrorBox(this, tr("Cannot modify \"%1\"").arg(QString(WatchList[sel].WatchName)), tr("It might not possible to assign a value to \"%1\".").arg(QString(WatchList[sel].Expression)));
    }
    updateWatch();
}

void WatchView::editWatchSlot()
{
    QString expr;
    if(SimpleInputBox(this, tr("Enter the expression to watch"), "", expr, tr("Example: [EAX]")))
        DbgCmdExecDirect(QString("SetWatchExpression ").append(getSelectedId()).append(",").append(expr).toUtf8().constData());
    updateWatch();
}

void WatchView::watchdogDisableSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"disabled\"").arg(getSelectedId()).toUtf8().constData());
    updateWatch();
}

void WatchView::watchdogChangedSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"changed\"").arg(getSelectedId()).toUtf8().constData());
    updateWatch();
}

void WatchView::watchdogUnchangedSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"unchanged\"").arg(getSelectedId()).toUtf8().constData());
    updateWatch();
}

void WatchView::watchdogIsTrueSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"istrue\"").arg(getSelectedId()).toUtf8().constData());
    updateWatch();
}

void WatchView::watchdogIsFalseSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"isfalse\"").arg(getSelectedId()).toUtf8().constData());
    updateWatch();
}
