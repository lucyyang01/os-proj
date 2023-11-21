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
  GList* jobs;
  GHashTable* jobInfo;
  int counter; //global counter for unique jobids
} coordinator;

typedef struct {
  int jobID;
  int n_reduce; //num tasks to reduce, decrememnt once we reduce one?
  u_int n_map; //num tasks to map == num files
  struct {
		u_int files_len; //num map tasks, decrement once we map one?
		path *files_val;
	} files;
  //don't mess with args?
  struct {
		u_int args_len;
		char *args_val;
	} args;

	char *app;
  path output_dir;
  //maintain a list of tasks within jobs
  //populate these lists within submit job?
  //parse thru the path string (commas) and use that info to fill out a task struct
  GList* mapTasks;
  GList* reduceTasks;
  //keep track of job status
  bool done;
  bool failed;
} job;

typedef struct {
  int taskID;
  path* file;
} task;



void coordinator_init(coordinator** coord_ptr);
#endif
