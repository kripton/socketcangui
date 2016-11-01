/* written 2009, 2010 by Jannis Achstetter
 * contact: kripton@kripserver.net
 *
 * developed at Hochschule Aschaffenburg
 * licensed under the terms of the General Public
 * License (GPL) version 3.0
 */

#include "setupdialog.h"

using namespace std;

/*!
 * Initialise the dialog.
 * @param parent Parent for the dialog window
 */
SetupDialog::SetupDialog(QWidget *parent)
    : QDialog(parent)
{
  // Main Layout of the dialog with two columns
  QVBoxLayout *mainlayout = new QVBoxLayout;
  QHBoxLayout *listlayout = new QHBoxLayout;
  QGroupBox *filterlist = new QGroupBox;
  QGroupBox *peakbitrate = new QGroupBox;
  QVBoxLayout *filterlayout = new QVBoxLayout;
  QVBoxLayout *peaklayout = new QVBoxLayout;
  filterlist->setLayout(filterlayout);
  peakbitrate->setLayout(peaklayout);
  filterlist->setTitle(tr("CAN hardware filters"));
  peakbitrate->setTitle(tr("Set bitrate for PEAK adapters"));
  listlayout->addWidget(filterlist);
  listlayout->addWidget(peakbitrate);
  QPushButton *closebutton = new QPushButton(tr("Close"));
  mainlayout->addLayout(listlayout);
  mainlayout->addWidget(closebutton);
  connect(closebutton, SIGNAL(clicked()), this, SLOT(accept()));

  // Layout for the "PEAK adapter bitrate"-thingy
  QLabel *ifacelabel = new QLabel(tr("PEAK adapters:"));
  QPushButton *updateifaces = new QPushButton(tr("Update"));
  QHBoxLayout *ifacesheader = new QHBoxLayout;
  ifacesheader->addWidget(ifacelabel);
  ifacesheader->addWidget(updateifaces);
  peaklayout->addLayout(ifacesheader);
  ifacelist = new QTreeWidget;
  ifacelist->setColumnCount(3);
  ifacelist->setHeaderLabels(QStringList() << "#" << "Type" << "Iface name" << "Bitrate");
  peaklayout->addWidget(ifacelist);
  QLabel *bitratelabel = new QLabel(tr("Bitrate:"));
  bitratecombo = new QComboBox;
  QPushButton *bitrateset = new QPushButton(tr("Set"));
  QHBoxLayout *bitratelayout = new QHBoxLayout;
  bitratelayout->addWidget(bitratelabel);
  bitratelayout->addWidget(bitratecombo);
  bitratelayout->addWidget(bitrateset);
  peaklayout->addLayout(bitratelayout);
  connect(updateifaces, SIGNAL(clicked()), this, SLOT(updatepeaklist()));
  connect(bitrateset, SIGNAL(clicked()), this, SLOT(setbitrate()));

  bitratecombo->addItem(tr("5 kBit/s"));
  bitratecombo->addItem(tr("10 kBit/s"));
  bitratecombo->addItem(tr("20 kBit/s"));
  bitratecombo->addItem(tr("33.33 kbit/s"));
  bitratecombo->addItem(tr("50 kBit/s"));
  bitratecombo->addItem(tr("95.2 kbit/s"));
  bitratecombo->addItem(tr("100 kBit/s"));
  bitratecombo->addItem(tr("125 kBit/s"));
  bitratecombo->addItem(tr("250 kBit/s"));
  bitratecombo->addItem(tr("500 kBit/s"));
  bitratecombo->addItem(tr("1 Mbit/s"));

  bitratearray[0] = CAN_BAUD_5K;
  bitratearray[1] = CAN_BAUD_10K;
  bitratearray[2] = CAN_BAUD_20K;
  bitratearray[3] = CAN_BAUD_33K;
  bitratearray[4] = CAN_BAUD_50K;
  bitratearray[5] = CAN_BAUD_95K;
  bitratearray[6] = CAN_BAUD_100K;
  bitratearray[7] = CAN_BAUD_125K;
  bitratearray[8] = CAN_BAUD_250K;
  bitratearray[9] = CAN_BAUD_500K;
  bitratearray[10] = CAN_BAUD_1M;

  // Everything that has to to with the CAN filters
  QLabel *hw0label = new QLabel(tr("Hardware filter 1:"));
  QLabel *hw1label = new QLabel(tr("Hardware filter 2:"));
  QLabel *hw2label = new QLabel(tr("Hardware filter 3:"));
  QLabel *hw3label = new QLabel(tr("Hardware filter 4:"));
  hwfilter[0] = new QLineEdit("0:0");
  hwfilter[1] = new QLineEdit("");
  hwfilter[2] = new QLineEdit("");
  hwfilter[3] = new QLineEdit("");
  filterlayout->addWidget(hw0label);
  filterlayout->addWidget(hwfilter[0]);
  filterlayout->addWidget(hw1label);
  filterlayout->addWidget(hwfilter[1]);
  filterlayout->addWidget(hw2label);
  filterlayout->addWidget(hwfilter[2]);
  filterlayout->addWidget(hw3label);
  filterlayout->addWidget(hwfilter[3]);

  QHBoxLayout *filterbuttonlayout = new QHBoxLayout;
  QPushButton *filterclear = new QPushButton(tr("Clear CAN filters"));
  QPushButton *filterapply = new QPushButton(tr("Apply CAN filters"));
  filterbuttonlayout->addWidget(filterclear);
  filterbuttonlayout->addWidget(filterapply);
  filterlayout->addLayout(filterbuttonlayout);

  connect(filterclear, SIGNAL(clicked()), this, SLOT(clearfilter()));
  connect(filterapply, SIGNAL(clicked()), this, SLOT(applyfilter()));

  QLabel *filterhelp = new QLabel(tr("\
For help how to use the CAN filters, please see the thesis/documentation<br />\
However, here is a short hint:<br />\
&lt;can_id&gt;:&lt;can_mask&gt; (matches when <br />\
&lt;received_can_id&gt; & mask == can_id & mask)<br />\
&lt;can_id&gt;~&lt;can_mask&gt; (matches when <br />\
&lt;received_can_id&gt; & mask != can_id & mask)<br />\
#<error_mask> (set error frame filter, see include/linux/can/error.h)<br />\
CAN IDs, masks and data content are given and expected in <br />\
hexadecimal values. When can_id and can_mask are both 8 digits,<br />\
they are assumed to be 29 bit EFF.<br />\
Examples:<br />\
123:7FF, 400:700, #000000FF, 400~7F0<br />\
0~0, #FFFFFFFF    (log only error frames but no(!) data frames)<br />\
0:0, #FFFFFFFF    (log error frames and also all data frames)<br />\
92345678:DFFFFFFF (match only for extended CAN ID 12345678)<br />\
123:7FF (matches CAN ID 123 - including EFF and RTR frames)<br />\
123:C00007FF (matches CAN ID 123 - only SFF and non-RTR frames)<br />"));

  filterlayout->addWidget(filterhelp);

  setLayout(mainlayout);
  setWindowTitle(tr("Setup socketcangui"));

  // Scan for connected PEAK adpaters
  updatepeaklist();
}

