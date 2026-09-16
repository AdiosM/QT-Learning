#include "mainwindow.h"
#include"LoginDialog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    LoginDialog loginDialog;
    if(loginDialog.exec()!=QDialog::Accepted)
    {
        return 0;
    }

    MainWindow window;
    window.show();
    return QApplication::exec();
}
