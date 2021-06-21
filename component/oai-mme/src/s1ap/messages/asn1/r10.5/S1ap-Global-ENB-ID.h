/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "S1AP-IEs"
 * 	found in "S1AP-IEs.asn"
 */

#ifndef _S1ap_Global_ENB_ID_H_
#define _S1ap_Global_ENB_ID_H_

#include <asn_application.h>

/* Including external dependencies */
#include <constr_SEQUENCE.h>
#include "S1ap-ENB-ID.h"
#include "S1ap-PLMNidentity.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct S1ap_IE_Extensions;

/* S1ap-Global-ENB-ID */
typedef struct S1ap_Global_ENB_ID {
  S1ap_PLMNidentity_t pLMNidentity;
  S1ap_ENB_ID_t eNB_ID;
  struct S1ap_IE_Extensions* iE_Extensions /* OPTIONAL */;
  /*
   * This type is extensible,
   * possible extensions are below.
   */

  /* Context for parsing across buffer boundaries */
  asn_struct_ctx_t _asn_ctx;
} S1ap_Global_ENB_ID_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_S1ap_Global_ENB_ID;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "S1ap-IE-Extensions.h"

#endif /* _S1ap_Global_ENB_ID_H_ */
#include <asn_internal.h>
