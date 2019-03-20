#include "os.h"

#include "sha512_256.h"

typedef uint64_t word_t;

#define WORD_SIZE sizeof(word_t)
#define WORD_SIZE_BITS (WORD_SIZE * 8)

#define BLOCK_SIZE (WORD_SIZE * 16)
#define BLOCK_SIZE_BITS (BLOCK_SIZE * 8)

#define TURNS 80

#define SHR(a,b) ((a) >> (b))
#define ROTR(a,b) (((a) >> (b)) | ((a) << (WORD_SIZE_BITS-(b))))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define EP0(x) (ROTR(x,28) ^ ROTR(x,34) ^ ROTR(x,39))
#define EP1(x) (ROTR(x,14) ^ ROTR(x,18) ^ ROTR(x,41))
#define SIG0(x) (ROTR(x,1) ^ ROTR(x,8) ^ SHR(x,7))
#define SIG1(x) (ROTR(x,19) ^ ROTR(x,61) ^ SHR(x,6))

static const word_t SHA512_CONSTANTS[TURNS] = {
	0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc, 0x3956c25bf348b538,
	0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118, 0xd807aa98a3030242, 0x12835b0145706fbe,
	0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2, 0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235,
	0xc19bf174cf692694, 0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
	0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5, 0x983e5152ee66dfab,
	0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4, 0xc6e00bf33da88fc2, 0xd5a79147930aa725,
	0x06ca6351e003826f, 0x142929670a0e6e70, 0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed,
	0x53380d139d95b3df, 0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
	0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30, 0xd192e819d6ef5218,
	0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8, 0x19a4c116b8d2d0c8, 0x1e376c085141ab53,
	0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8, 0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373,
	0x682e6ff3d6b2b8a3, 0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
	0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b, 0xca273eceea26619c,
	0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178, 0x06f067aa72176fba, 0x0a637dc5a2c898a6,
	0x113f9804bef90dae, 0x1b710b35131c471b, 0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc,
	0x431d67c49c100d4c, 0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817
};

void sha512_compress(word_t res[BLOCK_SIZE], word_t state[8]) {
	word_t a, b, c, d, e, f, g, h, t1, t2;
	word_t m[TURNS] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	unsigned int i, j, k;

	for (i = 0, j = 0; i < 16; i++, j += WORD_SIZE) {
		for (k = 0; k < WORD_SIZE; k++) {
			m[i] |= (res[j + k] << (WORD_SIZE_BITS - 8 - k * 8));
		}
	}
	for (; i < TURNS; i++) {
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
	}

	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];
	f = state[5];
	g = state[6];
	h = state[7];

	for (i = 0; i < TURNS; i++) {
		t1 = h + EP1(e) + CH(e, f, g) + SHA512_CONSTANTS[i] + m[i];
		t2 = EP0(a) + MAJ(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
	state[5] += f;
	state[6] += g;
	state[7] += h;
}

void sha512_compute(const uint8_t data[], const size_t size, word_t state[8]) {
	word_t datalength = 0;
	uint64_t bitlength = 0;
	word_t result[BLOCK_SIZE];
	unsigned int i;

	//compressing
	for (i = 0; i < size; i++) {
		result[datalength] = data[i];
		datalength++;
		if (datalength == BLOCK_SIZE) {
			sha512_compress(result, state);
			bitlength += BLOCK_SIZE_BITS;
			datalength = 0;
		}
	}

	i = datalength;

	//padding
	if (datalength < BLOCK_SIZE - 8) {
		result[i++] = 0x80;
		while (i < BLOCK_SIZE - 8) {
			result[i++] = 0;
		}
	} else {
		result[i++] = 0x80;
		while (i < BLOCK_SIZE) {
			result[i++] = 0;
		}
		sha512_compress(result, state);
		os_memset(result, 0, BLOCK_SIZE - 8);
	}

	//append
	bitlength += datalength * 8;
	for (i = 0; i < 8; i++) {
		result[BLOCK_SIZE - 1 - i] = bitlength >> (i * 8);
	}
	sha512_compress(result, state);
}

void sha512_256(const uint8_t *data, int size, uint8_t *hash) {
	word_t state[8] = {
		0x22312194fc2bf72c, 0x9f555fa3c84c64c2, 0x2393b86b6f53b151, 0x963877195940eabd,
		0x96283ee2a88effe3, 0xbe5e1e2553863992, 0x2b0199fc2c85b8aa, 0x0eb72ddc81c52ca2
	};
	unsigned int i, j;

	sha512_compute(data, size, state);

	for (i = 0; i < WORD_SIZE; i++) {
		for (j = 0; j < SHA512_256_HASH_SIZE / WORD_SIZE; j++) {
			hash[i + WORD_SIZE * j] = state[j] >> (WORD_SIZE_BITS - 8 - i * 8);
		}
	}
}
