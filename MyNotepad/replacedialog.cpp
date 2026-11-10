#include "replacedialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

ReplaceDialog::ReplaceDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("替换"));
    setModal(false);
    resize(560, 250);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QGridLayout *formLayout = new QGridLayout;
    formLayout->addWidget(new QLabel(QStringLiteral("查找目标："), this), 0, 0);
    m_findEdit = new QLineEdit(this);
    m_findEdit->setClearButtonEnabled(true);
    formLayout->addWidget(m_findEdit, 0, 1);

    formLayout->addWidget(new QLabel(QStringLiteral("替换为："), this), 1, 0);
    m_replaceEdit = new QLineEdit(this);
    m_replaceEdit->setClearButtonEnabled(true);
    formLayout->addWidget(m_replaceEdit, 1, 1);

    m_caseCheck = new QCheckBox(QStringLiteral("区分大小写"), this);
    formLayout->addWidget(m_caseCheck, 2, 0, 1, 2);

    QGroupBox *directionBox = new QGroupBox(QStringLiteral("方向"), this);
    QHBoxLayout *directionLayout = new QHBoxLayout(directionBox);
    m_upRadio = new QRadioButton(QStringLiteral("向上"), directionBox);
    m_downRadio = new QRadioButton(QStringLiteral("向下"), directionBox);
    m_downRadio->setChecked(true);
    directionLayout->addWidget(m_upRadio);
    directionLayout->addWidget(m_downRadio);
    formLayout->addWidget(directionBox, 3, 0, 1, 2);

    QPushButton *findButton =
        new QPushButton(QStringLiteral("查找下一个"), this);
    QPushButton *replaceButton =
        new QPushButton(QStringLiteral("替换"), this);
    QPushButton *replaceAllButton =
        new QPushButton(QStringLiteral("全部替换"), this);
    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Cancel)
        ->setText(QStringLiteral("取消"));

    formLayout->addWidget(findButton, 0, 2);
    formLayout->addWidget(replaceButton, 1, 2);
    formLayout->addWidget(replaceAllButton, 2, 2);
    formLayout->addWidget(buttonBox, 3, 2);
    formLayout->setColumnStretch(1, 1);
    mainLayout->addLayout(formLayout);

    connect(findButton, &QPushButton::clicked, this, [this]() {
        if (!findText().isEmpty()) {
            emit findRequested(findText(), caseSensitive(), backward());
        }
    });
    connect(replaceButton, &QPushButton::clicked, this, [this]() {
        if (!findText().isEmpty()) {
            emit replaceRequested(findText(),
                                  replacementText(),
                                  caseSensitive(),
                                  backward());
        }
    });
    connect(replaceAllButton, &QPushButton::clicked, this, [this]() {
        if (!findText().isEmpty()) {
            emit replaceAllRequested(findText(),
                                     replacementText(),
                                     caseSensitive());
        }
    });
    connect(m_findEdit, &QLineEdit::returnPressed, this, [this]() {
        if (!findText().isEmpty()) {
            emit findRequested(findText(), caseSensitive(), backward());
        }
    });
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::close);
}

void ReplaceDialog::setFindText(const QString &text)
{
    m_findEdit->setText(text);
    m_findEdit->selectAll();
    m_findEdit->setFocus();
}

QString ReplaceDialog::findText() const
{
    return m_findEdit->text();
}

QString ReplaceDialog::replacementText() const
{
    return m_replaceEdit->text();
}

bool ReplaceDialog::caseSensitive() const
{
    return m_caseCheck->isChecked();
}

bool ReplaceDialog::backward() const
{
    return m_upRadio->isChecked();
}
