///*
// * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
// * contributor license agreements.  See the NOTICE file distributed with
// * this work for additional information regarding copyright ownership.
// * The OpenAirInterface Software Alliance licenses this file to You under
// * the terms found in the LICENSE file in the root of this source tree.
// *
// * Unless required by applicable law or agreed to in writing, software
// * distributed under the License is distributed on an "AS IS" BASIS,
// * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// * See the License for the specific language governing permissions and
// * limitations under the License.
// *-------------------------------------------------------------------------------
// * For more information about the OpenAirInterface (OAI) Software Alliance:
// *      contact@openairinterface.org
// */
//
///*****************************************************************************
//  Source      emm_esm.c
//
//  Version     0.1
//
//  Date        2012/10/16
//
//  Product     NAS stack
//
//  Subsystem   EPS Mobility Management
//
//  Author      Frederic Maurel
//
//  Description Defines the EMMESM Service Access Point that provides
//        interlayer services to the EPS Session Management sublayer
//        for service registration and activate/deactivate PDN
//        connections.
//
//*****************************************************************************/
//#include "../../nas_emm/sap/emm_esm.h"
//
//#include <pthread.h>
//#include <inttypes.h>
//#include <stdint.h>
//#include <stdbool.h>
//#include <string.h>
//#include <stdlib.h>
//#include <arpa/inet.h>
//
//#include "../../nas_emm/LowerLayer.h"
//#include "bstrlib.h"
//
//#include "3gpp_24.007.h"
//#include "3gpp_24.008.h"
//#include "common_defs.h"
//#include "common_defs.h"
//#include "log.h"
//
//
///****************************************************************************/
///****************  E X T E R N A L    D E F I N I T I O N S  ****************/
///****************************************************************************/
//
///****************************************************************************/
///*******************  L O C A L    D E F I N I T I O N S  *******************/
///****************************************************************************/
//
///*
//   String representation of EMMESM-SAP primitives
//*/
// static const char                      *_emm_esm_primitive_str[] = {
//  "EMMESM_RELEASE_IND",
//  "EMMESM_UNITDATA_REQ",
//  "EMMESM_ACTIVATE_BEARER_REQ",
//  "EMMESM_MODIFY_BEARER_REQ",
//  "EMMESM_DEACTIVATE_BEARER_REQ",
//  "EMMESM_UNITDATA_IND",
//};
//
///****************************************************************************/
///******************  E X P O R T E D    F U N C T I O N S  ******************/
///****************************************************************************/

///****************************************************************************
// **                                                                        **
// ** Name:    emm_esm_send()                                            **
// **                                                                        **
// ** Description: Processes the EMMESM Service Access Point primitive       **
// **                                                                        **
// ** Inputs:  msg:       The EMMESM-SAP primitive to process        **
// **      Others:    None                                       **
// **                                                                        **
// ** Outputs:     None                                                      **
// **      Return:    RETURNok, RETURNerror                      **
// **      Others:    None                                       **
// **                                                                        **
// ***************************************************************************/

//  case _EMMESM_ACTIVATE_BEARER_REQ:
//    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_ESM_MME, NULL, 0,
//    "_EMMESM_ACTIVATE_BEARER_REQ ue id " MME_UE_S1AP_ID_FMT " ", msg->ue_id);
//    rc = lowerlayer_activate_bearer_req (msg->ue_id,
//    msg->u.activate_bearer.ebi,
//        msg->u.activate_bearer.mbr_dl,
//        msg->u.activate_bearer.mbr_ul,
//        msg->u.activate_bearer.gbr_dl,
//        msg->u.activate_bearer.gbr_ul,
//        msg->u.activate_bearer.msg);
//    break;
//
//  case _EMMESM_MODIFY_BEARER_REQ:
//    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_ESM_MME, NULL, 0,
//    "_EMMESM_MODIFY_BEARER_REQ ue id " MME_UE_S1AP_ID_FMT " ", msg->ue_id); rc
//    = lowerlayer_modify_bearer_req (msg->ue_id, msg->u.modify_bearer.ebi,
//        msg->u.modify_bearer.mbr_dl,
//        msg->u.modify_bearer.mbr_ul,
//        msg->u.modify_bearer.gbr_dl,
//        msg->u.modify_bearer.gbr_ul,
//        msg->u.modify_bearer.msg);
//    break;
//
//  case _EMMESM_DEACTIVATE_BEARER_REQ:
//    MSC_LOG_RX_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_ESM_MME, NULL, 0,
//    "_EMMESM_DEACTIVATE_BEARER_REQ ue id " MME_UE_S1AP_ID_FMT " ",
//    msg->ue_id); rc = lowerlayer_deactivate_bearer_req (msg->ue_id,
//    msg->u.deactivate_bearer.ebi,
//        msg->u.deactivate_bearer.msg);
//    break;
//
//  default:
//    break;
//  }
//
//  if (rc != RETURNok) {
//    MSC_LOG_RX_DISCARDED_MESSAGE (MSC_NAS_EMM_MME, MSC_NAS_EMM_MME, NULL, 0,
//    "_EMMESM_UNKNOWN(primitive id %d) ue id " MME_UE_S1AP_ID_FMT " ",
//    primitive, msg->ue_id); OAILOG_WARNING (LOG_NAS_EMM, "EMMESM-SAP - Failed
//    to process primitive %s (%d)\n", _emm_esm_primitive_str[primitive -
//    _EMMESM_START - 1], primitive);
//  }
//
//  OAILOG_FUNC_RETURN (LOG_NAS_EMM, rc);
//}
//
///****************************************************************************/
///*********************  L O C A L    F U N C T I O N S  *********************/
///****************************************************************************/
