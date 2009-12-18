/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_UDPSENDSTRATEGY_H
#define DCPS_UDPSENDSTRATEGY_H

#include "Udp_Export.h"

#include "UdpDataLink_rch.h"

#include "dds/DCPS/transport/framework/TransportSendStrategy.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Udp_Export UdpSendStrategy
  : public TransportSendStrategy {
public:
  explicit UdpSendStrategy(UdpDataLink* link);

  virtual void stop_i();

protected:
  virtual ssize_t send_bytes_i(const iovec iov[], int n);

private:
  UdpDataLink_rch link_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* DCPS_UDPSENDSTRATEGY_H */
