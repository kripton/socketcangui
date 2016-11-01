/* written 2009, 2010 by Jannis Achstetter
 * contact: kripton@kripserver.net
 *
 * developed at Hochschule Aschaffenburg
 * licensed under the terms of the General Public
 * License (GPL) version 3.0
 */

// socketcangui is the mainwindow class of the socketcangui application

#include <QFile>

#include "socketcangui.h"
#include "canlogfile.h"
#include "canthread.h"

using namespace std;

/*!
 * Application startup; initialisation.
 * @param parent Parent for the main window
 */
socketcangui::socketcangui(QWidget *parent) :
        QMainWindow(parent) {

  // Best size depends on resolution, dpi and font size
  resize(775, 600);
  setWindowIcon(QIcon(":/icons/images/socketcangui.png"));

  // Instantiate the main object, the canlogfile
  myclf = new canlogfile;
  setCentralWidget(myclf);
  connect(myclf, SIGNAL(modified()), this, SLOT(fileModified()));

  // Initialize our canthread as beeing not active
  mycanthread.stop();

  // Instantiate the setup dialog but keep it hidden until needed
  setupdialog = new SetupDialog(this);
  setupdialog->hide();
  connect(setupdialog, SIGNAL(setfilter(QStringList)), &mycanthread, SLOT(setfilter(QStringList)));

  // Set up the main parts of the GUI
  createActions();
  createMenus();
  createToolBars();
  createStatusBar();
  createDockWidgets();

  // Clear the canlogfile and prepare the application to use is
  socketcangui::newFile();

  // Scan for network interfaces on the host
  updateinterfacelist();

  // Let the counter display zeros
  struct threadstatus tmpstat;
  bzero(&tmpstat, sizeof(tmpstat));
  updateStatus(tmpstat);

  // Inform the user
  statusBar->showMessage(tr("socketcangui is ready"), 2000);
}

/*!
 * Used when changing the language.
 * @param e Event that cause the function to be called
 */
void socketcangui::changeEvent(QEvent *e) {
  QMainWindow::changeEvent(e);
  switch (e->type()) {
  case QEvent::LanguageChange:
    setWindowTitle(QApplication::translate("socketcangui", "socketcangui", 0, QApplication::UnicodeUTF8));
    break;
  default:
    break;
  }
}

/*!
 * Application shall be terminated.
 * @param event Event that cause the function to be called
 */
void socketcangui::closeEvent(QCloseEvent *event) {
  statusBar->showMessage(tr("Exiting ..."), 2000);
  // Check whether the file has been modified and prompt the user if so
  if (okToContinue()) {
    // Stop the thread and wait for graceful exit
    mycanthread.stop();
    mycanthread.wait();
    event->accept();
  } else {
    event->ignore();
  }
}

/*!
 * Create a new, empty file.
 */
void socketcangui::newFile() {
  // Check whether the file has been modified and prompt the user if so
  if (okToContinue()) {
    myclf->clear();
    setCurrentFile("");
    statusBar->showMessage(tr("File cleared"), 2000);
  }
}

/*!
 * Open and load a file.
 */
void socketcangui::open() {
  // Check whether the file has been modified and prompt the user if so
  if (okToContinue()) {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open CAN logfile"), ".", tr("CAN logfiles (*.clf)"));
    if (!fileName.isEmpty()) loadFile(fileName);
  }
}

/*!
 * Save the current file under the same name
 * @return Success of the operation
 */
bool socketcangui::save() {
  if (curFile.isEmpty()) {
    return saveAs();
  } else {
    return saveFile(curFile);
  }
}

/*!
 * Save the current file under a new name
 * @return Success of the operation
 */
bool socketcangui::saveAs() {
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save CAN logfile"), ".", tr("CAN logfiles (*.clf)"));
  if (fileName.isEmpty()) return false;
  return saveFile(fileName);
}

/*!
 * Display the about dialog.
 */
void socketcangui::about() {
  QMessageBox::about(this, tr("About socketcangui"), tr(
      "<h2>socketcangui 1.0</h2>"
      "<p>written 2009/2010 by Jannis Achstetter</p>"
      "<p>developed at Hochschule Aschaffenburg</p>"
      "<p>licensed under the terms of the General Public License (GPL) version 3.0</p>"
      "<br /><p><a href=mailto:kripton@kripserver.net>kripton@kripserver.net</a></p>"));
}

