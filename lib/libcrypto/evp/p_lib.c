/* $OpenBSD: p_lib.c,v 1.54 2024/01/04 17:01:26 tb Exp $ */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 *
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 *
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 *
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */
/* ====================================================================
 * Copyright (c) 2006 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.OpenSSL.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    licensing@OpenSSL.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.OpenSSL.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/cmac.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509.h>

#ifndef OPENSSL_NO_DH
#include <openssl/dh.h>
#endif
#ifndef OPENSSL_NO_DSA
#include <openssl/dsa.h>
#endif
#ifndef OPENSSL_NO_EC
#include <openssl/ec.h>
#endif
#ifndef OPENSSL_NO_RSA
#include <openssl/rsa.h>
#endif

#include "evp_local.h"

extern const EVP_PKEY_ASN1_METHOD cmac_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD dh_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD dsa_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD dsa1_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD dsa2_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD dsa3_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD dsa4_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD eckey_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD ed25519_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD gostimit_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD gostr01_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD gostr12_256_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD gostr12_512_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD hmac_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD rsa_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD rsa2_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD rsa_pss_asn1_meth;
extern const EVP_PKEY_ASN1_METHOD x25519_asn1_meth;

static const EVP_PKEY_ASN1_METHOD *asn1_methods[] = {
	&cmac_asn1_meth,
	&dh_asn1_meth,
	&dsa_asn1_meth,
	&dsa1_asn1_meth,
	&dsa2_asn1_meth,
	&dsa3_asn1_meth,
	&dsa4_asn1_meth,
	&eckey_asn1_meth,
	&ed25519_asn1_meth,
	&gostimit_asn1_meth,
	&gostr01_asn1_meth,
	&gostr12_256_asn1_meth,
	&gostr12_512_asn1_meth,
	&hmac_asn1_meth,
	&rsa_asn1_meth,
	&rsa2_asn1_meth,
	&rsa_pss_asn1_meth,
	&x25519_asn1_meth,
};

#define N_ASN1_METHODS (sizeof(asn1_methods) / sizeof(asn1_methods[0]))

int
EVP_PKEY_asn1_get_count(void)
{
	return N_ASN1_METHODS;
}

const EVP_PKEY_ASN1_METHOD *
EVP_PKEY_asn1_get0(int idx)
{
	if (idx < 0 || idx >= N_ASN1_METHODS)
		return NULL;

	return asn1_methods[idx];
}

static const EVP_PKEY_ASN1_METHOD *
pkey_asn1_find(int pkey_id)
{
	const EVP_PKEY_ASN1_METHOD *ameth;
	int i;

	for (i = EVP_PKEY_asn1_get_count() - 1; i >= 0; i--) {
		ameth = EVP_PKEY_asn1_get0(i);
		if (ameth->pkey_id == pkey_id)
			return ameth;
	}

	return NULL;
}

/*
 * XXX - fix this. In what looks like an infinite loop, this API only makes two
 * calls to pkey_asn1_find(): If the type resolves to an aliased ASN.1 method,
 * the second call will find the method it aliases. Codify this in regress and
 * make this explicit in code.
 */
const EVP_PKEY_ASN1_METHOD *
EVP_PKEY_asn1_find(ENGINE **pe, int type)
{
	const EVP_PKEY_ASN1_METHOD *mp;

	if (pe != NULL)
		*pe = NULL;

	for (;;) {
		if ((mp = pkey_asn1_find(type)) == NULL)
			break;
		if ((mp->pkey_flags & ASN1_PKEY_ALIAS) == 0)
			break;
		type = mp->base_method->pkey_id;
	}

	return mp;
}

const EVP_PKEY_ASN1_METHOD *
EVP_PKEY_asn1_find_str(ENGINE **pe, const char *str, int len)
{
	const EVP_PKEY_ASN1_METHOD *ameth;
	int i;

	if (len == -1)
		len = strlen(str);
	if (pe != NULL)
		*pe = NULL;
	for (i = EVP_PKEY_asn1_get_count() - 1; i >= 0; i--) {
		ameth = EVP_PKEY_asn1_get0(i);
		if (ameth->pkey_flags & ASN1_PKEY_ALIAS)
			continue;
		if (((int)strlen(ameth->pem_str) == len) &&
		    !strncasecmp(ameth->pem_str, str, len))
			return ameth;
	}
	return NULL;
}

