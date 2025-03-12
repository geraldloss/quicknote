#include "editor.h"
#include <QVBoxLayout>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QByteArray>
#include <zlib.h>
#include <QMenu>
#include <QInputDialog>
#include <QSettings>
#include <QColorDialog>
#include <QKeySequenceEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QShortcut>
#include <QApplication>
#include "qhotkey.h"
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QSystemTrayIcon>
#include "translations.h"

/**
 * @brief Erstellt und gibt den Pfad zum Datenverzeichnis zurück
 */
QString Editor::getDataDir() const
{
    QString path = QDir::homePath() + "/.local/share/quicknote";
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return path;
}

/**
 * @brief Gibt den Pfad zur History-Datei zurück
 * @return Absoluter Pfad zur History-Datei im .local/share/quicknote Verzeichnis
 */
QString Editor::getHistoryFile() const
{
    return getDataDir() + "/history.gz";
}

/**
 * @brief Konstruktor - Initialisiert den Editor und seine Komponenten
 * @param parent Das übergeordnete Widget
 */
Editor::Editor(QWidget *parent) : QMainWindow(parent), m_currentHistoryIndex(-1), m_deactivateHistoryEvent(false), m_toggleHotkey(nullptr), m_localServer(nullptr), m_dontSaveSettings(false), m_trayIcon(nullptr)
{
    setupSingleInstance();
    if (m_localServer == nullptr) return;  // Beende wenn andere Instanz läuft
    
    m_textEdit = new QTextEdit(this);
    setCentralWidget(m_textEdit);
    
    // Installiere globalen Event-Filter direkt hier
    qApp->installEventFilter(this);
    
    loadSettings();
    applyColors();
    
    setupContextMenu();
    
    m_textEdit->setUndoRedoEnabled(false);
    m_textEdit->installEventFilter(this);
    
    setWindowTitle("QuickNote");
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    
    m_deactivateHistoryEvent = true;
    loadHistory();
    m_deactivateHistoryEvent = false;
    setupShortcuts();
    setupGlobalShortcut();
    
    connect(m_textEdit, &QTextEdit::textChanged, this, &Editor::onTextChanged);
    
    setupTrayIcon();
    
    // Fenster initial verstecken
    hide();
}

Editor::~Editor()
{
    saveSettings();
}

/**
 * @brief Komprimiert Daten mit zlib im gzip-Format
 */
QByteArray compressData(const QByteArray& data) 
{
    // Erstellt einen neuen Puffer für die komprimierten Daten
    QByteArray compressed;
    compressed.resize(data.size() + 16);  // Extra Platz für Header und Footer
    
    // Initialisiert die zlib-Struktur
    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    
    // Initialisiert den Kompressor im gzip-Format
    deflateInit2(&zs, 
                1,              // Level 1 = schnellste Kompression
                Z_DEFLATED,     // Komprimierungsmethode
                31,            // 15 für normale Kompression + 16 für gzip header
                8,             // Standard Speicherlevel
                Z_DEFAULT_STRATEGY);
    
    // Setzt die Ein- und Ausgabeparameter
    zs.avail_in = data.size();
    zs.next_in = (Bytef*)data.data();
    zs.avail_out = compressed.size();
    zs.next_out = (Bytef*)compressed.data();
    
    // Führt die Kompression durch
    deflate(&zs, Z_FINISH);
    compressed.resize(zs.total_out);
    deflateEnd(&zs);
    
    return compressed;
}

/**
 * @brief Dekomprimiert gzip-komprimierte Daten
 */
