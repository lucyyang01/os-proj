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





// /* GET_TASK RPC implementation. */
// get_task_reply* get_task_1_svc(void* argp, struct svc_req* rqstp) {
//   static get_task_reply result;

//   printf("Received get task request\n");
//   result.file = "";
//   result.output_dir = "";
//   result.app = "";
//   result.wait = true;
//   result.args.args_len = 0;

//   /* TODO */
//   //no jobs to do
//   if(g_list_length(state->jobs) == 0) {
//     result.job_id = 0;
//     result.task = 0;
//     result.n_map = 0;
//     result.n_reduce = 0;
//     result.reduce = false;
//     result.args.args_val = "";
//     return &result;
//   }
//   //get the fifo job
//   int* first_id = (int*) g_list_first(state->jobs);
//   job* first_job = g_hash_table_lookup(state->jobInfo, GINT_TO_POINTER(*first_id));
//   if(first_job != NULL && first_job->done == false && first_job->failed == false) { 
//     result.job_id = first_job->jobID;
//     result.output_dir = strdup(first_job->output_dir);
//     result.app = strdup(first_job->app);
//     result.args.args_len = first_job->args.args_len;
//     if (first_job->args.args_val != NULL) {
//       result.args.args_val = strdup(first_job->args.args_val);
//     } else {
//         result.args.args_val = ""; //or should it be null?
//     }
//     if (first_job->n_map_completed < first_job->n_map) {
//       char* task_file = g_hash_table_lookup(first_job->mapTasks, GINT_TO_POINTER(first_job->n_map_completed));
//       result.task = first_job->n_map_completed;
//       //printf("task id: %d\n", first_job->n_map_completed);
//       result.file = strdup(task_file);
//       //printf("MADE IT HERE\n");
//       result.n_reduce = first_job->n_reduce;
//       result.n_map = first_job->n_map;
//       result.reduce = false;
//       result.wait = false;
//     } else {
//         if (first_job->n_reduce_completed < first_job->n_reduce) {
//           result.task = first_job->n_reduce_completed;
//           result.n_reduce = first_job->n_reduce;
//           result.n_map = first_job->n_map;
//           result.reduce = true;
//           result.wait = false;
//         } else {
//           result.job_id = 0;
//           result.task = 0;
//           result.n_map = 0;
//           result.n_reduce = 0;
//           result.reduce = false;
//           result.args.args_val = "";
//         }
//       }
//     } else {
//       result.job_id = 0;
//       result.task = 0;
//       result.n_map = 0;
//       result.n_reduce = 0;
//       result.reduce = false;
//       result.args.args_val = "";
//     }
//   //if no map and no reduce tasks left, assign wait to false

//   return &result;
// }