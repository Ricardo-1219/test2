#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QRadioButton;

class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(QWidget *parent = nullptr);
    void setFindText(const QString &text);

signals:
    void findRequested(const QString &text, bool caseSensitive, bool backward);

private slots:
    void emitFindRequest();

private:
    QLineEdit *m_findEdit = nullptr;
    QCheckBox *m_caseCheck = nullptr;
    QRadioButton *m_upRadio = nullptr;
    QRadioButton *m_downRadio = nullptr;
};

#endif