QByteArray decompressData(const QByteArray& data)
{
    if (data.isEmpty()) return QByteArray();

    QByteArray decompressed;
    const int CHUNK = 16384;
    decompressed.resize(CHUNK);
    
    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    zs.avail_in = data.size();
    zs.next_in = (Bytef*)data.data();
    
    if (inflateInit2(&zs, 47) != Z_OK) {
        qDebug() << "Fehler bei inflateInit2";
        return QByteArray();
    }

    int ret;
    do {
        zs.avail_out = CHUNK;
        zs.next_out = (Bytef*)(decompressed.data() + zs.total_out);
        
        ret = inflate(&zs, Z_NO_FLUSH);
        
        if (ret < 0) {
            qDebug() << "Fehler bei inflate:" << ret;
            inflateEnd(&zs);
            return QByteArray();
        }
        
        if (zs.avail_out == 0) {
            decompressed.resize(decompressed.size() + CHUNK);
        }
        
    } while (ret != Z_STREAM_END);

    decompressed.resize(zs.total_out);
    inflateEnd(&zs);
    
    return decompressed;
}

/**
 * @brief Speichert den aktuellen Zustand in die History
 * 
 * Fügt den aktuellen Text zur History hinzu, wenn er sich vom letzten
 * Zustand unterscheidet. Begrenzt die History auf 100 Einträge.
 * Speichert die History komprimiert.
 */
void Editor::saveHistory()
{
    if (m_deactivateHistoryEvent) return;

    QString currentText = m_textEdit->toPlainText();
    int cursorPos = m_textEdit->textCursor().position();

    // Erstelle JSON-Objekt für den aktuellen Zustand
    QJsonObject currentState;
    currentState["text"] = currentText;
    currentState["cursor"] = cursorPos;

    // Prüfe ob sich der Text oder Cursorposition geändert hat
    bool shouldSave = m_history.isEmpty();
    if (!shouldSave && m_currentHistoryIndex >= 0) {
        QJsonObject lastState = m_history[m_currentHistoryIndex].toObject();
        shouldSave = lastState["text"].toString() != currentText ||
                    lastState["cursor"].toInt() != cursorPos;
    }

    // Wenn sich der Text oder Cursorposition geändert hat, speichere den aktuellen Zustand
    if (shouldSave)
    {
        // Lösche alle Einträge nach dem aktuellen Index
        while (m_history.size() > m_currentHistoryIndex + 1) {
            m_history.removeLast();
        }

        m_currentHistoryIndex++;

        if (m_history.size() >= m_maxHistorySize) {
            m_history.removeAt(0);
            m_currentHistoryIndex--;
        }
        
        m_history.append(currentState);

        // Speichere History und Index getrennt
        m_state["currentIndex"] = m_currentHistoryIndex;

        QFile file(getHistoryFile());
        if (file.open(QIODevice::WriteOnly))
        {
            QJsonObject saveData;
            saveData["history"] = m_history;
            saveData["state"] = m_state;  // Speichere state separat
            
            QJsonDocument doc(saveData);
            QByteArray data = doc.toJson();
            QByteArray compressedData = compressData(data);
            file.write(compressedData);
            // file.write(data);
            file.close();
        }
    }
}

/**
 * @brief Lädt die gespeicherte History aus der JSON-Datei
 * 
 * Liest die komprimierte History-Datei und dekomprimiert sie.
 */
void Editor::loadHistory()
{
    QFile file(getHistoryFile());
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray compressedData = file.readAll();
        QByteArray data = decompressData(compressedData);
        // QByteArray data = file.readAll();

        // Überprüfe, ob die Daten gültig sind
        if (data.isEmpty()) {
            qDebug() << "Fehler: Dekomprimierte Daten sind leer";
            file.close();
            return;
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        
        
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "JSON Parse Fehler:" << parseError.errorString();
            file.close();
            return;
        }
        
        if (!doc.isObject()) {
            qDebug() << "Fehler: JSON-Dokument ist kein Objekt";
            file.close();
            return;
        }
        
        QJsonObject saveData = doc.object();
        
        // Überprüfe, ob die erwarteten Schlüssel existieren
        if (!saveData.contains("history") || !saveData["history"].isArray()) {
            qDebug() << "Fehler: Keine gültige History in den Daten gefunden";
            qDebug() << "SaveData:" << saveData;
            file.close();
            return;
        }
        
        m_history = saveData["history"].toArray();
        m_state = saveData["state"].toObject();
        m_currentHistoryIndex = m_state["currentIndex"].toInt();
        
        if (m_currentHistoryIndex >= 0 && m_currentHistoryIndex < m_history.size()) {
            QJsonObject currentHistory = m_history[m_currentHistoryIndex].toObject();
            m_textEdit->setText(currentHistory["text"].toString());
            
            QTextCursor cursor = m_textEdit->textCursor();
            auto cursorPos = currentHistory["cursor"].toInt();
            cursor.setPosition(cursorPos);
            m_textEdit->setTextCursor(cursor);
        }
        
        file.close();
    }
}

