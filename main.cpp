#include "widget.h"
#include "config.h"
#include "TranslationManager.h"
#include <QApplication>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyleSheet(R"(
        QToolTip {
            background-color: white;
            color: white;
            border: 1px solid gray;
            border-radius: 4px;
            padding: 4px;
        }
    )");

    QSettings s(ORG, APP);
    QString code = s.value(KEY_LANG, "zh_CN").toString();
    TranslationManager::instance().loadInitial(code);

    Widget w;

    QStringList args = QCoreApplication::arguments();
    if (args.size() > 1) {
        w.openFileFromPath(QFileInfo(args.at(1)).absoluteFilePath());
    }

    w.show();
    return a.exec();
}
