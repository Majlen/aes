/**
 * aes.c
 * AES encryptor/decryptor
 * 
 * Uses PKCS#7 padding and Cipher Block Chaining
 *
 * @author Milan Sevcik (majlen@civ.zcu.cz)
 */

#define _POSIX_C_SOURCE 2

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

//Function definitions
void help();
void Cipher();
void Decipher();
void KeyExpansion();
void ShiftRows();
void SubBytes();
void MixColumns();
void AddRoundKey(uint8_t*);
void InvShiftRows();
void InvSubBytes();
void InvMixColumns();
uint32_t SubWord(uint32_t);
uint32_t RotWord(uint32_t);
uint8_t xtime(uint8_t);

//Bytes containing current state in every step
uint8_t state[16];
//S-Box
uint8_t s_box[256] = {
	0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
	0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
	0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
	0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
	0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
	0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
	0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
	0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
	0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
	0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
	0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
	0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
	0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
	0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
	0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
	0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};
//Inverse S-Box
uint8_t invs_box[256] = {
	0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
	0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
	0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
	0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
	0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
	0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
	0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
	0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
	0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
	0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
	0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
	0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
	0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
	0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D, 0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
	0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
	0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D
};
//Key - In the case of AES256 the expanded key is 240B long (or 60 words)
uint8_t key[240];
//Key length in 32b words - default is for AES128
uint8_t keyWords = 4;
//Rounds of cipher - default is for AES128
uint8_t rounds = 10;

/**
 * Entry point of program
 * @param argc argument count
 * @param argv arguments processed by getopt
 * @returns Return code of program (0 - success, 1 - failure)
 */
int main(int argc, char* argv[]) {
	void (*aesfunc)() = NULL;
	FILE* keyfile = NULL;
	FILE* infile = stdin;
	
	char c;
	while ((c = getopt(argc, argv, "hcdk:b:i:")) != -1 ) {
		switch (c) {
			case 'h':
				//Print help
				help();
				return 0;
			case 'c':
				//Cipher
				aesfunc = Cipher;
				break;
			case 'd':
				//Decipher
				aesfunc = Decipher;
				break;
			case 'k':
				//Keyfile
				keyfile = fopen(optarg, "rb");
				break;
			case 'b':
				//Bit length
			{
				int bits = (int)strtol(optarg, NULL, 10);
				if (bits == 192) {
					keyWords = 6;
					rounds = 12;
				} else if (bits == 256) {
					keyWords = 8;
					rounds = 14;
				} else if (bits != 128) {
					fprintf(stderr, "Supported is only AES128, AES192 and AES256. Quitting.\n");
					return 1;
				}
			}
			break;
			case 'i':
				//Input file
				infile = fopen(optarg, "rb");
				break;
		}
	}
	
	if (keyfile == NULL) {
		fprintf(stderr, "There is no keyfile specified. Quitting.\n");
		return 1;
	}
	
	//Obtain key from file and generate all round keys
	if (fread(key, 4, keyWords, keyfile) != keyWords) {
		fprintf(stderr, "Error reading key.\n");
		return 1;
	}
	fclose(keyfile);
	KeyExpansion();
	
	//Random initialization vector for CBC block mode
	uint8_t iv[16];
	if (aesfunc == Cipher) {
		srand((unsigned int)time(NULL));
		for (int i = 0; i < 16; i += sizeof(int)) {
			union {
				int xbytes;
				uint8_t byte[sizeof(int)];
			} temp;
			temp.xbytes = rand();
			for (int j = 0; j < sizeof(int); j++) {
				iv[i+j] = temp.byte[j];
				putc(iv[i+j], stdout);
			}
		}
	} else {
		for (int i = 0; i < 16; i++) {
			int byte = getc(infile);
			if (byte >= 0)
				iv[i] = byte;
			else {
				fprintf(stderr, "Data corruption - size incorrect, it should always be multiple of 16 bytes. Quitting.\n");
				return 1;
			}
		}
	}
	
	state[0] = getc(infile);
	if (state[0] < 0) {
		fprintf(stderr, "Data corruption - size incorrect, it should always be multiple of 16 bytes. Quitting.\n");
		return 1;
	}
	
	int finished = 0;
	while (!finished) {
		//Fill the block
		for (int i = 1; i < 16; i++) {
			int byte = getc(infile);
			if (byte >= 0)
				state[i] = byte;
			else {
				//End of stream
				if (aesfunc == Cipher) {
					//Add padding PKCS#7
					if ((i == 1) && (state[0] == 16)) {
						for (int j = i; j < 16; j++) {
							state[j] = 16;
						}
					} else {
						for (int j = i; j < 16; j++) {
							state[j] = 16-i;
						}
					}
				} else {
					fprintf(stderr, "Data corruption - size incorrect, it should always be multiple of 16 bytes. Quitting.\n");
					return 1;
				}
				finished = 1;
				break;
			}
		}
		
		//Step of CBC - XOR message with IV
		uint8_t nextiv[16];
		if (aesfunc == Cipher) {
			for (int i = 0; i < 16; i++) {
				state[i] = state[i] ^ iv[i];
			}
		} else {
			memcpy(nextiv, state, 16);
		}
		
		aesfunc();
		
		int temp = getc(infile);
		
		int bytes_to_print = 16;
		//Step of CBC - XOR deciphered message with IV
		if (aesfunc == Decipher) {
			for (int i = 0; i < 16; i++) {
				state[i] = state[i] ^ iv[i];
			}
			memcpy(iv, nextiv, 16);
			
			if (temp < 0) {
				finished = 1;
				
				//Delete padding - it should always be present
				if ((state[15] <= 16) && (state[15] > 0)) {
					for (int i = 1; i < state[15]; i++) {
						if (state[15-i] != state[15]) {
							fprintf(stderr, "Data corruption - padding incorrect. Quitting.\n");
							return 1;
						}
					}
					bytes_to_print = 16-state[15];
				} else {
					fprintf(stderr, "Data corruption - padding incorrect. Quitting.\n");
					return 1;
				}
			}
		} else {
			memcpy(iv, state, 16);
		}
		
		//Print the block
		for (int i = 0; i < bytes_to_print; i++) {
			putc(state[i], stdout);
			//printf("%x ", state[i]);
			//if ((i+1) % 4 == 0) {
			//	printf("\n");
			//}
		}
		
		if (temp < 0) {
			state[0] = 16;
		} else {
			state[0] = temp;
		}
	}
	
	if (infile != stdin) {
		fclose(infile);
	}
	return 0;
}

