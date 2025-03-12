#include <QApplication>
#include "editor.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    Editor editor;
    editor.hide();  // Verstecke das Fenster direkt nach der Erstellung
    
    return app.exec();
} 