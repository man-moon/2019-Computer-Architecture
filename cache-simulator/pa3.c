/**********************************************************************
 * Copyright (c) 2019
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>

 /*====================================================================*/
 /*          ****** DO NOT MODIFY ANYTHING FROM THIS LINE ******       */
 /* To avoid security error on Visual Studio */
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)

enum cache_simulator_constants {
	CACHE_HIT = 0,
	CACHE_MISS,

	CB_INVALID = 0,	/* Cache block is invalid */
	CB_VALID = 1,	/* Cache block is valid */

	CB_CLEAN = 0,	/* Cache block is clean */
	CB_DIRTY = 1,	/* Cache block is dirty */

	BYTES_PER_WORD = 4,	/* This is 32 bit machine (1 word is 4 bytes) */
	MAX_NR_WORDS_PER_BLOCK = 32,	/* Maximum cache block size */
};


typedef unsigned char bool;
#define true  1
#define false 0

/* 8 KB Main memory */
static unsigned char memory[8 << 10] = {
	0xde, 0xad, 0xbe, 0xef, 0xba, 0xda, 0xca, 0xfe,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
	'h',  'e',  'l',  'l',  'o',  ' ' , 'w' , 'o',
	'r',  'l',  'd',  '!',  0x89, 0xab, 0xcd, 0xef,
	0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e,
	0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e,
	0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e,
	0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e,
};

/* Cache block */
struct cache_block {
	bool valid;				/* Whether the block is valid or invalid.
							   Use CB_INVALID or CB_VALID macro above  */
	bool dirty;				/* Whether the block is updated or not.
							   Use CB_CLEAN or CB_DIRTY macro above */
	unsigned int tag;		/* Tag */
	unsigned int timestamp;	/* Timestamp or clock cycles to implement LRU */
	unsigned char data[BYTES_PER_WORD * MAX_NR_WORDS_PER_BLOCK];
	/* Each block holds 4 words */
};

/* An 1-D array for cache blocks. */
static struct cache_block* cache = NULL;

/* The size of cache block. The value is set during the initialization */
static int nr_words_per_block = 4;

/* Number of cache blocks. The value is set during the initialization */
static int nr_blocks = 16;

/* Number of ways for the cache. Note @nr_ways == 1 means direct mapped cache
 * and @nr_ways == nr_blocks implies fully associative cache */
static int nr_ways = 2;

/* Number of @nr_ways-way sets in the cache. This value will be set according to
 * @nr_blocks and @nr_ways values */
static int nr_sets = 8;

/* Clock cycles */
const int cycles_hit = 1;
const int cycles_miss = 100;

/* Clock cycles so far */
static unsigned int cycles = 0;


/**
 * strmatch()
 *
 * DESCRIPTION
 *   Compare strings @str and @expect and return 1 if they are the same.
 *   You may use this function to simplify string matching :)
 *
 * RETURN
 *   1 if @str and @expect are the same
 *   0 otherwise
 */
static inline bool strmatch(char* const str, const char* expect)
{
	return (strlen(str) == strlen(expect)) && (strncmp(str, expect, strlen(expect)) == 0);
}

/**
 * log2_discrete
 *
 * DESCRIPTION
 *   Return the integer part of log_2(@n). FREE TO USE IN YOUR IMPLEMENTATION.
 *   Will be useful for calculating the length of tag for a given address.
 */
static int log2_discrete(int n)
{
	int result = -1;
	do {
		n = n >> 1;
		result++;
	} while (n);

	return result;
}
/*          ****** DO NOT MODIFY ANYTHING UP TO THIS LINE ******      */
/*====================================================================*/
int check_cache_hit(unsigned int tag, unsigned int index) {
	for (int i = nr_ways * index; i < nr_ways * (index + 1); i++) {
		if (cache[i].valid == CB_VALID && cache[i].tag == tag)
			return i;
	}
	return -1;
}

