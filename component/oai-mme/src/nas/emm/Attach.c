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

/*****************************************************************************

  Source      Attach.c

  Version     0.1

  Date        2012/12/04

  Product     NAS stack

  Subsystem   EPS Mobility Management

  Author      Frederic Maurel, Lionel GAUTHIER

  Description Defines the attach related EMM procedure executed by the
        Non-Access Stratum.

        To get internet connectivity from the network, the network
        have to know about the UE. When the UE is switched on, it
        has to initiate the attach procedure to get initial access
        to the network and register its presence to the Evolved
        Packet Core (EPC) network in order to receive EPS services.

        As a result of a successful attach procedure, a context is
        created for the UE in the MME, and a default bearer is esta-
        blished between the UE and the PDN-GW. The UE gets the home
        agent IPv4 and IPv6 addresses and full connectivity to the
        IP network.

        The network may also initiate the activation of additional
        dedicated bearers for the support of a specific service.

*****************************************************************************/

#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bstrlib.h"

#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "3gpp_36.401.h"
#include "3gpp_requirements_24.301.h"
#include "assertions.h"
#include "common_types.h"
#include "conversions.h"
#include "dynamic_memory_check.h"
#include "gcc_diag.h"
#include "log.h"
#include "msc.h"

#include "mme_app_session_context.h"
#include "mme_app_ue_context.h"

#include "NasSecurityAlgorithms.h"
#include "as_message.h"
#include "emm_cause.h"
#include "emm_data.h"
#include "emm_proc.h"
#include "emm_sap.h"
#include "mme_api.h"
#include "nas_message.h"
#include "nas_timer.h"

#include "mme_app_defs.h"
#include "mme_config.h"
#include "nas_itti_messaging.h"
#include "networkDef.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* String representation of the EPS attach type */
static const char *_emm_attach_type_str[] = {"EPS", "IMSI", "EMERGENCY",
                                             "RESERVED"};

/*
   --------------------------------------------------------------------------
        Internal data handled by the attach procedure in the MME
   --------------------------------------------------------------------------
*/
/*
   Timer handlers
*/
static void _emm_attach_t3450_handler(void *);

/*
   Functions that may initiate EMM common procedures
*/
static int _emm_start_attach_proc_authentication(
    emm_data_context_t *emm_context, nas_emm_attach_proc_t *attach_proc);
static int _emm_start_attach_proc_security(emm_data_context_t *emm_context,
                                           nas_emm_attach_proc_t *attach_proc);

static int _emm_attach_security(emm_data_context_t *emm_context);

static int _emm_attach_success_identification_cb(
    emm_data_context_t *emm_context);
static int _emm_attach_failure_identification_cb(
    emm_data_context_t *emm_context);
static int _emm_attach_success_authentication_cb(
    emm_data_context_t *emm_context);
static int _emm_attach_failure_authentication_cb(
    emm_data_context_t *emm_context);
static int _emm_attach_success_security_cb(
    struct emm_data_context_s *emm_context);
static int _emm_attach_failure_security_cb(emm_data_context_t *emm_context);

/*
   Abnormal case attach procedures
*/
static int _emm_attach_release(emm_data_context_t *emm_context);
static int _emm_attach_abort(struct emm_data_context_s *emm_context,
                             struct nas_emm_base_proc_s *emm_base_proc);
static int _emm_attach_run_procedure(emm_data_context_t *emm_context);
static int _emm_send_attach_accept(emm_data_context_t *const emm_context);

static bool _emm_attach_ies_have_changed(mme_ue_s1ap_id_t ue_id,
                                         emm_attach_request_ies_t *const ies1,
                                         emm_attach_request_ies_t *const ies2);

static void _emm_proc_create_procedure_attach_request(
    emm_data_context_t *const ue_context, emm_attach_request_ies_t *const ies,
    retry_cb_t retry_cb);

static int _emm_attach_retry_procedure(mme_ue_s1ap_id_t ue_id);

static int _emm_attach_update(emm_data_context_t *const emm_context,
                              emm_attach_request_ies_t *const ies);

static int _emm_attach_accept_retx(emm_data_context_t *emm_context);

static int emm_proc_attach_request_validity(
    emm_data_context_t *emm_context, mme_ue_s1ap_id_t new_ue_id,
    emm_attach_request_ies_t *const ies);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
   --------------------------------------------------------------------------
            Attach procedure executed by the MME
   --------------------------------------------------------------------------
*/
/*
 *
 * Name:    emm_proc_attach_request()
 *
 * Description: Performs the UE requested attach procedure
 *
 *              3GPP TS 24.301, section 5.5.1.2.3
 *      The network may initiate EMM common procedures, e.g. the
 *      identification, authentication and security mode control
 *      procedures during the attach procedure, depending on the
 *      information received in the ATTACH REQUEST message (e.g.
 *      IMSI, GUTI and KSI).
 *
 * Inputs:  ue_id:      UE lower layer identifier. If the ue_id gives us an EMM
 * context, we could find an mme_ue_s1ap_id via s-tmsi and re-use it. type: Type
 * of the requested attach ies:       Information ElementStrue if the security
 * context is of type Others:    _emm_data
 *
 * Outputs:     None
 *      Return:    RETURNok, RETURNerror
 *      Others:    _emm_data
 *
 */
