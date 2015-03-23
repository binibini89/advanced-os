#include"rvm.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>


seqsrchst_t name2basest;

redo_t redo_log;

//char * redofilepath;

int compare_keys(seqsrchst_key a, seqsrchst_key b) {

	int x, y;

	x = *(int *)a;
	y = *(int *)b;

	if (x == y) {
		return 1;
	}
	else {
		return 0;
	}
}

/*
  Initialize the library with the specified directory as backing store.
*/
rvm_t rvm_init(const char *directory){

	fprintf(stderr, "entering rvm_init\n");

	rvm_t rvm;

	rvm = (rvm_t) malloc(sizeof(struct _rvm_t));

	redo_log = (redo_t) malloc(sizeof(struct _redo_t));

	int res;

	res = mkdir(directory, S_IRWXU);

	if (res != 0) {
		perror("mkdir()");
	}

	char * redofile = "/redofile";

	char * redofilepath = calloc(1, strlen(redofile) + strlen(directory) + 1);

	strcat(redofilepath, directory);
	strcat(redofilepath, redofile);

	rvm->redofd = open(redofilepath, O_RDWR | O_CREAT, S_IRWXU);

	redo_log->numentries = 0;

	seqsrchst_init(&rvm->segst, compare_keys);

	seqsrchst_init(&name2basest, compare_keys);

	strcpy(rvm->prefix, directory);

	//rvm->redofd = open(redofilepath, O_RDWR);

	close(rvm->redofd);

	fprintf(stderr, "leaving rvm_init\n");

	return rvm;

}
/*
  map a segment from disk into memory. If the segment does not already exist,
  then create it and give it size size_to_create. If the segment exists but is
  shorter than size_to_create, then extend it until it is long enough. It is an
  error to try to map the same segment twice.
*/
void *rvm_map(rvm_t rvm, const char *segname, int size_to_create){

	fprintf(stderr, "entering rvm_map\n");

	struct stat st;

	int fd;
	//void * buff;
	//buff = malloc(sizeof(size_to_create));

	void * tmp;
	segment_t seg;
	seqsrchst_key segnp = &segname;

	size_t sz = 0;

	//char * segfilepath;
	//segfilepath = calloc(1, strlen(segname) + strlen(rvm->prefix) + 1);

	if (stat(segname, &st) < 0) {
		seg = (segment_t) malloc(sizeof(struct _segment_t));
		strcpy(seg->segname, segname);
		//fd = open(seg->segname, O_RDWR | O_CREAT, S_IRWXU);
		seg->segbase = malloc(sizeof(size_to_create));
		seg->size = size_to_create;
		seg->cur_trans = NULL;
		seqsrchst_put(&rvm->segst, seg->segbase, seg);
		seqsrchst_put(&name2basest, seg->segname, seg->segbase);
		//pread(fd, seg, size_to_create, 0);
		//close(fd);

		//segfilepath = calloc(1, strlen(seg->segname) + strlen(rvm->prefix) + 1);

		//strcat(segfilepath, rvm->prefix);
		//strcat(segfilepath, seg->segname);

		fd = open(seg->segname, O_RDWR | O_CREAT, S_IRWXU);

		read(fd, seg->segbase, sz);

		truncate(seg->segname, size_to_create);

		close(fd);

		fprintf(stderr, "leaving rvm_map file doesn't exist\n");

		return seg->segbase;
	}
	else {
		tmp = seqsrchst_get(&name2basest, segnp);
		seg = seqsrchst_get(&rvm->segst, tmp);
		if (seg != NULL) {

			fprintf(stderr, "can't call map() consecutively\n");

			return NULL;
		}
		else {
			rvm_truncate_log(rvm);
			seg = (segment_t) calloc(1, sizeof(struct _segment_t));
			sz = (size_t) st.st_size;

			//segfilepath = calloc(1, strlen(seg->segname) + strlen(rvm->prefix) + 1);

			//strcat(segfilepath, rvm->prefix);
			//strcat(segfilepath, seg->segname);

			if (sz < size_to_create) {
				truncate(seg->segname, size_to_create);
				seg->size = size_to_create;
			}
			else {
				seg->size = (int)sz;
			}

			seg->segbase = malloc(seg->size);
			fd = open(seg->segname, O_RDWR);
			read(fd, seg->segbase, sz);
			//if (sz < size_to_create) {
				//truncate(seg->segname, size_to_create);
				//seg->size = size_to_create;
			//}
			//else {
				//seg->size = (int)sz;
			//}
			//seg->segbase = &seg;
			seg->cur_trans = NULL;
			seqsrchst_put(&rvm->segst, seg->segbase, seg);
			//seqsrchst_put(&name2basest, seg->segname, seg->segbase);
			close(fd);

			fprintf(stderr, "leaving rvm_map file exists\n");

			return seg->segbase;
		}
	}

	fprintf(stderr, "leaving rvm_map other\n");

	return seg->segbase;

}

