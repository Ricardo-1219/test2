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

void MainWindow::findRequested(const QString &text,
                               bool caseSensitive,
                               bool backward)
{
    findNext(text, caseSensitive, backward, true);
}

bool MainWindow::findNext(const QString &text,
                          bool caseSensitive,
                          bool backward,
                          bool showNotFound)
{
    if (text.isEmpty()) {
        return false;
    }

    QTextDocument::FindFlags flags;
    if (caseSensitive) {
        flags |= QTextDocument::FindCaseSensitively;
    }
    if (backward) {
        flags |= QTextDocument::FindBackward;
    }

    QTextCursor start = m_editor->textCursor();
    if (start.hasSelection()) {
        start.setPosition(
            backward ? start.selectionStart() : start.selectionEnd());
    }

    QTextCursor found = m_editor->document()->find(text, start, flags);
    if (found.isNull()) {
        QTextCursor edge(m_editor->document());
        edge.movePosition(backward ? QTextCursor::End
                                   : QTextCursor::Start);
        found = m_editor->document()->find(text, edge, flags);
    }

    if (found.isNull()) {
        if (showNotFound) {
            QMessageBox::information(
                this,
                tr("查找"),
                tr("未找到“%1”。").arg(text));
        }
        return false;
    }

    m_editor->setTextCursor(found);
    m_editor->ensureCursorVisible();
    return true;
}

void MainWindow::replaceRequested(const QString &text,
                                  const QString &replacement,
                                  bool caseSensitive,
                                  bool backward)
{
    replaceCurrent(text, replacement, caseSensitive, backward);
}

bool MainWindow::replaceCurrent(const QString &text,
                                const QString &replacement,
                                bool caseSensitive,
                                bool backward)
{
    const QTextCursor current = m_editor->textCursor();
    const Qt::CaseSensitivity sensitivity =
        caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    const bool currentMatches =
        current.hasSelection()
        && current.selectedText().compare(text, sensitivity) == 0;

    if (!currentMatches
        && !findNext(text, caseSensitive, backward, true)) {
        return false;
    }

    m_editor->textCursor().insertText(replacement);
    return true;
}

int MainWindow::replaceAll(const QString &text,
                           const QString &replacement,
                           bool caseSensitive)
{
    if (text.isEmpty()) {
        return 0;
    }

    QTextDocument::FindFlags flags;
    if (caseSensitive) {
        flags |= QTextDocument::FindCaseSensitively;
    }

    int count = 0;
    QTextCursor cursor(m_editor->document());
    cursor.movePosition(QTextCursor::Start);
    cursor.beginEditBlock();
    while (true) {
        QTextCursor found = m_editor->document()->find(text, cursor, flags);
        if (found.isNull()) {
            break;
        }
        found.insertText(replacement);
        cursor = found;
        ++count;
    }
    cursor.endEditBlock();

    if (count > 0) {
        m_editor->setTextCursor(cursor);
        m_editor->ensureCursorVisible();
    }
    return count;
}

void MainWindow::replaceAllRequested(const QString &text,
                                     const QString &replacement,
                                     bool caseSensitive)
{
    const int count = replaceAll(text, replacement, caseSensitive);
    if (count == 0) {
        QMessageBox::information(
            this,
            tr("替换"),
            tr("未找到“%1”，没有执行替换。").arg(text));
    } else {
        QMessageBox::information(
            this,
            tr("替换"),
            tr("已替换 %1 处。").arg(count));
    }
}

void MainWindow::markDocumentModified()
{
    if (!m_modified) {
        setModifiedState(true);
    }
    updateStatusBar();
}

void MainWindow::autoSave()
{
    if (m_currentFilePath.isEmpty() || !m_modified
        || m_editor->isReadOnly()) {
        return;
    }

    if (saveToFile(m_currentFilePath, false)) {
        ui->statusbar->showMessage(
            tr("已自动保存：%1").arg(QFileInfo(m_currentFilePath).fileName()),
            3000);
    } else {
        ui->statusbar->showMessage(tr("自动保存失败，文件仍未保存。"), 5000);
    }
}

void MainWindow::updateStatusBar()
{
    if (!m_lengthLabel || !m_editor) {
        return;
    }

    const QString text = m_editor->toPlainText();
    const QTextCursor cursor = m_editor->textCursor();
    m_lengthLabel->setText(tr("长度: %1").arg(text.size()));
    m_lineCountLabel->setText(tr("行数: %1").arg(m_editor->blockCount()));
    m_cursorLabel->setText(
        tr("行: %1  列: %2")
            .arg(cursor.blockNumber() + 1)
            .arg(cursor.positionInBlock() + 1));
}

