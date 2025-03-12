#ifndef EDITOR_H
#define EDITOR_H

#include <QMainWindow>
#include <QTextEdit>
#include <QFile>
#include <QDir>
#include <QShortcut>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QAction>
#include <QTextCursor>
#include <QKeyEvent>
#include <zlib.h>
#include <QJsonObject>
#include <QColor>
#include <QKeySequence>
#include <QKeySequenceEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDialog>
#include <QSpinBox>
#include <QComboBox>
#include "qhotkey.h"
#include <QtNetwork/QLocalServer>
#include <QSystemTrayIcon>

class Editor : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Konstruktor für den Editor
     * @param parent Das übergeordnete Widget
     */
    Editor(QWidget *parent = nullptr);

    /**
     * @brief Destruktor, speichert den Text beim Beenden
     */
    ~Editor();

protected:
    /**
     * @brief Filtert Events für Tastenkombinationen
     * @param obj Das Objekt, das das Event empfängt
     * @param event Das zu verarbeitende Event
     * @return true wenn Event verarbeitet wurde, sonst false
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    void closeEvent(QCloseEvent *event) override;

private:
    // attributes
    QTextEdit* m_textEdit;
    QJsonArray m_history;
    int m_currentHistoryIndex;
    bool m_deactivateHistoryEvent;
    bool m_dontSaveSettings;
    int m_maxHistorySize;
    QJsonObject m_state;
    QColor m_backgroundColor;
    QColor m_textColor;
    QKeySequence m_toggleWindowShortcut;
    QHotkey* m_toggleHotkey;
    QLocalServer* m_localServer;
    QSystemTrayIcon* m_trayIcon;
    QString m_language;  // Neue Variable für die Sprache

    // methods
    /**
     * @brief Gibt den Pfad zur History-Datei zurück
     * @return Pfad zur History-Datei
     */
    QString getHistoryFile() const;

    /**
     * @brief Richtet die Tastenkombinationen ein
     */
    void setupShortcuts();

    /**
     * @brief Speichert den Verlauf in eine JSON-Datei
     */
    void saveHistory();

    /**
     * @brief Lädt den Verlauf aus der JSON-Datei
     */
    void loadHistory();

    /**
     * @brief Führt eine Redo-Operation aus
     */
    void executeRedo();

    /**
     * @brief Führt eine Undo-Operation aus
     */
    void executeUndo();

    void setupContextMenu();
    void loadSettings();
    void saveSettings();

    /**
     * @brief Erstellt und gibt den Pfad zum Datenverzeichnis zurück
     * @return Absoluter Pfad zum .local/share/quicknote Verzeichnis
     */
    QString getDataDir() const;

    void applyColors();

    void setupGlobalShortcut();

    void saveHistoryIndex();  // Neue Methode

    void setupSingleInstance();

    void setupTrayIcon();

    void setupSettingsMenu(QMenu* settingsMenu);  // Neue Methode

private slots:
    /**
     * @brief Wird aufgerufen, wenn sich der Text ändert
     */
    void onTextChanged();
};

#endif 