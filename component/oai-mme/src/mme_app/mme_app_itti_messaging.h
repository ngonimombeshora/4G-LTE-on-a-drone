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

/*! \file mme_app_itti_messaging.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_MME_APP_ITTI_MESSAGING_SEEN
#define FILE_MME_APP_ITTI_MESSAGING_SEEN

#include "mme_app_procedures.h"

int mme_app_notify_s1ap_ue_context_released(const mme_ue_s1ap_id_t ue_idP);
int mme_app_send_nas_signalling_connection_rel_ind(
    const mme_ue_s1ap_id_t ue_id);
void mme_app_send_s11_delete_bearer_cmd(teid_t local_teid,
                                        teid_t saegw_s11_teid,
                                        struct sockaddr* saegw_s11_ip_address,
                                        ebi_list_t* ebi_list);
int mme_app_send_s11_release_access_bearers_req(mme_ue_s1ap_id_t ue_id);
int mme_app_send_s11_create_session_req(
    const mme_ue_s1ap_id_t ue_id, const imsi_t* const imsi_p,
    pdn_context_t* pdn_context, tai_t* serving_tai,
    const protocol_configuration_options_t* const pco,
    const bool is_from_s10_tau);
void mme_app_send_s11_modify_bearer_req(
    const struct ue_session_pool_s* const ue_session_pool,
    pdn_context_t* pdn_context, uint8_t flags);
int mme_app_remove_s10_tunnel_endpoint(teid_t local_teid,
                                       struct sockaddr* peer_ip);
int mme_app_send_delete_session_request(
    struct ue_context_s* const ue_context_p, const ebi_t ebi,
    const struct sockaddr* saegw_s11_addr, const teid_t saegw_s11_teid,
    const bool noDelete, const bool handover,
    const uint8_t internal_flags); /**< Moved Delete Session Request from
                                      mme_app_detach. */

/**
 * If cause value is nonzero, it will be set.
 * If it is zero, it will be derived from the cause values of the bearer
 * contexts.
 */
int mme_app_send_s11_create_bearer_rsp(
    teid_t mme_teid_s11, teid_t s_gw_teid_s11_s4,
    gtpv2c_cause_value_t cause_value, void* trxn,
    bearer_contexts_to_be_created_t* bcs_tbc);

int mme_app_send_s11_update_bearer_rsp(
    teid_t mme_teid_s11, teid_t s_gw_teid_s11_s4,
    gtpv2c_cause_value_t extra_cause, void* trxn,
    bearer_contexts_to_be_updated_t* bcs_tbu);

int mme_app_send_s11_delete_bearer_rsp(teid_t mme_teid_s11,
                                       teid_t s_gw_teid_s11_s4,
                                       gtpv2c_cause_value_t cause_value,
                                       void* trxn, ebi_list_t* ebi_list);

void mme_app_itti_nas_context_response(ue_context_t* ue_context,
                                       nas_s10_context_t* s10_context_val);
void mme_app_itti_nas_pdn_connectivity_response(
    mme_ue_s1ap_id_t ue_id, const ebi_t default_ebi,
    const gtpv2c_cause_value_t gtpv2c_cause_value);
void mme_app_itti_forward_relocation_response(
    ue_context_t* ue_context, struct PdnContexts* pdnContexts,
    mme_app_s10_proc_mme_handover_t* s10_handover_proc,
    bstring target_to_source_container);
void mme_app_send_s1ap_handover_cancel_acknowledge(
    mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id,
    sctp_assoc_id_t assoc_id);
void mme_app_send_s1ap_handover_preparation_failure(
    mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id,
    sctp_assoc_id_t assoc_id, enum s1cause cause);
void mme_app_send_s10_forward_relocation_response_err(
    teid_t mme_source_s10_teid, struct sockaddr* mme_source_ip_address,
    void* trxn, gtpv2c_cause_value_t gtpv2cCause);
void _mme_app_send_nas_context_response_err(mme_ue_s1ap_id_t ueId,
                                            gtpv2c_cause_value_t cause_val);
void mme_app_send_s1ap_mme_status_transfer(
    mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id,
    uint32_t enb_id, status_transfer_bearer_list_t* status_transfer);
void mme_app_send_s1ap_path_switch_request_failure(
    mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id,
    sctp_assoc_id_t assoc_id, const S1AP_Cause_PR cause_type);

void notify_s1ap_new_ue_mme_s1ap_id_association(
    const sctp_assoc_id_t assoc_id, const enb_ue_s1ap_id_t enb_ue_s1ap_id,
    const mme_ue_s1ap_id_t mme_ue_s1ap_id);

void mme_app_send_s1ap_e_rab_mofification_confirm(
    const mme_ue_s1ap_id_t mme_ue_s1ap_id,
    const enb_ue_s1ap_id_t enb_ue_s1ap_id,
    const mme_app_s1ap_proc_modify_bearer_ind_t* const proc);

void mme_app_send_s1ap_mme_configuration_transfer(target_type_t target_enb_type,
                                                  tai_t* target_tai,
                                                  ecgi_t* target_global_enb_id,
                                                  target_type_t source_enb_type,
                                                  tai_t* source_tai,
                                                  ecgi_t* source_global_enb_id,
                                                  enb_conf_reply_t* conf_reply);

#endif /* FILE_MME_APP_ITTI_MESSAGING_SEEN */
