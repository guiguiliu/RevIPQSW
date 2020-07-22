#ifndef __LOWI_NETLINK_SOCKET_RECEIVER_H__
#define __LOWI_NETLINK_SOCKET_RECEIVER_H__

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

        LOWI Netlink Socket receiver Header file

GENERAL DESCRIPTION
  This file contains the structure definitions and function prototypes for
  LOWI NetLink Socket Receiver

Copyright (c) 2015-2016,2018 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/

#include <base_util/sync.h>
#include <lowi_server/lowi_scan_result_receiver.h>
#include <lowi_server/lowi_event_receiver.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <regex.h>

namespace qc_loc_fw
{

/**
 * Base class to provide the mechanism to get the netlink socket messages
 * It is run in a separate thread.
 * It's responsibility is to receive the results and provide the results
 * to it's listener.
 */
class LOWINetlinkSocketReceiver: public LOWIScanResultReceiver
{
protected:

  /**
  * Main loop of the Receiver
  */
  virtual void run ();
  /**
   * Parse the received netlink packet
   * @param char* buff buffer to hold netlink message
   * @param int bytesRx length of the received message
   */
  void processNetlinkPacket( char *buf, int bytesRx );
  /**
   * unpack the netlink message to get the driver
   * interface name and state
   * @param struct nlmsghr *: netlink header received
   * @param LOWIDriverInterface&: driver interface information decoded from the
   *                           NL message
   * @return bool: true if expected interfaces detected, false otherwise
   */
  bool unpackRTMLinkMessage(struct nlmsghdr *pNetlinkHdr,
                            LOWIDriverInterface &intf);
  /**
   * block on the select call
   * @return 1 if success, errno,-1 or zero otherwise
   */
  int block ();
  /**
   * read the netlink message on receive call
   * and parse the received netlink packet.
   * @return 1 if success, errno,-1 or zero otherwise
   */
  int readParseNetlinkMessage ();
  /**
   * Terminates the Receiver thread.
   *
   * @return true if success, false otherwise
   */
  virtual bool terminate ();

public:
  /**
   * different state of the socket and thread for netlinksocketreceiver
   * thread
   */
  enum eState
  {
    /** failed to create socket*/
    SOCKET_FAILURE,
    /** creation of socket is success */
    SOCKET_SUCCESS
  };
  /**
   * returns the different states of the NetLink
   * Receiver.
   * defined in eState enum
   * @return eState states of NetLink receiver.
   */
  LOWINetlinkSocketReceiver::eState getState() const;
  /**
   * Constructor
   * @param LOWIScanResultReceiverListener* listener to be notified
   *                                        with the netlink results
   */
  LOWINetlinkSocketReceiver(LOWIScanResultReceiverListener* listener);
  /** Destructor */
  virtual ~LOWINetlinkSocketReceiver();
private:
  static const char * const  TAG;
  /**
   * create the netlink socket for receiving the netlink messages
   * @return zero if success, -1 if failed.
   */
  int createNLSocket();

  /** netlink socket descriptor */
  static int mNetlinkSock;
  /** communication pipe */
  int mPipeFd[2];
  /** netlink socket creation status */
  eState mState;
  /** name pattern used to identify the driver interfaces */
  regex_t mPattern;
protected:
};
} // namespace
#endif //#ifndef __LOWI_NETLINK_SOCKET_RECEIVER_H__
