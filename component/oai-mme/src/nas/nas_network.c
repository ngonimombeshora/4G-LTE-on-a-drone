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
  Source      nas_network.h

  Version     0.1

  Date        2012/09/20

  Product     NAS stack

  Subsystem   NAS main process

  Author      Frederic Maurel, Lionel GAUTHIER

  Description NAS procedure functions triggered by the network

*****************************************************************************/
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "bstrlib.h"

#include "log.h"
#include "mme_config.h"
#include "nas_network.h"

#include "../network/as_message.h"
#include "common_types.h"
#include "emm/nas_emm_proc.h"
#include "esm/nas_esm_proc.h"
#include "ies/TrackingAreaIdentity.h"
#include "util/nas_timer.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    nas_network_initialize()                                  **
 **                                                                        **
 ** Description: Initializes network internal data                         **
 **                                                                        **
 ** Inputs:  None                                                      **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void nas_network_initialize(mme_config_t* mme_config_p) {
  OAILOG_FUNC_IN(LOG_NAS);
  /*
   * Initialize the internal NAS processing data
   */
  nas_timer_init();
  nas_proc_initialize(mme_config_p);
  OAILOG_FUNC_OUT(LOG_NAS);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/
