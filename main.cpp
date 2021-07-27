#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

const int BLOCK_SIZE_BYTES = 8;
const int SYNC_SIZE_BYTES = BLOCK_SIZE_BYTES; 
const int KEY_SIZE_BYTES = 8; 
const int S_SIZE_BYTES = BLOCK_SIZE_BYTES / 2;

const int C1 = 0x13101022; 
const int C2 = 0x12101015; 

//Таблица замен
const unsigned char table[8][16]
{
	4, 10, 9, 2, 13, 8, 0, 14, 6, 11, 1, 12, 7, 15, 5, 3,
	14, 11, 4, 12, 6, 13, 15, 10, 2, 3, 8, 1, 0, 7, 5, 9,
	5, 8, 1, 13, 10, 3, 4, 2, 14, 15, 12, 7, 6, 0, 9, 11,
	7, 13, 10, 1, 0, 8, 9, 15, 14, 4, 6, 12, 11, 2, 5, 3,
	6, 12, 7, 1, 5, 15, 13, 8, 4, 10, 9, 14, 0, 3, 11, 2,
	4, 11, 10, 0, 7, 2, 1, 13, 3, 6, 8, 5, 9, 12, 15, 14,
	13, 11, 4, 1, 3, 15, 5, 9, 0, 10, 14, 7, 6, 8, 2, 12,
	1, 15, 13, 0, 5, 7, 10, 4, 9, 2, 3, 14, 6, 11, 8, 12
};

union Block {
	unsigned char N[BLOCK_SIZE_BYTES];
	uint32_t t[2] = { 0 };
};

union Sync { 
	unsigned char sync[SYNC_SIZE_BYTES + 1] = "ibas2307"; 
	uint32_t S[2];
};

union S {
	unsigned char blocks[S_SIZE_BYTES];
	uint32_t sum;
};

union Key {
	unsigned char K[KEY_SIZE_BYTES + 1];
	unsigned char blocks[KEY_SIZE_BYTES];
};

uint32_t circular_shift_left(uint32_t a, int times);
uint32_t circular_shift_left_one(uint32_t a);
void c32z(uint32_t *N, Key K);
void main_step(uint32_t *N, unsigned char X);
void gamma(FILE* in, FILE* out, Key K, long file_length);

//Шифрование
void gamma(FILE* in, FILE* out, Key K, long file_length)
{
	Sync *sync = new Sync();
	c32z(sync->S, K);

	Block newBlock; 
	 
	for (int i = 0; i < file_length / BLOCK_SIZE_BYTES; i++)
	{
		for (int k = 0; k < BLOCK_SIZE_BYTES; k++)
			newBlock.N[k] = getc(in);

		sync->S[0] += C1; 
		sync->S[1] = (sync->S[1] + C2 - 1) % UINT32_MAX + 1;

		c32z(sync->S, K);

		newBlock.t[0] ^= sync->S[0];
		newBlock.t[1] ^= sync->S[1];

		for (int k = 0; k < BLOCK_SIZE_BYTES; k++)
			putc(newBlock.N[k], out);
	}

	for (int k = 0; k < file_length % BLOCK_SIZE_BYTES; k++)
		newBlock.N[k] = getc(in);

	if (file_length % BLOCK_SIZE_BYTES)
	{
		sync->S[0] += C1;
		sync->S[1] = (sync->S[1] + C2 - 1) % UINT32_MAX + 1;

		c32z(sync->S, K);

		newBlock.t[0] ^= sync->S[0];
		newBlock.t[1] ^= sync->S[1];
	}

	for (int k = 0; k < file_length % BLOCK_SIZE_BYTES; k++)
		putc(newBlock.N[k], out);
}

void c32z(uint32_t *N, Key K)
{
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 8; j++)
			main_step(N, K.blocks[j]);

	for (int j = 0; j < 8; j++)
		main_step(N, K.blocks[7 - j]);
}

void main_step(uint32_t *N, unsigned char X)
{
	S s; 
	s.sum = (N[0] + X);

	for (int m = 0; m < S_SIZE_BYTES; m++)
		s.blocks[m] = table[2 * m][s.blocks[m] % 16] + table[2 * m + 1][s.blocks[m] >> 4] << 4;

	s.sum = circular_shift_left(s.sum, 11);
	s.sum ^= N[1];
	N[1] = N[0];
	N[0] = s.sum;
}

uint32_t circular_shift_left(uint32_t a, int times)
{
	for (int i = 0; i < times; i++)
		a = circular_shift_left_one(a);
	return a;
}
uint32_t circular_shift_left_one(uint32_t a)
{
	int A = a >> 7;
	a = a << 1;
	a = a + A;

	return a;
}


int main()
{
	Key K;
	long file_length;

	//Щифрование
	FILE *in = fopen("text", "rb");
	fseek(in, 0, SEEK_END);
	file_length = ftell(in);
	rewind(in);

	FILE *out = fopen("out", "wb");

	FILE *key = fopen("key", "r");
	fscanf(key, "%s", K.K);
	fclose(key);

	gamma(in, out, K, file_length);

	fclose(in);
	fclose(out);

	//Дешифрование
	in = fopen("out", "rb");
	out = fopen("decrypted", "wb");

	gamma(in, out, K, file_length);

	fclose(in);
	fclose(out);
	return 0;
}