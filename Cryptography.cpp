#include "Cryptography.h"
#include "Time.h"

#ifdef WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
	#include <wincrypt.h>
#elif defined POSIX
	#include <openssl/evp.h>
#endif

#include <math.h>
#include <random>

using namespace std;
using namespace Utilities::Cryptography;

static bool seeded = false;
static random_device device;
static mt19937_64 generator(device());

void Utilities::Cryptography::SHA512(uint8* source, uint32 length, uint8 hashOutput[SHA512_LENGTH]) {
#ifdef WINDOWS
	HCRYPTPROV provider = 0;
	HCRYPTHASH hasher = 0;
	DWORD hashLength;

	CryptAcquireContext(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
	CryptCreateHash(provider, CALG_SHA_512, 0, 0, &hasher);
	CryptHashData(hasher, source, length, 0);

	CryptGetHashParam(hasher, HP_HASHVAL, hashOutput, &hashLength, 0);

	CryptDestroyHash(hasher);
	CryptReleaseContext(provider, 0);
#elif defined POSIX
	EVP_MD_CTX *ctx = EVP_MD_CTX_create();

	EVP_DigestInit_ex(ctx, EVP_sha512(), nullptr);
	EVP_DigestUpdate(ctx, (void*)source, length);
	EVP_DigestFinal_ex(ctx, (unsigned char*)hashOutput, nullptr);
	
	EVP_MD_CTX_destroy(ctx);
#endif
}

void Utilities::Cryptography::SHA1(uint8* source, uint32 length, uint8 hashOutput[SHA1_LENGTH]) {
#ifdef WINDOWS
	HCRYPTPROV provider = 0;
	HCRYPTHASH hasher = 0;
	DWORD hashLength;

	CryptAcquireContext(&provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
	CryptCreateHash(provider, CALG_SHA1, 0, 0, &hasher);
	CryptHashData(hasher, source, length, 0);

	CryptGetHashParam(hasher, HP_HASHVAL, hashOutput, &hashLength, 0);

	CryptDestroyHash(hasher);
	CryptReleaseContext(provider, 0);
#elif defined POSIX
	EVP_MD_CTX *ctx = EVP_MD_CTX_create();

	EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr);
	EVP_DigestUpdate(ctx, (void*)source, length);
	EVP_DigestFinal_ex(ctx, (unsigned char*)hashOutput, nullptr);
#endif
}

void Utilities::Cryptography::RandomBytes(uint8* buffer, uint64 count) {
	uniform_int_distribution<uint64> distribution(0, 0xFFFFFFFFFFFFFFFF);

	if (count > 0) {
		for (; count > 7; count -= 8)
			*((uint64*)(buffer + count - 8)) = distribution(generator);

		for (; count > 0; count--)
			*(buffer + count - 1) = (uint8)distribution(generator);
	}
}

uint64 Utilities::Cryptography::RandomInt64(int64 floor, int64 ceiling) {
	uniform_int_distribution<int64> distribution(floor, ceiling);
	
	return distribution(generator);
}