//------------------------------------------------------------------------------
int emm_proc_attach_request(mme_ue_s1ap_id_t ue_id,
                            emm_attach_request_ies_t *const ies,
                            emm_data_context_t **duplicate_emm_ue_ctx_pP) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;
  emm_fsm_state_t fsm_state = EMM_DEREGISTERED;
  ue_context_t *ue_context = NULL;

  emm_data_context_t *new_emm_ue_ctx = NULL;

  imsi64_t imsi64 = INVALID_IMSI64;
  mme_ue_s1ap_id_t old_ue_id = INVALID_MME_UE_S1AP_ID;
  nas_emm_attach_proc_t *attach_procedure = NULL;

  if (ies->imsi) {
    imsi64 = imsi_to_imsi64(ies->imsi);
  }

  OAILOG_INFO(LOG_NAS_EMM,
              "EMM-PROC  ATTACH - EPS attach type = %s (%d) requested "
              "(ue_id=" MME_UE_S1AP_ID_FMT ")\n",
              _emm_attach_type_str[ies->type], ies->type, ue_id);

  /**
   * First check if the MME_APP UE context is valid.
   */
  if (INVALID_MME_UE_S1AP_ID == ue_id) {
    /** Received an invalid UE_ID. */
    OAILOG_INFO(LOG_NAS_EMM,
                "EMM-PROC  ATTACH - Received an invalid ue_id. Not continuing "
                "with the attach request. \n",
                ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNerror);
  }
  /** Retrieve the MME_APP UE context. It must always exist. It may or may not
   * be linked to an existing EMM Data context. */
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id(
      &mme_app_desc.mme_ue_contexts, ue_id);
  if (!ue_context) {
    OAILOG_INFO(LOG_NAS_EMM,
                "EMM-PROC  ATTACH - For ueId " MME_UE_S1AP_ID_FMT
                " no UE context exists. \n",
                ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNerror);
  }
  // Check whether request if for emergency bearer service.

  /*
   * Requirement MME24.301R10_5.5.1.1_1
   * MME not configured to support attach for emergency bearer services
   * shall reject any request to attach with an attach type set to "EPS
   * emergency attach".
   */
  if (!(_emm_data.conf.eps_network_feature_support &
        EPS_NETWORK_FEATURE_SUPPORT_EMERGENCY_BEARER_SERVICES_IN_S1_MODE_SUPPORTED) &&
      (EMM_ATTACH_TYPE_EMERGENCY == ies->type)) {
    REQUIREMENT_3GPP_24_301(R10_5_5_1__1);
    // TODO: update this if/when emergency attach is supported
    /*
     * Do not accept the UE to attach for emergency services
     */
    struct nas_emm_attach_proc_s no_attach_proc = {0};
    no_attach_proc.ue_id = ue_id;
    no_attach_proc.emm_cause = ies->imei ? EMM_CAUSE_IMEI_NOT_ACCEPTED
                                         : EMM_CAUSE_NOT_AUTHORIZED_IN_PLMN;
    no_attach_proc.esm_msg_out = NULL;
    rc = _emm_attach_reject((struct nas_base_proc_s *)&no_attach_proc, NULL);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
  }

  /**
   * We will remain the split between the NAS and MME_APP. We may create MME_APP
   * contexts without NAS messages (Handover) and NAS contexts should only be
   * created via NAS messages.. Get the UE's EMM context if it exists by
   * MME_UE_S1AP_ID, IMSI and GUTI and proceed with it. Check if the ID is valid
   * and proceed with it.
   *
   * todo: @Lionel: we only check when no S-TMSI was found via GUTI and IMSI if
   * an EMM context is existing. 24.301 does not say anything about that the EMM
   * context should be discarded when no S-TMSI was sent. We assume that the
   * remaining variables are OK and don't check them here.
   */
  *duplicate_emm_ue_ctx_pP = emm_data_context_get(&_emm_data, ue_id);
  if (!(*duplicate_emm_ue_ctx_pP)) {
    /** It can only have IMSI or GUTI. */
    if (ies->guti && !(INVALID_M_TMSI == ies->guti->m_tmsi)) {
      (*duplicate_emm_ue_ctx_pP) =
          emm_data_context_get_by_guti(&_emm_data, ies->guti);
      if ((*duplicate_emm_ue_ctx_pP)) {
        /** We found an EMM context. Don't clean it up, continue to use it. */
        OAILOG_WARNING(LOG_NAS_EMM,
                       "EMM-PROC  - We found an EMM context from GUTI " GUTI_FMT
                       " with old mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ". \n",
                       GUTI_ARG(&(*duplicate_emm_ue_ctx_pP)->_guti),
                       (*duplicate_emm_ue_ctx_pP)->ue_id);
        /** Continue to check for EMM context and their validity. */
      }
    } else if (ies->imsi) { /**< If we could not find one per IMSI. */
      (*duplicate_emm_ue_ctx_pP) =
          emm_data_context_get_by_imsi(&_emm_data, imsi64);
      if ((*duplicate_emm_ue_ctx_pP)) {
        OAILOG_WARNING(
            LOG_NAS_EMM,
            "EMM-PROC  - We found an EMM context from IMSI " IMSI_64_FMT
            " with old mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT ". \n",
            (*duplicate_emm_ue_ctx_pP)->_imsi64,
            (*duplicate_emm_ue_ctx_pP)->ue_id);
      }
    } /** If we have an EMM UE context already, the S-TMSI was matched to an
         GUTI successfully, so the IMSI should be OK, as well. */
  }
  if ((*duplicate_emm_ue_ctx_pP)) { /**< We found a matching EMM context via
                                       IMSI or S-TMSI. */
    /** Check the validity of the existing EMM context It may or may not have
     * another MME_APP UE context. */
    mme_ue_s1ap_id_t old_mme_ue_id = (*duplicate_emm_ue_ctx_pP)->ue_id;
    rc = emm_proc_attach_request_validity((*duplicate_emm_ue_ctx_pP), ue_id,
                                          ies);
    /** Retrieve the duplicate ue context again. */
    (*duplicate_emm_ue_ctx_pP) =
        emm_data_context_get(&_emm_data, old_mme_ue_id);
    /** Check the return value. */
    if (rc != RETURNok) {
      /** Not continuing with the Attach-Request (it might be rejected, a
       * previous attach accept might have been resent or just ignored). */
      OAILOG_WARNING(LOG_NAS_EMM,
                     "EMM-PROC  - Not continuing with the Attach-Request. "
                     "\n"); /**< All rejects should be sent and everything
                               should be handled by now. */
      /** Check the duplicate UE detection, remove the old duplicate UE, too. */
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
    } else {
      OAILOG_INFO(
          LOG_NAS_EMM,
          "EMM-PROC  - Continuing to handle the new Attach-Request. \n");
    }
  } else {
    OAILOG_WARNING(LOG_NAS_EMM,
                   "EMM-PROC  - No old EMM context exists. Continuing with new "
                   "EMM context for " MME_UE_S1AP_ID_FMT ". \n",
                   ue_id);
    /** Continuing depending on the error status. */
  }

  OAILOG_INFO(
      LOG_NAS_EMM,
      "EMM-PROC  - Continuing for Attach Request for UE_ID " MME_UE_S1AP_ID_FMT
      " after validation of the attach request. \n",
      ue_id);
  if (!(*duplicate_emm_ue_ctx_pP)) {
    OAILOG_INFO(
        LOG_NAS_EMM,
        "EMM-PROC  - No old EMM context was found for UE_ID " MME_UE_S1AP_ID_FMT
        ". \n",
        ue_id);
    /*
     * Create UE's EMM context
     */
    new_emm_ue_ctx =
        (emm_data_context_t *)calloc(1, sizeof(emm_data_context_t));
    if (!new_emm_ue_ctx) {
      OAILOG_WARNING(LOG_NAS_EMM,
                     "EMM-PROC  - Failed to create EMM context for "
                     "ueId " MME_UE_S1AP_ID_FMT ". \n",
                     ue_id);
      /*
       * Do not accept the UE to attach to the network
       */
      struct nas_emm_attach_proc_s no_attach_proc = {0};
      no_attach_proc.ue_id = ue_id;
      no_attach_proc.emm_cause = EMM_CAUSE_ILLEGAL_UE;
      no_attach_proc.esm_msg_out = NULL;
      rc = _emm_attach_reject((struct nas_base_proc_s *)&no_attach_proc, NULL);
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc); /**< Return with an error. */
    }
    /** Initialize the EMM context only in this case, else we have a
     * DEREGISTERED but valid EMM context. */
    /** Check if the EMM context has been initialized, if so set the values. */
    new_emm_ue_ctx->ue_id = ue_id;
    new_emm_ue_ctx->is_dynamic = true;
    OAILOG_NOTICE(LOG_NAS_EMM,
                  "EMM-PROC  - Create EMM context ue_id = " MME_UE_S1AP_ID_FMT
                  "\n",
                  ue_id);
    new_emm_ue_ctx->attach_type = ies->type;
    new_emm_ue_ctx->additional_update_type = ies->additional_update_type;
    new_emm_ue_ctx->emm_cause = EMM_CAUSE_SUCCESS;
    emm_init_context(
        new_emm_ue_ctx); /**< Initialize the context, we might do it again if
                            the security was not verified. */
    /** Set the received eksi. */
    emm_ctx_set_security_eksi(new_emm_ue_ctx, ies->ksi);
    /** Add the newly created EMM context. */
    DevAssert(RETURNok == emm_data_context_add(&_emm_data, new_emm_ue_ctx));
  } else if ((*duplicate_emm_ue_ctx_pP)->emm_cause != EMM_CAUSE_SUCCESS) {
    OAILOG_INFO(LOG_NAS_EMM,
                "EMM-PROC  - Old EMM context for UE_ID " MME_UE_S1AP_ID_FMT
                " was invalid. \n",
                ue_id);
    /** If the UE-IDs match, we cannot continue with the current TAU. */
    if (ue_id == (*duplicate_emm_ue_ctx_pP)->ue_id) {
      OAILOG_ERROR(
          LOG_NAS_EMM,
          "EMM-PROC  - The old failed EMM context has same "
          "ueId " MME_UE_S1AP_ID_FMT
          ". Rejecting attach request (should also implicitly detach). \n",
          ue_id);
      struct nas_emm_attach_proc_s no_attach_proc = {0};
      no_attach_proc.ue_id = ue_id;
      no_attach_proc.emm_cause = (*duplicate_emm_ue_ctx_pP)->emm_cause;
      no_attach_proc.esm_msg_out = NULL;
      rc = _emm_attach_reject((struct nas_base_proc_s *)&no_attach_proc, NULL);
      (*duplicate_emm_ue_ctx_pP) =
          emm_data_context_get(&_emm_data, ue_id); /**< Refresh reference. */
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
    }
    /*
     * Create UE's EMM context
     */
    OAILOG_WARNING(LOG_NAS_EMM,
                   "EMM-PROC  - The old failed EMM context has a different "
                   "ueId " MME_UE_S1AP_ID_FMT
                   " than the new one " MME_UE_S1AP_ID_FMT
                   ". Continuing with attach procedure. \n",
                   (*duplicate_emm_ue_ctx_pP)->ue_id, ue_id);
    new_emm_ue_ctx =
        (emm_data_context_t *)calloc(1, sizeof(emm_data_context_t));
    if (!new_emm_ue_ctx) {
      OAILOG_WARNING(LOG_NAS_EMM,
                     "EMM-PROC  - Failed to create EMM context for "
                     "ueId " MME_UE_S1AP_ID_FMT ". \n",
                     ue_id);
      /*
       * Do not accept the UE to attach to the network
       */
      struct nas_emm_attach_proc_s no_attach_proc = {0};
      no_attach_proc.ue_id = ue_id;
      no_attach_proc.emm_cause = EMM_CAUSE_ILLEGAL_UE;
      no_attach_proc.esm_msg_out = NULL;
      rc = _emm_attach_reject((struct nas_base_proc_s *)&no_attach_proc, NULL);
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc); /**< Return with an error. */
    }
    /** Initialize the EMM context only in this case, else we have a
     * DEREGISTERED but valid EMM context. */
    /** Check if the EMM context has been initialized, if so set the values. */
    // todo: num_attach_request?!
    new_emm_ue_ctx->ue_id = ue_id;
    new_emm_ue_ctx->is_dynamic = true;
    OAILOG_NOTICE(LOG_NAS_EMM,
                  "EMM-PROC  - Create EMM context ue_id = " MME_UE_S1AP_ID_FMT
                  "\n",
                  ue_id);
    new_emm_ue_ctx->attach_type = ies->type;
    new_emm_ue_ctx->additional_update_type = ies->additional_update_type;
    new_emm_ue_ctx->emm_cause = EMM_CAUSE_SUCCESS;
    emm_init_context(
        new_emm_ue_ctx); /**< Initialize the context, we might do it again if
                            the security was not verified. */
    /** Add the newly created EMM context: ue_id was checked, imsi&guti were
     * cleared. */
    DevAssert(RETURNok == emm_data_context_add(&_emm_data, new_emm_ue_ctx));
  } else {
    OAILOG_WARNING(
        LOG_NAS_EMM,
        "We have a valid EMM context with IMSI " IMSI_64_FMT
        " from a previous registration with ueId " MME_UE_S1AP_ID_FMT ". \n",
        (*duplicate_emm_ue_ctx_pP)->_imsi64, (*duplicate_emm_ue_ctx_pP)->ue_id);
    /*
     * We have an EMM context with GUTI from a previous registration.
     * Validate it, if the message has been received with a security header and
     * if the security header can be validated we can continue using it.
     */
    if ((*duplicate_emm_ue_ctx_pP)->ue_id != ue_id) {
      new_emm_ue_ctx = calloc(1, sizeof(emm_data_context_t));
      memcpy(new_emm_ue_ctx, (*duplicate_emm_ue_ctx_pP),
             sizeof(**duplicate_emm_ue_ctx_pP));
      new_emm_ue_ctx->_emm_fsm_state = EMM_DEREGISTERED;

      mme_ue_s1ap_id_t old_ue_id = (*duplicate_emm_ue_ctx_pP)->ue_id;
      /** Remove the (old) EMM context from the tables, update the EMM context
       * ue_id. */
      new_emm_ue_ctx->ue_id = ue_id;
      /*
       * The old UE Id can not already be in REGISTERED state.
       * If there are some differences of the current structure to the received
       * message, the context will be implicitly detached (together with all its
       * layers). Else, if we can use it, we can also continue with the newly
       * created UE references and don't have to re-link any of them to the old
       * UE context.
       */
      OAILOG_WARNING(
          LOG_NAS_EMM,
          "EMM-PROC  - Updated the EMM Context with IMSI " IMSI_64_FMT
          " from old ueId " MME_UE_S1AP_ID_FMT
          " to new ueId " MME_UE_S1AP_ID_FMT ". \n",
          new_emm_ue_ctx->_imsi64, old_ue_id, ue_id);
      emm_sap_t emm_sap = {0};
      emm_sap.primitive =
          EMMCN_IMPLICIT_DETACH_UE; /**< UE context will be purged. */
      emm_sap.u.emm_cn.u.emm_cn_implicit_detach.emm_cause =
          0; /**< Not sending detach type. */
      emm_sap.u.emm_cn.u.emm_cn_implicit_detach.detach_type =
          0; /**< Not sending detach type. */
      emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id =
          (*duplicate_emm_ue_ctx_pP)->ue_id;
      /*
       * Don't send the detach type, such that no NAS Detach Request is sent to
       * the UE. Depending on the cause, the MME_APP will check and inform the
       * NAS layer to continue with the procedure, before the timer expires.
       */
      emm_sap_send(&emm_sap);

      /** Set the new attach procedure into pending mode and continue with it
       * after the completion of the duplicate removal. */
      void *unused = NULL;
      _emm_proc_create_procedure_attach_request(new_emm_ue_ctx, ies,
                                                _emm_attach_retry_procedure);
      nas_emm_attach_proc_t *attach_proc =
          get_nas_specific_procedure_attach(new_emm_ue_ctx);
      nas_stop_T_retry_specific_procedure(
          new_emm_ue_ctx->ue_id, &attach_proc->emm_spec_proc.retry_timer,
          unused);
      nas_start_T_retry_specific_procedure(
          new_emm_ue_ctx->ue_id, &attach_proc->emm_spec_proc.retry_timer,
          attach_proc->emm_spec_proc.retry_cb, (void *)new_emm_ue_ctx->ue_id);
      /** Set the old mme_ue_s1ap id which will be checked. */
      attach_proc->emm_spec_proc.old_ue_id = old_ue_id;
      *duplicate_emm_ue_ctx_pP = NULL;
      /*
       * Perform implicit detach on the old UE with cause of duplicate
       * detection. Not continuing with the new attach procedure until the old
       * one has been successfully deallocated.
       */
      // todo: this probably will not work, we need to update the enb_ue_s1ap_id
      // of the old UE context to continue to work with it!
      //              unlock_ue_contexts(ue_context);
      //             unlock_ue_contexts(imsi_ue_mm_ctx);
      emm_init_context(new_emm_ue_ctx);
      DevAssert(RETURNok == emm_data_context_add(&_emm_data, new_emm_ue_ctx));
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
    } else {
      new_emm_ue_ctx = (*duplicate_emm_ue_ctx_pP);
      /** Unlink the new procedure. */
      (*duplicate_emm_ue_ctx_pP) = NULL;
    }
  }

  /** Continue with the new or existing EMM context. */
  DevAssert(new_emm_ue_ctx);
  DevAssert(new_emm_ue_ctx->emm_cause == EMM_CAUSE_SUCCESS);
  _emm_proc_create_procedure_attach_request(new_emm_ue_ctx, ies,
                                            _emm_attach_retry_procedure);
  if ((*duplicate_emm_ue_ctx_pP)) {
    /** Set the new attach procedure into pending mode and continue with it
     * after the completion of the duplicate removal. */
    void *unused = NULL;
    nas_emm_attach_proc_t *attach_proc =
        get_nas_specific_procedure_attach(new_emm_ue_ctx);
    nas_stop_T_retry_specific_procedure(
        new_emm_ue_ctx->ue_id, &attach_proc->emm_spec_proc.retry_timer, unused);
    nas_start_T_retry_specific_procedure(
        new_emm_ue_ctx->ue_id, &attach_proc->emm_spec_proc.retry_timer,
        attach_proc->emm_spec_proc.retry_cb, (void *)new_emm_ue_ctx->ue_id);
    /** Set the old mme_ue_s1ap id which will be checked. */
    attach_proc->emm_spec_proc.old_ue_id = (*duplicate_emm_ue_ctx_pP)->ue_id;
    /*
     * Nothing else to do with the current attach procedure. Leave it on hold.
     * Continue to handle the new attach procedure.
     */
    /*
     * Perform implicit detach on the old UE with cause of duplicate detection.
     * Not continuing with the new attach procedure until the old one has been
     * successfully deallocated.
     */
    // todo: this probably will not work, we need to update the enb_ue_s1ap_id
    // of the old UE context to continue to work with it!
    //              unlock_ue_contexts(ue_context);
    //             unlock_ue_contexts(imsi_ue_mm_ctx);

    //    /** Clean up new UE context that was created to handle new attach
    //    request. */ emm_sap_t                               emm_sap = {0};
    //    emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE; /**< UE context will be
    //    purged. */ emm_sap.u.emm_cn.u.emm_cn_implicit_detach.emm_cause   =
    //    (*duplicate_emm_ue_ctx_pP)->emm_cause; /**< Not sending detach type.
    //    */ emm_sap.u.emm_cn.u.emm_cn_implicit_detach.detach_type = 0; /**< Not
    //    sending detach type. */
    //    emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id =
    //    (*duplicate_emm_ue_ctx_pP)->ue_id;
    //    /*
    //     * Don't send the detach type, such that no NAS Detach Request is sent
    //     to the UE.
    //     * Depending on the cause, the MME_APP will check and inform the NAS
    //     layer to continue with the procedure, before the timer expires.
    //     */
    //    emm_sap_send (&emm_sap);
    //
    //    sleep(1);

    OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);

  } else {
    rc = _emm_attach_run_procedure(new_emm_ue_ctx);
    if (rc != RETURNok) {
      OAILOG_WARNING(LOG_NAS_EMM,
                     "Failed to initialize EMM callback functions");
      new_emm_ue_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      nas_emm_attach_proc_t *attach_proc =
          get_nas_specific_procedure_attach(new_emm_ue_ctx);
      attach_proc->emm_cause = new_emm_ue_ctx->emm_cause;
      attach_proc->esm_msg_out = NULL;
      rc = _emm_attach_reject((struct nas_base_proc_s *)attach_proc, NULL);
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNerror);
    }
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
  }
}

/*
 *
 * Name:        emm_proc_attach_reject()
 *
 * Description: Performs the protocol error abnormal case
 *
 *              3GPP TS 24.301, section 5.5.1.2.7, case b
 *              If the ATTACH REQUEST message is received with a protocol
 *              error, the network shall return an ATTACH REJECT message.
 *
 * Inputs:  ue_id:              UE lower layer identifier
 *                  emm_cause: EMM cause code to be reported
 *                  Others:    None
 *
 * Outputs:     None
 *                  Return:    RETURNok, RETURNerror
 *                  Others:    _emm_data
 *
 */
