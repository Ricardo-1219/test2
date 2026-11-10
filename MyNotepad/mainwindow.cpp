#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdialog.h"
#include "codeeditor.h"
#include "finddialog.h"
#include "replacedialog.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDateTime>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPalette>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QToolBar>

#include <algorithm>

namespace {
constexpr int AutoSaveIntervalMilliseconds = 60 * 1000;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_editor = ui->textEdit;

    configureActions();
    createStatusBarWidgets();

    ui->actionWordWrap->setChecked(true);
    ui->actionLineNumbers->setChecked(true);
    m_editor->setLineNumbersVisible(true);
    m_editor->setLineWrapMode(QPlainTextEdit::WidgetWidth);

    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(AutoSaveIntervalMilliseconds);
    connect(m_autoSaveTimer, &QTimer::timeout,
            this, &MainWindow::autoSave);
    m_autoSaveTimer->start();

    setCurrentFile(QString());
    setModifiedState(false);
    updateStatusBar();
    updateActionStates();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::configureActions()
{
    ui->actionNew->setIcon(QIcon(QStringLiteral(":/icons/new.svg")));
    ui->actionOpen->setIcon(QIcon(QStringLiteral(":/icons/open.svg")));
    ui->actionSave->setIcon(QIcon(QStringLiteral(":/icons/save.svg")));
    ui->actionSaveAs->setIcon(QIcon(QStringLiteral(":/icons/saveas.svg")));
    ui->actionUndo->setIcon(QIcon(QStringLiteral(":/icons/undo.svg")));
    ui->actionRedo->setIcon(QIcon(QStringLiteral(":/icons/redo.svg")));
    ui->actionCut->setIcon(QIcon(QStringLiteral(":/icons/cut.svg")));
    ui->actionCopy->setIcon(QIcon(QStringLiteral(":/icons/copy.svg")));
    ui->actionPaste->setIcon(QIcon(QStringLiteral(":/icons/paste.svg")));
    ui->actionFind->setIcon(QIcon(QStringLiteral(":/icons/find.svg")));
    ui->actionReplace->setIcon(QIcon(QStringLiteral(":/icons/replace.svg")));
    ui->actionWordWrap->setIcon(QIcon(QStringLiteral(":/icons/wrap.svg")));
    ui->actionLineNumbers->setIcon(QIcon(QStringLiteral(":/icons/line-number.svg")));
    ui->actionFont->setIcon(QIcon(QStringLiteral(":/icons/font.svg")));
    ui->actionAbout->setIcon(QIcon(QStringLiteral(":/icons/about.svg")));
    ui->toolBar->setIconSize(QSize(22, 22));

    connect(ui->actionNew, &QAction::triggered,
            this, &MainWindow::newFile);
    connect(ui->actionOpen, &QAction::triggered, this, [this]() {
        const QString filePath = QFileDialog::getOpenFileName(
            this,
            tr("打开文件"),
            QString(),
            tr("文本文件 (*.txt *.md *.cpp *.h *.ui *.pro);;所有文件 (*.*)"));
        if (!filePath.isEmpty()) {
            openFile(filePath);
        }
    });
    connect(ui->actionSave, &QAction::triggered, this, [this]() {
        saveFile();
    });
    connect(ui->actionSaveAs, &QAction::triggered, this, [this]() {
        saveFileAs();
    });
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    connect(ui->actionUndo, &QAction::triggered,
            m_editor, &QPlainTextEdit::undo);
    connect(ui->actionRedo, &QAction::triggered,
            m_editor, &QPlainTextEdit::redo);
    connect(ui->actionCut, &QAction::triggered,
            m_editor, &QPlainTextEdit::cut);
    connect(ui->actionCopy, &QAction::triggered,
            m_editor, &QPlainTextEdit::copy);
    connect(ui->actionPaste, &QAction::triggered,
            m_editor, &QPlainTextEdit::paste);
    connect(ui->actionSelectAll, &QAction::triggered,
            m_editor, &QPlainTextEdit::selectAll);

    connect(ui->actionFind, &QAction::triggered,
            this, &MainWindow::showFindDialog);
    connect(ui->actionReplace, &QAction::triggered,
            this, &MainWindow::showReplaceDialog);
    connect(ui->actionFont, &QAction::triggered,
            this, &MainWindow::chooseFont);
    connect(ui->actionTextColor, &QAction::triggered,
            this, &MainWindow::chooseTextColor);
    connect(ui->actionTextBackground, &QAction::triggered,
            this, &MainWindow::chooseTextBackgroundColor);
    connect(ui->actionEditorBackground, &QAction::triggered,
            this, &MainWindow::chooseEditorBackgroundColor);
    connect(ui->actionWordWrap, &QAction::toggled,
            this, &MainWindow::setWordWrapEnabled);
    connect(ui->actionLineNumbers, &QAction::toggled,
            this, &MainWindow::setLineNumbersEnabled);
    connect(ui->actionToolBar, &QAction::toggled,
            ui->toolBar, &QWidget::setVisible);
    connect(ui->actionStatusBar, &QAction::toggled,
            ui->statusbar, &QWidget::setVisible);
    connect(ui->actionAbout, &QAction::triggered,
            this, &MainWindow::showAboutDialog);

    connect(m_editor, &QPlainTextEdit::textChanged,
            this, &MainWindow::markDocumentModified);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateStatusBar);
    connect(m_editor, &QPlainTextEdit::selectionChanged,
            this, &MainWindow::updateActionStates);
    connect(m_editor, &QPlainTextEdit::undoAvailable,
            ui->actionUndo, &QAction::setEnabled);
    connect(m_editor, &QPlainTextEdit::redoAvailable,
            ui->actionRedo, &QAction::setEnabled);
    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &MainWindow::updateActionStates);
}

