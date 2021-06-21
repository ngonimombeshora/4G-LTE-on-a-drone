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
/*! \file gtpv1u.h
 * \brief
 * \author Sebastien ROUX, Lionel Gauthier
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr
 */

#ifndef FILE_GTPV1_U_SEEN
#define FILE_GTPV1_U_SEEN

#include <arpa/inet.h>
#include <net/if.h>
#include "3gpp_23.003.h"

#if ENABLE_OPENFLOW || ENABLE_OPENFLOW_MOSAIC
#include "pgw_pcef_emulation.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GTPU_HEADER_OVERHEAD_MAX 64

/*
 * This structure defines the management hooks for GTP tunnels.
 * The following hooks can be defined; unless noted otherwise, they are
 * optional and can be filled with a null pointer.
 *
 * int (*init)(struct in_addr *ue_net, uint32_t mask,
 *             int mtu, int *fd0, int *fd1u);
 *     This function is called when initializing GTP network device. How to use
 *     these input parameters are defined by the actual function
 * implementations.
 *         @ue_net: subnet assigned to UEs
 *         @mask: network mask for the UE subnet
 *         @mtu: MTU for the GTP network device.
 *         @fd0: socket file descriptor for GTPv0.
 *         @fd1u: socket file descriptor for GTPv1u.
 *
 * int (*uninit)(void);
 *     This function is called to destroy GTP network device.
 *
 * int (*reset)(void);
 *     This function is called to reset the GTP network device to clean state.
 *
 * int (*add_tunnel)(struct in_addr ue, struct in_addr enb, uint32_t i_tei,
 * uint32_t o_tei, imsi_t imsi); Add a gtp tunnel.
 *         @ue: UE IP address
 *         @enb: eNB IP address
 *         @i_tei: RX GTP Tunnel ID
 *         @o_tei: TX GTP Tunnel ID.
 *         @imsi: UE IMSI
 *
 * int (*del_tunnel)(uint32_t i_tei, uint32_t o_tei);
 *     Delete a gtp tunnel.
 *         @ue: UE IP address
 *         @i_tei: RX GTP Tunnel ID
 *         @o_tei: TX GTP Tunnel ID.
 */
struct gtp_tunnel_ops {
  int (*init)(struct in_addr* ue_net, struct in_addr* ue_netmask, int mtu,
              int* fd0, int* fd1u);
  int (*uninit)(void);
  int (*reset)(void);
#if ENABLE_LIBGTPNL
  int (*add_tunnel)(struct in_addr ue, struct in_addr enb, uint32_t i_tei,
                    uint32_t o_tei, uint8_t bearer_id);
  int (*del_tunnel)(struct in_addr ue, uint32_t i_tei, uint32_t o_tei);
#endif
#if ENABLE_OPENFLOW
  int (*add_tunnel)(struct in_addr ue, struct in_addr enb, uint32_t i_tei,
                    uint32_t o_tei, ebi_t ebi, imsi_t imsi,
                    const pcc_rule_t* const rule);
  int (*del_tunnel)(struct in_addr ue, uint32_t i_tei, uint32_t o_tei,
                    const pcc_rule_t* const rule);
#endif
#if ENABLE_OPENFLOW_MOSAIC
  int (*add_tunnel)(struct in_addr ue, struct in_addr enb, uint32_t i_tei,
                    uint32_t o_tei, imsi_t imsi);
  int (*del_tunnel)(struct in_addr ue, uint32_t i_tei, uint32_t o_tei);
#endif
};

uint32_t gtpv1u_new_teid(void);

const struct gtp_tunnel_ops* gtp_tunnel_ops_init(void);

#ifdef __cplusplus
}
#endif
#endif /* FILE_GTPV1_U_SEEN */
