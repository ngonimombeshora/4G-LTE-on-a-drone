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

/*! \file mme_app_procedures.c
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
#include "common_defs.h"
#include "common_types.h"
#include "conversions.h"
#include "dynamic_memory_check.h"
#include "intertask_interface.h"
#include "log.h"
#include "mme_app_bearer_context.h"
#include "mme_app_defs.h"
#include "mme_app_extern.h"
#include "mme_app_itti_messaging.h"
#include "mme_app_procedures.h"
#include "mme_app_ue_context.h"
#include "mme_config.h"
#include "msc.h"
#include "timer.h"

static void mme_app_free_s11_procedure_create_bearer(
    mme_app_s11_proc_t **s11_proc);
static void mme_app_free_s11_procedure_update_bearer(
    mme_app_s11_proc_t **s11_proc);
static void mme_app_free_s11_procedure_delete_bearer(
    mme_app_s11_proc_t **s11_proc);
static void mme_app_free_s10_procedure_mme_handover(
    mme_app_s10_proc_t **s10_proc);

//------------------------------------------------------------------------------
void mme_app_delete_s11_procedures(
    struct ue_session_pool_s *const ue_session_pool) {
  mme_app_s11_proc_t *s11_proc1 = NULL;
  mme_app_s11_proc_t *s11_proc2 = NULL;

  s11_proc1 =
      LIST_FIRST(&ue_session_pool->s11_procedures); /* Faster List Deletion. */
  while (s11_proc1) {
    s11_proc2 = LIST_NEXT(s11_proc1, entries);
    if (MME_APP_S11_PROC_TYPE_CREATE_BEARER == s11_proc1->type) {
      mme_app_free_s11_procedure_create_bearer(&s11_proc1);
    } else if (MME_APP_S11_PROC_TYPE_UPDATE_BEARER == s11_proc1->type) {
      mme_app_free_s11_procedure_update_bearer(&s11_proc1);
    } else if (MME_APP_S11_PROC_TYPE_DELETE_BEARER == s11_proc1->type) {
      mme_app_free_s11_procedure_delete_bearer(&s11_proc1);
    }  // else ...
    s11_proc1 = s11_proc2;
  }
  LIST_INIT(&ue_session_pool->s11_procedures);
}

//------------------------------------------------------------------------------
mme_app_s11_proc_create_bearer_t *mme_app_create_s11_procedure_create_bearer(
    struct ue_session_pool_s *const ue_session_pool) {
  /** Check if the list of S11 procedures is empty. */
  if (!LIST_EMPTY(&ue_session_pool->s11_procedures)) {
    OAILOG_ERROR(
        LOG_MME_APP,
        "UE with ueId " MME_UE_S1AP_ID_FMT
        " has already a S11 procedure ongoing. Cannot create CBR procedure. \n",
        ue_session_pool->privates.mme_ue_s1ap_id);
    return NULL;
  }

  mme_app_s11_proc_create_bearer_t *s11_proc_create_bearer =
      calloc(1, sizeof(mme_app_s11_proc_create_bearer_t));
  s11_proc_create_bearer->proc.proc.type = MME_APP_BASE_PROC_TYPE_S11;
  s11_proc_create_bearer->proc.type = MME_APP_S11_PROC_TYPE_CREATE_BEARER;
  mme_app_s11_proc_t *s11_proc = (mme_app_s11_proc_t *)s11_proc_create_bearer;

  /** Initialize the of the procedure. */

  LIST_INIT(&ue_session_pool->s11_procedures);
  LIST_INSERT_HEAD((&ue_session_pool->s11_procedures), s11_proc, entries);

  return s11_proc_create_bearer;
}

//------------------------------------------------------------------------------
mme_app_s11_proc_t *mme_app_get_s11_procedure(
    struct ue_session_pool_s *const ue_session_pool) {
  return LIST_FIRST(&ue_session_pool->s11_procedures);
}

//------------------------------------------------------------------------------
mme_app_s11_proc_create_bearer_t *mme_app_get_s11_procedure_create_bearer(
    struct ue_session_pool_s *const ue_session_pool) {
  mme_app_s11_proc_t *s11_proc = NULL;

  LIST_FOREACH(s11_proc, &ue_session_pool->s11_procedures, entries) {
    if (MME_APP_S11_PROC_TYPE_CREATE_BEARER == s11_proc->type) {
      return (mme_app_s11_proc_create_bearer_t *)s11_proc;
    }
  }
  return NULL;
}

//------------------------------------------------------------------------------
void mme_app_delete_s11_procedure_create_bearer(
    struct ue_session_pool_s *const ue_session_pool) {
  mme_app_s11_proc_t *s11_proc = NULL, *s11_proc_safe = NULL;

  LIST_FOREACH_SAFE(s11_proc, &ue_session_pool->s11_procedures, entries,
                    s11_proc_safe) {
    if (MME_APP_S11_PROC_TYPE_CREATE_BEARER == s11_proc->type) {
      LIST_REMOVE(s11_proc, entries);
      mme_app_free_s11_procedure_create_bearer(&s11_proc);
      return;
    }
  }

  if (LIST_EMPTY(&ue_session_pool->s11_procedures)) {
    LIST_INIT(&ue_session_pool->s11_procedures);
    OAILOG_INFO(LOG_MME_APP,
                "UE with ueId " MME_UE_S1AP_ID_FMT
                " has no more S11 procedures left. Cleared the list. \n",
                ue_session_pool->privates.mme_ue_s1ap_id);
  }
}

//------------------------------------------------------------------------------
mme_app_s11_proc_update_bearer_t *mme_app_create_s11_procedure_update_bearer(
    struct ue_session_pool_s *const ue_session_pool) {
  /** Check if the list of S11 procedures is empty. */
  if (!LIST_EMPTY(&ue_session_pool->s11_procedures)) {
    OAILOG_ERROR(
        LOG_MME_APP,
        "UE with ueId " MME_UE_S1AP_ID_FMT
        " has already a S11 procedure ongoing. Cannot create UBR procedure. \n",
        ue_session_pool->privates.mme_ue_s1ap_id);
    return NULL;
  }
  mme_app_s11_proc_update_bearer_t *s11_proc_update_bearer =
      calloc(1, sizeof(mme_app_s11_proc_update_bearer_t));
  s11_proc_update_bearer->proc.proc.type = MME_APP_BASE_PROC_TYPE_S11;
  s11_proc_update_bearer->proc.type = MME_APP_S11_PROC_TYPE_UPDATE_BEARER;
  mme_app_s11_proc_t *s11_proc = (mme_app_s11_proc_t *)s11_proc_update_bearer;

  /** Initialize the of the procedure. */

  LIST_INIT(&ue_session_pool->s11_procedures);
  LIST_INSERT_HEAD((&ue_session_pool->s11_procedures), s11_proc, entries);

  return s11_proc_update_bearer;
}