void MainWindow::createStatusBarWidgets()
{
    m_lengthLabel = new QLabel(this);
    m_lineCountLabel = new QLabel(this);
    m_cursorLabel = new QLabel(this);
    m_authorLabel = new QLabel(QStringLiteral("作者：陈宇衡"), this);

    m_lengthLabel->setMinimumWidth(130);
    m_lineCountLabel->setMinimumWidth(100);
    m_cursorLabel->setMinimumWidth(145);
    m_authorLabel->setMinimumWidth(120);

    ui->statusbar->addPermanentWidget(m_lengthLabel);
    ui->statusbar->addPermanentWidget(m_lineCountLabel);
    ui->statusbar->addPermanentWidget(m_cursorLabel);
    ui->statusbar->addPermanentWidget(m_authorLabel);
    ui->statusbar->showMessage(QStringLiteral("就绪"), 3000);
}

void MainWindow::newFile()
{
    if (!maybeSave()) {
        return;
    }

    const QSignalBlocker blocker(m_editor);
    m_editor->clear();
    setCurrentFile(QString());
    setModifiedState(false);
    updateStatusBar();
    m_editor->setFocus();
}

bool MainWindow::openFile(const QString &filePath, bool showError)
{
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        if (showError) {
            QMessageBox::warning(
                this,
                tr("打开失败"),
                tr("文件不存在或不是一个普通文件：\n%1").arg(filePath));
        }
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (showError) {
            QMessageBox::warning(
                this,
                tr("打开失败"),
                tr("无法读取文件：\n%1\n\n%2").arg(filePath, file.errorString()));
        }
        return false;
    }

    const QByteArray data = file.readAll();
    file.close();
    if (data.contains('\0')) {
        if (showError) {
            QMessageBox::warning(
                this,
                tr("打开失败"),
                tr("该文件包含二进制数据，无法作为文本打开。"));
        }
        return false;
    }

    if (!maybeSave()) {
        return false;
    }

    {
        const QSignalBlocker blocker(m_editor);
        m_editor->setPlainText(QString::fromUtf8(data));
    }

    setCurrentFile(filePath);
    setModifiedState(false);
    m_editor->moveCursor(QTextCursor::Start);
    updateStatusBar();
    ui->statusbar->showMessage(
        tr("已打开：%1").arg(QFileInfo(filePath).fileName()), 4000);
    return true;
}

bool MainWindow::saveFile()
{
    if (m_currentFilePath.isEmpty()) {
        return saveFileAs();
    }
    return saveToFile(m_currentFilePath, true);
}

bool MainWindow::saveFileAs()
{
    QString suggestedName = m_currentFilePath;
    if (suggestedName.isEmpty()) {
        suggestedName = QStringLiteral("未命名.txt");
    }

    const QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("另存为"),
        suggestedName,
        tr("文本文件 (*.txt);;所有文件 (*.*)"));
    if (filePath.isEmpty()) {
        return false;
    }

    return saveToFile(filePath, true);
}