/**
 * @brief Event-Handler für Textänderungen
 * 
 * Speichert den Text und aktualisiert die History,
 * außer wenn m_skipHistoryUpdate gesetzt ist.
 */
void Editor::onTextChanged()
{
    saveHistory();  // Nur noch History speichern
}

/**
 * @brief Richtet die Tastenkombinationen für den Editor ein
 * 
 * Konfiguriert Shortcuts für:
 * - Strg+L: Trennlinie einfügen
 */
void Editor::setupShortcuts()
{
    // Trennlinie einfügen (Strg+L)
    QAction* lineAction = new QAction("Line", m_textEdit);  // Parent ist m_textEdit
    lineAction->setShortcut(Qt::CTRL | Qt::Key_L);
    lineAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);  // Wichtig!
    m_textEdit->addAction(lineAction);  // Action zum TextEdit
    connect(lineAction, &QAction::triggered, [this]() {
        QTextCursor cursor = m_textEdit->textCursor();
        cursor.movePosition(QTextCursor::EndOfLine);
        cursor.insertText("\n----------------------------------------------------------------------------\n");
        m_textEdit->setTextCursor(cursor);
    });
}

/**
 * @brief Führt eine Redo-Operation aus
 * 
 * Stellt den nächsten Zustand aus der History wieder her,
 * falls verfügbar.
 */
void Editor::executeRedo()
{
    if (m_currentHistoryIndex < m_history.size() - 1) {
        m_deactivateHistoryEvent = true;
        m_currentHistoryIndex++;
        
        QJsonObject state = m_history[m_currentHistoryIndex].toObject();
        m_textEdit->setText(state["text"].toString());
        
        QTextCursor cursor = m_textEdit->textCursor();
        cursor.setPosition(state["cursor"].toInt());
        m_textEdit->setTextCursor(cursor);
        
        m_deactivateHistoryEvent = false;
        saveHistoryIndex();  // Statt saveHistory()
    }
}

/**
 * @brief Führt eine Undo-Operation aus
 * 
 * Stellt den vorherigen Zustand aus der History wieder her,
 * falls verfügbar. Verhindert dabei das Hinzufügen des
 * wiederhergestellten Zustands zur History.
 */
void Editor::executeUndo()
{
    if (m_currentHistoryIndex > 0) {
        m_deactivateHistoryEvent = true;
        m_currentHistoryIndex--;
        
        QJsonObject state = m_history[m_currentHistoryIndex].toObject();
        m_textEdit->setText(state["text"].toString());
        
        QTextCursor cursor = m_textEdit->textCursor();
        cursor.setPosition(state["cursor"].toInt());
        m_textEdit->setTextCursor(cursor);
        
        m_deactivateHistoryEvent = false;
        saveHistoryIndex();  // Statt saveHistory()
    }
}

/**
 * @brief Filtert Tastatur-Events für Undo/Redo
 * 
 * Verarbeitet:
 * - Strg+Z: Undo
 * - Strg+Shift+Z: Redo
 * - Strg+Y: Alternative Redo
 * 
 * @param obj Das Objekt, das das Event empfängt
 * @param event Das zu verarbeitende Event
 * @return true wenn Event verarbeitet wurde, sonst false
 */
bool Editor::eventFilter(QObject *obj, QEvent *event)
{
        // Andere Shortcuts nur bei aktivem Fenster
    if (obj == m_textEdit && isActiveWindow() && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->matches(QKeySequence::Undo)) {
            executeUndo();
            return true;
        }
        else if (keyEvent->matches(QKeySequence::Redo)) {
            executeRedo();
            return true;
        }
        else if (keyEvent->modifiers() == Qt::CTRL && keyEvent->key() == Qt::Key_Y) {
            executeRedo();
            return true;
        }
    }
    
    return QMainWindow::eventFilter(obj, event);
}

