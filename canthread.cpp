/* written 2009, 2010 by Jannis Achstetter
 * contact: s25581@h-ab.de
 *
 * developed at Hochschule Aschaffenburg
 * licensed under the terms of the General Public
 * License (GPL) version 3.0
 */

#include "canthread.h"

#include <iostream>

using namespace std;

/*!
 * Constructor, initialising the thread as stopped.
 */
canthread::canthread() {
  stopped = true;
}

/*!
 * Stop the thread.
 */
void canthread::stop() {
  stopped = true;
}

/*!
 * Set the name of the interface to use.
 * @param ifnametobeset Name of the interface to be used
 */
void canthread::setifname (QString ifnametobeset) {
  ifname = ifnametobeset;
}

/*!
 * Send away one packet.
 * @param sendpacket Packet-data to be sent
 */
void canthread::sendmsg(canpacket sendpacket) {
  struct can_frame frame;
  int nbytes;

  // Do nothing if the thread is not running
  if (stopped) return;

  // Construct the canpacket to be sent away
  frame.can_id = sendpacket.identifier;
  if (sendpacket.ide) frame.can_id = frame.can_id | CAN_EFF_FLAG;
  frame.can_dlc = sendpacket.dlc;
  memcpy(frame.data, sendpacket.data, 8);
  sendpacket.interface = 0;       // not used yet
  sendpacket.crc = 0;             // not used yet
  sendpacket.direction = false;   // we sent it
  sendpacket.rtr = false;         // not used yet
  gettimeofday(&sendpacket.tv, NULL);

#ifdef DEBUG
  cerr << "sending canframe ..." << endl;
  cerr.flush();
#endif

  // send the frame, modify the counters and add the frame to the canlogfile
  if ((nbytes = write(sockfd, &frame, sizeof(frame))) != sizeof(frame)) {
    cerr << "Problem while writing frame!" << endl; cerr.flush();
  } else {
    mystatus.outcounter++;
    mystatus.outbcounter = mystatus.outbcounter + frame.can_dlc;

    statusChanged(mystatus);
    dataarrived(sendpacket);
  }
}

/*!
 * Set the CAN hardware filters for the socket.
 * @param hwfilter List of the filters to be applied
 */
void canthread::setfilter(QStringList hwfilter) {
  struct can_filter *rfilter;
  int numfilter = 0;
  int err_mask = 0;

  // Currently, we allow 4 CAN hardware filters
  rfilter = (struct can_filter *)malloc(sizeof(struct can_filter) * 4);

#ifdef DEBUG
  cerr << "SET CAN FILTER NOW" << endl;
  cerr << "Hwfilter 1: " << hwfilter.at(0).toAscii().constData() << endl;
  cerr << "Hwfilter 2: " << hwfilter.at(1).toAscii().constData() << endl;
  cerr << "Hwfilter 3: " << hwfilter.at(2).toAscii().constData() << endl;
  cerr << "Hwfilter 4: " << hwfilter.at(3).toAscii().constData() << endl;
  cerr.flush();
#endif

  // For every filter, check whether it is an ID-filter or the error_mask
  // taken from can-utils/candump.c
  for (int i = 0; i <=3; i++) {
    if (hwfilter.at(i).isEmpty()) continue;
    if (sscanf(hwfilter.at(i).toAscii().constData(), "%lx:%lx", (long unsigned int *)&rfilter[numfilter].can_id, (long unsigned int *)&rfilter[numfilter].can_mask) == 2) {
      rfilter[numfilter].can_mask &= ~CAN_ERR_FLAG;
      numfilter++;
    } else if (sscanf(hwfilter.at(i).toAscii().constData(), "%lx~%lx", (long unsigned int *)&rfilter[numfilter].can_id, (long unsigned int *)&rfilter[numfilter].can_mask) == 2) {
      rfilter[numfilter].can_id |= CAN_INV_FILTER;
      rfilter[numfilter].can_mask &= ~CAN_ERR_FLAG;
      numfilter++;
    } else if (sscanf(hwfilter.at(i).toAscii().constData(), "#%lx", (long unsigned int *)&err_mask) != 1) {
      cerr << "Error parsing CAN filter " << i << endl;
      cerr.flush();
    }
  }

#ifdef DEBUG
  cerr << "We got " << numfilter << " ID filters and err_mask is " << err_mask << endl;
  cerr.flush();
#endif

  // Now apply the filters to the socket
  if (err_mask) setsockopt(sockfd, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &err_mask, sizeof(err_mask));
  if (numfilter) setsockopt(sockfd, SOL_CAN_RAW, CAN_RAW_FILTER, rfilter, numfilter * sizeof(struct can_filter));

  free(rfilter);
}

