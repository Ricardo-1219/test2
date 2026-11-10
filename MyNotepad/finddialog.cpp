#include "finddialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

FindDialog::FindDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("查找"));
    setModal(false);
    resize(460, 190);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QGridLayout *formLayout = new QGridLayout;
    formLayout->addWidget(new QLabel(QStringLiteral("查找目标："), this), 0, 0);

    m_findEdit = new QLineEdit(this);
    m_findEdit->setClearButtonEnabled(true);
    formLayout->addWidget(m_findEdit, 0, 1, 1, 2);

    m_caseCheck = new QCheckBox(QStringLiteral("区分大小写"), this);
    formLayout->addWidget(m_caseCheck, 1, 0, 1, 2);

    QGroupBox *directionBox = new QGroupBox(QStringLiteral("方向"), this);
    QHBoxLayout *directionLayout = new QHBoxLayout(directionBox);
    m_upRadio = new QRadioButton(QStringLiteral("向上"), directionBox);
    m_downRadio = new QRadioButton(QStringLiteral("向下"), directionBox);
    m_downRadio->setChecked(true);
    directionLayout->addWidget(m_upRadio);
    directionLayout->addWidget(m_downRadio);
    formLayout->addWidget(directionBox, 2, 1);

    QPushButton *findButton =
        new QPushButton(QStringLiteral("查找下一个"), this);
    findButton->setDefault(true);
    formLayout->addWidget(findButton, 0, 3, 1, 2);

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Cancel)
        ->setText(QStringLiteral("取消"));
    formLayout->addWidget(buttonBox, 1, 3, 2, 2);

    formLayout->setColumnStretch(1, 1);
    mainLayout->addLayout(formLayout);

    connect(findButton, &QPushButton::clicked,
            this, &FindDialog::emitFindRequest);
    connect(m_findEdit, &QLineEdit::returnPressed,
            this, &FindDialog::emitFindRequest);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::close);
}

void FindDialog::setFindText(const QString &text)
{
    m_findEdit->setText(text);
    m_findEdit->selectAll();
    m_findEdit->setFocus();
}

void FindDialog::emitFindRequest()
{
    if (m_findEdit->text().isEmpty()) {
        m_findEdit->setFocus();
        return;
    }
    emit findRequested(m_findEdit->text(),
                       m_caseCheck->isChecked(),
                       m_upRadio->isChecked());
}
