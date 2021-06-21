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

/*! \file mme_app_itti_messaging.c
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bstrlib.h"

#include "assertions.h"
#include "common_defs.h"
#include "common_types.h"
#include "conversions.h"
#include "dynamic_memory_check.h"
#include "esm_cause.h"
#include "gcc_diag.h"
#include "intertask_interface.h"
#include "log.h"
#include "mme_app_apn_selection.h"
#include "mme_app_bearer_context.h"
#include "mme_app_defs.h"
#include "mme_app_extern.h"
#include "mme_app_itti_messaging.h"
#include "mme_app_pdn_context.h"
#include "mme_app_session_context.h"
#include "mme_app_ue_context.h"
#include "mme_app_wrr_selection.h"
#include "mme_config.h"
#include "msc.h"

// todo: also check this for home/macro
//------------------------------------------------------------------------------
void mme_app_itti_ue_context_release(mme_ue_s1ap_id_t mme_ue_s1ap_id,
                                     enb_ue_s1ap_id_t enb_ue_s1ap_id,
                                     enum s1cause cause, uint32_t enb_id) {
  MessageDef *message_p;
  OAILOG_FUNC_IN(LOG_MME_APP);

  message_p =
      itti_alloc_new_message(TASK_MME_APP, S1AP_UE_CONTEXT_RELEASE_COMMAND);
  memset((void *)&message_p->ittiMsg.s1ap_ue_context_release_command, 0,
         sizeof(itti_s1ap_ue_context_release_command_t));
  S1AP_UE_CONTEXT_RELEASE_COMMAND(message_p).mme_ue_s1ap_id = mme_ue_s1ap_id;
  S1AP_UE_CONTEXT_RELEASE_COMMAND(message_p).enb_ue_s1ap_id = enb_ue_s1ap_id;
  S1AP_UE_CONTEXT_RELEASE_COMMAND(message_p).enb_id = enb_id;
  S1AP_UE_CONTEXT_RELEASE_COMMAND(message_p).cause = cause;
  MSC_LOG_TX_MESSAGE(
      MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0,
      "0 S1AP_UE_CONTEXT_RELEASE_COMMAND enb_ue_s1ap_id %06" PRIX32 " ",
      S1AP_UE_CONTEXT_RELEASE_COMMAND(message_p).enb_ue_s1ap_id);
  itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_itti_notify_request(const imsi64_t imsi,
                                 const plmn_t *handovered_plmn,
                                 const bool mobility_completion) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  MessageDef *message_p = NULL;
  emm_data_context_t *emm_context = NULL;
  s6a_notify_req_t *s6a_nr_req = NULL;

  message_p = itti_alloc_new_message(TASK_MME_APP, S6A_NOTIFY_REQ);

  if (message_p == NULL) {
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }

  s6a_nr_req = &message_p->ittiMsg.s6a_notify_req;

  /** Recheck that the UE context is found by the IMSI. */
  if ((emm_context = emm_data_context_get_by_imsi(&_emm_data, imsi)) == NULL) {
    OAILOG_ERROR(LOG_MME_APP,
                 "That's embarrassing as we don't know this IMSI " IMSI_64_FMT
                 ". \n",
                 imsi);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }

  IMSI_TO_STRING(&emm_context->_imsi, s6a_nr_req->imsi,
                 IMSI_BCD_DIGITS_MAX + 1);

  s6a_nr_req->imsi_length = strlen(s6a_nr_req->imsi);

  if (mobility_completion) {
    s6a_nr_req->single_registration_indiction = SINGLE_REGITRATION_INDICATION;
  }

  memcpy(&s6a_nr_req->visited_plmn, handovered_plmn, sizeof(plmn_t));

  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_S6A_MME, NULL, 0,
                     "0 S6A_NOTIFY_REQ ue id " MME_UE_S1AP_ID_FMT " ", ue_id);

  itti_send_msg_to_task(TASK_S6A, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
int mme_app_send_nas_signalling_connection_rel_ind(
    const mme_ue_s1ap_id_t ue_id) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  MessageDef *message_p = NULL;
  int rc = RETURNok;

  message_p =
      itti_alloc_new_message(TASK_MME_APP, NAS_SIGNALLING_CONNECTION_REL_IND);

  NAS_SIGNALLING_CONNECTION_REL_IND(message_p).ue_id = ue_id;

  MSC_LOG_TX_MESSAGE(
      MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0,
      "0 NAS_SIGNALLING_CONNECTION_REL_IND ue id " MME_UE_S1AP_ID_FMT " ",
      ue_id);

  rc = itti_send_msg_to_task(TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN(LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
void mme_app_send_s11_delete_bearer_cmd(teid_t local_teid,
                                        teid_t saegw_s11_teid,
                                        struct sockaddr *saegw_s11_ip_address,
                                        ebi_list_t *ebi_list) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  /*
   * Keep the identifier to the default APN.
   */
  MessageDef *message_p = NULL;

  /** Trigger a Delete Bearer Command. */
  message_p = itti_alloc_new_message(TASK_MME_APP, S11_DELETE_BEARER_COMMAND);
  DevAssert(message_p != NULL);
  itti_s11_delete_bearer_command_t *s11_delete_bearer_command =
      &message_p->ittiMsg.s11_delete_bearer_command;
  /** Take the last one. */
  s11_delete_bearer_command->local_teid = local_teid;
  s11_delete_bearer_command->teid = saegw_s11_teid;
  memcpy((void *)&s11_delete_bearer_command->edns_peer_ip, saegw_s11_ip_address,
         saegw_s11_ip_address->sa_family == AF_INET
             ? sizeof(struct sockaddr_in)
             : sizeof(struct sockaddr_in6));

  memcpy(&s11_delete_bearer_command->ebi_list, ebi_list, sizeof(ebi_list_t));
  itti_send_msg_to_task(TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_DEBUG(LOG_MME_APP,
               "Triggered Delete Bearer Command from released e_rab indication "
               "to teid %x from local teid %x",
               saegw_s11_teid, local_teid);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
int mme_app_send_s11_release_access_bearers_req(mme_ue_s1ap_id_t ue_id) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  /*
   * Keep the identifier to the default APN
   */
  MessageDef *message_p = NULL;
  itti_s11_release_access_bearers_request_t *release_access_bearers_request_p =
      NULL;
  pdn_context_t *pdn_context = NULL;
  int rc = RETURNok;

  ue_session_pool_t *ue_session_pool =
      mme_ue_session_pool_exists_mme_ue_s1ap_id(
          &mme_app_desc.mme_ue_session_pools, ue_id);
  if (!ue_session_pool) {
    OAILOG_ERROR(LOG_MME_APP,
                 "NO UE session pool for UE with Id " MME_UE_S1AP_ID_FMT ". \n",
                 ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }
  pdn_context = RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
  if (!pdn_context) {
    OAILOG_ERROR(LOG_MME_APP,
                 "NO PDN Context for UE with Id " MME_UE_S1AP_ID_FMT ". \n",
                 ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }

  message_p =
      itti_alloc_new_message(TASK_MME_APP, S11_RELEASE_ACCESS_BEARERS_REQUEST);
  release_access_bearers_request_p =
      &message_p->ittiMsg.s11_release_access_bearers_request;
  memset((void *)release_access_bearers_request_p, 0,
         sizeof(itti_s11_release_access_bearers_request_t));

  /** Sending one RAB for all PDNs also in the specification. */
  release_access_bearers_request_p->local_teid =
      ue_session_pool->privates.fields.mme_teid_s11;
  release_access_bearers_request_p->teid = pdn_context->s_gw_teid_s11_s4;
  memcpy(
      (void *)&release_access_bearers_request_p->edns_peer_ip,
      (struct sockaddr *)&pdn_context->s_gw_addr_s11_s4,
      ((struct sockaddr *)&pdn_context->s_gw_addr_s11_s4)->sa_family == AF_INET
          ? sizeof(struct sockaddr_in)
          : sizeof(struct sockaddr_in6));
  release_access_bearers_request_p->originating_node = NODE_TYPE_MME;
  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0,
                     "0 S11_RELEASE_ACCESS_BEARERS_REQUEST teid %u",
                     release_access_bearers_request_p->teid);
  rc = itti_send_msg_to_task(TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN(LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
int mme_app_send_s11_create_session_req(
    const mme_ue_s1ap_id_t ue_id, const imsi_t *const imsi_p,
    pdn_context_t *pdn_context, tai_t *serving_tai,
    const protocol_configuration_options_t *const pco,
    const bool is_from_s10_tau) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  MessageDef *message_p = NULL;
  ue_context_t *ue_context = NULL;
  ue_session_pool_t *ue_session_pool = NULL;
  itti_s11_create_session_request_t *session_request_p = NULL;
  int rc = RETURNok;

  // todo: handover flag in operation-identifier?!
  // todo: further move parameters from ue_context to ue_sp
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id(
      &mme_app_desc.mme_ue_contexts, ue_id);
  if (!ue_context) {
    OAILOG_ERROR(LOG_MME_APP,
                 "No UE context for UE " MME_UE_S1AP_ID_FMT
                 ". Cannot send CSR. \n",
                 ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }

  ue_session_pool = mme_ue_session_pool_exists_mme_ue_s1ap_id(
      &mme_app_desc.mme_ue_session_pools, ue_id);
  if (!ue_session_pool) {
    OAILOG_ERROR(LOG_MME_APP,
                 "No UE session pool for UE " MME_UE_S1AP_ID_FMT
                 ". Cannot send CSR. \n",
                 ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }
  if (!pdn_context) {
    OAILOG_ERROR(LOG_MME_APP,
                 "No pdn context for UE " MME_UE_S1AP_ID_FMT
                 ". Cannot send CSR. \n",
                 ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }

  OAILOG_DEBUG(LOG_MME_APP, "Sending CSR for imsi " IMSI_64_FMT "\n",
               ue_context->privates.fields.imsi);

  message_p = itti_alloc_new_message(TASK_MME_APP, S11_CREATE_SESSION_REQUEST);
  /*
   * WARNING:
   * Some parameters should be provided by NAS Layer:
   * - ue_time_zone
   * - mei
   * - uli
   * - uci
   * Some parameters should be provided by HSS:
   * - PGW address for CP
   * - paa
   * - ambr
   * and by MME Application layer:
   * - selection_mode
   * Set these parameters with random values for now.
   */
  session_request_p = &message_p->ittiMsg.s11_create_session_request;
  //  memset (session_request_p, 0, sizeof (itti_s11_create_session_request_t));
  /*
   * As the create session request is the first exchanged message and as
   * no tunnel had been previously setup, the distant teid is set to 0.
   * The remote teid will be provided in the response message.
   */
  session_request_p->teid = ue_session_pool->privates.fields.saegw_teid_s11;
  /** IMSI. */
  memcpy((void *)&session_request_p->imsi, imsi_p, sizeof(imsi_t));
  // message content was set to 0
  /*
   * Copy the MSISDN
   */
  memcpy(session_request_p->msisdn.digit, ue_context->privates.fields.msisdn,
         blength(ue_context->privates.fields.msisdn));
  session_request_p->msisdn.length =
      blength(ue_context->privates.fields.msisdn);
  session_request_p->rat_type = RAT_EUTRAN;
  /**
   * Set the indication flag.
   */
  memset(&session_request_p->indication_flags, 0,
         sizeof(session_request_p->indication_flags));  // TO DO
  session_request_p->indication_flags.crsi = 0x1;

  if (is_from_s10_tau) {
    session_request_p->indication_flags.oi =
        0x1; /** Currently only setting for idle TAU. */
  }

  session_request_p->uli.present |= ULI_ECGI;
  session_request_p->uli.s.ecgi = ue_context->privates.fields.e_utran_cgi;
  session_request_p->uli.present |= ULI_TAI;
  tai_to_Tai(serving_tai, &session_request_p->uli.s.tai);

  // todo: apn restrictions!

  // Actually, since S and P GW are bundled together, there is no PGW selection
  // (based on PGW id in ULA, or DNS query based on FQDN)
  struct sockaddr *edns_peer_ip = NULL;
  if (1) {
    // TODO prototype may change
    mme_app_select_service(serving_tai, &edns_peer_ip, S11_SGW_GTP_C);
    //    session_request_p->peer_ip.in_addr = mme_config.ipv4.
  }
  if (!edns_peer_ip) {
    OAILOG_ERROR(LOG_MME_APP,
                 "No EDNS Peer IP for UE " MME_UE_S1AP_ID_FMT
                 " could be found for TAI " TAI_FMT ". Cannot send CSR. \n",
                 ue_id, TAI_ARG(serving_tai));
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }
  memcpy((void *)&session_request_p->edns_peer_ip, edns_peer_ip,
         edns_peer_ip->sa_family == AF_INET ? sizeof(struct sockaddr_in)
                                            : sizeof(struct sockaddr_in6));
  /*
   * Copy the subscribed APN-AMBR to the sgw create session request message
   */
  memcpy(&session_request_p->ambr, &pdn_context->subscribed_apn_ambr,
         sizeof(ambr_t));
  /*
   * Default EBI
   */
  session_request_p->default_ebi = pdn_context->default_ebi;
  /** Set the bearer contexts to be created (incl. TFT). */
  DevAssert(!session_request_p->bearer_contexts_to_be_created);
  session_request_p->bearer_contexts_to_be_created =
      calloc(1, sizeof(bearer_contexts_to_be_created_t));
  mme_app_get_bearer_contexts_to_be_created(
      pdn_context, session_request_p->bearer_contexts_to_be_created,
      BEARER_STATE_MME_CREATED);

  /*
   * Asking for default bearer in initial UE message.
   * Use the address of ue_context as unique TEID: Need to find better here
   * and will generate unique id only for 32 bits platforms.
   */
  OAI_GCC_DIAG_OFF(pointer - to - int - cast);
  session_request_p->sender_fteid_for_cp.teid =
      ue_context->privates.fields.mme_teid_s11;
  OAI_GCC_DIAG_ON(pointer - to - int - cast);
  session_request_p->sender_fteid_for_cp.interface_type = S11_MME_GTP_C;
  mme_config_read_lock(&mme_config);
  if (mme_config.ip.s11_mme_v4.s_addr) {
    session_request_p->sender_fteid_for_cp.ipv4_address =
        mme_config.ip.s11_mme_v4;
    session_request_p->sender_fteid_for_cp.ipv4 = 1;
  }
  if (memcmp(&mme_config.ip.s11_mme_v6.s6_addr, (void *)&in6addr_any,
             sizeof(mme_config.ip.s11_mme_v6.s6_addr)) != 0) {
    memcpy(session_request_p->sender_fteid_for_cp.ipv6_address.s6_addr,
           mme_config.ip.s11_mme_v6.s6_addr,
           sizeof(mme_config.ip.s11_mme_v6.s6_addr));
    session_request_p->sender_fteid_for_cp.ipv6 = 1;
  }
  mme_config_unlock(&mme_config);

  memcpy(session_request_p->apn, pdn_context->apn_subscribed->data,
         blength(pdn_context->apn_subscribed));
  // todo: set the full apn name
  // memcpy (session_request_p->apn,
  // ue_context->pending_pdn_connectivity_req_apn->data,
  // ue_context->pending_pdn_connectivity_req_apn->slen);

  /*
   * Set PDN type for pdn_type and PAA even if this IE is redundant
   */
  session_request_p->pdn_type = pdn_context->pdn_type;
  session_request_p->paa.pdn_type = pdn_context->pdn_type;

  if (!pdn_context->paa) {
    /*
     * UE DHCPv4 allocated ip address
     */
    session_request_p->paa.ipv4_address.s_addr = INADDR_ANY;
    session_request_p->paa.ipv6_address = in6addr_any;
  } else {
    //       memcpy (session_request_p->paa.ipv4_address,
    //       ue_context->pending_pdn_connectivity_req_pdn_addr->data, 4); /**<
    //       String to array. */
    session_request_p->paa.ipv4_address.s_addr =
        pdn_context->paa->ipv4_address.s_addr;
    memcpy(&session_request_p->paa.ipv6_address,
           &pdn_context->paa->ipv6_address,
           sizeof(session_request_p->paa.ipv6_address));
    session_request_p->paa.ipv6_prefix_length =
        pdn_context->paa->ipv6_prefix_length;
  }
  //  session_request_p->apn_restriction = 0x00; todo: set them where?
  if (pco &&
      pco->num_protocol_or_container_id) { /**< todo: Should not exist in
                                              handover, where to get them?. */
    copy_protocol_configuration_options(&session_request_p->pco, pco);
  } else if (pdn_context->pco &&
             pdn_context->pco->num_protocol_or_container_id) {
    copy_protocol_configuration_options(&session_request_p->pco,
                                        pdn_context->pco);
  }

  session_request_p->serving_network.mcc[0] = serving_tai->plmn.mcc_digit1;
  session_request_p->serving_network.mcc[1] = serving_tai->plmn.mcc_digit2;
  session_request_p->serving_network.mcc[2] = serving_tai->plmn.mcc_digit3;
  session_request_p->serving_network.mnc[0] = serving_tai->plmn.mnc_digit1;
  session_request_p->serving_network.mnc[1] = serving_tai->plmn.mnc_digit2;
  session_request_p->serving_network.mnc[2] = serving_tai->plmn.mnc_digit3;
  session_request_p->selection_mode = MS_O_N_P_APN_S_V;
  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0,
                     "0 S11_CREATE_SESSION_REQUEST imsi " IMSI_64_FMT,
                     ue_contextP->imsi);
  OAILOG_DEBUG(LOG_MME_APP, "Sending CSR for imsi (2) " IMSI_64_FMT "\n",
               ue_context->privates.fields.imsi);
  rc = itti_send_msg_to_task(TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN(LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
void mme_app_send_s11_modify_bearer_req(
    const ue_session_pool_t *const ue_session_pool, pdn_context_t *pdn_context,
    uint8_t flags) {
  OAILOG_FUNC_IN(LOG_MME_APP);

  MessageDef *message_p = NULL;
  message_p = itti_alloc_new_message(TASK_MME_APP, S11_MODIFY_BEARER_REQUEST);

  /*
   * WARNING:
   * Some parameters should be provided by NAS Layer:
   * - ue_time_zone
   * - mei
   * - uli
   * - uci
   * Some parameters should be provided by HSS:
   * - PGW address for CP
   * - paa
   * - ambr
   * and by MME Application layer:
   * - selection_mode
   * Set these parameters with random values for now.
   */
  itti_s11_modify_bearer_request_t *s11_modify_bearer_request =
      &message_p->ittiMsg.s11_modify_bearer_request;
  /*
   * As the create session request is the first exchanged message and as
   * no tunnel had been previously setup, the distant teid is set to 0.
   * The remote teid will be provided in the response message.
   */

  s11_modify_bearer_request->local_teid =
      ue_session_pool->privates.fields.mme_teid_s11;
  /*
   * Delay Value in integer multiples of 50 millisecs, or zero
   */
  s11_modify_bearer_request->delay_dl_packet_notif_req = 0;  // TO DO
  memcpy(
      (void *)&s11_modify_bearer_request->edns_peer_ip,
      (struct sockaddr *)&pdn_context->s_gw_addr_s11_s4,
      ((struct sockaddr *)&pdn_context->s_gw_addr_s11_s4)->sa_family == AF_INET
          ? sizeof(struct sockaddr_in)
          : sizeof(struct sockaddr_in6));

  // todo: IPv6 SAE-GW address
  s11_modify_bearer_request->teid = pdn_context->s_gw_teid_s11_s4;
  /** Add the bearers to establish. */
  bearer_context_new_t *bearer_context_to_establish = NULL;
  STAILQ_FOREACH(bearer_context_to_establish, &pdn_context->session_bearers,
                 entries) {
    DevAssert(bearer_context_to_establish);
    /** Add them to the bearers list of the MBR. */
    if ((bearer_context_to_establish->bearer_state &
         BEARER_STATE_ENB_CREATED) &&
        bearer_context_to_establish->enb_fteid_s1u.teid) {
      /** Bearer which are currently pending modification or deletion, will
       * still be continued to be modified, worst case, they will be modified
       * twice. */
      OAILOG_DEBUG(LOG_MME_APP,
                   "Adding EBI %d as bearer context to be modified for "
                   "UE " MME_UE_S1AP_ID_FMT ". \n",
                   bearer_context_to_establish->ebi,
                   ue_session_pool->privates.mme_ue_s1ap_id);
      s11_modify_bearer_request->bearer_contexts_to_be_modified
          .bearer_context[s11_modify_bearer_request
                              ->bearer_contexts_to_be_modified
                              .num_bearer_context]
          .eps_bearer_id = bearer_context_to_establish->ebi;
      memcpy(&s11_modify_bearer_request->bearer_contexts_to_be_modified
                  .bearer_context[s11_modify_bearer_request
                                      ->bearer_contexts_to_be_modified
                                      .num_bearer_context]
                  .s1_eNB_fteid,
             &bearer_context_to_establish->enb_fteid_s1u,
             sizeof(bearer_context_to_establish->enb_fteid_s1u));
      s11_modify_bearer_request->bearer_contexts_to_be_modified
          .num_bearer_context++;
    } else {
      /** Check the ESM Bearer Context status, add if it is in ESM_ACTIVE state.
       */
      if (bearer_context_to_establish->esm_ebr_context.status ==
          ESM_EBR_ACTIVE) {
        OAILOG_WARNING(LOG_MME_APP,
                       "Adding EBI %d as bearer context to be removed for "
                       "UE " MME_UE_S1AP_ID_FMT ". \n",
                       bearer_context_to_establish->ebi,
                       ue_session_pool->privates.mme_ue_s1ap_id);
        s11_modify_bearer_request->bearer_contexts_to_be_removed
            .bearer_context[s11_modify_bearer_request
                                ->bearer_contexts_to_be_removed
                                .num_bearer_context]
            .eps_bearer_id = bearer_context_to_establish->ebi;
        s11_modify_bearer_request->bearer_contexts_to_be_removed
            .bearer_context[s11_modify_bearer_request
                                ->bearer_contexts_to_be_removed
                                .num_bearer_context]
            .cause.cause_value = NO_RESOURCES_AVAILABLE;
        s11_modify_bearer_request->bearer_contexts_to_be_removed
            .num_bearer_context++;
      } else {
        OAILOG_WARNING(LOG_MME_APP,
                       "Not adding EBI %d as bearer context to be removed for "
                       "UE " MME_UE_S1AP_ID_FMT
                       ", since ESM status is not ACTIVE yet %d. \n",
                       bearer_context_to_establish->ebi,
                       ue_session_pool->privates.mme_ue_s1ap_id,
                       bearer_context_to_establish->esm_ebr_context.status);
      }
    }
  }

  OAI_GCC_DIAG_OFF(pointer - to - int - cast);
  s11_modify_bearer_request->sender_fteid_for_cp.teid =
      ue_session_pool->privates.fields.mme_teid_s11;
  OAI_GCC_DIAG_ON(pointer - to - int - cast);
  s11_modify_bearer_request->sender_fteid_for_cp.interface_type = S11_MME_GTP_C;

  mme_config_read_lock(&mme_config);
  if (mme_config.ip.s11_mme_v4.s_addr) {
    s11_modify_bearer_request->sender_fteid_for_cp.ipv4_address =
        mme_config.ip.s11_mme_v4;
    s11_modify_bearer_request->sender_fteid_for_cp.ipv4 = 1;
  }
  if (memcmp(&mme_config.ip.s11_mme_v6.s6_addr, (void *)&in6addr_any,
             sizeof(mme_config.ip.s11_mme_v6.s6_addr)) != 0) {
    memcpy(s11_modify_bearer_request->sender_fteid_for_cp.ipv6_address.s6_addr,
           mme_config.ip.s11_mme_v6.s6_addr,
           sizeof(mme_config.ip.s11_mme_v6.s6_addr));
    s11_modify_bearer_request->sender_fteid_for_cp.ipv6 = 1;
  }
  mme_config_unlock(&mme_config);

  /** These should already be removed.. */
  s11_modify_bearer_request->mme_fq_csid.node_id_type =
      GLOBAL_UNICAST_IPv4;                          // TO DO
  s11_modify_bearer_request->mme_fq_csid.csid = 0;  // TO DO ...
  memset(&s11_modify_bearer_request->indication_flags, 0,
         sizeof(s11_modify_bearer_request->indication_flags));  // TO DO
  s11_modify_bearer_request->rat_type = RAT_EUTRAN;
  s11_modify_bearer_request->internal_flags =
      flags; /**< Will be copied into the transactions and evaluated with the
                response. */
  /*
   * S11 stack specific parameter. Not used in standalone EPC mode.
   */
  s11_modify_bearer_request->trxn = NULL;
  /** Update the bearer state with Modify Bearer Response, not here. */
  // todo: apn restrictions!
  itti_send_msg_to_task(TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Method to send the S1AP MME Status Transfer to the target-eNB.
 * Will not make any changes in the UE context.
 * No F-Container will/needs to be stored temporarily.
 */
void mme_app_send_s1ap_mme_status_transfer(
    mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id,
    uint32_t enb_id, status_transfer_bearer_list_t *status_transfer) {
  MessageDef *message_p = NULL;

  OAILOG_FUNC_IN(LOG_MME_APP);
  /**
   * Prepare a S1AP ITTI message without changing the UE context.
   */
  message_p = itti_alloc_new_message(TASK_MME_APP, S1AP_MME_STATUS_TRANSFER);
  DevAssert(message_p != NULL);
  itti_s1ap_status_transfer_t *status_transfer_p =
      &message_p->ittiMsg.s1ap_mme_status_transfer;
  memset(status_transfer_p, 0, sizeof(itti_s1ap_status_transfer_t));
  status_transfer_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  status_transfer_p->enb_ue_s1ap_id =
      enb_ue_s1ap_id; /**< Just ENB_UE_S1AP_ID. */
  /** Set the current enb_id. */
  status_transfer_p->enb_id = enb_id;
  /** Set the E-UTRAN Target-To-Source-Transparent-Container. */
  status_transfer_p->status_transfer_bearer_list = status_transfer;
  // todo: what will the enb_ue_s1ap_ids for single mme s1ap handover will be..
  // ?
  OAILOG_INFO(LOG_MME_APP,
              "Sending S1AP MME_STATUS_TRANSFER command to the target eNodeB "
              "for enbUeS1apId " ENB_UE_S1AP_ID_FMT " and enbId %d. \n",
              enb_ue_s1ap_id, enb_id);
  /** The ENB_ID/Stream information in the UE_Context are still the ones for the
   * source-ENB and the SCTP-UE_ID association is not set yet for the new eNB.
   */
  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0,
                     "MME_APP Sending S1AP MME_STATUS_TRANSFER.");
  /** Sending a message to S1AP. */
  itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Send an S1AP Path Switch Request Failure to the S1AP layer.
 * Not triggering release of resources, everything will stay as it it.
 * The MME_APP ITTI message elements though need to be deallocated.
 */
void mme_app_send_s1ap_path_switch_request_failure(
    mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id,
    sctp_assoc_id_t assoc_id, const S1AP_Cause_PR cause_type) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  /** Send a S1AP Path Switch Request Failure TO THE TARGET ENB. */
  MessageDef *message_p =
      itti_alloc_new_message(TASK_MME_APP, S1AP_PATH_SWITCH_REQUEST_FAILURE);
  DevAssert(message_p != NULL);

  itti_s1ap_path_switch_request_failure_t *s1ap_path_switch_request_failure_p =
      &message_p->ittiMsg.s1ap_path_switch_request_failure;
  memset((void *)s1ap_path_switch_request_failure_p, 0,
         sizeof(itti_s1ap_path_switch_request_failure_t));

  /** Set the identifiers. */
  s1ap_path_switch_request_failure_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  s1ap_path_switch_request_failure_p->enb_ue_s1ap_id = enb_ue_s1ap_id;
  s1ap_path_switch_request_failure_p->assoc_id =
      assoc_id; /**< To whatever the new SCTP association is. */
  /** Set the negative cause. */
  s1ap_path_switch_request_failure_p->cause_type = cause_type;

  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0,
                     "MME_APP Sending S1AP PATH_SWITCH_REQUEST_FAILURE");
  /** Sending a message to S1AP. */
  itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Send an S1AP E-RAB Modification confirm to the S1AP layer.
 * More IEs will be added soon.
 */
void mme_app_send_s1ap_e_rab_mofification_confirm(
    const mme_ue_s1ap_id_t mme_ue_s1ap_id,
    const enb_ue_s1ap_id_t enb_ue_s1ap_id,
    const mme_app_s1ap_proc_modify_bearer_ind_t *const proc) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  /** Send a S1AP E-RAB MODIFICATION CONFIRM TO THE ENB. */
  MessageDef *message_p =
      itti_alloc_new_message(TASK_MME_APP, S1AP_E_RAB_MODIFICATION_CNF);
  DevAssert(message_p != NULL);

  itti_s1ap_e_rab_modification_cnf_t *s1ap_e_rab_modification_cnf_p =
      &message_p->ittiMsg.s1ap_e_rab_modification_cnf;

  /** Set the identifiers. */
  s1ap_e_rab_modification_cnf_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  s1ap_e_rab_modification_cnf_p->enb_ue_s1ap_id = enb_ue_s1ap_id;

  for (int i = 0; i < proc->e_rab_modified_list.no_of_items; ++i) {
    s1ap_e_rab_modification_cnf_p->e_rab_modify_list.e_rab_id[i] =
        proc->e_rab_modified_list.e_rab_id[i];
  }
  s1ap_e_rab_modification_cnf_p->e_rab_modify_list.no_of_items =
      proc->e_rab_modified_list.no_of_items;

  for (int i = 0; i < proc->e_rab_failed_to_be_modified_list.no_of_items; ++i) {
    s1ap_e_rab_modification_cnf_p->e_rab_failed_to_modify_list.item[i]
        .e_rab_id = proc->e_rab_failed_to_be_modified_list.item[i].e_rab_id;
    s1ap_e_rab_modification_cnf_p->e_rab_failed_to_modify_list.item[i].cause =
        proc->e_rab_failed_to_be_modified_list.item[i].cause;
  }
  s1ap_e_rab_modification_cnf_p->e_rab_failed_to_modify_list.no_of_items =
      proc->e_rab_failed_to_be_modified_list.no_of_items;

  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0,
                     "MME_APP Sending S1AP E_RAB_MODIFICATION_CONFIRM");
  /** Sending a message to S1AP. */
  itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Send an S1AP MME Configuration Transfer to the S1AP layer.
 */
void mme_app_send_s1ap_mme_configuration_transfer(
    target_type_t target_enb_type, tai_t *target_tai,
    ecgi_t *target_global_enb_id, target_type_t source_enb_type,
    tai_t *source_tai, ecgi_t *source_global_enb_id,
    enb_conf_reply_t *conf_reply) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  /** Send a S1AP Path Switch Request Failure TO THE TARGET ENB. */
  MessageDef *message_p =
      itti_alloc_new_message(TASK_MME_APP, S1AP_CONFIGURATION_TRANSFER);
  DevAssert(message_p != NULL);

  itti_s1ap_configuration_transfer_t *s1ap_mme_configuration_transfer_p =
      &message_p->ittiMsg.s1ap_configuration_transfer;

  /** Set the identifiers. */
  s1ap_mme_configuration_transfer_p->target_enb_type = target_enb_type;
  s1ap_mme_configuration_transfer_p->source_enb_type = source_enb_type;
  s1ap_mme_configuration_transfer_p->target_global_enb_id =
      *target_global_enb_id;
  s1ap_mme_configuration_transfer_p->source_global_enb_id =
      *source_global_enb_id;
  s1ap_mme_configuration_transfer_p->target_tai = *target_tai;
  s1ap_mme_configuration_transfer_p->source_tai = *source_tai;
  s1ap_mme_configuration_transfer_p->conf_reply =
      conf_reply; /** Sending a message to S1AP. */
  itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

////------------------------------------------------------------------------------
// int
// mme_app_modify_bearers(const mme_ue_s1ap_id_t mme_ue_s1ap_id,
// bearer_contexts_to_be_removed_t * bcs_to_be_removed)
//{
//  uint8_t                                 i = 0;
//  /*
//   * Keep the identifier to the default APN
//   */
//  context_identifier_t                    context_identifier = 0;
//  itti_s11_modify_bearer_request_t       *s11_modify_bearer_request = NULL;
//  int                                     rc = RETURNok;
//  // todo: handover flag in operation-identifier?!
//  OAILOG_FUNC_IN (LOG_MME_APP);
//  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id
//  (&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id); if(!ue_context){
//    OAILOG_INFO(LOG_MME_APP, "No UE context is found" MME_UE_S1AP_ID_FMT ".
//    \n", ue_context->privates.mme_ue_s1ap_id); OAILOG_FUNC_RETURN
//    (LOG_MME_APP, RETURNerror);
//  }
//  OAILOG_DEBUG (LOG_MME_APP, "Sending MBR for imsi " IMSI_64_FMT "\n",
//  ue_context->privates.fields.imsi); message_p = itti_alloc_new_message
//  (TASK_MME_APP, S11_MODIFY_BEARER_REQUEST); AssertFatal (message_p ,
//  "itti_alloc_new_message Failed");
//  /** Go through all PDN Contexts and for each PDN context, check for not
//  activated bearers. */ pdn_context_t * registered_pdn_ctx; RB_FOREACH
//  (registered_pdn_ctx, PdnContexts, &ue_context->pdn_contexts) {
//    if(registered_pdn_ctx){
//      bearer_context_t * bearer_context = NULL;
//      RB_FOREACH (bearer_context, SessionBearers,
//      &registered_pdn_ctx->session_bearers) {
//        if(bearer_context->bearer_state & BEARER_STATE_ACTIVE){
//          /** Continue to next pdn. */
//          continue;
//        }else{
//          if(bearer_context->bearer_state & BEARER_STATE_ENB_CREATED){
//            /** Found a PDN. Establish the bearer contexts. */
//            OAILOG_INFO(LOG_MME_APP, "Establishing the bearers for UE_CONTEXT
//            for UE " MME_UE_S1AP_ID_FMT " triggered by handover notify (not
//            active but ENB Created). \n",
//            ue_context->privates.mme_ue_s1ap_id);
//            mme_app_send_s11_modify_bearer_req(ue_session_pool, pdn_context);
//            OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
//          }
//        }
//      }
// OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
//}

//------------------------------------------------------------------------------
int mme_app_remove_s10_tunnel_endpoint(teid_t local_teid,
                                       struct sockaddr *peer_ip) {
  OAILOG_FUNC_IN(LOG_MME_APP);

  MessageDef *message_p =
      itti_alloc_new_message(TASK_MME_APP, S10_REMOVE_UE_TUNNEL);
  DevAssert(message_p != NULL);
  message_p->ittiMsg.s10_remove_ue_tunnel.local_teid = local_teid;

  memcpy((void *)&message_p->ittiMsg.s10_remove_ue_tunnel.mme_peer_ip, peer_ip,
         (peer_ip->sa_family == AF_INET) ? sizeof(struct sockaddr_in)
                                         : sizeof(struct sockaddr_in6));

  //  message_p->ittiMsg.s10_remove_ue_tunnel.cause = LOCAL_DETACH;
  if (local_teid == (teid_t)0) {
    OAILOG_DEBUG(LOG_MME_APP,
                 "Sending remove tunnel request for with null teid! \n");
  } else if (!peer_ip->sa_family) {
    OAILOG_DEBUG(LOG_MME_APP,
                 "Sending remove tunnel request for with null peer ip! \n");
  }
  itti_send_msg_to_task(TASK_S10, INSTANCE_DEFAULT, message_p);

  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
}

/*
 * Cu
 */
//------------------------------------------------------------------------------
int mme_app_send_delete_session_request(
    struct ue_context_s *const ue_context, const ebi_t ebi,
    const struct sockaddr *saegw_s11_addr, const teid_t saegw_s11_teid,
    const bool noDelete, const bool handover, const uint8_t internal_flags) {
  MessageDef *message_p = NULL;
  int rc = RETURNok;
  OAILOG_FUNC_IN(LOG_MME_APP);

  message_p = itti_alloc_new_message(TASK_MME_APP, S11_DELETE_SESSION_REQUEST);
  AssertFatal(message_p, "itti_alloc_new_message Failed");
  S11_DELETE_SESSION_REQUEST(message_p).local_teid =
      ue_context->privates.fields.mme_teid_s11;
  S11_DELETE_SESSION_REQUEST(message_p).teid = saegw_s11_teid;
  S11_DELETE_SESSION_REQUEST(message_p).lbi = ebi;            // default bearer
  S11_DELETE_SESSION_REQUEST(message_p).noDelete = noDelete;  // default bearer
  S11_DELETE_SESSION_REQUEST(message_p).noDelete = noDelete;  // default bearer
  S11_DELETE_SESSION_REQUEST(message_p).internal_flags = internal_flags;

  OAI_GCC_DIAG_OFF(pointer - to - int - cast);
  S11_DELETE_SESSION_REQUEST(message_p).sender_fteid_for_cp.teid =
      ue_context->privates.fields.mme_teid_s11;
  OAI_GCC_DIAG_ON(pointer - to - int - cast);
  S11_DELETE_SESSION_REQUEST(message_p).sender_fteid_for_cp.interface_type =
      S11_MME_GTP_C;

  mme_config_read_lock(&mme_config);
  if (saegw_s11_addr->sa_family == AF_INET) {
    S11_DELETE_SESSION_REQUEST(message_p).sender_fteid_for_cp.ipv4 = 1;
    S11_DELETE_SESSION_REQUEST(message_p).sender_fteid_for_cp.ipv4_address =
        mme_config.ip.s11_mme_v4;
  } else {
    S11_DELETE_SESSION_REQUEST(message_p).sender_fteid_for_cp.ipv6 = 1;
    memcpy((void *)&S11_DELETE_SESSION_REQUEST(message_p)
               .sender_fteid_for_cp.ipv6_address,
           (void *)&mme_config.ip.s11_mme_v6, sizeof(mme_config.ip.s11_mme_v6));
  }
  mme_config_unlock(&mme_config);

  if (handover) {
    S11_DELETE_SESSION_REQUEST(message_p).indication_flags.oi = 0x0;
    S11_DELETE_SESSION_REQUEST(message_p).indication_flags.si = 0x1;
  } else {
    S11_DELETE_SESSION_REQUEST(message_p).indication_flags.oi = 0x1;
    S11_DELETE_SESSION_REQUEST(message_p).indication_flags.si = 0x0;
  }

  /*
   * S11 stack specific parameter. Not used in standalone epc mode
   */
  S11_DELETE_SESSION_REQUEST(message_p).trxn = NULL;
  memcpy((void *)&S11_DELETE_SESSION_REQUEST(message_p).edns_peer_ip,
         saegw_s11_addr,
         saegw_s11_addr->sa_family == AF_INET ? sizeof(struct sockaddr_in)
                                              : sizeof(struct sockaddr_in6));

  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0,
                     "0  S11_DELETE_SESSION_REQUEST teid %u lbi %u",
                     S11_DELETE_SESSION_REQUEST(message_p).teid,
                     S11_DELETE_SESSION_REQUEST(message_p).lbi);

  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0,
                     "0 S11_DELETE_SESSION_REQUEST imsi " IMSI_64_FMT,
                     ue_contextP->imsi);
  rc = itti_send_msg_to_task(TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN(LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
/**
 * Send an S1AP Handover Cancel Acknowledge to the S1AP layer.
 */
void mme_app_send_s1ap_handover_cancel_acknowledge(
    mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id,
    sctp_assoc_id_t assoc_id) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  MessageDef *message_p =
      itti_alloc_new_message(TASK_MME_APP, S1AP_HANDOVER_CANCEL_ACKNOWLEDGE);
  DevAssert(message_p != NULL);

  itti_s1ap_handover_cancel_acknowledge_t *s1ap_handover_cancel_acknowledge_p =
      &message_p->ittiMsg.s1ap_handover_cancel_acknowledge;
  memset((void *)s1ap_handover_cancel_acknowledge_p, 0,
         sizeof(itti_s1ap_handover_cancel_acknowledge_t));

  /** Set the identifiers. */
  s1ap_handover_cancel_acknowledge_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  s1ap_handover_cancel_acknowledge_p->enb_ue_s1ap_id = enb_ue_s1ap_id;
  s1ap_handover_cancel_acknowledge_p->assoc_id = assoc_id;

  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0,
                     "MME_APP Sending S1AP HANDOVER_CANCEL_ACKNOWLEDGE");
  /** Sending a message to S10. */
  itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Send an S1AP Handover Preparation Failure to the S1AP layer.
 * Not triggering release of resources, everything will stay as it it.
 * The MME_APP ITTI message elements though need to be deallocated.
 */
void mme_app_send_s1ap_handover_preparation_failure(
    mme_ue_s1ap_id_t mme_ue_s1ap_id, enb_ue_s1ap_id_t enb_ue_s1ap_id,
    sctp_assoc_id_t assoc_id, enum s1cause cause) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  /** Send a S1AP HANDOVER PREPARATION FAILURE TO THE SOURCE ENB. */
  MessageDef *message_p =
      itti_alloc_new_message(TASK_MME_APP, S1AP_HANDOVER_PREPARATION_FAILURE);
  DevAssert(message_p != NULL);
  DevAssert(cause != S1AP_SUCCESSFUL_HANDOVER);

  itti_s1ap_handover_preparation_failure_t
      *s1ap_handover_preparation_failure_p =
          &message_p->ittiMsg.s1ap_handover_preparation_failure;
  memset((void *)s1ap_handover_preparation_failure_p, 0,
         sizeof(itti_s1ap_handover_preparation_failure_t));

  /** Set the identifiers. */
  s1ap_handover_preparation_failure_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  s1ap_handover_preparation_failure_p->enb_ue_s1ap_id = enb_ue_s1ap_id;
  s1ap_handover_preparation_failure_p->assoc_id = assoc_id;
  /** Set the negative cause. */
  s1ap_handover_preparation_failure_p->cause = cause;

  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0,
                     "MME_APP Sending S1AP HANDOVER_PREPARATION_FAILURE");
  /** Sending a message to S1AP. */
  itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
void notify_s1ap_new_ue_mme_s1ap_id_association(
    const sctp_assoc_id_t assoc_id, const enb_ue_s1ap_id_t enb_ue_s1ap_id,
    const mme_ue_s1ap_id_t mme_ue_s1ap_id) {
  MessageDef *message_p = NULL;
  itti_mme_app_s1ap_mme_ue_id_notification_t *notification_p = NULL;

  OAILOG_FUNC_IN(LOG_MME_APP);

  message_p =
      itti_alloc_new_message(TASK_MME_APP, MME_APP_S1AP_MME_UE_ID_NOTIFICATION);
  notification_p = &message_p->ittiMsg.mme_app_s1ap_mme_ue_id_notification;
  memset(notification_p, 0, sizeof(itti_mme_app_s1ap_mme_ue_id_notification_t));
  notification_p->enb_ue_s1ap_id = enb_ue_s1ap_id;
  notification_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  notification_p->sctp_assoc_id = assoc_id;

  itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, message_p);
  OAILOG_DEBUG(LOG_MME_APP,
               " Sent MME_APP_S1AP_MME_UE_ID_NOTIFICATION to S1AP for UE Id %u "
               "and enbUeS1apId %u\n",
               notification_p->mme_ue_s1ap_id, notification_p->enb_ue_s1ap_id);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
int mme_app_send_s11_create_bearer_rsp(
    teid_t mme_teid_s11, teid_t s_gw_teid_s11_s4,
    gtpv2c_cause_value_t cause_value, void *trxn,
    bearer_contexts_to_be_created_t *bcs_tbc) {
  /*
   * Keep the identifier to the default APN.
   */
  context_identifier_t context_identifier = 0;
  MessageDef *message_p = NULL;
  int rc = RETURNok;

  // todo: handover flag in operation-identifier?!

  OAILOG_FUNC_IN(LOG_MME_APP);
  DevAssert(bcs_tbc);

  message_p = itti_alloc_new_message(TASK_MME_APP, S11_CREATE_BEARER_RESPONSE);
  AssertFatal(message_p, "itti_alloc_new_message Failed");
  itti_s11_create_bearer_response_t *s11_create_bearer_response =
      &message_p->ittiMsg.s11_create_bearer_response;
  s11_create_bearer_response->local_teid = mme_teid_s11;
  s11_create_bearer_response->trxn = trxn;
  s11_create_bearer_response->cause.cause_value = 0;

  /** Check if a direct reject-cause is given, if so set it for all bearer
   * contexts. */
  /** Iterate through the bearers to be created and check which ones where
   * established. */
  for (int num_bc = 0; num_bc < bcs_tbc->num_bearer_context; num_bc++) {
    if (cause_value && cause_value != REQUEST_ACCEPTED) {
      bcs_tbc->bearer_context[num_bc].cause.cause_value = cause_value;
      if (!s11_create_bearer_response->cause.cause_value)
        s11_create_bearer_response->cause.cause_value = cause_value;
    }
    /** Check if the bearer is a session bearer. */
    if (bcs_tbc->bearer_context[num_bc].cause.cause_value == REQUEST_ACCEPTED) {
      if (s11_create_bearer_response->cause.cause_value == REQUEST_REJECTED)
        s11_create_bearer_response->cause.cause_value =
            REQUEST_ACCEPTED_PARTIALLY;
      if (!s11_create_bearer_response->cause.cause_value)
        s11_create_bearer_response->cause.cause_value = REQUEST_ACCEPTED;
    } else { /**< Reject or empty. */
      if (s11_create_bearer_response->cause.cause_value == REQUEST_ACCEPTED)
        s11_create_bearer_response->cause.cause_value =
            REQUEST_ACCEPTED_PARTIALLY;
      if (!s11_create_bearer_response->cause.cause_value)
        s11_create_bearer_response->cause.cause_value = REQUEST_REJECTED;
    }
    /** The error case is at least PARTIALLY ACCEPTED. */
    s11_create_bearer_response->bearer_contexts.bearer_context[num_bc]
        .cause.cause_value = bcs_tbc->bearer_context[num_bc].cause.cause_value;
    if (s11_create_bearer_response->cause.cause_value == REQUEST_ACCEPTED)
      s11_create_bearer_response->bearer_contexts.bearer_context[num_bc]
          .eps_bearer_id = bcs_tbc->bearer_context[num_bc].eps_bearer_id;
    //  FTEID eNB
    memcpy(&s11_create_bearer_response->bearer_contexts.bearer_context[num_bc]
                .s1u_enb_fteid,
           &bcs_tbc->bearer_context[num_bc].s1u_enb_fteid,
           sizeof(bcs_tbc->bearer_context[num_bc].s1u_enb_fteid));
    // FTEID SGW S1U
    memcpy(&s11_create_bearer_response->bearer_contexts.bearer_context[num_bc]
                .s1u_sgw_fteid,
           &bcs_tbc->bearer_context[num_bc].s1u_sgw_fteid,
           sizeof(bcs_tbc->bearer_context[num_bc]
                      .s1u_sgw_fteid));  ///< This IE shall be sent on the S11
                                         ///< interface. It shall be used
    s11_create_bearer_response->bearer_contexts.num_bearer_context++;
  }
  s11_create_bearer_response->teid = s_gw_teid_s11_s4;
  ////  ////  mme_config_read_lock (&mme_config);
  ////////  session_request_p->peer_ip = mme_config.ipv4.sgw_s11;
  ////////  mme_config_unlock (&mme_config);
  //////  // TODO perform SGW selection
  //////  // Actually, since S and P GW are bundled together, there is no PGW
  /// selection (based on PGW id in ULA, or DNS query based on FQDN) /  if (1) {
  ////    // TODO prototype may change
  ////    mme_app_select_sgw(serving_tai, &session_request_p->peer_ip);
  ////  }

  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0,
                     "0 S11_CREATE_BEARER_RESPONSE teid %u",
                     s11_create_bearer_response->teid);
  itti_send_msg_to_task(TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN(LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
int mme_app_send_s11_update_bearer_rsp(
    teid_t mme_teid_s11, teid_t s_gw_teid_s11_s4,
    gtpv2c_cause_value_t extra_cause, void *trxn,
    bearer_contexts_to_be_updated_t *bcs_tbu) {
  /*
   * Keep the identifier to the default APN.
   */
  context_identifier_t context_identifier = 0;
  MessageDef *message_p = NULL;
  int rc = RETURNok;

  // todo: handover flag in operation-identifier?!
  OAILOG_FUNC_IN(LOG_MME_APP);
  DevAssert(bcs_tbu);

  message_p = itti_alloc_new_message(TASK_MME_APP, S11_UPDATE_BEARER_RESPONSE);
  AssertFatal(message_p, "itti_alloc_new_message Failed");
  itti_s11_update_bearer_response_t *s11_update_bearer_response =
      &message_p->ittiMsg.s11_update_bearer_response;
  s11_update_bearer_response->local_teid = mme_teid_s11;
  s11_update_bearer_response->trxn = trxn;
  s11_update_bearer_response->cause.cause_value = 0;
  // todo: pco, uli
  /** Iterate through the bearers to be created and check which ones where
   * established. */
  for (int num_bc = 0; num_bc < bcs_tbu->num_bearer_context; num_bc++) {
    if (bcs_tbu->bearer_context[num_bc].cause.cause_value == REQUEST_ACCEPTED) {
      if (s11_update_bearer_response->cause.cause_value == REQUEST_REJECTED)
        s11_update_bearer_response->cause.cause_value =
            REQUEST_ACCEPTED_PARTIALLY;
      else if (!s11_update_bearer_response->cause.cause_value)
        s11_update_bearer_response->cause.cause_value = REQUEST_ACCEPTED;
      else {
        /** Leave the code. */
      }
    } else { /**< Reject or empty. */
      if (s11_update_bearer_response->cause.cause_value == REQUEST_ACCEPTED)
        s11_update_bearer_response->cause.cause_value =
            REQUEST_ACCEPTED_PARTIALLY;
      else if (!s11_update_bearer_response->cause.cause_value)
        s11_update_bearer_response->cause.cause_value =
            (bcs_tbu->bearer_context[num_bc].cause.cause_value)
                ? bcs_tbu->bearer_context[num_bc].cause.cause_value
                : REQUEST_REJECTED;
      else {
        /** Leave the code. */
      }
    }
    OAILOG_DEBUG(LOG_MME_APP,
                 "Bearer with ebi %d updated with result code %d. New UBResp "
                 "cause value %d. \n",
                 bcs_tbu->bearer_context[num_bc].eps_bearer_id,
                 bcs_tbu->bearer_context[num_bc].cause,
                 s11_update_bearer_response->cause.cause_value);

    /** The error case is at least PARTIALLY ACCEPTED. */
    s11_update_bearer_response->bearer_contexts.bearer_context[num_bc]
        .eps_bearer_id = bcs_tbu->bearer_context[num_bc].eps_bearer_id;
    s11_update_bearer_response->bearer_contexts.bearer_context[num_bc]
        .cause.cause_value =
        (bcs_tbu->bearer_context[num_bc].cause.cause_value)
            ? bcs_tbu->bearer_context[num_bc].cause.cause_value
            : REQUEST_REJECTED;
    // todo: pco
    /** No FTEIDs to be set. */
    s11_update_bearer_response->bearer_contexts.num_bearer_context++;
  }
  s11_update_bearer_response->teid = s_gw_teid_s11_s4;
  if (extra_cause) s11_update_bearer_response->cause.cause_value = extra_cause;
  ////  ////  mme_config_read_lock (&mme_config);
  ////////  session_request_p->peer_ip = mme_config.ipv4.sgw_s11;
  ////////  mme_config_unlock (&mme_config);
  //////  // TODO perform SGW selection
  //////  // Actually, since S and P GW are bundled together, there is no PGW
  /// selection (based on PGW id in ULA, or DNS query based on FQDN) /  if (1) {
  ////    // TODO prototype may change
  ////    mme_app_select_sgw(serving_tai, &session_request_p->peer_ip);
  ////  }

  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0,
                     "0 S11_UPDATE_BEARER_RESPONSE teid %u",
                     s11_update_bearer_response->teid);
  itti_send_msg_to_task(TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN(LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
int mme_app_send_s11_delete_bearer_rsp(teid_t mme_teid_s11,
                                       teid_t s_gw_teid_s11_s4,
                                       gtpv2c_cause_value_t cause_value,
                                       void *trxn, ebi_list_t *ebi_list) {
  /*
   * Keep the identifier to the default APN.
   */
  MessageDef *message_p = NULL;
  int rc = RETURNok;
  // todo: handover flag in operation-identifier?!
  OAILOG_FUNC_IN(LOG_MME_APP);
  DevAssert(ebi_list);
  if (cause_value == 0) cause_value = REQUEST_REJECTED;

  message_p = itti_alloc_new_message(TASK_MME_APP, S11_DELETE_BEARER_RESPONSE);
  AssertFatal(message_p, "itti_alloc_new_message Failed");
  itti_s11_delete_bearer_response_t *s11_delete_bearer_response =
      &message_p->ittiMsg.s11_delete_bearer_response;
  s11_delete_bearer_response->local_teid = mme_teid_s11;
  s11_delete_bearer_response->trxn = trxn;
  s11_delete_bearer_response->cause.cause_value = cause_value;
  /** Not setting the default ebi. */

  /** Iterate through the bearers to be created and check which ones where
   * established. */
  for (int num_ebi = 0; num_ebi < ebi_list->num_ebi; num_ebi++) {
    /** Check if the bearer is a session bearer. */
    s11_delete_bearer_response->bearer_contexts.bearer_context[num_ebi]
        .cause.cause_value = cause_value;
    s11_delete_bearer_response->bearer_contexts.bearer_context[num_ebi]
        .eps_bearer_id = ebi_list->ebis[num_ebi];
    s11_delete_bearer_response->bearer_contexts.num_bearer_context++;
  }
  s11_delete_bearer_response->teid = s_gw_teid_s11_s4;
  ////  ////  mme_config_read_lock (&mme_config);
  ////////  session_request_p->peer_ip = mme_config.ipv4.sgw_s11;
  ////////  mme_config_unlock (&mme_config);
  //////  // TODO perform SGW selection
  //////  // Actually, since S and P GW are bundled together, there is no PGW
  /// selection (based on PGW id in ULA, or DNS query based on FQDN) /  if (1) {
  ////    // TODO prototype may change
  ////    mme_app_select_sgw(serving_tai, &session_request_p->peer_ip);
  ////  }

  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_S11_MME, NULL, 0,
                     "0 S11_DELETE_BEARER_RESPONSE teid %u",
                     s11_delete_bearer_response->teid);
  itti_send_msg_to_task(TASK_S11, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN(LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
void mme_app_itti_nas_context_response(ue_context_t *ue_context,
                                       nas_s10_context_t *s10_context_val) {
  MessageDef *message_p = NULL;
  int rc = RETURNok;

  OAILOG_FUNC_IN(LOG_MME_APP);
  DevAssert(ue_context);
  /*
   * Send NAS context response procedure.
   * Build a NAS_CONTEXT_INFO message and fill it.
   * Depending on the cause, NAS layer can perform an TAU_REJECT or move on with
   * the TAU validation. NAS layer.
   *
   * todo: nas_ctx_fail!
   */
  message_p = itti_alloc_new_message(TASK_MME_APP, NAS_CONTEXT_RES);
  itti_nas_context_res_t *nas_context_res = &message_p->ittiMsg.nas_context_res;
  /** Set the cause. */
  /** Set the UE identifiers. */
  nas_context_res->ue_id = ue_context->privates.mme_ue_s1ap_id;
  /** Fill the elements of the NAS message from S10 CONTEXT_RESPONSE. */

  /** todo: Convert the GTPv2c IMSI struct to the NAS IMSI struct. */
  nas_context_res->imsi = ue_context->privates.fields.imsi;
  //  memset (&(ue_context->pending_pdn_connectivity_req_imsi), 0, 16); /**<
  //  IMSI in create session request. */ memcpy
  //  (&(ue_context->pending_pdn_connectivity_req_imsi),
  //  &(s10_context_response_pP->imsi.digit),
  //  s10_context_response_pP->imsi.length);
  //  ue_context->pending_pdn_connectivity_req_imsi_length =
  //  s10_context_response_pP->imsi.length;
  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0,
                     "0 NAS_CONTEXT_RES sgw_s1u_teid %u ebi %u qci %u prio %u",
                     current_bearer_p->s_gw_fteid_s1u.teid, bearer_id,
                     current_bearer_p->qci, current_bearer_p->priority_level);
  rc = itti_send_msg_to_task(TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
}

//------------------------------------------------------------------------------
void mme_app_itti_nas_pdn_connectivity_response(
    mme_ue_s1ap_id_t ue_id, const ebi_t default_ebi,
    const gtpv2c_cause_value_t gtpv2c_cause_value) {
  OAILOG_FUNC_IN(LOG_MME_APP);

  MessageDef *message_p = NULL;
  int rc = RETURNok;
  OAILOG_INFO(LOG_MME_APP,
              "Informing the NAS layer about the received "
              "CREATE_SESSION_RESPONSE for UE " MME_UE_S1AP_ID_FMT ". \n",
              ue_id);
  message_p = itti_alloc_new_message(TASK_MME_APP, NAS_PDN_CONNECTIVITY_RSP);
  itti_nas_pdn_connectivity_rsp_t *nas_pdn_connectivity_rsp =
      &message_p->ittiMsg.nas_pdn_connectivity_rsp;
  nas_pdn_connectivity_rsp->ue_id = ue_id;
  nas_pdn_connectivity_rsp->linked_ebi = default_ebi;
  nas_pdn_connectivity_rsp->esm_cause =
      (gtpv2c_cause_value == REQUEST_ACCEPTED ||
       gtpv2c_cause_value == REQUEST_ACCEPTED_PARTIALLY)
          ? ESM_CAUSE_SUCCESS
          : ESM_CAUSE_NETWORK_FAILURE;
  if (nas_pdn_connectivity_rsp->esm_cause == ESM_CAUSE_NETWORK_FAILURE) {
    if (gtpv2c_cause_value == NO_RESOURCES_AVAILABLE)
      nas_pdn_connectivity_rsp->esm_cause = ESM_CAUSE_INSUFFICIENT_RESOURCES;
  }

  itti_send_msg_to_task(TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
void mme_app_itti_forward_relocation_response(
    ue_context_t *ue_context, struct PdnContexts *pdn_contexts,
    mme_app_s10_proc_mme_handover_t *s10_handover_proc,
    bstring target_to_source_container) {
  MessageDef *message_p = NULL;
  int rc = RETURNok;

  OAILOG_FUNC_IN(LOG_MME_APP);
  DevAssert(ue_context);
  OAILOG_INFO(LOG_MME_APP,
              "Sending Forward Relocation Response to source MME for "
              "UE " MME_UE_S1AP_ID_FMT ". \n",
              ue_context->privates.mme_ue_s1ap_id);

  message_p =
      itti_alloc_new_message(TASK_MME_APP, S10_FORWARD_RELOCATION_RESPONSE);
  DevAssert(message_p != NULL);

  itti_s10_forward_relocation_response_t *forward_relocation_response_p =
      &message_p->ittiMsg.s10_forward_relocation_response;

  /** Get the Handov
  /** Set the target S10 TEID. */
  forward_relocation_response_p->teid =
      s10_handover_proc->remote_mme_teid
          .teid; /**< Only a single target-MME TEID can exist at a time. */

  /**
   * todo: Get the MME from the origin TAI.
   * Currently only one MME is supported.
   */
  memcpy((void *)&forward_relocation_response_p->mme_peer_ip,
         s10_handover_proc->proc.peer_ip,
         (s10_handover_proc->proc.peer_ip->sa_family == AF_INET
              ? sizeof(struct sockaddr_in)
              : sizeof(struct sockaddr_in6)));

  forward_relocation_response_p->trxn =
      s10_handover_proc->forward_relocation_trxn;
  /** Set the cause. */
  forward_relocation_response_p->cause.cause_value = REQUEST_ACCEPTED;
  /** Set all bearers. */
  pdn_context_t *registered_pdn_ctx = NULL;
  RB_FOREACH(registered_pdn_ctx, PdnContexts, pdn_contexts) {
    if (!registered_pdn_ctx) continue;
    bearer_context_new_t *bearer_context_setup = NULL;
    STAILQ_FOREACH(bearer_context_setup, &registered_pdn_ctx->session_bearers,
                   entries) {
      if (!bearer_context_setup) continue;
      /** EBI. */
      forward_relocation_response_p->handovered_bearers
          .bearer_context[forward_relocation_response_p->handovered_bearers
                              .num_bearer_context]
          .eps_bearer_id = bearer_context_setup->ebi;
      forward_relocation_response_p->handovered_bearers
          .bearer_context[forward_relocation_response_p->handovered_bearers
                              .num_bearer_context]
          .cause.cause_value = REQUEST_ACCEPTED;
      forward_relocation_response_p->handovered_bearers.num_bearer_context++;
    }
  }

  /** Set the Source MME_S10_FTEID the same as in S11. */
  OAI_GCC_DIAG_OFF(pointer - to - int - cast);
  forward_relocation_response_p->s10_target_mme_teid.teid =
      ue_context->privates.fields
          .local_mme_teid_s10; /**< This one also sets the context pointer. */
  OAI_GCC_DIAG_ON(pointer - to - int - cast);
  forward_relocation_response_p->s10_target_mme_teid.interface_type =
      S10_MME_GTP_C;
  /** Set the MME address. */
  mme_config_read_lock(&mme_config);
  if (mme_config.ip.s11_mme_v4.s_addr) {
    forward_relocation_response_p->s10_target_mme_teid.ipv4_address =
        mme_config.ip.s11_mme_v4;
    forward_relocation_response_p->s10_target_mme_teid.ipv4 = 1;
  }
  if (memcmp(&mme_config.ip.s11_mme_v6.s6_addr, (void *)&in6addr_any,
             sizeof(mme_config.ip.s11_mme_v6.s6_addr)) != 0) {
    memcpy(
        forward_relocation_response_p->s10_target_mme_teid.ipv6_address.s6_addr,
        mme_config.ip.s11_mme_v6.s6_addr,
        sizeof(mme_config.ip.s11_mme_v6.s6_addr));
    forward_relocation_response_p->s10_target_mme_teid.ipv6 = 1;
  }
  mme_config_unlock(&mme_config);

  /** Set S10 F-Cause. */
  forward_relocation_response_p->f_cause.fcause_type = FCAUSE_S1AP;
  forward_relocation_response_p->f_cause.fcause_s1ap_type = FCAUSE_S1AP_RNL;
  forward_relocation_response_p->f_cause.fcause_value =
      0;  // todo: set these values later.. currently just RNL

  /** Set the E-UTRAN container. */
  forward_relocation_response_p->eutran_container.container_type = 3;
  /** Just link the bstring. Will be purged in the S10 message. */
  forward_relocation_response_p->eutran_container.container_value =
      target_to_source_container;

  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0,
                     "MME_APP Sending S10 FORWARD_RELOCATION_RESPONSE");
  OAILOG_INFO(LOG_MME_APP,
              "Successfully prepared Forward Relocation Response to source MME "
              "for UE " MME_UE_S1AP_ID_FMT " Sending to S10. \n",
              ue_context->privates.mme_ue_s1ap_id);

  /**
   * Sending a message to S10.
   * No changes in the contexts, flags, timers, etc.. needed.
   */
  itti_send_msg_to_task(TASK_S10, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
/**
 * Send an S10 Forward Relocation response with error cause.
 * It shall not trigger creating a local S10 tunnel.
 * Parameter is the TEID & IP of the SOURCE-MME.
 */
void mme_app_send_s10_forward_relocation_response_err(
    teid_t mme_source_s10_teid, struct sockaddr *mme_source_ip_address,
    void *trxn, gtpv2c_cause_value_t gtpv2cCause) {
  OAILOG_FUNC_IN(LOG_MME_APP);

  /** Send a Forward Relocation RESPONSE with error cause: RELOCATION_FAILURE.
   */
  MessageDef *message_p =
      itti_alloc_new_message(TASK_MME_APP, S10_FORWARD_RELOCATION_RESPONSE);
  DevAssert(message_p != NULL);

  itti_s10_forward_relocation_response_t *forward_relocation_response_p =
      &message_p->ittiMsg.s10_forward_relocation_response;

  /**
   * Set the TEID of the source MME.
   * No need to set local_teid since no S10 tunnel will be created in error
   * case.
   */
  forward_relocation_response_p->teid = mme_source_s10_teid;
  /** Set the IPv4 address of the source MME. */
  memcpy((void *)&forward_relocation_response_p->mme_peer_ip,
         mme_source_ip_address,
         mme_source_ip_address->sa_family == AF_INET
             ? sizeof(struct sockaddr_in)
             : sizeof(struct sockaddr_in6));
  forward_relocation_response_p->cause.cause_value = gtpv2cCause;
  forward_relocation_response_p->trxn = trxn;
  /** Sending a message to S10. */
  itti_send_msg_to_task(TASK_S10, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

/**
 * Send a NAS Context Response with error code.
 * It shall not trigger a TAU/Attach reject at the local (TARGET) MME, since no
 * UE context information could be retrieved.
 */
//------------------------------------------------------------------------------
void _mme_app_send_nas_context_response_err(mme_ue_s1ap_id_t ueId,
                                            gtpv2c_cause_value_t cause_val) {
  MessageDef *message_p = NULL;
  OAILOG_FUNC_IN(LOG_MME_APP);

  /** Send a Context RESPONSE with error cause. */
  message_p = itti_alloc_new_message(TASK_MME_APP, NAS_CONTEXT_FAIL);
  DevAssert(message_p != NULL);
  itti_nas_context_fail_t *nas_context_fail =
      &message_p->ittiMsg.nas_context_fail;
  memset((void *)nas_context_fail, 0, sizeof(itti_nas_context_fail_t));

  /** Set the cause. */
  nas_context_fail->cause = cause_val;
  /** Set the UE identifiers. */
  nas_context_fail->ue_id = ueId;
  MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0,
                     "MME_APP Sending NAS NAS_CONTEXT_FAIL to NAS");
  /** Sending a message to NAS. */
  itti_send_msg_to_task(TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}
