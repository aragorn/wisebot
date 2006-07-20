#include "common_core.h"
#include "common_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if HAVE_INTTYPES_H
#  include <inttypes.h>
#elif HAVE_STDINT_H
#  include <stdint.h>
#else
#  error need inttypes.h or stdint.h
#endif

#define DATA_COUNT 500000

typedef struct {
	int value;
	int dummy[6];
} data_t;

#define SWAP(a,b,t) \
	t = a; a = b; b = t;

void reverse_data(data_t* data, int count)
{
	int i, t;

	for ( i = 0; i < count/2; i++ ) {
		SWAP(data[i].value, data[count-i-1].value, t);
	}
}

void makedata_asc(data_t* data, int count)
{
	int i;

	for ( i = 0; i < count; i++ ) {
		data[i].value = i;
	}
}

// 톱날모양 분포
// 1,2,3,4,1,2,3,2,3,4,5,2,3,4...
void makedata_saw(data_t* data, int count)
{
	int i, c, r;

	c = 0;
	for ( i = 0; i < count; i++ ) {
		if ( c > count/1000 ) c = 0;

		r = rand() % 20 - 4;

		if ( r > 0 ) {
			if ( r > 10 ) r = 1;
		}
		else r = 0;
		c += r;

		data[i].value = c;
	}
}

void makedata_random1(data_t* data, int count)
{
	int i;

	for ( i = 0; i < count; i++ ) {
		data[i].value = rand() % (count/3*2);
	}
}

#define GETTIME(t) \
	gettimeofday(&tv, NULL); \
	t = tv.tv_sec * 1000000 + tv.tv_usec;

int cmp1(const void* dest, const void* sour, void* userdata)
{
	return ((data_t*) dest)->value - ((data_t*) sour)->value;
}

int cmp2(const void* dest, const void* sour)
{
	return ((data_t*) dest)->value - ((data_t*) sour)->value;
}

int main(int argc, char** argv)
{
	data_t* data;
	uint64_t time_old, time_new;
	struct timeval tv;
	int i, j, data_count;

	if ( argc > 1 ) {
		data_count = atoi(argv[1]);
		if ( data_count <= 0 ) {
			printf("invalid data_count: %s\n", argv[1]);
			return 1;
		}
	}
	else data_count = DATA_COUNT;

	srand(time(NULL));
	printf("data_count: %d\n", data_count);

	data = (data_t*) malloc(data_count*sizeof(data_t));

	for ( i = 0; i < 100; i++ ) {
		if ( i == 0 )
			printf("===================================\n");

		for ( j = 0; j < 100; j++ ) {
			const char *msg1, *msg2 = NULL;
			int break_now = 0;

			switch (i) {
				case 0:
					makedata_asc(data, data_count);
					msg1 = "asc";
					break;
				case 1:
					makedata_asc(data, data_count);
					reverse_data(data, data_count);
					msg1 = "desc";
					break;
				case 2:
					makedata_random1(data, data_count);
					msg1 = "random1";
					break;
				case 3:
					makedata_saw(data, data_count);
					msg1 = "saw";
					break;
				case 4:
					makedata_saw(data, data_count);
					reverse_data(data, data_count);
					msg1 = "saw rev";
					break;
				default:
					break_now = 1;
					break;
			}
			if ( break_now ) break;

			if ( j == 0 ) {
				printf("%s\n", msg1);
				printf("-----------------------------------\n");
			}

			GETTIME(time_old);
			switch (j) {
				case 0:
					qsort2(data, data_count, sizeof(data_t), NULL, cmp1);
					msg2 = "qsort2";
					break;
				case 1:
					qsort(data, data_count, sizeof(data_t), cmp2);
					msg2 = "qsort";
					break;
				case 2:
					mergesort(data, data_count, sizeof(data_t), cmp2);
					msg2 = "mergesort";
					break;
				default:
					break_now = 1;
					break;
			}
			if ( break_now ) break;

			GETTIME(time_new);
			printf("%9s: %6" PRIu64 " ms\n", msg2, (time_new-time_old)/1000);
		}

		if ( j > 0 )
			printf("===================================\n");
	}

	free(data);

	return 0;
}

