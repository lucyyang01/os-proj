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
  printf("I REACHED SUBMIT JOB\n");


  printf("Received submit job request\n");

  /* TODO */

  //validate provided application name
  app valid = get_app(argp->app);
  if (valid.name == NULL)
    return (int*) -1;

  //create a job
  job* new_job = malloc(sizeof(job));
  new_job->jobID = state->counter;
  new_job->app = argp->app;
  new_job->n_map = argp->files.files_len;
  char* str = strdup(*argp->files.files_val);
  //printf("duplicated string: %s\n", str);
  char* token;
  new_job->mapTasks = NULL;
  //printf("deliminated token: %s\n", token);

  //printf("start tokenizing\n");
  int idCounter = 0;
  //these should all be map tasks
  while ((token = strtok(str, ",")) != NULL) {
    //printf("while loop token: %s\n", token);
    task* new_task = malloc(sizeof(task));
    new_task->file = strdup(token);
    new_task->taskID = idCounter;
    new_task->reduce = false;
    idCounter += 1;
    //put inside job's maptasks
    new_job->mapTasks = g_list_append(new_job->mapTasks, new_task);
    str = NULL; //this was a quick fix to the while loop going infinitely?
  }
  printf("argp output dir: %s\n", argp->output_dir);
  new_job->output_dir = strdup(argp->output_dir);
  printf("new job output dir: %s\n", new_job->output_dir);

  new_job->args.args_len = argp->args.args_len;
  //printf("args val string: %s\n", argp->args.args_val);
  if(argp->args.args_val != NULL) {
    new_job->args.args_val = strdup(argp->args.args_val);
  } else {
    new_job->args.args_val = NULL;
  }
  new_job->n_reduce = argp->n_reduce;
  new_job->done = false;
  new_job->failed = false;


  //increment counter for next job's id
  state->counter += 1;

  //add to queue in fcfs order
  state->jobs = g_list_append(state->jobs, GINT_TO_POINTER(new_job->jobID));

  //printf("Size of job queue (submit job): %d\n", g_list_length(state->jobs));
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
  //assuming that argp is the jobid?

  //get the correct job from the ht
  job* lookup = g_hash_table_lookup(state->jobInfo, GINT_TO_POINTER(*argp));
  
  if (lookup == NULL) {
    result.done = false;
    result.failed = false;
    result.invalid_job_id = true;
  } else {
    result.done = lookup->done;
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
  printf("I REACHED GET TASK\n");
  printf("Size of job queue (get task): %d\n", g_list_length(state->jobs));
  //printf("size of jobinfo: %d\n", )

  /* TODO */
  //pop the first job off the queue? (only remove jobs once done == true)
  if(g_list_length(state->jobs) > 0) {
    //get jobid from job queue
    int* first_id = (int*) g_list_first(state->jobs);
    //look up job struct from hashtable
    job* first_job = g_hash_table_lookup(state->jobInfo, GINT_TO_POINTER(*first_id));
    result.job_id = first_job->jobID;
    printf("job id: %d\n", first_job->jobID);
    printf("output dir: %s\n", first_job->output_dir);
    result.output_dir = strdup(first_job->output_dir);
    result.app = strdup(first_job->app);
    result.args.args_len = first_job->args.args_len;
    if (first_job->args.args_val != NULL) {
      result.args.args_val = strdup(first_job->args.args_val);
    } else {
      result.args.args_val = "";
    }
    printf("assigned result metadata\n");
    //Set task to a map task number between 0 and n_map-1 inclusive or a reduce task number between 0 and n_reduce-1 inclusive.
    //check map tasks > 0:
    if (first_job->n_map > 0) {
      printf("assigning a map task\n");
      //assign a map task
      task* first_task = (task*) g_list_first(first_job->mapTasks);
      result.task = first_task->taskID;
      result.file = strdup(first_task->file);
      result.n_reduce = first_job->n_reduce;
      result.n_map = first_job->n_map - 1;
      result.reduce = false;
      result.wait = false;
      //decrement nmap, remove task from list
      first_job->n_map -= 1;
      first_job->mapTasks = g_list_remove(first_job->mapTasks, first_task);
    } else  {
      if (first_job->n_reduce > 0 && first_job->n_map == 0) {
        //assing a reduce task
        first_job->n_reduce -= 1;
        result.task = first_job->n_reduce;
        result.reduce = true;
        result.wait = false;
      }
    }
  }
  //if no map and no reduce tasks left, assign wait to false
  
  return &result;
}

/* FINISH_TASK RPC implementation. */
void* finish_task_1_svc(finish_task_request* argp, struct svc_req* rqstp) {
  static char* result;

  printf("Received finish task request\n");

  /* TODO */

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
