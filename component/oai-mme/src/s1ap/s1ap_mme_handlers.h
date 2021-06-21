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

/*! \file s1ap_mme_handlers.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_S1AP_MME_HANDLERS_SEEN
#define FILE_S1AP_MME_HANDLERS_SEEN
#include "intertask_interface.h"
#include "s1ap_mme.h"
#define MAX_NUM_PARTIAL_S1_CONN_RESET 256

/** \brief Handle decoded incoming messages from SCTP
 * \param assoc_id SCTP association ID
 * \param stream Stream number
 * \param pdu The message decoded by the ASN1C decoder
 * @returns int
 **/
int s1ap_mme_handle_message(const sctp_assoc_id_t assoc_id,
                            const sctp_stream_id_t stream,
                            S1AP_S1AP_PDU_t* pdu);

int s1ap_mme_handle_ue_cap_indication(const sctp_assoc_id_t assoc_id,
                                      const sctp_stream_id_t stream,
                                      S1AP_S1AP_PDU_t* message);

/** \brief Handle an S1 Setup request message.
 * Typically add the eNB in the list of served eNB if not present, simply reset
 * UEs association otherwise. S1SetupResponse message is sent in case of success
 *or S1SetupFailure if the MME cannot accept the configuration received. \param
 *assoc_id SCTP association ID \param stream Stream number \param pdu The
 *message decoded by the ASN1C decoder
 * @returns int
 **/
int s1ap_mme_handle_s1_setup_request(const sctp_assoc_id_t assoc_id,
                                     const sctp_stream_id_t stream,
                                     S1AP_S1AP_PDU_t* pdu);

int s1ap_mme_handle_path_switch_request(const sctp_assoc_id_t assoc_id,
                                        const sctp_stream_id_t stream,
                                        S1AP_S1AP_PDU_t* pdu);

int s1ap_mme_handle_ue_context_release_request(const sctp_assoc_id_t assoc_id,
                                               const sctp_stream_id_t stream,
                                               S1AP_S1AP_PDU_t* pdu);

int s1ap_handle_ue_context_release_command(
    const itti_s1ap_ue_context_release_command_t* const
        ue_context_release_command_pP);

int s1ap_mme_handle_ue_context_release_complete(const sctp_assoc_id_t assoc_id,
                                                const sctp_stream_id_t stream,
                                                S1AP_S1AP_PDU_t* pdu);

int s1ap_mme_handle_initial_context_setup_failure(
    const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t* pdu);

int s1ap_mme_handle_initial_context_setup_response(
    const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t* pdu);

int s1ap_handle_sctp_disconnection(const sctp_assoc_id_t assoc_id, bool reset);

int s1ap_handle_new_association(sctp_new_peer_t* sctp_new_peer_p);

int s1ap_mme_set_cause(S1AP_Cause_t* cause_p, const S1AP_Cause_PR cause_type,
                       const long cause_value);

int s1ap_mme_generate_s1_setup_failure(const sctp_assoc_id_t assoc_id,
                                       const S1AP_Cause_PR cause_type,
                                       const long cause_value,
                                       const long time_to_wait);

/*** HANDLING EXPIRED TIMERS. */
void s1ap_mme_handle_ue_context_rel_comp_timer_expiry(void* ue_ref_p);

void s1ap_mme_handle_mme_mobility_completion_timer_expiry(void* ue_ref_p);

int s1ap_mme_handle_erab_setup_response(const sctp_assoc_id_t assoc_id,
                                        const sctp_stream_id_t stream,
                                        S1AP_S1AP_PDU_t* message);

int s1ap_mme_handle_erab_modify_response(const sctp_assoc_id_t assoc_id,
                                         const sctp_stream_id_t stream,
                                         S1AP_S1AP_PDU_t* message);

int s1ap_mme_handle_erab_release_response(const sctp_assoc_id_t assoc_id,
                                          const sctp_stream_id_t stream,
                                          S1AP_S1AP_PDU_t* message);

int s1ap_mme_handle_erab_release_indication(const sctp_assoc_id_t assoc_id,
                                            const sctp_stream_id_t stream,
                                            S1AP_S1AP_PDU_t* message);

int s1ap_mme_handle_erab_modification_indication(const sctp_assoc_id_t assoc_id,
                                                 const sctp_stream_id_t stream,
                                                 S1AP_S1AP_PDU_t* pdu);

void s1ap_mme_generate_erab_modification_confirm(
    const itti_s1ap_e_rab_modification_cnf_t* const conf);

int s1ap_handle_enb_initiated_reset_ack(
    const itti_s1ap_enb_initiated_reset_ack_t* const enb_reset_ack_p);

int s1ap_mme_handle_error_ind_message(const sctp_assoc_id_t assoc_id,
                                      const sctp_stream_id_t stream,
                                      S1AP_S1AP_PDU_t* message);

int s1ap_mme_handle_enb_reset(const sctp_assoc_id_t assoc_id,
                              const sctp_stream_id_t stream,
                              S1AP_S1AP_PDU_t* message);

int s1ap_mme_handle_enb_configuration_transfer(const sctp_assoc_id_t assoc_id,
                                               const sctp_stream_id_t stream,
                                               S1AP_S1AP_PDU_t* message);

/***************************** *
 * HANDOVER MESSAGING.
 * **************************  */
int s1ap_mme_handle_path_switch_request(const sctp_assoc_id_t assoc_id,
                                        const sctp_stream_id_t stream,
                                        S1AP_S1AP_PDU_t* pdu);

int s1ap_mme_handle_handover_preparation(const sctp_assoc_id_t assoc_id,
                                         const sctp_stream_id_t stream,
                                         S1AP_S1AP_PDU_t* pdu);

int s1ap_mme_handle_handover_cancel(const sctp_assoc_id_t assoc_id,
                                    const sctp_stream_id_t stream,
                                    S1AP_S1AP_PDU_t* message);

int s1ap_mme_handle_handover_notification(const sctp_assoc_id_t assoc_id,
                                          const sctp_stream_id_t stream,
                                          S1AP_S1AP_PDU_t* message);

int s1ap_mme_handle_handover_resource_allocation_response(
    const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t* message);

int s1ap_mme_handle_handover_resource_allocation_failure(
    const sctp_assoc_id_t assoc_id, const sctp_stream_id_t stream,
    S1AP_S1AP_PDU_t* message);

int s1ap_mme_handle_enb_status_transfer(const sctp_assoc_id_t assoc_id,
                                        const sctp_stream_id_t stream,
                                        S1AP_S1AP_PDU_t* message);

#endif /* FILE_S1AP_MME_HANDLERS_SEEN */
