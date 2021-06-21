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

/*! \file mme_app_wrr_selection.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <inttypes.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "TrackingAreaIdentity.h"
#include "common_defs.h"
#include "common_types.h"
#include "dynamic_memory_check.h"
#include "log.h"
#include "mme_app_edns_emulation.h"
#include "mme_app_wrr_selection.h"
#include "mme_config.h"

//------------------------------------------------------------------------------
void mme_app_select_service(const tai_t* const tai,
                            struct sockaddr** const service_ip_addr,
                            const interface_type_t interface_type) {
  // see in 3GPP TS 29.303 version 10.5.0 Release 10:
  // 5.2 Procedures for Discovering and Selecting an MME or SGW (service:
  // ="x-3gpp-mme:x-s10/s11" )
  // ....

  // do it the simplest way for now
  //
  char tmp[8];
  bstring application_unique_string = bfromcstr("tac-lb");
  if (0 < snprintf(tmp, 8, "%02x", tai->tac & 0x00FF)) {
    bcatcstr(application_unique_string, tmp);
  } else {
    memset((void*)*service_ip_addr, 0, sizeof(struct sockaddr));
    return;
  }
  bcatcstr(application_unique_string, ".tac-hb");
  if (0 < snprintf(tmp, 8, "%02x", tai->tac >> 8)) {
    bcatcstr(application_unique_string, tmp);
  } else {
    goto lookup_error;
  }
  bcatcstr(application_unique_string, ".tac.epc.mnc");
  uint16_t mnc = (tai->plmn.mnc_digit1 * 10) + tai->plmn.mnc_digit2;
  if (10 > tai->plmn.mnc_digit3) {
    mnc = (mnc * 10) + tai->plmn.mnc_digit3;
  }
  if (0 < snprintf(tmp, 8, "%03u", mnc)) {
    bcatcstr(application_unique_string, tmp);
  } else {
    goto lookup_error;
  }
  bcatcstr(application_unique_string, ".mcc");
  if (0 < snprintf(tmp, 8, "%u%u%u", tai->plmn.mcc_digit1, tai->plmn.mcc_digit2,
                   tai->plmn.mcc_digit3)) {
    bcatcstr(application_unique_string, tmp);
  } else {
    goto lookup_error;
  }
  bcatcstr(application_unique_string, ".3gppnetwork.org");

  mme_app_edns_get_wrr_entry(application_unique_string, interface_type,
                             service_ip_addr);

  if (*service_ip_addr) {
    if ((*service_ip_addr)->sa_family == AF_INET) {
      OAILOG_DEBUG(
          LOG_MME_APP, "Service lookup %s returned %s\n",
          application_unique_string->data,
          inet_ntoa(((struct sockaddr_in*)*service_ip_addr)->sin_addr));
    } else {
      char ipv6[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, (void*)*service_ip_addr, ipv6, INET6_ADDRSTRLEN);
      OAILOG_DEBUG(LOG_MME_APP, "Service lookup %s returned %s\n",
                   application_unique_string->data, ipv6);
    }
  }

  bdestroy_wrapper(&application_unique_string);
  return;

lookup_error:
  OAILOG_WARNING(LOG_MME_APP, "Failed service lookup for TAI " TAI_FMT "\n",
                 TAI_ARG(tai));
  memset((void*)service_ip_addr, 0, sizeof(struct sockaddr));
  bdestroy_wrapper(&application_unique_string);
  return;
}