/**
 * Prints help
 */
void help() {
	printf("Usage:\n");
	printf("-h\tPrint (this) help\n");
	printf("-c\tCipher mode\n");
	printf("-d\tDecipher mode\n");
	printf("-k\tKey file\n");
	printf("-b\tBit length (supported 128, 192, 256 - default 128)\n");
	printf("-i\tInput file (default stdin)\n");
}

/**
 * Function performing encryption
 */
void Cipher() {
	//Can omit the index - first 16B are used
	AddRoundKey(key);
	
	//Last round omits the MixColumns step - starting with 1
	for (int i = 1; i < rounds; i++) {
		SubBytes();
		ShiftRows();
		MixColumns();
		AddRoundKey(&key[i*16]);
	}
	
	//Last round
	SubBytes();
	ShiftRows();
	AddRoundKey(&key[rounds*16]);
}

/**
 * Function performing decryption
 */
void Decipher() {
	AddRoundKey(&key[rounds*16]);
	
	//Last round omits the InvMixColumns step - ending with 1
	for (int i = rounds-1; i > 0; i--) {
		InvShiftRows();
		InvSubBytes();
		AddRoundKey(&key[i*16]);
		InvMixColumns();
	}
	
	//Last round
	InvShiftRows();
	InvSubBytes();
	AddRoundKey(key);
}

/**
 * Function performing 32 bit word substitution using S-Box, used in KeyExpansion
 * @param word word to be substituted
 * @returns substituted word
 */
inline uint32_t SubWord(uint32_t word) {
	uint8_t* wordB = (uint8_t*)&word;
	for (int i = 0; i < 4; i++) {
		wordB[i] = s_box[wordB[i]];
	}
	
	return word;
}

/**
 * Function performing 32 bit word rotation by 8 bits, used in KeyExpansion
 * @param word word to be rotated
 * @returns rotated word
 */
inline uint32_t RotWord(uint32_t word) {
	return (word << 8) | (word >> (32-8));
}

/**
 * Key expansion process
 */
void KeyExpansion() {
	//Round constant
	//FIXME: Endians
	union {
		uint32_t word;
		uint8_t byte[4];
	} Rcon;
	uint32_t temp;
	Rcon.word = 0;
	//Rcon.byte[0] = 1;
	Rcon.byte[3] = 1;
	uint32_t* key32 = (uint32_t*)key;
	
	for (int i = keyWords; i < 4*(rounds+1); i++) {
		
		temp = htonl(key32[i-1]);
		
		if ((i % keyWords) == 0) {
			temp = SubWord(RotWord(temp)) ^ Rcon.word;
			Rcon.byte[3] = xtime(Rcon.byte[3]);
		} else if ((keyWords > 6) && ((i % keyWords) == 4)) {
			temp = SubWord(temp);
		}
		
		temp = htonl(key32[i-keyWords]) ^ temp;
		key32[i] = ntohl(temp);
	}
}

/**
 * Phase of the algorithm rotating rows of state
 * Row 0 is not rotated
 * Row 1 is rotated by 1 byte to the left
 * Row 2 is rotated by 2 bytes to the left
 * Row 3 is rotated by 3 bytes to the left (or 1 byte to the right)
 */
void ShiftRows() {
	uint8_t temp1, temp2;
	
	//Row 2
	temp1 = state[1];
	for (int i = 0; i < 3; i++) {
		state[1+i*4] = state[1+(1+i)*4];
	}
	state[13] = temp1;
	
	//Row 3
	temp1 = state[2];
	temp2 = state[6];
	for (int i = 0; i < 2; i++) {
		state[2+i*4] = state[2+(2+i)*4];
	}
	state[10] = temp1;
	state[14] = temp2;
	
	//Row 4 (shift left by 3 is the same as shift right by 1)
	temp1 = state[15];
	for (int i = 2; i >= 0; i--) {
		state[3+(1+i)*4] = state[3+i*4];
	}
	state[3] = temp1;
}

