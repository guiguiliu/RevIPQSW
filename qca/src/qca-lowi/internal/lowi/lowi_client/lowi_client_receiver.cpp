/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

        LOWI Client Receiver

GENERAL DESCRIPTION
  This file contains the implementation of LOWI Client Receiver

Copyright (c) 2012-2013, 2017-2018 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

(c) 2012-2013 Qualcomm Atheros, Inc.
  All Rights Reserved.
  Qualcomm Atheros Confidential and Proprietary.

=============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <base_util/log.h>

#include <base_util/postcard.h>
#include <mq_client/mq_client.h>
#include <inc/lowi_defines.h>
#include <inc/lowi_client_receiver.h>

using namespace qc_loc_fw;

#define REGISTER_REQUEST_TIMEOUT_MSEC 500  // Wait timeout for registration
#define CONNECTION_RETRY_COUNT_MAX    3    // Number of times to retry registration

// Time window from boot during which connection request to message server
// will be retried.
#define CONNECTION_RETRY_TIME_SEC_MAX    30

const char* const LOWIClientReceiver::TAG = "LOWIClientReceiver";

LOWIClientReceiver::LOWIClientReceiver(const char * const socket_name,
    BlockingQueue * const pLocalMsgQueue,
    MessageQueueClient * const conn,
    MessageQueueServiceCallback* const callback,
    const char* const client_name)
: mSocketName (socket_name),
  mMsgQueue (pLocalMsgQueue),
  mMsgQueueClient (conn),
  mCallback (callback),
  mClientName (client_name),
  mReceiverThread (NULL)
{
  log_debug(TAG, "LOWIClientReceiver::LOWIClientReceiver ()");
}

LOWIClientReceiver::~LOWIClientReceiver()
{
  log_verbose (TAG, "~LOWIClientReceiver");

  if (NULL != mMsgQueueClient)
  {
    if (0 == mMsgQueueClient->shutdown())
    {
      log_debug (TAG, "~LOWIClientReceiver - About to join");
      mReceiverThread->join ();
      log_debug (TAG, "~LOWIClientReceiver - After join complete");
    }
    delete mMsgQueueClient;
  }
  if (mReceiverThread != NULL)
  {
    delete mReceiverThread;
  }
}

bool LOWIClientReceiver::init ()
{
  bool retVal = false;
  // In case the connection fails for the first time we will retry
  int retry_count = CONNECTION_RETRY_COUNT_MAX;
  log_verbose(TAG, "init ()");
  do
  {
    // Delete the thread first if exists already
    if (NULL != mReceiverThread)
    {
      delete mReceiverThread;
      mReceiverThread = NULL;
    }

    // Start the Receiver thread. No need to delete the runnable at destruction.
    mReceiverThread = Thread::createInstance(TAG, this, false);
    if (mReceiverThread == NULL)
    {
      log_warning (TAG, "init() - Unable to create receiver thread instance");
      break;
    }
    else
    {
      mReceiverThread->launch ();

      // Set the time out value for which the thread will block
      TimeDiff timeout_diff(true);
      timeout_diff.add_msec(REGISTER_REQUEST_TIMEOUT_MSEC);

      bool is_queue_closed = false;

      // Message to retrieve. Block until time out or until
      // any message is received in the queue. This is to ensure that
      // the receiver thread starts and able to do the registration with
      // IPC hub before this main thread is able to send any requests.
      void * ptr = 0;
      mMsgQueue->pop (&ptr, timeout_diff, &is_queue_closed);
      if (0 != ptr)
      {
        log_verbose(TAG, "init () - Message received"
            " in blocking q");
        retVal = true;
        break;
      }
      else
      {
        // timeout or queue closed?
        if(is_queue_closed)
        {
          // queue closed case
          log_warning(TAG, "init () - "
              "queue closed");
          break;
        }
        else
        {
          // By default the Timestamp constructor initializes to CLOCK_BOOTTIME
          Timestamp timeSinceBoot;
          // Most likely a timeout
          // we cannot be sure what caused this, but probably
          // connect to the socket failed. Retry only if
          // the phone has recently booted up.
          log_warning(TAG, "%s - Request Timeout at %" PRIu64,
                      __FUNCTION__, timeSinceBoot.getTimestampPtr()->tv_sec);
          if (timeSinceBoot.getTimestampPtr()->tv_sec > CONNECTION_RETRY_TIME_SEC_MAX)
          {
            break;
          }
          --retry_count;
        }
      }
    }
  } while (retry_count);
  return retVal;
}

// This code is executed in the Receiver thread context
void LOWIClientReceiver::run()
{
  // Initiate the msg queue client
  mMsgQueueClient->setServerNameDup (mSocketName);
  int retVal = mMsgQueueClient->connect ();
  log_debug (TAG, "connect retVal: %d", retVal);

  if (retVal == 0)
  {
    log_verbose (TAG, "connect successful");
    // Send Register message
    OutPostcard * card = OutPostcard::createInstance();
    if (NULL != card)
    {
      card->init();
      card->addString("TO", "SERVER");
      card->addString("FROM", mClientName);
      card->addString("REQ", "REGISTER");
      card->finalize();
      retVal = mMsgQueueClient->send(card->getEncodedBuffer());
      delete card;
    }

    if (0 == retVal)
    {
      // Unblock the main thread by posting a message in the blocking
      // queue at which it is waiting to receive the confirmation of
      // Registration completion.
      const char* const msg = "RegisterComplete";
      this->mMsgQueue->push ((void*) msg);
    }

    // Start the receiver thread loop
    mMsgQueueClient->run_block (this->mCallback);
  }
  else
  {
    // Connect failed. No need to post to the message queue
    // which will make it time out.
    log_warning (TAG, "connect failed %d", retVal);
  }
  log_verbose (TAG, "run complete");
}

