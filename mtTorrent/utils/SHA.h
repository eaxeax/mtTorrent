#pragma once

#define SHA_DIGEST_LENGTH 20

void SHA1(const unsigned char* d, size_t n, unsigned char* md);