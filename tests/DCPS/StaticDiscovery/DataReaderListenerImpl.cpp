/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include "DataReaderListenerImpl.h"
#include "TestMsgTypeSupportC.h"
#include "TestMsgTypeSupportImpl.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/GuidConverter.h"
#if !defined (DDS_HAS_MINIMUM_BIT)
#include "dds/DdsDcpsCoreTypeSupportC.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

DataReaderListenerImpl::~DataReaderListenerImpl()
{
  ACE_DEBUG((LM_DEBUG, "(%P|%t) DataReader %C is being destroyed\n", OpenDDS::DCPS::LogGuid(reader_guid_).c_str()));
  if (expected_samples_ && received_samples_ != expected_samples_) {
    ACE_ERROR((LM_ERROR, "ERROR: expected %d but received %d\n",
               expected_samples_, received_samples_));
  } else if (expected_samples_) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Expected number of samples received\n"));
  }
}

void
DataReaderListenerImpl::set_guid(const OpenDDS::DCPS::GUID_t& guid)
{
  reader_guid_ = guid;
  ACE_DEBUG((LM_DEBUG, "(%P|%t) DataReader %C created\n", OpenDDS::DCPS::LogGuid(reader_guid_).c_str()));
}

void
DataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr /*reader*/,
  const DDS::RequestedDeadlineMissedStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr /*reader*/,
  const DDS::RequestedIncompatibleQosStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SampleRejectedStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr /*reader*/,
  const DDS::LivelinessChangedStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  TestMsgDataReader_var reader_i =
    TestMsgDataReader::_narrow(reader);

  if (!reader_i) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(-1);
  }

  TestMsg message;
  DDS::SampleInfo info;

  DDS::ReturnCode_t error = reader_i->take_next_sample(message, info);

  while (error == DDS::RETCODE_OK) {
    if (info.valid_data) {
      DDS::Subscriber_var subscriber = reader->get_subscriber();
      DDS::DomainParticipant_var participant = subscriber->get_participant();
      const OpenDDS::DCPS::GUID_t writer_guid = dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant.in())->get_repoid(info.publication_handle);
      SampleSetMap::iterator it = guid_received_samples_.find(writer_guid);
      if (it == guid_received_samples_.end()) {
        it = guid_received_samples_.insert(SampleSetMap::value_type(writer_guid, std::set<int>())).first;
        if (expect_all_samples_) {
          it->second.insert(0);
        }
      }
      if (reliable_ && !it->second.empty()) {
        int expected = *(it->second.rbegin()) + 1;
        if (message.value != expected) {
          ACE_ERROR((LM_ERROR, "(%P|%t) Missing Data Detected Between Reliable Endpoints: expected message %d but got %d\n", expected, message.value));
        }
        OPENDDS_ASSERT(message.value == expected);
      }
      it->second.insert(message.value);

      ACE_DEBUG((LM_INFO, "(%P|%t) Reader %C got message %d (#%d from writer %C)\n", OpenDDS::DCPS::LogGuid(reader_guid_).c_str(), received_samples_, message.value, OpenDDS::DCPS::LogGuid(writer_guid).c_str()));
      if (++received_samples_ == expected_samples_) {
        done_callback_(builtin_read_error_, reader_guid_);
      }
    }
    error = reader_i->take_next_sample(message, info);
  }
}

void
DataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SubscriptionMatchedStatus& status)
{
  OPENDDS_ASSERT(status.current_count >= 0);
  if (status.current_count > total_writers_) {
    ACE_ERROR((LM_ERROR, "(%P|%t) DataReaderListenerImpl::on_subscription_matched: more writers than expected\n"));
  }
  OPENDDS_ASSERT(previous_count_ + status.current_count_change == status.current_count);
  previous_count_ = status.current_count;
#ifndef DDS_HAS_MINIMUM_BIT
  if (check_bits_ && status.current_count_change > 0) {
    DDS::PublicationBuiltinTopicDataDataReader_var rdr =
      DDS::PublicationBuiltinTopicDataDataReader::_narrow(builtin_);
    DDS::PublicationBuiltinTopicDataSeq data;
    DDS::SampleInfoSeq infos;

    DDS::ReturnCode_t ret =
      rdr->read(data, infos, DDS::LENGTH_UNLIMITED,
                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
    if (ret != DDS::RETCODE_OK && ret != DDS::RETCODE_NO_DATA) {
      ACE_ERROR((LM_ERROR, "ERROR: %P could not read publication BIT: %d\n", ret));
      builtin_read_error_ = true;
      return;
    }

    bool found_valid = false;
    for (CORBA::ULong i = 0; i < data.length(); ++i) {
      if (infos[i].valid_data) {
        found_valid = true;
        if (OpenDDS::DCPS::DCPS_debug_level > 4) {
          ACE_DEBUG((LM_DEBUG,
                     "(%P|%t) Read Publication BIT with key: %x %x %x and handle %d\n"
                     "\tTopic: %C\tType: %C\n",
                     data[i].key.value[0], data[i].key.value[1],
                     data[i].key.value[2], infos[i].instance_handle,
                     data[i].topic_name.in(),
                     data[i].type_name.in()));
        }
      }
    }

    if (found_valid) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) Successfully read publication BITs\n"));
    }

  }
#else
ACE_UNUSED_ARG(status);
#endif /* DDS_HAS_MINIMUM_BIT */
}

void
DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SampleLostStatus& /*status*/)
{
}

#ifndef DDS_HAS_MINIMUM_BIT
void DataReaderListenerImpl::set_builtin_datareader (
  DDS::DataReader_ptr builtin)
{
  builtin_ = DDS::DataReader::_duplicate(builtin);
}
#endif /* DDS_HAS_MINIMUM_BIT */
