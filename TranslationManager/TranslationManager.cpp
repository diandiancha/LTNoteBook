#include "TranslationManager.h"
#include <QWidget>
#include <QApplication>
#include <QCoreApplication>

TranslationManager& TranslationManager::instance() {
    static TranslationManager inst;
    return inst;
}

void TranslationManager::loadInitial(const QString& code) {
    if (qtTranslator.load(QLocale(code),
                          "qtbase", "_",
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QCoreApplication::installTranslator(&qtTranslator);
    }
    if (code != "zh_CN") {
        QString qm = QStringLiteral(":/i18n/app_%1.qm").arg(code);
        if (appTranslator.load(qm)) {
            QCoreApplication::installTranslator(&appTranslator);
        }
    }
}

void TranslationManager::switchLanguage(const QString& code) {
    QCoreApplication::removeTranslator(&qtTranslator);
    QCoreApplication::removeTranslator(&appTranslator);

    loadInitial(code);

    const auto topWidgets = QApplication::topLevelWidgets();
    for (QWidget* w : topWidgets) {
        QEvent ev(QEvent::LanguageChange);
        QCoreApplication::sendEvent(w, &ev);
    }
}
