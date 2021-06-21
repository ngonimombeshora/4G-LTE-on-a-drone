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
// WARNING: Do not include this header directly. Use intertask_interface.h
// instead.

/*! \file s6a_messages_def.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

MESSAGE_DEF(S6A_UPDATE_LOCATION_REQ, MESSAGE_PRIORITY_MED,
            s6a_update_location_req_t, s6a_update_location_req)
MESSAGE_DEF(S6A_UPDATE_LOCATION_ANS, MESSAGE_PRIORITY_MED,
            s6a_update_location_ans_t, s6a_update_location_ans)
MESSAGE_DEF(S6A_AUTH_INFO_REQ, MESSAGE_PRIORITY_MED, s6a_auth_info_req_t,
            s6a_auth_info_req)
MESSAGE_DEF(S6A_AUTH_INFO_ANS, MESSAGE_PRIORITY_MED, s6a_auth_info_ans_t,
            s6a_auth_info_ans)
MESSAGE_DEF(S6A_CANCEL_LOCATION_REQ, MESSAGE_PRIORITY_MED,
            s6a_cancel_location_req_t, s6a_cancel_location_req)
MESSAGE_DEF(S6A_RESET_REQ, MESSAGE_PRIORITY_MED, s6a_reset_req_t, s6a_reset_req)
/** Notify Request. */
MESSAGE_DEF(S6A_NOTIFY_REQ, MESSAGE_PRIORITY_MED, s6a_notify_req_t,
            s6a_notify_req)
MESSAGE_DEF(S6A_NOTIFY_ANS, MESSAGE_PRIORITY_MED, s6a_notify_ans_t,
            s6a_notify_ans)