//------------------------------------------------------------------------------
int emm_proc_attach_reject(mme_ue_s1ap_id_t ue_id, emm_cause_t emm_cause) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  emm_data_context_t *emm_context = NULL;
  int rc = RETURNerror;

  emm_context = emm_data_context_get(&_emm_data, ue_id);
  if (emm_context) {
    if (is_nas_specific_procedure_attach_running(emm_context)) {
      nas_emm_attach_proc_t *attach_proc =
          (nas_emm_attach_proc_t *)
              emm_context->emm_procedures->emm_specific_proc;

      // todo: review attach rejects
      //      attach_proc->emm_cause = emm_cause;
      //      emm_sap_t                               emm_sap = {0};
      //      emm_sap.primitive = EMMREG_ATTACH_REJ;
      //      emm_sap.u.emm_reg.ue_id = ue_id;
      //      emm_sap.u.emm_reg.notify = false;
      //      emm_sap.u.emm_reg.free_proc = true;
      //      emm_sap.u.emm_reg.u.attach.proc = attach_proc;
      //      rc = emm_sap_send (&emm_sap);
      attach_proc->emm_cause = emm_cause;
      rc = _emm_attach_reject(&attach_proc->emm_spec_proc.emm_proc.base_proc,
                              NULL);
    } else {
      OAILOG_INFO(
          LOG_NAS_EMM,
          "EMM-PROC  - No attach procedure for (ue_id=" MME_UE_S1AP_ID_FMT
          ")\n",
          ue_id);
    }
  } else {
    OAILOG_INFO(LOG_NAS_EMM,
                "EMM-PROC  - No EMM Context for (ue_id=" MME_UE_S1AP_ID_FMT
                ")\n",
                ue_id);
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/*
 *
 * Name:    emm_proc_attach_complete()
 *
 * Description: Terminates the attach procedure upon receiving Attach
 *      Complete message from the UE.
 *
 *              3GPP TS 24.301, section 5.5.1.2.4
 *      Upon receiving an ATTACH COMPLETE message, the MME shall
 *      stop timer T3450, enter state EMM-REGISTERED and consider
 *      the GUTI sent in the ATTACH ACCEPT message as valid.
 *
 * Inputs:  ue_id:      UE lower layer identifier
 *      Others:    _emm_data
 *
 * Outputs:     None
 *      Return:    RETURNok, RETURNerror
 *      Others:    _emm_data, T3450
 *
 */
//------------------------------------------------------------------------------
int emm_proc_attach_complete(mme_ue_s1ap_id_t ue_id, int emm_cause,
                             const nas_message_decode_status_t status) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  emm_data_context_t *emm_context = NULL;
  nas_emm_attach_proc_t *attach_proc = NULL;
  int rc = RETURNerror;
  emm_sap_t emm_sap = {0};

  OAILOG_INFO(LOG_NAS_EMM,
              "EMM-PROC  - EPS attach complete (ue_id=" MME_UE_S1AP_ID_FMT
              ")\n",
              ue_id);

  /*
   * Get the UE context
   */
  emm_context = emm_data_context_get(&_emm_data, ue_id);

  if (emm_context) {
    if (is_nas_specific_procedure_attach_running(emm_context)) {
      attach_proc = (nas_emm_attach_proc_t *)
                        emm_context->emm_procedures->emm_specific_proc;

      /*
       * Upon receiving an ATTACH COMPLETE message, the MME shall enter state
       * EMM-REGISTERED and consider the GUTI sent in the ATTACH ACCEPT message
       * as valid.
       */
      REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__20);
      emm_ctx_set_attribute_valid(emm_context, EMM_CTXT_MEMBER_GUTI);
      /** Add the EMM context by GUTI. */
      emm_data_context_add_guti(&_emm_data, emm_context);
      // TODO LG REMOVE emm_context_add_guti(&_emm_data,
      // &ue_context->emm_context);
      emm_ctx_clear_old_guti(emm_context);
    } else {
      NOT_REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__20);
      OAILOG_INFO(LOG_NAS_EMM,
                  "UE " MME_UE_S1AP_ID_FMT
                  " ATTACH COMPLETE discarded (EMM procedure not found)\n",
                  ue_id);
    }
  } else {
    NOT_REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__20);
    OAILOG_ERROR(LOG_NAS_EMM,
                 "UE " MME_UE_S1AP_ID_FMT
                 " ATTACH COMPLETE discarded (context not found)\n",
                 ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
  }

  /*
   * Set the network attachment indicator
   */
  emm_context->is_has_been_attached = true;
  /*
   * Notify EMM that attach procedure has successfully completed
   */
  emm_sap.primitive =
      EMMREG_ATTACH_CNF; /**< No Common_REG_CNF since GUTI might not have been
                            sent (EMM_DEREG-> EMM_REG) // todo: attach_complete
                            arrives always? */
  // todo: for EMM_REG_COMMON_PROC_CNF, do we need common_proc (explicit) like
  // SMC, AUTH, INFO? and not implicit guti reallocation where none exists?
  emm_sap.u.emm_reg.ue_id = ue_id;
  //    emm_sap.u.emm_reg.notify = true;
  //    emm_sap.u.emm_reg.free_proc = true;
  emm_sap.u.emm_reg.u.attach.proc = attach_proc;
  rc = emm_sap_send(&emm_sap);

  /*
   * Check if the UE is in registered state.
   */
  if (emm_context->_emm_fsm_state != EMM_REGISTERED) {
    OAILOG_WARNING(LOG_NAS_EMM,
                   "EMM-PROC  - EMM Context for ueId " MME_UE_S1AP_ID_FMT
                   " is still not in EMM_REGISTERED state although ATTACH_CNF "
                   "has arrived. "
                   "Removing failed EMM context implicitly.. \n",
                   ue_id);
    emm_sap_t emm_sap = {0};
    emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;
    emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = ue_id;
    emm_sap_send(&emm_sap);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
  }

  /** ESM Failure has to be handled separately. */

  //  unlock_ue_contexts(ue_context);
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/
/**
 * Returns SUCCESS --> if the Attach-Request could be validated/not rejected/no
 * retransmission of a previous attach-accept. In that we continue with the
 * handling of the Attach_Request. Return  ERROR   --> ignore the request (might
 * be rejected or the previous attach accept might be retransmitted).
 *
 * No matter on what is returned, we might or might not also implicitly detach
 * the EMM context.
 */