int
EVP_PKEY_asn1_get0_info(int *ppkey_id, int *ppkey_base_id, int *ppkey_flags,
    const char **pinfo, const char **ppem_str,
    const EVP_PKEY_ASN1_METHOD *ameth)
{
	if (!ameth)
		return 0;
	if (ppkey_id)
		*ppkey_id = ameth->pkey_id;
	if (ppkey_base_id)
		*ppkey_base_id = ameth->base_method->pkey_id;
	if (ppkey_flags)
		*ppkey_flags = ameth->pkey_flags;
	if (pinfo)
		*pinfo = ameth->info;
	if (ppem_str)
		*ppem_str = ameth->pem_str;
	return 1;
}

const EVP_PKEY_ASN1_METHOD*
EVP_PKEY_get0_asn1(const EVP_PKEY *pkey)
{
	return pkey->ameth;
}

int
EVP_PKEY_bits(const EVP_PKEY *pkey)
{
	if (pkey && pkey->ameth && pkey->ameth->pkey_bits)
		return pkey->ameth->pkey_bits(pkey);
	return 0;
}

int
EVP_PKEY_security_bits(const EVP_PKEY *pkey)
{
	if (pkey == NULL)
		return 0;
	if (pkey->ameth == NULL || pkey->ameth->pkey_security_bits == NULL)
		return -2;

	return pkey->ameth->pkey_security_bits(pkey);
}

int
EVP_PKEY_size(const EVP_PKEY *pkey)
{
	if (pkey && pkey->ameth && pkey->ameth->pkey_size)
		return pkey->ameth->pkey_size(pkey);
	return 0;
}

int
EVP_PKEY_save_parameters(EVP_PKEY *pkey, int mode)
{
#ifndef OPENSSL_NO_DSA
	if (pkey->type == EVP_PKEY_DSA) {
		int ret = pkey->save_parameters;

		if (mode >= 0)
			pkey->save_parameters = mode;
		return (ret);
	}
#endif
#ifndef OPENSSL_NO_EC
	if (pkey->type == EVP_PKEY_EC) {
		int ret = pkey->save_parameters;

		if (mode >= 0)
			pkey->save_parameters = mode;
		return (ret);
	}
#endif
	return (0);
}

int
EVP_PKEY_copy_parameters(EVP_PKEY *to, const EVP_PKEY *from)
{
	if (to->type != from->type) {
		EVPerror(EVP_R_DIFFERENT_KEY_TYPES);
		goto err;
	}

	if (EVP_PKEY_missing_parameters(from)) {
		EVPerror(EVP_R_MISSING_PARAMETERS);
		goto err;
	}
	if (from->ameth && from->ameth->param_copy)
		return from->ameth->param_copy(to, from);

err:
	return 0;
}

int
EVP_PKEY_missing_parameters(const EVP_PKEY *pkey)
{
	if (pkey->ameth && pkey->ameth->param_missing)
		return pkey->ameth->param_missing(pkey);
	return 0;
}

int
EVP_PKEY_cmp_parameters(const EVP_PKEY *a, const EVP_PKEY *b)
{
	if (a->type != b->type)
		return -1;
	if (a->ameth && a->ameth->param_cmp)
		return a->ameth->param_cmp(a, b);
	return -2;
}

int
EVP_PKEY_cmp(const EVP_PKEY *a, const EVP_PKEY *b)
{
	if (a->type != b->type)
		return -1;

	if (a->ameth) {
		int ret;
		/* Compare parameters if the algorithm has them */
		if (a->ameth->param_cmp) {
			ret = a->ameth->param_cmp(a, b);
			if (ret <= 0)
				return ret;
		}

		if (a->ameth->pub_cmp)
			return a->ameth->pub_cmp(a, b);
	}

	return -2;
}

EVP_PKEY *
EVP_PKEY_new(void)
{
	EVP_PKEY *pkey;

	if ((pkey = calloc(1, sizeof(*pkey))) == NULL) {
		EVPerror(ERR_R_MALLOC_FAILURE);
		return NULL;
	}

	pkey->type = EVP_PKEY_NONE;
	pkey->references = 1;
	pkey->save_parameters = 1;

	return pkey;
}

