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

/*! \file scenario_player_messages_types.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_SCENARIO_PLAYER_MESSAGES_TYPES_SEEN
#define FILE_SCENARIO_PLAYER_MESSAGES_TYPES_SEEN

#define SP_SCENARIO_TICK(mSGpTR) (mSGpTR)->ittiMsg.scenario_tick

typedef struct sp_scenario_tick_s {
  void* scenario;
} sp_scenario_tick_t;

#endif /* FILE_SCENARIO_PLAYER_MESSAGES_TYPES_SEEN */
