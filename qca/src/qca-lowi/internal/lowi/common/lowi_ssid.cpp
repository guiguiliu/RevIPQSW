/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

        LOWI SSID

GENERAL DESCRIPTION
  This file contains the implementation of LOWISsid

  Copyright (c) 2012-2013,2016,2018 Qualcomm Technologies, Inc.
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

#include <inc/lowi_ssid.h>

using namespace qc_loc_fw;
const char* const LOWISsid::TAG = "LOWISsid";
/////////////////////
// LOWISsid
/////////////////////
LOWISsid::LOWISsid ()
{
  // Default values
  m_isSsidValid = false;
  m_ssid_length = 0;
  memset(m_ssid, 0, sizeof (m_ssid));
  m_isSsidSet = false;
}

int LOWISsid::setSSID(const unsigned char * const ssid, const int length)
{
  int result = 0;
  uint16 valid_len = (uint16)length;
  do
  {
    m_isSsidValid = false;

    if (NULL == ssid)
    {
      log_verbose (TAG, "ssid null!");
      // NULL SSID is valid.
      m_ssid_length = 0;
      m_isSsidValid = true;
      break;
    }
    else if (0 == length)
    {
      log_verbose (TAG, "length 0!");
      // SSID with length 0 is still valid
      m_ssid_length = 0;
      m_isSsidValid = true;
      break;
    }

    if(length < 0)
    {
      log_error (TAG, "ssid invalid length!");
      result = -3;
      break;
    }
    else if(length > (int) sizeof(m_ssid))
    {
      log_debug (TAG, "ssid length more than allowed. Truncating");
      valid_len = sizeof (m_ssid);
    }

    memcpy(m_ssid, ssid, valid_len);
    m_ssid_length = valid_len;
    m_isSsidValid = true;
    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(TAG, "setSSID failed %d", result);
  }

  // SSID set function is called. Set to true
  m_isSsidSet = true;
  return result;
}

bool LOWISsid::isSSIDValid() const
{
  return m_isSsidValid;
}
int LOWISsid::getSSID(unsigned char * const pSsid, int * const pLength) const
{
  // Check if set ssid was ever called. If not, get ssid does not need
  // to report error.
  if (false == m_isSsidSet) {
    log_verbose (TAG, "SSID was never set!");
    return -1;
  }

  int result = 1;
  do
  {
    if (NULL == pSsid)
    {
      log_verbose (TAG, "Invalid argument - ssid null!");
      result = -2;
      break;
    }

    if(NULL == pLength)
    {
      log_verbose (TAG, "Invalid argument - ssid length null!");
      result = -3;
      break;
    }

    if(!m_isSsidValid)
    {
      log_error (TAG, "ssid invalid!");
      result = -4;
      break;
    }
    for (uint16 ii = 0; ii < m_ssid_length && ii < SSID_LEN; ++ii)
    {
      pSsid [ii] = m_ssid[ii];
    }
    *pLength = (int)m_ssid_length;
    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(TAG, "getSSID failed %d", result);
  }
  return result;
}

int LOWISsid::compareTo (const LOWISsid & ssid)
{
  if (this->m_ssid_length != ssid.m_ssid_length)
  {
    return -1;
  }
  for (uint16 jj = 0; jj < m_ssid_length; ++jj)
  {
    if (this->m_ssid [jj] != ssid.m_ssid[jj])
    {
      log_error (TAG, "SSID does not match");
      return -1;
    }
  }
  return 0;
}

void LOWISsid::print (const uint8* const ssid, const int len)
{
  char str [SSID_LEN+1] = {0};
  log_verbose(TAG, "%s: SSID = %s, Len = %d\n",
              __FUNCTION__, LOWISsid::toString(ssid, len, str), len);
}

const char*
LOWISsid::toString (const uint8* const ssid, const uint8 len,
                             char str[SSID_LEN+1])
{
  uint8 print_len = len;
  if (NULL == ssid)
  {
    log_debug(TAG, "%s - NULL pointer, return empty string", __FUNCTION__);
    // Return string
    str[0] = ' ';
    str[1] = '\0';
  }
  else
  {
    /* Just want to copy up to SSID_LEN; cap it if it's more */
    if (print_len > SSID_LEN)
    {
      print_len = SSID_LEN;
    }
    memcpy(str, ssid, print_len);

    /* Null terminate the string */
    str[print_len] = '\0';

    /* Go through each character and if it's non-ASCII, convert it to "_" */
    for (int ii = 0; ii < print_len; ++ii)
    {
      /* If it's not printable character */
      if (!isprint(str[ii]))
      {
        str[ii] = '_';
      }
    }
  }
  return str;
}
