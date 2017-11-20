#include "SourceView.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QDesktopServices>
#include "Configuration.h"

SourceView::SourceView(QString path, int line, QWidget* parent)
    : ReferenceView(true, parent),
      mIpLine(0)
{
    mSourcePath = path;
    mList->enableColumnSorting(false);
    mSearchList->enableColumnSorting(false);

    addColumnAt(sizeof(duint) * 2, tr("Address"));
    addColumnAt(6, tr("Line"));
    addColumnAt(0, tr("Code"));

    connect(this, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(sourceContextMenu(QMenu*)));

    loadFile();
    setSelection(line);
    auto cip = DbgValFromString("cip");
    mList->disassembleAtSlot(0, cip);
    mSearchList->disassembleAtSlot(0, cip);

    mMenuBuilder = new MenuBuilder(this);
    mMenuBuilder->addAction(makeAction(tr("Open source file"), SLOT(openSourceFileSlot())));
    mMenuBuilder->addAction(makeAction(tr("Show source file in directory"), SLOT(showInDirectorySlot())));
    mMenuBuilder->loadFromConfig();
}

void SourceView::setSelection(int line)
{
    mCurList->scrollSelect(line - 1);
    reloadData(); //repaint
}

QString SourceView::getSourcePath()
{
    return mSourcePath;
}

void SourceView::loadFile()
{
    QFile file(mSourcePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        return; //error?
    }
    QTextStream in(&file);
    int lineNum = 0;
    while(!in.atEnd())
    {
        QString line = in.readLine().replace('\t', "    "); //replace tabs with four spaces
        setRowCount(lineNum + 1);
        duint displacement = 0;
        duint addr = DbgFunctions()->GetAddrFromLine(mSourcePath.toUtf8().constData(), lineNum + 1, &displacement);
        if(addr && !displacement)
            setCellContent(lineNum, 0, ToPtrString(addr));
        setCellContent(lineNum, 1, QString("%1").arg(lineNum + 1));
        setCellContent(lineNum, 2, line);
        lineNum++;
    }
    reloadData();
    file.close();
}

void SourceView::sourceContextMenu(QMenu* menu)
{
    menu->addSeparator();
    mMenuBuilder->build(menu);
}

void SourceView::openSourceFileSlot()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(mSourcePath));
}

void SourceView::showInDirectorySlot()
{
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(mSourcePath);
    auto process = new QProcess(this);
    process->start("explorer.exe", args);
}
