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

Source      emm_cn.h

Version     0.1

Date        2013/12/05

Product     NAS stack

Subsystem   EPS Core Network

Author      Sebastien Roux, Lionel GAUTHIER

Description

*****************************************************************************/

#ifndef FILE_EMM_CN_SEEN
#define FILE_EMM_CN_SEEN

#include "emm_cnDef.h"
int emm_cn_send(const emm_cn_t* msg);
// int emm_cn_pdn_connectivity_res (emm_cn_pdn_res_t * msg_pP);
int _emm_wrapper_esm_accept(mme_ue_s1ap_id_t ue_id, bstring* esm_rsp,
                            eps_bearer_context_status_t ebr_status);
int _emm_wrapper_esm_reject(mme_ue_s1ap_id_t ue_id, bstring* esm_rsp);

#endif /* FILE_EMM_CN_SEEN */