bool MainWindow::saveToFile(const QString &filePath, bool showError)
{
    if (filePath.isEmpty()) {
        return false;
    }

    const QByteArray data = m_editor->toPlainText().toUtf8();
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (showError) {
            QMessageBox::warning(
                this,
                tr("保存失败"),
                tr("无法写入文件：\n%1\n\n%2").arg(filePath, file.errorString()));
        }
        return false;
    }

    const qint64 written = file.write(data);
    if (written != data.size() || !file.commit()) {
        if (showError) {
            QMessageBox::warning(
                this,
                tr("保存失败"),
                tr("文件写入不完整，请检查磁盘空间或目录权限。"));
        }
        return false;
    }

    setCurrentFile(filePath);
    setModifiedState(false);
    ui->statusbar->showMessage(
        tr("已保存：%1").arg(QFileInfo(filePath).fileName()), 4000);
    return true;
}

bool MainWindow::maybeSave()
{
    if (!m_modified) {
        return true;
    }

    const QMessageBox::StandardButton result = QMessageBox::warning(
        this,
        tr("文件已修改"),
        tr("“%1”有未保存的修改。是否保存？").arg(displayFileName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);

    if (result == QMessageBox::Save) {
        return saveFile();
    }
    if (result == QMessageBox::Discard) {
        return true;
    }
    return false;
}

void MainWindow::showFindDialog()
{
    if (!m_findDialog) {
        m_findDialog = new FindDialog(this);
        connect(m_findDialog, &FindDialog::findRequested,
                this, &MainWindow::findRequested);
    }

    QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection() && cursor.selectedText().size() <= 100) {
        m_findDialog->setFindText(cursor.selectedText());
    }
    m_findDialog->show();
    m_findDialog->raise();
    m_findDialog->activateWindow();
}

void MainWindow::showReplaceDialog()
{
    if (!m_replaceDialog) {
        m_replaceDialog = new ReplaceDialog(this);
        connect(m_replaceDialog, &ReplaceDialog::findRequested,
                this, &MainWindow::findRequested);
        connect(m_replaceDialog, &ReplaceDialog::replaceRequested,
                this, &MainWindow::replaceRequested);
        connect(m_replaceDialog, &ReplaceDialog::replaceAllRequested,
                this, &MainWindow::replaceAllRequested);
    }

    QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection() && cursor.selectedText().size() <= 100) {
        m_replaceDialog->setFindText(cursor.selectedText());
    }
    m_replaceDialog->show();
    m_replaceDialog->raise();
    m_replaceDialog->activateWindow();
}

void MainWindow::showAboutDialog()
{
    if (!m_aboutDialog) {
        m_aboutDialog = new AboutDialog(this);
    }
    m_aboutDialog->show();
    m_aboutDialog->raise();
    m_aboutDialog->activateWindow();
}

void MainWindow::chooseFont()
{
    bool accepted = false;
    const QFont font = QFontDialog::getFont(
        &accepted, m_editor->font(), this, tr("选择字体"));
    if (accepted) {
        m_editor->setFont(font);
        ui->statusbar->showMessage(
            tr("字体：%1，%2 pt").arg(font.family()).arg(font.pointSize()), 4000);
    }
}

void MainWindow::chooseTextColor()
{
    const QColor color = QColorDialog::getColor(
        m_editor->palette().color(QPalette::Text),
        this,
        tr("选择字体颜色"));
    if (!color.isValid()) {
        return;
    }

    QTextCharFormat format;
    format.setForeground(color);
    m_editor->mergeCurrentCharFormat(format);
}

void MainWindow::chooseTextBackgroundColor()
{
    const QColor color = QColorDialog::getColor(
        Qt::yellow, this, tr("选择字体背景色"));
    if (!color.isValid()) {
        return;
    }

    QTextCharFormat format;
    format.setBackground(color);
    m_editor->mergeCurrentCharFormat(format);
}

void MainWindow::chooseEditorBackgroundColor()
{
    const QColor color = QColorDialog::getColor(
        m_editor->viewport()->palette().color(QPalette::Base),
        this,
        tr("选择编辑器背景色"));
    if (!color.isValid()) {
        return;
    }

    QPalette palette = m_editor->palette();
    palette.setColor(QPalette::Base, color);
    palette.setColor(QPalette::Window, color);
    m_editor->setPalette(palette);
    m_editor->viewport()->setAutoFillBackground(true);
}

void MainWindow::setWordWrapEnabled(bool enabled)
{
    m_editor->setLineWrapMode(
        enabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

void MainWindow::setLineNumbersEnabled(bool enabled)
{
    m_editor->setLineNumbersVisible(enabled);
}

