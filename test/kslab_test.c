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

#ifndef PAGE_SIZE
#define PAGE_SIZE (getpagesize())
#endif

#define SMALL_MAX 32
#define BIG_MAX   (1024)
#define BIG_COUNT 16
#define DEBUGU(fmt,...)

unsigned long long
rdtsc(void)
{
	unsigned long long ret;
	__asm__ volatile ("rdtsc" : "=A" (ret));
	return ret;
}

struct A {
	char abc[SMALL_MAX];
};

struct B {
	char abc[BIG_MAX];
};

int
test_small_cache_oneobj(void)
{
	struct kmem_cache *kc;
	struct A *p, *q, *r;
	int i;
	int rd;


	kc = kmem_cache_create("test_small_cache1", sizeof(struct A), 0, NULL, NULL);
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
test_big_cache_oneobj(void)
{
	struct kmem_cache *kc;
	struct B *p, *q, *r;
	int i;
	int rd;

	kc = kmem_cache_create("test_big_cache1", sizeof(struct B), 0, NULL, NULL);
	assert(kc);
	
	for (i = 0 ; i < BIG_COUNT ; i ++) {
		p = (struct B *)kmem_cache_alloc(kc, KM_SLEEP);
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
test_small_cache_multiobj(void)
{
	unsigned long long start, end, alloc_time = 0, free_time = 0;
	struct kmem_cache *kc;
	struct A *p;
	struct A *q[200], *r[200];
	int i, sz;
	int rd;

	kc = kmem_cache_create("test_small_cache2", sizeof(struct A), 0, NULL, NULL);
	assert(kc);

	sz = PAGE_SIZE/kc->sz_aligned;

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
test_big_cache_multiobj(void)
{
	unsigned long long start, end, alloc_time = 0, free_time = 0;
	struct kmem_cache *kc;
	struct B *p;
	struct B *q[BIG_COUNT], *r[BIG_COUNT];
	int i, sz;
	int rd;

	kc = kmem_cache_create("test_big_cache2", sizeof(struct B), 0, NULL, NULL);
	assert(kc);

	sz = BIG_COUNT;
	for (i = 0 ; i < sz ; i ++) {
		start = rdtsc();
		q[i] = (struct B *)kmem_cache_alloc(kc, KM_SLEEP);
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
		r[i] = ((struct B *)kmem_cache_alloc(kc, KM_SLEEP));
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
	printf("sizeof A: %d\n", sizeof(struct A));
	printf("sizeof B: %d\n", sizeof(struct B));
	
	/* Testing small cache*/
	test_small_cache_oneobj();
	test_small_cache_multiobj();

	/* Testing big cache */
	test_big_cache_oneobj();
	test_big_cache_multiobj();

	return 0;
}
