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
Source      nas_emm_proc.h

Version     0.1

Date        2012/09/20

Product     NAS EMM stack

Subsystem   NAS EMM main process

Author      Frederic Maurel, Lionel GAUTHIER, Dincer Beken

Description NAS EMM procedure call manager

*****************************************************************************/
#ifndef FILE_NAS_EMM_PROC_SEEN
#define FILE_NAS_EMM_PROC_SEEN

#include "common_defs.h"
#include "emm_cnDef.h"
#include "mme_config.h"
#include "networkDef.h"

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
 *          NAS procedures triggered by the user
 * --------------------------------------------------------------------------
 */

/*
 * --------------------------------------------------------------------------
 *      NAS procedures triggered by the network
 * --------------------------------------------------------------------------
 */

int nas_proc_establish_ind(const mme_ue_s1ap_id_t ue_id,
                           const tai_t originating_tai, const ecgi_t ecgi,
                           const as_cause_t as_cause, STOLEN_REF bstring* msg);

int nas_proc_dl_transfer_cnf(const mme_ue_s1ap_id_t ueid,
                             const nas_error_code_t status,
                             STOLEN_REF bstring* nas_msg);
int nas_proc_dl_transfer_rej(const mme_ue_s1ap_id_t ueid,
                             const nas_error_code_t status,
                             STOLEN_REF bstring* nas_msg);
int nas_proc_ul_transfer_ind(const mme_ue_s1ap_id_t ueid,
                             const tai_t originating_tai, const ecgi_t cgi,
                             STOLEN_REF bstring* msg);

/*
 * --------------------------------------------------------------------------
 *      NAS EMM procedures triggered by the mme applicative layer
 * --------------------------------------------------------------------------
 */
int nas_proc_authentication_info_answer(s6a_auth_info_ans_t* ans);
int nas_proc_auth_param_res(mme_ue_s1ap_id_t ue_id, uint8_t nb_vectors,
                            eutran_vector_t* vectors);
int nas_proc_auth_param_fail(mme_ue_s1ap_id_t ue_id, nas_cause_t cause);
int nas_proc_deregister_ue(uint32_t ue_id);

int nas_proc_signalling_connection_rel_ind(mme_ue_s1ap_id_t ue_id);
int nas_proc_implicit_detach_ue_ind(mme_ue_s1ap_id_t ue_id, uint8_t emm_cause,
                                    uint8_t detach_type, bool clr);
/** NAS context response. */
int nas_proc_context_res(itti_nas_context_res_t* nas_context_res);
int nas_proc_context_fail(mme_ue_s1ap_id_t ue_id, gtpv2c_cause_value_t cause);

#endif /* FILE_NAS_PROC_SEEN*/
