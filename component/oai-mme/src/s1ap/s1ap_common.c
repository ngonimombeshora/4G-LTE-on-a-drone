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

/*! \file s1ap_common.c
   \brief s1ap procedures for both eNB and MME
   \author Sebastien ROUX <sebastien.roux@eurecom.fr>
   \date 2012
   \version 0.1
*/

#include <stdint.h>

#include "dynamic_memory_check.h"
#include "log.h"
#include "s1ap_common.h"

int asn_debug = 0;
int asn1_xer_print = 0;
// TODO: (amar) Unused function check with OAI
void s1ap_handle_criticality(S1AP_Criticality_t criticality) {}
