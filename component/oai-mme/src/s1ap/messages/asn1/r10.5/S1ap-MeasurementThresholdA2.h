/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "S1AP-IEs"
 * 	found in "S1AP-IEs.asn"
 */

#ifndef _S1ap_MeasurementThresholdA2_H_
#define _S1ap_MeasurementThresholdA2_H_

#include <asn_application.h>

/* Including external dependencies */
#include <constr_CHOICE.h>
#include "S1ap-Threshold-RSRP.h"
#include "S1ap-Threshold-RSRQ.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum S1ap_MeasurementThresholdA2_PR {
  S1ap_MeasurementThresholdA2_PR_NOTHING, /* No components present */
  S1ap_MeasurementThresholdA2_PR_threshold_RSRP,
  S1ap_MeasurementThresholdA2_PR_threshold_RSRQ,
  /* Extensions may appear below */

} S1ap_MeasurementThresholdA2_PR;

/* S1ap-MeasurementThresholdA2 */
typedef struct S1ap_MeasurementThresholdA2 {
  S1ap_MeasurementThresholdA2_PR present;
  union S1ap_MeasurementThresholdA2_u {
    S1ap_Threshold_RSRP_t threshold_RSRP;
    S1ap_Threshold_RSRQ_t threshold_RSRQ;
    /*
     * This type is extensible,
     * possible extensions are below.
     */
  } choice;

  /* Context for parsing across buffer boundaries */
  asn_struct_ctx_t _asn_ctx;
} S1ap_MeasurementThresholdA2_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_S1ap_MeasurementThresholdA2;

#ifdef __cplusplus
}
#endif

#endif /* _S1ap_MeasurementThresholdA2_H_ */
#include <asn_internal.h>
