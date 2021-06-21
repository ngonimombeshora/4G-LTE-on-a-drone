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

/*! \file sctp_primitives_server.c
 *  \brief Main server primitives
 *  \author Sebastien ROUX, Lionel GAUTHIER
 *  \date 2013
 *  \version 1.0
 *  @ingroup _sctp
 *  @{
 */

#if !defined(HAVE_LIBSCTP)
#error "You must install libsctp-dev"
#endif

#ifndef FILE_SCTP_PRIMITIVES_SERVER_SEEN
#define FILE_SCTP_PRIMITIVES_SERVER_SEEN
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "mme_config.h"

/** \brief SCTP data received callback
 \param buffer pointer to buffer received
 \param length pointer to the length of buffer
 **/
typedef void (*sctp_recv_callback)(uint8_t* buffer, uint32_t length);

/** \brief SCTP Init function. Initialize SCTP layer
 \param mme_config The global MME configuration structure
 @returns -1 on error, 0 otherwise.
 **/
struct mme_config_s;
int sctp_init(const struct mme_config_s* mme_config_p);

#endif /* FILE_SCTP_PRIMITIVES_SERVER_SEEN */

/* @} */