//------------------------------------------------------------------------------
mme_app_s11_proc_update_bearer_t *mme_app_get_s11_procedure_update_bearer(
    struct ue_session_pool_s *const ue_session_pool) {
  mme_app_s11_proc_t *s11_proc = NULL;

  LIST_FOREACH(s11_proc, &ue_session_pool->s11_procedures, entries) {
    if (MME_APP_S11_PROC_TYPE_UPDATE_BEARER == s11_proc->type) {
      return (mme_app_s11_proc_update_bearer_t *)s11_proc;
    }
  }
  return NULL;
}

//------------------------------------------------------------------------------
void mme_app_delete_s11_procedure_update_bearer(
    struct ue_session_pool_s *const ue_session_pool) {
  /** Check if the list of S11 procedures is empty. */
  mme_app_s11_proc_t *s11_proc = NULL, *s11_proc_safe = NULL;

  LIST_FOREACH_SAFE(s11_proc, &ue_session_pool->s11_procedures, entries,
                    s11_proc_safe) {
    if (MME_APP_S11_PROC_TYPE_UPDATE_BEARER == s11_proc->type) {
      LIST_REMOVE(s11_proc, entries);
      mme_app_free_s11_procedure_update_bearer(&s11_proc);
      return;
    }
  }

  if (LIST_EMPTY(&ue_session_pool->s11_procedures)) {
    LIST_INIT(&ue_session_pool->s11_procedures);
    OAILOG_INFO(LOG_MME_APP,
                "UE with ueId " MME_UE_S1AP_ID_FMT
                " has no more S11 procedures left. Cleared the list. \n",
                ue_session_pool->privates.mme_ue_s1ap_id);
  }
}

//------------------------------------------------------------------------------
mme_app_s11_proc_delete_bearer_t *mme_app_create_s11_procedure_delete_bearer(
    struct ue_session_pool_s *const ue_session_pool) {
  if (!LIST_EMPTY(&ue_session_pool->s11_procedures)) {
    OAILOG_ERROR(
        LOG_MME_APP,
        "UE with ueId " MME_UE_S1AP_ID_FMT
        " has already a S11 procedure ongoing. Cannot create DBR procedure. \n",
        ue_session_pool->privates.mme_ue_s1ap_id);
    return NULL;
  }

  mme_app_s11_proc_delete_bearer_t *s11_proc_delete_bearer =
      calloc(1, sizeof(mme_app_s11_proc_delete_bearer_t));
  s11_proc_delete_bearer->proc.proc.type = MME_APP_BASE_PROC_TYPE_S11;
  s11_proc_delete_bearer->proc.type = MME_APP_S11_PROC_TYPE_DELETE_BEARER;
  mme_app_s11_proc_t *s11_proc = (mme_app_s11_proc_t *)s11_proc_delete_bearer;

  /** Initialize the of the procedure. */

  LIST_INIT(&ue_session_pool->s11_procedures);
  LIST_INSERT_HEAD((&ue_session_pool->s11_procedures), s11_proc, entries);

  return s11_proc_delete_bearer;
}

//------------------------------------------------------------------------------
mme_app_s11_proc_delete_bearer_t *mme_app_get_s11_procedure_delete_bearer(
    struct ue_session_pool_s *const ue_session_pool) {
  mme_app_s11_proc_t *s11_proc = NULL;

  LIST_FOREACH(s11_proc, &ue_session_pool->s11_procedures, entries) {
    if (MME_APP_S11_PROC_TYPE_DELETE_BEARER == s11_proc->type) {
      return (mme_app_s11_proc_delete_bearer_t *)s11_proc;
    }
  }
  return NULL;
}

//------------------------------------------------------------------------------
void mme_app_delete_s11_procedure_delete_bearer(
    struct ue_session_pool_s *const ue_session_pool) {
  mme_app_s11_proc_t *s11_proc = NULL, *s11_proc_safe = NULL;

  LIST_FOREACH_SAFE(s11_proc, &ue_session_pool->s11_procedures, entries,
                    s11_proc_safe) {
    if (MME_APP_S11_PROC_TYPE_DELETE_BEARER == s11_proc->type) {
      LIST_REMOVE(s11_proc, entries);
      mme_app_free_s11_procedure_delete_bearer(&s11_proc);
      return;
    }
  }

  if (LIST_EMPTY(&ue_session_pool->s11_procedures)) {
    LIST_INIT(&ue_session_pool->s11_procedures);

    OAILOG_INFO(LOG_MME_APP,
                "UE with ueId " MME_UE_S1AP_ID_FMT
                " has no more S11 procedures left. Cleared the list. \n",
                ue_session_pool->privates.mme_ue_s1ap_id);
  }
}

//------------------------------------------------------------------------------
static void mme_app_free_s11_procedure_create_bearer(
    mme_app_s11_proc_t **s11_proc_cbr) {
  // DO here specific releases (memory,etc)
  /** Remove the bearer contexts to be setup. */
  mme_app_s11_proc_create_bearer_t **s11_proc_create_bearer_pp =
      (mme_app_s11_proc_create_bearer_t **)s11_proc_cbr;
  free_bearer_contexts_to_be_created(&(*s11_proc_create_bearer_pp)->bcs_tbc);
  free_wrapper((void **)s11_proc_create_bearer_pp);
}

//------------------------------------------------------------------------------
static void mme_app_free_s11_procedure_update_bearer(
    mme_app_s11_proc_t **s11_proc_ubr) {
  // DO here specific releases (memory,etc)
  /** Remove the bearer contexts to be setup. */
  mme_app_s11_proc_update_bearer_t **s11_proc_update_bearer_pp =
      (mme_app_s11_proc_update_bearer_t **)s11_proc_ubr;
  if ((*s11_proc_update_bearer_pp)->bcs_tbu)
    free_bearer_contexts_to_be_updated(&(*s11_proc_update_bearer_pp)->bcs_tbu);
  free_wrapper((void **)s11_proc_update_bearer_pp);
}

//------------------------------------------------------------------------------
static void mme_app_free_s11_procedure_delete_bearer(
    mme_app_s11_proc_t **s11_proc_dbr) {
  // DO here specific releases (memory,etc)
  /** Remove the bearer contexts to be setup. */
  //  mme_app_s11_proc_delete_bearer_t ** s11_proc_delete_bearer_pp =
  //  (mme_app_s11_proc_delete_bearer_t**)s11_proc_dbr;
  free_wrapper((void **)s11_proc_dbr);
}

/**
 * S10 Procedures.
 */
