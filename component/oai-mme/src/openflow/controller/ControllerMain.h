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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "pgw_pcef_emulation.h"

#define CONTROLLER_ADDR "127.0.0.1"
#define CONTROLLER_PORT 6654
#define NUM_WORKERS 2

int start_of_controller(void);

int stop_of_controller(void);

int openflow_controller_add_gtp_tunnel(struct in_addr ue, struct in_addr enb,
                                       uint32_t i_tei, uint32_t o_tei,
                                       const char* imsi,
                                       const pcc_rule_t* const rule);

int openflow_controller_del_gtp_tunnel(struct in_addr ue, uint32_t i_tei,
                                       uint32_t o_tei,
                                       const pcc_rule_t* const rule);

int openflow_controller_stop_dl_data_notification_ue(struct in_addr ue,
                                                     uint16_t timeout);

#ifdef __cplusplus
}
#endif