/*
  unmap a segment from memory.
*/
void rvm_unmap(rvm_t rvm, void *segbase){

	fprintf(stderr, "entering rvm_unmap\n");

	segment_t val;

	val = seqsrchst_delete(&rvm->segst, segbase);

	memset(val->segname, 0, 128);
	val->size = 0;
	val->segbase = NULL;
	val = NULL;
	free(val);

	fprintf(stderr, "leaving rvm_unmap\n");

	return;

}

/*
  destroy a segment completely, erasing its backing store. This function should
  not be called on a segment that is currently mapped.
 */
void rvm_destroy(rvm_t rvm, const char *segname){

	fprintf(stderr, "entering rvm_destroy\n");

	void * tmp;
	segment_t seg;
	seqsrchst_key segnp = &segname;

	tmp = seqsrchst_get(&name2basest, segnp);
	seg = seqsrchst_get(&rvm->segst, tmp);

	if (seg == NULL) {
		remove(segname);
	}
	else {
		fprintf(stderr, "can't destroy segement that's  currently mapped\n");
	}

	fprintf(stderr, "leaving rvm_destroy\n");

	return;

}

/*
  begin a transaction that will modify the segments listed in segbases. If any of
  the specified segments is already being modified by a transaction, then the call
  should fail and return (trans_t) -1. Note that trant_t needs to be able to be
  typecasted to an integer type.
 */
trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases){

	fprintf(stderr, "entering rvm_begin_trans\n");

	int i;
	long reterr = -1;

	segment_t val[numsegs];
	//val = calloc(numsegs, sizeof(struct _segment_t));

	trans_t trans;
	trans = (trans_t) malloc(sizeof(struct _trans_t));

	for (i = 0; i < numsegs; i++) {
		val[i] = seqsrchst_get(&rvm->segst, segbases[i]);
		if (val[i]->cur_trans != NULL) {
			fprintf(stderr, "leaving rvm_begin_trans error\n");
			return (trans_t)reterr;
		}
		val[i]->cur_trans = trans;
	}

	trans->rvm = rvm;

	trans->numsegs = numsegs;

	trans->segments = calloc(numsegs, sizeof(segment_t));

	memcpy(&trans->segments[0], &val[0], sizeof(struct _segment_t));

	//fprintf(stderr, "trans->segments[0]->segname = %s\n", trans->segments[0]->segname);
/*
	for (i = 0; i < numsegs; i++) {
		fprintf(stderr, "trans->segments[i] = %p\n", trans->segments[i]);
		//fprintf(stderr, "val[i] = %p\n", val[i]);
		//trans->segments[i] = val[i];
		 memcpy(&trans->segments[i], &val[i], sizeof(struct _segment_t));
	}
*/
	fprintf(stderr, "leaving rvm_begin_trans success\n");

	return trans;

}

/*
  declare that the library is about to modify a specified range of memory in the
  specified segment. The segment must be one of the segments specified in the call
  to rvm_begin_trans. Your library needs to ensure that the old memory has been
  saved, in case an abort is executed. It is legal call rvm_about_to_modify multiple
  times on the same memory area.
*/
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size){

	fprintf(stderr, "entering rvm_about_to_modify\n");

	segment_t val;

	val = seqsrchst_get(&tid->rvm->segst, segbase);

	if (val->cur_trans == NULL) {
		fprintf(stderr, "must call begin_trans before about_to_modify\n");
		return;
	}

	mod_t * modi;
	modi = (mod_t*) calloc(1, sizeof(mod_t));

	//void * modp = &modi;

	modi->offset = offset;

	modi->size = size;

	modi->undo = calloc(1, sizeof(size));

	void * segbase_chg;

	segbase_chg = (void *)((intptr_t)segbase) + offset;

	memcpy(&modi->undo, &segbase_chg, size);

	steque_init(&val->mods);

	steque_push(&val->mods, (steque_item)modi);

	fprintf(stderr, "leaving rvm_about_to_modify\n");

	return;

}