unsigned int find_cache_index(unsigned int index) {
	unsigned int cur_cache_index = -1;
	unsigned int min_timestamp = 0xfffffff;
	unsigned int min_timestamp_index;
	for (int i = nr_ways * index; i < (nr_ways * (index + 1)); i++) {
		if (cache[i].valid == false) {
			cur_cache_index = i;
			break;
		}
		if (min_timestamp > cache[i].timestamp) {
			min_timestamp = cache[i].timestamp;
			min_timestamp_index = i;
		}
	}
	if (cur_cache_index == -1)
		cur_cache_index = min_timestamp_index;

	return cur_cache_index;
}
/**************************************************************************
 * load_word
 *
 * DESCRIPTION
 *   Simulate the case when the processor is handling a lw instruction for @addr.
 *   To that end, you should look up the cache blocks to find the block
 *   containing the target address @addr. If exists, it's cache hit; return
 *   CACHE_HIT after updating the cache block's timestamp with @cycles.
 *   If not, replace the LRU cache block in the set. Should handle dirty blocks
 *   properly according to the write-back semantic.
 *
 * PARAMAMETERS
 *   @addr: Target address to load
 *
 * RETURN
 *   CACHE_HIT on cache hit, CACHE_MISS otherwise
 *
 */

int load_word(unsigned int addr)
{
	int nr_bits_index = log2_discrete(nr_sets);
	int nr_bits_offset = log2_discrete(BYTES_PER_WORD * nr_words_per_block);
	int nr_bits_tag = 32 - (nr_bits_index + nr_bits_offset);
	unsigned int tag = addr >> (32 - nr_bits_tag);
	unsigned int index = (addr << (nr_bits_tag)) >> (32 - nr_bits_index);
	unsigned int offset = (addr << (32 - nr_bits_offset)) >> (32 - nr_bits_offset);
	int result;



	//cache hit
	int o = check_cache_hit(tag, index);
	if (o != -1) {
		cache[o].timestamp = cycles;
		result = CACHE_HIT;
	}



	//cache miss
	else {
		//find a cache block to use.
		unsigned int cur_index = find_cache_index(index);

		if (cache[cur_index].dirty == CB_DIRTY) {
			//write its previous data back to the memory.
			unsigned int memory_addr = cache[cur_index].tag << (32 - nr_bits_tag);
			memory_addr += (cur_index / nr_ways) << nr_bits_offset;
			for (int i = 0; i < nr_words_per_block * BYTES_PER_WORD; i++) {
				memory[memory_addr + i] = cache[cur_index].data[i];
			}
		}
		cache[cur_index].valid = CB_INVALID;



		//load data from memory into the cache block.
		unsigned int tmp_addr = addr >> nr_bits_offset << nr_bits_offset;
		for (int i = 0; i < nr_words_per_block; i++) {
			unsigned int data = 0;
			for (int j = 0; j < 4; j++) {
				data += memory[tmp_addr + i * 4 + j] << (3 - j) * 8;
			}
			int offset = i * 4;
			for (int j = 0; j < 4; j++) {
				cache[cur_index].data[offset + j] = (char)(data >> (3 - j) * 8);
			}
		}



		cache[cur_index].dirty = CB_CLEAN;
		cache[cur_index].valid = CB_VALID;
		cache[cur_index].tag = tag;
		cache[cur_index].timestamp = cycles;
		result = CACHE_MISS;
	}
	return result;
}


/**************************************************************************
 * store_word
 *
 * DESCRIPTION
 *   Simulate the case when the processor is handling sw instruction.
 *   Cache should be write-back and write-allocate. Note that the least
 *   recently used (LRU) block should be replaced in case of eviction.
 *
 * PARAMETERS
 *   @addr: Starting address for @data
 *   @data: New value for @addr. @data is 1-word in size
 *
 * RETURN
 *   CACHE_HIT on cache hit, CACHE_MISS otherwise
 *
 */
