#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class AboutDialog;
class CodeEditor;
class FindDialog;
class QCloseEvent;
class QLabel;
class QTimer;
class ReplaceDialog;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    bool openFile(const QString &filePath, bool showError = true);
    bool runSelfTest(const QString &outputDirectory);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    bool saveFile();
    bool saveFileAs();
    void showFindDialog();
    void showReplaceDialog();
    void showAboutDialog();
    void chooseFont();
    void chooseTextColor();
    void chooseTextBackgroundColor();
    void chooseEditorBackgroundColor();
    void setWordWrapEnabled(bool enabled);
    void setLineNumbersEnabled(bool enabled);
    void updateWindowTitle();
    void updateStatusBar();
    void updateActionStates();
    void markDocumentModified();
    void autoSave();
    void findRequested(const QString &text,
                       bool caseSensitive,
                       bool backward);
    void replaceRequested(const QString &text,
                          const QString &replacement,
                          bool caseSensitive,
                          bool backward);
    void replaceAllRequested(const QString &text,
                             const QString &replacement,
                             bool caseSensitive);

private:
    bool maybeSave();
    bool saveToFile(const QString &filePath, bool showError);
    bool findNext(const QString &text,
                  bool caseSensitive,
                  bool backward,
                  bool showNotFound);
    bool replaceCurrent(const QString &text,
                        const QString &replacement,
                        bool caseSensitive,
                        bool backward);
    int replaceAll(const QString &text,
                   const QString &replacement,
                   bool caseSensitive);
    void setCurrentFile(const QString &filePath);
    void setModifiedState(bool modified);
    QString displayFileName() const;
    void configureActions();
    void createStatusBarWidgets();
    void addTestResult(QStringList *results,
                       const QString &name,
                       bool passed,
                       const QString &details);
    void captureWidget(QWidget *widget,
                       const QString &filePath,
                       const QString &title,
                       QStringList *results);

    Ui::MainWindow *ui = nullptr;
    CodeEditor *m_editor = nullptr;
    QLabel *m_lengthLabel = nullptr;
    QLabel *m_lineCountLabel = nullptr;
    QLabel *m_cursorLabel = nullptr;
    QLabel *m_authorLabel = nullptr;
    AboutDialog *m_aboutDialog = nullptr;
    FindDialog *m_findDialog = nullptr;
    ReplaceDialog *m_replaceDialog = nullptr;
    QTimer *m_autoSaveTimer = nullptr;
    QString m_currentFilePath;
    bool m_modified = false;
};

#endif
