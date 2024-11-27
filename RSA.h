#pragma once
#define INTMSB 0x80000000

int fastExponentiation(const int base, const int exponent, const int modulus);

// TODO change data types later
struct keyPair
{
	int n;
	int e;
	int d;
};

void encrypt(unsigned char* data, int n, int e, int dataLength);
void decrypt(unsigned char* data, int n, int d, int dataLength);
int getPhi(int p, int q);
int extendedEuclideanAlgorithm(const int a, const int b, int* x, int* y);
int getDecryptionKey(int e, int phi);
