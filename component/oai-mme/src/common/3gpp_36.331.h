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

/*! \file 3gpp_36.331.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_3GPP_36_331_SEEN
#define FILE_3GPP_36_331_SEEN

// could be extracted with asn1 tool

typedef enum EstablishmentCause {
  EMERGENCY = 1,
  HIGH_PRIORITY_ACCESS,
  MT_ACCESS,
  MO_SIGNALLING,
  MO_DATA,
  DELAY_TOLERANT_ACCESS_V1020,
  SPARE2,
  SPARE1
} rrc_establishment_cause_t;

#endif /* FILE_3GPP_36_331_SEEN */
