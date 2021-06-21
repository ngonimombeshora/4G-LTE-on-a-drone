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

/*! \file 3gpp_requirements_36.413.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_3GPP_REQUIREMENTS_36_413_SEEN
#define FILE_3GPP_REQUIREMENTS_36_413_SEEN

#include "3gpp_requirements.h"
#include "log.h"

#define REQUIREMENT_3GPP_36_413(rElEaSe_sEcTiOn__OaImark)              \
  REQUIREMENT_3GPP_SPEC(LOG_S1AP,                                      \
                        "Hit 3GPP TS 36_413" #rElEaSe_sEcTiOn__OaImark \
                        " : " rElEaSe_sEcTiOn__OaImark##_BRIEF "\n")
#define NO_REQUIREMENT_3GPP_36_413(rElEaSe_sEcTiOn__OaImark)                \
  REQUIREMENT_3GPP_SPEC(                                                    \
      LOG_S1AP, "#NOT IMPLEMENTED 3GPP TS 36_413" #rElEaSe_sEcTiOn__OaImark \
                " : " rElEaSe_sEcTiOn__OaImark##_BRIEF "\n")
#define NOT_REQUIREMENT_3GPP_36_413(rElEaSe_sEcTiOn__OaImark)            \
  REQUIREMENT_3GPP_SPEC(                                                 \
      LOG_S1AP, "#NOT ASSERTED 3GPP TS 36_413" #rElEaSe_sEcTiOn__OaImark \
                " : " rElEaSe_sEcTiOn__OaImark##_BRIEF "\n")

//-----------------------------------------------------------------------------------------------------------------------
#define R10_8_3_3_2__2 \
  "MME36.413R10_8.3.3.2_2: Successful Operation\
                                                                                                                        \
    The UE CONTEXT RELEASE COMMAND message shall contain the UE S1AP ID pair IE if available, otherwise the             \
    message shall contain the MME UE S1AP ID IE."

#define R10_8_3_3_2__2_BRIEF                                                   \
  "UE CONTEXT RELEASE COMMAND contains UE S1AP ID pair IE or at least MME UE " \
  "S1AP ID IE"

#endif /* FILE_3GPP_REQUIREMENTS_36_413_SEEN */