static int remove_s10_tunnel_endpoint(struct ue_context_s *ue_context,
                                      struct sockaddr *peer_ip) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  int rc = RETURNerror;
  //  /** Removed S10 tunnel endpoint. */
  if (ue_context->privates.fields.local_mme_teid_s10 == (teid_t)0) {
    OAILOG_ERROR(LOG_MME_APP,
                 "UE with ueId " MME_UE_S1AP_ID_FMT
                 " has no local S10 teid. Not triggering tunnel removal. \n",
                 ue_context->privates.mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, rc);
  }
  rc = mme_app_remove_s10_tunnel_endpoint(
      ue_context->privates.fields.local_mme_teid_s10, peer_ip);
  /** Deregister the key. */
  mme_ue_context_update_coll_keys(
      &mme_app_desc.mme_ue_contexts, ue_context,
      ue_context->privates.enb_s1ap_id_key, ue_context->privates.mme_ue_s1ap_id,
      ue_context->privates.fields.imsi,
      ue_context->privates.fields.mme_teid_s11, INVALID_TEID,
      &ue_context->privates.fields.guti);
  OAILOG_FUNC_RETURN(LOG_MME_APP, rc);
}

//------------------------------------------------------------------------------
void mme_app_delete_s10_procedures(struct ue_context_s *const ue_context) {
  if (ue_context->s10_procedures) {
    mme_app_s10_proc_t *s10_proc1 = NULL;
    mme_app_s10_proc_t *s10_proc2 = NULL;

    // todo: intra
    s10_proc1 =
        LIST_FIRST(ue_context->s10_procedures); /* Faster List Deletion. */
    while (s10_proc1) {
      s10_proc2 = LIST_NEXT(s10_proc1, entries);
      if (MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER == s10_proc1->type) {
        /** Stop the timer. */
        if (s10_proc1->timer.id != MME_APP_TIMER_INACTIVE_ID) {
          if (timer_remove(s10_proc1->timer.id, NULL)) {
            OAILOG_ERROR(LOG_MME_APP,
                         "Failed to stop the procedure timer for inter-MMME "
                         "handover for UE id  %d \n",
                         ue_context->privates.mme_ue_s1ap_id);
            s10_proc1->timer.id = MME_APP_TIMER_INACTIVE_ID;
          }
        }
        s10_proc1->timer.id = MME_APP_TIMER_INACTIVE_ID;
        //        if(s10_proc1->target_mme)
        //          remove_s10_tunnel_endpoint(ue_context, s10_proc1);
      }  // else ...
      else if (MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER == s10_proc1->type) {
        /** Stop the timer. */
        if (s10_proc1->timer.id != MME_APP_TIMER_INACTIVE_ID) {
          if (timer_remove(s10_proc1->timer.id, NULL)) {
            OAILOG_ERROR(LOG_MME_APP,
                         "Failed to stop the procedure timer for inter-MMME "
                         "handover for UE id  %d \n",
                         ue_context->privates.mme_ue_s1ap_id);
            s10_proc1->timer.id = MME_APP_TIMER_INACTIVE_ID;
          }
        }
        s10_proc1->timer.id = MME_APP_TIMER_INACTIVE_ID;
      }  // else ...
      s10_proc1 = s10_proc2;
    }
    LIST_INIT(ue_context->s10_procedures);
    free_wrapper((void **)&ue_context->s10_procedures);
  }
}

