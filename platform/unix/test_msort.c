/* $Id$ */
#include "softbot.h"
#include "../generic/msort.h"

int comp(const void *v, const void *u)
{
	if ( *(int*)v > *(int*)u )
		return 1;
	else if ( *(int*)v < *(int*)u )
		return -1;
	else
		return 0;
}

void test_simple_array()
{
	int a[10]={9,128,1,2,5,2,10,11,15,25};
	int i=0;

	printf("original: ");
	for (i=0; i<10; i++) {
		printf("%d, ",a[i]);
	}

	mergesort(a,10,sizeof(int),&comp);

	printf("\nsorted: ");
	for (i=0; i<10; i++) {
		printf("%d, ",a[i]);
	}

	printf("\n");
}

typedef struct {
	int v1;
	char v2;
	long realval;
	void *dummy;
} complex_stuct;

int comp2(const void *v, const void *u)
{
	if ( ((complex_stuct*)v)->realval > ((complex_stuct*)u)->realval )
		return 1;
	else if ( ((complex_stuct*)v)->realval < ((complex_stuct*)u)->realval )
		return -1;
	else
		return 0;

}

void test_structure_array()
{
	complex_stuct a[20];
	long v[20]={10,2,5,16,45,2,7,25,87,43,349,65,10,6,11,10,10,66,17,3000};
	int i=0;

	for (i=0; i<20; i++) {
		a[i].realval=v[i];
	}

	printf("\noriginal: ");
	for (i=0; i<20; i++) {
		printf("%ld, ",a[i].realval);
	}
	printf("\n");

	mergesort(a,20,sizeof(complex_stuct),&comp2);

	printf("sorted: ");
	for (i=0; i<20; i++) {
		printf("%ld, ",a[i].realval);
	}
	printf("\n");
}

void cmp_qsort_msort()
{
#define MAX_VALUE 10000
#define AR_SIZE 500
	int a[AR_SIZE];
	int b[AR_SIZE];
	int i=0;
	struct timeval t1,t2;

	for (i=0; i<AR_SIZE; i++) {
		a[i] = 1+(int) (MAX_VALUE*rand()/(RAND_MAX+1.0));
	}
	memcpy(b,a,sizeof(int)*AR_SIZE);

	gettimeofday(&t1,NULL);
	mergesort(b,AR_SIZE,sizeof(int),&comp);
	gettimeofday(&t2,NULL);

	printf("msort time: %f sec \n",
			(double)(t2.tv_sec-t1.tv_sec+(double)(t2.tv_usec-t1.tv_usec)/1000000));

	gettimeofday(&t1,NULL);
	qsort(a,AR_SIZE,sizeof(int),&comp);
	gettimeofday(&t2,NULL);

	printf("qsort time: %f sec \n",
			(double)(t2.tv_sec-t1.tv_sec+(double)(t2.tv_usec-t1.tv_usec)/1000000));
}

int main()
{
	test_simple_array();
	test_structure_array();
	cmp_qsort_msort();
	return 0;
}