/*!
 * Open a recent opened file again.
 */
void socketcangui::openRecentFile() {
  // Check whether the file has been modified and prompt the user if so
  if (okToContinue()) {
    QAction *action = qobject_cast<QAction *> (sender());
    if (action) loadFile(action->data().toString());
  }
}

/*!
 * Called whenever the file has been modified since loading.
 */
void socketcangui::fileModified() {
  setWindowModified(true);
}

/*!
 * Rescan for network interfaces.
 */
void socketcangui::updateinterfacelist () {
  statusBar->showMessage(tr("Network interface list is beeing updated ..."), 2000);

  // Open the file that has all the information
  QFile netdevfile("/proc/net/dev");
  QTextStream stream(&netdevfile);
  QString line;
  struct ifreq ifr;
  struct sockaddr_can addr;
  short i = 0;
  int tmpsock = 0;

  // Get all known network interfaces (from /proc/net/dev) and check
  // whether they are CAN devices and if they are up or not
  netdevfile.open(QIODevice::ReadOnly);
  ifacelist->clear();
  ifacelistitems.clear();
  ifacecombo->clear();
  do {
    line = stream.readLine();
    if (line.contains("|") || line.length() == 0) continue; //Header or empty line
    QStringList list = line.split(" ", QString::SkipEmptyParts);
    list = list.at(0).split(":");

    tmpsock = socket(AF_CAN, SOCK_RAW, CAN_RAW);
    memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
    strcpy(ifr.ifr_name, list.at(0).toAscii().constData());

    addr.can_family = AF_CAN;

    if (ioctl(tmpsock, SIOCGIFINDEX, &ifr) < 0) cerr << "INDEX ERROR WITH" << list.at(0).toAscii().constData() << endl; cerr.flush();
    addr.can_ifindex = ifr.ifr_ifindex;
    // If bind fails this means that the device cannot handle CAN frames
    if (bind(tmpsock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
#ifdef DEBUG
      cerr << list.at(0).toAscii().constData() << " is not a CAN device; skipping" << endl;
      cerr.flush();
#endif
      continue;
    }
    // Run another ioctl to see whether the interface is up or not
    if (ioctl(tmpsock, SIOCGIFFLAGS, &ifr) < 0) cerr << "FLAGS ERROR WITH" << list.at(0).toAscii().constData() << endl; cerr.flush();
    if (ifr.ifr_flags & IFF_UP) {
      ifacelistitems.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList() << list.at(0) << tr("up")));
      ifacelistitems.at(i)->setBackground(1, QBrush(LIGHTGREEN));
    } else {
      ifacelistitems.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList() << list.at(0) << tr("down")));
      ifacelistitems.at(i)->setBackground(1, QBrush(LIGHTRED));
    }
    ifacelist->addTopLevelItems(ifacelistitems);
    ifacecombo->addItem(list.at(0));
    i++;
  } while (!line.isNull());
  netdevfile.close();
#ifdef DEBUG
  cerr << "Updated interface list" << endl;
  cerr.flush();
#endif
  statusBar->showMessage(tr("Network interface list has been updated"), 2000);
}

/*!
 * Called when the list of network interfaces has been double-clicked.
 * Starts or stops the capture.
 * @param index Which index has been double-clicked on
 */
void socketcangui::ifacelistdclicked(const QModelIndex & index) {
#ifdef DEBUG
    cerr << "ifaceslist has been dclicked at row " << index.row() << "iface: " << ifacelistitems.at(index.row())->text(0).toLocal8Bit().data() << endl;
    cerr.flush();
#endif
  QString ifacename = ifacelistitems.at(index.row())->text(0);
  ifacecombo->setCurrentIndex(ifacecombo->findText(ifacename, Qt::MatchCaseSensitive));
  startorstopthread();
}

/*!
 * Refresh the thread's status in the GUI.
 * @param newstat The new status to be displayed in the GUI
 */