static int emm_proc_attach_request_validity(
    emm_data_context_t *emm_context, mme_ue_s1ap_id_t new_ue_id,
    emm_attach_request_ies_t *const ies) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;
  mme_ue_s1ap_id_t old_ue_id = emm_context->ue_id;
  if (emm_context->ue_id != new_ue_id) {
    OAILOG_WARNING(LOG_NAS_EMM,
                   "EMM-PROC  - The UE_ID EMM context from IMSI " IMSI_64_FMT
                   " with old mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT
                   " is not equal to new ueId" MME_UE_S1AP_ID_FMT ". \n",
                   emm_context->_imsi64, emm_context->ue_id, new_ue_id);
    //       new_emm_ue_ctx->ue_id = ue_id; /**< The old UE_ID may still refer
    //       to another MME_APP context, with which we may like to continue. */
  }
  emm_fsm_state_t fsm_state = emm_fsm_get_state(emm_context);
  nas_emm_attach_proc_t *attach_procedure =
      get_nas_specific_procedure_attach(emm_context);
  nas_emm_tau_proc_t *tau_procedure =
      get_nas_specific_procedure_tau(emm_context);
  // todo: may these parameters change?
  //     new_emm_ue_ctx->attach_type = type;
  //     new_emm_ue_ctx->additional_update_type = additional_update_type;
  /*
   * Check if there is an ongoing SMC procedure.
   * In any case, we just abort the current SMC procedure, which will keep the
   * context, state, other commons and bearers, etc.. We continue with the
   * specific procedure. If the SMC was part of the specific procedure, we don't
   * care here. Thats the collision between the specific procedures. No matter
   * what the state is and if the received message differs or not, remove the
   * EMM_CTX (incl. security), bearers, all other common procedures and abort
   * any running specific procedures.
   */
  if (is_nas_common_procedure_smc_running(emm_context)) {
    REQUIREMENT_3GPP_24_301(R10_5_4_3_7_c);
    nas_emm_smc_proc_t *smc_proc = get_nas_common_procedure_smc(emm_context);
    emm_sap_t emm_sap = {0};
    emm_sap.primitive =
        EMMREG_COMMON_PROC_ABORT; /**< May cause an implicit detach, which may
                                     purge the EMM context. */
    emm_sap.u.emm_reg.ue_id = emm_context->ue_id;
    emm_sap.u.emm_reg.notify =
        false; /**< Because SMC itself did not fail, some other event (some
                  external event like a collision) occurred. Aborting the common
                  procedure. Not high severity of error. */
    emm_sap.u.emm_reg.free_proc =
        true; /**< We will not restart the procedure again. */
    emm_sap.u.emm_reg.u.common.common_proc = &smc_proc->emm_com_proc;
    emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
        smc_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
    MSC_LOG_TX_MESSAGE(MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0,
                       "0 EMMREG_PROC_ABORT (SMC) ue id " MME_UE_S1AP_ID_FMT
                       " ",
                       emm_context->mme_ue_s1ap_id);
    rc = emm_sap_send(&emm_sap);
    // Allocate new context and process the new request as fresh attach request
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
    /** We don't need to check the EMM context further. It is deallocated, but
     * continue with processing the Attach Request., return. */ // todo: check that the emm_ctx is nulled. (else, need to do it here).
  }

  /*
   * Check if there is an Authentication Procedure running.
   * No collisions are mentioned, but just abort the current authentication
   * procedure, if it was triggered by MME/OAM interface. Consider last
   * UE/EPS_Security context to be valid, keep the state and everything. If it
   * was part of a specific procedure, we will deal with it too, below, anyway.
   *
   * We don't need to check if an EMM context exists or not. It must exist if
   * SMC was not aborted.
   */
  if (is_nas_common_procedure_authentication_running(emm_context)) {
    if (attach_procedure || tau_procedure) {
      OAILOG_WARNING(LOG_NAS_EMM,
                     "EMM-PROC  - The EMM context from IMSI " IMSI_64_FMT
                     " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT
                     " has an an authentication procedure as part of a "
                     "specific procedure. "
                     "Common identification procedure will be aborted (as part "
                     "of the possibly aborted specific procedure below, if the "
                     "specific procedure is to be aborted. \n",
                     emm_context->_imsi64, emm_context->ue_id);
    } else {
      /** Not defined explicitly in the specifications, but we will abort the
       * common procedure. */
      OAILOG_WARNING(
          LOG_NAS_EMM,
          "EMM-PROC  - The EMM context from IMSI " IMSI_64_FMT
          " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT
          " has an authentication procedure running but no specific procedure. "
          "Aborting the authentication procedure run currently. \n",
          emm_context->_imsi64, emm_context->ue_id);
      nas_emm_auth_proc_t *auth_proc =
          get_nas_common_procedure_authentication(emm_context);
      emm_sap_t emm_sap = {0};
      emm_sap.primitive = EMMREG_COMMON_PROC_ABORT;
      emm_sap.u.emm_reg.ue_id =
          emm_context
              ->ue_id; /**< This should be enough to get the EMM context. */
      emm_sap.u.emm_reg.notify =
          false; /**< Again, the authentification procedure itself not
                    considered as failed but aborted from outside. */
      emm_sap.u.emm_reg.free_proc = true;
      emm_sap.u.emm_reg.u.common.common_proc = &auth_proc->emm_com_proc;
      emm_sap.u.emm_reg.u.common.previous_emm_fsm_state =
          auth_proc->emm_com_proc.emm_proc.previous_emm_fsm_state;
      // TODOdata->notify_failure = true;
      MSC_LOG_TX_MESSAGE(MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0,
                         "0 EMMREG_PROC_ABORT (AUTH) ue id " MME_UE_S1AP_ID_FMT
                         " ",
                         emm_context->mme_ue_s1ap_id);
      rc = emm_sap_send(&emm_sap);
      /** Keep all the states, context, bearers and other common and specific
       * procedures, just stopping the timer and deallocating the common
       * procedure. */
      DevAssert(rc == RETURNok);
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
    }
  }

  /**
   * Get the emm_context again.
   * Above abort procedures might have removed the emm_context, continue if one
   * exists.
   */
  emm_context = emm_data_context_get(&_emm_data, old_ue_id);
  if (!emm_context) {
    OAILOG_WARNING(LOG_NAS_EMM,
                   "EMM-PROC  - No EMM context for UE " MME_UE_S1AP_ID_FMT
                   " exists after first phase of verification. \n",
                   old_ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
  }

  /*
   * We don't need to check the identification procedure. If it is not part of
   * an attach procedure (no specific attach procedure is running - assuming all
   * common procedures belong to the specific procedure), we let it run. It may
   * independently trigger a timeout with consequences.
   *
   * If there is a specific attach procedure running, see below. All running
   * common procedures will assumed to be part of the specific procedure and
   * will be aborted together with the specific procedure.
   */
  if (is_nas_common_procedure_identification_running(emm_context)) {
    /*
     * We just check if a collision with the same specific procedure is
     * occurring for log reasons. If a collision with a running specific
     * procedure occurs below, all common procedures will be assumed to be part
     * of the specific procedure and aborted accordingly, anyway.
     */
    if (attach_procedure || tau_procedure) {
      REQUIREMENT_3GPP_24_301(
          R10_5_4_4_6_d__2); /**< Will be aborted eventually below. */
      REQUIREMENT_3GPP_24_301(R10_5_4_4_6_d__1);
      OAILOG_WARNING(LOG_NAS_EMM,
                     "EMM-PROC  - The EMM context from IMSI " IMSI_64_FMT
                     " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT
                     " has an an identification procedure as part of a "
                     "specific procedure. "
                     "Common identification procedure will be aborted (as part "
                     "of the possibly aborted specific procedure below, if the "
                     "specific procedure is to be aborted. \n",
                     emm_context->_imsi64, emm_context->ue_id);
    } else {
      REQUIREMENT_3GPP_24_301(R10_5_4_4_6_c);  // continue
      OAILOG_WARNING(
          LOG_NAS_EMM,
          "EMM-PROC  - The EMM context from IMSI " IMSI_64_FMT
          " with mme_ue_s1ap_id " MME_UE_S1AP_ID_FMT
          " has an identification procedure running but no specific procedure. "
          "Letting the procedure run currently. \n",
          emm_context->_imsi64, emm_context->ue_id);
      /**
       * Specification does not say, that we need to abort the identification
       * procedure. This may cause that a missing response detaches the UE after
       * this attach complete successfully! Unlike SMC, no abort.
       */
    }
  }
  /** Again, also in the new version, we can make a clean cut. *
   *
   * UE may or may not be in COMMON-PROCEDURE-INITIATED state (common procedure
   * active or implicit GUTI reallocation), DEREGISTERED or already REGISTERED.
   * Further check on collision with other specific procedures.
   * Checking if EMM_REGISTERED or already has been registered.
   * Already been registered only makes sense if  COMMON procedures are
   * activated.
   */
  if ((EMM_REGISTERED == fsm_state) || emm_context->is_has_been_attached) {
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_f);
    OAILOG_WARNING(LOG_NAS_EMM,
                   "EMM-PROC  - the new ATTACH REQUEST is progressed and the "
                   "current registered EMM context, "
                   "together with the states, common procedures and bearers "
                   "will be destroyed. \n");
    DevAssert(!get_nas_specific_procedure_attach(emm_context));
    /*
     * Perform an implicit detach on the current context.
     * This will check if there are any common procedures are running, and
     * destroy them, too. We don't need to check if a TAU procedure is running,
     * too. It will be destroyed in the implicit detach. It will destroy any EMM
     * context. EMM_CTX wil be nulled (not EMM_DEREGISTERED).
     */
    // Trigger clean up
    emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    OAILOG_FUNC_RETURN(
        LOG_NAS_EMM,
        RETURNok); /**< Return with the deallocated (NULL) EMM context and
                      continue to process the Attach Request. */
  }
  /*
   * Check collisions with specific procedures.
   * Specific attach and tau procedures run until attach/tau_complete!
   * No attach completed yet.
   */
  if (attach_procedure &&
      (is_nas_attach_accept_sent(
          attach_procedure))) {  // && (!emm_ctx->is_attach_complete_received):
                                 // implicit
    if (_emm_attach_ies_have_changed(emm_context->ue_id, attach_procedure->ies,
                                     ies)) {
      OAILOG_WARNING(LOG_NAS_EMM,
                     "EMM-PROC  - Attach parameters have changed\n");
      REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_d__1);
      /*
       * If one or more of the information elements in the ATTACH REQUEST
       * message differ from the ones received within the previous ATTACH
       * REQUEST message, the previously initiated attach procedure shall be
       * aborted if the ATTACH COMPLETE message has not been received and the
       * new attach procedure shall be progressed;
       */
      emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      // Allocate new context and process the new request as fresh attach
      // request
      OAILOG_FUNC_RETURN(
          LOG_NAS_EMM,
          RETURNok); /**< Return with the deallocated (NULL) EMM context and
                        continue to process the Attach Request. */
    } else {
      //            imsi_ue_mm_ctx->emm_context.num_attach_request++;
      REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_d__2);
      /*
       * If nothing has changed, we assume that no new
       * context/reference/description has to be removed.
       * - if the information elements do not differ, then the ATTACH ACCEPT
       * message shall be resent and the timer T3450 shall be restarted if an
       * ATTACH COMPLETE message is expected. In that case, the retransmission
       * counter related to T3450 is not incremented.
       */
      _emm_attach_accept_retx(emm_context);
      // Clean up new UE context that was created to handle new attach request
      if (new_ue_id != emm_context->ue_id)
        nas_itti_esm_detach_ind(new_ue_id, false);
      free_emm_attach_request_ies(&ies);
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNerror);
    }
  } else if ((attach_procedure) &&
             (!is_nas_attach_accept_sent(attach_procedure)) &&
             (!is_nas_attach_reject_sent(attach_procedure))) {
    if (_emm_attach_ies_have_changed(emm_context->ue_id, attach_procedure->ies,
                                     ies)) {
      OAILOG_WARNING(LOG_NAS_EMM,
                     "EMM-PROC  - Attach parameters have changed\n");
      REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_e__1);
      /*
       * If one or more of the information elements in the ATTACH REQUEST
       * message differs from the ones received within the previous ATTACH
       * REQUEST message, the previously initiated attach procedure shall be
       * aborted and the new attach procedure shall be executed;
       */
      emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      // Allocate new context and process the new request as fresh attach
      // request
      OAILOG_FUNC_RETURN(
          LOG_NAS_EMM,
          RETURNok); /**< Return with the deallocated (NULL) EMM context and
                        continue to process the Attach Request. */
    } else {
      REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_e__2);
      /*
       * If the information elements do not differ, then the network shall
       * continue with the previous attach procedure and shall ignore the second
       * ATTACH REQUEST message.
       *
       * ** This does not work, beacause the previous MME_APP UE context will
       * not be valid anymore. In this case, we had to transfer every
       * modification of the old MME_APP UE context into the new one. ***
       */
      OAILOG_WARNING(LOG_NAS_EMM,
                     "EMM-PROC  - Received duplicated Attach Request, dropping "
                     "the newly received Attach request but .\n");
      emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      // Allocate new context and process the new request as fresh attach
      // request
      OAILOG_FUNC_RETURN(
          LOG_NAS_EMM,
          RETURNok); /**< Return with the deallocated (NULL) EMM context and
                        continue to process the Attach Request. */
    }
  }
  /** Check for collisions with tracking area update. */
  else if (tau_procedure &&
           is_nas_tau_accept_sent(
               tau_procedure)) {  // && (!emm_ctx->is_tau_complete_received):
                                  // implicit
    /** Check for a retransmission first. */
    //      new_emm_ue_ctx->num_attach_request++;
    /** Implicitly detach the UE context, which will remove all common and
     * specific procedures. */
    emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
  } else if (/* && (0 < new_emm_ue_ctx->num_attach_request) */
             (tau_procedure)) {
    DevAssert(!is_nas_tau_accept_sent(tau_procedure));
    DevAssert(!is_nas_tau_reject_sent(tau_procedure));
    OAILOG_WARNING(
        LOG_NAS_EMM,
        "EMM-PROC  - Received Attach Request while TAU procedure is ongoing. "
        "Implicitly detaching the UE context and continuing with the "
        "attach request for the UE_ID " MME_UE_S1AP_ID_FMT ". \n",
        emm_context->ue_id);
    /** Implicitly detach the UE context, which will remove all common and
     * specific procedures. */
    emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
    //
    /** No collision with a specific procedure and UE is in EMM_DEREGISTERED
     * state (only state where we can continue with the UE context. */
  } else if (EMM_DEREGISTERED == fsm_state) {
    /** We assume that no bearers etc.. exist when no MME_APP context exists. */

    // todo: is this an undefined state?
    /*
     * Silently discard continue with the DEREGISTERED EMM context.
     * Must be sure, that no COMPLETE message or so is received.
     */
    OAILOG_INFO(LOG_NAS_EMM,
                "EMM-PROC  - Received Attach Request for a DEREGISTERED "
                "UE_ID " MME_UE_S1AP_ID_FMT
                ". "
                "Will continue with the existing EMM_CTX's and let current "
                "common procedures run. Silently discarding all specific "
                "procedures (XXX_ACCEPT sent, etc.. ). \n",
                emm_context->ue_id);
    // todo: to be sure, just silently discard all specific procedures, is this
    // necessary or should we assert?
    //       emm_context_silently_reset_procedures(new_emm_ue_ctx); /**< Discard
    //       all pending COMPLETEs, here (specific: XX_accept sent already). The
    //       complete message will then be discarded when specific XX_ACCEPT
    //       flag is off! */
    /*
     * Assuming nothing to reset when continuing with DEREGISTERED EMM context.
     * Continue using its EPS security context. Will ask for new subscription
     * data in the HSS with ULR (also to register the MME again).
     */
    ue_context_t *ue_context = mme_ue_context_exists_mme_ue_s1ap_id(
        &mme_app_desc.mme_ue_contexts, emm_context->ue_id);
    DevAssert(!ue_context);  // todo: error handling here?
    /** Check that the security context could be validated. */
    if (!(IS_EMM_CTXT_PRESENT_SECURITY(emm_context) &&
          ies->decode_status.integrity_protected_message &&
          ies->decode_status.mac_matched)) {
      OAILOG_WARNING(
          LOG_NAS_EMM,
          "EMM-PROC  - Received Attach Request while UE context does not have "
          "a valid security context. Performing an implicit detach. \n");
      /** Set the EMM cause to invalid. */
      emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      /** Continue to handle the TAU Request inside a new EMM context. */
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
    }
    /*
     * We don't need to check if any parameters have changed.
     * If we received an integrity protected message and if the MAC header could
     * be verified we will assume that the current security context is valid.
     * todo: checking for UE/MS network capabilities and replaying them if
     * changed? assuming not changed.
     */
    // todo: 5G has something like REGISTRATION UPDATE!
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
  } else {  // else  ((ue_context) && ((EMM_DEREGISTERED < fsm_state ) &&
            // (EMM_REGISTERED != fsm_state)))
    /*
     * This should also consider a Service Request Procedure.
     */
    OAILOG_INFO(
        LOG_NAS_EMM,
        "EMM-PROC  - Received Attach Request for a UE_ID " MME_UE_S1AP_ID_FMT
        " in an unhandled state. "
        "Implicitly detaching the UE context and continuing with the new "
        "attach request. \n",
        emm_context->ue_id);
    /** todo: we assume that no procedures are running. */
    /** Perform an implicit detach. */
    emm_context->emm_cause = EMM_CAUSE_ILLEGAL_UE;
    OAILOG_FUNC_RETURN(
        LOG_NAS_EMM,
        RETURNok); /**< Return with the deallocated (NULL) EMM context and
                      continue to process the Attach Request. */
  }
}

static void _emm_proc_create_procedure_attach_request(
    emm_data_context_t *const emm_context, emm_attach_request_ies_t *const ies,
    retry_cb_t retry_cb) {
  nas_emm_attach_proc_t *attach_proc = nas_new_attach_procedure(emm_context);
  AssertFatal(attach_proc, "TODO Handle this");
  if ((attach_proc)) {
    attach_proc->ies = ies;
    ((nas_emm_base_proc_t *)attach_proc)->abort = _emm_attach_abort;
    ((nas_emm_base_proc_t *)attach_proc)->fail_in =
        NULL;  // No parent procedure
    ((nas_emm_base_proc_t *)attach_proc)->time_out = _emm_attach_t3450_handler;
    /** No callback is needed for Attach complete. */
    ((nas_emm_base_proc_t *)attach_proc)->success_notif = NULL;
    attach_proc->emm_spec_proc.retry_cb = retry_cb;
  }
  OAILOG_DEBUG(LOG_NAS_EMM, " CREATED NEW ATTACH PROC %p. \n ", attach_proc);
}
/*
 * --------------------------------------------------------------------------
 * Timer handlers
 * --------------------------------------------------------------------------
 */

/*
 *
 * Name:    _emm_attach_t3450_handler()
 *
 * Description: T3450 timeout handler
 *
 *              3GPP TS 24.301, section 5.5.1.2.7, case c
 *      On the first expiry of the timer T3450, the network shall
 *      retransmit the ATTACH ACCEPT message and shall reset and
 *      restart timer T3450. This retransmission is repeated four
 *      times, i.e. on the fifth expiry of timer T3450, the at-
 *      tach procedure shall be aborted and the MME enters state
 *      EMM-DEREGISTERED.
 *
 * Inputs:  args:      handler parameters
 *      Others:    None
 *
 * Outputs:     None
 *      Return:    None
 *      Others:    None
 *
 */
static void _emm_attach_t3450_handler(void *args) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  mme_ue_s1ap_id_t ue_id = (mme_ue_s1ap_id_t)(args);

  /** Get the emm context. */
  emm_data_context_t *emm_context = emm_data_context_get(&_emm_data, ue_id);
  if (!emm_context) {
    OAILOG_ERROR(LOG_NAS_EMM,
                 "EMM-PROC  - T3450 timer expired, no EMM context for "
                 "UE " MME_UE_S1AP_ID_FMT ". Removing the attach procedure. \n",
                 ue_id);
    nas_itti_esm_detach_ind(ue_id, false);
    OAILOG_FUNC_OUT(LOG_NAS_EMM);
  }

  if (is_nas_specific_procedure_attach_running(emm_context)) {
    nas_emm_attach_proc_t *attach_proc =
        get_nas_specific_procedure_attach(emm_context);
    attach_proc->T3450.id = NAS_TIMER_INACTIVE_ID;
    attach_proc->attach_accept_sent++;
    OAILOG_WARNING(LOG_NAS_EMM,
                   "EMM-PROC  - T3450 timer expired, retransmission "
                   "counter = %d\n",
                   attach_proc->attach_accept_sent);
    if (attach_proc->attach_accept_sent < ATTACH_COUNTER_MAX) {
      REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_c__1);
      /*
       * On the first expiration of the timer, the network shall retransmit the
       * ATTACH ACCEPT message and shall reset and restart timer T3450.
       */
      _emm_attach_accept_retx(emm_context);
    } else {
      REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_c__2);
      /*
       * Abort the attach procedure
       */
      emm_sap_t emm_sap = {0};
      emm_sap.primitive = EMMREG_ATTACH_ABORT;
      emm_sap.u.emm_reg.ue_id = attach_proc->ue_id;
      //      emm_sap.u.emm_reg.notify= true;
      //      emm_sap.u.emm_reg.free_proc = true;
      emm_sap.u.emm_reg.u.attach.proc = attach_proc;
      emm_sap_send(&emm_sap);
    }
    // TODO REQUIREMENT_3GPP_24_301(R10_5_5_1_2_7_c__3) not coded
  }
  OAILOG_FUNC_OUT(LOG_NAS_EMM);
}

