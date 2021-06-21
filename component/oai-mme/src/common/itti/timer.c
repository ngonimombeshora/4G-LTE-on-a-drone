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
#include "dynamic_memory_check.h"
#include "intertask_interface.h"
#include "log.h"
#include "queue.h"
#include "timer.h"

int timer_handle_signal(siginfo_t *info);

struct timer_elm_s {
  task_id_t task_id;  ///< Task ID which has requested the timer
  int32_t instance;   ///< Instance of the task which has requested the timer
  timer_t timer;      ///< Unique timer id
  timer_type_t type;  ///< Timer type
  void
      *timer_arg;  ///< Optional argument that will be passed when timer expires
  STAILQ_ENTRY(timer_elm_s) entries;  ///< Pointer to next element
};

typedef struct timer_desc_s {
  STAILQ_HEAD(timer_list_head, timer_elm_s) timer_queue;
  pthread_mutex_t timer_list_mutex;
  struct timespec timeout;
} timer_desc_t;

static timer_desc_t timer_desc;

#define TIMER_SEARCH(vAR, tIMERfIELD, tIMERvALUE, tIMERqUEUE) \
  do {                                                        \
    STAILQ_FOREACH(vAR, tIMERqUEUE, entries) {                \
      if (((vAR)->tIMERfIELD == tIMERvALUE)) break;           \
    }                                                         \
  } while (0)

int timer_handle_signal(siginfo_t *info) {
  struct timer_elm_s *timer_p;
  MessageDef *message_p;
  timer_has_expired_t *timer_expired_p;
  task_id_t task_id;
  int32_t instance;

  /*
   * Get back pointer to timer list element
   */
  timer_p = (struct timer_elm_s *)info->si_ptr;
  // LG: To many traces for msc timer:
  // TMR_DEBUG("Timer with id 0x%lx has expired", (long)timer_p->timer);
  task_id = timer_p->task_id;
  instance = timer_p->instance;
  message_p = itti_alloc_new_message(TASK_TIMER, TIMER_HAS_EXPIRED);
  timer_expired_p = &message_p->ittiMsg.timer_has_expired;
  timer_expired_p->timer_id = (long)timer_p->timer;
  timer_expired_p->arg = timer_p->timer_arg;

  /*
   * Timer is a one shot timer, remove it
   */
  if (timer_p->type == TIMER_ONE_SHOT) {
    //         if (timer_delete(timer_p->timer) < 0) {
    //             TMR_DEBUG("Failed to delete timer 0x%lx",
    //             (long)timer_p->timer);
    //         }
    //         TMR_DEBUG("Removed timer 0x%lx", (long)timer_p->timer);
    //         pthread_mutex_lock(&timer_desc.timer_list_mutex);
    //         STAILQ_REMOVE(&timer_desc.timer_queue, timer_p, timer_elm_s,
    //         entries); pthread_mutex_unlock(&timer_desc.timer_list_mutex);
    //         free_wrapper(timer_p);
    //         timer_p = NULL;
    if (timer_remove((long)timer_p->timer, NULL) != 0) {
      OAILOG_DEBUG(LOG_ITTI, "Failed to delete timer 0x%lx\n",
                   (long)timer_p->timer);
    }
  }

  /*
   * Notify task of timer expiry
   */
  if (abs(task_id) > TASK_MAX) {
    OAILOG_ERROR(
        LOG_ITTI,
        " TIMER OBJECT %x (with inner timer %x) task_id %d is invalid. \n",
        timer_p, timer_p->timer, timer_p->task_id);
    itti_free(TASK_TIMER, message_p);
    return -1;
  }
  if (itti_send_msg_to_task(task_id, instance, message_p) < 0) {
    OAILOG_DEBUG(LOG_ITTI, "Failed to send msg TIMER_HAS_EXPIRED to task %u\n",
                 task_id);
    itti_free(TASK_TIMER, message_p);
    return -1;
  }

  return 0;
}