void socketcangui::updateStatus(struct threadstatus newstat) {
  statusincounter->setText(QString(tr("<table width=100%><tr><td>Packets in:</td><td align=right>%1</td></tr></table>")).arg(newstat.incounter));
  statusoutcounter->setText(QString(tr("<table width=100%><tr><td>Packets out:</td><td align=right>%1</td></tr></table>")).arg(newstat.outcounter));
  statusinbcounter->setText(QString(tr("<table width=100%><tr><td>Bytes in:</td><td align=right>%1</td></tr></table>")).arg(newstat.inbcounter));
  statusoutbcounter->setText(QString(tr("<table width=100%><tr><td>Bytes out:</td><td align=right>%1</td></tr></table>")).arg(newstat.outbcounter));
}

/*!
 * Called when the user changed a value in the sendtable.
 * @param item Which item has been changed
 * @param column Column that has been changed
 */
void socketcangui::sendtablechanged(QTreeWidgetItem * item, int column) {
  bool ok; // Used for QString.toXXX-conversions

#ifdef DEBUG
    cerr << "Item in send table has been changed! (col " << column << ")" << endl;
    cerr.flush();
#endif

  // if the timer has been running, stop it now so that we can change the parameters
  if (timerlist[item->text(0).toInt(&ok)]->isActive()) timerlist[item->text(0).toInt(&ok)]->stop();

  // Check every single value. If they are invalid, mark them red and
  // set the "Active" flag to "0"

  // check that the INTERVAL is okay
  if (item->text(1).toInt(&ok) > 0 && ok == TRUE && item->text(1).toInt(&ok) <= 3600000) {
#ifdef DEBUG
    cerr << "Interval is valid (" << item->text(1).toInt(&ok) << " ms)" << endl;
#endif
    item->setBackground(1, QBrush(TRANSPARENT));
    timerlist[item->text(0).toInt(&ok)]->setInterval(item->text(1).toInt(&ok));
  } else {
    item->setBackground(1, QBrush(LIGHTRED));
    item->setText(6, "0");
  }

  // check that the CAN ID is okay
  if (item->text(2).toULong(&ok, 16) <= 0x1FFFFFFF && ok == TRUE) {
#ifdef DEBUG
    cerr << "CAN ID is valid (" << item->text(2).toULong(&ok, 16) << ")" << endl;
#endif
    item->setBackground(2, QBrush(TRANSPARENT));
  } else {
    item->setBackground(2, QBrush(LIGHTRED));
    item->setText(6, "0");
  }

  // check that the EFF is okay
  if (item->text(3) == "0" || item->text(3) == "1") {
#ifdef DEBUG
    cerr << "EFF is valid (" << item->text(3).toAscii().constData() << ")" << endl;
#endif
    item->setBackground(3, QBrush(TRANSPARENT));
  } else {
    item->setBackground(3, QBrush(LIGHTRED));
    item->setText(6, "0");
  }

  // check that the DLC is okay
  if (item->text(4).toUInt(&ok) <= 8 && ok == TRUE) {
#ifdef DEBUG
    cerr << "DLC is valid (" << item->text(4).toULong(&ok) << ")" << endl;
#endif
    item->setBackground(4, QBrush(TRANSPARENT));
  } else {
    item->setBackground(4, QBrush(LIGHTRED));
    item->setText(6, "0");
  }

  // check that the DATA is okay
  if ((quint64)item->text(5).toULongLong(&ok, 16) <= (quint64)0xFFFFFFFFFFFFFFFF && ok == TRUE) {
#ifdef DEBUG
    cerr << "DATA is valid (" << item->text(5).toULongLong(&ok, 16) << ")" << endl;
#endif
    item->setBackground(5, QBrush(TRANSPARENT));
  } else {
    item->setBackground(5, QBrush(LIGHTRED));
    item->setText(6, "0");
  }

  if (item->text(6) == "0") {
    // Timer shall be stopped or any value is invalid
    timerlist[item->text(0).toInt(&ok)]->stop();
  } else if (item->text(6) == "1") {
    // Timer shall be started AND everything is valid!
    timerlist[item->text(0).toInt(&ok)]->start();
    statusBar->showMessage(tr("Send event updated"), 2000);
  }
#ifdef DEBUG
  cerr.flush();
#endif
}

