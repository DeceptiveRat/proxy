#include "RSA.h"

int fastExponentiation(const int base, const int exponent, const int modulus)
{
	int result = 1;

	// find first 1 bit in the exponent
	int startLocation = 0;

	while(((INTMSB >> startLocation) & exponent) == 0)
		startLocation++;

	for(int i = startLocation; i < 32; i++)
	{
		result *= result;
		result %= modulus;

		if(((INTMSB >> i) & exponent) != 0)
		{
			result *= base;
			result %= modulus;
		}
	}

	return result;
}

void encrypt(unsigned char* data, int n, int e, int dataLength)
{
	for(int i = 0; i < dataLength; i++)
	{
		data[i] = fastExponentiation(data[i], e, n);
	}
}

void decrypt(unsigned char* data, int n, int d, int dataLength)
{
	for(int i = 0;i<dataLength;i++)
	{
		data[i] = fastExponentiation(data[i], d, n);
	}
}

int getPhi(int p, int q)
{
	return (p - 1) * (q - 1);
}

/*
 * gcd(e, d) = a*x + b*y
 */
int extendedEuclideanAlgorithm(const int a, const int b, int* x, int* y)
{
	if(b == 0)
	{
		*x = 1;
		*y = 0;
		return a;
	}

	else
	{
		int xPrime;
		int yPrime;
		int gcd = extendedEuclideanAlgorithm(b, a % b, &xPrime, &yPrime);
		*x = yPrime;
		*y = xPrime - (a / b) * yPrime;
		return gcd;
	}
}

int getDecryptionKey(int e, int phi)
{
	int d;
	int x;
	extendedEuclideanAlgorithm(phi, e, &x, &d);
	return d;
}