/*!
 * Start the thread and enter it's main loop.
 * To stop the thread, call the stop()-function.
 */
void canthread::run() {
  struct sockaddr_can addr;
  struct ifreq ifr;
  int ret;
  struct timeval tv; //timeval of canframe just recvd
  canpacket mypacket;
  struct pollfd rdfs;

  // "new" (revmsg)-method
  struct can_frame frame;
  struct iovec iov;
  struct msghdr msg;
  char ctrlmsg[CMSG_SPACE(sizeof(struct timeval)) + CMSG_SPACE(sizeof(__u32))];
  int nbytes;
  //struct cmsghdr *cmsg; //see below

  // Better safe than sorry
  bzero(&mypacket, sizeof(mypacket));

  // Reset the counters
  mystatus.incounter = 0;
  mystatus.outcounter = 0;
  mystatus.inbcounter = 0;
  mystatus.outbcounter = 0;
  statusChanged(mystatus);

  sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (sockfd < 0) {
    cerr << "ERROR opening socket" << endl; cerr.flush();
  }
  addr.can_family = AF_CAN;
#ifdef DEBUG
  cerr << "I shall use interface \"" << ifname.toLocal8Bit().constData() << "\"" << endl;
  cerr.flush();
#endif
  strcpy(ifr.ifr_name, ifname.toLocal8Bit().constData());
  ioctl(sockfd, SIOCGIFINDEX, &ifr);
  addr.can_ifindex = ifr.ifr_ifindex;

  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    cerr << "Error binding" << endl; cerr.flush();
  }

  // Get ready to poll()
  rdfs.fd = sockfd;
  rdfs.events = POLLIN;

  stopped = false;

  // "new" method, taken from can-utils/candump.c
  /* these settings are static and can be held out of the hot path */
  iov.iov_base = &frame;
  msg.msg_name = &addr;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = &ctrlmsg;

  while (!stopped) {
     // poll() waits until data arrives or 100msec have passed
    ret = poll(&rdfs, 1, 100);
    if (ret < 0) {
      cerr << "poll() error >_<" << endl; cerr.flush();
    } else if (ret != 0) {
      // "new" (recvmsg)-method
      iov.iov_len = sizeof(frame);
      msg.msg_namelen = sizeof(addr);
      msg.msg_controllen = sizeof(ctrlmsg);
      msg.msg_flags = 0;

      nbytes = recvmsg(sockfd, &msg, 0);

      // Get the CAN frame's timestamp
      // From candump.c but the thing below works better
      //for (cmsg = CMSG_FIRSTHDR(&msg); cmsg && (cmsg->cmsg_level == SOL_SOCKET); cmsg = CMSG_NXTHDR(&msg,cmsg)) {
      //  if (cmsg->cmsg_type == SO_TIMESTAMP) tv = *(struct timeval *)CMSG_DATA(cmsg);
      //}
      if (ioctl(sockfd, SIOCGSTAMP, &tv) < 0) perror("SIOCGSTAMP");

      // Update the counters
      mystatus.incounter++;
      mystatus.inbcounter = mystatus.inbcounter + frame.can_dlc;

      mypacket.dlc = frame.can_dlc;
      mypacket.direction = true; // we recvd that packet
      // omit EFF, RTR, ERR flags so that only the CAN ID remains
      mypacket.identifier = frame.can_id & CAN_EFF_MASK;
      mypacket.rtr = frame.can_id & CAN_RTR_FLAG;
      mypacket.ide = frame.can_id & CAN_EFF_FLAG;
      mypacket.err = frame.can_id & CAN_ERR_FLAG;
      memcpy(mypacket.data, frame.data, 8);
      memcpy(&mypacket.tv, &tv, sizeof(struct timeval));

      // Pass the packet to the main thread
      dataarrived(mypacket);
      statusChanged(mystatus);
    }
  }

  // Thread shall be stopped here
  close(sockfd);
  stopped = true;
}
