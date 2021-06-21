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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bstrlib.h"

#include "assertions.h"
#include "backtrace.h"
#include "intertask_interface.h"
#include "timer.h"

#include "log.h"
#include "signals.h"

#if defined(LOG_D) && defined(LOG_E)
#define SIG_DEBUG(x, args...) OAILOG_D(EMU, x, ##args)
#define SIG_ERROR(x, args...) OAILOG_E(EMU, x, ##args)
#endif

#ifndef SIG_DEBUG
#define SIG_DEBUG(x, args...)              \
  do {                                     \
    fprintf(stdout, "[SIG][D]" x, ##args); \
  } while (0)
#endif
#ifndef SIG_ERROR
#define SIG_ERROR(x, args...)              \
  do {                                     \
    fprintf(stdout, "[SIG][E]" x, ##args); \
  } while (0)
#endif

static sigset_t set;

int signal_mask(void) {
  /*
   * We set the signal mask to avoid threads other than the main thread
   * * * to receive the timer signal. Note that threads created will inherit
   * this
   * * * configuration.
   */
  sigemptyset(&set);
  sigaddset(&set, SIGTIMER);
  sigaddset(&set, SIGUSR1);
  sigaddset(&set, SIGABRT);
  sigaddset(&set, SIGSEGV);
  sigaddset(&set, SIGINT);

  if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
    perror("sigprocmask");
    return -1;
  }

  return 0;
}

int signal_handle(int *end) {
  int ret;
  siginfo_t info;

  sigemptyset(&set);
  sigaddset(&set, SIGTIMER);
  sigaddset(&set, SIGUSR1);
  sigaddset(&set, SIGABRT);
  sigaddset(&set, SIGSEGV);
  sigaddset(&set, SIGINT);

  if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
    perror("sigprocmask");
    return -1;
  }

  /*
   * Block till a signal is received.
   * * * NOTE: The signals defined by set are required to be blocked at the time
   * * * of the call to sigwait() otherwise sigwait() is not successful.
   */
  if ((ret = sigwaitinfo(&set, &info)) == -1) {
    perror("sigwait");
    return ret;
  }
  // printf("Received signal %d\n", info.si_signo);

  /*
   * Real-time signals are non constant and are therefore not suitable for
   * * * use in switch.
   */
  if (info.si_signo == SIGTIMER) {
    timer_handle_signal(&info);
  } else {
    /*
     * Dispatch the signal to sub-handlers
     */
    switch (info.si_signo) {
      case SIGUSR1:
        SIG_DEBUG("Received SIGUSR1\n");
        *end = 1;
        break;

      case SIGSEGV: /* Fall through */
      case SIGABRT:
        SIG_DEBUG("Received SIGABORT\n");
        backtrace_handle_signal(&info);
        break;

      case SIGINT:
        printf("Received SIGINT\n");
        itti_send_terminate_message(TASK_UNKNOWN);
        *end = 1;
        break;

      default:
        SIG_ERROR("Received unknown signal %d\n", info.si_signo);
        break;
    }
  }

  return 0;
}
