#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Awake Buddy");
    a.setQuitOnLastWindowClosed(false);

    // Handle --export-icon flag
    QCommandLineParser parser;
    QCommandLineOption exportIconOpt("export-icon", "Export app icon to .ico file", "path");
    parser.addOption(exportIconOpt);
    parser.parse(a.arguments());

    if (parser.isSet(exportIconOpt)) {
        MainWindow w;
        return w.exportIcon(parser.value(exportIconOpt)) ? 0 : 1;
    }

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Awake-Buddy_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return a.exec();
}