int store_word(unsigned int addr, unsigned int data)
{
	int nr_bits_index = log2_discrete(nr_sets);
	int nr_bits_offset = log2_discrete(BYTES_PER_WORD * nr_words_per_block);
	int nr_bits_tag = 32 - (nr_bits_index + nr_bits_offset);
	unsigned int tag = addr >> (32 - nr_bits_tag);
	unsigned int index = (addr << (nr_bits_tag)) >> (32 - nr_bits_index);
	unsigned int offset = (addr << (32 - nr_bits_offset)) >> (32 - nr_bits_offset);
	int result;



	//cache hit
	int o = check_cache_hit(tag, index);
	if (o != -1) {
		//write the new data into the cache block.
		cache[o].data[offset] = data >> 24;
		cache[o].data[offset + 1] = data >> 16;
		cache[o].data[offset + 2] = data >> 8;
		cache[o].data[offset + 3] = data;

		cache[o].timestamp = cycles;
		cache[o].dirty = CB_DIRTY;
		result = CACHE_HIT;
	}



	//cache miss
	else {
		//find a cache block to use.
		unsigned int cur_index = find_cache_index(index);

		if (cache[cur_index].dirty == CB_DIRTY) {
			//write its previous data back to the memory.
			unsigned int memory_addr = cache[cur_index].tag << (32 - nr_bits_tag);
			memory_addr += (cur_index / nr_ways) << nr_bits_offset;
			for (int i = 0; i < nr_words_per_block * BYTES_PER_WORD; i++) {
				memory[memory_addr + i] = cache[cur_index].data[i];
			}
		}
		cache[cur_index].valid = CB_INVALID;



		//load data from memory into the cache block.
		unsigned int tmp_addr = addr >> nr_bits_offset << nr_bits_offset;
		for (int i = 0; i < nr_words_per_block; i++) {
			unsigned int data = 0;
			for (int j = 0; j < 4; j++) {
				data += memory[tmp_addr + i * 4 + j] << (3 - j) * 8;
			}
			int offset = i * 4;
			for (int j = 0; j < 4; j++) {
				cache[cur_index].data[offset + j] = (char)(data >> (3 - j) * 8);
			}
		}



		//write the new data into the cache block.
		cache[cur_index].data[offset] = data >> 24;
		cache[cur_index].data[offset + 1] = data >> 16;
		cache[cur_index].data[offset + 2] = data >> 8;
		cache[cur_index].data[offset + 3] = data;



		cache[cur_index].timestamp = cycles;
		cache[cur_index].dirty = CB_DIRTY;
		cache[cur_index].valid = CB_VALID;
		cache[cur_index].tag = tag;
		result = CACHE_MISS;
	}
	return result;
}


/**************************************************************************
 * init_simulator
 *
 * DESCRIPTION
 *   This function is called before starting the simulation. This is the
 *   perfect place to put your initialization code.
 */
void init_simulator(void)
{
	/* TODO: You may place your initialization code here */

}



/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING FROM THIS LINE ******       */
static void __show_cache(void)
{
	for (int i = 0; i < nr_blocks; i++) {
		fprintf(stderr, "[%3d] %c%c %8x %8u | ", i,
			cache[i].valid == CB_VALID ? 'v' : ' ',
			cache[i].dirty == CB_DIRTY ? 'd' : ' ',
			cache[i].tag, cache[i].timestamp);
		for (int j = 0; j < BYTES_PER_WORD * nr_words_per_block; j++) {
			fprintf(stderr, "%02x", cache[i].data[j]);
			if ((j + 1) % 4 == 0) fprintf(stderr, " ");
		}
		fprintf(stderr, "\n");
		if (nr_ways > 1 && ((i + 1) % nr_ways == 0)) printf("\n");
	}
}

static void __dump_memory(unsigned int start)
{
	for (int i = start; i < start + 64; i++) {
		if (i % 16 == 0) {
			fprintf(stderr, "[0x%08x] ", i);
		}
		fprintf(stderr, "%02x", memory[i]);
		if ((i + 1) % 4 == 0) fprintf(stderr, " ");
		if ((i + 1) % 16 == 0) fprintf(stderr, "\n");
	}
}

static void __init_cache(void)
{
	cache = (struct cache_block*)malloc(sizeof(struct cache_block) * nr_blocks);

	for (int i = 0; i < nr_blocks; i++) {
		struct cache_block* c = cache + i;

		c->valid = CB_INVALID;
		c->dirty = CB_CLEAN;
		c->tag = 0;
		c->timestamp = 0;
		memset(c->data, 0x00, sizeof(c->data));
	}
}

static void __fini_cache(void)
{
	free(cache);
}

