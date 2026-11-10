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