/*
commit all changes that have been made within the specified transaction. When the call
returns, then enough information should have been saved to disk so that, even if the
program crashes, the changes will be seen by the program when it restarts.
*/
void rvm_commit_trans(trans_t tid){

	fprintf(stderr, "entering rvm_commit_trans\n");

	//fcntl(tid->rvm->redofd, F_SETLKW);


	mod_t* moda;
	//moda = (void**) malloc(sizeof(void*));

	void * memaddr;
	void * mvp;

	int i, j, totalu = 0;

	redo_log->entries = calloc(tid->numsegs, sizeof(segentry_t));


	segentry_t entry[tid->numsegs];

	for (i = 0; i < tid->numsegs; i++) {
		strcpy(entry[i].segname, tid->segments[i]->segname);
		entry[i].segsize = tid->segments[i]->size;
		entry[i].numupdates = 0;
		entry[i].updatesize = 0;
		entry[i].sizes = calloc(steque_size(&tid->segments[i]->mods), sizeof(int));
		entry[i].offsets = calloc(steque_size(&tid->segments[i]->mods), sizeof(int));
		entry[i].data = NULL;
	}


	for (i = 0; i < tid->numsegs; i++) {
		for (j = 0; j < steque_size(&tid->segments[i]->mods); j++) {
			moda = steque_front(&tid->segments[i]->mods);
			entry[i].offsets[j] = moda->offset;
			entry[i].sizes[j] = moda->size;
			entry[i].updatesize += entry[i].sizes[j];
			entry[i].numupdates++;
			steque_cycle(&tid->segments[i]->mods);
		}
		entry[i].data = malloc(sizeof(entry[i].updatesize));
		mvp = &entry[i].data;
		for (j = 0; j < entry[i].numupdates; j++) {
			memaddr = (void *)((intptr_t)tid->segments[i]->segbase) + entry[i].offsets[j];
			mvp = (void *)((intptr_t)mvp) + entry[i].sizes[j];
			memcpy(&mvp, &memaddr, entry[i].sizes[j]);
		}


		memcpy(&redo_log->entries[redo_log->numentries], &entry[i], sizeof(segentry_t));

		redo_log->numentries++;

		totalu += entry[i].updatesize;
	}

	size_t res;

	char * redofile = "/redofile";

	char * redofilepath = calloc(1, strlen(redofile) + strlen(tid->rvm->prefix) + 1);

	strcat(redofilepath, tid->rvm->prefix);
	strcat(redofilepath, redofile);

	tid->rvm->redofd = open(redofilepath, O_RDWR, S_IRWXU);

	res = pwrite(tid->rvm->redofd, redo_log, totalu, 0);

	close(tid->rvm->redofd);

	if (res <= 0) {
		fprintf(stderr, "error writing to redo log\n");
	}



	for (i = 0; i < tid->numsegs; i++) {
		tid->segments[i]->cur_trans = NULL;
	}

	//fcntl(tid->rvm->redofd, F_UNLCK);

	fprintf(stderr, "leaving rvm_commit_trans\n");

	return;

}

/*
  undo all changes that have happened within the specified transaction.
 */
void rvm_abort_trans(trans_t tid){

	fprintf(stderr, "entering rvm_abort_trans\n");

	int i, j;

	mod_t* moda;

	void * memaddr;

	for (i = 0; i < tid->numsegs; i++) {
		for (j = 0; j < steque_size(&tid->segments[i]->mods); j++) {
			moda = steque_front(&tid->segments[i]->mods);
			memaddr = (void *)((intptr_t)tid->segments[i]->segbase) + moda->offset;
			memcpy(&memaddr, &moda->undo, moda->size);
			steque_cycle(&tid->segments[i]->mods);
		}
	}

	for (i = 0; i < tid->numsegs; i++) {
		tid->segments[i]->cur_trans = NULL;
	}

	fprintf(stderr, "leaving rvm_abort_trans\n");

	return;

}

/*
 play through any committed or aborted items in the log file(s) and shrink the log file(s)
 as much as possible.
*/
void rvm_truncate_log(rvm_t rvm){

	fprintf(stderr, "entering rvm_truncate_log\n");

	//fcntl(rvm->redofd, F_SETLKW);

	int i, j, fd;

	void * mvp;

	struct stat st;

	size_t sz = 0;

	redo_t redob;

	//redob = calloc(1, sizeof(struct _redo_t));

	char * redofile = "/redofile";

	char * redofilepath = calloc(1, strlen(redofile) + strlen(rvm->prefix) + 1);

	strcat(redofilepath, rvm->prefix);
	strcat(redofilepath, redofile);

	rvm->redofd = open(redofilepath, O_RDWR, S_IRWXU);

	if (stat(redofilepath, &st) == 0) {
		sz = (size_t) st.st_size;
	}

	redob = calloc(1, sz);

	read(rvm->redofd, redob, sz);

	for (i = 0; i < redob->numentries; i++) {
		fd = open(redob->entries[i].segname, O_RDWR);

		fprintf(stderr, "redob->entries[i].segname = %s\n", redob->entries[i].segname);

		mvp = &redob->entries[i].data;
		for (j = 0; j < redob->entries[i].numupdates; j++) {
			if (j == 0) {
				pwrite(fd, redob->entries[i].data, redob->entries[i].sizes[j], redob->entries[i].offsets[j]);
			}
			else {
				mvp = (void *)((intptr_t)mvp) + redob->entries[i].sizes[j];
				pwrite(fd, mvp, redob->entries[i].sizes[j], redob->entries[i].offsets[j]);
			}
		}
		close(fd);
	}

	//fcntl(rvm->redofd, F_UNLCK);

	close(rvm->redofd);

	fprintf(stderr, "leaving rvm_truncate_log\n");

	return;

}
