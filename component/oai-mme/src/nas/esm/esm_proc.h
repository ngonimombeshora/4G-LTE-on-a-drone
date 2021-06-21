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
Source      esm_proc.h

Version     0.1

Date        2013/01/02

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines the EPS Session Management procedures executed at
        the ESM Service Access Points.

*****************************************************************************/
#ifndef __ESM_PROC_H__
#define __ESM_PROC_H__

#include "common_defs.h"
#include "common_types.h"
#include "mme_app_esm_procedures.h"
#include "networkDef.h"
// todo: temporarily until better solution
#include "LowerLayer.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *              ESM status procedure
 * --------------------------------------------------------------------------
 */

int esm_proc_status_ind(mme_ue_s1ap_id_t ue_id, const proc_tid_t pti, ebi_t ebi,
                        esm_cause_t* esm_cause);

/*
 * --------------------------------------------------------------------------
 *          PDN connectivity procedure
 * --------------------------------------------------------------------------
 */

/*
 * PDN Connectivity procedure (UE triggered - incl. initial attach).
 */
nas_esm_proc_pdn_connectivity_t* _esm_proc_create_pdn_connectivity_procedure(
    mme_ue_s1ap_id_t ue_id, imsi_t* imsi, pti_t pti);
void _esm_proc_free_pdn_connectivity_procedure(
    nas_esm_proc_pdn_connectivity_t** esm_proc_pdn_connectivity);
nas_esm_proc_pdn_connectivity_t* _esm_proc_get_pdn_connectivity_procedure(
    mme_ue_s1ap_id_t ue_id, pti_t pti);

/*
 * Bearer context procedure, which may or may not be a transactional procedure
 * (triggered by UE/congestion, or CN).
 */

esm_cause_t esm_proc_pdn_connectivity_request(
    mme_ue_s1ap_id_t ue_id, imsi_t* imsi, tai_t* visited_tai,
    nas_esm_proc_pdn_connectivity_t* const esm_proc_pdn_connectivity,
    const apn_configuration_t* apn_configuration);

esm_cause_t esm_proc_pdn_connectivity_retx(
    const mme_ue_s1ap_id_t ue_id,
    nas_esm_proc_pdn_connectivity_t* const esm_proc_pdn_connectivity,
    ESM_msg* esm_rsp_msg);

void esm_proc_pdn_connectivity_failure(
    mme_ue_s1ap_id_t ue_id,
    nas_esm_proc_pdn_connectivity_t* esm_pdn_connectivity_proc);

esm_cause_t esm_proc_pdn_config_res(mme_ue_s1ap_id_t ue_id, bool* is_attach,
                                    pti_t* pti, imsi_t* imsi,
                                    tai_t* visited_tai,
                                    eps_bearer_context_status_t* active_ebrs);

esm_cause_t esm_proc_pdn_connectivity_res(
    mme_ue_s1ap_id_t ue_id,
    nas_esm_proc_pdn_connectivity_t* esm_proc_pdn_connectivity);

/*
 * --------------------------------------------------------------------------
 *              PDN disconnect procedure
 * --------------------------------------------------------------------------
 */

esm_cause_t esm_proc_pdn_disconnect_request(mme_ue_s1ap_id_t ue_id,
                                            proc_tid_t pti, pdn_cid_t pdn_cid,
                                            ebi_t linked_ebi);

void esm_proc_detach_request(mme_ue_s1ap_id_t ue_id, bool clr);

/*
 * --------------------------------------------------------------------------
 *              ESM information procedure
 * --------------------------------------------------------------------------
 */

void esm_proc_esm_information_request(
    nas_esm_proc_pdn_connectivity_t* esm_pdn_connectivity,
    ESM_msg* esm_result_msg);

void esm_proc_esm_information_response(
    mme_ue_s1ap_id_t ue_id, pti_t pti,
    nas_esm_proc_pdn_connectivity_t* nas_pdn_connectivity_proc,
    const esm_information_response_msg* const esm_information_resp);

/*
 * --------------------------------------------------------------------------
 *      Default EPS bearer context activation procedure
 * --------------------------------------------------------------------------
 */
