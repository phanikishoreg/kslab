/*
 * Slab-allocator test program
 *
 * GPLv3
 * Copyright 2016 - Phani Gadepalli
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <kslab.h>

#define MAX 32
#define DEBUGU(fmt,...)

unsigned long long
rdtsc(void)
{
	unsigned long long ret;
	__asm__ volatile ("rdtsc" : "=A" (ret));
	return ret;
}

struct A {
	char abc[MAX];
};

int
test_cache1(void)
{
	struct kmem_cache *kc;
	struct A *p, *q, *r;
	int i;
	int rd;

	printf("sizeof A: %d\n", sizeof(struct A));

	kc = kmem_cache_create("hello", sizeof(struct A), 0, NULL, NULL);
	assert(kc);
	
	for (i = 0 ; i < kc->sz_aligned ; i ++) {
		p = (struct A *)kmem_cache_alloc(kc, KM_SLEEP);
		assert(p);

		rd = rand();
		printf("%s:%d\n", p->abc, rd);

		sprintf(p->abc, "%d", rd);

		kmem_cache_free(kc, p);
	}

	kmem_cache_destroy(kc);

	return 0;
}

int
test_cache2(void)
{
	unsigned long long start, end, alloc_time = 0, free_time = 0;
	struct kmem_cache *kc;
	struct A *p;
	struct A *q[200], *r[200];
	int i, sz;
	int rd;

	printf("sizeof A: %d\n", sizeof(struct A));

	kc = kmem_cache_create("hello", sizeof(struct A), 0, NULL, NULL);
	assert(kc);

//	q = (struct A *) malloc(kc->sz_aligned * sizeof(struct A *));
//	memset(q, 0, kc->sz_aligned * sizeof(struct A *));
	sz = getpagesize()/kc->sz_aligned;

	for (i = 0 ; i < sz ; i ++) {
		start = rdtsc();
		q[i] = (struct A *)kmem_cache_alloc(kc, KM_SLEEP);
		end = rdtsc();
		alloc_time += (end - start);

		rd = rand();

		sprintf(q[i]->abc, "%d", rd);
	}
	alloc_time /= sz;

	for ( ; i > 0 ; i --) {
		start = rdtsc();
		kmem_cache_free(kc, q[i - 1]);
		end = rdtsc();

		free_time += (end - start);
	}
	free_time /= sz;

	for (i = 0 ; i < sz ; i ++) {
		r[i] = ((struct A *)kmem_cache_alloc(kc, KM_SLEEP));
		assert(strlen(r[i]->abc) != 0);
		assert(q[i] == r[i]);
		assert(strcmp(q[i]->abc, r[i]->abc) == 0);
	}

	printf("Time taken in alloc: %llu free: %llu\n", alloc_time, free_time);
	//printf("trying after full\n");
	//kmem_cache_alloc(kc, KM_SLEEP);
	//r = (struct A *)kmem_cache_alloc(kc, KM_NOSLEEP);
	//assert(r == NULL);

	for ( ; i > 0 ; i --) {
		kmem_cache_free(kc, q[i - 1]);
	}
	kmem_cache_destroy(kc);

	return 0;
}

int
main(void)
{

	//test_cache1();
	test_cache2();

	return 0;
}
