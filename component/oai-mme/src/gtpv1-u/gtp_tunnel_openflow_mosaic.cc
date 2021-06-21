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

#include "3gpp_23.003.h"
#include "assertions.h"
#include "common_defs.h"
#include "conversions.h"
#include "gtp_controller.h"
#include "gtpv1u.h"
#include "log.h"

extern struct gtp_tunnel_ops gtp_tunnel_ops;

int openflow_uninit(void) {
  int ret;
  if ((ret = stop_of_controller()) >= 0) {
    OAILOG_ERROR(LOG_GTPV1U, "Could not stop openflow controller on uninit\n");
  }
  return ret;
}

int openflow_init(struct in_addr *ue_net, uint32_t mask, int mtu, int *fd0,
                  int *fd1u) {
  AssertFatal(start_of_controller() >= 0,
              "Could not start openflow controller\n");
  return 0;
}

int openflow_reset(void) {
  int rv = 0;
  return rv;
}

int openflow_add_tunnel(struct in_addr ue, struct in_addr enb, uint32_t i_tei,
                        uint32_t o_tei, imsi_t imsi) {
  char *imsi_str[IMSI_BCD_DIGITS_MAX + 1] = {0};
  IMSI_TO_STRING((&imsi), imsi_str, IMSI_BCD_DIGITS_MAX + 1);

  return openflow_controller_add_gtp_tunnel(ue, enb, i_tei, o_tei,
                                            (const char *)imsi_str);
}

int openflow_del_tunnel(struct in_addr ue, uint32_t i_tei, uint32_t o_tei) {
  return openflow_controller_del_gtp_tunnel(ue, i_tei);
}

static const struct gtp_tunnel_ops openflow_ops = {
    .init = openflow_init,
    .uninit = openflow_uninit,
    .reset = openflow_reset,
    .add_tunnel = openflow_add_tunnel,
    .del_tunnel = openflow_del_tunnel,
};

const struct gtp_tunnel_ops *gtp_tunnel_ops_init(void) {
  OAILOG_DEBUG(LOG_GTPV1U, "Initializing gtp_tunnel_ops_openflow\n");
  return &openflow_ops;
}