//------------------------------------------------------------------------------
static void mme_app_handle_mme_s10_handover_completion_timer_expiry(
    mme_app_s10_proc_mme_handover_t *s10_proc_mme_handover) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  MessageDef *message_p = NULL;
  /** Get the IMSI. */
  //  imsi64_t imsi64 = imsi_to_imsi64(&s10_proc_mme_handover->imsi);
  struct ue_context_s *ue_context = mme_ue_context_exists_mme_ue_s1ap_id(
      &mme_app_desc.mme_ue_contexts, s10_proc_mme_handover->mme_ue_s1ap_id);
  DevAssert(ue_context);
  OAILOG_WARNING(
      LOG_MME_APP,
      "Expired- MME S10 Handover Completion timer for UE " MME_UE_S1AP_ID_FMT
      " run out. "
      "Performing S1AP UE Context Release Command and successive NAS implicit "
      "detach. \n",
      ue_context->privates.mme_ue_s1ap_id);
  s10_proc_mme_handover->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;

  /** Check if an NAS context exists (this might happen if a TAU_COMPLETE has
   * not arrived or arrived and was discarded due security reasons. */
  emm_data_context_t *emm_context =
      emm_data_context_get(&_emm_data, ue_context->privates.mme_ue_s1ap_id);
  if (emm_context) {
    OAILOG_WARNING(
        LOG_MME_APP,
        " **** ABNORMAL **** An EMM context for UE " MME_UE_S1AP_ID_FMT
        " exists. Performing implicit detach due time out of handover "
        "completion timer! \n",
        ue_context->privates.mme_ue_s1ap_id);
    /** Check if the CLR flag has been set. */
    message_p =
        itti_alloc_new_message(TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert(message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id =
        ue_context->privates
            .mme_ue_s1ap_id; /**< We don't send a Detach Type such that no
                                Detach Request is sent to the UE if handover is
                                performed. */

    MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0,
                       "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task(TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  } else {
    ue_context->privates.s1_ue_context_release_cause = S1AP_HANDOVER_CANCELLED;
    /*
     * Send a UE Context Release Command which would trigger a context release.
     * The e_cgi IE will be set with Handover Notify.
     */
    /** Send a FW-Relocation Response error if no local teid is set (no
     * FW-Relocation Response is send yet). */
    if (!ue_context->privates.fields.local_mme_teid_s10) {
      mme_app_send_s10_forward_relocation_response_err(
          s10_proc_mme_handover->remote_mme_teid.teid,
          s10_proc_mme_handover->proc.peer_ip,
          (void *)s10_proc_mme_handover->forward_relocation_trxn,
          REQUEST_REJECTED);
    }
    /** Delete the procedure. */
    mme_app_delete_s10_procedure_mme_handover(ue_context);

    /** Trigger an ESM detach, also removing all PDN contexts in the MME and the
     * SAE-GW. */
    message_p = itti_alloc_new_message(TASK_MME_APP, NAS_ESM_DETACH_IND);
    DevAssert(message_p != NULL);
    message_p->ittiMsg.nas_esm_detach_ind.ue_id =
        ue_context->privates
            .mme_ue_s1ap_id; /**< We don't send a Detach Type such that no
                                Detach Request is sent to the UE if handover is
                                performed. */

    MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0,
                       "0 NAS_ESM_DETACH_IND");
    itti_send_msg_to_task(TASK_NAS_ESM, INSTANCE_DEFAULT, message_p);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
}

//------------------------------------------------------------------------------
static void mme_app_handle_mobility_completion_timer_expiry(
    mme_app_s10_proc_mme_handover_t *s10_proc_mme_handover) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  struct ue_context_s *ue_context = mme_ue_context_exists_mme_ue_s1ap_id(
      &mme_app_desc.mme_ue_contexts, s10_proc_mme_handover->mme_ue_s1ap_id);
  DevAssert(ue_context != NULL);
  MessageDef *message_p = NULL;
  OAILOG_INFO(
      LOG_MME_APP,
      "Expired- MME Handover Completion timer for UE " MME_UE_S1AP_ID_FMT
      " run out. \n",
      ue_context->privates.mme_ue_s1ap_id);
  /*
   * This timer is only expired in the inter-MME handover case for the source
   * MME. The timer will be stopped when successfully the S10 Forward Relocation
   * Completion message arrives.
   */
  if (s10_proc_mme_handover->pending_clear_location_request) {
    OAILOG_INFO(LOG_MME_APP,
                "CLR flag is set for UE " MME_UE_S1AP_ID_FMT
                ". Performing implicit detach. \n",
                ue_context->privates.mme_ue_s1ap_id);
    s10_proc_mme_handover->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;
    ue_context->privates.s1_ue_context_release_cause = S1AP_NAS_DETACH;
    /** Check if the CLR flag has been set. */
    message_p =
        itti_alloc_new_message(TASK_MME_APP, NAS_IMPLICIT_DETACH_UE_IND);
    DevAssert(message_p != NULL);
    message_p->ittiMsg.nas_implicit_detach_ue_ind.ue_id =
        ue_context->privates
            .mme_ue_s1ap_id; /**< We don't send a Detach Type such that no
                                Detach Request is sent to the UE if handover is
                                performed. */
    message_p->ittiMsg.nas_implicit_detach_ue_ind.clr =
        s10_proc_mme_handover
            ->pending_clear_location_request; /**< Inform about the CLR. */

    MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_NAS_MME, NULL, 0,
                       "0 NAS_IMPLICIT_DETACH_UE_IND_MESSAGE");
    itti_send_msg_to_task(TASK_NAS_EMM, INSTANCE_DEFAULT, message_p);
  } else {
    OAILOG_WARNING(LOG_MME_APP,
                   "CLR flag is NOT set for UE " MME_UE_S1AP_ID_FMT
                   ". Not performing implicit detach. \n",
                   ue_context->privates.mme_ue_s1ap_id);
    /*
     * Handover failed on the target MME side.
     * Aborting the handover procedure and leaving the UE intact.
     * Going into Idle mode.
     */
    /** Remove the context in the target eNB, if it is INTRA-Handover. */
    if (s10_proc_mme_handover->proc.type ==
        MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER) {
      OAILOG_WARNING(LOG_MME_APP,
                     "Releasing the request first for the target-ENB for INTRA "
                     "handover for UE " MME_UE_S1AP_ID_FMT
                     ". Not performing implicit detach. \n",
                     ue_context->privates.mme_ue_s1ap_id);
      mme_app_itti_ue_context_release(
          ue_context->privates.mme_ue_s1ap_id,
          s10_proc_mme_handover->target_enb_ue_s1ap_id, S1AP_HANDOVER_FAILED,
          s10_proc_mme_handover->target_ecgi.cell_identity.enb_id);
      ue_context->privates.s1_ue_context_release_cause = S1AP_HANDOVER_FAILED;
    } else {
      OAILOG_WARNING(LOG_MME_APP,
                     "Releasing the request for the source-ENB for INTER "
                     "handover for UE " MME_UE_S1AP_ID_FMT
                     ". Not performing implicit detach. \n",
                     ue_context->privates.mme_ue_s1ap_id);
      /** Check if the UE is in connected mode. */
      if (ue_context->privates.fields.ecm_state == ECM_CONNECTED) {
        OAILOG_DEBUG(LOG_MME_APP,
                     "UE " MME_UE_S1AP_ID_FMT
                     " is in ECM connected state (assuming handover). \n",
                     ue_context->privates.mme_ue_s1ap_id);
        /*
         * Intra and we are source, so remove the handover procedure locally and
         * send Release Command to source eNB. We might have send HO-Command,
         * but FW-Relocation-Complete message has not arrived. So we must set
         * the source ENB into idle mode manually.
         */
        if (s10_proc_mme_handover
                ->ho_command_sent) { /**< This is the error timeout. The real
                                        mobility completion timeout comes from
                                        S1AP. */
          OAILOG_WARNING(LOG_MME_APP,
                         "HO command already set for UE. Setting S1AP "
                         "reference to idle mode for UE " MME_UE_S1AP_ID_FMT
                         ". Not performing implicit detach. \n",
                         ue_context->privates.mme_ue_s1ap_id);
          mme_app_itti_ue_context_release(
              ue_context->privates.mme_ue_s1ap_id,
              ue_context->privates.fields.enb_ue_s1ap_id, S1AP_HANDOVER_FAILED,
              ue_context->privates.fields.e_utran_cgi.cell_identity.enb_id);
          /** Remove the handover procedure. */
          mme_app_delete_s10_procedure_mme_handover(ue_context);
          OAILOG_FUNC_OUT(LOG_MME_APP);

        } else {
          if (s10_proc_mme_handover->due_tau) {
            /** Send a Notify Request to trigger idle TAU. */
            OAILOG_WARNING(LOG_MME_APP,
                           "Source MME CLR not received (Idle Tau - source "
                           "side) for UE " MME_UE_S1AP_ID_FMT
                           ". Triggering a notify to HSS. \n",
                           ue_context->privates.mme_ue_s1ap_id);
            mme_app_itti_notify_request(ue_context->privates.fields.imsi,
                                        &s10_proc_mme_handover->target_tai.plmn,
                                        true);
            mme_app_delete_s10_procedure_mme_handover(ue_context);
            OAILOG_FUNC_OUT(LOG_MME_APP);
          } else {
            /** This should not happen. The Ho-Cancel should come first. */
            OAILOG_WARNING(LOG_MME_APP,
                           "HO command not set yet for UE. Setting S1AP "
                           "reference to idle mode for UE " MME_UE_S1AP_ID_FMT
                           ". Not performing implicit detach. \n",
                           ue_context->privates.mme_ue_s1ap_id);
            mme_app_send_s1ap_handover_preparation_failure(
                ue_context->privates.mme_ue_s1ap_id,
                ue_context->privates.fields.enb_ue_s1ap_id,
                ue_context->privates.fields.sctp_assoc_id_key,
                S1AP_HANDOVER_FAILED);

            /** Inform the target side of the failure. */
            message_p = itti_alloc_new_message(TASK_MME_APP,
                                               S10_RELOCATION_CANCEL_REQUEST);
            DevAssert(message_p != NULL);
            itti_s10_relocation_cancel_request_t *relocation_cancel_request_p =
                &message_p->ittiMsg.s10_relocation_cancel_request;
            memset((void *)relocation_cancel_request_p, 0,
                   sizeof(itti_s10_relocation_cancel_request_t));
            relocation_cancel_request_p->teid =
                s10_proc_mme_handover->proc
                    .remote_teid; /**< May or may not be 0. */
            relocation_cancel_request_p->local_teid =
                ue_context->privates.fields
                    .local_mme_teid_s10; /**< May or may not be 0. */
            // todo: check the table!
            memcpy((void *)&relocation_cancel_request_p->mme_peer_ip,
                   s10_proc_mme_handover->proc.peer_ip,
                   (s10_proc_mme_handover->proc.peer_ip->sa_family == AF_INET)
                       ? sizeof(struct sockaddr_in)
                       : sizeof(struct sockaddr_in6));
            /** IMSI. */
            memcpy((void *)&relocation_cancel_request_p->imsi,
                   &s10_proc_mme_handover->imsi, sizeof(imsi_t));
            MSC_LOG_TX_MESSAGE(MSC_MMEAPP_MME, MSC_S10_MME, NULL, 0,
                               "0 RELOCATION_CANCEL_REQUEST_MESSAGE");
            itti_send_msg_to_task(TASK_S10, INSTANCE_DEFAULT, message_p);

            /** Not setting UE into idle mode at source (not changing the UE
             * state). Still removing the S10 tunnel (see what happens..). */
            remove_s10_tunnel_endpoint(ue_context,
                                       s10_proc_mme_handover->proc.peer_ip);
            mme_app_delete_s10_procedure_mme_handover(ue_context);
            OAILOG_FUNC_OUT(LOG_MME_APP);
          }
        }
      } else {
        OAILOG_DEBUG(LOG_MME_APP,
                     "UE " MME_UE_S1AP_ID_FMT
                     " is in idle state (assuming idle tau). Removing handover "
                     "procedure. \n",
                     ue_context->privates.mme_ue_s1ap_id);
        /** Send a Notify Request to trigger idle TAU. */
        mme_app_itti_notify_request(ue_context->privates.fields.imsi,
                                    &s10_proc_mme_handover->target_tai.plmn,
                                    true);
        mme_app_delete_s10_procedure_mme_handover(ue_context);
        OAILOG_FUNC_OUT(LOG_MME_APP);
      }
    }

    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
}

