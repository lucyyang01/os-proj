/**
 * The MapReduce coordinator.
 */

#include "coordinator.h"

#ifndef SIG_PF
#define SIG_PF void (*)(int)
#endif

/* Global coordinator state. */
coordinator* state;

extern void coordinator_1(struct svc_req*, SVCXPRT*);

/* Set up and run RPC server. */
int main(int argc, char** argv) {
  register SVCXPRT* transp;

  pmap_unset(COORDINATOR, COORDINATOR_V1);

  transp = svcudp_create(RPC_ANYSOCK);
  if (transp == NULL) {
    fprintf(stderr, "%s", "cannot create udp service.");
    exit(1);
  }
  if (!svc_register(transp, COORDINATOR, COORDINATOR_V1, coordinator_1, IPPROTO_UDP)) {
    fprintf(stderr, "%s", "unable to register (COORDINATOR, COORDINATOR_V1, udp).");
    exit(1);
  }

  transp = svctcp_create(RPC_ANYSOCK, 0, 0);
  if (transp == NULL) {
    fprintf(stderr, "%s", "cannot create tcp service.");
    exit(1);
  }
  if (!svc_register(transp, COORDINATOR, COORDINATOR_V1, coordinator_1, IPPROTO_TCP)) {
    fprintf(stderr, "%s", "unable to register (COORDINATOR, COORDINATOR_V1, tcp).");
    exit(1);
  }

  coordinator_init(&state);

  svc_run();
  fprintf(stderr, "%s", "svc_run returned");
  exit(1);
  /* NOTREACHED */
}

/* EXAMPLE RPC implementation. */
int* example_1_svc(int* argp, struct svc_req* rqstp) {
  static int result;

  result = *argp + 1;

  return &result;
}

/* SUBMIT_JOB RPC implementation. */
int* submit_job_1_svc(submit_job_request* argp, struct svc_req* rqstp) {
  static int result;

  printf("Received submit job request\n");

  /* TODO */
  //validate provided application name
  app valid = get_app(argp->app);
  if (valid.name == NULL)
    return (int*) -1;

  //create a job
  job* new_job = malloc(sizeof(job));
  new_job->jobID = state->counter;
  new_job->app = strdup(argp->app);
  new_job->n_map = argp->files.files_len;
  new_job->mapTasks = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
  new_job->n_map_completed = 0;
  new_job->n_map_assigned = 0;
  new_job->n_reduce_assigned = 0;
  new_job->n_reduce_completed = 0;
  for (int i = 0; i < argp->files.files_len; i++) {
    char* dup = strdup(argp->files.files_val[i]);
    g_hash_table_insert(new_job->mapTasks, GINT_TO_POINTER(i), dup);
  }
  new_job->output_dir = strdup(argp->output_dir);

  new_job->args.args_len = argp->args.args_len;
  if(argp->args.args_val != NULL) {
    new_job->args.args_val = strdup(argp->args.args_val);
  } else {
    new_job->args.args_val = "";
  }
  new_job->n_reduce = argp->n_reduce;
  new_job->done = false;
  new_job->failed = false;

  //increment counter for next job's id
  state->counter += 1;

  //add to queue in fcfs order
  state->jobs = g_list_append(state->jobs, GINT_TO_POINTER(new_job->jobID));

  //add to jobinfo hashtable
  g_hash_table_insert(state->jobInfo, GINT_TO_POINTER(new_job->jobID), new_job);
  /* Do not modify the following code. */
  /* BEGIN */
  struct stat st;
  if (stat(argp->output_dir, &st) == -1) {
    mkdirp(argp->output_dir);
  }

  return &result;
  /* END */
}

/* POLL_JOB RPC implementation. */
poll_job_reply* poll_job_1_svc(int* argp, struct svc_req* rqstp) {
  static poll_job_reply result;

  printf("Received poll job request\n");

  /* TODO */
  //get the correct job from the ht
  job* lookup = g_hash_table_lookup(state->jobInfo, GINT_TO_POINTER(*argp));

  //TODO: check that n_map_completed == n_map && n_reduce_completed == n_reduce
  //if invalid jobid was passed in
  if (lookup == NULL) {
    //printf("MADE IT HERE\n");
    result.done = false;
    result.failed = false;
    result.invalid_job_id = true;
    //we found lookup
  } else {
    //we're done 
    if(lookup->n_map_completed == lookup->n_map && lookup->n_reduce_completed == lookup->n_reduce) {
      //printf("FINISHING\n");
      result.done = true;
      lookup->done = true;
      //state->jobs = g_list_remove(state->jobs, GINT_TO_POINTER(*argp));
    } else {
      result.done = false;
    }
    result.failed = lookup->failed;
    result.invalid_job_id = false;
  }
  return &result;
}