// Really ugly but the timeout()-SIGNAL won't tell us which timer it was
void socketcangui::sendtimer0fired() {sendtimerfired(0);}
void socketcangui::sendtimer1fired() {sendtimerfired(1);}
void socketcangui::sendtimer2fired() {sendtimerfired(2);}
void socketcangui::sendtimer3fired() {sendtimerfired(3);}
void socketcangui::sendtimer4fired() {sendtimerfired(4);}
void socketcangui::sendtimer5fired() {sendtimerfired(5);}
void socketcangui::sendtimer6fired() {sendtimerfired(6);}
void socketcangui::sendtimer7fired() {sendtimerfired(7);}
void socketcangui::sendtimer8fired() {sendtimerfired(8);}
void socketcangui::sendtimer9fired() {sendtimerfired(9);}

/*!
 * To be called when a timer has fired, id as parameter.
 * @param id ID of the timer that has just fired
 */
void socketcangui::sendtimerfired(int id) {
  bool ok; // Used for QString.toXXX-conversions
  struct canpacket sendpacket;
  qulonglong mydata;

#ifdef DEBUG
  cerr << "Timer id " << id << " fired!" << endl;
  cerr.flush();
#endif

  // Better safe than sorry
  bzero(&sendpacket, sizeof(sendpacket));

  // Construct the packet to be sent from the values in the sendtable
  sendpacket.identifier = timerdisplaylist.at(id)->text(2).toULong(&ok, 16);
  if (timerdisplaylist.at(id)->text(3) == "1") {
    sendpacket.ide = true;
  }
  sendpacket.dlc = timerdisplaylist.at(id)->text(4).toULong(&ok, 16);
  mydata = timerdisplaylist.at(id)->text(5).toULongLong(&ok, 16);
  memcpy(sendpacket.data, &mydata, 8);

  // Let the canthread send the packet
  mycanthread.sendmsg(sendpacket);
}

/*!
 * Start or stop a CAN interface-thread.
 */
void socketcangui::startorstopthread() {
  QString ifacename = ifacecombo->currentText();
#ifdef DEBUG
  cerr << "button clicked " << mycanthread.isRunning() << " iface: " << ifacename.toLocal8Bit().data() << endl;
  cerr.flush();
#endif
  if (mycanthread.isRunning()) {
    // Show the user that we will wait for the thread to finish gracefully
    QApplication::setOverrideCursor(Qt::WaitCursor);
#ifdef DEBUG
    cerr << "Exiting thread" << endl;
    cerr.flush();
#endif
    mycanthread.stop();
#ifdef DEBUG
    cerr << "Waiting for thread to finish" << endl;
    cerr.flush();
#endif
    mycanthread.wait();
#ifdef DEBUG
    cerr << "Thread has finished" << endl;
    cerr.flush();
#endif
    capturepb->setText(tr("Start"));
    statusdisplaylabel->setText(tr("Idle"));
    statusdisplaylabel->setStyleSheet(HTMLLIGHTRED);
    statusBar->showMessage(tr("CAN thread stopped"), 2000);
    QApplication::restoreOverrideCursor();
  } else {
    mycanthread.setifname(ifacename);
    mycanthread.start();
    capturepb->setText(tr("Stop"));
    statusdisplaylabel->setText(tr("Running"));
    statusdisplaylabel->setStyleSheet(HTMLLIGHTGREEN);
    statusBar->showMessage(tr("CAN thread started"), 2000);
  }
}

/*!
 * INIT: create (menu) actions.
 */
