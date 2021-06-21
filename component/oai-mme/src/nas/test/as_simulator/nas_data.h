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
      Eurecom OpenAirInterface 3
      Copyright(c) 2012 Eurecom

Source    nas_data.h

Version   0.1

Date    2013/04/11

Product   Access-Stratum sublayer simulator

Subsystem Non-Access-Stratum data

Author    Frederic Maurel

Description Defines constants and functions used by the AS simulator
    process.

*****************************************************************************/

#ifndef __NAS_DATA_H__
#define __NAS_DATA_H__

#include "../../ies/DetachType.h"
#include "../../ies/EmmCause.h"
#include "../../ies/EpsAttachType.h"
#include "../../ies/EpsMobileIdentity.h"
#include "../../ies/EsmCause.h"
#include "../../ies/NasKeySetIdentifier.h"
#include "../../ies/NasRequestType.h"
#include "../../ies/NasSecurityAlgorithms.h"
#include "../../ies/PdnAddress.h"
#include "../../ies/PdnType.h"
#include "../../ies/TrackingAreaIdentityList.h"
#include "GprsTimer.h"
#include "IdentityType2.h"
#include "MobileIdentity.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

const char* emmMsgType(int type);
const char* esmMsgType(int type);

const char* emmCauseCode(emm_cause_t code);
const char* esmCauseCode(esm_cause_t code);

const char* attachType(const eps_attach_type_t* type);
const char* detachType(const detach_type_t* type);

ssize_t epsIdentity(char* buffer, size_t len, const EpsMobileIdentity* ident);
const char* identityType(const IdentityType2* type);
ssize_t mobileIdentity(char* buffer, size_t len, const MobileIdentity* ident);

const char* requestType(const request_type_t* type);
const char* pdnType(const PdnType* type);
ssize_t pdnAddress(char* buffer, size_t len, const PdnAddress* addr);

ssize_t nasKeySetIdentifier(char* buffer, size_t len,
                            const NasKeySetIdentifier* ksi);
ssize_t authenticationParameter(char* buffer, size_t len,
                                const OctetString* param);
const char* nasCipheringAlgorithm(const NasSecurityAlgorithms* algo);
const char* nasIntegrityAlgorithm(const NasSecurityAlgorithms* algo);

ssize_t gprsTimer(char* buffer, size_t len, const GprsTimer* timer);
ssize_t taiList(char* buffer, size_t len, const TrackingAreaIdentityList* tai);

#endif  // __NAS_DATA_H__