/* GET_TASK RPC implementation. */
get_task_reply* get_task_1_svc(void* argp, struct svc_req* rqstp) {
  static get_task_reply result;

  printf("Received get task request\n");
  result.file = "";
  result.output_dir = "";
  result.app = "";
  result.wait = true;
  result.args.args_len = 0;

  /* TODO */
  for(int i = 0; i < g_list_length(state->jobs); i++) {
    int* curr_job_id = (int*) g_list_nth(state->jobs, i);
    job* curr_job = g_hash_table_lookup(state->jobInfo, GINT_TO_POINTER(*curr_job_id));
    if (curr_job->done == false && curr_job->failed == false) {
      //result.args.args_val = strdup(curr_job->args.args_val);
      //if there's still map tasks to be assigned

      if (curr_job->n_map_assigned < curr_job->n_map) {
        char* task_file = g_hash_table_lookup(curr_job->mapTasks, GINT_TO_POINTER(curr_job->n_map_assigned));
        result.task = curr_job->n_map_assigned;
        result.file = strdup(task_file);
        result.reduce = false;
        result.wait = false;
        curr_job->n_map_assigned += 1;
        result.job_id = curr_job->jobID;
        result.output_dir = strdup(curr_job->output_dir);
        result.app = strdup(curr_job->app);
        result.args.args_len = curr_job->args.args_len;
        result.n_reduce = curr_job->n_reduce;
        result.n_map = curr_job->n_map;
        // if (curr_job->args.args_val != NULL) {
        //   result.args.args_val = strdup(curr_job->args.args_val);
        // } else {
        //   result.args.args_val = ""; //or should it be null?
        // }
        result.args.args_val = strdup(curr_job->args.args_val);
        return &result;
      }
      //if all map tasks are completed, but not all reduce tasks are done
      if(curr_job->n_map_completed == curr_job->n_map && curr_job->n_reduce_completed < curr_job->n_reduce) {
        //are there any more reduce tasks to assign?
        if(curr_job->n_reduce_assigned < curr_job->n_reduce) {
          //assign the reduce task
          result.job_id = curr_job->jobID;
          result.output_dir = strdup(curr_job->output_dir);
          result.app = strdup(curr_job->app);
          result.args.args_len = curr_job->args.args_len;
          result.n_reduce = curr_job->n_reduce;
          result.n_map = curr_job->n_map;
          // if (curr_job->args.args_val != NULL) {
          //   result.args.args_val = strdup(curr_job->args.args_val);
          // } else {
          //   result.args.args_val = NULL; //or should it be null?
          // }
          //output dir could be corrupted
          //the counters are getting reset
          //check behavior for tasks that are finished, but stay in the job queue
          result.args.args_val = strdup(curr_job->args.args_val);
          result.task = curr_job->n_reduce_assigned;
          curr_job->n_reduce_assigned += 1;
          result.wait = false;
          result.reduce = true;
          return &result;
        } else {
          continue;
        }
      }
    } else {
      continue;
    }
  }
  return &result;
}

/* FINISH_TASK RPC implementation. */
void* finish_task_1_svc(finish_task_request* argp, struct svc_req* rqstp) {
  static char* result;

  printf("Received finish task request\n");

  /* TODO */
  job* curr_job = g_hash_table_lookup(state->jobInfo, GINT_TO_POINTER(argp->job_id));
  //if the job failed, set the failed field
  if (curr_job == NULL) {
    return (void*)&result;
  }
  if(argp->success == false) {
    curr_job->failed = true;
    return (void*)&result;
  }
  if (argp->reduce != true) {
    curr_job->n_map_completed += 1;
  } else {
    curr_job->n_reduce_completed += 1;
  }
  return (void*)&result;
}

/* Initialize coordinator state. */
void coordinator_init(coordinator** coord_ptr) {
  *coord_ptr = malloc(sizeof(coordinator));
  (*coord_ptr)->jobInfo = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
  (*coord_ptr)->counter = 0;
  (*coord_ptr)->jobs = NULL;
  coordinator* coord = *coord_ptr;
  /* TODO */
}