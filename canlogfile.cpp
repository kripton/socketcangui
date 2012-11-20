/* written 2009, 2010 by Jannis Achstetter
 * contact: s25581@h-ab.de
 *
 * developed at Hochschule Aschaffenburg
 * licensed under the terms of the General Public
 * License (GPL) version 3.0
 */

#include "canlogfile.h"

#include <iostream>

using namespace std;

/*!
 * Constructor that initialises the model and sets the headers.
 * @param parent Parent of the canlogfile
 */
canlogfile::canlogfile(QTreeView *parent) : QTreeView(parent) {
  QStandardItemModel *mymodel = new QStandardItemModel(0, 10, parent);
  mymodel->setHorizontalHeaderLabels(QStringList() << tr("#") << tr("Timestamp") << tr("Interface") << tr("Dir") << tr("RTR") << tr("EFF") << tr("ERR") << tr("CAN ID") << tr("DLC") << tr("Data") << tr("CRC"));

  // Somehow, setModel() does not what I think it should, we need the assignment for it to work
  setModel(mymodel);
  model = mymodel;

  // Hide yet unused columns
  setColumnHidden(2, 1);  // Interface
  setColumnHidden(10, 1); // CRC

  // Make the display tree view uneditable
  setEditTriggers(QAbstractItemView::NoEditTriggers);

  // Clear the entries to resize the columns correctly
  clear();
}

/*!
 * Returns the number of canpackets in the file.
 * @return number of canpackets in the file
 */
int canlogfile::getdataitemcount() {
  return model->rowCount();
}

/*!
 * Deletes all canpackets from the file
 */
void canlogfile::clear() {
  // May take some time, so let the user know we are working
  QApplication::setOverrideCursor(Qt::WaitCursor);

  // This takes extremely long with many elements :/
  model->removeRows(0, model->rowCount());

  // Insert one row with maximum data content and resize them
  // to fit all the data in. Then, delete the row again
  model->insertRow(0);
  model->setData(model->index(0, 0), "888888 ");
  model->setData(model->index(0, 1), QDateTime(QDate(2888, 12, 22), QTime(18, 58, 58, 888)).toString("dd.MM.yyyy hh:mm:ss.zzz"));
  model->setData(model->index(0, 2), "can88 ");
  model->setData(model->index(0, 3), "<");
  model->setData(model->index(0, 4), "0");
  model->setData(model->index(0, 5), "0");
  model->setData(model->index(0, 6), "0");
  model->setData(model->index(0, 7), "ABCDEFGHI ");
  model->setData(model->index(0, 8), "8 ");
  model->setData(model->index(0, 9), "BB BB BB BB BB BB BB BB ");
  model->setData(model->index(0, 10), "0000 ");
  for (int i = 0; i <= model->columnCount(); i++) {
    resizeColumnToContents(i);
  }
  setSortingEnabled(true);
  setAlternatingRowColors(true);
  model->removeRows(0, model->rowCount());

  QApplication::restoreOverrideCursor();
}

/*!
 * Reads a file containing canpackets.
 * @param fileName Name of the file to be read
 * @return success of the operation
 */
bool canlogfile::readFile(const QString &fileName) {
  // Open the file
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, tr("socketcangui"), tr("Cannot read file %1:\n%2.").arg(file.fileName()).arg(file.errorString()));
    return false;
  }

  // Create the stream to handle the data
  QDataStream in(&file);
  in.setVersion(QDataStream::Qt_4_4);

  // Check the file magic (first 4 bytes)
  quint32 magic;
  in >> magic;
  if (magic != MagicNumber) {
    QMessageBox::warning(this, tr("socketcangui"), tr("This is not a CAN logfile or the version does not match!"));
    return false;
  }

  // Start with no data
  clear();

  qint64 row;
  qint64 column;
  QString str;
  qint64 oldrow = 999;

  // Now read the file and insert line by line
  QApplication::setOverrideCursor(Qt::WaitCursor);
  while (!in.atEnd()) {
    in >> row >> column >> str;
    if (row != oldrow) {
      model->insertRow(model->rowCount());
      oldrow = row;
    }
    model->setData(model->index(row, column), str);
#ifdef DEBUG
    cerr << "Inserting into ROW" << row << " COL " << column << endl;
    cerr.flush();
#endif
  }

  QApplication::restoreOverrideCursor();
  return true;
}

/*!
 * Writes all canpackets to a file.
 * @param fileName Name of the file to be written to
 * @return success of the operation
 */
bool canlogfile::writeFile(const QString &fileName) {
  // Check wether the file name is ok
  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly)) {
    QMessageBox::warning(this, tr("socketcangui"), tr("Cannot write file %1:\n%2.").arg(file.fileName()).arg(file.errorString()));
    return false;
  }

  // Create the stream to handle the data
  QDataStream out(&file);
  out.setVersion(QDataStream::Qt_4_4);

  // Write the file magic (first 4 bytes)
  out << quint32(MagicNumber);

  // Now write the data to the file
  QApplication::setOverrideCursor(Qt::WaitCursor);
  for (qint64 row = 0; row < model->rowCount(); ++row) {
    for (qint64 column = 0; column < model->columnCount(); ++column) {
      QString str = model->data(model->index(row, column)).toString();
      if (!str.isEmpty())
        out << qint64(row) << qint64(column) << str;
    }
  }
  QApplication::restoreOverrideCursor();
  return true;
}

/*!
 * Adds one canpacket to the bottom of the file.
 * @param packet Packet to be added
 */
void canlogfile::adddataitem(canpacket packet) {
  int msec = 0;

  // calculate the timestamp when the packet arrived
  QDateTime timestamp = QDateTime::fromTime_t(packet.tv.tv_sec);
  msec = packet.tv.tv_usec / 1000;
#ifdef DEBUG
    cerr << "usec: " << packet.tv.tv_usec << "msec: " << msec << endl;
    cerr.flush();
#endif
  timestamp = timestamp.addMSecs(msec);

  // construct the data string as hex-values-string
  QString datadisplay;
  QChar dataelem;
  for (int i = 0; i < packet.dlc; i++) {
    dataelem = dataelem.fromAscii(packet.data[i]);
    datadisplay = datadisplay.append(QString("%1 ").arg((short)packet.data[i], 2, 16, QChar('0')));
  }

  // append the data packet
  int rownum = model->rowCount();
  model->insertRow(rownum);
  model->setData(model->index(rownum, 0), rownum);
  model->setData(model->index(rownum, 1), timestamp.toString(tr("dd.MM.yyyy hh:mm:ss.zzz")));
  model->setData(model->index(rownum, 2), 0);
  model->setData(model->index(rownum, 3), packet.direction ? tr("<") : tr(">"));
  model->setData(model->index(rownum, 4), packet.rtr ? tr("1") : tr("0"));
  model->setData(model->index(rownum, 5), packet.ide ? tr("1") : tr("0"));
  model->setData(model->index(rownum, 6), packet.err ? tr("1") : tr("0"));
  model->setData(model->index(rownum, 7), packet.identifier);
  model->setData(model->index(rownum, 8), packet.dlc);
  model->setData(model->index(rownum, 9), datadisplay);
  model->setData(model->index(rownum, 10), 0); // CRC
  scrollToBottom();

  somethingChanged();
}

/*!
 * SLOT to be called when an item changed or has been added.
 */
void canlogfile::somethingChanged() {
  emit modified();
}
