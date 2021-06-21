/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the terms found in the LICENSE file in the root of this source tree.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include "ControllerEvents.h"
#include <netinet/in.h>
#include <string.h>

using namespace fluid_msg;

namespace openflow {

ControllerEvent::ControllerEvent(fluid_base::OFConnection* ofconn,
                                 const ControllerEventType type)
    : ofconn_(ofconn), type_(type) {}

const ControllerEventType ControllerEvent::get_type() const { return type_; }

fluid_base::OFConnection* ControllerEvent::get_connection() const {
  return ofconn_;
}

DataEvent::DataEvent(fluid_base::OFConnection* ofconn,
                     fluid_base::OFHandler& ofhandler, const void* data,
                     const size_t len, const ControllerEventType type)
    : ControllerEvent(ofconn, type),
      ofhandler_(ofhandler),
      data_(static_cast<const uint8_t*>(data)),
      len_(len) {}

DataEvent::~DataEvent() { ofhandler_.free_data(const_cast<uint8_t*>(data_)); }

const uint8_t* DataEvent::get_data() const { return data_; }

const size_t DataEvent::get_length() const { return len_; }

PacketInEvent::PacketInEvent(fluid_base::OFConnection* ofconn,
                             fluid_base::OFHandler& ofhandler, const void* data,
                             const size_t len)
    : DataEvent(ofconn, ofhandler, data, len, EVENT_PACKET_IN) {}

SwitchUpEvent::SwitchUpEvent(fluid_base::OFConnection* ofconn,
                             fluid_base::OFHandler& ofhandler, const void* data,
                             const size_t len)
    : DataEvent(ofconn, ofhandler, data, len, EVENT_SWITCH_UP) {}

SwitchDownEvent::SwitchDownEvent(fluid_base::OFConnection* ofconn)
    : ControllerEvent(ofconn, EVENT_SWITCH_DOWN) {}

ErrorEvent::ErrorEvent(fluid_base::OFConnection* ofconn,
                       const struct ofp_error_msg* error_msg)
    : error_type_(ntohs(error_msg->type)),
      error_code_(ntohs(error_msg->code)),
      ControllerEvent(ofconn, EVENT_ERROR) {}

const uint16_t ErrorEvent::get_error_type() const { return error_type_; }

const uint16_t ErrorEvent::get_error_code() const { return error_code_; }

ExternalEvent::ExternalEvent(const ControllerEventType type)
    : ControllerEvent(NULL, type) {}

void ExternalEvent::set_of_connection(fluid_base::OFConnection* ofconn) {
  ofconn_ = ofconn;
}

AddGTPTunnelEvent::AddGTPTunnelEvent(const struct in_addr ue_ip,
                                     const struct in_addr enb_ip,
                                     const uint32_t in_tei,
                                     const uint32_t out_tei, const char* imsi,
                                     const pcc_rule_t* const rule)
    : ue_ip_(ue_ip),
      enb_ip_(enb_ip),
      in_tei_(in_tei),
      out_tei_(out_tei),
      imsi_(imsi),
      rule_(rule),
      ExternalEvent(EVENT_ADD_GTP_TUNNEL) {}

const struct in_addr& AddGTPTunnelEvent::get_ue_ip() const { return ue_ip_; }

const struct in_addr& AddGTPTunnelEvent::get_enb_ip() const { return enb_ip_; }

const uint32_t AddGTPTunnelEvent::get_in_tei() const { return in_tei_; }

const uint32_t AddGTPTunnelEvent::get_out_tei() const { return out_tei_; }

const std::string& AddGTPTunnelEvent::get_imsi() const { return imsi_; }

const pcc_rule_t* const AddGTPTunnelEvent::get_rule() const { return rule_; }

DeleteGTPTunnelEvent::DeleteGTPTunnelEvent(const struct in_addr ue_ip,
                                           const uint32_t in_tei,
                                           const uint32_t out_tei,
                                           const pcc_rule_t* const rule)
    : ue_ip_(ue_ip),
      in_tei_(in_tei),
      out_tei_(out_tei),
      rule_(rule),
      ExternalEvent(EVENT_DELETE_GTP_TUNNEL) {}

const struct in_addr& DeleteGTPTunnelEvent::get_ue_ip() const { return ue_ip_; }

const uint32_t DeleteGTPTunnelEvent::get_in_tei() const { return in_tei_; }

const uint32_t DeleteGTPTunnelEvent::get_out_tei() const { return out_tei_; }

const pcc_rule_t* const DeleteGTPTunnelEvent::get_rule() const { return rule_; }

StopDLDataNotificationEvent::StopDLDataNotificationEvent(
    const struct in_addr ue_ip, uint16_t time_out)
    : ue_ip_(ue_ip),
      time_out_(time_out),
      ExternalEvent(EVENT_STOP_DL_DATA_NOTIFICATION) {}

const struct in_addr& StopDLDataNotificationEvent::get_ue_ip() const {
  return ue_ip_;
}

const uint16_t StopDLDataNotificationEvent::get_time_out() const {
  return time_out_;
}

}  // namespace openflow