int
EVP_PKEY_up_ref(EVP_PKEY *pkey)
{
	return CRYPTO_add(&pkey->references, 1, CRYPTO_LOCK_EVP_PKEY) > 1;
}

static void
evp_pkey_free_pkey_ptr(EVP_PKEY *pkey)
{
	if (pkey == NULL || pkey->ameth == NULL || pkey->ameth->pkey_free == NULL)
		return;

	pkey->ameth->pkey_free(pkey);
	pkey->pkey.ptr = NULL;
}

void
EVP_PKEY_free(EVP_PKEY *pkey)
{
	if (pkey == NULL)
		return;

	if (CRYPTO_add(&pkey->references, -1, CRYPTO_LOCK_EVP_PKEY) > 0)
		return;

	evp_pkey_free_pkey_ptr(pkey);
	sk_X509_ATTRIBUTE_pop_free(pkey->attributes, X509_ATTRIBUTE_free);
	freezero(pkey, sizeof(*pkey));
}

int
EVP_PKEY_set_type(EVP_PKEY *pkey, int type)
{
	const EVP_PKEY_ASN1_METHOD *ameth;

	evp_pkey_free_pkey_ptr(pkey);

	if ((ameth = EVP_PKEY_asn1_find(NULL, type)) == NULL) {
		EVPerror(EVP_R_UNSUPPORTED_ALGORITHM);
		return 0;
	}
	if (pkey != NULL) {
		pkey->ameth = ameth;
		pkey->type = pkey->ameth->pkey_id;
	}

	return 1;
}

int
EVP_PKEY_set_type_str(EVP_PKEY *pkey, const char *str, int len)
{
	const EVP_PKEY_ASN1_METHOD *ameth;

	evp_pkey_free_pkey_ptr(pkey);

	if ((ameth = EVP_PKEY_asn1_find_str(NULL, str, len)) == NULL) {
		EVPerror(EVP_R_UNSUPPORTED_ALGORITHM);
		return 0;
	}
	if (pkey != NULL) {
		pkey->ameth = ameth;
		pkey->type = pkey->ameth->pkey_id;
	}

	return 1;
}

int
EVP_PKEY_assign(EVP_PKEY *pkey, int type, void *key)
{
	if (!EVP_PKEY_set_type(pkey, type))
		return 0;

	return (pkey->pkey.ptr = key) != NULL;
}

EVP_PKEY *
EVP_PKEY_new_raw_private_key(int type, ENGINE *engine,
    const unsigned char *private_key, size_t len)
{
	EVP_PKEY *pkey;

	if ((pkey = EVP_PKEY_new()) == NULL)
		goto err;

	if (!EVP_PKEY_set_type(pkey, type))
		goto err;

	if (pkey->ameth->set_priv_key == NULL) {
		EVPerror(EVP_R_OPERATION_NOT_SUPPORTED_FOR_THIS_KEYTYPE);
		goto err;
	}
	if (!pkey->ameth->set_priv_key(pkey, private_key, len)) {
		EVPerror(EVP_R_KEY_SETUP_FAILED);
		goto err;
	}

	return pkey;

 err:
	EVP_PKEY_free(pkey);

	return NULL;
}

EVP_PKEY *
EVP_PKEY_new_raw_public_key(int type, ENGINE *engine,
    const unsigned char *public_key, size_t len)
{
	EVP_PKEY *pkey;

	if ((pkey = EVP_PKEY_new()) == NULL)
		goto err;

	if (!EVP_PKEY_set_type(pkey, type))
		goto err;

	if (pkey->ameth->set_pub_key == NULL) {
		EVPerror(EVP_R_OPERATION_NOT_SUPPORTED_FOR_THIS_KEYTYPE);
		goto err;
	}
	if (!pkey->ameth->set_pub_key(pkey, public_key, len)) {
		EVPerror(EVP_R_KEY_SETUP_FAILED);
		goto err;
	}

	return pkey;

 err:
	EVP_PKEY_free(pkey);

	return NULL;
}

