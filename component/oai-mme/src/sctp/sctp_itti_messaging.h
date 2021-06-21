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

/*! \file sctp_itti_messaging.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_SCTP_ITTI_MESSAGING_SEEN
#define FILE_SCTP_ITTI_MESSAGING_SEEN
#include "common_defs.h"

int sctp_itti_send_lower_layer_conf(const task_id_t origin_task_id,
                                    const sctp_assoc_id_t assoc_id,
                                    const sctp_stream_id_t stream,
                                    const uint32_t mme_ue_s1ap_id,
                                    const bool is_success);

int sctp_itti_send_new_association(const sctp_assoc_id_t assoc_id,
                                   const sctp_stream_id_t instreams,
                                   const sctp_stream_id_t outstreams);

int sctp_itti_send_new_message_ind(STOLEN_REF bstring* payload,
                                   const sctp_assoc_id_t assoc_id,
                                   const sctp_stream_id_t stream,
                                   const sctp_stream_id_t instreams,
                                   const sctp_stream_id_t outstreams);

int sctp_itti_send_com_down_ind(const sctp_assoc_id_t assoc_id);

#endif /* FILE_SCTP_ITTI_MESSAGING_SEEN */