void Editor::setupContextMenu()
{
    m_textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_textEdit, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu *menu = m_textEdit->createStandardContextMenu();
        menu->addSeparator();
        
        // Direkt ins Hauptmenü
        setupSettingsMenu(menu);
        
        menu->addSeparator();
        QAction *quitAction = menu->addAction(Translations::get("quit"));
        connect(quitAction, &QAction::triggered, this, [this]() {
            QMessageBox::StandardButton reply = QMessageBox::question(this, 
                Translations::get("quit"),
                Translations::get("quit_confirm"),
                QMessageBox::Yes | QMessageBox::No);
            
            if (reply == QMessageBox::Yes) {
                QApplication::quit();
            }
        });
        
        menu->exec(m_textEdit->mapToGlobal(pos));
        delete menu;
    });
}

void Editor::loadSettings()
{
    QString path = QDir::homePath() + "/.config/quicknote";
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QSettings settings(path + "/settings.conf", QSettings::IniFormat);
    m_maxHistorySize = settings.value("maxHistorySize", 9999).toInt();
    m_backgroundColor = settings.value("backgroundColor", QColor(255, 250, 205)).value<QColor>();  // LemonChiffon
    m_textColor = settings.value("textColor", QColor(0, 0, 0)).value<QColor>();
    m_toggleWindowShortcut = QKeySequence(settings.value("toggleWindowShortcut").toString());
    m_language = settings.value("language", "en").toString();
    Translations::setLanguage(m_language);
    
    // Wende Farben an
    applyColors();
    
    // Lade Fenstergeometrie
    QRect geometry = settings.value("windowGeometry", QRect(100, 100, 800, 600)).toRect();
    setGeometry(geometry);
}

void Editor::saveSettings()
{
    if (m_dontSaveSettings) return;
    
    QString path = QDir::homePath() + "/.config/quicknote";
    QSettings settings(path + "/settings.conf", QSettings::IniFormat);
    settings.setValue("maxHistorySize", m_maxHistorySize);
    settings.setValue("backgroundColor", m_backgroundColor);
    settings.setValue("textColor", m_textColor);
    settings.setValue("toggleWindowShortcut", m_toggleWindowShortcut.toString());
    settings.setValue("language", m_language);
    
    // Speichere Fenstergeometrie
    settings.setValue("windowGeometry", geometry());
}

void Editor::applyColors()
{
    QPalette p = m_textEdit->palette();
    p.setColor(QPalette::Base, m_backgroundColor);
    p.setColor(QPalette::Text, m_textColor);
    m_textEdit->setPalette(p);
}

void Editor::closeEvent(QCloseEvent *event)
{
    // Beim X-Button nur verstecken
    event->ignore();
    hide();
}

void Editor::setupGlobalShortcut()
{
    if (m_toggleHotkey) {
        delete m_toggleHotkey;
    }
    
    // Verwende Rollen-Taste als Standard
    QKeySequence shortcut;
    if (m_toggleWindowShortcut.isEmpty()) {
        shortcut = QKeySequence(65300);  // Der Key-Code für Scroll Lock

    } else {
        shortcut = m_toggleWindowShortcut;
    }
    
    m_toggleHotkey = new QHotkey(shortcut, true, this);
    if (!m_toggleHotkey->isRegistered()) {
        qDebug() << "Hotkey konnte nicht registriert werden:" << shortcut.toString();
    }
    
    connect(m_toggleHotkey, &QHotkey::activated, this, [this]() {
        if (isVisible()) {
            hide();
        } else {
            show();
            raise();
            activateWindow();
        }
    });
}

