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

/*! \file s6a_messages_types.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_S6A_MESSAGES_TYPES_SEEN
#define FILE_S6A_MESSAGES_TYPES_SEEN

#define S6A_UPDATE_LOCATION_REQ(mSGpTR) \
  (mSGpTR)->ittiMsg.s6a_update_location_req
#define S6A_UPDATE_LOCATION_ANS(mSGpTR) \
  (mSGpTR)->ittiMsg.s6a_update_location_ans
#define S6A_AUTH_INFO_REQ(mSGpTR) (mSGpTR)->ittiMsg.s6a_auth_info_req
#define S6A_AUTH_INFO_ANS(mSGpTR) (mSGpTR)->ittiMsg.s6a_auth_info_ans
#define S6A_CANCEL_LOCATION_REQ(mSGpTR) \
  (mSGpTR)->ittiMsg.s6a_cancel_location_req
#define S6A_RESET_REQ(mSGpTR) (mSGpTR)->ittiMsg.s6a_reset_req

/** Notify Request/Answer. */
#define S6A_NOTIFY_REQ(mSGpTR) (mSGpTR)->ittiMsg.s6a_notify_req
#define S6A_NOTIFY_ANS(mSGpTR) (mSGpTR)->ittiMsg.s6a_notify_ans

#define AUTS_LENGTH 14
#define RESYNC_PARAM_LENGTH AUTS_LENGTH + RAND_LENGTH_OCTETS

typedef struct s6a_update_location_req_s {
  mme_ue_s1ap_id_t ue_id;
  imsi64_t imsi64;

#define SKIP_SUBSCRIBER_DATA (0x1)
  unsigned skip_subscriber_data : 1;
#define INITIAL_ATTACH (0x1)
  unsigned initial_attach : 1;

  char imsi[IMSI_BCD_DIGITS_MAX + 1];  // username
  uint8_t imsi_length;                 // username

  plmn_t visited_plmn;  // visited plmn id
  rat_type_t rat_type;  // rat type

  // missing                           // origin host
  // missing                           // origin realm

  // missing                           // destination host
  // missing                           // destination realm
} s6a_update_location_req_t;

typedef struct s6a_update_location_ans_s {
  mme_ue_s1ap_id_t ue_id;
  s6a_result_t result;  // Result of the update location request procedure
  subscription_data_t* subscription_data;  // subscriber status,
  //  bool 			       initial_attach;

  // Maximum Requested Bandwidth Uplink, downlink
  // access restriction data
  // msisdn
  // apn_config_profile_t  apn_config_profile;// APN configuration profile

  network_access_mode_t access_mode;
  rau_tau_timer_t rau_tau_timer;
  char imsi[IMSI_BCD_DIGITS_MAX + 1];
  uint8_t imsi_length;

} s6a_update_location_ans_t;

typedef struct s6a_auth_info_req_s {
  char imsi[IMSI_BCD_DIGITS_MAX + 1];
  uint8_t imsi_length;
  plmn_t visited_plmn;
  /* Number of vectors to retrieve from HSS, should be equal to one */
  uint8_t nb_of_vectors;

  /* Bit to indicate that USIM has requested a re-synchronization of SQN */
  unsigned re_synchronization : 1;
  /* AUTS to provide to AUC.
   * Only present and interpreted if re_synchronization == 1.
   */
  uint8_t auts[RESYNC_PARAM_LENGTH];
} s6a_auth_info_req_t;

typedef struct s6a_auth_info_ans_s {
  char imsi[IMSI_BCD_DIGITS_MAX + 1];
  uint8_t imsi_length;

  /* Result of the authentication information procedure */
  s6a_result_t result;
  /* Authentication info containing the vector(s) */
  authentication_info_t auth_info;
} s6a_auth_info_ans_t;

typedef struct s6a_cancel_location_req_s {
  char imsi[IMSI_BCD_DIGITS_MAX + 1];  // username
  uint8_t imsi_length;                 // username

  uint32_t clr_flags;  // clr flags

  cancellation_type_t cancellation_type;
} s6a_cancel_location_req_t;

typedef struct s6a_reset_req_s {
  // todo: maybe username?
} s6a_reset_req_t;

typedef struct s6a_notify_req_s {
#define SINGLE_REGITRATION_INDICATION (0x1)
  unsigned single_registration_indiction : 1;

  char imsi[IMSI_BCD_DIGITS_MAX + 1];
  uint8_t imsi_length;
  plmn_t visited_plmn;

} s6a_notify_req_t;

typedef struct s6a_notify_ans_s {
  s6a_result_t result;

} s6a_notify_ans_t;

#endif /* FILE_S6A_MESSAGES_TYPES_SEEN */
