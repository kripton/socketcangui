/* written 2009, 2010 by Jannis Achstetter
 * contact: kripton@kripserver.net
 *
 * developed at Hochschule Aschaffenburg
 * licensed under the terms of the General Public
 * License (GPL) version 3.0
 */

#ifndef SOCKETCANGUI_H
#define SOCKETCANGUI_H

#include <QtGui>

// Things needed to scan for interfaces and get their type and status
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <iostream>

#include "canthread.h"
#include "setupdialog.h"

// Color definitions for the user interface
#define LIGHTRED        QColor(255, 192, 192)           //!< Color used for tree view items
#define LIGHTGREEN      QColor(192, 255, 192)           //!< Color used for tree view items
#define TRANSPARENT     QColor(255, 255, 255, 0)        //!< Color used for tree view items
#define HTMLLIGHTRED    "QLabel {background: #FFC0C0}"  //!< Color used for label elements
#define HTMLLIGHTGREEN  "QLabel {background: #C0FFC0}"  //!< Color used for label elements

class canlogfile;

/*!
 * Main class of the software keeping all the GUI stuff together and hosting the canlogiles
 */
class socketcangui: public QMainWindow {
Q_OBJECT

public:
  socketcangui(QWidget *parent = 0);    //!< Application startup; initialisation

protected:
  void changeEvent(QEvent *e);          //!< Used when changing the language
  void closeEvent(QCloseEvent *event);  //!< Application shall be terminated

private slots:
  void newFile();                       //!< Create a new, empty file
  void open();                          //!< Open and load a file
  bool save();                          //!< Save the current file under the same name
  bool saveAs();                        //!< Save the current file under a new name
  void about();                         //!< Display the about dialog
  void openRecentFile();                //!< Open a recent opened file again
  void fileModified();                  //!< Called whenever the file has been modified since loading
  void updateinterfacelist();           //!< Rescan for network interfaces
  void ifacelistdclicked(const QModelIndex & index);  //!< Called when the list of network interfaces has been double-clicked
  void updateStatus(struct threadstatus newstat); //!< Refresh the thread's status in the GUI
  void sendtablechanged(QTreeWidgetItem * item, int column);  //!< Called when the user changed a value in the sendtable
  // Really ugly but the timeout()-SIGNAL won't tell us which timer it was
  void sendtimer0fired();               //!< to be called when timer0 has fired
  void sendtimer1fired();               //!< to be called when timer1 has fired
  void sendtimer2fired();               //!< to be called when timer2 has fired
  void sendtimer3fired();               //!< to be called when timer3 has fired
  void sendtimer4fired();               //!< to be called when timer4 has fired
  void sendtimer5fired();               //!< to be called when timer5 has fired
  void sendtimer6fired();               //!< to be called when timer6 has fired
  void sendtimer7fired();               //!< to be called when timer7 has fired
  void sendtimer8fired();               //!< to be called when timer8 has fired
  void sendtimer9fired();               //!< to be called when timer9 has fired
  void sendtimerfired(int id);          //!< To be called when a timer has fired, id as parameter
  void startorstopthread();             //!< Start or stop a CAN interface-thread

private:
  QTreeWidget *ifacelist;               //!< widget to display network interfaces
  QList<QTreeWidgetItem *> ifacelistitems;  //!< list of network interface items
  QComboBox *ifacecombo;                //!< combobox that displays an network interface
  QPushButton *capturepb;               //!< Button to start or stop a CAN thread

  QDockWidget *statuswidgetDock;        //!< Dock item displaying the status
  QLabel *statusdisplaylabel;           //!< Showing "Running" or "Idle"
  QLabel *statusincounter;              //!< Counter display packets in
  QLabel *statusoutcounter;             //!< Counter display packets out
  QLabel *statusinbcounter;             //!< Counter display bytes in
  QLabel *statusoutbcounter;            //!< Counter display bytes out

  QTreeWidget *sendtable;               //!< Widget to show the 10 send timers
  QList<QTreeWidgetItem *> timerdisplaylist;  //!< List with send timer values
  QTimer *timerlist[10];                //!< Our 10 send timers

  void createActions();                 //!< INIT: create (menu) actions
  void createMenus();                   //!< INIT: create menus
  void createToolBars();                //!< INIT: create toolbars
  void createStatusBar();               //!< INIT: create status bar
  void createDockWidgets();             //!< INIT: create dock widgets
  bool okToContinue();                  //!< Ask user to save file if modified
  bool loadFile(const QString &fileName); //!< Tell canlogfile to load a file
  bool saveFile(const QString &fileName); //!< Tell canlogfile to write the file
  void setCurrentFile(const QString &fileName); //!< Set the current file name
  void updateRecentFileActions();       //!< Update the recent file list
  QString strippedName(const QString &fullFileName);  //!< Short file name (without leading path name)

  canthread mycanthread;                //!< The canthread that does the work for us

  canlogfile *myclf;                    //!< Logfile currently open
  QStringList recentFiles;              //!< List of recently opened files
  QString curFile;                      //!< Currently opened file name

  enum {MaxRecentFiles = 5 /*!< files to be shown in the open recent list */ };
  QAction *recentFileActions[MaxRecentFiles]; //!< actions what to do with the recent files
  QAction *separatorAction;             //!< simple menu seperator

  QMenu *fileMenu;                      //!< top level menu: file
  QMenu *optionsMenu;                   //!< top level menu: options (not used yet)
  QMenu *helpMenu;                      //!< top level menu: help
  QToolBar *fileToolBar;                //!< file tool bar
  QToolBar *otherToolBar;               //!< other tool bar (setup and exit)
  QStatusBar *statusBar;                //!< status bar

  QAction *newAction;                   //!< action: new file
  QAction *openAction;                  //!< action: open file
  QAction *saveAction;                  //!< action: save file
  QAction *saveAsAction;                //!< action: save file as
  QAction *exitAction;                  //!< action: exit program
  QAction *aboutAction;                 //!< action: about dialog
  QAction *aboutQtAction;               //!< action: about Qt
  QAction *setupAction;                 //!< action: setup dialog

  SetupDialog *setupdialog;             //!< instance of the setup dialog we use
};

#endif // SOCKETCANGUI_H