//------------------------------------------------------------------------------
static int _emm_attach_release(emm_data_context_t *emm_context) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;

  if (emm_context) {
    mme_ue_s1ap_id_t ue_id = emm_context->ue_id;
    OAILOG_WARNING(
        LOG_NAS_EMM,
        "EMM-PROC  - Release UE context data (ue_id=" MME_UE_S1AP_ID_FMT ")\n",
        ue_id);

    /** Not removing the downloaded subscription data. */

    /*
     * Release the EMM context
     */
    _clear_emm_ctxt(emm_context->ue_id);
  }

  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/*
 *
 * Name:    _emm_attach_reject()
 *
 * Description: Checks if an attach procedure is ongoing and proceeds with the
 * accept.
 *
 *              3GPP TS 24.301, section 5.5.1.2.5
 *      If the attach request cannot be accepted by the network,
 *      the MME shall send an ATTACH REJECT message to the UE in-
 *      including an appropriate EMM cause value.
 *
 * Inputs:  args:      UE context data
 *      Others:    None
 *
 * Outputs:     None
 *      Return:    RETURNok, RETURNerror
 *      Others:    None
 *
 */
int _emm_attach_reject(nas_emm_attach_proc_t *attach_proc, bstring rsp) {
  emm_sap_t emm_sap = {0};
  emm_data_context_t *emm_context = NULL;

  OAILOG_FUNC_IN(LOG_NAS_EMM);

  OAILOG_WARNING(LOG_NAS_EMM,
                 "EMM-PROC  - EMM attach procedure not accepted "
                 "by the network (ue_id=" MME_UE_S1AP_ID_FMT ", cause=%d)\n",
                 attach_proc->ue_id, attach_proc->emm_cause);

  /*
   * Notify EMM-AS SAP that Attach Reject message has to be sent
   * onto the network
   */
  emm_sap.primitive = EMMAS_ESTABLISH_REJ;
  emm_sap.u.emm_as.u.establish.ue_id = attach_proc->ue_id;
  emm_sap.u.emm_as.u.establish.eps_id.guti = NULL;
  emm_sap.u.emm_as.u.establish.emm_cause = attach_proc->emm_cause;
  emm_sap.u.emm_as.u.establish.nas_info = EMM_AS_NAS_INFO_ATTACH;
  if (rsp) {
    emm_sap.u.emm_as.u.establish.nas_msg = rsp;
    emm_sap.u.emm_as.u.establish.emm_cause = EMM_CAUSE_ESM_FAILURE;
  } else {
    OAILOG_WARNING(LOG_NAS_EMM,
                   "EMM-PROC  - ESM message is missing but attach reject "
                   "reason due ESM. Continuing with attach reject.\n");
  }
  /*
   * Setup EPS NAS security data
   */
  emm_context = emm_data_context_get(&_emm_data, attach_proc->ue_id);
  if (emm_context) {
    emm_as_set_security_data(&emm_sap.u.emm_as.u.establish.sctx,
                             &emm_context->_security, false, false);
  } else {
    emm_as_set_security_data(&emm_sap.u.emm_as.u.establish.sctx, NULL, false,
                             false);
  }
  int rc = emm_sap_send(&emm_sap);
  /** Remove the subscription. */
  subscription_data_t *subscription_data =
      mme_api_remove_subscription_data(emm_context->_imsi64);
  if (subscription_data) {
    free_wrapper((void **)&subscription_data);
  }

  // Release EMM context
  _clear_emm_ctxt(emm_context->ue_id);

  //  unlock_ue_contexts(ue_context);
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/*
 *
 * Name:    _emm_attach_abort()
 *
 * Description: Aborts the attach procedure
 *
 * Inputs:  args:      Attach procedure data to be released
 *      Others:    None
 *
 * Outputs:     None
 *      Return:    RETURNok, RETURNerror
 *      Others:    T3450
 *
 */
//------------------------------------------------------------------------------
static int _emm_attach_abort(struct emm_data_context_s *emm_context,
                             struct nas_emm_base_proc_s *emm_base_proc) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;

  nas_emm_attach_proc_t *attach_proc =
      get_nas_specific_procedure_attach(emm_context);
  if (attach_proc) {
    mme_ue_s1ap_id_t ue_id = emm_context->ue_id;

    OAILOG_WARNING(
        LOG_NAS_EMM,
        "EMM-PROC  - Abort the attach procedure (ue_id=" MME_UE_S1AP_ID_FMT
        ")\n",
        ue_id);

    // todo: stop the T3450 timer.

    /*
     * Notify ESM that the network locally refused PDN connectivity
     * to the UE
     */
    MSC_LOG_TX_MESSAGE(
        MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0,
        "0 EMMCN_IMPLICIT_DETACH_UE ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
    // Trigger clean up
    // todo: for example for        REQUIREMENT_3GPP_24_301(R10_5_4_4_6_f) we
    // may keep the UE as DEREGISTERED with a valid GUTI?! So @ Implicit D
    emm_sap_t emm_sap = {0};
    emm_sap.primitive = EMMCN_IMPLICIT_DETACH_UE;
    emm_sap.u.emm_cn.u.emm_cn_implicit_detach.ue_id = ue_id;
    emm_sap.u.emm_cn.u.emm_cn_implicit_detach.detach_type =
        EMM_DETACH_TYPE_NOT_REATTACH;
    rc = emm_sap_send(&emm_sap);
  }

  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/*
 * --------------------------------------------------------------------------
 * Functions that may initiate EMM common procedures
 * --------------------------------------------------------------------------
 */

//------------------------------------------------------------------------------
static int _emm_attach_run_procedure(emm_data_context_t *emm_context) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;
  nas_emm_attach_proc_t *attach_proc =
      get_nas_specific_procedure_attach(emm_context);

  if (attach_proc) {
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_3__1);

    if (attach_proc->ies->last_visited_registered_tai) {
      emm_ctx_set_valid_lvr_tai(emm_context,
                                attach_proc->ies->last_visited_registered_tai);
    } else {
      emm_ctx_clear_lvr_tai(emm_context);
    }

    if (IS_EMM_CTXT_PRESENT_SECURITY(emm_context)) {
      /*
       * No AUTH or SMC is needed, directly perform ULR/Subscription (and always
       * perform it due to MeshFlow reasons). Assume that the UE/MS network
       * capabilities are not changed and don't need to be replayed.
       */
      nas_itti_esm_data_ind(emm_context->ue_id,
                            attach_proc->ies->esm_msg_attach_proc,
                            attach_proc->ies->imsi,
                            attach_proc->ies->last_visited_registered_tai);
      attach_proc->ies->esm_msg_attach_proc = NULL;
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
    }
    /** Set LVR TAI again. */
    if (attach_proc->ies->last_visited_registered_tai) {
      emm_ctx_set_valid_lvr_tai(emm_context,
                                attach_proc->ies->last_visited_registered_tai);
    }
    // todo:present, valid?
    emm_context->originating_tai = *attach_proc->ies->originating_tai;

    /** Setting the UE & MS network capabilities as present and validating them
     * after replaying / successfully SMC. */
    emm_ctx_set_ue_nw_cap(emm_context, attach_proc->ies->ue_network_capability);
    if (attach_proc->ies->ms_network_capability) {
      emm_ctx_set_ms_nw_cap(emm_context,
                            attach_proc->ies->ms_network_capability);
    }
    /** Continue to attach with the current EPS security context. Check if IMSI
     * is received. */
    if (attach_proc->ies->imsi) {
      OAILOG_WARNING(LOG_NAS_EMM,
                     "EMM-PROC  - Received an IMSI in the attach request IE "
                     "for ue_id=" MME_UE_S1AP_ID_FMT
                     ". Continuing with authentication procedure. \n",
                     emm_context->ue_id);
      /** Register the received IMSI and continue with the security context. */
      imsi64_t imsi64 = imsi_to_imsi64(attach_proc->ies->imsi);
      emm_ctx_set_valid_imsi(emm_context, attach_proc->ies->imsi, imsi64);
      emm_data_context_upsert_imsi(&_emm_data, emm_context);
      OAILOG_INFO(LOG_NAS_EMM,
                  "EMM-PROC  - EMM context for the ue_id=" MME_UE_S1AP_ID_FMT
                  " missing valid and active EPS security context. \n",
                  emm_context->ue_id);
      rc = _emm_start_attach_proc_authentication(emm_context, attach_proc);
    } else if (attach_proc->ies->guti) {
      /** Check if a valid imsi exists. */
      OAILOG_INFO(LOG_NAS_EMM,
                  "EMM-PROC  - Received an GUTI " GUTI_FMT
                  " in the attach request IE for ue_id=" MME_UE_S1AP_ID_FMT
                  ". "
                  "Continuing with identification procedure. \n",
                  GUTI_ARG(attach_proc->ies->guti), emm_context->ue_id);
      rc = emm_proc_identification(emm_context, (nas_emm_proc_t *)attach_proc,
                                   IDENTITY_TYPE_2_IMSI,
                                   _emm_attach_success_identification_cb,
                                   _emm_attach_failure_identification_cb);
    } else if (attach_proc->ies->imei) {
      // emergency allowed if go here, but have to be implemented...
      AssertFatal(0, "TODO emergency");
    }
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_attach_retry_procedure(mme_ue_s1ap_id_t ue_id) {
  /** Validate that no old EMM context exists. */
  OAILOG_FUNC_IN(LOG_NAS_EMM);

  OAILOG_DEBUG(
      LOG_NAS_EMM,
      "EMM-PROC  - ENTERED Attach retry: UE " MME_UE_S1AP_ID_FMT ". \n", ue_id);

  int rc = RETURNerror;
  emm_data_context_t *emm_context = emm_data_context_get(&_emm_data, ue_id);
  if (!emm_context) {
    OAILOG_ERROR(
        LOG_NAS_EMM,
        "EMM-PROC  - Attach retry: no EMM context for UE " MME_UE_S1AP_ID_FMT
        ". Triggering implicit detach. \n",
        ue_id);
    nas_itti_esm_detach_ind(ue_id, false);
    OAILOG_FUNC_OUT(LOG_NAS_EMM);
  }

  nas_emm_attach_proc_t *attach_proc =
      (nas_emm_attach_proc_t *)get_nas_specific_procedure(emm_context);
  emm_data_context_t *duplicate_emm_context =
      emm_data_context_get(&_emm_data, attach_proc->emm_spec_proc.old_ue_id);
  ue_context_t *duplicate_ue_context = mme_ue_context_exists_mme_ue_s1ap_id(
      &mme_app_desc.mme_ue_contexts, attach_proc->emm_spec_proc.old_ue_id);

  /** Get the attach procedure. */
  //  DevAssert(attach_proc);

  //  if(attach_proc){
  //	  /** Stop the timer. */
  //	   void * unused = NULL;
  //	   nas_stop_T_retry_specific_procedure(attach_proc->ue_id,
  //&attach_proc->emm_spec_proc.retry_timer, unused);
  //  }

  if (duplicate_emm_context) {
    /** Send an attach reject back. */
    OAILOG_WARNING(
        LOG_NAS_EMM,
        "EMM-PROC  - An old EMM context for ue_id=" MME_UE_S1AP_ID_FMT
        " still existing. Aborting the attach procedure. \n",
        attach_proc->emm_spec_proc.old_ue_id);
    rc = _emm_attach_reject(&attach_proc->emm_spec_proc.emm_proc.base_proc,
                            NULL);
    //
    //    emm_sap_t emm_sap                      = {0};
    //    emm_sap.primitive                      = EMMREG_ATTACH_REJ;
    //    emm_sap.u.emm_reg.ue_id                = attach_proc->ue_id;
    //    emm_sap.u.emm_reg.notify               = true;
    //    emm_sap.u.emm_reg.free_proc            = true;
    //    emm_sap.u.emm_reg.u.attach.proc        = attach_proc;
    //    // don't care emm_sap.u.emm_reg.u.attach.is_emergency = false;
    //    rc = emm_sap_send (&emm_sap);
    //
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
  }
  /* Check that no old MME_APP UE context exists. */
  if (duplicate_ue_context) {
    OAILOG_WARNING(LOG_NAS_EMM,
                   "EMM-PROC  - An old UE context for ue_id=" MME_UE_S1AP_ID_FMT
                   " still existing. Aborting the attach procedure. \n",
                   attach_proc->emm_spec_proc.old_ue_id);
    /** Send an attach reject back. */
    rc = _emm_attach_reject(&attach_proc->emm_spec_proc.emm_proc.base_proc,
                            NULL);

    //    emm_sap_t emm_sap                      = {0};
    //    emm_sap.primitive                      = EMMREG_ATTACH_REJ;
    //    emm_sap.u.emm_reg.ue_id                = attach_proc->ue_id;
    //    emm_sap.u.emm_reg.notify               = true;
    //    emm_sap.u.emm_reg.free_proc            = true;
    //    emm_sap.u.emm_reg.u.attach.proc        = attach_proc;
    //    // don't care emm_sap.u.emm_reg.u.attach.is_emergency = false;
    //    rc = emm_sap_send (&emm_sap);
    /** Stop continuing with the procedure. The MME_APP context is assumed to be
     * removed, too. */
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
  }
  OAILOG_WARNING(
      LOG_NAS_EMM,
      "EMM-PROC  - No old EMM/UE context exists for ue_id=" MME_UE_S1AP_ID_FMT
      ". Continuing with attach procedure for new ueId " MME_UE_S1AP_ID_FMT
      ". \n",
      attach_proc->emm_spec_proc.old_ue_id, emm_context->ue_id);

  rc = _emm_attach_run_procedure(emm_context);
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_attach_success_identification_cb(
    emm_data_context_t *emm_context) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;

  nas_emm_attach_proc_t *attach_proc =
      get_nas_specific_procedure_attach(emm_context);

  if (attach_proc) {
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_3__1);
    rc = _emm_start_attach_proc_authentication(
        emm_context,
        attach_proc);  //, IDENTITY_TYPE_2_IMSI, _emm_attach_authentified,
                       //_emm_attach_release);
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_attach_failure_identification_cb(
    emm_data_context_t *emm_context) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;
  AssertFatal(0, "Cannot happen...\n");
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_start_attach_proc_authentication(
    emm_data_context_t *emm_context, nas_emm_attach_proc_t *attach_proc) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;

  if ((emm_context) && (attach_proc)) {
    rc = emm_proc_authentication(emm_context, &attach_proc->emm_spec_proc,
                                 _emm_attach_success_authentication_cb,
                                 _emm_attach_failure_authentication_cb);
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_attach_success_authentication_cb(
    emm_data_context_t *emm_context) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;

  /*
   * Only continue with SMC if a specific procedure is running.
   * todo: why don't we check parent procedure, only? In this way, it is
   * evaluated as it is part of the specific procedure, even if no parent
   * procedure exists (common initiated after specific, and then some specific
   * occurs).
   */
  nas_emm_attach_proc_t *attach_proc =
      get_nas_specific_procedure_attach(emm_context);

  if (attach_proc) {
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_3__1);
    rc = _emm_start_attach_proc_security(emm_context, attach_proc);
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_attach_failure_authentication_cb(
    emm_data_context_t *emm_context) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;
  nas_emm_attach_proc_t *attach_proc =
      get_nas_specific_procedure_attach(emm_context);

  if (attach_proc) {
    attach_proc->emm_cause = EMM_CAUSE_IMSI_UNKNOWN_IN_HSS;
    // TODO could be in callback of attach procedure triggered by
    // EMMREG_ATTACH_REJ
    rc = _emm_attach_reject(&attach_proc->emm_spec_proc.emm_proc.base_proc,
                            NULL);

    //    emm_sap_t emm_sap                      = {0};
    //    emm_sap.primitive                      = EMMREG_ATTACH_REJ;
    //    emm_sap.u.emm_reg.ue_id                = attach_proc->ue_id;
    //    emm_sap.u.emm_reg.notify               = true;
    //    emm_sap.u.emm_reg.free_proc            = true;
    //    emm_sap.u.emm_reg.u.attach.proc = attach_proc;
    //    // don't care emm_sap.u.emm_reg.u.attach.is_emergency = false;
    //    rc = emm_sap_send (&emm_sap);
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_start_attach_proc_security(emm_data_context_t *emm_context,
                                           nas_emm_attach_proc_t *attach_proc) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;

  if ((emm_context) && (attach_proc)) {
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_3__1);

    /*
     * Create new NAS security context
     */
    ksi_t old_ksi = emm_context->_security.eksi;
    emm_ctx_clear_security(emm_context);
    emm_ctx_set_security_eksi(emm_context, old_ksi);

    rc = emm_proc_security_mode_control(
        emm_context, &attach_proc->emm_spec_proc, emm_context->_security.eksi,
        _emm_attach_success_security_cb, _emm_attach_failure_security_cb);
    if (rc != RETURNok) {
      /*
       * Failed to initiate the security mode control procedure
       */
      OAILOG_WARNING(
          LOG_NAS_EMM,
          "ue_id=" MME_UE_S1AP_ID_FMT
          "EMM-PROC  - Failed to initiate security mode control procedure\n",
          emm_context->ue_id);
      attach_proc->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      /*
       * Do not accept the UE to attach to the network
       */
      emm_sap_t emm_sap = {0};
      emm_sap.primitive = EMMREG_ATTACH_REJ;
      emm_sap.u.emm_reg.ue_id = emm_context->ue_id;
      emm_sap.u.emm_reg.notify = true;
      emm_sap.u.emm_reg.free_proc = true;
      emm_sap.u.emm_reg.u.attach.proc = attach_proc;
      // dont care emm_sap.u.emm_reg.u.attach.is_emergency = false;
      rc = emm_sap_send(&emm_sap);
    }
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//------------------------------------------------------------------------------
static int _emm_attach_success_security_cb(
    struct emm_data_context_s *emm_context) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;

  nas_emm_attach_proc_t *attach_proc =
      get_nas_specific_procedure_attach(emm_context);

  if (attach_proc) {
    nas_itti_esm_data_ind(
        emm_context->ue_id, attach_proc->ies->esm_msg_attach_proc,
        &emm_context->_imsi, attach_proc->ies->originating_tai);
    attach_proc->ies->esm_msg_attach_proc = NULL;
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
}

//------------------------------------------------------------------------------
static int _emm_attach_failure_security_cb(emm_data_context_t *emm_context) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;
  nas_emm_attach_proc_t *attach_proc =
      get_nas_specific_procedure_attach(emm_context);

  if (attach_proc) {
    _emm_attach_release(emm_context);
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

//
//  rc = _emm_start_attach_proc_authentication (emm_context, attach_proc);//,
//  IDENTITY_TYPE_2_IMSI, _emm_attach_authentified, _emm_attach_release);
//
//  if ((emm_context) && (attach_proc)) {
//    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_3__1);
//    mme_ue_s1ap_id_t                        ue_id = emm_context->ue_id;
//    OAILOG_INFO (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Setup
//    NAS security\n", ue_id);
//
//    attach_proc->emm_spec_proc.emm_proc.base_proc.success_notif =
//    _emm_attach_success_authentication_cb;
//    attach_proc->emm_spec_proc.emm_proc.base_proc.failure_notif =
//    _emm_attach_failure_authentication_cb;
//    /*
//     * Create new NAS security context
//     */
//    emm_ctx_clear_security(emm_context);
//
//    /*
//     * Initialize the security mode control procedure
//     */
//    rc = emm_proc_security_mode_control (ue_id, emm_context->auth_ksi,
//                                         _emm_attach, _emm_attach_release);
//
//    if (rc != RETURNok) {
//      /*
//       * Failed to initiate the security mode control procedure
//       */
//      OAILOG_WARNING (LOG_NAS_EMM, "ue_id=" MME_UE_S1AP_ID_FMT "EMM-PROC  -
//      Failed to initiate security mode control procedure\n", ue_id);
//      attach_proc->emm_cause = EMM_CAUSE_ILLEGAL_UE;
//      /*
//       * Do not accept the UE to attach to the network
//       */
//      emm_sap_t emm_sap                      = {0};
//      emm_sap.primitive                      = EMMREG_ATTACH_REJ;
//      emm_sap.u.emm_reg.ue_id                = ue_id;
//      emm_sap.u.emm_reg.notify               = true;
//      emm_sap.u.emm_reg.free_proc            = true;
//      emm_sap.u.emm_reg.u.attach.attach_proc = attach_proc;
//      // dont care emm_sap.u.emm_reg.u.attach.is_emergency = false;
//      rc = emm_sap_send (&emm_sap);
//    }
//  }
//  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
//}
/*
 *
 * Name:        _emm_attach_security()
 *
 * Description: Initiates security mode control EMM common procedure.
 *
 * Inputs:          args:      security argument parameters
 *                  Others:    None
 *
 * Outputs:     None
 *                  Return:    RETURNok, RETURNerror
 *                  Others:    _emm_data
 *
 */
//------------------------------------------------------------------------------
int emm_attach_security(struct emm_data_context_s *emm_context) {
  return _emm_attach_security(emm_context);
}

//------------------------------------------------------------------------------
static int _emm_attach_security(emm_data_context_t *emm_context) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;

  nas_emm_attach_proc_t *attach_proc =
      get_nas_specific_procedure_attach(emm_context);

  if (attach_proc) {
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_3__1);
    mme_ue_s1ap_id_t ue_id = emm_context->ue_id;
    OAILOG_INFO(LOG_NAS_EMM,
                "ue_id=" MME_UE_S1AP_ID_FMT " EMM-PROC  - Setup NAS security\n",
                emm_context->ue_id);

    /*
     * Create new NAS security context
     */
    emm_ctx_clear_security(emm_context);
    /*
     * Initialize the security mode control procedure
     */
    rc = emm_proc_security_mode_control(
        emm_context, &attach_proc->emm_spec_proc, attach_proc->ksi,
        _emm_attach_success_security_cb, _emm_attach_release);

    if (rc != RETURNok) {
      /*
       * Failed to initiate the security mode control procedure
       */
      OAILOG_WARNING(
          LOG_NAS_EMM,
          "ue_id=" MME_UE_S1AP_ID_FMT
          "EMM-PROC  - Failed to initiate security mode control procedure\n",
          emm_context->ue_id);
      attach_proc->emm_cause = EMM_CAUSE_ILLEGAL_UE;
      /*
       * Do not accept the UE to attach to the network
       */
      emm_sap_t emm_sap = {0};
      emm_sap.primitive = EMMREG_ATTACH_REJ;
      emm_sap.u.emm_reg.ue_id = emm_context->ue_id;
      emm_sap.u.emm_reg.notify = true;
      emm_sap.u.emm_reg.free_proc = true;
      emm_sap.u.emm_reg.u.attach.proc = attach_proc;
      // dont care emm_sap.u.emm_reg.u.attach.is_emergency = false;
      rc = emm_sap_send(&emm_sap);
    }
  }

  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/*
   --------------------------------------------------------------------------
                MME specific local functions
   --------------------------------------------------------------------------
*/

//------------------------------------------------------------------------------
int _emm_wrapper_attach_accept(mme_ue_s1ap_id_t ue_id, bstring esm_msg) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  /** Check for the validity of the EMM context. No need to inform the ESM layer
   * immediately. */
  emm_data_context_t *emm_context = emm_data_context_get(&_emm_data, ue_id);
  if (!emm_context) {
    OAILOG_INFO(LOG_NAS_EMM,
                "ue_id=" MME_UE_S1AP_ID_FMT
                " EMM-PROC  - No EMM context for ue_id " MME_UE_S1AP_ID_FMT
                " exists. \n",
                ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_ESM, RETURNerror);
  }
  nas_emm_attach_proc_t *attach_proc =
      get_nas_specific_procedure_attach(emm_context);
  if (!attach_proc) {
    OAILOG_INFO(LOG_NAS_EMM,
                "ue_id=" MME_UE_S1AP_ID_FMT
                " EMM-PROC  - No attach procedure for ue_id " MME_UE_S1AP_ID_FMT
                " exists. \n",
                ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_ESM, RETURNerror);
  }
  // todo: lock_emm_context
  if (attach_proc->esm_msg_out) bdestroy_wrapper(&attach_proc->esm_msg_out);
  attach_proc->esm_msg_out = bstrcpy(esm_msg);

  DevAssert(!attach_proc->ies->esm_msg_attach_proc);

  int rc = _emm_send_attach_accept(emm_context);
  if (rc != RETURNerror) {
    if (IS_EMM_CTXT_PRESENT_OLD_GUTI(emm_context) &&
        (memcmp(&emm_context->_old_guti, &emm_context->_guti,
                sizeof(emm_context->_guti)))) {
      /*
       * Implicit GUTI reallocation;
       * Notify EMM that common procedure has been initiated
       * LG: TODO check this, seems very suspicious
       */
      emm_sap_t emm_sap = {0};

      emm_sap.primitive = EMMREG_COMMON_PROC_REQ;
      emm_sap.u.emm_reg.ue_id = ue_id;
      MSC_LOG_TX_MESSAGE(
          MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0,
          "0 EMMREG_COMMON_PROC_REQ ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
      rc = emm_sap_send(&emm_sap);
    }
  }
  // todo: unlock_emm_context

  OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
}

//------------------------------------------------------------------------------
int _emm_wrapper_attach_reject(mme_ue_s1ap_id_t ue_id, bstring esm_msg) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  /** Check for the validity of the EMM context. No need to inform the ESM layer
   * immediately. */
  emm_data_context_t *emm_context = emm_data_context_get(&_emm_data, ue_id);
  if (!emm_context) {
    OAILOG_INFO(LOG_NAS_EMM,
                "ue_id=" MME_UE_S1AP_ID_FMT
                " EMM-PROC  - No EMM context for ue_id " MME_UE_S1AP_ID_FMT
                " exists. \n",
                ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_ESM, RETURNerror);
  }
  nas_emm_attach_proc_t *attach_proc =
      get_nas_specific_procedure_attach(emm_context);
  if (!attach_proc) {
    OAILOG_INFO(LOG_NAS_EMM,
                "ue_id=" MME_UE_S1AP_ID_FMT
                " EMM-PROC  - No attach procedure for ue_id " MME_UE_S1AP_ID_FMT
                " exists. \n",
                ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_ESM, RETURNerror);
  }

  int rc = _emm_attach_reject(attach_proc, esm_msg);
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/*
 *
 * Name:    _emm_send_attach_accept()
 *
 * Description: Sends ATTACH ACCEPT message and start timer T3450
 *
 * Inputs:  data:      Attach accept retransmission data
 *      Others:    None
 *
 * Outputs:     None
 *      Return:    RETURNok, RETURNerror
 *      Others:    T3450
 *
 */

//        attach_proc->emm_cause = EMM_CAUSE_ESM_FAILURE;
//        bdestroy_wrapper (&attach_proc->ies->esm_msg);
//        attach_proc->esm_msg_out = esm_sap.send;
//        rc = _emm_attach_reject
//        (&attach_proc->emm_spec_proc.emm_proc.base_proc); OAILOG_FUNC_RETURN
//        (LOG_NAS_EMM, rc);

//------------------------------------------------------------------------------
static int _emm_send_attach_accept(emm_data_context_t *const emm_context) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  int rc = RETURNerror;

  // may be caused by timer not stopped when deleted context
  if (emm_context) {
    emm_sap_t emm_sap = {0};
    nas_emm_attach_proc_t *attach_proc =
        get_nas_specific_procedure_attach(emm_context);
    ue_context_t *ue_context = mme_ue_context_exists_mme_ue_s1ap_id(
        &mme_app_desc.mme_ue_contexts, emm_context->ue_id);
    mme_ue_s1ap_id_t ue_id = emm_context->ue_id;

    if (!attach_proc) {
      OAILOG_ERROR(LOG_NAS_EMM,
                   "ue_id=" MME_UE_S1AP_ID_FMT
                   " EMM-PROC  - No Attach procedure exists for sending Attach "
                   "Accept. \n",
                   ue_id);
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
    }

    _emm_attach_update(emm_context, attach_proc->ies);
    /*
     * Notify EMM-AS SAP that Attach Accept message together with an Activate
     * Default EPS Bearer Context Request message has to be sent to the UE
     */
    emm_sap.primitive = EMMAS_ESTABLISH_CNF;
    emm_sap.u.emm_as.u.establish.puid =
        attach_proc->emm_spec_proc.emm_proc.base_proc.nas_puid;
    emm_sap.u.emm_as.u.establish.ue_id = ue_id;
    emm_sap.u.emm_as.u.establish.nas_info = EMM_AS_NAS_INFO_ATTACH;

    NO_REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__3);
    if (ue_context->privates.fields.ue_radio_capability) {
      bdestroy_wrapper(&ue_context->privates.fields.ue_radio_capability);
    }
    //----------------------------------------
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__9);
    // the set of emm_sap.u.emm_as.u.establish.new_guti is for including the
    // GUTI in the attach accept message
    // ONLY ONE MME NOW NO S10
    if (!IS_EMM_CTXT_PRESENT_GUTI(emm_context)) {
      // Sure it is an unknown GUTI in this MME
      guti_t old_guti = emm_context->_old_guti;
      guti_t guti = {.gummei.plmn = {0},
                     .gummei.mme_gid = 0,
                     .gummei.mme_code = 0,
                     .m_tmsi = INVALID_M_TMSI};
      clear_guti(&guti);

      rc = mme_api_new_guti(&emm_context->_imsi, &old_guti, &guti,
                            &emm_context->originating_tai,
                            &emm_context->_tai_list);
      if (RETURNok == rc) {
        emm_ctx_set_guti(emm_context, &guti);
        emm_ctx_set_attribute_valid(emm_context, EMM_CTXT_MEMBER_TAI_LIST);
        //----------------------------------------
        REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__6);
        REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__10);
        memcpy(&emm_sap.u.emm_as.u.establish.tai_list, &emm_context->_tai_list,
               sizeof(tai_list_t));
      } else {
        OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNerror);
      }
    } else {
      // Set the TAI attributes from the stored context for resends.
      memcpy(&emm_sap.u.emm_as.u.establish.tai_list, &emm_context->_tai_list,
             sizeof(tai_list_t));
    }

    emm_sap.u.emm_as.u.establish.eps_id.guti = &emm_context->_guti;

    if (!IS_EMM_CTXT_VALID_GUTI(emm_context) &&
        IS_EMM_CTXT_PRESENT_GUTI(emm_context) &&
        IS_EMM_CTXT_PRESENT_OLD_GUTI(emm_context)) {
      /*
       * Implicit GUTI reallocation;
       * include the new assigned GUTI in the Attach Accept message
       */
      OAILOG_INFO(LOG_NAS_EMM,
                  "ue_id=" MME_UE_S1AP_ID_FMT
                  " EMM-PROC  - Implicit GUTI reallocation, include the new "
                  "assigned GUTI in the Attach Accept message\n",
                  ue_id);
      emm_sap.u.emm_as.u.establish.new_guti = &emm_context->_guti;
    } else if (!IS_EMM_CTXT_VALID_GUTI(emm_context) &&
               IS_EMM_CTXT_PRESENT_GUTI(emm_context)) {
      /*
       * include the new assigned GUTI in the Attach Accept message
       */
      OAILOG_INFO(LOG_NAS_EMM,
                  "ue_id=" MME_UE_S1AP_ID_FMT
                  " EMM-PROC  - Include the new assigned GUTI in the Attach "
                  "Accept message\n",
                  ue_id);
      emm_sap.u.emm_as.u.establish.new_guti = &emm_context->_guti;
    } else {  // IS_EMM_CTXT_VALID_GUTI(ue_context) is true
      emm_sap.u.emm_as.u.establish.new_guti = NULL;
    }
    //----------------------------------------
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__14);
    emm_sap.u.emm_as.u.establish.eps_network_feature_support =
        &_emm_data.conf.eps_network_feature_support;

    /*
     * Delete any preexisting UE radio capabilities, pursuant to
     * GPP 24.310:5.5.1.2.4
     */
    // Note: this is safe from double-free errors because it sets to NULL
    // after freeing, which free treats as a no-op.
    bdestroy_wrapper(&ue_context->privates.fields.ue_radio_capability);

    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data(&emm_sap.u.emm_as.u.establish.sctx,
                             &emm_context->_security, false, true);
    emm_sap.u.emm_as.u.establish.encryption =
        emm_context->_security.selected_algorithms.encryption;
    emm_sap.u.emm_as.u.establish.integrity =
        emm_context->_security.selected_algorithms.integrity;
    OAILOG_DEBUG(LOG_NAS_EMM,
                 "ue_id=" MME_UE_S1AP_ID_FMT
                 " EMM-PROC  - encryption = 0x%X (0x%X)\n",
                 ue_id, emm_sap.u.emm_as.u.establish.encryption,
                 emm_context->_security.selected_algorithms.encryption);
    OAILOG_DEBUG(LOG_NAS_EMM,
                 "ue_id=" MME_UE_S1AP_ID_FMT
                 " EMM-PROC  - integrity  = 0x%X (0x%X)\n",
                 ue_id, emm_sap.u.emm_as.u.establish.integrity,
                 emm_context->_security.selected_algorithms.integrity);
    /*
     * Get the activate default EPS bearer context request message to
     * transfer within the ESM container of the attach accept message
     */
    emm_sap.u.emm_as.u.establish.nas_msg = attach_proc->esm_msg_out;
    OAILOG_TRACE(
        LOG_NAS_EMM,
        "ue_id=" MME_UE_S1AP_ID_FMT
        " EMM-PROC  - nas_msg  src size = %d nas_msg  dst size = %d \n",
        ue_id, blength(attach_proc->esm_msg_out),
        blength(emm_sap.u.emm_as.u.establish.nas_msg));

    // Send T3402
    emm_sap.u.emm_as.u.establish.t3402 = &mme_config.nas_config.t3402_min;

    /** Increment the number of attach_accept's sent. */
    attach_proc->attach_accept_sent++;

    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__2);
    MSC_LOG_TX_MESSAGE(MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0,
                       "0 EMMAS_ESTABLISH_CNF ue id " MME_UE_S1AP_ID_FMT " ",
                       ue_id);
    rc = emm_sap_send(&emm_sap);

    /**
     * todo: remove this after locking.
     */

    if (RETURNerror != rc) {
      emm_data_context_t *emm_context1 =
          emm_data_context_get(&_emm_data, ue_id);
      attach_proc = NULL;
      if (emm_context1) {
        attach_proc = get_nas_specific_procedure_attach(emm_context1);
      }
      if (attach_proc) {
        /*
         * Start T3450 timer
         */
        void *unused = NULL;
        nas_stop_T3450(ue_id, &attach_proc->T3450, unused);
        nas_start_T3450(ue_id, &attach_proc->T3450,
                        attach_proc->emm_spec_proc.emm_proc.base_proc.time_out,
                        (void *)attach_proc->ue_id);
        /**
         * Implicit GUTI reallocation;
         * * * * Notify EMM that common procedure has been initiated
         */
        memset(&emm_sap, 0, sizeof(emm_sap_t));
        emm_sap.primitive = EMMREG_COMMON_PROC_REQ;
        emm_sap.u.emm_reg.ue_id = ue_id;
        MSC_LOG_TX_MESSAGE(
            MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0,
            "0 EMMREG_COMMON_PROC_REQ ue id " MME_UE_S1AP_ID_FMT " ", ue_id);
        rc = emm_sap_send(&emm_sap);
      } else {
        OAILOG_ERROR(LOG_NAS_EMM,
                     "TODO: THREADING ISSUES // NO EMM_CTX/PROCESS AFTER "
                     "SENDING SAP!\n");
      }
    } else {
      OAILOG_WARNING(LOG_NAS_EMM, "ERROR WHILE SENDING ATTACH_ACCEPT! \n");
    }
  } else {
    OAILOG_WARNING(LOG_NAS_EMM,
                   "Could not find an EMM context @ Attach Accept! \n");
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}
/*
 *
 * Name:    _emm_attach_accept_retx()
 *
 * Description: Retransmit ATTACH ACCEPT message and restart timer T3450
 *
 * Inputs:  data:      Attach accept retransmission data
 *      Others:    None
 * Outputs:     None
 *      Return:    RETURNok, RETURNerror
 *      Others:    T3450
 *
 */
