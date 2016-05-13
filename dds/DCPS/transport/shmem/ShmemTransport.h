/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_SHMEMTRANSPORT_H
#define OPENDDS_SHMEMTRANSPORT_H

#include "Shmem_Export.h"

#include "ShmemDataLink.h"
#include "ShmemDataLink_rch.h"

#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/PoolAllocator.h"

#include <string>

namespace OpenDDS {
namespace DCPS {

class ShmemInst;

class OpenDDS_Shmem_Export ShmemTransport : public TransportImpl {
public:
  explicit ShmemTransport(const TransportInst_rch& inst);

  // used by our DataLink:
  ShmemAllocator* alloc() { return alloc_; }
  std::string address();
  void signal_semaphore();

protected:
  virtual AcceptConnectResult connect_datalink(const RemoteTransport& remote,
                                               const ConnectionAttribs& attribs,
                                               TransportClient* client);

  virtual AcceptConnectResult accept_datalink(const RemoteTransport& remote,
                                              const ConnectionAttribs& attribs,
                                              TransportClient* client);

  virtual void stop_accepting_or_connecting(TransportClient* client,
                                            const RepoId& remote_id);

  virtual bool configure_i(TransportInst* config);

  virtual void shutdown_i();

  virtual bool connection_info_i(TransportLocator& info) const;

  virtual void release_datalink(DataLink* link);

  virtual std::string transport_type() const { return "shmem"; }

private:

  /// Create a new link (using make_datalink) and add it to the map
  DataLink* add_datalink(const std::string& remote_address);

  /// Create the DataLink object and start it
  ShmemDataLink* make_datalink(const std::string& remote_address);

  std::pair<std::string, std::string> blob_to_key(const TransportBLOB& blob);

  void read_from_links(); // callback from ReadTask

  RcHandle<ShmemInst> config_i_;

  typedef ACE_Thread_Mutex        LockType;
  typedef ACE_Guard<LockType>     GuardType;

  LockType links_lock_;

  /// Map of fully associated DataLinks for this transport.  Protected
  /// by links_lock_.
  typedef OPENDDS_MAP(std::string, ShmemDataLink_rch) ShmemDataLinkMap;
  ShmemDataLinkMap links_;

  ShmemAllocator* alloc_;

  struct ReadTask : ACE_Task_Base {
    ReadTask(ShmemTransport* outer, ACE_sema_t semaphore);
    int svc();
    void stop();

    ShmemTransport* outer_;
    ACE_sema_t semaphore_;
    bool stopped_;

  }* read_task_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_SHMEMTRANSPORT_H */