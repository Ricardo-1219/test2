#include "aboutdialog.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QFont>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("关于小牛文本编辑器"));
    setModal(true);
    resize(460, 300);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 20);
    layout->setSpacing(14);

    QLabel *title = new QLabel(QStringLiteral("小牛文本编辑器"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel *version = new QLabel(
        QStringLiteral("V2.0  |  Qt %1").arg(QString::fromLatin1(qVersion())),
        this);
    version->setAlignment(Qt::AlignCenter);
    layout->addWidget(version);

    QLabel *details = new QLabel(
        QStringLiteral(
            "开发者：陈宇衡\n"
            "学号：2024414290203\n"
            "班级：软件工程2班\n\n"
            "东莞理工学院计算机科学与技术学院\n"
            "2026 年秋季 Qt 应用程序开发实验二"),
        this);
    details->setAlignment(Qt::AlignCenter);
    details->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(details, 1);

    QDialogButtonBox *box =
        new QDialogButtonBox(QDialogButtonBox::Ok, this);
    box->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(box);

    setWindowIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
}