/*!
 * Update the list of PEAK adapters and their bitrates.
 */
void SetupDialog::updatepeaklist() {
  bool ok;
  // Open the device file that contains all the information we want
  QFile peakdevfile("/proc/pcan");
  if (!peakdevfile.exists()) return;

  QTextStream stream(&peakdevfile);
  QString line;
  QString currentbitrate = QString("???");

  // Read through the file
  // If it is not empty and contains no dashes, it is an interface
  // Hopefully, they don't change the format one day? :/
  peakdevfile.open(QIODevice::ReadOnly);
  ifacelist->clear();
  ifacelistitems.clear();
  do {
    line = stream.readLine();
    if (!line.contains("-") && !line.isEmpty()) {
      // This is an interface line
      QStringList list = line.split(" ", QString::SkipEmptyParts);
#ifdef DEBUG
      cerr << "#: \"" << list.at(0).toAscii().constData() << "\" Type: \"" << list.at(1).toAscii().constData() << "\" IFname: \"" << list.at(2).toAscii().constData() << "\"" << endl;
      cerr.flush();
#endif
      // Find out the current bitrate
      for (quint16 i = 0; i < 11; i++) {
        if (list.at(5).toUInt(&ok, 16) == bitratearray[i]) {
          switch (i) {
          case 0: currentbitrate = QString("5 kBit/s"); break;
          case 1: currentbitrate = QString("10 kBit/s"); break;
          case 2: currentbitrate = QString("20 kBit/s"); break;
          case 3: currentbitrate = QString("33.33 kBit/s"); break;
          case 4: currentbitrate = QString("50 kBit/s"); break;
          case 5: currentbitrate = QString("95.2 kBit/s"); break;
          case 6: currentbitrate = QString("100 kBit/s"); break;
          case 7: currentbitrate = QString("125 kBit/s"); break;
          case 8: currentbitrate = QString("250 kBit/s"); break;
          case 9: currentbitrate = QString("500 kBit/s"); break;
          case 10: currentbitrate = QString("1 MBit/s"); break;
          }
        }
      }
      // Add it to the list
      ifacelistitems.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList() << list.at(0) << list.at(1) << list.at(2) << currentbitrate));
      ifacelist->addTopLevelItems(ifacelistitems);
    }
  } while (!line.isNull());
  peakdevfile.close();
  // Make the table look better
  ifacelist->resizeColumnToContents(0);
  ifacelist->resizeColumnToContents(1);
  ifacelist->resizeColumnToContents(2);
  ifacelist->resizeColumnToContents(3);
}

