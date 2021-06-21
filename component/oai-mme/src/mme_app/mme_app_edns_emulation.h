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

#ifndef FILE_MME_APP_EDNS_EMULATION_SEEN
#define FILE_MME_APP_EDNS_EMULATION_SEEN

/*! \file mme_app_edns_emulation.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include "3gpp_29.274.h"
#include "mme_config.h"

// struct in_addr* mme_app_edns_get_sgw_entry(bstring id);
// struct in_addr* mme_app_edns_get_mme_entry(bstring id);
// int mme_app_edns_add_sgw_entry(bstring id, struct in_addr in_addr);
// int mme_app_edns_add_mme_entry(bstring id, struct in_addr in_addr);

void mme_app_edns_get_wrr_entry(bstring id,
                                const interface_type_t interface_type,
                                struct sockaddr** sockaddr);

int mme_app_edns_add_wrr_entry(bstring id, struct sockaddr* edns_ip_addr,
                               const interface_type_t interface_type);

int mme_app_edns_init(const mme_config_t* mme_config_p);
void mme_app_edns_exit(void);

#endif
