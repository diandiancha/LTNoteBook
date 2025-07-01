#pragma once
#include <QTranslator>
#include <QCoreApplication>
#include <QString>
#include <QLibraryInfo>
#include <QLocale>

class TranslationManager {
public:
    static TranslationManager& instance();

    void loadInitial(const QString& code);

    void switchLanguage(const QString& code);

private:
    TranslationManager() = default;
    QTranslator qtTranslator;
    QTranslator appTranslator;
};