void Editor::saveHistoryIndex()
{
    m_state["currentIndex"] = m_currentHistoryIndex;
    
    QFile file(getHistoryFile());
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject saveData;
        saveData["history"] = m_history;
        saveData["state"] = m_state;
        
        QJsonDocument doc(saveData);
        QByteArray data = doc.toJson();
        QByteArray compressedData = compressData(data);
        file.write(compressedData);
        file.close();
    }
}

void Editor::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(QIcon::fromTheme("accessories-text-editor"), this);
    m_trayIcon->setToolTip("QuickNote");
    
    QMenu* trayMenu = new QMenu(this);
    
    // QuickNote-Button zum Einblenden
    QAction* showAction = trayMenu->addAction("QuickNote");
    connect(showAction, &QAction::triggered, this, [this]() {
        show();
        raise();
        activateWindow();
    });
    
    trayMenu->addSeparator();
    
    // Direkt ins Hauptmenü
    setupSettingsMenu(trayMenu);
    
    trayMenu->addSeparator();
    
    // Beenden-Option
    QAction* quitAction = trayMenu->addAction(Translations::get("quit"));
    connect(quitAction, &QAction::triggered, this, [this]() {
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            Translations::get("quit"),
            Translations::get("quit_confirm"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            QApplication::quit();
        }
    });
    
    m_trayIcon->setContextMenu(trayMenu);
    m_trayIcon->show();
}

