#ifndef WORDEDITDIALOG_H
#define WORDEDITDIALOG_H

#include <QDialog>
#include <QPushButton>
#include "Imports.h"

class ValidateExpressionThread;

namespace Ui
{
    class WordEditDialog;
}

class WordEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WordEditDialog(QWidget* parent = 0);
    ~WordEditDialog();
    void validateExpression(QString expression);
    void setup(QString title, duint defVal, int byteCount);
    duint getVal();
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);

protected:
    void saveCursorPositions();
    void restoreCursorPositions();

private slots:
    void expressionChanged(bool validExpression, bool validPointer, dsint value);
    void on_signedLineEdit_textEdited(const QString & arg1);
    void on_unsignedLineEdit_textEdited(const QString & arg1);

private:
    Ui::WordEditDialog* ui;
    duint mWord;
    ValidateExpressionThread* mValidateThread;

    int mHexLineEditPos;
    int mSignedEditPos;
    int mUnsignedEditPos;
};

#endif // WORDEDITDIALOG_H
