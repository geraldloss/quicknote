#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

#include <QString>
#include <QMap>

class Translations {
public:
    static void setLanguage(const QString& lang);
    static QString get(const QString& key);

private:
    static QString currentLanguage;
    static const QMap<QString, QString> englishTranslations;
    static const QMap<QString, QString> germanTranslations;
    static const QMap<QString, QString> frenchTranslations;
    static const QMap<QString, QString> spanishTranslations;
    static const QMap<QString, QString> italianTranslations;
    static const QMap<QString, QString> chineseTranslations;
};

#endif 