#ifndef REPLACEDIALOG_H
#define REPLACEDIALOG_H

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QRadioButton;

class ReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplaceDialog(QWidget *parent = nullptr);
    void setFindText(const QString &text);

signals:
    void findRequested(const QString &text, bool caseSensitive, bool backward);
    void replaceRequested(const QString &text,
                          const QString &replacement,
                          bool caseSensitive,
                          bool backward);
    void replaceAllRequested(const QString &text,
                             const QString &replacement,
                             bool caseSensitive);

private:
    QString findText() const;
    QString replacementText() const;
    bool caseSensitive() const;
    bool backward() const;

private:
    QLineEdit *m_findEdit = nullptr;
    QLineEdit *m_replaceEdit = nullptr;
    QCheckBox *m_caseCheck = nullptr;
    QRadioButton *m_upRadio = nullptr;
    QRadioButton *m_downRadio = nullptr;
};

#endif