void socketcangui::createActions() {
  newAction = new QAction(tr("&New"), this);
  newAction->setIcon(QIcon(":/icons/images/document-new.png"));
  newAction->setShortcut(tr("Ctrl+N"));
  newAction->setStatusTip(tr("Create a new CAN logfile"));
  connect(newAction, SIGNAL(triggered()), this, SLOT(newFile()));

  openAction = new QAction(tr("&Open..."), this);
  openAction->setIcon(QIcon(":/icons/images/document-open.png"));
  openAction->setShortcut(tr("Ctrl+O"));
  openAction->setStatusTip(tr("Open an existing CAN logfile"));
  connect(openAction, SIGNAL(triggered()), this, SLOT(open()));

  saveAction = new QAction(tr("&Save"), this);
  saveAction->setIcon(QIcon(":/icons/images/document-save.png"));
  saveAction->setShortcut(tr("Ctrl+S"));
  saveAction->setStatusTip(tr("Save the CAN logfile"));
  connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));

  saveAsAction = new QAction(tr("Save &As..."), this);
  saveAsAction->setIcon(QIcon(":/icons/images/document-save-as.png"));
  saveAsAction->setStatusTip(tr("Save the CAN logfile under a new name"));
  connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs()));

  for (int i = 0; i < MaxRecentFiles; ++i) {
    recentFileActions[i] = new QAction(this);
    recentFileActions[i]->setVisible(false);
    connect(recentFileActions[i], SIGNAL(triggered()), this,
        SLOT(openRecentFile()));
  }

  exitAction = new QAction(tr("E&xit"), this);
  exitAction->setIcon(QIcon(":/icons/images/application-exit.png"));
  exitAction->setShortcut(tr("Ctrl+X"));
  exitAction->setStatusTip(tr("Exit the application"));
  connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

  aboutAction = new QAction(tr("&About"), this);
  aboutAction->setIcon(QIcon(":/icons/images/socketcangui.png"));
  aboutAction->setStatusTip(tr("Show the application's About box"));
  connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

  aboutQtAction = new QAction(tr("About &Qt"), this);
  aboutQtAction->setIcon(QIcon(":/icons/images/qt-logo.png"));
  aboutQtAction->setStatusTip(tr("Show the Qt library's About box"));
  connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

  setupAction = new QAction(tr("Setup"), this);
  setupAction->setIcon(QIcon(":/icons/images/configure.png"));
  setupAction->setStatusTip(tr("Setup CAN filters and bitrate for PEAK adapters"));
  connect(setupAction, SIGNAL(triggered()), setupdialog, SLOT(show()));
}

/*!
 * INIT: create menus.
 */
void socketcangui::createMenus() {
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(newAction);
  fileMenu->addAction(openAction);
  fileMenu->addAction(saveAction);
  fileMenu->addAction(saveAsAction);
  separatorAction = fileMenu->addSeparator();
  for (int i = 0; i < MaxRecentFiles; ++i)
    fileMenu->addAction(recentFileActions[i]);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAction);

  optionsMenu = menuBar()->addMenu(tr("&Options"));
  optionsMenu->addAction(setupAction);

  menuBar()->addSeparator();

  helpMenu = menuBar()->addMenu(tr("&Help"));
  helpMenu->addAction(aboutAction);
  helpMenu->addAction(aboutQtAction);
}

/*!
 * INIT: create toolbars.
 */
void socketcangui::createToolBars() {
  fileToolBar = addToolBar(tr("&File"));
  fileToolBar->addAction(newAction);
  fileToolBar->addAction(openAction);
  fileToolBar->addAction(saveAction);
  otherToolBar = addToolBar(tr("&Tools"));
  otherToolBar->addAction(setupAction);
  otherToolBar->addAction(exitAction);
}

/*!
 * INIT: create status bar.
 */
void socketcangui::createStatusBar() {
  statusBar = new QStatusBar(this);
  statusBar->setObjectName("statusBar");
  statusBar->setEnabled(true);
  setStatusBar(statusBar);
}

/*!
 * INIT: create dock widgets.
 */
