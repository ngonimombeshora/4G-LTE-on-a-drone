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

/*! \file mme_app_edns_emulation.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bstrlib.h"

#include "common_defs.h"
#include "dynamic_memory_check.h"
#include "log.h"
#include "mme_app_edns_emulation.h"
#include "mme_config.h"
#include "obj_hashtable.h"

static obj_hash_table_t *sgw_e_dns_entries = NULL;
static obj_hash_table_t *mme_e_dns_entries = NULL;

//------------------------------------------------------------------------------
void mme_app_edns_get_wrr_entry(bstring id,
                                const interface_type_t interface_type,
                                struct sockaddr **sockaddr) {
  switch (interface_type) {
    case S10_MME_GTP_C:
      obj_hashtable_get(mme_e_dns_entries, bdata(id), blength(id),
                        (void **)sockaddr);
      return;
    case S11_SGW_GTP_C:
      obj_hashtable_get(sgw_e_dns_entries, bdata(id), blength(id),
                        (void **)sockaddr);
      return;
    default:
      return;
  }
}

//------------------------------------------------------------------------------
int mme_app_edns_add_wrr_entry(bstring id, struct sockaddr *edns_ip_addr,
                               const interface_type_t interface_type) {
  char *cid = calloc(1, blength(id) + 1);
  if (cid) {
    strncpy(cid, (const char *)id->data, blength(id));

    struct sockaddr_in *data;
    if (edns_ip_addr->sa_family == AF_INET) {
      data = malloc(sizeof(struct sockaddr_in));
      memcpy((void *)data, (struct sockaddr_in *)edns_ip_addr,
             sizeof(struct sockaddr_in));
    } else if (edns_ip_addr->sa_family == AF_INET6) {
      data = malloc(sizeof(struct sockaddr_in6));
      memcpy((void *)data, (struct sockaddr_in6 *)edns_ip_addr,
             sizeof(struct sockaddr_in6));
    } else {
      OAILOG_DEBUG(LOG_MME_APP, "Unknown socket address family %d",
                   edns_ip_addr->sa_family);
      free_wrapper(&cid);
      return RETURNerror;
    }
    if (data) {
      hashtable_rc_t rc;
      switch (interface_type) {
        case S10_MME_GTP_C:
          rc = obj_hashtable_insert(mme_e_dns_entries, cid, strlen(cid), data);
          break;
        case S11_SGW_GTP_C:
          rc = obj_hashtable_insert(sgw_e_dns_entries, cid, strlen(cid), data);
          break;
        default:
          OAILOG_DEBUG(LOG_MME_APP, "Unknown interface type %d",
                       interface_type);
          return RETURNerror;
      }
      /** Key is copied inside. */
      free_wrapper(&cid);
      if (HASH_TABLE_OK == rc) return RETURNok;
    }
    if (cid) free_wrapper(&cid);
  }
  return RETURNerror;
}

//------------------------------------------------------------------------------
int mme_app_edns_init(const mme_config_t *mme_config_p) {
  int rc = RETURNok;
  sgw_e_dns_entries = obj_hashtable_create(
      min(64, MME_CONFIG_MAX_SERVICE), NULL, free_wrapper, free_wrapper, NULL);
  mme_e_dns_entries = obj_hashtable_create(
      min(64, MME_CONFIG_MAX_SERVICE), NULL, free_wrapper, free_wrapper, NULL);
  if (sgw_e_dns_entries && mme_e_dns_entries) {
    /** Add the service (s10 or s11). */
    for (int i = 0; i < mme_config_p->e_dns_emulation.nb_service_entries; i++) {
      rc |= mme_app_edns_add_wrr_entry(
          mme_config_p->e_dns_emulation.service_id[i],
          &mme_config_p->e_dns_emulation.sockaddr[i],
          mme_config_p->e_dns_emulation.interface_type[i]);
    }
    //    /** Add the neighboring MMEs. */
    //    for (int i = 0; i < mme_config_p->e_dns_emulation.nb_mme_entries; i++)
    //    {
    //      rc |=
    //      mme_app_edns_add_mme_entry(mme_config_p->e_dns_emulation.mme_id[i],
    //      mme_config_p->e_dns_emulation.mme_ip_addr[i]);
    //    }
    OAILOG_DEBUG(LOG_MME_APP, "Failed to populate eDNS");
    return rc;
  }
  OAILOG_DEBUG(LOG_MME_APP, "Failed to create eDNS hashtables");
  return RETURNerror;
}

//------------------------------------------------------------------------------
void mme_app_edns_exit(void) {
  obj_hashtable_destroy(sgw_e_dns_entries);
  obj_hashtable_destroy(mme_e_dns_entries);
}
