/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "S1AP-IEs"
 * 	found in "S1AP-IEs.asn"
 */

#ifndef _S1ap_ThresholdEventA2_H_
#define _S1ap_ThresholdEventA2_H_

#include <asn_application.h>

/* Including external dependencies */
#include <constr_SEQUENCE.h>
#include "S1ap-MeasurementThresholdA2.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct S1ap_IE_Extensions;

/* S1ap-ThresholdEventA2 */
typedef struct S1ap_ThresholdEventA2 {
  S1ap_MeasurementThresholdA2_t measurementThreshold;
  struct S1ap_IE_Extensions* iE_Extensions /* OPTIONAL */;
  /*
   * This type is extensible,
   * possible extensions are below.
   */

  /* Context for parsing across buffer boundaries */
  asn_struct_ctx_t _asn_ctx;
} S1ap_ThresholdEventA2_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_S1ap_ThresholdEventA2;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "S1ap-IE-Extensions.h"

#endif /* _S1ap_ThresholdEventA2_H_ */
#include <asn_internal.h>