void socketcangui::createDockWidgets() {
  // Send widget (table; normally in the lower part)
  QWidget *sendwidget = new QWidget;
  QHBoxLayout *sendwidgetLayout = new QHBoxLayout;
  sendtable = new QTreeWidget;
  sendtable->setColumnCount(6);
  sendtable->setAlternatingRowColors(true);
  sendtable->setHeaderLabels(QStringList() << tr("#") << tr("Interval (ms)") << tr("CAN ID (hex)") << tr("EFF") << tr("DLC") << tr("Data (hex)") << tr("Active"));
  timerdisplaylist.clear();
  QString tmpstring;
  // insert ten send rows
  for (int i = 0; i < 10; i++) {
    tmpstring = QString::number(i);
    timerdisplaylist.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList() << tmpstring << "0" << "0" << "0" << "0" << "0" << "0"));
    // make the row editable
    timerdisplaylist.at(i)->setFlags(timerdisplaylist.at(i)->flags() | Qt::ItemIsEditable);
  }
  sendtable->addTopLevelItems(timerdisplaylist);
  // create the ten send timers
  for (int i = 0; i < 10; i++) {
    timerlist[i] = new QTimer;
  }
  // Really ugly but the timeout()-SIGNAL won't tell us which timer it was
  connect(timerlist[0], SIGNAL(timeout()), this, SLOT(sendtimer0fired()));
  connect(timerlist[1], SIGNAL(timeout()), this, SLOT(sendtimer1fired()));
  connect(timerlist[2], SIGNAL(timeout()), this, SLOT(sendtimer2fired()));
  connect(timerlist[3], SIGNAL(timeout()), this, SLOT(sendtimer3fired()));
  connect(timerlist[4], SIGNAL(timeout()), this, SLOT(sendtimer4fired()));
  connect(timerlist[5], SIGNAL(timeout()), this, SLOT(sendtimer5fired()));
  connect(timerlist[6], SIGNAL(timeout()), this, SLOT(sendtimer6fired()));
  connect(timerlist[7], SIGNAL(timeout()), this, SLOT(sendtimer7fired()));
  connect(timerlist[8], SIGNAL(timeout()), this, SLOT(sendtimer8fired()));
  connect(timerlist[9], SIGNAL(timeout()), this, SLOT(sendtimer9fired()));
  connect(sendtable, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(sendtablechanged(QTreeWidgetItem *, int)));

  sendwidgetLayout->addWidget(sendtable);
  sendwidget->setMinimumHeight(100);
  sendwidget->setLayout(sendwidgetLayout);
  QDockWidget *sendwidgetDock = new QDockWidget(tr("Send packets"));
  sendwidgetDock->setWidget(sendwidget);
  addDockWidget(Qt::BottomDockWidgetArea, sendwidgetDock);
  // make the dock widget non-closeable
  sendwidgetDock->setFeatures(sendwidgetDock->features() & ~QDockWidget::DockWidgetClosable);

  // Control widget (normally on the right side)
  QWidget *controlwidget = new QWidget;
  QVBoxLayout *controlwidgetLayout = new QVBoxLayout;
  controlwidget->setLayout(controlwidgetLayout);
  QDockWidget *controlwidgetDock = new QDockWidget(tr("Capture control"));
  controlwidgetDock->setWidget(controlwidget);
  addDockWidget(Qt::RightDockWidgetArea, controlwidgetDock);
  // make the dock widget non-closeable
  controlwidgetDock->setFeatures(controlwidgetDock->features() & ~QDockWidget::DockWidgetClosable);

  QLabel *ifacelabel = new QLabel(tr("CAN Interfaces:"));
  QPushButton *updateifaces = new QPushButton(tr("Update"));
  QHBoxLayout *ifacesheader = new QHBoxLayout;
  ifacesheader->addWidget(ifacelabel);
  ifacesheader->addWidget(updateifaces);
  controlwidgetLayout->addLayout(ifacesheader);
  ifacelist = new QTreeWidget;
  ifacelist->setColumnCount(2);
  ifacelist->setHeaderLabels(QStringList() << tr("Interface") << tr("Status"));
  controlwidgetLayout->addWidget(ifacelist);
  connect(updateifaces, SIGNAL(clicked()), this, SLOT(updateinterfacelist()));
  connect(ifacelist, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(ifacelistdclicked(QModelIndex)));

  QHBoxLayout *capturelayout = new QHBoxLayout;
  controlwidgetLayout->addLayout(capturelayout);
  QLabel *capturelabel = new QLabel(tr("Capture Interface:"));
  capturelayout->addWidget(capturelabel);

  ifacecombo = new QComboBox;
  capturelayout->addWidget(ifacecombo);
  capturepb = new QPushButton(tr("Start"));
  capturelayout->addWidget(capturepb);

  // Tell Qt that canpacket can be used with SLOTs and SIGNALs
  qRegisterMetaType<canpacket>("canpacket");
  connect(capturepb, SIGNAL(clicked()), this, SLOT(startorstopthread()));
  connect(&mycanthread, SIGNAL(dataarrived(canpacket)), myclf, SLOT(adddataitem(canpacket)));

  // Tell Qt that threadstatus can be used with SLOTs and SIGNALs
  qRegisterMetaType<threadstatus>("threadstatus");
  connect(&mycanthread, SIGNAL(statusChanged(threadstatus)), this, SLOT(updateStatus(threadstatus)));

  // Status widget (normally on the right side)
  QWidget *statuswidget = new QWidget;
  QVBoxLayout *statuswidgetLayout = new QVBoxLayout;
  statuswidget->setLayout(statuswidgetLayout);
  statuswidgetDock = new QDockWidget(tr("Capture status"));
  statuswidgetDock->setWidget(statuswidget);
  addDockWidget(Qt::RightDockWidgetArea, statuswidgetDock);
  // make the dock widget non-closeable
  statuswidgetDock->setFeatures(statuswidgetDock->features() & ~QDockWidget::DockWidgetClosable);

  statusdisplaylabel = new QLabel(tr("Idle"));
  statusdisplaylabel->setStyleSheet(HTMLLIGHTRED);
  statusdisplaylabel->setFrameShape(QFrame::Panel);
  statusdisplaylabel->setFrameShadow(QLabel::Sunken);
  statusdisplaylabel->setAlignment(Qt::AlignHCenter);
  QHBoxLayout *statusdisplaylayout = new QHBoxLayout;
  statusdisplaylayout->addWidget(statusdisplaylabel);
  statuswidgetLayout->addLayout(statusdisplaylayout);

  statusincounter = new QLabel("");
  statuswidgetLayout->addWidget(statusincounter);
  statusoutcounter = new QLabel("");
  statuswidgetLayout->addWidget(statusoutcounter);
  statusinbcounter = new QLabel("");
  statuswidgetLayout->addWidget(statusinbcounter);
  statusoutbcounter = new QLabel("");
  statuswidgetLayout->addWidget(statusoutbcounter);
}

