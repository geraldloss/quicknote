#include "translations.h"

QString Translations::currentLanguage = "en";

void Translations::setLanguage(const QString& lang) 
{ 
    currentLanguage = lang; 
}

QString Translations::get(const QString& key) 
{
    if (currentLanguage == "de" && germanTranslations.contains(key)) {
        return germanTranslations[key];
    }
    else if (currentLanguage == "fr" && frenchTranslations.contains(key)) {
        return frenchTranslations[key];
    }
    else if (currentLanguage == "es" && spanishTranslations.contains(key)) {
        return spanishTranslations[key];
    }
    else if (currentLanguage == "it" && italianTranslations.contains(key)) {
        return italianTranslations[key];
    }
    else if (currentLanguage == "zh" && chineseTranslations.contains(key)) {
        return chineseTranslations[key];
    }
    return englishTranslations[key];
}

const QMap<QString, QString> Translations::englishTranslations = {
    {"settings", "Settings..."},
    {"clear_history", "Clear History"},
    {"quit", "Quit"},
    {"quit_confirm", "Do you really want to quit QuickNote?"},
    {"max_history", "Maximum History Length:"},
    {"bg_color", "Background Color..."},
    {"text_color", "Text Color..."},
    {"shortcut", "Toggle Window Shortcut:"},
    {"default_shortcut", "Default Shortcut (ScrollLock)"},
    {"clear_history_confirm", "Do you really want to clear the entire history?"},
    {"language", "Language"},
    {"english", "English"},
    {"german", "Deutsch"},
    {"french", "Français"},
    {"spanish", "Español"},
    {"italian", "Italiano"},
    {"chinese", "中文"}
};

const QMap<QString, QString> Translations::germanTranslations = {
    {"settings", "Einstellungen..."},
    {"clear_history", "History löschen"},
    {"quit", "Beenden"},
    {"quit_confirm", "Möchten Sie QuickNote wirklich beenden?"},
    {"max_history", "Maximale History-Länge:"},
    {"bg_color", "Hintergrundfarbe..."},
    {"text_color", "Textfarbe..."},
    {"shortcut", "Shortcut Ein- und Ausblenden:"},
    {"default_shortcut", "Default Shortcut (ScrollLock)"},
    {"clear_history_confirm", "Möchten Sie wirklich die gesamte History löschen?"},
    {"language", "Sprache"},
    {"english", "English"},
    {"german", "Deutsch"},
    {"french", "Français"},
    {"spanish", "Español"},
    {"italian", "Italiano"},
    {"chinese", "中文"}
};

const QMap<QString, QString> Translations::frenchTranslations = {
    {"settings", "Paramètres..."},
    {"clear_history", "Effacer l'historique"},
    {"quit", "Quitter"},
    {"quit_confirm", "Voulez-vous vraiment quitter QuickNote ?"},
    {"max_history", "Longueur maximale de l'historique:"},
    {"bg_color", "Couleur de fond..."},
    {"text_color", "Couleur du texte..."},
    {"shortcut", "Raccourci afficher/masquer:"},
    {"default_shortcut", "Raccourci par défaut (ScrollLock)"},
    {"clear_history_confirm", "Voulez-vous vraiment effacer tout l'historique ?"},
    {"language", "Langue"},
    {"english", "English"},
    {"german", "Deutsch"},
    {"french", "Français"},
    {"spanish", "Español"},
    {"italian", "Italiano"},
    {"chinese", "中文"}
};

const QMap<QString, QString> Translations::spanishTranslations = {
    {"settings", "Ajustes..."},
    {"clear_history", "Borrar historial"},
    {"quit", "Salir"},
    {"quit_confirm", "¿Realmente desea salir de QuickNote?"},
    {"max_history", "Longitud máxima del historial:"},
    {"bg_color", "Color de fondo..."},
    {"text_color", "Color del texto..."},
    {"shortcut", "Atajo mostrar/ocultar:"},
    {"default_shortcut", "Atajo predeterminado (ScrollLock)"},
    {"clear_history_confirm", "¿Realmente desea borrar todo el historial?"},
    {"language", "Idioma"},
    {"english", "English"},
    {"german", "Deutsch"},
    {"french", "Français"},
    {"spanish", "Español"},
    {"italian", "Italiano"},
    {"chinese", "中文"}
};

const QMap<QString, QString> Translations::italianTranslations = {
    {"settings", "Impostazioni..."},
    {"clear_history", "Cancella cronologia"},
    {"quit", "Esci"},
    {"quit_confirm", "Vuoi davvero uscire da QuickNote?"},
    {"max_history", "Lunghezza massima cronologia:"},
    {"bg_color", "Colore sfondo..."},
    {"text_color", "Colore testo..."},
    {"shortcut", "Scorciatoia mostra/nascondi:"},
    {"default_shortcut", "Scorciatoia predefinita (ScrollLock)"},
    {"clear_history_confirm", "Vuoi davvero cancellare tutta la cronologia?"},
    {"language", "Lingua"},
    {"english", "English"},
    {"german", "Deutsch"},
    {"french", "Français"},
    {"spanish", "Español"},
    {"italian", "Italiano"},
    {"chinese", "中文"}
};

const QMap<QString, QString> Translations::chineseTranslations = {
    {"settings", "设置..."},
    {"clear_history", "清除历史"},
    {"quit", "退出"},
    {"quit_confirm", "确实要退出 QuickNote 吗？"},
    {"max_history", "最大历史长度:"},
    {"bg_color", "背景颜色..."},
    {"text_color", "文字颜色..."},
    {"shortcut", "显示/隐藏快捷键:"},
    {"default_shortcut", "默认快捷键 (ScrollLock)"},
    {"clear_history_confirm", "确实要清除所有历史记录吗？"},
    {"language", "语言"},
    {"english", "English"},
    {"german", "Deutsch"},
    {"french", "Français"},
    {"spanish", "Español"},
    {"italian", "Italiano"},
    {"chinese", "中文"}
}; 