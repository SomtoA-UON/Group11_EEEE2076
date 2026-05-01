#include "mainwindow.h"

#include <QApplication>
//main.cpp
/**@file
* This file initialises the Qt app
*/
/** main function
* Initialises the Qt app, creates the main window and displays it. Then it starts the event loop to run the program.
*@param argc is the number of Arguments passed to the program
*@param argv is the argument vector, it is an array of strings containing the actual arguments.
*@return a.exec starts the event loop, the integer 0 will be returned assuming the project terminates as expected.
*/
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
