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

/*! \file itti_free_defined_msg.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bstrlib.h"

#include "3gpp_23.003.h"
#include "3gpp_24.007.h"
#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "3gpp_33.401.h"
#include "3gpp_36.331.h"
#include "3gpp_36.401.h"
#include "assertions.h"
#include "common_defs.h"
#include "common_types.h"
#include "dynamic_memory_check.h"
#include "intertask_interface.h"
#include "itti_free_defined_msg.h"
#include "security_types.h"

//------------------------------------------------------------------------------
void itti_free_msg_content(MessageDef* const message_p) {
  switch (ITTI_MSG_ID(message_p)) {
    case ASYNC_SYSTEM_COMMAND: {
      if (ASYNC_SYSTEM_COMMAND(message_p).system_command) {
        bdestroy_wrapper(&ASYNC_SYSTEM_COMMAND(message_p).system_command);
      }
    } break;

    /* NAS ESM. */
    case NAS_ESM_DATA_IND:
      bdestroy_wrapper(&message_p->ittiMsg.nas_esm_data_ind.req);
      break;
    case NAS_ESM_DETACH_IND:
    case NAS_PDN_CONFIG_RSP:
    case NAS_PDN_CONFIG_FAIL:
      /** Nothing to do. */
      break;

    case NAS_PDN_CONNECTIVITY_RSP:
    case NAS_PDN_DISCONNECT_REQ:
    case NAS_PDN_DISCONNECT_RSP:
      /* Nothing to do. */
      break;

      break;
    case NAS_ACTIVATE_EPS_BEARER_CTX_REQ:
    case NAS_MODIFY_EPS_BEARER_CTX_REQ:
      /** Not removing the uintptr.
       * The structure will be removed together with the MME_APP S11 procedure.
       */
      break;
    case NAS_ACTIVATE_EPS_BEARER_CTX_CNF:
    case NAS_ACTIVATE_EPS_BEARER_CTX_REJ:
      /* Nothing to do. */
      break;

    case NAS_MODIFY_EPS_BEARER_CTX_CNF:
    case NAS_MODIFY_EPS_BEARER_CTX_REJ:
      /* Nothing to do. */
      break;

    case NAS_DEACTIVATE_EPS_BEARER_CTX_REQ:
    case NAS_DEACTIVATE_EPS_BEARER_CTX_CNF:
      /** Nothing to do. */
      break;
    /* NAS EMM. */
    case NAS_DOWNLINK_DATA_REQ:
      bdestroy_wrapper(&message_p->ittiMsg.nas_dl_data_req.nas_msg);
      AssertFatal(NULL == message_p->ittiMsg.nas_dl_data_req.nas_msg,
                  "TODO clean pointer");
      break;
    case NAS_ERAB_SETUP_REQ:
      bdestroy_wrapper(&message_p->ittiMsg.nas_erab_setup_req.nas_msg);
      break;
    case NAS_ERAB_MODIFY_REQ:
      bdestroy_wrapper(&message_p->ittiMsg.nas_erab_modify_req.nas_msg);
      break;
    case NAS_ERAB_RELEASE_REQ:
      bdestroy_wrapper(&message_p->ittiMsg.nas_erab_release_req.nas_msg);
      break;
    case NAS_CONTEXT_REQ:
      bdestroy_wrapper(&message_p->ittiMsg.nas_context_req.nas_msg);
      break;
    case NAS_CONTEXT_RES:
    case NAS_CONTEXT_FAIL:
      // DO nothing
      break;
    case NAS_CONNECTION_ESTABLISHMENT_CNF:
      bdestroy_wrapper(&message_p->ittiMsg.nas_conn_est_cnf.nas_msg);
      AssertFatal(NULL == message_p->ittiMsg.nas_conn_est_cnf.nas_msg,
                  "TODO clean pointer");
      break;
    case NAS_DL_DATA_CNF:
      // DO nothing
      break;
    case NAS_DL_DATA_REJ:
      bdestroy_wrapper(&message_p->ittiMsg.nas_dl_data_rej.nas_msg);
      AssertFatal(NULL == message_p->ittiMsg.nas_dl_data_rej.nas_msg,
                  "TODO clean pointer");
      break;

    case NAS_INITIAL_UE_MESSAGE:
      bdestroy_wrapper(
          &message_p->ittiMsg.nas_initial_ue_message.nas.initial_nas_msg);
      AssertFatal(
          NULL == message_p->ittiMsg.nas_initial_ue_message.nas.initial_nas_msg,
          "TODO clean pointer");
      break;
    case NAS_UPLINK_DATA_IND:
      bdestroy_wrapper(&message_p->ittiMsg.nas_ul_data_ind.nas_msg);
      AssertFatal(NULL == message_p->ittiMsg.nas_ul_data_ind.nas_msg,
                  "TODO clean pointer");
      break;
    case NAS_DETACH_REQ:
      // DO nothing
      break;

    case GTPV1U_CREATE_TUNNEL_REQ:
    case GTPV1U_CREATE_TUNNEL_RESP:
    case GTPV1U_UPDATE_TUNNEL_REQ:
    case GTPV1U_UPDATE_TUNNEL_RESP:
    case GTPV1U_DELETE_TUNNEL_REQ:
    case GTPV1U_DELETE_TUNNEL_RESP:
      // DO nothing
      break;

    case GTPV1U_TUNNEL_DATA_IND:
    case GTPV1U_TUNNEL_DATA_REQ:
      // UNUSED actually
      break;

    case SGI_CREATE_ENDPOINT_REQUEST:
      break;

    case SGI_CREATE_ENDPOINT_RESPONSE: {
      clear_protocol_configuration_options(
          &message_p->ittiMsg.sgi_create_end_point_response.pco);
    } break;

    case SGI_UPDATE_ENDPOINT_REQUEST:
    case SGI_UPDATE_ENDPOINT_RESPONSE:
    case SGI_DELETE_ENDPOINT_REQUEST:
    case SGI_DELETE_ENDPOINT_RESPONSE:
      // DO nothing
      break;

    case MME_APP_INITIAL_CONTEXT_SETUP_RSP:
      // DO nothing
      break;

    case MME_APP_CONNECTION_ESTABLISHMENT_CNF: {
      int num =
          message_p->ittiMsg.mme_app_connection_establishment_cnf.no_of_e_rabs;
      for (int i = 0; i < num; i++) {
        bdestroy_wrapper(
            &message_p->ittiMsg.mme_app_connection_establishment_cnf
                 .transport_layer_address[i]);
        bdestroy_wrapper(&message_p->ittiMsg
                              .mme_app_connection_establishment_cnf.nas_pdu[i]);
        AssertFatal(
            NULL == message_p->ittiMsg.mme_app_connection_establishment_cnf
                        .nas_pdu[i],
            "TODO clean pointer");
      }
    } break;

    case S11_CREATE_SESSION_REQUEST: {
      clear_protocol_configuration_options(
          &message_p->ittiMsg.s11_create_session_request.pco);
      if (message_p->ittiMsg.s11_create_session_request
              .bearer_contexts_to_be_created) {
        free_bearer_contexts_to_be_created(
            &message_p->ittiMsg.s11_create_session_request
                 .bearer_contexts_to_be_created);
      }
    } break;

    case S11_CREATE_SESSION_RESPONSE: {
      if (message_p->ittiMsg.s11_create_session_response.paa)
        free_wrapper(&message_p->ittiMsg.s11_create_session_response.paa);
      clear_protocol_configuration_options(
          &message_p->ittiMsg.s11_create_session_response.pco);
    } break;

    case S11_CREATE_BEARER_REQUEST: {
      clear_protocol_configuration_options(
          &message_p->ittiMsg.s11_create_bearer_request.pco);
      if (message_p->ittiMsg.s11_create_bearer_request.bearer_contexts) {
        free_bearer_contexts_to_be_created(
            &message_p->ittiMsg.s11_create_bearer_request.bearer_contexts);
      }
    } break;

    case S11_UPDATE_BEARER_REQUEST: {
      clear_protocol_configuration_options(
          &message_p->ittiMsg.s11_update_bearer_request.pco);
      if (message_p->ittiMsg.s11_update_bearer_request.bearer_contexts) {
        free_bearer_contexts_to_be_updated(
            &message_p->ittiMsg.s11_update_bearer_request.bearer_contexts);
      }
    } break;

    case S11_DELETE_BEARER_REQUEST: {
      clear_protocol_configuration_options(
          &message_p->ittiMsg.s11_delete_bearer_request.pco);
      //    if(message_p->ittiMsg.s11_delete_bearer_request.failed_bearer_contexts){
      //      free_bearer_contexts_to_be_deleted(&message_p->ittiMsg.s11_delete_bearer_request.failed_bearer_contexts);
      //    }
    } break;

    case S11_BEARER_RESOURCE_COMMAND: {
      clear_traffic_flow_template(
          &message_p->ittiMsg.s11_bearer_resource_command.tad);
    } break;

    case S11_CREATE_BEARER_RESPONSE: {
      clear_protocol_configuration_options(
          &message_p->ittiMsg.s11_create_bearer_response.pco);
    } break;

    case S11_MODIFY_BEARER_REQUEST:
    case S11_MODIFY_BEARER_RESPONSE:
    case S11_DELETE_SESSION_REQUEST:
    case S11_DELETE_BEARER_COMMAND:
    case S11_DELETE_BEARER_FAILURE_INDICATION:
    case S11_BEARER_RESOURCE_FAILURE_INDICATION:
      // DO nothing (trxn)
      break;

    case S11_DELETE_SESSION_RESPONSE: {
      clear_protocol_configuration_options(
          &message_p->ittiMsg.s11_delete_session_response.pco);
    } break;

    case S11_RELEASE_ACCESS_BEARERS_REQUEST:
    case S11_RELEASE_ACCESS_BEARERS_RESPONSE:
    case S11_DOWNLINK_DATA_NOTIFICATION:
    case S11_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE:
      // DO nothing (trxn)
      break;

    case S1AP_UPLINK_NAS_LOG:
    case S1AP_UE_CAPABILITY_IND_LOG:
    case S1AP_INITIAL_CONTEXT_SETUP_LOG:
    case S1AP_NAS_NON_DELIVERY_IND_LOG:
    case S1AP_DOWNLINK_NAS_LOG:
    case S1AP_S1_SETUP_LOG:
    case S1AP_INITIAL_UE_MESSAGE_LOG:
    case S1AP_UE_CONTEXT_RELEASE_REQ_LOG:
    case S1AP_UE_CONTEXT_RELEASE_COMMAND_LOG:
    case S1AP_UE_CONTEXT_RELEASE_LOG:
    case S1AP_E_RABSETUP_RESPONSE_LOG:
    case S1AP_E_RABMODIFY_RESPONSE_LOG:
    case S1AP_E_RABRELEASE_RESPONSE_LOG:
      /* *** */
    case S1AP_HANDOVER_NOTIFY_LOG:
    case S1AP_ENB_STATUS_TRANSFER_LOG:
    case S1AP_HANDOVER_CANCEL_LOG:
    case S1AP_HANDOVER_REQUIRED_LOG:
    case S1AP_PATH_SWITCH_REQUEST_LOG:
    case S1AP_ENB_RESET_LOG:

      /* *** */
    case S1AP_INITIAL_CONTEXT_SETUP_FAILURE_LOG:
    case S1AP_HANDOVER_FAILURE_LOG:
      /* *** */
    case S1AP_HANDOVER_REQUEST_ACKNOWLEDGE_LOG:

      // DO nothing
      break;

    case S1AP_INITIAL_UE_MESSAGE:
      //    free_wrapper(&message_p->ittiMsg.s1ap_initial_ue_message.test_plmn);
      if (message_p->ittiMsg.s1ap_initial_ue_message.nas)
        bdestroy_wrapper(&message_p->ittiMsg.s1ap_initial_ue_message.nas);
      break;

    case S1AP_E_RAB_SETUP_REQ: {
      for (int i = 0; i < message_p->ittiMsg.s1ap_e_rab_setup_req
                              .e_rab_to_be_setup_list.no_of_items;
           i++) {
        bdestroy_wrapper(
            &message_p->ittiMsg.s1ap_e_rab_setup_req.e_rab_to_be_setup_list
                 .item[i]
                 .nas_pdu);
        bdestroy_wrapper(
            &message_p->ittiMsg.s1ap_e_rab_setup_req.e_rab_to_be_setup_list
                 .item[i]
                 .transport_layer_address);
      }
    } break;

    case S1AP_E_RAB_SETUP_RSP: {
      for (int i = 0;
           i <
           message_p->ittiMsg.s1ap_e_rab_setup_rsp.e_rab_setup_list.no_of_items;
           i++) {
        bdestroy_wrapper(
            &message_p->ittiMsg.s1ap_e_rab_setup_rsp.e_rab_setup_list.item[i]
                 .transport_layer_address);
      }
    } break;

    case S1AP_E_RAB_MODIFY_REQ: {
      for (int i = 0; i < message_p->ittiMsg.s1ap_e_rab_modify_req
                              .e_rab_to_be_modified_list.no_of_items;
           i++) {
        bdestroy_wrapper(
            &message_p->ittiMsg.s1ap_e_rab_modify_req.e_rab_to_be_modified_list
                 .item[i]
                 .nas_pdu);
        bdestroy_wrapper(
            &message_p->ittiMsg.s1ap_e_rab_modify_req.e_rab_to_be_modified_list
                 .item[i]
                 .transport_layer_address);
      }
    } break;

    case S1AP_E_RAB_MODIFY_RSP: {
      for (int i = 0; i < message_p->ittiMsg.s1ap_e_rab_modify_rsp
                              .e_rab_modify_list.no_of_items;
           i++) {
        bdestroy_wrapper(
            &message_p->ittiMsg.s1ap_e_rab_modify_rsp.e_rab_modify_list.item[i]
                 .transport_layer_address);
      }
    } break;

    case S1AP_E_RAB_RELEASE_REQ: {
      bdestroy_wrapper(&message_p->ittiMsg.s1ap_e_rab_release_req.nas_pdu);
    } break;

    case S1AP_E_RAB_RELEASE_RSP:
      break;

    case S1AP_ENB_INITIATED_RESET_REQ:
      if (message_p->ittiMsg.s1ap_enb_initiated_reset_req.ue_to_reset_list)
        free_wrapper((void**)&message_p->ittiMsg.s1ap_enb_initiated_reset_req
                         .ue_to_reset_list);
      break;

    case S1AP_ENB_INITIATED_RESET_ACK:
      if (message_p->ittiMsg.s1ap_enb_initiated_reset_ack.ue_to_reset_list)
        free_wrapper((void**)&message_p->ittiMsg.s1ap_enb_initiated_reset_ack
                         .ue_to_reset_list);
      break;
    case S1AP_UE_CAPABILITIES_IND:
      break;

    case S1AP_ENB_DEREGISTERED_IND:
    case S1AP_DEREGISTER_UE_REQ:
    case S1AP_UE_CONTEXT_RELEASE_REQ:
    case S1AP_UE_CONTEXT_RELEASE_COMMAND:
    case S1AP_UE_CONTEXT_RELEASE_COMPLETE:

    case S1AP_PATH_SWITCH_REQUEST:
    case S1AP_PATH_SWITCH_REQUEST_FAILURE:
    case S1AP_HANDOVER_FAILURE:
    case S1AP_HANDOVER_PREPARATION_FAILURE:
    case S1AP_HANDOVER_CANCEL:
    case S1AP_HANDOVER_CANCEL_ACKNOWLEDGE:
    case S1AP_HANDOVER_NOTIFY:
    case S1AP_PAGING:
      // DO nothing
      break;

    case S6A_UPDATE_LOCATION_REQ:
    case S6A_AUTH_INFO_REQ:
    case S6A_AUTH_INFO_ANS:
    case S6A_CANCEL_LOCATION_REQ:
    case S6A_RESET_REQ:
      // DO nothing
      break;
    case S6A_UPDATE_LOCATION_ANS:
      if (message_p->ittiMsg.s6a_update_location_ans.subscription_data)
        free_wrapper((void**)&message_p->ittiMsg.s6a_update_location_ans
                         .subscription_data);
      break;

    case SCTP_INIT_MSG:
      // DO nothing (ipv6_address statically allocated)
      break;

    case S1AP_CONFIGURATION_TRANSFER:
      if (message_p->ittiMsg.s1ap_configuration_transfer.conf_reply) {
        for (int reply_count = 0;
             reply_count < message_p->ittiMsg.s1ap_configuration_transfer
                               .conf_reply->reply_count;
             reply_count++) {
          bdestroy_wrapper(&message_p->ittiMsg.s1ap_configuration_transfer
                                .conf_reply->addresses[reply_count]);
        }
        free_wrapper(
            (void**)&message_p->ittiMsg.s1ap_configuration_transfer.conf_reply);
      }
      break;

    case SCTP_DATA_REQ:
      bdestroy_wrapper(&message_p->ittiMsg.sctp_data_req.payload);
      AssertFatal(NULL == message_p->ittiMsg.sctp_data_req.payload,
                  "TODO clean pointer");
      break;

    case SCTP_DATA_IND:
      bdestroy_wrapper(&message_p->ittiMsg.sctp_data_ind.payload);
      AssertFatal(NULL == message_p->ittiMsg.sctp_data_ind.payload,
                  "TODO clean pointer");
      break;

    case SCTP_DATA_CNF:
    case SCTP_NEW_ASSOCIATION:
    case SCTP_CLOSE_ASSOCIATION:
      // DO nothing
      break;

    case UDP_INIT:
    case UDP_DATA_REQ:
    case UDP_DATA_IND:
      /** Changed to stacked buffer. */
      break;

    case S1AP_PATH_SWITCH_REQUEST_ACKNOWLEDGE:
      /** Bearer Contexts to be switched. */
      if (message_p->ittiMsg.s1ap_path_switch_request_ack
              .bearer_ctx_to_be_switched_list) {
        free_bearer_contexts_to_be_created(
            &message_p->ittiMsg.s1ap_path_switch_request_ack
                 .bearer_ctx_to_be_switched_list);
      }
      break;

    case S1AP_HANDOVER_REQUIRED:
      bdestroy_wrapper(&message_p->ittiMsg.s1ap_handover_required
                            .eutran_source_to_target_container);
      break;

    case S1AP_HANDOVER_REQUEST:
      /** E-UTRAN Container. */
      bdestroy_wrapper(&message_p->ittiMsg.s1ap_handover_request
                            .source_to_target_eutran_container);
      AssertFatal(NULL == message_p->ittiMsg.s1ap_handover_request
                              .source_to_target_eutran_container,
                  "TODO clean pointer");

      /** Bearer Context to Be Setup. */
      if (message_p->ittiMsg.s1ap_handover_request
              .bearer_ctx_to_be_setup_list) {
        free_bearer_contexts_to_be_created(
            &message_p->ittiMsg.s1ap_handover_request
                 .bearer_ctx_to_be_setup_list);
      }
      break;

      //  case S1AP_HANDOVER_REQUEST_ACKNOWLEDGE:
      //    bdestroy_wrapper(&message_p->ittiMsg.s1ap_handover_request_acknowledge.target_to_source_eutran_container);
      //    for (int i = 0; i <
      //    message_p->ittiMsg.s1ap_handover_request_acknowledge.no_of_e_rabs;
      //    i++) {
      //      bdestroy_wrapper
      //      (&message_p->ittiMsg.s1ap_handover_request_acknowledge.transport_layer_address[i]);
      ////          bdestroy_wrapper
      ///(&message_p->ittiMsg.s1ap_e_rab_setup_req.e_rab_to_be_setup_list.item[i].transport_layer_address);
      //    }
      //    break;

    case S1AP_HANDOVER_COMMAND:
      bdestroy_wrapper(&message_p->ittiMsg.s1ap_handover_command
                            .eutran_target_to_source_container);
      /** Bearer Context to Be Setup. */
      if (message_p->ittiMsg.s1ap_handover_command
              .bearer_ctx_to_be_forwarded_list) {
        free_bearer_contexts_to_be_created(
            &message_p->ittiMsg.s1ap_handover_command
                 .bearer_ctx_to_be_forwarded_list);
      }

      break;

    case S1AP_ENB_STATUS_TRANSFER:
      if (message_p->ittiMsg.s1ap_mme_status_transfer
              .status_transfer_bearer_list) {
        free_wrapper(&message_p->ittiMsg.s1ap_enb_status_transfer
                          .status_transfer_bearer_list);
      }
      break;
    case S1AP_MME_STATUS_TRANSFER:
      if (message_p->ittiMsg.s1ap_mme_status_transfer
              .status_transfer_bearer_list) {
        free_wrapper(&message_p->ittiMsg.s1ap_mme_status_transfer
                          .status_transfer_bearer_list);
      }
      break;

      /**
       * S10 Messages.
       */
    case S10_FORWARD_RELOCATION_REQUEST:
      /** Transparent Container. */
      bdestroy_wrapper(&message_p->ittiMsg.s10_forward_relocation_request
                            .f_container.container_value);
      AssertFatal(NULL == message_p->ittiMsg.s10_forward_relocation_request
                              .f_container.container_value,
                  "TODO clean pointer");
      /** PDN Connections. */
      if (message_p->ittiMsg.s10_forward_relocation_request.pdn_connections) {
        free_mme_ue_eps_pdn_connections(
            &message_p->ittiMsg.s10_forward_relocation_request.pdn_connections);
      }
      /** MM Context. */
      if (message_p->ittiMsg.s10_forward_relocation_request.ue_eps_mm_context) {
        free_mm_context_eps(&message_p->ittiMsg.s10_forward_relocation_request
                                 .ue_eps_mm_context);
      }
      /** Source SGW FQDN. */
      bdestroy_wrapper(
          &message_p->ittiMsg.s10_forward_relocation_request.source_sgw_fqdn);
      break;

    case S10_FORWARD_RELOCATION_RESPONSE:
      /** Transparent Container. */
      bdestroy_wrapper(&message_p->ittiMsg.s10_forward_relocation_response
                            .eutran_container.container_value);
      break;

    case S10_FORWARD_ACCESS_CONTEXT_NOTIFICATION:
      /** EUTRAN Container. */
      if (message_p->ittiMsg.s10_forward_access_context_notification
              .status_transfer_bearer_list) {
        free_wrapper(&message_p->ittiMsg.s10_forward_access_context_notification
                          .status_transfer_bearer_list);
      }
      break;
    case S10_CONTEXT_REQUEST:
      bdestroy_wrapper(&message_p->ittiMsg.s10_context_request
                            .complete_request_message.request_value);
      break;
    case S10_CONTEXT_RESPONSE:
      /** PDN Connections. */
      if (message_p->ittiMsg.s10_context_response.pdn_connections) {
        free_mme_ue_eps_pdn_connections(
            &message_p->ittiMsg.s10_context_response.pdn_connections);
      }
      /** MM Context. */
      if (message_p->ittiMsg.s10_context_response.ue_eps_mm_context) {
        free_mm_context_eps(
            &message_p->ittiMsg.s10_context_response.ue_eps_mm_context);
      }
      /** Source SGW FQDN. */
      bdestroy_wrapper(
          &message_p->ittiMsg.s10_context_response.source_sgw_fqdn);
      break;
  }
}
