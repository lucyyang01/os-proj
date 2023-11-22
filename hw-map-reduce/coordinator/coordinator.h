/**
 * The MapReduce coordinator.
 */

#ifndef H1_H__
#define H1_H__
#include "../rpc/rpc.h"
#include "../lib/lib.h"
#include "../app/app.h"
#include "job.h"
#include <glib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef char *path;


typedef struct {
  /* TODO */
  GList* jobs; //queue of jobs
  GHashTable* jobInfo; //hashmap of jobs
  int counter; //global counter for unique jobids
} coordinator;

typedef struct {
  int jobID;
  int n_reduce; //num tasks to reduce, decrememnt once we reduce one?
  u_int n_map; //num tasks to map == num files

  //don't mess with args?
  struct {
		u_int args_len;
		char *args_val;
	} args;

	char *app;
  path output_dir;
  
  //GList* taskIDs;
  GHashTable* mapTasks; //map taskid to file path
  int n_map_completed;
  int n_reduce_completed;
  bool done;
  bool failed;
} job;

typedef struct {
  int taskID;
  char* file;
  bool reduce;
} task;



void coordinator_init(coordinator** coord_ptr);
#endif