/*!
 * Set the bitrate of selected adapters.
 */
void SetupDialog::setbitrate() {
  // This takes some time, so let the user know we are working
  QApplication::setOverrideCursor(Qt::WaitCursor);
#ifdef DEBUG
  cerr << "Bitrate shall be set to " << bitratearray[bitratecombo->currentIndex()] << endl;
  cerr.flush();
#endif
  QString devname;
  QFile *charfile = new QFile;
  QTextStream stream;
  foreach (QTreeWidgetItem *item, ifacelist->selectedItems()) {
#ifdef DEBUG
    cerr << "Doing adapter " << item->text(0).toAscii().constData() << endl;
    cerr.flush();
  #endif
    devname = QString("/dev/pcan");
    devname.append(item->text(0));
#ifdef DEBUG
    cerr << "Charfile is \"" << devname.toAscii().constData() << "\"" << endl;
    cerr.flush();
  #endif
    charfile->setFileName(devname);
    if (!charfile->exists()) return;
    charfile->open(QIODevice::WriteOnly);
    stream.setDevice(charfile);
    // Write the "magic string"
    stream << "i " << bitratearray[bitratecombo->currentIndex()] << " e" << endl;
    stream.flush();
    charfile->close();
  }
  QApplication::restoreOverrideCursor();

  // Update the modified bitrates in the list of adapters
  updatepeaklist();
}

/*!
 * Reset the CAN filters to their default values.
 */
void SetupDialog::clearfilter() {
  hwfilter[0]->setText("0:0");
  hwfilter[1]->setText("");
  hwfilter[2]->setText("");
  hwfilter[3]->setText("");
  applyfilter();
}

/*!
 * Call this function to applay the filters.
 */
void SetupDialog::applyfilter() {
  // Construct the filterlist and emit the signal
  QStringList hwfilterlist = QStringList() << hwfilter[0]->text() << hwfilter[1]->text() << hwfilter[2]->text() << hwfilter[3]->text();
  emit setfilter(hwfilterlist);
}
