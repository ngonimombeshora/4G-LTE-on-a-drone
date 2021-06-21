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

/*! \file mme_app_transport.c
  \brief
  \author Lionel Gauthier
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
#include "conversions.h"
#include "intertask_interface.h"
#include "log.h"
#include "mme_config.h"
#include "msc.h"

#include "mme_app_defs.h"
#include "mme_app_extern.h"
#include "mme_app_ue_context.h"

#include "common_defs.h"
#include "secu_defs.h"

//------------------------------------------------------------------------------
int mme_app_handle_nas_dl_req(itti_nas_dl_data_req_t *const nas_dl_req_pP)
//------------------------------------------------------------------------------
{
  OAILOG_FUNC_IN(LOG_MME_APP);
  MessageDef *message_p = NULL;
  int rc = RETURNok;

  message_p = itti_alloc_new_message(TASK_MME_APP, NAS_DOWNLINK_DATA_REQ);

  ue_context_t *ue_context = mme_ue_context_exists_mme_ue_s1ap_id(
      &mme_app_desc.mme_ue_contexts, nas_dl_req_pP->ue_id);
  if (ue_context) {
    OAILOG_DEBUG(
        LOG_MME_APP,
        "DOWNLINK NAS TRANSPORT Found enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT
        " mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT "\n",
        ue_context->privates.fields.enb_ue_s1ap_id, nas_dl_req_pP->ue_id);

    // todo: we don't change the ECM signaling state here!

    NAS_DOWNLINK_DATA_REQ(message_p).enb_ue_s1ap_id =
        ue_context->privates.fields.enb_ue_s1ap_id;
    NAS_DOWNLINK_DATA_REQ(message_p).ue_id = nas_dl_req_pP->ue_id;
    //    NAS_DOWNLINK_DATA_REQ (message_p).enb_id                 =
    //    nas_dl_req_pP->enb_id;
    NAS_DOWNLINK_DATA_REQ(message_p).enb_id =
        ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id;
    NAS_DOWNLINK_DATA_REQ(message_p).nas_msg = nas_dl_req_pP->nas_msg;
    nas_dl_req_pP->nas_msg = NULL;

    MSC_LOG_TX_MESSAGE(
        MSC_MMEAPP_MME, MSC_S1AP_MME, NULL, 0,
        "0 DOWNLINK NAS TRANSPORT enb_ue_s1ap_id " ENB_UE_S1AP_ID_FMT
        " ue id " MME_UE_S1AP_ID_FMT " ",
        ue_context->privates.fields.enb_ue_s1ap_id, nas_dl_req_pP->ue_id);

    int to_task = (RUN_MODE_SCENARIO_PLAYER == mme_config.run_mode)
                      ? TASK_MME_SCENARIO_PLAYER
                      : TASK_S1AP;
    rc = itti_send_msg_to_task(to_task, INSTANCE_DEFAULT, message_p);

    /* We don't set the ECM state to connected, this is not the place, it should
     * be connected when initial UE context release request is received. */
    OAILOG_TRACE(
        LOG_MME_APP,
        " MME_APP:DOWNLINK NAS TRANSPORT. MME_UE_S1AP_ID " MME_UE_S1AP_ID_FMT
        " and ENB_UE_S1AP_ID " ENB_UE_S1AP_ID_FMT ". \n",
        nas_dl_req_pP->ue_id, ue_context->privates.fields.enb_ue_s1ap_id);

    // Check the transaction status. And trigger the UE context release command
    // accrordingly.
    if (nas_dl_req_pP->transaction_status != AS_SUCCESS &&
        nas_dl_req_pP->transaction_status != AS_TERMINATED_NAS_LIGHT) {
      ue_context->privates.s1_ue_context_release_cause =
          S1AP_NAS_NORMAL_RELEASE;

      if (ue_context->privates.fields.mm_state == UE_REGISTERED) {
        ue_session_pool_t *ue_session_pool =
            mme_ue_session_pool_exists_mme_ue_s1ap_id(
                &mme_app_desc.mme_ue_session_pools, nas_dl_req_pP->ue_id);
        if (ue_session_pool) {
          /** The ECM Connection will always be active, check the bearers. */
          pdn_context_t *pdn_context =
              RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);
          if (pdn_context) {
            bearer_context_new_t *first_bearer =
                STAILQ_FIRST(&pdn_context->session_bearers);
            if (first_bearer) {
              if (first_bearer->bearer_state & BEARER_STATE_ACTIVE) {
                OAILOG_INFO(
                    LOG_MME_APP,
                    " MME_APP: Bearer of UE MME_UE_S1AP_ID " MME_UE_S1AP_ID_FMT
                    " are active. Triggering bearer release first. \n",
                    nas_dl_req_pP->ue_id);
                mme_app_send_s11_release_access_bearers_req(
                    nas_dl_req_pP->ue_id); /**< Release Access bearers and then
                                              send context release request.  */
              } else {
                OAILOG_INFO(
                    LOG_MME_APP,
                    " MME_APP: Bearer of UE MME_UE_S1AP_ID " MME_UE_S1AP_ID_FMT
                    " are NOT active. Continuing directly with S1AP release. "
                    "\n",
                    nas_dl_req_pP->ue_id);
                // Notify S1AP to send UE Context Release Command to eNB.
                mme_app_itti_ue_context_release(
                    ue_context->privates.mme_ue_s1ap_id,
                    ue_context->privates.fields.enb_ue_s1ap_id,
                    ue_context->privates.s1_ue_context_release_cause,
                    ue_context->privates.fields.e_utran_cgi.cell_identity
                        .enb_id);
              }
            } else {
              OAILOG_ERROR(LOG_MME_APP,
                           " MME_APP: No valid first bearer for UE "
                           "MME_UE_S1AP_ID " MME_UE_S1AP_ID_FMT
                           " was found. Neglecting resource freeing.\n",
                           nas_dl_req_pP->ue_id);
            }
          } else {
            OAILOG_ERROR(LOG_MME_APP,
                         " MME_APP: No valid first PDN context for UE "
                         "MME_UE_S1AP_ID " MME_UE_S1AP_ID_FMT
                         " was found. Neglecting resource freeing.\n",
                         nas_dl_req_pP->ue_id);
          }
        } else {
          OAILOG_WARNING(LOG_MME_APP,
                         " MME_APP:DOWNLINK NAS TRANSPORT (termination). No UE "
                         "session pool was found for REGISTERED UE: "
                         "MME_UE_S1AP_ID " MME_UE_S1AP_ID_FMT
                         " and ENB_UE_S1AP_ID " ENB_UE_S1AP_ID_FMT ". \n",
                         nas_dl_req_pP->ue_id,
                         ue_context->privates.fields.enb_ue_s1ap_id);
        }
      } else {
        OAILOG_ERROR(LOG_MME_APP,
                     " MME_APP: UE MME_UE_S1AP_ID " MME_UE_S1AP_ID_FMT
                     " is in UE_UNREGISTERED state. Triggering detach (S11 "
                     "should already be removeD).\n",
                     nas_dl_req_pP->ue_id);
        mme_app_itti_ue_context_release(
            ue_context->privates.mme_ue_s1ap_id,
            ue_context->privates.fields.enb_ue_s1ap_id,
            ue_context->privates.s1_ue_context_release_cause,
            ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id);
      }
    } else {
      //      OAILOG_ERROR(LOG_MME_APP, "DOWNLINK NAS TRANSPORT failed
      //      mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT " not found. Not terminating
      //      bearers due error cause %d.\n", nas_dl_req_pP->ue_id,
      //      nas_dl_req_pP->transaction_status);
    }
  } else {
    OAILOG_ERROR(
        LOG_MME_APP,
        "DOWNLINK NAS TRANSPORT failed mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT
        " not found\n",
        nas_dl_req_pP->ue_id);
  }

  OAILOG_FUNC_RETURN(LOG_MME_APP, rc);
}
