/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include "CryptoManager.h"

#include <boost/scoped_array.hpp>

#include "File.h"
#include "LogManager.h"
#include "ClientManager.h"
#include "version.h"

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include <bzlib.h>

namespace dcpp {

void* CryptoManager::tmpKeysMap[KEY_LAST] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
CriticalSection* CryptoManager::cs = NULL;
int CryptoManager::idxVerifyData = 0;
char CryptoManager::idxVerifyDataName[] = APPNAME ".VerifyData";
CryptoManager::SSLVerifyData CryptoManager::trustedKeyprint = { false, "trusted_keyp" };

CryptoManager::CryptoManager()
:
	certsLoaded(false),
	lock("EXTENDEDPROTOCOLABCABCABCABCABCABC"),
	pk("DCPLUSPLUS" VERSIONSTRING)
{
	cs = new CriticalSection[CRYPTO_num_locks()];
	CRYPTO_set_locking_callback(locking_function);

	SSL_library_init();
	SSL_load_error_strings();

	clientContext.reset(SSL_CTX_new(SSLv23_client_method()));
	serverContext.reset(SSL_CTX_new(SSLv23_server_method()));

	idxVerifyData = SSL_get_ex_new_index(0, idxVerifyDataName, NULL, NULL, NULL);

	if(clientContext && serverContext) {
		// Check that openssl rng has been seeded with enough data
		sslRandCheck();

		// Init temp data for DH keys
		for(int i = KEY_FIRST; i != KEY_RSA_512; ++i)
			tmpKeysMap[i] = getTmpDH(getKeyLength(static_cast<TLSTmpKeys>(i)));

		// and same for RSA keys
		for(int i = KEY_RSA_512; i != KEY_LAST; ++i)
			tmpKeysMap[i] = getTmpRSA(getKeyLength(static_cast<TLSTmpKeys>(i)));

		SSL_CTX_set_options(clientContext, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
		SSL_CTX_set_options(serverContext, SSL_OP_SINGLE_DH_USE | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

		EC_KEY* tmp_ecdh;
		if ((tmp_ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1)) != NULL) {
			SSL_CTX_set_options(serverContext, SSL_OP_SINGLE_ECDH_USE);
			SSL_CTX_set_tmp_ecdh(serverContext, tmp_ecdh);

			EC_KEY_free(tmp_ecdh);
		}

		SSL_CTX_set_tmp_dh_callback(serverContext, CryptoManager::tmp_dh_cb);
		SSL_CTX_set_tmp_rsa_callback(serverContext, CryptoManager::tmp_rsa_cb);

		SSL_CTX_set_verify(clientContext, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, verify_callback);
		SSL_CTX_set_verify(serverContext, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, verify_callback);
	}
}

CryptoManager::~CryptoManager() {
	CRYPTO_set_locking_callback(NULL);
	delete[] cs;

	/* thread-local cleanup */ 
	ERR_remove_thread_state(NULL);

	clientContext.reset();
	serverContext.reset();

	for(int i = KEY_FIRST; i != KEY_RSA_512; ++i) {
		if(tmpKeysMap[i]) DH_free((DH*)tmpKeysMap[i]);
	}

	for(int i = KEY_RSA_512; i != KEY_LAST; ++i) {
		if(tmpKeysMap[i]) RSA_free((RSA*)tmpKeysMap[i]);
	}

	/* global application exit cleanup (after all SSL activity is shutdown) */ 
	ERR_free_strings(); 
	EVP_cleanup(); 
	CRYPTO_cleanup_all_ex_data();
}

bool CryptoManager::TLSOk() const noexcept {
	return certsLoaded && !keyprint.empty();
}

void CryptoManager::generateCertificate() {
	// Generate certificate using OpenSSL
	if(SETTING(TLS_PRIVATE_KEY_FILE).empty()) {
		throw CryptoException(_("No private key file chosen"));
	}
	if(SETTING(TLS_CERTIFICATE_FILE).empty()) {
		throw CryptoException(_("No certificate file chosen"));
	}

	ssl::BIGNUM bn(BN_new());
	ssl::RSA rsa(RSA_new());
	ssl::EVP_PKEY pkey(EVP_PKEY_new());
	ssl::X509_NAME nm(X509_NAME_new());
	ssl::X509 x509ss(X509_new());
	ssl::ASN1_INTEGER serial(ASN1_INTEGER_new());

	if(!bn || !rsa || !pkey || !nm || !x509ss || !serial) {
		throw CryptoException(_("Error generating certificate"));
	}

	int days = 90;
	int keylength = 2048;

#define CHECK(n) if(!(n)) { throw CryptoException(#n); }

	// Generate key pair
	CHECK((BN_set_word(bn, RSA_F4)))
	CHECK((RSA_generate_key_ex(rsa, keylength, bn, NULL)))
	CHECK((EVP_PKEY_set1_RSA(pkey, rsa)))

	ByteVector fieldBytes;

	// Add common name (use cid)
	string name = ClientManager::getInstance()->getMyCID().toBase32().c_str();
	fieldBytes.assign(name.begin(), name.end());
	CHECK((X509_NAME_add_entry_by_NID(nm, NID_commonName, MBSTRING_ASC, &fieldBytes[0], fieldBytes.size(), -1, 0)))

	// Add an organisation
	string org = "DCPlusPlus (OSS/SelfSigned)";
	fieldBytes.assign(org.begin(), org.end());
	CHECK((X509_NAME_add_entry_by_NID(nm, NID_organizationName, MBSTRING_ASC, &fieldBytes[0], fieldBytes.size(), -1, 0)))

	// Generate unique serial
	CHECK((BN_pseudo_rand(bn, 64, 0, 0)))
	CHECK((BN_to_ASN1_INTEGER(bn, serial)))

	// Prepare self-signed cert
	CHECK((X509_set_version(x509ss, 0x02))) // This is actually V3
	CHECK((X509_set_serialNumber(x509ss, serial)))
	CHECK((X509_set_serialNumber(x509ss, serial)))
	CHECK((X509_set_issuer_name(x509ss, nm)))
	CHECK((X509_set_subject_name(x509ss, nm)))
	CHECK((X509_gmtime_adj(X509_get_notBefore(x509ss), 0)))
	CHECK((X509_gmtime_adj(X509_get_notAfter(x509ss), (long)60*60*24*days)))
	CHECK((X509_set_pubkey(x509ss, pkey)))

	// Sign using own private key
	CHECK((X509_sign(x509ss, pkey, EVP_sha256())))

#undef CHECK
	// Write the key and cert
	{
		File::ensureDirectory(SETTING(TLS_PRIVATE_KEY_FILE));
		FILE* f = dcpp_fopen(SETTING(TLS_PRIVATE_KEY_FILE).c_str(), "w");
		if(!f) {
			return;
		}
		PEM_write_RSAPrivateKey(f, rsa, NULL, NULL, 0, NULL, NULL);
		fclose(f);
	}
	{
		File::ensureDirectory(SETTING(TLS_CERTIFICATE_FILE));
		FILE* f = dcpp_fopen(SETTING(TLS_CERTIFICATE_FILE).c_str(), "w");
		if(!f) {
			File::deleteFile(SETTING(TLS_PRIVATE_KEY_FILE));
			return;
		}
		PEM_write_X509(f, x509ss);
		fclose(f);
	}
}

void CryptoManager::sslRandCheck() {
	if(!RAND_status()) {
#ifdef _WIN32
		RAND_screen();
#endif
	}
}

int CryptoManager::getKeyLength(TLSTmpKeys key) {
	switch (key) {
		case KEY_DH_512:
		case KEY_RSA_512:
			return 512;
		case KEY_DH_1024:
		case KEY_RSA_1024:
			return 1024;
		case KEY_DH_2048:
		case KEY_RSA_2048:
			return 2048;
		case KEY_DH_4096:
			return 4096;
		default:
			dcassert(0); return 0;
	}
}

DH* CryptoManager::getTmpDH(int keyLen) {
#ifndef WITH_WEAK_KEYS
	if (keyLen < 2048)
		return NULL;
#endif

	DH* tmpDH = DH_new();
	if(!tmpDH) return NULL;

	switch (keyLen) {
		case 512: {
			static unsigned char dh512_p[]={
				0x9E,0xE8,0x07,0x39,0xD4,0xE0,0x47,0x6B,0xBC,0x1F,0xD5,0x9E,
				0xDB,0xC3,0x1F,0xD8,0xAF,0xF2,0x57,0x07,0x2D,0x98,0x9A,0x1E,
				0xEB,0xBA,0xFD,0x8D,0x67,0xD0,0x54,0xDD,0xD5,0xCF,0xA1,0xF7,
				0x36,0xB7,0x47,0x42,0xC2,0x64,0x57,0x2D,0xAC,0xFC,0x3E,0x71,
				0x84,0xEE,0x0D,0xCA,0xA8,0x0A,0x1C,0xC7,0x0B,0xA1,0xC4,0x3F,
				0x3F,0x87,0x79,0x63,
			};
			tmpDH->p = BN_bin2bn(dh512_p, sizeof(dh512_p), 0);
			break;
		}

		case 1024: {
			static unsigned char dh1024_p[]={
				0xCF,0x19,0xF7,0xF9,0x94,0x06,0x46,0xF4,0x0E,0x90,0xF4,0x52,
				0xC4,0xA1,0xEA,0xD2,0xA2,0x7C,0xF9,0x6B,0xDD,0x67,0x5A,0x66,
				0x1A,0xCF,0x7C,0xC8,0x82,0x34,0x1E,0x61,0xAF,0x48,0x11,0xDA,
				0x40,0x90,0x6E,0x79,0xEA,0x11,0xDE,0x04,0x46,0x3B,0x2D,0xA5,
				0x4E,0x6B,0x4A,0x4A,0xE4,0xF8,0xF1,0x93,0x04,0xC2,0x90,0x2B,
				0x6C,0xA8,0xDD,0x8B,0xFE,0x29,0xDE,0xB7,0xBF,0xE1,0x24,0x5C,
				0x60,0xB0,0x52,0xAB,0xAA,0x4D,0xED,0x27,0x36,0xE3,0xBD,0xBB,
				0xCB,0xA3,0x44,0x01,0x97,0x8E,0x8E,0x2C,0x37,0x03,0x7A,0xD2,
				0xC3,0xC3,0x43,0xA3,0x54,0x4C,0xBD,0x88,0x44,0x6B,0xD9,0xC5,
				0xF0,0xD5,0x14,0x18,0xE7,0xBE,0xF7,0x3E,0x8E,0x87,0x47,0x5E,
				0xB7,0xFB,0x6F,0xC1,0xA5,0x21,0x01,0x83,
			};
			tmpDH->p = BN_bin2bn(dh1024_p, sizeof(dh1024_p), 0);
			break;
		}

		case 2048: {
			static unsigned char dh2048_p[]={
				0xD5,0x68,0xA8,0xCD,0xE0,0x49,0xF9,0x14,0xEC,0x80,0x38,0x57,
				0x17,0xCD,0xA2,0x63,0x74,0x7A,0x96,0x85,0x94,0x2D,0x46,0x89,
				0x0E,0x15,0xBC,0xD6,0xEE,0xA5,0x6D,0xB9,0x8D,0x56,0x30,0xEF,
				0x25,0x39,0x42,0x3D,0x94,0xAB,0x7C,0xD5,0x29,0x12,0x11,0x86,
				0x5D,0x6A,0xEA,0x5F,0x58,0xBF,0xDE,0x15,0xC7,0xBE,0x9F,0x5B,
				0x90,0x34,0x94,0xC7,0xB6,0x19,0xAE,0x60,0xA7,0x82,0xF2,0x8E,
				0x80,0x79,0xD3,0xA5,0xAA,0x92,0xFA,0x47,0x68,0x99,0xBC,0x07,
				0xA2,0xAF,0x4C,0x52,0x77,0x0A,0x35,0x97,0x0D,0x9F,0x47,0x9D,
				0x25,0x03,0xCA,0xBE,0x8A,0xEA,0xFF,0x82,0x65,0x56,0xA5,0x31,
				0x7D,0xFC,0x8C,0x8F,0x1A,0x25,0x77,0x00,0xE4,0x28,0x1A,0x8E,
				0x04,0x8A,0x44,0x56,0x0F,0xC4,0x04,0xD8,0x23,0xEF,0xDF,0x46,
				0xF5,0xED,0xCA,0xED,0xA9,0x8B,0xBC,0x6A,0xC9,0x00,0x78,0x77,
				0xC2,0x38,0xBE,0x28,0x7A,0x1E,0x95,0x54,0xC0,0xB4,0xB8,0x81,
				0xFD,0xF6,0xAA,0x75,0x34,0x0B,0x9E,0x2C,0x62,0xB5,0xEF,0x8E,
				0xD4,0xD9,0x4B,0xDA,0x13,0x1C,0x2E,0x93,0x35,0x84,0x96,0xBB,
				0xCB,0xA4,0xEC,0x09,0x47,0x9B,0x39,0x5E,0x80,0x3A,0x7A,0xDB,
				0x4B,0x46,0xCD,0x4D,0x4C,0x74,0x1D,0x89,0x50,0xDE,0xC2,0xED,
				0xBC,0x47,0xA7,0x3A,0x8B,0x58,0xC3,0x7C,0xA6,0xEC,0x72,0x07,
				0x14,0x95,0x7E,0xC2,0xCD,0x5A,0x69,0xB7,0x08,0x6B,0x80,0xDF,
				0x2E,0x76,0x52,0xEB,0xF3,0x33,0x14,0x97,0x6A,0x95,0xBC,0xE2,
				0x8A,0x94,0x23,0xB6,0x9A,0x6F,0x42,0xAF,0x34,0x80,0x67,0x7D,
				0xC6,0x18,0xBB,0x6B,
			};
			tmpDH->p = BN_bin2bn(dh2048_p, sizeof(dh2048_p), 0);
			break;
		}

		case 4096: {
			static unsigned char dh4096_p[]={
				0xCA,0x35,0xA8,0xBB,0x65,0x33,0x28,0xC6,0x3F,0xD7,0x21,0x55,
				0x95,0xDF,0xC0,0xDC,0x11,0x10,0x23,0x2D,0x1E,0xD6,0x52,0x23,
				0xA1,0x52,0xB8,0xDD,0x4A,0x25,0xEE,0xF4,0x78,0xB6,0x89,0x9E,
				0xB6,0x33,0xEB,0x01,0xA6,0x46,0x31,0xD8,0x3D,0x12,0xB4,0x7B,
				0x1F,0x64,0x0C,0x84,0x10,0x80,0xFB,0x4F,0x74,0x21,0xA3,0x9B,
				0xF5,0x97,0xD1,0x05,0x97,0x9D,0x52,0x4F,0x91,0x3C,0xE1,0xA8,
				0x97,0xE0,0x33,0x9D,0xCB,0x9D,0x9D,0x2A,0xB5,0x3E,0xF5,0x8D,
				0x7F,0xEE,0x91,0xEE,0x4E,0xC5,0xA6,0xAB,0x54,0xD9,0xC2,0xA5,
				0x0D,0x2E,0xEA,0x1A,0x39,0xFD,0x30,0x4A,0x1C,0xB7,0x34,0x2B,
				0x7D,0x51,0xF6,0xB1,0xD1,0x8D,0xCE,0x28,0xDC,0xF9,0xDE,0x34,
				0xAF,0x1E,0xD1,0x7C,0xC2,0xD3,0x38,0x8E,0xBD,0x35,0x01,0x53,
				0xDD,0x2E,0xB5,0x83,0xC8,0xEF,0x08,0x15,0x59,0x6E,0xA3,0xC4,
				0x71,0x57,0x8C,0x4D,0xFD,0xA7,0x19,0x40,0x88,0x68,0x4E,0xD6,
				0x8F,0x5C,0xE5,0xEC,0xCF,0x5F,0xEB,0x9A,0xA4,0x66,0x40,0x8B,
				0x02,0x87,0xA7,0x3A,0x58,0x81,0xF1,0x6A,0x14,0x16,0x75,0x7D,
				0x33,0x01,0xEB,0x3F,0xA8,0x02,0xDC,0x09,0x32,0x12,0x00,0x20,
				0x47,0xFF,0x01,0x14,0xFE,0x9E,0x3A,0x44,0x4D,0xED,0x85,0xD5,
				0xDA,0x2F,0xE3,0x99,0xCE,0xDA,0x84,0x64,0xCB,0x0C,0x8C,0x00,
				0x90,0x19,0x70,0x1C,0x00,0x1D,0x63,0x3C,0x77,0x16,0x8D,0x3D,
				0x86,0x97,0x22,0x23,0x2F,0x7B,0xAB,0xB8,0xEB,0x94,0xA4,0x01,
				0xAA,0x34,0xBA,0xEA,0x7D,0x7A,0x37,0xB7,0x0C,0x75,0xEB,0x00,
				0x8D,0x52,0x7A,0xE2,0xDF,0x78,0x7C,0x4F,0x54,0x9E,0xA4,0xDD,
				0xC9,0xFC,0x08,0x7C,0x45,0x70,0x43,0x0F,0x39,0xE3,0x7E,0x48,
				0xB8,0xDC,0x9D,0xEC,0xB9,0x51,0x29,0x86,0x29,0x60,0xF6,0x4F,
				0xF7,0xCA,0xDD,0x3B,0x7F,0xAE,0xE2,0x54,0x4C,0x53,0x42,0x55,
				0xC0,0x39,0x24,0xE1,0x1A,0xAD,0x9E,0xCC,0x75,0x5E,0xF1,0xE2,
				0xD6,0xAE,0xCD,0x9A,0x91,0xC3,0x7B,0xE5,0x29,0xAD,0xCA,0xC2,
				0x00,0xC1,0xF9,0xF4,0x6D,0xD2,0x4B,0xD4,0x5A,0x56,0x39,0xCD,
				0xAC,0xCA,0xE7,0xD1,0x8C,0x15,0x4D,0x2B,0x59,0x67,0x29,0x72,
				0xE7,0x40,0x14,0x81,0x9E,0x26,0x48,0xF8,0x6C,0x51,0xF5,0xBE,
				0x64,0xD1,0xF4,0x4D,0x98,0xE7,0xFD,0x5E,0x23,0x1E,0xDF,0xBA,
				0xBD,0x2E,0xB1,0x81,0x26,0x98,0x9C,0x2F,0xE8,0xD5,0x32,0x6B,
				0x94,0x91,0x8C,0x2E,0xB8,0xD9,0xC9,0x2F,0x22,0x9D,0xA6,0x52,
				0x02,0xDF,0x99,0x63,0x64,0x7E,0xB8,0x68,0xAB,0x17,0x54,0x7E,
				0xED,0x9E,0xD1,0x45,0x64,0x36,0x65,0xE8,0x09,0x50,0xAB,0xB0,
				0xD4,0x8C,0x79,0x9F,0x4C,0xB8,0x26,0x45,0xBE,0x0F,0xDE,0x14,
				0x6F,0xEC,0x70,0x21,0x1A,0xA0,0x1D,0xD0,0x7D,0xA2,0x0F,0x85,
				0xA5,0x7C,0xC1,0x0A,0x74,0xB1,0x7B,0x5A,0xD2,0xC4,0x0F,0xD5,
				0x90,0x24,0x3E,0xEC,0x89,0x7E,0xB8,0xED,0x6E,0x19,0x85,0xB9,
				0x58,0x36,0xA1,0x33,0x7D,0x14,0xFE,0x4F,0x55,0xA9,0xB6,0x42,
				0x7E,0x97,0x2A,0x96,0x50,0x14,0x0D,0xEA,0x02,0xB1,0xD2,0x22,
				0xEB,0xE7,0xF4,0xAC,0xB6,0x37,0xCA,0xAB,0x4A,0x1E,0x4D,0x4E,
				0xCF,0xFE,0x5D,0xEF,0x23,0x78,0xC6,0xBB,
			};
			tmpDH->p = BN_bin2bn(dh4096_p, sizeof(dh4096_p), 0);
			break;
		}
	}

	static unsigned char dh_g[]={
		0x02,
	};

	tmpDH->g = BN_bin2bn(dh_g, sizeof(dh_g), 0);

	if(!tmpDH->p || !tmpDH->g) {
		DH_free(tmpDH);
		return NULL;
	} else return tmpDH;
}

RSA* CryptoManager::getTmpRSA(int keyLen) {
#ifndef WITH_WEAK_KEYS
	if (keyLen < 2048)
		return NULL;
#endif

	RSA* tmpRSA = RSA_new();
	BIGNUM* bn = BN_new();

	if(!bn || !BN_set_word(bn, RSA_F4) || !RSA_generate_key_ex(tmpRSA, keyLen, bn, NULL)) {
		if (bn) BN_free(bn);
		RSA_free(tmpRSA);
		return NULL;
	}

	BN_free(bn);
	return tmpRSA;
}

void CryptoManager::loadCertificates() noexcept {
	if(!clientContext || !serverContext)
		return;

	keyprint.clear();
	certsLoaded = false;

	const string& cert = SETTING(TLS_CERTIFICATE_FILE);
	const string& key = SETTING(TLS_PRIVATE_KEY_FILE);

	if(cert.empty() || key.empty()) {
		LogManager::getInstance()->message(_("TLS disabled, no certificate file set"));
		return;
	}

	if(File::getSize(cert) == -1 || File::getSize(key) == -1 || !checkCertificate()) {
		// Try to generate them...
		try {
			generateCertificate();
			LogManager::getInstance()->message(_("Generated new TLS certificate"));
		} catch(const CryptoException& e) {
			LogManager::getInstance()->message(str(F_("TLS disabled, failed to generate certificate: %1%") % e.getError()));
			return;
		}
	}

	if(
		!ssl::SSL_CTX_use_certificate_file(serverContext, cert.c_str(), SSL_FILETYPE_PEM) ||
		!ssl::SSL_CTX_use_certificate_file(clientContext, cert.c_str(), SSL_FILETYPE_PEM)
	) {
		LogManager::getInstance()->message(_("Failed to load certificate file"));
		return;
	}

	if(
		!ssl::SSL_CTX_use_PrivateKey_file(serverContext, key.c_str(), SSL_FILETYPE_PEM) ||
		!ssl::SSL_CTX_use_PrivateKey_file(clientContext, key.c_str(), SSL_FILETYPE_PEM)
	) {
		LogManager::getInstance()->message(_("Failed to load private key"));
		return;
	}

	auto certs = File::findFiles(SETTING(TLS_TRUSTED_CERTIFICATES_PATH), "*.pem");
	auto certs2 = File::findFiles(SETTING(TLS_TRUSTED_CERTIFICATES_PATH), "*.crt");
	certs.insert(certs.end(), certs2.begin(), certs2.end());

	for(auto& i: certs) {
		if(
			SSL_CTX_load_verify_locations(clientContext, i.c_str(), NULL) != SSL_SUCCESS ||
			SSL_CTX_load_verify_locations(serverContext, i.c_str(), NULL) != SSL_SUCCESS
		) {
			LogManager::getInstance()->message(str(F_("Failed to load trusted certificate from %1%") % Util::addBrackets(i)));
		}
	}

	loadKeyprint(cert.c_str());

	certsLoaded = true;
}

bool CryptoManager::checkCertificate() noexcept {
	auto x509 = ssl::getX509(SETTING(TLS_CERTIFICATE_FILE).c_str());
	if(!x509) {
		return false;
	}

	ASN1_INTEGER* sn = X509_get_serialNumber(x509);
	if(!sn || !ASN1_INTEGER_get(sn)) {
		return false;
	}

	X509_NAME* name = X509_get_subject_name(x509);
	if(!name) {
		return false;
	}

	string cn = getNameEntryByNID(name, NID_commonName);
	if(cn != ClientManager::getInstance()->getMyCID().toBase32()) {
		return false;
	}

	ASN1_TIME* t = X509_get_notAfter(x509);
	if(t) {
		if(X509_cmp_current_time(t) < 0) {
			return false;
		}
	}
	return true;
}

const ByteVector& CryptoManager::getKeyprint() const noexcept {
	return keyprint;
}

void CryptoManager::loadKeyprint(const string& file) noexcept {
	auto x509 = ssl::getX509(SETTING(TLS_CERTIFICATE_FILE).c_str());
	if(x509) {
		keyprint = ssl::X509_digest(x509, EVP_sha256());
	}
}

SSL_CTX* CryptoManager::getSSLContext(SSLContext wanted) {
	switch(wanted) {
		case SSL_CLIENT: return clientContext;
		case SSL_SERVER: return serverContext;
		default: return NULL;
	}
}

void CryptoManager::locking_function(int mode, int n, const char* /*file*/, int /*line*/) {
	if(mode & CRYPTO_LOCK) {
		cs[n].lock();
	} else {
		cs[n].unlock();
	}
}

DH* CryptoManager::tmp_dh_cb(SSL* /*ssl*/, int /*is_export*/, int keylength) {
#ifndef WITH_WEAK_KEYS
	if (keylength < 2048)
		return (DH*)tmpKeysMap[KEY_DH_2048];
#endif

	void* tmpDH = NULL;
	switch(keylength) {
		case 512:
			tmpDH = tmpKeysMap[KEY_DH_512]; break;
		case 1024:
			tmpDH = tmpKeysMap[KEY_DH_1024]; break;
		case 2048:
			tmpDH = tmpKeysMap[KEY_DH_2048]; break;
		case 4096: 
			tmpDH = tmpKeysMap[KEY_DH_4096]; break;
	}

	return (DH*)(tmpDH ? tmpDH : tmpKeysMap[KEY_DH_2048]);
}

RSA* CryptoManager::tmp_rsa_cb(SSL* /*ssl*/, int /*is_export*/, int keylength) {
#ifndef WITH_WEAK_KEYS
	if (keylength < 2048)
		return (RSA*)tmpKeysMap[KEY_RSA_2048];
#endif

	void* tmpRSA = NULL;
	switch(keylength) {
		case 512:
			tmpRSA = tmpKeysMap[KEY_RSA_512]; break;
		case 1024:
			tmpRSA = tmpKeysMap[KEY_RSA_1024]; break;
		case 2048:
			tmpRSA = tmpKeysMap[KEY_RSA_2048]; break;
	}

	return (RSA*)(tmpRSA ? tmpRSA : tmpKeysMap[KEY_RSA_2048]);
}

int CryptoManager::verify_callback(int preverify_ok, X509_STORE_CTX *ctx) {
	int err = X509_STORE_CTX_get_error(ctx);
	SSL* ssl = (SSL*)X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
	SSLVerifyData* verifyData = (SSLVerifyData*)SSL_get_ex_data(ssl, CryptoManager::idxVerifyData);

	// TODO: we should make sure that the trusted certificate store never overules KeyPrint, if present, because certificate pinning on an individual certificate is a stronger method of verification.

	// verifyData is unset only when KeyPrint has been pinned and we are not skipping errors due to incomplete chains
	// we can fail here f.ex. if the certificate has expired but is still pinned with KeyPrint
	if(!verifyData)
		return preverify_ok;

	bool allowUntrusted = verifyData->first;
	string keyp = verifyData->second;

	if(!keyp.empty()) {
		X509* cert = X509_STORE_CTX_get_current_cert(ctx);
		if(!cert)
			return 0;

		string kp2(keyp);
		if(kp2.compare(0, 12, "trusted_keyp") == 0) {
			// Possible follow up errors, after verification of a partial chain
			if(err == X509_V_ERR_CERT_UNTRUSTED || err == X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE) {
				X509_STORE_CTX_set_error(ctx, X509_V_OK);
				return 1;
			}
		} else if(kp2.compare(0, 7, "SHA256/") != 0)
			return allowUntrusted ? 1 : 0;

		ByteVector kp = ssl::X509_digest(cert, EVP_sha256());
		ByteVector kp2v(kp.size());

		Encoder::fromBase32(&kp2[7], &kp2v[0], kp2v.size());
		if(std::equal(kp.begin(), kp.end(), kp2v.begin())) {
			// KeyPrint validated, we can get rid of it (to avoid unnecessary passes)
			SSL_set_ex_data(ssl, CryptoManager::idxVerifyData, NULL);

			if(err != X509_V_OK) {
				// This is the right way to get the certificate store, although it is rather roundabout
				X509_STORE* store = SSL_CTX_get_cert_store(SSL_get_SSL_CTX(ssl));
				dcassert(store == ctx->ctx);

				// Hide the potential library error about trying to add a dupe
				ERR_set_mark();
				if(X509_STORE_add_cert(store, cert)) {
					X509_STORE_CTX_set_error(ctx, X509_V_OK);
					X509_verify_cert(ctx);
					err = X509_STORE_CTX_get_error(ctx);
				} else ERR_pop_to_mark();

				// KeyPrint was not root certificate or we don't have the issuer certificate, the best we can do is trust the pinned KeyPrint
				if(err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN || err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY || err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT) {
					X509_STORE_CTX_set_error(ctx, X509_V_OK);
					// Set this to allow ignoring any follow up errors caused by the incomplete chain
					SSL_set_ex_data(ssl, CryptoManager::idxVerifyData, &CryptoManager::trustedKeyprint);
					return 1;
				}
			}

			return (err == X509_V_OK) ? 1 : 0;
		} else {
			if(X509_STORE_CTX_get_error_depth(ctx) > 0)
				return 1;
		}
	}

	if(allowUntrusted) {
		// We let untrusted certificates through unconditionally, when allowed, but we like to complain
		if(!preverify_ok && err != X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) {
			X509* cert = NULL;
			if((cert = X509_STORE_CTX_get_current_cert(ctx)) != NULL) {
				X509_NAME* subject = X509_get_subject_name(cert);
				string tmp, line;

				tmp = getNameEntryByNID(subject, NID_commonName);
				if(!tmp.empty()) {
					CID certCID(tmp);
					if(certCID)
						tmp = Util::toString(ClientManager::getInstance()->getNicks(certCID));
					line += (!line.empty() ? ", " : "") + tmp;
				}

				tmp = getNameEntryByNID(subject, NID_organizationName);
				if(!tmp.empty())
					line += (!line.empty() ? ", " : "") + tmp;

				ByteVector kp = ssl::X509_digest(cert, EVP_sha256());
				string keyp = "SHA256/" + Encoder::toBase32(&kp[0], kp.size());

				LogManager::getInstance()->message(str(F_("Certificate verification for %1% failed with error: %2% (certificate KeyPrint: %3%)") % line % X509_verify_cert_error_string(err) % keyp));
			}
		}

		return 1;
	}

	return preverify_ok;
}

string CryptoManager::getNameEntryByNID(X509_NAME* name, int nid) noexcept {
	int i = X509_NAME_get_index_by_NID(name, nid, -1);
	if(i == -1) {
		return Util::emptyString;
	}

	X509_NAME_ENTRY* entry = X509_NAME_get_entry(name, i);
	ASN1_STRING* str = X509_NAME_ENTRY_get_data(entry);
	if(!str) {
		return Util::emptyString;
	}

	unsigned char* buf = 0;
	i = ASN1_STRING_to_UTF8(&buf, str);
	if(i < 0) {
		return Util::emptyString;
	}

	std::string out((char*)buf, i);
	OPENSSL_free(buf);

	return out;
}

void CryptoManager::decodeBZ2(const uint8_t* is, size_t sz, string& os) {
	bz_stream bs = { 0 };

	if(BZ2_bzDecompressInit(&bs, 0, 0) != BZ_OK)
		throw CryptoException(_("Error during decompression"));

	// We assume that the files aren't compressed more than 2:1...if they are it'll work anyway,
	// but we'll have to do multiple passes...
	size_t bufsize = 2*sz;
	boost::scoped_array<char> buf(new char[bufsize]);

	bs.avail_in = sz;
	bs.avail_out = bufsize;
	bs.next_in = reinterpret_cast<char*>(const_cast<uint8_t*>(is));
	bs.next_out = &buf[0];

	int err;

	os.clear();

	while((err = BZ2_bzDecompress(&bs)) == BZ_OK) {
		if (bs.avail_in == 0 && bs.avail_out > 0) { // error: BZ_UNEXPECTED_EOF
			BZ2_bzDecompressEnd(&bs);
			throw CryptoException(_("Error during decompression"));
		}
		os.append(&buf[0], bufsize-bs.avail_out);
		bs.avail_out = bufsize;
		bs.next_out = &buf[0];
	}

	if(err == BZ_STREAM_END)
		os.append(&buf[0], bufsize-bs.avail_out);

	BZ2_bzDecompressEnd(&bs);

	if(err < 0) {
		// This was a real error
		throw CryptoException(_("Error during decompression"));
	}
}

string CryptoManager::keySubst(const uint8_t* aKey, size_t len, size_t n) {
	boost::scoped_array<uint8_t> temp(new uint8_t[len + n * 10]);

	size_t j=0;

	for(size_t i = 0; i<len; i++) {
		if(isExtra(aKey[i])) {
			temp[j++] = '/'; temp[j++] = '%'; temp[j++] = 'D';
			temp[j++] = 'C'; temp[j++] = 'N';
			switch(aKey[i]) {
			case 0: temp[j++] = '0'; temp[j++] = '0'; temp[j++] = '0'; break;
			case 5: temp[j++] = '0'; temp[j++] = '0'; temp[j++] = '5'; break;
			case 36: temp[j++] = '0'; temp[j++] = '3'; temp[j++] = '6'; break;
			case 96: temp[j++] = '0'; temp[j++] = '9'; temp[j++] = '6'; break;
			case 124: temp[j++] = '1'; temp[j++] = '2'; temp[j++] = '4'; break;
			case 126: temp[j++] = '1'; temp[j++] = '2'; temp[j++] = '6'; break;
			}
			temp[j++] = '%'; temp[j++] = '/';
		} else {
			temp[j++] = aKey[i];
		}
	}
	return string((const char*)&temp[0], j);
}

string CryptoManager::makeKey(const string& aLock) {
	if(aLock.size() < 3)
		return Util::emptyString;

	boost::scoped_array<uint8_t> temp(new uint8_t[aLock.length()]);
	uint8_t v1;
	size_t extra=0;

	v1 = (uint8_t)(aLock[0]^5);
	v1 = (uint8_t)(((v1 >> 4) | (v1 << 4)) & 0xff);
	temp[0] = v1;

	string::size_type i;

	for(i = 1; i<aLock.length(); i++) {
		v1 = (uint8_t)(aLock[i]^aLock[i-1]);
		v1 = (uint8_t)(((v1 >> 4) | (v1 << 4))&0xff);
		temp[i] = v1;
		if(isExtra(temp[i]))
			extra++;
	}

	temp[0] = (uint8_t)(temp[0] ^ temp[aLock.length()-1]);

	if(isExtra(temp[0])) {
		extra++;
	}

	return keySubst(&temp[0], aLock.length(), extra);
}

} // namespace dcpp
