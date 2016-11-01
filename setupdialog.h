/* written 2009, 2010 by Jannis Achstetter
 * contact: kripton@kripserver.net
 *
 * developed at Hochschule Aschaffenburg
 * licensed under the terms of the General Public
 * License (GPL) version 3.0
 */

#ifndef SETUPDIALOG_H_
#define SETUPDIALOG_H_

#include <QtGui>

#include <iostream>

// Taken from the PEAK CAN driver manual
#define CAN_BAUD_1M     0x0014 //   1 Mbit/s
#define CAN_BAUD_500K   0x001C // 500 kBit/s
#define CAN_BAUD_250K   0x011C // 250 kBit/s
#define CAN_BAUD_125K   0x031C // 125 kBit/s
#define CAN_BAUD_100K   0x432F // 100 kBit/s
#define CAN_BAUD_50K    0x472F //  50 kBit/s
#define CAN_BAUD_20K    0x532F //  20 kBit/s
#define CAN_BAUD_10K    0x672F //  10 kBit/s
#define CAN_BAUD_5K     0x7F7F //   5 kBit/s

// Other bitrates (calculated using PEAK's BRCAN-tool
// ( http://www.peak-system.com/forum/viewtopic.php?f=125&t=87 )
#define CAN_BAUD_33K    0x2F11 // 33.33 kBit/s
#define CAN_BAUD_95K    0x0D12 // 95.2 kBit/s

/*!
 * Dialog to set up the CAN hardware filters and set the bitrate for PEAK can adapters
 */
class SetupDialog : public QDialog
{
  Q_OBJECT

public:
  SetupDialog(QWidget *parent = 0);       //!< Initialize the dialog

signals:
  void setfilter(QStringList hwfilter);   //!< Will be emitted when the filters shall be applied

private slots:
  void updatepeaklist();                  //!< Update the list of PEAK adapters and their bitrates
  void setbitrate();                      //!< Set the bitrate of selected adapters
  void clearfilter();                     //!< Reset the CAN filters to their default values
  void applyfilter();                     //!< Call this function to apply the filters

private:
  QTreeWidget *ifacelist;                 //!< widget to display network interfaces
  QList<QTreeWidgetItem *> ifacelistitems;  //!< list of network interface items
  QComboBox *bitratecombo;               //!< Combobox to select the bitrate to be set
  QLineEdit *hwfilter[4];                 //!< The four QLineEdits containing the filter strings

  quint16 bitratearray[9];               //!< array with possible bitrates
};

#endif /* SETUPDIALOG_H_ */
