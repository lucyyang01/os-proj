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
  int n_map_assigned;
  int n_reduce_completed;
  int n_reduce_assigned;
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


/* TODO */
//   if (g_list_length(state->jobs) > 0) {
//     int* first_id = (int*) g_list_first(state->jobs);
//     job* first_job = g_hash_table_lookup(state->jobInfo, GINT_TO_POINTER(*first_id));
//     //there are map tasks left to be assigned
//     result.job_id = first_job->jobID;
//     result.output_dir = strdup(first_job->output_dir);
//     result.app = strdup(first_job->app);
//     result.args.args_len = first_job->args.args_len;
//     result.n_reduce = first_job->n_reduce;
//     result.n_map = first_job->n_map;
//     if (first_job->args.args_val != NULL) {
//       result.args.args_val = strdup(first_job->args.args_val);
//     } else {
//       result.args.args_val = ""; //or should it be null?
//     }
//     //if there's still map tasks to be assigned
//     if (first_job->n_map_assigned < first_job->n_map) {
//       char* task_file = g_hash_table_lookup(first_job->mapTasks, GINT_TO_POINTER(first_job->n_map_assigned));
//       result.task = first_job->n_map_assigned;
//       result.file = strdup(task_file);
//       result.reduce = false;
//       result.wait = false;
//       first_job->n_map_assigned += 1;
//       return &result;
//     }
//     //if all map tasks are completed, but not all reduce tasks are done
//     if(first_job->n_map_completed == first_job->n_map && first_job->n_reduce_completed < first_job->n_reduce) {
//       //are there any more reduce tasks to assign?
//       if(first_job->n_reduce_assigned < first_job->n_reduce) {
//         //assign the reduce task
//         result.task = first_job->n_reduce_assigned;
//         first_job->n_reduce_assigned += 1;
//         result.wait = false;
//         result.reduce = true;
//         return &result;
//       //all reduce tasks assigned, but not all of them have completed
//       } else {
//         //if there's another job on the queue, get it and assign a map
//         result.wait = true;
//         return &result;
//       }
//     }
//   }
//   return &result;
// }