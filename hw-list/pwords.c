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
  FILE *infile;
} pthread_args;


void wrapper_fn( void* args) {
  pthread_args* pargs = (pthread_args *) args;
  count_words(pargs->wclist, pargs->infile);
  //return;
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
    //malloc a struct containing information to pass to pthread_create
    pthread_t tid;
    for (int i = 1; i < argc - 1; i++) {
      FILE *infile = NULL;
      infile = fopen(argv[i], "r");
      if (infile == NULL) {
        return 1;
      }
      //pthread_args *args = malloc(sizeof(pthread_args));
      pthread_args args;
      args.infile = infile;
      args.wclist = &word_counts;

      pthread_create(&tid, NULL, (void*) wrapper_fn, &args);
      fclose(infile);
      //sys_pthread_join()
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
  }

  /* Output final result of all threads' work. */
  wordcount_sort(&word_counts, less_count);
  fprint_words(&word_counts, stdout);
  return 0;
}
