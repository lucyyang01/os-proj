/*

Copyright © 2019 University of California, Berkeley

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

word_count provides lists of words and associated count

Functional methods take the head of a list as first arg.
Mutators take a reference to a list as first arg.
*/

#include "word_count.h"

/* Basic utilities */

char *new_string(char *str) {
  char *new_str = (char *) malloc(strlen(str) + 1);
  if (new_str == NULL) {
    return NULL;
  }
  return strcpy(new_str, str);
}

int init_words(WordCount **wclist) {
  /* Initialize word count.
     Returns 0 if no errors are encountered
     in the body of this function; 1 otherwise.
  */
  *wclist = NULL;
  return 0;
}

ssize_t len_words(WordCount *wchead) {
  /* Return -1 if any errors are
     encountered in the body of
     this function.
  */
    size_t len = 0;
    WordCount *curr = wchead;
    while (curr != NULL) {
      len += 1;
      curr = (curr)->next;
    }
    return len;
}

WordCount *find_word(WordCount *wchead, char *word) {
  /* Return count for word, if it exists */
  //returns a wordcount node
  WordCount* wc = NULL;
  WordCount* curr = wchead;
  while (curr != NULL) {
    if (strcmp((curr)->word, word)) {
      wc = wchead;
    }
    curr = (curr)->next;
  }
  return wc;
}

int add_word(WordCount **wclist, char *word) {
  /* If word is present in word_counts list, increment the count.
     Otherwise insert with count 1.
     Returns 0 if no errors are encountered in the body of this function; 1 otherwise.
  */
  WordCount* found = find_word(*wclist, word);
  printf("word array: %s\n", word);
  //if the word present in list
  if (found != NULL) {
    (found)->count += 1;
  } else {
    WordCount *new_word = (WordCount*) malloc(sizeof(WordCount));
    if (new_word == NULL) {
      printf("new word not malloced properly");
      return 1;
    }
    // new_word->word = word;
    // new_word->count = 1;
    // new_word->next = NULL;
    new_word->word = (char*) malloc(sizeof(word));
    if (new_word->word == NULL) {
      return 1;
    }
    new_word->word = word;
    new_word->count = 1;
    new_word->next = (WordCount*) malloc(sizeof(WordCount));
    if (new_word->next == NULL) {
      printf("new word next not malloced properly");
      return 1;
    }
    new_word->next = NULL;
    //this is the first word added
    if(*wclist == NULL) {
      *wclist = new_word;
    //there are other nodes in the list so we have to traverse to the end
    } else {
      WordCount *last_node = *wclist;
      //last_node should contain the last node
      while(last_node->next != NULL ) {
        last_node = last_node->next;
      }
      last_node->next = new_word;
    }
  }
  return 0;
}

void fprint_words(WordCount *wchead, FILE *ofile) {
  /* print word counts to a file */
  WordCount *wc;
  for (wc = wchead; wc; wc = wc->next) {
    fprintf(ofile, "%i\t%s\n", wc->count, wc->word);
  }
}

