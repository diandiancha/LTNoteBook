#include "widget.h"
#include "config.h"
#include "TranslationManager.h"
#include <QApplication>
#include <QSettings>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    QSettings s(ORG, APP);
    QString code = s.value(KEY_LANG, "zh_CN").toString();

    Widget w;
    TranslationManager::instance().loadInitial(code);
    QStringList args = QCoreApplication::arguments();
    if (args.size() > 1) {
        w.openFileFromPath(QFileInfo(args.at(1)).absoluteFilePath());
    }

    w.show();
    return a.exec();
}