void esm_proc_default_eps_bearer_context(
    mme_ue_s1ap_id_t ue_id, ESM_msg* const esm_rsp_msg,
    nas_esm_proc_pdn_connectivity_t* const esm_proc_pdn_connectivity);

void esm_proc_default_eps_bearer_context_accept(
    mme_ue_s1ap_id_t ue_id,
    const nas_esm_proc_pdn_connectivity_t* const esm_pdn_connectivity_proc);

/*
 * --------------------------------------------------------------------------
 *      Dedicated EPS bearer context activation procedure
 * --------------------------------------------------------------------------
 */
esm_cause_t esm_proc_dedicated_eps_bearer_context(
    mme_ue_s1ap_id_t ue_id, const proc_tid_t pti, const bool retry,
    int* retx_count, ebi_t linked_ebi, const pdn_cid_t pdn_cid,
    bearer_context_to_be_created_t* bc_tbc, ESM_msg* esm_rsp_msg);

esm_cause_t esm_proc_dedicated_eps_bearer_context_accept(
    mme_ue_s1ap_id_t ue_id, ebi_t ebi,
    nas_esm_proc_bearer_context_t* esm_bearer_procedure);

void esm_proc_dedicated_eps_bearer_context_reject(
    mme_ue_s1ap_id_t ue_id, ebi_t ebi,
    nas_esm_proc_bearer_context_t* esm_bearer_procedure, esm_cause_t esm_cause);

/*
 * --------------------------------------------------------------------------
 *      EPS bearer context modification procedure
 * --------------------------------------------------------------------------
 */
esm_cause_t esm_proc_modify_eps_bearer_context(
    mme_ue_s1ap_id_t ue_id, const proc_tid_t pti, const bool retry,
    int* retx_count, const ebi_t linked_ebi, const pdn_cid_t pdn_cid,
    bearer_context_to_be_updated_t* bc_tbu, ambr_t* apn_ambr,
    ESM_msg* esm_rsp_msg);

esm_cause_t esm_proc_modify_eps_bearer_context_accept(
    mme_ue_s1ap_id_t ue_id, ebi_t ebi,
    nas_esm_proc_bearer_context_t* esm_bearer_procedure);

void esm_proc_modify_eps_bearer_context_reject(
    mme_ue_s1ap_id_t ue_id, ebi_t ebi,
    nas_esm_proc_bearer_context_t* esm_bearer_procedure, esm_cause_t esm_cause);

/*
 * --------------------------------------------------------------------------
 *      EPS bearer context deactivation procedure
 * --------------------------------------------------------------------------
 */
esm_cause_t esm_proc_eps_bearer_context_deactivate_request(
    mme_ue_s1ap_id_t ue_id, proc_tid_t* pti, const bool retry, int* retx_count,
    ebi_t* ebi, ebi_list_t* ded_ebis, ESM_msg* esm_rsp_msg);

/*
 * --------------------------------------------------------------------------
 *      EPS bearer context allocation procedure
 * --------------------------------------------------------------------------
 */
esm_cause_t esm_proc_bearer_resource_allocation_request(
    mme_ue_s1ap_id_t ue_id, const proc_tid_t pti, ebi_t ebi,
    const traffic_flow_aggregate_description_t* const tft);

/*
 * --------------------------------------------------------------------------
 *      EPS bearer context modification procedure
 * --------------------------------------------------------------------------
 */
esm_cause_t esm_proc_bearer_resource_modification_request(
    mme_ue_s1ap_id_t ue_id, const proc_tid_t pti, ebi_t ebi,
    esm_cause_t esm_cause_received,
    //  const traffic_flow_aggregate_description_t * const tad,
    traffic_flow_aggregate_description_t* const tad,
    const EpsQualityOfService* const new_flow_qos);

void esm_proc_bearer_resource_failure(mme_ue_s1ap_id_t ue_id,
                                      const proc_tid_t pti,
                                      ESM_msg* const esm_rsp_msg);

#endif /* __ESM_PROC_H__*/
