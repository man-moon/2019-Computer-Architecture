## Project #3: Cache Simulator

### *** Due on 24:00, December 13 (Friday) ***

### Goal

Complete a cache simulator to better understand the memory hierarchy in computers.

### Problem Specification

- We've learned in the class that how the memory hierarchy works and how much it influences on computer performance. This programming assignment is to implement a cache simulator which supports set associative cache configuration and write-back feature.

- The provided framework is basically a command-line interface by which you can instruct the simulator to process your input. Followings are the supported commands that the simulator can handle.
  - `lw @addr`: Simulate `lw` instruction. **COMPLETE THIS FEATURE**
  - `sw @addr @value`: Simulate `sw` instruction. **COMPLETE THIS FEATURE**
  - `show`:  Show cache blocks and their status
  - `dump @starting_addr`: Dump 64 bytes from memory starting `@starting_addr`
  - `cycles`: Print out the number of simulated clock cycles
  - `quit`: Terminate the simulator

- You should complete the simulator to make `lw` and `sw` work correctly. To this end, you have to complete **`load_word`** and **`store_word`** in the template code.
  - `load_word` is called by the framework in response to `lw` command. You should loads data from memory into corresponding cache block.
  - `store_word` is called by the framework in response to `sw` command. You should updates data for `@addr` to `@data`.

- The cache should be the **write-back cache**.

- **Replace the least-recently-used cache block** for multi-way set-associative configuration. You can use `cycles` as the current timestamp.

- During the initialization, the simulator gets inputs for the cache configuration. Specifically, number of blocks `(nr_blocks)` is for the number of cache blocks and `number of ways (nr_ways)` is for the ways per set. Note that the cache is effectively the direct mapped cache when `nr_ways == 1` whereas the cache is effectively fully associative cache when `nr_ways == nr_blocks`. Also `nr_sets` is set according to the inputted cache configuration.

- The framework will populate `struct cache_block *cache` to hold the cache blocks. In your implementation, you can access cache blocks from `cache[0]` to `cache[nr_blocks - 1]` which are in `struct cache_block` type. **DO NOT ALTER THE ALLOCATION CODE**.

- The words per block might be changed during the test (e.g., to contain 2 words per block instead of 4 words default). Write your code to handle the case.

- You may use `printf` as you want, but **DO NOT** `fprintf` to `stderr`, since it can disturb the grading system.


### Hints

- Suppose you have `@addr` to handle and the cache has 2^n sets (i.e., `nr_sets == 1 << n`). To obtain `n`, you can use to  `log2_discrete(nr_sets)` in the framework. It is safe to assume that the number of blocks is always the power of 2 and so does for the number of ways and the words in a block.
- Following operations might help you calculating some addresses.
  - `1 << 8 = 0b 1 0000 0000`
  - `(1 << 8) - 1 = 0b 1111 11111`
  - `~((1 << 8) - 1) = 0b 1111 .... 1111 1111 0000 0000`
- Be careful about the orders of operations. It is highly recommended to put parentheses always if you are not sure.


### Submission / Grading

- Use [PAsubmit](https://sslab.ajou.ac.kr/pasubmit) for submission
	- 320 pts + 10 pts
	- Testcases will be available soon
	- Some testcases might wil be hidden

- Source: pa3.c (300 pts)
	- Load word (50 pts)
	- Store word (50 pts)
	- Write-back feature (100 pts)
	- LRU (100 pts)

- Document: one PDF document (20 pts) including;
	- How you implement the least-recently-used replacement policy
	- How `load_word` is handled (explain hit and miss cases separately)
	- How `store_word` is handled (explain hit and miss cases separately)
	- Lesson learned (if you have any)
	- No more than three pages

- Git repository (HTTP URL) at http://git.ajou.ac.kr (10 pts). Use deploy token and deploy password.
  - Make sure the deploy token is valid through December 18, 2pm.

- WILL NOT ANSWER THE QUESTIONS ABOUT THOSE ALREADY SPECIFIED ON THE HANDOUT.
