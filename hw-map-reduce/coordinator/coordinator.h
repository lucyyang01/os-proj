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
  GHashTable* reassignTasks; //tasks that failed, to be reassigned
} coordinator;

typedef struct {
  int jobID;
  int n_reduce; //num tasks to reduce, decrememnt once we reduce one?
  u_int n_map; //num tasks to map == num files

  //don't mess with args?
	u_int args_len;
	char *args_val;

	char *app;
  path output_dir;
  
  //GList* taskIDs;
  GHashTable* mapTasks; //map taskid to file path
  //GHashTable* completionTimes; //does this need to be a map???
  //store list of tasks to be reassigned either per job (extra condition to check inside for loop)
  //can also store global list of tasks to be assigned in the state
  GHashTable* taskInfo; //list of tasks 
  int n_map_completed;
  int n_map_assigned;
  int n_reduce_completed;
  int n_reduce_assigned;
  bool done;
  bool failed;
} job;

typedef struct {
  time_t start_time;
  //struct timeval start_time;
  bool complete;
  bool reduce;
  int jobID; //id of corresponding job
  int taskID; //id of the task
  bool timeout;
  char* file; // will be null unless the task is a map task
  //char* output_dir;
} task;


void coordinator_init(coordinator** coord_ptr);
#endif
