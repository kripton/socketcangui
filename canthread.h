/* written 2009, 2010 by Jannis Achstetter
 * contact: s25581@h-ab.de
 *
 * developed at Hochschule Aschaffenburg
 * licensed under the terms of the General Public
 * License (GPL) version 3.0
 */

#ifndef CANTHREAD_H
#define CANTHREAD_H

#include <QThread>

#include <poll.h>

#include <sys/ioctl.h>
#include <net/if.h>

#include <sys/time.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include "canlogfile.h"

/*!
 * Holds the thread's internal status (packet counters, byte counters, error counters)
 */
struct threadstatus {
  quint64 incounter;                      //!< packets coming in
  quint64 outcounter;                     //!< packets going out
  quint64 inbcounter;                     //!< bytes coming in
  quint64 outbcounter;                    //!< bytes going out
};

/*!
 * Thread to fetch the packets from one interface, put them to the mainthread and send packets.
 */
class canthread: public QThread {
Q_OBJECT

public:
  canthread();                            //!< Constructor, initialising the thread as stopped
  void stop();                            //!< Stop the thread
  void setifname(QString ifnametobeset);  //!< Set the name of the interface to use
  void sendmsg(canpacket sendpacket);     //!< Send away one packet

signals:
  void dataarrived(canpacket mypacket);   //!< We have new data fetched
  void statusChanged(threadstatus mystatus);  //!< The status has changed

public slots:
  void setfilter(QStringList hwfilter);   //!< Set the CAN hardware filters for the socket

protected:
  void run();                             //!< Start the thread and enter it's main loop

private:
  volatile bool stopped;                  //!< Keep our status here internally
  QString ifname;                         //!< Name of network interface to be used
  int sockfd;                             //!< File descriptor of the socket we are working with
  struct threadstatus mystatus;           //!< Status of this thread
};

#endif // CANTHREAD_H
