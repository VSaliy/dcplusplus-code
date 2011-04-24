/*
 * SHA1Hash.h
 *
 *  Created on: 23 apr 2011
 *      Author: arnetheduck
 */

#ifndef DCPLUSPLUS_DCPP_SHA1HASH_H_
#define DCPLUSPLUS_DCPP_SHA1HASH_H_

#include <openssl/sha.h>

#include "HashValue.h"

namespace dcpp {

class SHA1Hash {
public:
	/** Hash size in bytes */
	static const size_t BITS = 160;
	static const size_t BYTES = BITS / 8;

	SHA1Hash() { SHA1_Init(&ctx); }

	~SHA1Hash() { }

	/** Calculates the Tiger hash of the data. */
	void update(const void* data, size_t len) { SHA1_Update(&ctx, data, len); }
	/** Call once all data has been processed. */
	uint8_t* finalize() { SHA1_Final(reinterpret_cast<unsigned char*>(&res), &ctx); return res; }

	uint8_t* getResult() { return res; }
private:
	SHA_CTX ctx;
	uint8_t res[BYTES];
};

typedef HashValue<SHA1Hash> SHA1Value;

} // namespace dcpp

#endif /* DCPLUSPLUS_DCPP_SHA1HASH_H_ */