int timer_setup(uint32_t interval_sec, uint32_t interval_us, task_id_t task_id,
                int32_t instance, timer_type_t type, void *timer_arg,
                long *timer_id) {
  struct sigevent se;
  struct itimerspec its;
  struct timer_elm_s *timer_p;
  timer_t timer;

  if (timer_id == NULL) {
    return -1;
  }

  AssertFatal(type < TIMER_TYPE_MAX, "Invalid timer type (%d/%d)!\n", type,
              TIMER_TYPE_MAX);
  /*
   * Allocate new timer list element
   */
  timer_p = malloc(sizeof(struct timer_elm_s));

  if (timer_p == NULL) {
    OAILOG_ERROR(LOG_ITTI, "Failed to create new timer element\n");
    return -1;
  }

  memset(&timer, 0, sizeof(timer_t));
  memset(&se, 0, sizeof(struct sigevent));
  timer_p->task_id = task_id;
  timer_p->instance = instance;
  timer_p->type = type;
  timer_p->timer_arg = timer_arg;
  /*
   * Setting up alarm
   */
  /*
   * Set and enable alarm
   */
  se.sigev_notify = SIGEV_SIGNAL;
  se.sigev_signo = SIGTIMER;
  se.sigev_value.sival_ptr = timer_p;

  /*
   * At the timer creation, the timer structure will be filled in with timer_id,
   * * * which is unique for this process. This id is allocated by kernel and
   * the
   * * * value might be used to distinguish timers.
   */
  if (timer_create(CLOCK_REALTIME, &se, &timer) < 0) {
    OAILOG_ERROR(LOG_ITTI, "Failed to create timer: (%s:%d)\n", strerror(errno),
                 errno);
    free_wrapper((void **)&timer_p);
    return -1;
  }

  /*
   * Fill in the first expiration value.
   */
  its.it_value.tv_sec = interval_sec;
  its.it_value.tv_nsec = interval_us * 1000;

  if (type == TIMER_PERIODIC) {
    /*
     * Asked for periodic timer. We set the interval time
     */
    its.it_interval.tv_sec = interval_sec;
    its.it_interval.tv_nsec = interval_us * 1000;
  } else {
    /*
     * Asked for one-shot timer. Do not set the interval field
     */
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
  }

  timer_settime(timer, 0, &its, NULL);
  /*
   * Simply set the timer_id argument. so it can be used by caller
   */
  *timer_id = (long)timer;
  OAILOG_TRACE(LOG_ITTI,
               "Requesting new %s timer with id 0x%lx that expires within "
               "%d sec and %d usec\n",
               type == TIMER_PERIODIC ? "periodic" : "single shot", *timer_id,
               interval_sec, interval_us);
  timer_p->timer = timer;
  /*
   * Lock the queue and insert the timer at the tail
   */
  pthread_mutex_lock(&timer_desc.timer_list_mutex);
  STAILQ_INSERT_TAIL(&timer_desc.timer_queue, timer_p, entries);
  pthread_mutex_unlock(&timer_desc.timer_list_mutex);
  return 0;
}

int timer_remove(long timer_id, void **arg) {
  int rc = 0;
  struct timer_elm_s *timer_p;

  OAILOG_TRACE(LOG_ITTI, "Removing timer 0x%lx\n", timer_id);
  pthread_mutex_lock(&timer_desc.timer_list_mutex);
  TIMER_SEARCH(timer_p, timer, ((timer_t)timer_id), &timer_desc.timer_queue);

  /*
   * We didn't find the timer in list
   */
  if (timer_p == NULL) {
    pthread_mutex_unlock(&timer_desc.timer_list_mutex);
    if (arg) *arg = NULL;
    OAILOG_WARNING(LOG_ITTI, "Didn't find timer 0x%lx in list\n", timer_id);
    return -1;
  }

  STAILQ_REMOVE(&timer_desc.timer_queue, timer_p, timer_elm_s, entries);
  pthread_mutex_unlock(&timer_desc.timer_list_mutex);

  // let user of API get back arg that can be an allocated memory (memory leak).

  if (arg) *arg = timer_p->timer_arg;
  if (timer_delete(timer_p->timer) < 0) {
    OAILOG_ERROR(LOG_ITTI, "Failed to delete timer 0x%lx\n",
                 (long)timer_p->timer);
    rc = -1;
  }

  OAILOG_TRACE(LOG_ITTI,
               "REMOVED TIMER OBJECT %x (inner timer %x) with task_id %d. \n",
               timer_p, timer_p->timer, timer_p->task_id);

  free_wrapper((void **)&timer_p);
  return rc;
}

int timer_init(void) {
  OAILOG_DEBUG(LOG_ITTI, "Initializing TIMER task interface\n");
  memset(&timer_desc, 0, sizeof(timer_desc_t));
  STAILQ_INIT(&timer_desc.timer_queue);
  pthread_mutex_init(&timer_desc.timer_list_mutex, NULL);
  OAILOG_DEBUG(LOG_ITTI, "Initializing TIMER task interface: DONE\n");
  return 0;
}
