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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/resource.h>
#include <sys/time.h>

#include <sched.h>

#include "gtpv1u_sgw_defs.h"
#include "mme_config.h"

#include "intertask_interface_init.h"

#include "ipv4_defs.h"
#include "log.h"
#include "s1ap_mme.h"
#include "sctp_primitives_server.h"
#include "sgw_lite_defs.h"
#include "timer.h"
#include "udp_primitives_server.h"

int main(int argc, char *argv[]) {
  int i = 0;
  int prio = 0;
  mme_config_t mme_config;
  MessageDef *message_p;

  struct sched_param param = {
      .sched_priority = 10,
  };
  config_parse_opt_line(argc, argv, &mme_config);
  fprintf(stdout, "Starting %s ITTI test\n", PACKAGE_STRING);

  if (setpriority(PRIO_PROCESS, 0, prio) < 0) {
    fprintf(stderr,
            "Cannot assign requested prio: %d\n"
            "%d:%s\n",
            prio, errno, strerror(errno));
    return -1;
  }

  if (sched_setscheduler(0, SCHED_RR, &param) < 0) {
    fprintf(stderr,
            "Cannot assign requested scheduler policy\n"
            "%d:%s\n",
            errno, strerror(errno));
    return -1;
  }

  /*
   * Calling each layer init function
   */
  log_init(&mme_config);
  itti_init(TASK_MAX, THREAD_MAX, MESSAGES_ID_MAX, tasks_info, messages_info,
            messages_definition_xml, NULL);
  sctp_init(&mme_config);
  udp_init(&mme_config);
  s1ap_mme_init(&mme_config);
  gtpv1u_init(&mme_config);
  ipv4_init(&mme_config);
  sgw_lite_init(&mme_config);
  message_p = itti_alloc_new_message(TASK_S1AP, MESSAGE_TEST);

  while (i < (1 << 15)) {
    if (send_broadcast_message(message_p) < 0) {
      fprintf(stderr, "Failed to send broadcast message %d\n", i);
    }

    i++;
  }

  fprintf(stderr, "Successfully sent %lu messages",
          get_current_message_number());
  return 0;
}