//------------------------------------------------------------------------------
static int _emm_attach_accept_retx(emm_data_context_t *emm_context) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);
  emm_sap_t emm_sap = {0};
  int rc = RETURNerror;

  if (!emm_context) {
    OAILOG_WARNING(LOG_NAS_EMM, "emm_ctx NULL\n");
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
  }

  mme_ue_s1ap_id_t ue_id = emm_context->ue_id;
  nas_emm_attach_proc_t *attach_proc =
      get_nas_specific_procedure_attach(emm_context);

  if (attach_proc) {
    if (!IS_EMM_CTXT_PRESENT_GUTI(emm_context)) {
      OAILOG_WARNING(LOG_NAS_EMM,
                     " No GUTI present in emm_ctx. Abormal case. Skipping Retx "
                     "of Attach Accept NULL\n");
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
    }
    /*
     * Notify EMM-AS SAP that Attach Accept message together with an Activate
     * Default EPS Bearer Context Request message has to be sent to the UE.
     * Retx of Attach Accept needs to be done via DL NAS Transport S1AP message
     */
    emm_sap.primitive = EMMAS_DATA_REQ;
    emm_sap.u.emm_as.u.data.ue_id = ue_id;
    emm_sap.u.emm_as.u.data.nas_info = EMM_AS_NAS_DATA_ATTACH_ACCEPT;
    memcpy(&emm_sap.u.emm_as.u.data.tai_list, &emm_context->_tai_list,
           sizeof(tai_list_t));
    emm_sap.u.emm_as.u.data.eps_id.guti = &emm_context->_guti;
    OAILOG_DEBUG(LOG_NAS_EMM,
                 "ue_id=" MME_UE_S1AP_ID_FMT
                 " EMM-PROC  - Include the same GUTI in the Attach Accept Retx "
                 "message\n",
                 ue_id);
    emm_sap.u.emm_as.u.data.new_guti = &emm_context->_guti;
    emm_sap.u.emm_as.u.data.eps_network_feature_support =
        &_emm_data.conf.eps_network_feature_support;
    /*
     * Setup EPS NAS security data
     */
    emm_as_set_security_data(&emm_sap.u.emm_as.u.data.sctx_data,
                             &emm_context->_security, false, true);
    emm_sap.u.emm_as.u.data.encryption =
        emm_context->_security.selected_algorithms.encryption;
    emm_sap.u.emm_as.u.data.integrity =
        emm_context->_security.selected_algorithms.integrity;
    /*
     * Get the activate default EPS bearer context request message to
     * transfer within the ESM container of the attach accept message
     */
    emm_sap.u.emm_as.u.data.nas_msg = attach_proc->esm_msg_out;
    OAILOG_TRACE(
        LOG_NAS_EMM,
        "ue_id=" MME_UE_S1AP_ID_FMT
        " EMM-PROC  - nas_msg  src size = %d nas_msg  dst size = %d \n",
        ue_id, blength(attach_proc->esm_msg_out),
        blength(emm_sap.u.emm_as.u.data.nas_msg));

    rc = emm_sap_send(&emm_sap);

    if (RETURNerror != rc) {
      OAILOG_INFO(LOG_NAS_EMM,
                  "ue_id=" MME_UE_S1AP_ID_FMT
                  " EMM-PROC  -Sent Retx Attach Accept message\n",
                  ue_id);

      // todo: REMOVE AFTER LOCKING
      emm_data_context_t *emm_context1 =
          emm_data_context_get(&_emm_data, ue_id);
      if (emm_context1) {
        attach_proc = get_nas_specific_procedure_attach(emm_context1);
        if (attach_proc) {
          /*
           * Re-start T3450 timer
           */
          void *unused = NULL;
          nas_stop_T3450(ue_id, &attach_proc->T3450, unused);
          nas_start_T3450(
              ue_id, &attach_proc->T3450,
              attach_proc->emm_spec_proc.emm_proc.base_proc.time_out,
              (void *)ue_id);
          OAILOG_INFO(LOG_NAS_EMM,
                      "ue_id=" MME_UE_S1AP_ID_FMT
                      " EMM-PROC  - SUCCESSFULLY RESTARTED T3450 AFTER SENDING "
                      "ATTACH ACCEPT message\n",
                      ue_id);
        }
      }
    } else {
      OAILOG_WARNING(LOG_NAS_EMM,
                     "ue_id=" MME_UE_S1AP_ID_FMT
                     " EMM-PROC  - Send failed- Retx Attach Accept message\n",
                     ue_id);
    }
  }
  OAILOG_FUNC_RETURN(LOG_NAS_EMM, rc);
}

