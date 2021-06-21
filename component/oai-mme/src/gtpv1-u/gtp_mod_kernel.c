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

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <libgtpnl/gtp.h>
#include <libgtpnl/gtpnl.h>
#include <libmnl/libmnl.h>
#include "bstrlib.h"

#include "3gpp_24.008.h"
#include "3gpp_29.274.h"
#include "common_defs.h"
#include "common_types.h"
#include "dynamic_memory_check.h"
#include "gtp_mod_kernel.h"
#include "gtpv1u_sgw_defs.h"
#include "log.h"
#include "pgw_pcef_emulation.h"
#include "spgw_config.h"

static struct {
  int genl_id;
  struct mnl_socket *nl;
  bool is_enabled;
} gtp_nl;

#define GTP_DEVNAME "gtp0"

//------------------------------------------------------------------------------
int gtp_mod_kernel_init(int *fd0, int *fd1u, struct in_addr *ue_net, int mask,
                        int gtp_dev_mtu) {
#if !GTP_KERNEL_MODULE_UNAVAILABLE
  // we don't need GTP v0, but interface with kernel requires 2 file descriptors
  *fd0 = socket(AF_INET, SOCK_DGRAM, 0);
  *fd1u = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in sockaddr_fd0 = {
      .sin_family = AF_INET,
      .sin_port = htons(3386),
      .sin_addr =
          {
              .s_addr = INADDR_ANY,
          },
  };
  struct sockaddr_in sockaddr_fd1 = {
      .sin_family = AF_INET,
      .sin_port = htons(GTPV1U_UDP_PORT),
      .sin_addr =
          {
              .s_addr = INADDR_ANY,
          },
  };

  if (bind(*fd0, (struct sockaddr *)&sockaddr_fd0, sizeof(sockaddr_fd0)) < 0) {
    OAILOG_ERROR(LOG_GTPV1U, "bind GTPv0 port");
    return RETURNerror;
  }
  if (bind(*fd1u, (struct sockaddr *)&sockaddr_fd1, sizeof(sockaddr_fd1)) < 0) {
    OAILOG_ERROR(LOG_GTPV1U, "bind S1U port");
    return RETURNerror;
  }

  if (gtp_dev_create(-1, GTP_DEVNAME, *fd0, *fd1u) < 0) {
    OAILOG_ERROR(LOG_GTPV1U, "Cannot create GTP tunnel device: %s\n",
                 strerror(errno));
    return RETURNerror;
  }
  gtp_nl.is_enabled = true;

  gtp_nl.nl = genl_socket_open();
  if (gtp_nl.nl == NULL) {
    OAILOG_ERROR(LOG_GTPV1U, "Cannot create genetlink socket\n");
    return RETURNerror;
  }
  gtp_nl.genl_id = genl_lookup_family(gtp_nl.nl, "gtp");
  if (gtp_nl.genl_id < 0) {
    OAILOG_ERROR(LOG_GTPV1U, "Cannot lookup GTP genetlink ID\n");
    return RETURNerror;
  }
  OAILOG_NOTICE(LOG_GTPV1U, "Using the GTP kernel mode (genl ID is %d)\n",
                gtp_nl.genl_id);

  bstring system_cmd =
      bformat("ip link set dev %s mtu %u", GTP_DEVNAME, gtp_dev_mtu);
  int ret = system((const char *)system_cmd->data);
  if (ret) {
    OAILOG_ERROR(LOG_GTPV1U, "ERROR in system command %s: %d at %s:%u\n",
                 bdata(system_cmd), ret, __FILE__, __LINE__);
    bdestroy_wrapper(&system_cmd);
    return RETURNerror;
  }
  bdestroy_wrapper(&system_cmd);

  struct in_addr ue_gw;
  ue_gw.s_addr = ue_net->s_addr | htonl(1);
  system_cmd =
      bformat("ip addr add %s/%u dev %s", inet_ntoa(ue_gw), mask, GTP_DEVNAME);
  ret = system((const char *)system_cmd->data);
  if (ret) {
    OAILOG_ERROR(LOG_GTPV1U, "ERROR in system command %s: %d at %s:%u\n",
                 bdata(system_cmd), ret, __FILE__, __LINE__);
    bdestroy_wrapper(&system_cmd);
    return RETURNerror;
  }
  bdestroy_wrapper(&system_cmd);

  OAILOG_DEBUG(LOG_GTPV1U, "Setting route to reach UE net %s via %s\n",
               inet_ntoa(*ue_net), GTP_DEVNAME);

  if (gtp_dev_config(GTP_DEVNAME, ue_net, mask) < 0) {
    OAILOG_ERROR(LOG_GTPV1U, "Cannot add route to reach network\n");
    return RETURNerror;
  }

  OAILOG_NOTICE(LOG_GTPV1U, "GTP kernel configured\n");
#endif
  return RETURNok;
}

//------------------------------------------------------------------------------
void gtp_mod_kernel_stop(void) {
#if !GTP_KERNEL_MODULE_UNAVAILABLE
  if (!gtp_nl.is_enabled) return;

  gtp_dev_destroy(GTP_DEVNAME);
#endif
}

//------------------------------------------------------------------------------
int gtp_mod_kernel_tunnel_add(struct in_addr ue, struct in_addr enb,
                              uint32_t i_tei, uint32_t o_tei,
                              uint8_t bearer_id) {
  int ret = RETURNok;
#if !GTP_KERNEL_MODULE_UNAVAILABLE
  struct gtp_tunnel *t;

  if (!gtp_nl.is_enabled) return RETURNok;

  t = gtp_tunnel_alloc();
  if (t == NULL) return RETURNerror;

  gtp_tunnel_set_ifidx(t, if_nametoindex(GTP_DEVNAME));
  gtp_tunnel_set_version(t, 1);
  gtp_tunnel_set_ms_ip4(t, &ue);
  gtp_tunnel_set_sgsn_ip4(t, &enb);
  gtp_tunnel_set_i_tei(t, i_tei);
  gtp_tunnel_set_o_tei(t, o_tei);
  gtp_tunnel_set_bearer_id(t, bearer_id);
  ret = gtp_add_tunnel(gtp_nl.genl_id, gtp_nl.nl, t);
  gtp_tunnel_free(t);
#endif
  return ret;
}

//------------------------------------------------------------------------------
int gtp_mod_kernel_tunnel_del(uint32_t i_tei, uint32_t o_tei) {
  int ret = RETURNok;
#if !GTP_KERNEL_MODULE_UNAVAILABLE
  struct gtp_tunnel *t;

  if (!gtp_nl.is_enabled) return RETURNok;

  t = gtp_tunnel_alloc();
  if (t == NULL) return RETURNerror;

  gtp_tunnel_set_ifidx(t, if_nametoindex(GTP_DEVNAME));
  gtp_tunnel_set_version(t, 1);
  // looking at kernel/drivers/net/gtp.c: not needed gtp_tunnel_set_ms_ip4(t,
  // &ue); looking at kernel/drivers/net/gtp.c: not needed
  // gtp_tunnel_set_sgsn_ip4(t, &enb);
  gtp_tunnel_set_i_tei(t, i_tei);
  gtp_tunnel_set_o_tei(t, o_tei);

  ret = gtp_del_tunnel(gtp_nl.genl_id, gtp_nl.nl, t);
  gtp_tunnel_free(t);

#endif
  return ret;
}

//------------------------------------------------------------------------------
bool gtp_mod_kernel_enabled(void) { return gtp_nl.is_enabled; }