//------------------------------------------------------------------------------
mme_app_s10_proc_mme_handover_t *mme_app_create_s10_procedure_mme_handover(
    struct ue_context_s *const ue_context, bool target_mme,
    mme_app_s10_proc_type_t s1ap_ho_type, struct sockaddr *sockaddr) {
  mme_app_s10_proc_mme_handover_t *s10_proc_mme_handover =
      calloc(1, sizeof(mme_app_s10_proc_mme_handover_t));
  // todo: checking hear for correct allocation
  if (!s10_proc_mme_handover) {
    return NULL;
  }
  s10_proc_mme_handover->proc.type = s1ap_ho_type;
  if (sockaddr) {
    s10_proc_mme_handover->proc.peer_ip = calloc(
        1, (sockaddr->sa_family == AF_INET6) ? sizeof(struct sockaddr_in6)
                                             : sizeof(struct sockaddr_in));
    memcpy(s10_proc_mme_handover->proc.peer_ip, sockaddr,
           (sockaddr->sa_family == AF_INET6) ? sizeof(struct sockaddr_in6)
                                             : sizeof(struct sockaddr_in));
  }

  s10_proc_mme_handover->mme_ue_s1ap_id = ue_context->privates.mme_ue_s1ap_id;

  /*
   * Start a fresh S10 MME Handover Completion timer for the forward relocation
   * request procedure. Give the procedure as the argument.
   */
  if (s1ap_ho_type == MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER && target_mme) {
    s10_proc_mme_handover->proc.proc.time_out =
        (time_out_t)mme_app_handle_mme_s10_handover_completion_timer_expiry;
    mme_config_read_lock(&mme_config);
    if (timer_setup(mme_config.mme_s10_handover_completion_timer * 1, 0,
                    TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,
                    (void *)(ue_context->privates.mme_ue_s1ap_id),
                    &(s10_proc_mme_handover->proc.timer.id)) < 0) {
      OAILOG_ERROR(LOG_MME_APP,
                   "Failed to start the MME Handover Completion timer for UE "
                   "id " MME_UE_S1AP_ID_FMT " for duration %d \n",
                   ue_context->privates.mme_ue_s1ap_id,
                   mme_config.mme_s10_handover_completion_timer * 1);
      s10_proc_mme_handover->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;
      /**
       * UE will be implicitly detached, if this timer runs out. It should be
       * manually removed. S10 FW Relocation Complete removes this timer.
       */
    } else {
      OAILOG_DEBUG(LOG_MME_APP,
                   "MME APP : Activated the MME Handover Completion timer UE "
                   "id  " MME_UE_S1AP_ID_FMT
                   ". "
                   "Waiting for UE to go back from IDLE mode to ACTIVE mode.. "
                   "Timer Id %lu. Timer duration %d \n",
                   ue_context->privates.mme_ue_s1ap_id,
                   s10_proc_mme_handover->proc.timer.id,
                   mme_config.mme_s10_handover_completion_timer * 1);
      /** Upon expiration, invalidate the timer.. no flag needed. */
    }
    mme_config_unlock(&mme_config);
  } else {
    /*
     * The case that it is INTER-MME-HANDOVER or source side, we start a
     * mme_app_s10_proc timer. It is run until the Forward_Relocation_Complete
     * message arrives. It is used, if no Handover Notify message arrives at the
     * target MME, that the source MME can eventually exit the handover
     * procedure.
     */
    s10_proc_mme_handover->proc.proc.time_out =
        (time_out_t)mme_app_handle_mobility_completion_timer_expiry;
    mme_config_read_lock(&mme_config);
    if (timer_setup(mme_config.mme_mobility_completion_timer * 1, 0,
                    TASK_MME_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,
                    (void *)(ue_context->privates.mme_ue_s1ap_id),
                    &(s10_proc_mme_handover->proc.timer.id)) < 0) {
      OAILOG_ERROR(LOG_MME_APP,
                   "Failed to start the MME Mobility Completion timer for UE "
                   "id " MME_UE_S1AP_ID_FMT " for duration %d \n",
                   ue_context->privates.mme_ue_s1ap_id,
                   (mme_config.mme_mobility_completion_timer * 1));
      s10_proc_mme_handover->proc.timer.id = MME_APP_TIMER_INACTIVE_ID;
      /**
       * UE will be implicitly detached, if this timer runs out. It should be
       * manually removed. S10 FW Relocation Complete removes this timer.
       */
    } else {
      OAILOG_DEBUG(LOG_MME_APP,
                   "MME APP : Activated the MME Mobility Completion timer for "
                   "the source MME for UE id  " MME_UE_S1AP_ID_FMT
                   ". "
                   "Waiting for UE to go back from IDLE mode to ACTIVE mode.. "
                   "Timer Id %lu. Timer duration %d \n",
                   ue_context->privates.mme_ue_s1ap_id,
                   s10_proc_mme_handover->proc.timer.id,
                   (mme_config.mme_mobility_completion_timer * 1));
      /** Upon expiration, invalidate the timer.. no flag needed. */
    }
    mme_config_unlock(&mme_config);
  }
  /*
   * The case that it is INTRA-MME-HANDOVER or source side, we don't start a
   * mme_app_s10_proc timer. We only leave the UE reference timer to remove the
   * UE_Reference towards the source enb. That's not part of the procedure and
   * also should run if the process is removed.
   */
  /** Add the S10 procedure. */
  mme_app_s10_proc_t *s10_proc = (mme_app_s10_proc_t *)s10_proc_mme_handover;

  s10_proc->target_mme = target_mme;
  if (!ue_context->s10_procedures) {
    ue_context->s10_procedures = calloc(1, sizeof(struct s10_procedures_s));
    LIST_INIT(ue_context->s10_procedures);
  }
  LIST_INSERT_HEAD((ue_context->s10_procedures), s10_proc, entries);
  return s10_proc_mme_handover;
}