/*
 * Description: Check whether the given attach parameters differs from
 *      those previously stored when the attach procedure has
 *      been initiated.
 *
 * Outputs:     None
 *      Return:    true if at least one of the parameters
 *             differs; false otherwise.
 *      Others:    None
 *
 */
//-----------------------------------------------------------------------------
static bool _emm_attach_ies_have_changed(mme_ue_s1ap_id_t ue_id,
                                         emm_attach_request_ies_t *const ies1,
                                         emm_attach_request_ies_t *const ies2) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);

  if (ies1->type != ies2->type) {
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT
                " Attach IEs changed: type EMM_ATTACH_TYPE\n",
                ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }
  if (ies1->is_native_sc != ies2->is_native_sc) {
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT
                " Attach IEs changed: Is native securitty context\n",
                ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }
  if (ies1->ksi != ies2->ksi) {
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT " Attach IEs changed: KSI %d -> %d \n",
                ue_id, ies1->ksi, ies2->ksi);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  /*
   * The GUTI if provided by the UE
   */
  if (ies1->is_native_guti != ies2->is_native_guti) {
    OAILOG_DEBUG(LOG_NAS_EMM,
                 "UE " MME_UE_S1AP_ID_FMT
                 " Attach IEs changed: Native GUTI %d -> %d \n",
                 ue_id, ies1->is_native_guti, ies2->is_native_guti);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }
  if ((ies1->guti) && (!ies2->guti)) {
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT " Attach IEs changed:  GUTI " GUTI_FMT
                " -> None\n",
                ue_id, GUTI_ARG(ies1->guti));
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  if ((!ies1->guti) && (ies2->guti)) {
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT
                " Attach IEs changed:  GUTI None ->  " GUTI_FMT "\n",
                ue_id, GUTI_ARG(ies2->guti));
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  if ((ies1->guti) && (ies2->guti)) {
    if (memcmp(ies1->guti, ies2->guti, sizeof(*ies1->guti))) {
      OAILOG_INFO(LOG_NAS_EMM,
                  "UE " MME_UE_S1AP_ID_FMT
                  " Attach IEs changed:  guti/tmsi " GUTI_FMT " -> " GUTI_FMT
                  "\n",
                  ue_id, GUTI_ARG(ies1->guti), GUTI_ARG(ies2->guti));
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
    }
  }

  /*
   * The IMSI if provided by the UE
   */
  if ((ies1->imsi) && (!ies2->imsi)) {
    imsi64_t imsi641 = imsi_to_imsi64(ies1->imsi);
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT
                " Attach IEs changed:  IMSI " IMSI_64_FMT " -> None\n",
                ue_id, imsi641);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  if ((!ies1->imsi) && (ies2->imsi)) {
    imsi64_t imsi642 = imsi_to_imsi64(ies2->imsi);
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT
                " Attach IEs changed:  IMSI None ->  " IMSI_64_FMT "\n",
                ue_id, imsi642);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  if ((ies1->guti) && (ies2->guti)) {
    imsi64_t imsi641 = imsi_to_imsi64(ies1->imsi);
    imsi64_t imsi642 = imsi_to_imsi64(ies2->imsi);
    if (memcmp(ies1->guti, ies2->guti, sizeof(*ies1->guti))) {
      OAILOG_INFO(LOG_NAS_EMM,
                  "UE " MME_UE_S1AP_ID_FMT
                  " Attach IEs changed:  IMSI " IMSI_64_FMT " -> " IMSI_64_FMT
                  "\n",
                  ue_id, imsi641, imsi642);
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
    }
  }

  /*
   * The IMEI if provided by the UE
   */
  if ((ies1->imei) && (!ies2->imei)) {
    char imei_str[16];

    IMEI_TO_STRING(ies1->imei, imei_str, 16);
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT
                " Attach IEs changed: imei %s/NULL (ctxt)\n",
                ue_id, imei_str);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  if ((!ies1->imei) && (ies2->imei)) {
    char imei_str[16];

    IMEI_TO_STRING(ies2->imei, imei_str, 16);
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT
                " Attach IEs changed: imei NULL/%s (ctxt)\n",
                ue_id, imei_str);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  if ((ies1->imei) && (ies2->imei)) {
    if (memcmp(ies1->imei, ies2->imei, sizeof(*ies2->imei)) != 0) {
      char imei_str[16];
      char imei2_str[16];

      IMEI_TO_STRING(ies1->imei, imei_str, 16);
      IMEI_TO_STRING(ies2->imei, imei2_str, 16);
      OAILOG_INFO(LOG_NAS_EMM,
                  "UE " MME_UE_S1AP_ID_FMT
                  " Attach IEs changed: imei %s/%s (ctxt)\n",
                  ue_id, imei_str, imei2_str);
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
    }
  }

  /*
   * The Last visited registered TAI if provided by the UE
   */
  if ((ies1->last_visited_registered_tai) &&
      (!ies2->last_visited_registered_tai)) {
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT " Attach IEs changed: LVR TAI " TAI_FMT
                "/NULL\n",
                ue_id, TAI_ARG(ies1->last_visited_registered_tai));
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  if ((!ies1->last_visited_registered_tai) &&
      (ies2->last_visited_registered_tai)) {
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT
                " Attach IEs changed: LVR TAI NULL/" TAI_FMT "\n",
                ue_id, TAI_ARG(ies2->last_visited_registered_tai));
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  if ((ies1->last_visited_registered_tai) &&
      (ies2->last_visited_registered_tai)) {
    if (memcmp(ies1->last_visited_registered_tai,
               ies2->last_visited_registered_tai,
               sizeof(*ies2->last_visited_registered_tai)) != 0) {
      OAILOG_INFO(LOG_NAS_EMM,
                  "UE " MME_UE_S1AP_ID_FMT
                  " Attach IEs changed: LVR TAI " TAI_FMT "/" TAI_FMT "\n",
                  ue_id, TAI_ARG(ies1->last_visited_registered_tai),
                  TAI_ARG(ies2->last_visited_registered_tai));
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
    }
  }

  /*
   * Originating TAI
   */
  if ((ies1->originating_tai) && (!ies2->originating_tai)) {
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT
                " Attach IEs changed: orig TAI " TAI_FMT "/NULL\n",
                ue_id, TAI_ARG(ies1->originating_tai));
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  if ((!ies1->originating_tai) && (ies2->originating_tai)) {
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT
                " Attach IEs changed: orig TAI NULL/" TAI_FMT "\n",
                ue_id, TAI_ARG(ies2->originating_tai));
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  if ((ies1->originating_tai) && (ies2->originating_tai)) {
    if (memcmp(ies1->originating_tai, ies2->originating_tai,
               sizeof(*ies2->originating_tai)) != 0) {
      OAILOG_INFO(LOG_NAS_EMM,
                  "UE " MME_UE_S1AP_ID_FMT
                  " Attach IEs changed: orig TAI " TAI_FMT "/" TAI_FMT "\n",
                  ue_id, TAI_ARG(ies1->originating_tai),
                  TAI_ARG(ies2->originating_tai));
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
    }
  }

  /*
   * Originating ECGI
   */
  if ((ies1->originating_ecgi) && (!ies2->originating_ecgi)) {
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT " Attach IEs changed: orig ECGI\n",
                ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  if ((!ies1->originating_ecgi) && (ies2->originating_ecgi)) {
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT " Attach IEs changed: orig ECGI\n",
                ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  if ((ies1->originating_ecgi) && (ies2->originating_ecgi)) {
    if (memcmp(ies1->originating_ecgi, ies2->originating_ecgi,
               sizeof(*ies2->originating_ecgi)) != 0) {
      OAILOG_INFO(LOG_NAS_EMM,
                  "UE " MME_UE_S1AP_ID_FMT " Attach IEs changed: orig ECGI\n",
                  ue_id);
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
    }
  }

  /*
   * UE network capability
   */
  if (memcmp(ies1->ue_network_capability, ies2->ue_network_capability,
             sizeof(*ies1->ue_network_capability))) {
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT
                " Attach IEs changed: UE network capability\n",
                ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  /*
   * MS network capability
   */
  if ((ies1->ms_network_capability) && (!ies2->ms_network_capability)) {
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT
                " Attach IEs changed: MS network capability\n",
                ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  if ((!ies1->ms_network_capability) && (ies2->ms_network_capability)) {
    OAILOG_INFO(LOG_NAS_EMM,
                "UE " MME_UE_S1AP_ID_FMT
                " Attach IEs changed: MS network capability\n",
                ue_id);
    OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
  }

  if ((ies1->ms_network_capability) && (ies2->ms_network_capability)) {
    if (memcmp(ies1->ms_network_capability, ies2->ms_network_capability,
               sizeof(*ies2->ms_network_capability)) != 0) {
      OAILOG_INFO(LOG_NAS_EMM,
                  "UE " MME_UE_S1AP_ID_FMT
                  " Attach IEs changed: MS network capability\n",
                  ue_id);
      OAILOG_FUNC_RETURN(LOG_NAS_EMM, true);
    }
  }
  // TODO ESM MSG ?

  OAILOG_FUNC_RETURN(LOG_NAS_EMM, false);
}

//------------------------------------------------------------------------------
void free_emm_attach_request_ies(emm_attach_request_ies_t **const ies) {
  if ((*ies)->guti) {
    free_wrapper((void **)&(*ies)->guti);
  }
  if ((*ies)->imsi) {
    free_wrapper((void **)&(*ies)->imsi);
  }
  if ((*ies)->imei) {
    free_wrapper((void **)&(*ies)->imei);
  }
  if ((*ies)->last_visited_registered_tai) {
    free_wrapper((void **)&(*ies)->last_visited_registered_tai);
  }
  if ((*ies)->originating_tai) {
    free_wrapper((void **)&(*ies)->originating_tai);
  }
  if ((*ies)->originating_ecgi) {
    free_wrapper((void **)&(*ies)->originating_ecgi);
  }
  if ((*ies)->ms_network_capability) {
    free_wrapper((void **)&(*ies)->ms_network_capability);
  }
  if ((*ies)->ue_network_capability) {
    free_wrapper((void **)&(*ies)->ue_network_capability);
  }
  if ((*ies)->esm_msg_attach_proc) {
    bdestroy_wrapper(&((*ies)->esm_msg_attach_proc));
  }
  if ((*ies)->drx_parameter) {
    free_wrapper((void **)&(*ies)->drx_parameter);
  }
  free_wrapper((void **)ies);
}

/*
  Name:    _emm_attach_update()

  Description: Update the EMM context with the given attach procedure
       parameters.

  Inputs:  ue_id:      UE lower layer identifier
       type:      Type of the requested attach
       ksi:       Security ket sey identifier
       guti:      The GUTI provided by the UE
       imsi:      The IMSI provided by the UE
       imei:      The IMEI provided by the UE
       eea:       Supported EPS encryption algorithms
       originating_tai Originating TAI (from eNB TAI)
       eia:       Supported EPS integrity algorithms
       esm_msg_pP:   ESM message contained with the attach re-
              quest
       Others:    None
  Outputs:     ctx:       EMM context of the UE in the network
       Return:    RETURNok, RETURNerror
       Others:    None

 */
//------------------------------------------------------------------------------
static int _emm_attach_update(emm_data_context_t *const emm_context,
                              emm_attach_request_ies_t *const ies) {
  OAILOG_FUNC_IN(LOG_NAS_EMM);

  /*
   * Emergency bearer services indicator
   */
  emm_context->is_emergency = (ies->type == EMM_ATTACH_TYPE_EMERGENCY);
  /*
   * Security key set identifier
   */
  if (emm_context->ksi != ies->ksi) {
    OAILOG_TRACE(LOG_NAS_EMM,
                 "UE id " MME_UE_S1AP_ID_FMT " Update ue ksi %d -> %d\n",
                 emm_context->ue_id, emm_context->ksi, ies->ksi);
    emm_context->ksi = ies->ksi;
  }
  /*
   * Supported EPS encryption algorithms
   */

  //----------------------------------------
  REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__4);
  emm_ctx_set_valid_ue_nw_cap(emm_context, ies->ue_network_capability);

  if (ies->ms_network_capability) {
    emm_ctx_set_valid_ms_nw_cap(emm_context, ies->ms_network_capability);
  } else {
    // optional IE
    emm_ctx_clear_ms_nw_cap(emm_context);
  }

  emm_context->originating_tai = *ies->originating_tai;
  emm_context->is_guti_based_attach = false;

  /*
   * The GUTI if provided by the UE. Trigger UE Identity Procedure to fetch IMSI
   */
  if (ies->guti) {
    emm_context->is_guti_based_attach = true;
  }
  /*
   * The IMSI if provided by the UE
   */
  if (ies->imsi) {
    imsi64_t new_imsi64 = imsi_to_imsi64(ies->imsi);
    if (new_imsi64 != emm_context->_imsi64) {
      emm_ctx_set_valid_imsi(emm_context, ies->imsi, new_imsi64);
    }
  }

  /*
   * The IMEI if provided by the UE
   */
  if (ies->imei) {
    emm_ctx_set_valid_imei(emm_context, ies->imei);
  }

  //----------------------------------------
  if (ies->drx_parameter) {
    REQUIREMENT_3GPP_24_301(R10_5_5_1_2_4__5);
    emm_ctx_set_valid_drx_parameter(emm_context, ies->drx_parameter);
  }

  OAILOG_FUNC_RETURN(LOG_NAS_EMM, RETURNok);
}
