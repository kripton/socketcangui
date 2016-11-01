/* written 2009, 2010 by Jannis Achstetter
 * contact: kripton@kripserver.net
 *
 * developed at Hochschule Aschaffenburg
 * licensed under the terms of the General Public
 * License (GPL) version 3.0
 */

#include <QtGui/QApplication>

#include "socketcangui.h"

/*!
 * Function to create an instance of the socketcangui-class and run it.
 * @param argc Command line parameter count
 * @param argv Vector to the command line arguments
 * @return Returncode of the QApplication
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    socketcangui w;
    w.show();
    return a.exec();
}