//------------------------------------------------------------------------------
mme_app_s10_proc_mme_handover_t *mme_app_get_s10_procedure_mme_handover(
    struct ue_context_s *const ue_context) {
  if (ue_context->s10_procedures) {
    mme_app_s10_proc_t *s10_proc = NULL;

    LIST_FOREACH(s10_proc, ue_context->s10_procedures, entries) {
      if (MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER == s10_proc->type) {
        return (mme_app_s10_proc_mme_handover_t *)s10_proc;
      } else if (MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER == s10_proc->type) {
        return (mme_app_s10_proc_mme_handover_t *)s10_proc;
      }
    }
  }
  return NULL;
}

//------------------------------------------------------------------------------
static void mme_app_free_s10_procedure_mme_handover(
    mme_app_s10_proc_t **s10_proc) {
  /** Remove the pending IEs. */
  mme_app_s10_proc_mme_handover_t **s10_proc_mme_handover_pp =
      (mme_app_s10_proc_mme_handover_t **)s10_proc;
  if ((*s10_proc_mme_handover_pp)->nas_s10_context.mm_eps_ctx) {
    free_wrapper((void *)&(
        (*s10_proc_mme_handover_pp)
            ->nas_s10_context.mm_eps_ctx)); /**< Setting the reference inside
                                               the procedure also to null. */
  }
  if ((*s10_proc_mme_handover_pp)
          ->source_to_target_eutran_f_container.container_value) {
    bdestroy_wrapper(
        &(*s10_proc_mme_handover_pp)
             ->source_to_target_eutran_f_container.container_value);
  }
  /*
   * Todo: Make a generic function for this (proc_element with free_wrapper_ie()
   * method).
   */
  /** PDN Connections. */
  if ((*s10_proc_mme_handover_pp)->pdn_connections) {
    free_mme_ue_eps_pdn_connections(
        &(*s10_proc_mme_handover_pp)->pdn_connections);
  }

  if ((*s10_proc_mme_handover_pp)->proc.peer_ip) {
    free_wrapper((void *)&(*s10_proc_mme_handover_pp)->proc.peer_ip);
  }

  (*s10_proc_mme_handover_pp)->s10_mme_handover_timeout =
      NULL;  // todo: deallocate too

  //  (*s10_proc_mme_handover_pp)->entries.le_next = NULL;
  //  (*s10_proc_mme_handover_pp)->entries.le_prev = NULL;

  // DO here specific releases (memory,etc)
  // nothing to do actually
  free_wrapper((void **)s10_proc_mme_handover_pp);
}

/*
 * Handover procedure used for inter and intra MME S1 handover.
 */
