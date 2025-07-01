#include "widget.h"
#include "config.h"
#include "TranslationManager.h"
#include <QApplication>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QSettings s(ORG, APP);
    QString code = s.value(KEY_LANG, "zh_CN").toString();

    TranslationManager::instance().loadInitial(code);

    Widget w;
    w.show();
    return a.exec();
}