/*!
 * Ask user to save file if modified.
 * @return True of the file can be cleared now, false otherwise
 */
bool socketcangui::okToContinue() {
  if (isWindowModified()) {
    int r = QMessageBox::warning(this, tr("socketcangui"),
        tr("The document has been modified.\n"
        "Do you want to save your changes?"), QMessageBox::Yes
        | QMessageBox::Default, QMessageBox::No, QMessageBox::Cancel
        | QMessageBox::Escape);
    if (r == QMessageBox::Yes) {
      return save();
    } else if (r == QMessageBox::Cancel) {
      return false;
    }
  }
  return true;
}

/*!
 * Tell canlogfile to load a file.
 * @param file Name name of the file to be loaded
 * @return True if the file has been loaded, false otherwise
 */
bool socketcangui::loadFile(const QString &fileName) {
  if (!myclf->readFile(fileName)) {
    statusBar->showMessage(tr("Loading canceled"), 2000);
    return false;
  }
  setCurrentFile(fileName);
  statusBar->showMessage(tr("File loaded"), 2000);
  return true;
}

/*!
 * Tell canlogfile to write the file.
 * @param fileName Name of the file to be written to
 * @return True if the file has been saved, false otherwise
 */
bool socketcangui::saveFile(const QString &fileName) {
  if (!myclf->writeFile(fileName)) {
    statusBar->showMessage(tr("Saving canceled"), 2000);
    return false;
  }
  setCurrentFile(fileName);
  statusBar->showMessage(tr("File saved"), 2000);
  return true;
}

/*!
 * Set the current file name.
 * @param fileName Name of the file in use now
 */
void socketcangui::setCurrentFile(const QString &fileName) {
  curFile = fileName;
  setWindowModified(false);

  QString shownName = tr("Untitled");
  if (!curFile.isEmpty()) {
    shownName = strippedName(curFile);
    recentFiles.removeAll(curFile);
    recentFiles.prepend(curFile);
    updateRecentFileActions();
  }

  setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("socketcangui")));
}

/*!
 * Update the recent file list.
 */
void socketcangui::updateRecentFileActions() {
  QMutableStringListIterator i(recentFiles);
  while (i.hasNext()) {
    if (!QFile::exists(i.next()))
      i.remove();
  }

  for (int j = 0; j < MaxRecentFiles; ++j) {
    if (j < recentFiles.count()) {
      QString text =
          tr("&%1 %2") .arg(j + 1) .arg(strippedName(recentFiles[j]));
      recentFileActions[j]->setText(text);
      recentFileActions[j]->setData(recentFiles[j]);
      recentFileActions[j]->setVisible(true);
    } else {
      recentFileActions[j]->setVisible(false);
    }
  }
  separatorAction->setVisible(!recentFiles.isEmpty());
}

/*!
 * Short file name (without leading path name).
 * @param fullFileName Name of the file to be shortened
 * @return Short file name (without leading path name)
 */
QString socketcangui::strippedName(const QString &fullFileName) {
  return QFileInfo(fullFileName).fileName();
}
