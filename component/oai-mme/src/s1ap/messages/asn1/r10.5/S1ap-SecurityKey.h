/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "S1AP-IEs"
 * 	found in "S1AP-IEs.asn"
 */

#ifndef _S1ap_SecurityKey_H_
#define _S1ap_SecurityKey_H_

#include <asn_application.h>

/* Including external dependencies */
#include <BIT_STRING.h>

#ifdef __cplusplus
extern "C" {
#endif

/* S1ap-SecurityKey */
typedef BIT_STRING_t S1ap_SecurityKey_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_S1ap_SecurityKey;
asn_struct_free_f S1ap_SecurityKey_free;
asn_struct_print_f S1ap_SecurityKey_print;
asn_constr_check_f S1ap_SecurityKey_constraint;
ber_type_decoder_f S1ap_SecurityKey_decode_ber;
der_type_encoder_f S1ap_SecurityKey_encode_der;
xer_type_decoder_f S1ap_SecurityKey_decode_xer;
xer_type_encoder_f S1ap_SecurityKey_encode_xer;

#ifdef __cplusplus
}
#endif

#endif /* _S1ap_SecurityKey_H_ */
#include <asn_internal.h>