/**
 * Phase of the algorithm substituting bytes of state with bytes of S-Box
 */
void SubBytes() {
	for (int i = 0; i < 16; i++) {
		state[i] = s_box[state[i]];
	}
}

/**
 * Phase of the algorithm mixing columns of state
 * For more info see NIST AES specification
 */
void MixColumns() {
	for (int i = 0; i < 4; i++) {
		uint8_t temp[4];
		memcpy(temp, &(state[i*4]), 4);
		state[i*4] = xtime(temp[0]) ^ (xtime(temp[1]) ^ temp[1]) ^ temp[2] ^ temp[3];
		state[i*4+1] = temp[0] ^ xtime(temp[1]) ^ (xtime(temp[2]) ^ temp[2]) ^ temp[3];
		state[i*4+2] = temp[0] ^ temp[1] ^ xtime(temp[2]) ^ (xtime(temp[3]) ^ temp[3]);
		state[i*4+3] = (xtime(temp[0]) ^ temp[0]) ^ temp[1] ^ temp[2] ^ xtime(temp[3]);
	}
}

/**
 * Phase of the algorithm XORing state with round key
 * @param key round key used in this phase
 */
void AddRoundKey(uint8_t* key) {
	for (int i = 0; i < 16; i++) {
		state[i] = state[i] ^ key[i];
	}
}

/**
 * Inverse function to ShiftRows, rotating rows of state
 * Row 0 is not rotated
 * Row 1 is rotated by 1 byte to the right
 * Row 2 is rotated by 2 bytes to the right (or 2 bytes to the left)
 * Row 3 is rotated by 3 bytes to the right (or 1 byte to the left)
 */
void InvShiftRows() {
	uint8_t temp1, temp2;
	
	//Row 2
	temp1 = state[13];
	for (int i = 2; i >= 0; i--) {
		state[1+(i+1)*4] = state[1+i*4];
	}
	state[1] = temp1;
	
	//Row 3
	temp1 = state[2];
	temp2 = state[6];
	for (int i = 0; i < 2; i++) {
		state[2+i*4] = state[2+(2+i)*4];
	}
	state[10] = temp1;
	state[14] = temp2;
	
	//Row 4 (shift right by 3 is the same as shift left by 1)
	temp1 = state[3];
	for (int i = 0; i < 3; i++) {
		state[3+i*4] = state[3+(i+1)*4];
	}
	state[15] = temp1;
}

/**
 * Inverse function to SubBytes
 * Substituting bytes of state with bytes of Inverse S-Box
 */
void InvSubBytes() {
	for (int i = 0; i < 16; i++) {
		state[i] = invs_box[state[i]];
	}
}

/**
 * Inverse function to MixColumns
 * For More info see NIST AES specification
 */
void InvMixColumns() {
	for (int i = 0; i < 4; i++) {
		uint8_t lookup[4][4];
		for (int j = 0; j < 4; j++) {
			lookup[j][0] = state[i*4+j];
			lookup[j][1] = xtime(lookup[j][0]);
			lookup[j][2] = xtime(lookup[j][1]);
			lookup[j][3] = xtime(lookup[j][2]);
		}
		
		state[i*4] = (lookup[0][3] ^ lookup[0][2] ^ lookup[0][1]) ^ (lookup[1][3] ^ lookup[1][1] ^ lookup[1][0]) ^ (lookup[2][3] ^ lookup[2][2] ^ lookup[2][0]) ^ (lookup[3][3] ^ lookup[3][0]);
		state[i*4+1] = (lookup[0][3] ^ lookup[0][0]) ^ (lookup[1][3] ^ lookup[1][2] ^ lookup[1][1]) ^ (lookup[2][3] ^ lookup[2][1] ^ lookup[2][0]) ^ (lookup[3][3] ^ lookup[3][2] ^ lookup[3][0]);
		state[i*4+2] = (lookup[0][3] ^ lookup[0][2] ^ lookup[0][0]) ^ (lookup[1][3] ^ lookup[1][0]) ^ (lookup[2][3] ^ lookup[2][2] ^ lookup[2][1]) ^ (lookup[3][3] ^ lookup[3][1] ^ lookup[3][0]);
		state[i*4+3] = (lookup[0][3] ^ lookup[0][1] ^ lookup[0][0]) ^ (lookup[1][3] ^ lookup[1][2] ^ lookup[1][0]) ^ (lookup[2][3] ^ lookup[2][0]) ^ (lookup[3][3] ^ lookup[3][2] ^ lookup[3][1]);
	}
}

/**
 * Function used to make multiplication in Gaulois field (finite field),
 * multitplies number by 2
 * @param num number to be multiplied by 2
 * @returns number multiplied by 2
 */
inline uint8_t xtime(uint8_t num) {
	if (num / 128 == 0)
		return (num << 1);
	else
		return ((num << 1) ^ 0x1b);
}