int
EVP_PKEY_get_raw_private_key(const EVP_PKEY *pkey,
    unsigned char *out_private_key, size_t *out_len)
{
	if (pkey->ameth->get_priv_key == NULL) {
		EVPerror(EVP_R_OPERATION_NOT_SUPPORTED_FOR_THIS_KEYTYPE);
		return 0;
	}
	if (!pkey->ameth->get_priv_key(pkey, out_private_key, out_len)) {
		EVPerror(EVP_R_GET_RAW_KEY_FAILED);
		return 0;
	}

	return 1;
}

int
EVP_PKEY_get_raw_public_key(const EVP_PKEY *pkey,
    unsigned char *out_public_key, size_t *out_len)
{
	if (pkey->ameth->get_pub_key == NULL) {
		EVPerror(EVP_R_OPERATION_NOT_SUPPORTED_FOR_THIS_KEYTYPE);
		return 0;
	}
	if (!pkey->ameth->get_pub_key(pkey, out_public_key, out_len)) {
		EVPerror(EVP_R_GET_RAW_KEY_FAILED);
		return 0;
	}

	return 1;
}

EVP_PKEY *
EVP_PKEY_new_CMAC_key(ENGINE *e, const unsigned char *priv, size_t len,
    const EVP_CIPHER *cipher)
{
	EVP_PKEY *pkey = NULL;
	CMAC_CTX *cmctx = NULL;

	if ((pkey = EVP_PKEY_new()) == NULL)
		goto err;
	if ((cmctx = CMAC_CTX_new()) == NULL)
		goto err;

	if (!EVP_PKEY_set_type(pkey, EVP_PKEY_CMAC))
		goto err;

	if (!CMAC_Init(cmctx, priv, len, cipher, NULL)) {
		EVPerror(EVP_R_KEY_SETUP_FAILED);
		goto err;
	}

	pkey->pkey.ptr = cmctx;

	return pkey;

 err:
	EVP_PKEY_free(pkey);
	CMAC_CTX_free(cmctx);

	return NULL;
}

void *
EVP_PKEY_get0(const EVP_PKEY *pkey)
{
	return pkey->pkey.ptr;
}

const unsigned char *
EVP_PKEY_get0_hmac(const EVP_PKEY *pkey, size_t *len)
{
	ASN1_OCTET_STRING *os;

	if (pkey->type != EVP_PKEY_HMAC) {
		EVPerror(EVP_R_EXPECTING_AN_HMAC_KEY);
		return NULL;
	}

	os = EVP_PKEY_get0(pkey);
	*len = os->length;

	return os->data;
}

#ifndef OPENSSL_NO_RSA
RSA *
EVP_PKEY_get0_RSA(EVP_PKEY *pkey)
{
	if (pkey->type == EVP_PKEY_RSA || pkey->type == EVP_PKEY_RSA_PSS)
		return pkey->pkey.rsa;

	EVPerror(EVP_R_EXPECTING_AN_RSA_KEY);
	return NULL;
}

RSA *
EVP_PKEY_get1_RSA(EVP_PKEY *pkey)
{
	RSA *rsa;

	if ((rsa = EVP_PKEY_get0_RSA(pkey)) == NULL)
		return NULL;

	RSA_up_ref(rsa);

	return rsa;
}

int
EVP_PKEY_set1_RSA(EVP_PKEY *pkey, RSA *key)
{
	int ret = EVP_PKEY_assign_RSA(pkey, key);
	if (ret != 0)
		RSA_up_ref(key);
	return ret;
}
#endif

#ifndef OPENSSL_NO_DSA
DSA *
EVP_PKEY_get0_DSA(EVP_PKEY *pkey)
{
	if (pkey->type != EVP_PKEY_DSA) {
		EVPerror(EVP_R_EXPECTING_A_DSA_KEY);
		return NULL;
	}
	return pkey->pkey.dsa;
}

DSA *
EVP_PKEY_get1_DSA(EVP_PKEY *pkey)
{
	DSA *dsa;

	if ((dsa = EVP_PKEY_get0_DSA(pkey)) == NULL)
		return NULL;

	DSA_up_ref(dsa);

	return dsa;
}

