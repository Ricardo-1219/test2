#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QPalette>
#include <QStyleFactory>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("小牛文本编辑器"));
    QCoreApplication::setOrganizationName(QStringLiteral("东莞理工学院"));

    const QStringList arguments = app.arguments();
    const int selfTestIndex = arguments.indexOf(QStringLiteral("--self-test"));
    if (selfTestIndex >= 0) {
        app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(246, 247, 249));
        palette.setColor(QPalette::WindowText, QColor(28, 32, 38));
        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::AlternateBase, QColor(244, 246, 249));
        palette.setColor(QPalette::Text, QColor(28, 32, 38));
        palette.setColor(QPalette::Button, QColor(244, 246, 249));
        palette.setColor(QPalette::ButtonText, QColor(28, 32, 38));
        palette.setColor(QPalette::Highlight, QColor(57, 126, 211));
        palette.setColor(QPalette::HighlightedText, Qt::white);
        app.setPalette(palette);
    }

    MainWindow window;

    if (selfTestIndex >= 0) {
        if (selfTestIndex + 1 >= arguments.size()) {
            QTextStream(stderr) << "Usage: MyNotepad --self-test <output-directory>\n";
            return 2;
        }

        const QDir outputDirectory(arguments.at(selfTestIndex + 1));
        if (!outputDirectory.exists() && !QDir().mkpath(outputDirectory.absolutePath())) {
            QTextStream(stderr) << "Cannot create self-test output directory.\n";
            return 2;
        }

        window.resize(1180, 760);
        window.show();
        app.processEvents();
        const bool passed = window.runSelfTest(outputDirectory.absolutePath());
        return passed ? 0 : 1;
    }

    if (arguments.size() > 1 && !arguments.at(1).startsWith(QStringLiteral("--"))) {
        window.openFile(arguments.at(1));
    }

    window.show();
    return app.exec();
}
