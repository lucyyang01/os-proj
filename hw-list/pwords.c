/*
 * Word count application with one thread per input file.
 *
 * You may modify this file in any way you like, and are expected to modify it.
 * Your solution must read each input file from a separate thread. We encourage
 * you to make as few changes as necessary.
 */

/*
 * Copyright © 2021 University of California, Berkeley
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <pthread.h>

#include "word_count.h"
#include "word_helpers.h"

/*
 * main - handle command line, spawning one thread per file.
 */
//int count_words(WordCount **wclist, FILE *infile) {
typedef struct pthread_args {
  word_count_list_t *wclist;
  char* infile;
} pthread_args;


void* wrapper_fn(void* args) {
  //args = (pthread_args*) args;
  pthread_args* pt = (pthread_args*) args;
  FILE* infile = fopen(pt->infile, "r");
  if (infile == NULL) {
    printf("NULL FILE");
    return NULL;
  }
  count_words(pt->wclist, infile);
  fclose(infile);
  pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
  /* Create the empty data structure. */
  word_count_list_t word_counts;
  init_words(&word_counts);

  if (argc <= 1) {
    /* Process stdin in a single thread. */
    count_words(&word_counts, stdin);
  } else {
    /* TODO */
    pthread_t threads[argc - 1];
    for(int i = 1; i < argc; i ++) {
      pthread_args* args = malloc(sizeof(pthread_args));
      if (args == NULL) {
        printf("MALLOCERRROR");
        return 1;
      }
      args->infile = argv[i];
      args->wclist = &word_counts;
      //void* newptr =args;
      int rc = pthread_create(&threads[i], NULL, wrapper_fn, (void*)args);
      if (rc) {
        printf("ERROR; THREAD");
        exit(-1);
      }
    }
    for (int i = 1; i < argc; i ++) {
      pthread_join(threads[i], NULL);
    }
  }

  /* Output final result of all threads' work. */
  wordcount_sort(&word_counts, less_count);
  fprint_words(&word_counts, stdout);
  return 0;
}

//call join to wait for all threads to finish
    //sys_pthread_join();
    //not concern political data empiricalalternative narr 2/3
    //narr made 2/3
    //narr deal with time
    //politically viable narr selective social worker alt narr 2/6
    //none pd all rh pc dehum designer above alt narr narr time 4/6
    //prop tax consumer data
    //all water usage
    //score none 5/6
    //score balance 5/6
    //unemployemnent
    //none


    /* TODO */
    //malloc a struct containing information to pass to pthread_create
  //   //list of threads
  //   pthread_t* threads = malloc((argc - 1) * sizeof(pthread_t));
  //   //list of thread args associated with thread
  //   pthread_args* threadargs = malloc((argc - 1)* sizeof(pthread_args));
  //   for (int i = 1; i < argc ; i++) {
  //     //pthread_t tid = i;
  //     FILE *infile = NULL;
  //     infile = fopen(argv[i], "r");
  //     if (infile == NULL) {
  //       return 1;
  //     }
  //     //threads[i - 1] = tid;
  //     pthread_args args;
  //     args.infile = infile;
  //     args.wclist = &word_counts;
  //     threadargs[i - 1] = args;
  //     int error = pthread_create(&threads[i - 1], NULL, wrapper_fn, &threadargs[i - 1]);
  //     if (error != 0) {
  //       printf("error creating thrfead");
  //       return 1;
  //     }
  //     fclose(infile);
  //     //sys_pthread_join()
  //   }

  //   for(int i = 0; i < argc - 2; i++) {
  //     pthread_join(threads[i], NULL);
  //   }
  //   free(threads);
  //   free(threadargs);
  // }