int
EVP_PKEY_set1_DSA(EVP_PKEY *pkey, DSA *key)
{
	int ret = EVP_PKEY_assign_DSA(pkey, key);
	if (ret != 0)
		DSA_up_ref(key);
	return ret;
}
#endif

#ifndef OPENSSL_NO_EC
EC_KEY *
EVP_PKEY_get0_EC_KEY(EVP_PKEY *pkey)
{
	if (pkey->type != EVP_PKEY_EC) {
		EVPerror(EVP_R_EXPECTING_A_EC_KEY);
		return NULL;
	}
	return pkey->pkey.ec;
}

EC_KEY *
EVP_PKEY_get1_EC_KEY(EVP_PKEY *pkey)
{
	EC_KEY *key;

	if ((key = EVP_PKEY_get0_EC_KEY(pkey)) == NULL)
		return NULL;

	EC_KEY_up_ref(key);

	return key;
}

int
EVP_PKEY_set1_EC_KEY(EVP_PKEY *pkey, EC_KEY *key)
{
	int ret = EVP_PKEY_assign_EC_KEY(pkey, key);
	if (ret != 0)
		EC_KEY_up_ref(key);
	return ret;
}
#endif


#ifndef OPENSSL_NO_DH
DH *
EVP_PKEY_get0_DH(EVP_PKEY *pkey)
{
	if (pkey->type != EVP_PKEY_DH) {
		EVPerror(EVP_R_EXPECTING_A_DH_KEY);
		return NULL;
	}
	return pkey->pkey.dh;
}

DH *
EVP_PKEY_get1_DH(EVP_PKEY *pkey)
{
	DH *dh;

	if ((dh = EVP_PKEY_get0_DH(pkey)) == NULL)
		return NULL;

	DH_up_ref(dh);

	return dh;
}

int
EVP_PKEY_set1_DH(EVP_PKEY *pkey, DH *key)
{
	int ret = EVP_PKEY_assign_DH(pkey, key);
	if (ret != 0)
		DH_up_ref(key);
	return ret;
}
#endif

int
EVP_PKEY_type(int type)
{
	const EVP_PKEY_ASN1_METHOD *ameth;

	if ((ameth = EVP_PKEY_asn1_find(NULL, type)) != NULL)
		return ameth->pkey_id;

	return NID_undef;
}

int
EVP_PKEY_id(const EVP_PKEY *pkey)
{
	return pkey->type;
}

int
EVP_PKEY_base_id(const EVP_PKEY *pkey)
{
	return EVP_PKEY_type(pkey->type);
}

static int
unsup_alg(BIO *out, const EVP_PKEY *pkey, int indent, const char *kstr)
{
	if (!BIO_indent(out, indent, 128))
		return 0;
	BIO_printf(out, "%s algorithm \"%s\" unsupported\n",
	    kstr, OBJ_nid2ln(pkey->type));
	return 1;
}

int
EVP_PKEY_print_public(BIO *out, const EVP_PKEY *pkey, int indent,
    ASN1_PCTX *pctx)
{
	if (pkey->ameth && pkey->ameth->pub_print)
		return pkey->ameth->pub_print(out, pkey, indent, pctx);

	return unsup_alg(out, pkey, indent, "Public Key");
}

int
EVP_PKEY_print_private(BIO *out, const EVP_PKEY *pkey, int indent,
    ASN1_PCTX *pctx)
{
	if (pkey->ameth && pkey->ameth->priv_print)
		return pkey->ameth->priv_print(out, pkey, indent, pctx);

	return unsup_alg(out, pkey, indent, "Private Key");
}

int
EVP_PKEY_print_params(BIO *out, const EVP_PKEY *pkey, int indent,
    ASN1_PCTX *pctx)
{
	if (pkey->ameth && pkey->ameth->param_print)
		return pkey->ameth->param_print(out, pkey, indent, pctx);
	return unsup_alg(out, pkey, indent, "Parameters");
}

int
EVP_PKEY_get_default_digest_nid(EVP_PKEY *pkey, int *pnid)
{
	if (!pkey->ameth || !pkey->ameth->pkey_ctrl)
		return -2;
	return pkey->ameth->pkey_ctrl(pkey, ASN1_PKEY_CTRL_DEFAULT_MD_NID,
	    0, pnid);
}