void MainWindow::updateActionStates()
{
    const bool hasSelection = m_editor->textCursor().hasSelection();
    ui->actionCut->setEnabled(hasSelection);
    ui->actionCopy->setEnabled(hasSelection);
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    ui->actionPaste->setEnabled(mimeData && mimeData->hasText());
    ui->actionUndo->setEnabled(m_editor->document()->isUndoAvailable());
    ui->actionRedo->setEnabled(m_editor->document()->isRedoAvailable());
    ui->actionSave->setEnabled(
        m_modified || (!m_editor->toPlainText().isEmpty()
                       && m_currentFilePath.isEmpty()));
}

void MainWindow::updateWindowTitle()
{
    setWindowTitle(tr("%1[*] - 小牛文本编辑器").arg(displayFileName()));
    setWindowModified(m_modified);
}

void MainWindow::setCurrentFile(const QString &filePath)
{
    m_currentFilePath = filePath;
    updateWindowTitle();
}

void MainWindow::setModifiedState(bool modified)
{
    m_modified = modified;
    m_editor->document()->setModified(modified);
    setWindowModified(modified);
    updateWindowTitle();
    updateActionStates();
}

QString MainWindow::displayFileName() const
{
    return m_currentFilePath.isEmpty()
        ? tr("未命名")
        : QFileInfo(m_currentFilePath).fileName();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::addTestResult(QStringList *results,
                               const QString &name,
                               bool passed,
                               const QString &details)
{
    results->append(QStringLiteral("%1\t%2\t%3")
                        .arg(passed ? QStringLiteral("PASS")
                                    : QStringLiteral("FAIL"),
                             name,
                             details));
}

void MainWindow::captureWidget(QWidget *widget,
                               const QString &filePath,
                               const QString &title,
                               QStringList *results)
{
    widget->show();
    widget->raise();
    QApplication::processEvents(QEventLoop::AllEvents, 150);
    const QPixmap image = widget->grab();
    const bool saved = image.save(filePath, "PNG");
    addTestResult(results,
                  title,
                  saved && !image.isNull(),
                  filePath);
}

bool MainWindow::runSelfTest(const QString &outputDirectory)
{
    QStringList results;
    const QDir output(outputDirectory);

    newFile();
    const QString sample =
        QStringLiteral(
            "小牛文本编辑器功能测试\n"
            "Qt application development\n"
            "查找目标：alpha   替换目标：beta\n"
            "alpha beta alpha\n"
            "第三行：中文、English、12345。\n");
    {
        const QSignalBlocker blocker(m_editor);
        m_editor->setPlainText(sample);
    }
    m_editor->moveCursor(QTextCursor::Start);
    m_editor->setFocus();
    setModifiedState(true);
    updateStatusBar();

    const QString validPath = output.filePath(QStringLiteral("自动测试文本.txt"));
    const bool saved = saveToFile(validPath, false);
    QFile validFile(validPath);
    const bool readBack = validFile.open(QIODevice::ReadOnly)
        && QString::fromUtf8(validFile.readAll()) == sample;
    validFile.close();
    addTestResult(&results,
                  QStringLiteral("保存与读回"),
                  saved && readBack && !m_modified,
                  validPath);

    m_editor->moveCursor(QTextCursor::End);
    m_editor->insertPlainText(QStringLiteral("触发修改状态"));
    const QString modifiedSample = m_editor->toPlainText();
    addTestResult(&results,
                  QStringLiteral("修改状态与窗口标题"),
                  m_modified && isWindowModified(),
                  displayFileName());

    const bool modifiedSaved = saveToFile(validPath, false);
    const bool opened = openFile(validPath, false);
    addTestResult(&results,
                  QStringLiteral("重新打开文本文件"),
                  modifiedSaved && opened
                      && m_editor->toPlainText() == modifiedSample
                      && !m_modified,
                  validPath);

    const QString missingPath = output.filePath(QStringLiteral("不存在.txt"));
    const bool missingRejected = !openFile(missingPath, false);
    addTestResult(&results,
                  QStringLiteral("打开不存在文件"),
                  missingRejected,
                  missingPath);

    const QString binaryPath = output.filePath(QStringLiteral("二进制异常.bin"));
    QFile binaryFile(binaryPath);
    if (binaryFile.open(QIODevice::WriteOnly)) {
        binaryFile.write(QByteArray("abc\0def", 7));
        binaryFile.close();
    }
    const bool binaryRejected = !openFile(binaryPath, false);
    addTestResult(&results,
                  QStringLiteral("拒绝二进制文件"),
                  binaryRejected,
                  binaryPath);

    {
        const QSignalBlocker blocker(m_editor);
        m_editor->setPlainText(QStringLiteral("alpha beta alpha"));
        m_editor->moveCursor(QTextCursor::Start);
    }
    const bool firstFind = findNext(QStringLiteral("alpha"), true, false, false);
    const bool firstMatch = m_editor->textCursor().selectedText()
        == QStringLiteral("alpha");
    const bool secondFind = findNext(QStringLiteral("alpha"), true, false, false);
    const int firstEnd = m_editor->textCursor().selectionEnd();
    addTestResult(&results,
                  QStringLiteral("连续查找与循环"),
                  firstFind && firstMatch && secondFind
                      && firstEnd > QStringLiteral("alpha").size(),
                  QStringLiteral("position=%1").arg(firstEnd));

    {
        const QSignalBlocker blocker(m_editor);
        m_editor->setPlainText(QStringLiteral("one two one"));
    }
    const int replacementCount = replaceAll(
        QStringLiteral("one"), QStringLiteral("1"), true);
    addTestResult(&results,
                  QStringLiteral("替换全部"),
                  replacementCount == 2
                      && m_editor->toPlainText() == QStringLiteral("1 two 1"),
                  QStringLiteral("count=%1").arg(replacementCount));

    const bool undoAvailable = m_editor->document()->isUndoAvailable();
    m_editor->undo();
    const bool undoWorked =
        m_editor->toPlainText() == QStringLiteral("one two one");
    m_editor->redo();
    const bool redoWorked =
        m_editor->toPlainText() == QStringLiteral("1 two 1");
    addTestResult(&results,
                  QStringLiteral("撤销与重做"),
                  undoAvailable && undoWorked && redoWorked,
                  QStringLiteral("undo=%1, redo=%2")
                      .arg(undoWorked)
                      .arg(redoWorked));

    QTextCursor selection = m_editor->textCursor();
    selection.select(QTextCursor::Document);
    m_editor->setTextCursor(selection);
    updateActionStates();
    addTestResult(&results,
                  QStringLiteral("剪切复制 Action 使能"),
                  ui->actionCut->isEnabled()
                      && ui->actionCopy->isEnabled(),
                  QStringLiteral("cut=%1, copy=%2")
                      .arg(ui->actionCut->isEnabled())
                      .arg(ui->actionCopy->isEnabled()));
    m_editor->moveCursor(QTextCursor::End);

    const bool invalidSaveRejected =
        !saveToFile(output.absolutePath(), false);
    addTestResult(&results,
                  QStringLiteral("异常保存路径"),
                  invalidSaveRejected,
                  output.absolutePath());

    setLineNumbersEnabled(false);
    const bool lineNumbersHidden = !m_editor->lineNumbersVisible();
    setLineNumbersEnabled(true);
    setWordWrapEnabled(false);
    const bool wrapDisabled =
        m_editor->lineWrapMode() == QPlainTextEdit::NoWrap;
    setWordWrapEnabled(true);
    addTestResult(&results,
                  QStringLiteral("行号与自动换行切换"),
                  lineNumbersHidden
                      && m_editor->lineNumbersVisible()
                      && wrapDisabled
                      && m_editor->lineWrapMode() == QPlainTextEdit::WidgetWidth,
                  QStringLiteral("lineNumbers=on, wrap=on"));

    {
        const QSignalBlocker blocker(m_editor);
        m_editor->setPlainText(sample);
        m_editor->moveCursor(QTextCursor::Start);
    }
    setCurrentFile(validPath);
    setModifiedState(false);
    updateStatusBar();
    resize(1180, 760);
    captureWidget(this,
                  output.filePath(QStringLiteral("主窗口.png")),
                  QStringLiteral("主窗口运行截图"),
                  &results);

    showFindDialog();
    if (m_findDialog) {
        m_findDialog->setFindText(QStringLiteral("alpha"));
        captureWidget(m_findDialog,
                      output.filePath(QStringLiteral("查找对话框.png")),
                      QStringLiteral("查找对话框截图"),
                      &results);
        m_findDialog->hide();
    }

    showReplaceDialog();
    if (m_replaceDialog) {
        m_replaceDialog->setFindText(QStringLiteral("alpha"));
        captureWidget(m_replaceDialog,
                      output.filePath(QStringLiteral("替换对话框.png")),
                      QStringLiteral("替换对话框截图"),
                      &results);
        m_replaceDialog->hide();
    }

    showAboutDialog();
    if (m_aboutDialog) {
        captureWidget(m_aboutDialog,
                      output.filePath(QStringLiteral("关于对话框.png")),
                      QStringLiteral("关于对话框截图"),
                      &results);
        m_aboutDialog->hide();
    }

    const bool allPassed =
        std::all_of(results.cbegin(), results.cend(), [](const QString &line) {
            return line.startsWith(QStringLiteral("PASS"));
        });

    QFile report(output.filePath(QStringLiteral("自动测试报告.txt")));
    if (report.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QString header =
            QStringLiteral(
                "小牛文本编辑器自动测试报告\n"
                "测试时间：%1\n"
                "结果：%2\n"
                "格式：状态<TAB>项目<TAB>详情\n\n")
                .arg(QDateTime::currentDateTime().toString(
                         QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                     allPassed ? QStringLiteral("全部通过")
                               : QStringLiteral("存在失败"));
        report.write(header.toUtf8());
        report.write(results.join(QLatin1Char('\n')).toUtf8());
        report.write("\n");
        report.close();
    }

    return allPassed;
}
