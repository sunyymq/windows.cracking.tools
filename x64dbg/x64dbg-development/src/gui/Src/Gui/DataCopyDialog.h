#ifndef DATACOPYDIALOG_H
#define DATACOPYDIALOG_H

#include <QDialog>
#include <QVector>
#include "Imports.h"

namespace Ui
{
    class DataCopyDialog;
}

class DataCopyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DataCopyDialog(const QVector<byte_t>* data, QWidget* parent = 0);
    ~DataCopyDialog();

private slots:
    void on_comboType_currentIndexChanged(int index);
    void on_buttonCopy_clicked();
    void on_spinBox_valueChanged(int arg1);

private:
    Ui::DataCopyDialog* ui;
    const QVector<byte_t>* mData;
    int mIndex;

    enum DataType
    {
        DataCByte = 0,
        DataCWord,
        DataCDword,
        DataCQword,
        DataCString,
        DataCUnicodeString,
        DataCShellcodeString,
        DataPascalByte,
        DataPascalWord,
        DataPascalDword,
        DataPascalQword,
        DataGUID,
        DataIPv4,
        DataIPv6,
        DataBase64,
        DataLast
    };

    struct FormatType
    {
        QString name;
        int itemsPerLine;
    };

    FormatType mTypes[DataLast];

    void printData(DataType type);
    QString printEscapedString(bool & bPrevWasHex, int ch, const char* hexFormat);
};

#endif // DATACOPYDIALOG_H
