/* written 2009, 2010 by Jannis Achstetter
 * contact: kripton@kripserver.net
 *
 * developed at Hochschule Aschaffenburg
 * licensed under the terms of the General Public
 * License (GPL) version 3.0
 */

#ifndef CANLOGFILE_H
#define CANLOGFILE_H

#include <QtGui>

/*!
 * Holds all data describing a CAN packet.
 * Some files are not used yet but exist for future enhancements.
 */
struct canpacket {
  int interface;              //!< the can interface this paket was recvd or sent on (not used yet)
  bool direction;             //!< 0 = we recvd that packet; 1 = we sent it
  unsigned short identifier;  //!< CAN ID (only the identifier, no flags)
  bool rtr;                   //!< RTR bit
  bool ide;                   //!< IDE bit (1 if extended ID, 0 otherwise)
  bool err;                   //!< ERROR bit (1 if error frame, 0 otherwise)
  unsigned char dlc;          //!< Data length code
  unsigned char data[8];      //!< Payload (data)
  unsigned short crc;         //!< CRC (not used yet)
  struct timeval tv;          //!< timeval it was recvd or sent
};

/*!
 * One "file" containing 0 to n CAN packets (and main widget of the program).
 */
class canlogfile: public QTreeView {
Q_OBJECT

public:
  explicit canlogfile(QTreeView *parent = 0);   //!< Constructor that initialises the model and sets the headers
  int getdataitemcount();                       //!< Returns the number of canpackets in the file
  void clear();                                 //!< Deletes all canpackets from the file
  bool readFile(const QString &fileName);       //!< Reads a file containing canpackets
  bool writeFile(const QString &fileName);      //!< Writes all canpackets to a file

signals:
  void modified();                              //!< file has been modified since the last open or save

public slots:
  void adddataitem(canpacket);                  //!< Adds one canpacket to the bottom of the file

private slots:
  void somethingChanged();                      //!< SLOT to be called when an item changed or has been added

private:
  QAbstractItemModel *model;                    //!< Model to be displayed containing all canpackets
  enum { MagicNumber = 0x636C6603 /*!< = "clf" + versionbyte */ };
};

#endif // CANLOGFILE_H