//------------------------------------------------------------------------------
void mme_app_delete_s10_procedure_mme_handover(
    struct ue_context_s *const ue_context) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  if (ue_context->s10_procedures) {
    mme_app_s10_proc_t *s10_proc = NULL, *s10_proc_safe = NULL;

    LIST_FOREACH_SAFE(s10_proc, ue_context->s10_procedures, entries,
                      s10_proc_safe) {
      if (MME_APP_S10_PROC_TYPE_INTER_MME_HANDOVER == s10_proc->type) {
        if (s10_proc->target_mme &&
            !((mme_app_s10_proc_mme_handover_t *)s10_proc)
                 ->handover_completed) {
          if (ue_context->privates.s1_ue_context_release_cause ==
                  S1AP_HANDOVER_CANCELLED ||
              ue_context->privates.s1_ue_context_release_cause ==
                  S1AP_HANDOVER_FAILED) {
            OAILOG_WARNING(
                LOG_MME_APP,
                "Handover for UE " MME_UE_S1AP_ID_FMT
                " failed on the target side. Continuing with the deletion. \n",
                ue_context->privates.mme_ue_s1ap_id);
          } else {
            OAILOG_ERROR(
                LOG_MME_APP,
                "Handover for UE " MME_UE_S1AP_ID_FMT
                " on target MME has not been completed yet.. Cannot delete. \n",
                ue_context->privates.mme_ue_s1ap_id);
            OAILOG_FUNC_OUT(LOG_MME_APP);
          }
        }

        LIST_REMOVE(s10_proc, entries);
        /*
         * Cannot remove the S10 tunnel endpoint with transaction.
         * The S10 tunnel endpoint will remain throughout the UE contexts
         * lifetime. It will be removed when the UE context is removed. The UE
         * context will also be registered by the S10 teid also. We cannot get
         * the process without getting the UE context, first.
         */
        /** Check if a timer is running, if so remove the timer. */
        if (s10_proc->timer.id != MME_APP_TIMER_INACTIVE_ID) {
          if (timer_remove(s10_proc->timer.id, NULL)) {
            OAILOG_ERROR(LOG_MME_APP,
                         "Failed to stop the procedure timer for -MMME "
                         "handover for UE id  %d \n",
                         ue_context->privates.mme_ue_s1ap_id);
            s10_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
          } else {
            OAILOG_DEBUG(LOG_MME_APP,
                         "Successfully removed timer for -MMME handover for UE "
                         "id  %d \n",
                         ue_context->privates.mme_ue_s1ap_id);
          }
        }
        s10_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
        /** Remove the S10 Tunnel endpoint and set the UE context S10 as
         * invalid. */
        //        if(s10_proc->target_mme)
        remove_s10_tunnel_endpoint(ue_context, s10_proc->peer_ip);
        mme_app_free_s10_procedure_mme_handover(&s10_proc);
      } else if (MME_APP_S10_PROC_TYPE_INTRA_MME_HANDOVER == s10_proc->type) {
        LIST_REMOVE(s10_proc, entries);
        /*
         * Cannot remove the S10 tunnel endpoint with transaction.
         * The S10 tunnel endpoint will remain throughout the UE contexts
         * lifetime. It will be removed when the UE context is removed. The UE
         * context will also be registered by the S10 teid also. We cannot get
         * the process without getting the UE context, first.
         */
        /** Check if a timer is running, if so remove the timer. */
        if (s10_proc->timer.id != MME_APP_TIMER_INACTIVE_ID) {
          if (timer_remove(s10_proc->timer.id, NULL)) {
            OAILOG_ERROR(LOG_MME_APP,
                         "Failed to stop the procedure timer for -MMME "
                         "handover for UE id  %d \n",
                         ue_context->privates.mme_ue_s1ap_id);
            s10_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
          }
        }
        s10_proc->timer.id = MME_APP_TIMER_INACTIVE_ID;
        /** Remove the S10 Tunnel endpoint and set the UE context S10 as
         * invalid. */
        mme_app_free_s10_procedure_mme_handover(&s10_proc);
      }
    }
    /** Remove the list. */
    LIST_INIT(ue_context->s10_procedures);
    free_wrapper((void **)&ue_context->s10_procedures);

    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  OAILOG_INFO(LOG_MME_APP,
              "No S10 Procedures existing for UE " MME_UE_S1AP_ID_FMT ". \n",
              ue_context->privates.mme_ue_s1ap_id);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

/*
 * Our procedure is not for a single message but currently for a whole inter_mme
 * handover procedure.
 */

//------------------------------------------------------------------------------
static void mme_app_free_s1ap_procedure_modify_bearer_ind(
    mme_app_s1ap_proc_t **s1ap_proc) {
  // DO here specific releases (memory,etc)
  // mme_app_s1ap_proc_modify_bearer_ind_t ** s1ap_proc_mbi =
  // (mme_app_s1ap_proc_modify_bearer_ind_t**)s1ap_proc;
  // ...
  free_wrapper((void **)s1ap_proc);
}

//------------------------------------------------------------------------------
void mme_app_delete_s1ap_procedures(ue_session_pool_t *const ue_session_pool) {
  if (ue_session_pool) {
    mme_app_s1ap_proc_t *s1ap_proc1 = NULL;
    mme_app_s1ap_proc_t *s1ap_proc2 = NULL;

    // todo: intra
    s1ap_proc1 = LIST_FIRST(
        &ue_session_pool->s1ap_procedures); /* Faster List Deletion. */
    while (s1ap_proc1) {
      s1ap_proc2 = LIST_NEXT(s1ap_proc1, entries);
      // Clean procedures upon their types
      if (MME_APP_S1AP_PROC_TYPE_E_RAB_MODIFY_BEARER_IND == s1ap_proc1->type) {
        mme_app_free_s1ap_procedure_modify_bearer_ind(&s1ap_proc1);
      } else {
        free_wrapper((void **)s1ap_proc1);
      }
      s1ap_proc1 = s1ap_proc2;
    }
    LIST_INIT(&ue_session_pool->s1ap_procedures);
    free_wrapper((void **)&ue_session_pool->s1ap_procedures);
  }
}

//------------------------------------------------------------------------------
mme_app_s1ap_proc_modify_bearer_ind_t *
mme_app_create_s1ap_procedure_modify_bearer_ind(
    ue_session_pool_t *const ue_session_pool) {
  /** Check if the list of S11 procedures is empty. */
  if (!LIST_EMPTY(&ue_session_pool->s1ap_procedures)) {
    OAILOG_ERROR(LOG_MME_APP,
                 "UE with ueId " MME_UE_S1AP_ID_FMT
                 " has already a S1AP procedure ongoing. Cannot create MBI "
                 "procedure. \n",
                 ue_session_pool->privates.mme_ue_s1ap_id);
    return NULL;
  }
  mme_app_s1ap_proc_modify_bearer_ind_t *proc =
      calloc(1, sizeof(mme_app_s1ap_proc_modify_bearer_ind_t));
  proc->proc.proc.type = MME_APP_BASE_PROC_TYPE_S1AP;
  proc->proc.type = MME_APP_S1AP_PROC_TYPE_E_RAB_MODIFY_BEARER_IND;
  mme_app_s1ap_proc_t *s1ap_proc = (mme_app_s1ap_proc_t *)proc;
  proc->mme_ue_s1ap_id = ue_session_pool->privates.mme_ue_s1ap_id;

  /** Initialize the of the procedure. */

  LIST_INIT(&ue_session_pool->s1ap_procedures);
  LIST_INSERT_HEAD((&ue_session_pool->s1ap_procedures), s1ap_proc, entries);

  return proc;
}
//------------------------------------------------------------------------------
int mme_app_run_s1ap_procedure_modify_bearer_ind(
    mme_app_s1ap_proc_modify_bearer_ind_t *proc,
    const itti_s1ap_e_rab_modification_ind_t *const e_rab_modification_ind) {
  OAILOG_FUNC_IN(LOG_MME_APP);
  memcpy((void *)&proc->e_rab_to_be_modified_list,
         (void *)&e_rab_modification_ind->e_rab_to_be_modified_list,
         sizeof(proc->e_rab_to_be_modified_list));

  memcpy((void *)&proc->e_rab_not_to_be_modified_list,
         (void *)&e_rab_modification_ind->e_rab_not_to_be_modified_list,
         sizeof(proc->e_rab_not_to_be_modified_list));

  ue_session_pool_t *ue_session_pool =
      mme_ue_session_pool_exists_mme_ue_s1ap_id(
          &mme_app_desc.mme_ue_session_pools, proc->mme_ue_s1ap_id);

  if (!ue_session_pool) {
    OAILOG_INFO(LOG_MME_APP,
                "No UE session pool is found" MME_UE_S1AP_ID_FMT ". \n",
                proc->mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNerror);
  }
  // todo: LOCK_UE_SESSION_POOL
  // todo: checking on procedures of the function..
  // mme_app_is_ue_context_clean(ue_context)?!?
  /** Get the PDN Context. */
  // TODO sort by pdn_context if different SGWs (not the case now)
  pdn_context_t *registered_pdn_ctx =
      RB_MIN(PdnContexts, &ue_session_pool->pdn_contexts);

  for (int nb_bearer = 0;
       nb_bearer < proc->e_rab_to_be_modified_list.no_of_items; nb_bearer++) {
    e_rab_to_be_modified_bearer_mod_ind_t *item =
        &proc->e_rab_to_be_modified_list.item[nb_bearer];
    /** Get the bearer context. */
    bearer_context_new_t *bearer_context = NULL;
    mme_app_get_session_bearer_context_from_all(ue_session_pool, item->e_rab_id,
                                                &bearer_context);
    if (!bearer_context) {
      OAILOG_ERROR(
          LOG_MME_APP,
          "No bearer context (ebi=%d) could be found for " MME_UE_S1AP_ID_FMT
          ". Skipping.. \n",
          item->e_rab_id, proc->mme_ue_s1ap_id);
      continue;
    }
    /** Set all bearers, not in the failed list, to inactive. */
    bearer_context->bearer_state &= (~BEARER_STATE_ACTIVE);
    /** Update the FTEID of the bearer context and uncheck the established
     * state. */
    bearer_context->enb_fteid_s1u.teid = item->s1_xNB_fteid.teid;
    bearer_context->enb_fteid_s1u.interface_type = S1_U_ENODEB_GTP_U;
    /** Set the IP address from the FTEID. */
    if (item->s1_xNB_fteid.ipv4) {
      bearer_context->enb_fteid_s1u.ipv4 = 1;
      bearer_context->enb_fteid_s1u.ipv4_address.s_addr =
          item->s1_xNB_fteid.ipv4_address.s_addr;
    }
    if (item->s1_xNB_fteid.ipv6) {
      bearer_context->enb_fteid_s1u.ipv6 = 1;
      memcpy(&bearer_context->enb_fteid_s1u.ipv6_address,
             &item->s1_xNB_fteid.ipv6_address, sizeof(item->s1_xNB_fteid));
    }
    bearer_context->bearer_state |= BEARER_STATE_ENB_CREATED;
    bearer_context->bearer_state |=
        BEARER_STATE_MME_CREATED;  // todo: remove this flag.. unnecessary
  }

  uint8_t flags = INTERNAL_FLAG_E_RAB_MOD_IND;
  mme_app_send_s11_modify_bearer_req(ue_session_pool, registered_pdn_ctx,
                                     flags);
  //  todo: UNLOCK_UE_SESSION_POOL;
  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
}
//------------------------------------------------------------------------------
mme_app_s1ap_proc_modify_bearer_ind_t *
mme_app_get_s1ap_procedure_modify_bearer_ind(
    ue_session_pool_t *const ue_session_pool) {
  mme_app_s1ap_proc_t *s1ap_proc = NULL;

  LIST_FOREACH(s1ap_proc, &ue_session_pool->s1ap_procedures, entries) {
    if (MME_APP_S1AP_PROC_TYPE_E_RAB_MODIFY_BEARER_IND == s1ap_proc->type) {
      return (mme_app_s1ap_proc_modify_bearer_ind_t *)s1ap_proc;
    }
  }
  return NULL;
}

//------------------------------------------------------------------------------
// return true if proc to be deleted
bool mme_app_s1ap_procedure_modify_bearer_ind_handle_modify_bearer_response(
    ue_context_t *const ue_context, mme_app_s1ap_proc_modify_bearer_ind_t *proc,
    const itti_s11_modify_bearer_response_t *const mbr,
    ue_session_pool_t *const ue_session_pool) {
  OAILOG_INFO(LOG_MME_APP,
              "modify_bearer_response for UE " MME_UE_S1AP_ID_FMT
              ". %d bearers modified\n",
              ue_context->privates.mme_ue_s1ap_id,
              mbr->bearer_contexts_modified.num_bearer_context);

  for (int i = 0; i < mbr->bearer_contexts_modified.num_bearer_context; ++i) {
    const bearer_context_modified_t *const mbr_bearer_context =
        &mbr->bearer_contexts_modified.bearer_context[i];

    // TODO check bearer id in
    bearer_context_new_t *current_bearer_p = NULL;
    mme_app_get_session_bearer_context_from_all(
        ue_session_pool, mbr_bearer_context->eps_bearer_id, &current_bearer_p);

    if (current_bearer_p) {
      // TODO check bearer state...
      if (mbr_bearer_context->cause.cause_value == REQUEST_ACCEPTED) {
        proc->e_rab_modified_list
            .e_rab_id[proc->e_rab_modified_list.no_of_items++] =
            mbr_bearer_context->eps_bearer_id;
        OAILOG_INFO(LOG_MME_APP,
                    "modify_bearer_response for UE " MME_UE_S1AP_ID_FMT
                    ". EBI %d modified OK\n",
                    ue_context->privates.mme_ue_s1ap_id,
                    mbr_bearer_context->eps_bearer_id);
      } else {
        proc->e_rab_failed_to_be_modified_list
            .item[proc->e_rab_failed_to_be_modified_list.no_of_items]
            .e_rab_id = mbr_bearer_context->eps_bearer_id;

        // TODO better gtpv2c to s1ap cause conversion ?
        proc->e_rab_failed_to_be_modified_list
            .item[proc->e_rab_failed_to_be_modified_list.no_of_items]
            .cause.present = S1AP_Cause_PR_misc;
        proc->e_rab_failed_to_be_modified_list
            .item[proc->e_rab_failed_to_be_modified_list.no_of_items]
            .cause.choice.misc = S1AP_CauseMisc_unspecified;
        proc->e_rab_failed_to_be_modified_list.no_of_items++;
        OAILOG_INFO(LOG_MME_APP,
                    "modify_bearer_response for UE " MME_UE_S1AP_ID_FMT
                    ". EBI %d modified ERROR\n",
                    ue_context->privates.mme_ue_s1ap_id,
                    mbr_bearer_context->eps_bearer_id);
      }
    } else {
      OAILOG_ERROR(
          LOG_MME_APP,
          "No bearer context (ebi=%d) could be found for " MME_UE_S1AP_ID_FMT
          ". Skipping.. \n",
          mbr_bearer_context->eps_bearer_id, proc->mme_ue_s1ap_id);
    }

    proc->num_status_received++;
  }

  if (proc->num_status_received ==
      proc->e_rab_to_be_modified_list.no_of_items) {
    mme_app_send_s1ap_e_rab_mofification_confirm(
        ue_context->privates.mme_ue_s1ap_id,
        ue_context->privates.fields.enb_ue_s1ap_id, proc);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void mme_app_delete_s1ap_procedure_modify_bearer_ind(
    ue_session_pool_t *const ue_session_pool) {
  /** Check if the list of S11 procedures is empty. */
  mme_app_s1ap_proc_t *s1ap_proc = NULL, *s1ap_proc_safe = NULL;

  LIST_FOREACH_SAFE(s1ap_proc, &ue_session_pool->s1ap_procedures, entries,
                    s1ap_proc_safe) {
    if (MME_APP_S1AP_PROC_TYPE_E_RAB_MODIFY_BEARER_IND == s1ap_proc->type) {
      LIST_REMOVE(s1ap_proc, entries);
      mme_app_free_s1ap_procedure_modify_bearer_ind(&s1ap_proc);
      return;
    }
  }

  if (LIST_EMPTY(&ue_session_pool->s1ap_procedures)) {
    LIST_INIT(&ue_session_pool->s1ap_procedures);
    OAILOG_INFO(LOG_MME_APP,
                "UE with ueId " MME_UE_S1AP_ID_FMT
                " has no more S1AP procedures left. Cleared the list. \n",
                ue_session_pool->privates.mme_ue_s1ap_id);
  }
}
