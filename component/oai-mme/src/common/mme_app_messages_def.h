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

/*! \file mme_app_messages_def.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

// WARNING: Do not include this header directly. Use intertask_interface.h
// instead.

MESSAGE_DEF(MME_APP_CONNECTION_ESTABLISHMENT_CNF, MESSAGE_PRIORITY_MED,
            itti_mme_app_connection_establishment_cnf_t,
            mme_app_connection_establishment_cnf)
MESSAGE_DEF(MME_APP_INITIAL_CONTEXT_SETUP_RSP, MESSAGE_PRIORITY_MED,
            itti_mme_app_initial_context_setup_rsp_t,
            mme_app_initial_context_setup_rsp)

/** New message to signal the MME_UE_S1AP_ID to S1AP. */
MESSAGE_DEF(MME_APP_S1AP_MME_UE_ID_NOTIFICATION, MESSAGE_PRIORITY_MED,
            itti_mme_app_s1ap_mme_ue_id_notification_t,
            mme_app_s1ap_mme_ue_id_notification)
MESSAGE_DEF(MME_APP_INITIAL_CONTEXT_SETUP_FAILURE, MESSAGE_PRIORITY_MED,
            itti_mme_app_initial_context_setup_failure_t,
            mme_app_initial_context_setup_failure)
MESSAGE_DEF(MME_APP_NAS_UPDATE_LOCATION_CNF, MESSAGE_PRIORITY_MED,
            itti_mme_app_nas_update_location_cnf_t,
            mme_app_nas_update_location_cnf)
