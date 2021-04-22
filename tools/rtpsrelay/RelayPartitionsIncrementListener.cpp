#include "RelayPartitionsIncrementListener.h"

namespace RtpsRelay {

RelayPartitionsIncrementListener::RelayPartitionsIncrementListener(RelayPartitionTable& relay_partition_table)
  : relay_partition_table_(relay_partition_table)
{}

void RelayPartitionsIncrementListener::on_data_available(DDS::DataReader_ptr reader)
{
  RelayPartitionsIncrementDataReader_var dr = RelayPartitionsIncrementDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ReaderListener::on_data_available failed to narrow RtpsRelay::RelayPartitionsIncrementDataReader\n")));
    return;
  }

  RelayPartitionsIncrementSeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->take(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ReaderListener::on_data_available failed to read\n")));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    switch (infos[idx].instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      {
        const auto& d = data[idx];
        relay_partition_table_.increment_insert(guid_to_repoid(d.application_participant_guid()),
                                                d.partitions());
      }
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      {
        const auto& d = data[idx];
        relay_partition_table_.increment_remove(guid_to_repoid(d.application_participant_guid()));
      }
      break;
    }
  }
}

}