static int __parse_command(char* command, int* nr_tokens, char* tokens[])
{
	char* curr = command;
	int token_started = false;
	*nr_tokens = 0;

	while (*curr != '\0') {
		if (isspace(*curr)) {
			*curr = '\0';
			token_started = false;
		}
		else {
			if (!token_started) {
				tokens[*nr_tokens] = curr;
				*nr_tokens += 1;
				token_started = true;
			}
		}
		curr++;
	}

	/* Exclude comments from tokens */
	for (int i = 0; i < *nr_tokens; i++) {
		if (strmatch(tokens[i], "//") || strmatch(tokens[i], "#")) {
			*nr_tokens = i;
			tokens[i] = NULL;
		}
	}

	return 0;
}

static void __simulate_cache(FILE* input)
{
	int argc;
	char* argv[10];
	char command[80];

	unsigned int hits = 0, misses = 0;

	__init_cache();
	if (input == stdin) printf(">> ");

	while (fgets(command, sizeof(command), input)) {
		unsigned int addr;
		int hit;

		__parse_command(command, &argc, argv);

		if (argc == 0) continue;

		if (strmatch(argv[0], "show")) {
			__show_cache();
			goto next;
		}
		else if (strmatch(argv[0], "dump")) {
			addr = argc == 1 ? 0 : strtoimax(argv[1], NULL, 0) & 0xfffffffc;
			__dump_memory(addr);
			goto next;
		}
		else if (strmatch(argv[0], "cycles")) {
			fprintf(stderr, "%3u %3u   %u\n", hits, misses, cycles);
			goto next;
		}
		else if (strmatch(argv[0], "quit")) {
			break;
		} if (strmatch(argv[0], "lw")) {
			if (argc == 1) {
				printf("Wrong input for lw\n");
				printf("Usage: lw <address to load>\n");
				goto next;
			}
			addr = strtoimax(argv[1], NULL, 0);
			hit = load_word(addr);
		}
		else if (strmatch(argv[0], "sw")) {
			if (argc != 3) {
				printf("Wrong input for sw\n");
				printf("Usage: sw <address to store> <word-size value to store>\n");
				goto next;
			}
			addr = strtoimax(argv[1], NULL, 0);
			hit = store_word(addr, strtoimax(argv[2], NULL, 0));
		}
		else {
			goto next;
		}

		if (hit == CACHE_HIT) {
			hits++;
			cycles += cycles_hit;
		}
		else {
			misses++;
			cycles += cycles_miss;
		}
	next:
		if (input == stdin) printf(">> ");
	}

	__fini_cache();
}

int main(int argc, const char* argv[])
{
	FILE* input = stdin;

	if (argc > 1) {
		input = fopen(argv[1], "r");
		if (!input) {
			perror("Input file error");
			return EXIT_FAILURE;
		}
	}


	if (input == stdin) {
		printf("*****************************************************\n");
		printf("*                    _                              *\n");
		printf("*      ___ __ _  ___| |__   ___                     *\n");
		printf("*     / __/ _` |/ __| '_ \\ / _ \\                    *\n");
		printf("*    | (_| (_| | (__| | | |  __/                    *\n");
		printf("*     \\___\\__,_|\\___|_| |_|\\___|                    *\n");
		printf("*    	 _                 _       _                *\n");
		printf("*     ___(_)_ __ ___  _   _| | __ _| |_ ___  _ __   *\n");
		printf("*    / __| | '_ ` _ \\| | | | |/ _` | __/ _ \\| '__|  *\n");
		printf("*    \\__ \\ | | | | | | |_| | | (_| | || (_) | |     *\n");
		printf("*    |___/_|_| |_| |_|\\__,_|_|\\__,_|\\__\\___/|_|     *\n");
		printf("*                                                   *\n");
		printf("*                                   2019.12         *\n");
		printf("*****************************************************\n\n");
	}

#ifndef _USE_DEFAULT
	if (input == stdin) printf("- words per block:  ");
	fscanf(input, "%d", &nr_words_per_block);
	if (input == stdin) printf("- number of blocks: ");
	fscanf(input, "%d", &nr_blocks);
	if (input == stdin) printf("- number of ways:   ");
	fscanf(input, "%d", &nr_ways);

	nr_sets = nr_blocks / nr_ways;
#endif

	init_simulator();
	__simulate_cache(input);

	if (input != stdin) fclose(input);

	return EXIT_SUCCESS;
}