void Editor::setupSettingsMenu(QMenu* settingsMenu)
{
    // Haupteinstellungen
    QAction *settingsAction = settingsMenu->addAction(Translations::get("settings"));
    connect(settingsAction, &QAction::triggered, this, [this]() {
        QDialog dialog(this);
        dialog.setWindowTitle(Translations::get("settings"));
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        
        // History-Länge
        QHBoxLayout *historyLayout = new QHBoxLayout();
        QLabel *historyLabel = new QLabel(Translations::get("max_history") + ":", &dialog);
        QSpinBox *historySpin = new QSpinBox(&dialog);
        historySpin->setRange(1, 99999);
        historySpin->setValue(m_maxHistorySize);
        historyLayout->addWidget(historyLabel);
        historyLayout->addWidget(historySpin);
        layout->addLayout(historyLayout);
        
        // Hintergrundfarbe
        QHBoxLayout *bgColorLayout = new QHBoxLayout();
        QPushButton *bgColorButton = new QPushButton(Translations::get("bg_color") + "...", &dialog);
        QFrame *bgColorPreview = new QFrame(&dialog);
        bgColorPreview->setAutoFillBackground(true);
        bgColorPreview->setFixedSize(20, 20);
        QPalette pal = bgColorPreview->palette();
        pal.setColor(QPalette::Window, m_backgroundColor);
        bgColorPreview->setPalette(pal);
        bgColorLayout->addWidget(bgColorButton);
        bgColorLayout->addWidget(bgColorPreview);
        layout->addLayout(bgColorLayout);
        
        connect(bgColorButton, &QPushButton::clicked, [this, bgColorPreview]() {
            QColor color = QColorDialog::getColor(m_backgroundColor, this, Translations::get("bg_color"));
            if (color.isValid()) {
                QPalette pal = bgColorPreview->palette();
                pal.setColor(QPalette::Window, color);
                bgColorPreview->setPalette(pal);
                m_backgroundColor = color;
            }
        });
        
        // Textfarbe
        QHBoxLayout *textColorLayout = new QHBoxLayout();
        QPushButton *textColorButton = new QPushButton(Translations::get("text_color") + "...", &dialog);
        QFrame *textColorPreview = new QFrame(&dialog);
        textColorPreview->setAutoFillBackground(true);
        textColorPreview->setFixedSize(20, 20);
        pal = textColorPreview->palette();
        pal.setColor(QPalette::Window, m_textColor);
        textColorPreview->setPalette(pal);
        textColorLayout->addWidget(textColorButton);
        textColorLayout->addWidget(textColorPreview);
        layout->addLayout(textColorLayout);
        
        connect(textColorButton, &QPushButton::clicked, [this, textColorPreview]() {
            QColor color = QColorDialog::getColor(m_textColor, this, Translations::get("text_color"));
            if (color.isValid()) {
                QPalette pal = textColorPreview->palette();
                pal.setColor(QPalette::Window, color);
                textColorPreview->setPalette(pal);
                m_textColor = color;
            }
        });
        
        // Shortcut
        QHBoxLayout *shortcutLayout = new QHBoxLayout();
        QLabel *shortcutLabel = new QLabel(Translations::get("shortcut") + ":", &dialog);
        QKeySequenceEdit *shortcutEdit = new QKeySequenceEdit(&dialog);
        shortcutEdit->setKeySequence(m_toggleWindowShortcut);
        QPushButton *clearButton = new QPushButton(Translations::get("default_shortcut"), &dialog);
        shortcutLayout->addWidget(shortcutLabel);
        shortcutLayout->addWidget(shortcutEdit);
        layout->addLayout(shortcutLayout);
        layout->addWidget(clearButton);
        
        connect(clearButton, &QPushButton::clicked, shortcutEdit, &QKeySequenceEdit::clear);
        
        // Sprachauswahl
        QHBoxLayout *langLayout = new QHBoxLayout();
        QLabel *langLabel = new QLabel(Translations::get("language") + ":", &dialog);
        QComboBox *langCombo = new QComboBox(&dialog);
        langCombo->addItem(Translations::get("english"), "en");
        langCombo->addItem(Translations::get("german"), "de");
        langCombo->addItem(Translations::get("french"), "fr");
        langCombo->addItem(Translations::get("spanish"), "es");
        langCombo->addItem(Translations::get("italian"), "it");
        langCombo->addItem(Translations::get("chinese"), "zh");
        langCombo->setCurrentText(m_language == "de" ? Translations::get("german") : m_language == "fr" ? Translations::get("french") : m_language == "es" ? Translations::get("spanish") : m_language == "it" ? Translations::get("italian") : m_language == "zh" ? Translations::get("chinese") : Translations::get("english"));
        langLayout->addWidget(langLabel);
        langLayout->addWidget(langCombo);
        layout->addLayout(langLayout);
        
        // OK/Cancel Buttons
        QDialogButtonBox *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        layout->addWidget(buttons);
        
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        
        if (dialog.exec() == QDialog::Accepted) {
            m_maxHistorySize = historySpin->value();
            m_toggleWindowShortcut = shortcutEdit->keySequence();
            applyColors();
            saveSettings();
            setupGlobalShortcut();
            
            // Sprache speichern
            QString newLang = langCombo->currentData().toString();
            if (newLang != m_language) {
                m_language = newLang;
                Translations::setLanguage(m_language);
                saveSettings();
                // Menüs neu aufbauen
                setupContextMenu();
                setupTrayIcon();
            }
        }
    });
    
    settingsMenu->addSeparator();
    
    // History löschen als separater Menüpunkt
    QAction *clearHistoryAction = settingsMenu->addAction(Translations::get("clear_history"));
    connect(clearHistoryAction, &QAction::triggered, this, [this]() {
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            Translations::get("clear_history"),
            Translations::get("clear_history_confirm"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            m_history = QJsonArray();
            m_currentHistoryIndex = -1;
            saveHistory();
        }
    });
}

void Editor::setupSingleInstance()
{
    // Prüfe auf andere Instanz
    QLocalSocket socket;
    socket.connectToServer("QuickNoteInstance");
    
    if (socket.waitForConnected(500)) {
        socket.close();
        m_dontSaveSettings = true;
        QTimer::singleShot(0, []() { QCoreApplication::exit(0); });
        return;
    }
    
    // Erstelle Server für diese Instanz
    m_localServer = new QLocalServer(this);
    QLocalServer::removeServer("QuickNoteInstance");
    if (!m_localServer->listen("QuickNoteInstance")) {
        qDebug() << "Server konnte nicht gestartet werden";
        return;
    }
    
    // Wenn sich eine neue Instanz verbindet, zeige das Fenster
    connect(m_localServer, &QLocalServer::newConnection, this, [this]() {
        show();
        raise();
        activateWindow();
    });
} 