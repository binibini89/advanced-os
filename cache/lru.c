/*
 * Implement an LFU replacement cache policy
 *
 * See the comments in gtcache.h for API documentation.
 *
 * The entry in the cache with the fewest hits since it entered the
 * cache (the most recent time) is evicted.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "gtcache.h"
/*
 * Include headers for data structures as you see fit
 */
#include "hshtbl.h"
#include "steque.h"
#include "indexminpq.h"
#include <string.h>
#include <sys/time.h>
#include <assert.h>

struct Entry {
	char *data;
	long time;
	char *url;
	int id;
	size_t val_sz;
};

struct timeval tv;

int max;

struct Entry * cache_entry;

hshtbl_t htable;

steque_t queue;

indexminpq_t priority_queue;

int compare_keys(void *key1, void* key2) {

	int key1_hits = *((int*)key1);
	int key2_hits = *((int*)key2);
	if (key1_hits > key2_hits) {
		return 1;
	}
	else if (key1_hits == key2_hits) {
		return 0;
	}
	else {
		return -1;
	}

}

int gtcache_init(size_t capacity, size_t min_entry_size, int num_levels){

	fprintf(stderr, "Entered init.\n");

	max = (capacity/min_entry_size);

	fprintf(stderr, "capacity = %zd\n max = %d\n", capacity, max);

	cache_entry = (struct Entry*) calloc(max, sizeof(struct Entry));

	int i;

	for (i = 0; i < max; i++) {
		cache_entry[i].data = NULL;
		cache_entry[i].url = NULL;
		cache_entry[i].id = 0;
		cache_entry[i].val_sz = 0;
	}

	hshtbl_init(&htable, 1000);

	steque_init(&queue);

	for (i = 1000; i > 0; i--) {
		steque_push(&queue, i);
	}

	indexminpq_init(&priority_queue, 1001, compare_keys);

	fprintf(stderr, "leaving init.\n");

	return 0;

}

void* gtcache_get(char *key, size_t *val_size){

	fprintf(stderr, "entering get.\n");

	char* res=NULL;
	void* tmp=NULL;
	void* cpy=NULL;
	long* time_temp;

	tmp = hshtbl_get(&htable, key);

	int i;

	if (tmp == NULL) {
		fprintf(stderr, "leaving get null.\n");
		return NULL;
	}
	else {
		for (i = 0; i < max; i++) {
			if (cache_entry[i].id == *((int* )tmp)) {

				gettimeofday(&tv, NULL);
				time_temp = malloc(sizeof(long));
				assert(time_temp);
				*time_temp = tv.tv_sec*1000000 + tv.tv_usec;
				cache_entry[i].time = *time_temp;

				res = (char*)malloc(strlen(cache_entry[i].data)+1);
				strcpy(res, cache_entry[i].data);
				cpy = malloc(sizeof(void*));
				cpy = res;
				indexminpq_changekey(&priority_queue, cache_entry[i].id, (indexminpq_key)time_temp);
				if (val_size != NULL) {
					val_size = &cache_entry[i].val_sz;
				}
				fprintf(stderr, "leaving get value.\n");
				return cpy;
			}
		}
	}

	fprintf(stderr, "leaving get other.\n");

	return NULL;

}

int gtcache_set(char *key, void *value, size_t val_size){

	fprintf(stderr, "entering set.\n");

	fprintf(stderr, "val_size = %zd\n", val_size);

	int i;
	indexminpq_key frt;
	long *tmp;

	if (indexminpq_size(&priority_queue) > max) {
		fprintf(stderr, "leaving set pq too big.\n");
		exit(1);
	}

	if (indexminpq_size(&priority_queue) < max) {
		for (i = 0; i < max; i++) {
			if (cache_entry[i].data == NULL) {
				cache_entry[i].data = calloc(1, val_size + 10);
				cache_entry[i].url = calloc(1, sizeof(key) + 10);
				//tmp = realloc( cache_entry, (max * sizeof(struct Entry)) + val_size + sizeof(key) );
				//cache_entry	= tmp;
				//usedMemory += val_size;
				cache_entry[i].val_sz = val_size;
				//tmp = &cache_entry[i].val_sz;
				cache_entry[i].data = value;
				//ptr[i] = cache_entry[i].data;
				//memcpy(cache_entry[i].data, value, val_size);
				cache_entry[i].url = key;
				cache_entry[i].id = steque_pop(&queue);

				gettimeofday(&tv, NULL);
				tmp = malloc(sizeof(long));
				assert(tmp);
				*tmp = tv.tv_sec*1000000 + tv.tv_usec;
				cache_entry[i].time = *tmp;

				hshtbl_put(&htable, key, &cache_entry[i].id);
				indexminpq_insert(&priority_queue, cache_entry[i].id, (indexminpq_key)tmp);
				fprintf(stderr, "leaving set size < max.\n");
				return 0;
			}
		}
	}

	frt = indexminpq_minkey(&priority_queue);

	if (indexminpq_size(&priority_queue) == max) {
		for (i = 0; i < max; i++) {
			if (cache_entry[i].time == *((long* )frt)) {
				hshtbl_delete(&htable, cache_entry[i].url);
				indexminpq_delete(&priority_queue, cache_entry[i].id);
				//usedMemory -= cache_entry[i].val_sz;
				//free(cache_entry[i].data);
				//free(cache_entry[i].url);
				//free(ptr[i]);
				cache_entry[i].data = NULL;
				cache_entry[i].url = NULL;
				//ptr[i] = NULL;
				//tmp = realloc( cache_entry, (max * sizeof(struct Entry)) + val_size + sizeof(key) + 10 );
				//cache_entry	= tmp;
				cache_entry[i].data = calloc(1, val_size + 10);
				cache_entry[i].url = calloc(1, sizeof(key) + 10);
				//usedMemory += val_size;
				cache_entry[i].val_sz = val_size;
				cache_entry[i].data = value;
				//ptr[i] = cache_entry[i].data;
				//memcpy(cache_entry[i].data, value, val_size);
				cache_entry[i].url = key;
				cache_entry[i].id = steque_pop(&queue);

				gettimeofday(&tv, NULL);
				tmp = malloc(sizeof(long));
				assert(tmp);
				*tmp = tv.tv_sec*1000000 + tv.tv_usec;
				cache_entry[i].time = *tmp;

				hshtbl_put(&htable, key, &cache_entry[i].id);
				indexminpq_insert(&priority_queue, cache_entry[i].id, (indexminpq_key)tmp);
				fprintf(stderr, "leaving set size == max.\n");
				return 0;
			}
		}
	}

	fprintf(stderr, "leaving set other.\n");

	return 1;
}

int gtcache_memused(){

	fprintf(stderr, "entering memused.\n");

	int i, sum=0;

	for (i = 0; i < max; i++) {
		sum += cache_entry[i].val_sz;
	}

	fprintf(stderr, "leaving memused. mem = %d\n", sum);

	return sum;

}

void gtcache_destroy(){

	fprintf(stderr, "entering cache destroy.\n");
/*
	int i;

	for (i = 0; i < max; i++) {
		if (cache_entry[i].data != NULL) {
		free(cache_entry[i].data);
		}
	}
*/

	free(cache_entry);
	steque_destroy(&queue);
	hshtbl_destroy(&htable);
	indexminpq_destroy(&priority_queue);

	fprintf(stderr, "leaving cache destroy.\n");

}
