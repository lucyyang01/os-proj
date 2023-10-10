#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens* tokens);
int cmd_help(struct tokens* tokens);
int cmd_pwd(struct tokens* tokens);
int cmd_cd(struct tokens* tokens);


/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens* tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t* fun;
  char* cmd;
  char* doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
    {cmd_help, "?", "show this help menu"},
    {cmd_exit, "exit", "exit the command shell"},
    {cmd_pwd, "pwd", "print current working directory"},
    {cmd_cd, "cd", "change working directory"}
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens* tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens* tokens) { exit(0); }

int cmd_pwd(unused struct tokens* tokens) {
  printf("%s\n", getcwd(NULL, 0));
  return 0;
}

int cmd_cd(unused struct tokens* tokens) {
  return chdir(tokens_get_token(tokens, 1));
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}



/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int main(unused int argc, unused char* argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens* tokens = tokenize(line);
    
    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));
    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      //START SETUP

      //FIRST GET NUMBER OF PIPES
      int numPipes = 0;
      for(int i = 0; i < tokens_get_length(tokens); i++) {
        if(*tokens_get_token(tokens, i) == '|') {
          numPipes++;
        }
      }

      //IF NO PIPING REQUIRED EXECUTE OLD CODE
      if(numPipes == 0) {
        pid_t child_pid;
        if ((child_pid = fork()) == 0) {
          //START SETUP
        bool outRedirect = false;
        bool inRedirect = false;
        char* outfile;
        char* infile;
        int argSize = 0;

        char* rest_of_args[tokens_get_length(tokens) + 1];
        for(int i = 0; i < tokens_get_length(tokens); i++) {
          //REDIRECTION CHECKS BEGIN
          if (*tokens_get_token(tokens, i) == '>') {
            outRedirect = true; 
            outfile = tokens_get_token(tokens, i + 1);
            i++; //want i to increment twice to skip the file arg
            continue; 
          }
          if (*tokens_get_token(tokens, i) == '<') {
            inRedirect = true;
            infile = tokens_get_token(tokens, i + 1);
            i++; //want i to increment twice to skip the file arg
            continue;
          } 
          //REDIRECTION CHECKS END

          rest_of_args[argSize] = tokens_get_token(tokens, i);
          argSize++;
        }
        //append null pointer to args
        rest_of_args[argSize] = NULL;
        //END SETUP 
        //START REDIRECTION
        if (outRedirect) {
          int outFD = open(outfile, O_CREAT | O_RDWR | O_TRUNC, 0666);
          dup2(outFD, STDOUT_FILENO);
          outRedirect = false;
          close(outFD);
        }
        if (inRedirect) {
          int inFD = open(infile, O_RDONLY);
          dup2(inFD, STDIN_FILENO);
          inRedirect = false;
          close(inFD);
        }
        //END REDIRECTION

        //START PATH RESOLUTION
        char *path = getenv("PATH");
        char *saveptr;
        char* token = strtok_r(path, ":", &saveptr);
        char* path_to_program = tokens_get_token(tokens, 0);
        while (token != NULL) {
          if(*path_to_program == '/') {
            execv(tokens_get_token(tokens, 0), rest_of_args);
          }
          char full_path[2048];
          strcpy(full_path, token);
          strcat(full_path, "/");
          strcat(full_path, path_to_program);
          execv(full_path, rest_of_args);
          token = (char*) strtok_r(NULL, ":", &saveptr);
        }
        //END PATH RESOLUTION
        //back in parent
        } else {
          int status;
          wait(&status);
        }
      } else { //PIPE BEGINS
        //POPULATE PIPES ARRAY
        int pipeFDs[numPipes][2]; 
        for(int i = 0; i < numPipes; i++) {
          int pipefd[2];
          pipe(pipefd);
          pipeFDs[i][0] = pipefd[0];
          pipeFDs[i][1] = pipefd[1];
        }
        //PARSE ARGUMENTS
        bool outRedirect = false;
        bool inRedirect = false;
        char* outfile;
        char* infile;
        int argSize = 0;

        int pipeIndices[numPipes + 1][2]; //stores the start, ending index of process i's cmd line arguments 
        pipeIndices[0][0] = 0;

        char* rest_of_args[tokens_get_length(tokens) + 1];
        for(int i = 0; i < tokens_get_length(tokens); i++) {
          //REDIRECTION CHECKS BEGIN
          if (*tokens_get_token(tokens, i) == '>') {
            outRedirect = true; 
            outfile = tokens_get_token(tokens, i + 1);
            i++; //want i to increment twice to skip the file arg
            continue; 
          }
          if (*tokens_get_token(tokens, i) == '<') {
            inRedirect = true;
            infile = tokens_get_token(tokens, i + 1);
            i++; //want i to increment twice to skip the file arg
            continue;
          } 
          //REDIRECTION CHECKS END
          //PIPE CHECKS BEGIN
          if(*tokens_get_token(tokens, i) == '|') {
            pipeIndices[numPipes][1] = i - 1; 
            numPipes++;
            pipeIndices[numPipes][0] = i + 1
            continue; //don't want to add | to arg arr
          }
          //PIPE CHECKS END
          rest_of_args[argSize] = tokens_get_token(tokens, i);
          argSize++;
        }

        //APPEND NULL POINTER TO ARGS
        rest_of_args[argSize] = NULL;

        //FORK CHILD PROCESSES
        for(int i = 0; i <= numPipes; i++) {
          pid_t child_pid;
          if ((child_pid = fork()) == 0) {
            //START HANDLE PIPES
            //i is the process 
            //we're at the first process
            if(i == 0) {
              //0 doesn't pipe stdin
              //redirect 0's out to pipe1's write
              dup2(STDOUT_FILENO, pipeFDs[i][1]);
              close(STDOUT_FILENO);
            }
            //we're at the last process
            elif(i == numPipes) {
              //last doesn't pipe stdout
              dup2(STDIN_FILENO, pipeFDs[i - 1][0]);
              close(STDIN_FILENO);
            }
            else { //every other pipe
            //, processi changes its stdin FD (0) to point to the read end of pipe_arr[i - 1] 
              dup2(STDIN_FILENO, pipeFDs[i - 1][0]);
              //and changes its stdout FD (1) to point to the write end of pipe_arr[i]
              dup2(STDOUT_FILENO, pipeFDs[i][1]);
              close(STDOUT_FILENO);
              close(STDIN_FILENO);
            }
            //END HANDLE PIPES

            //START REDIRECTION
            if (outRedirect) {
              int outFD = open(outfile, O_CREAT | O_RDWR | O_TRUNC, 0666);
              dup2(outFD, STDOUT_FILENO);
              outRedirect = false;
              close(outFD);
            }
            if (inRedirect) {
              int inFD = open(infile, O_RDONLY);
              dup2(inFD, STDIN_FILENO);
              inRedirect = false;
              close(inFD);
            }
            //END REDIRECTION

            //START PATH RESOLUTION
            char *path = getenv("PATH");
            char *saveptr;
            char* token = strtok_r(path, ":", &saveptr);
            char* path_to_program = tokens_get_token(tokens, pipeIndices[numPipes][0]);
            int pipeIndex = pipeIndices[numPipes][0];
            while (token != NULL) {
              token = (char*) strtok_r(NULL, ":", &saveptr);
              if(*path_to_program == '/') {
                execv(tokens_get_token(tokens, 0), rest_of_args);
              }
              char full_path[2048];
              strcpy(full_path, token);
              strcat(full_path, "/");
              strcat(full_path, path_to_program);
              execv(full_path, rest_of_args);
            }
            //END PATH RESOLUTION
          } else {
            //we are in parent
            close(pipeFDs[i][0]);
            close(pipeFDs[i][1]);
            wait(NULL);
          } 

        }

      }

      //PARENT PROCESS CODE CONTINUED
      int status;
      int i = 0;
      while(i <= numPipes) {
        wait(&status);
        i++;
        //clsoe fds in the 
        
      }    
        //parent process execution
          
        //SIGNALLING
        //nd figure out the minimal subset of signals in https://cs162.org/static/hw/hw-shell/docs/signal-handling/signals/ should the shell process/child process ignore/default on
        //try to be conservative about which signals ignore, and which signals to default action on
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}



// bool outRedirect = false;
      // bool inRedirect = false;
      // char* outfile;
      // char* infile;
      // int argSize = 0;

      // if(numPipes > 0) {
      //   int pipeFDs[numPipes][2]; 
      //   int pipeIndices[numPipes + 1][2]; //stores the start, ending index of process i's cmd line arguments 
      //   pipeIndices[0][0] = 0;
      // }
      // char* rest_of_args[tokens_get_length(tokens) + 1]; //stores cmd line arguments per process
      // numPipes = 0; //index into arrays
      // for(int i = 0; i < tokens_get_length(tokens); i++) {
      //   //REDIRECTION CHECKS BEGIN
      //   if (*tokens_get_token(tokens, i) == '>') {
      //     outRedirect = true; 
      //     outfile = tokens_get_token(tokens, i + 1);
      //     i++; //want i to increment twice to skip the file arg
      //     continue; 
      //   }
      //   if (*tokens_get_token(tokens, i) == '<') {
      //     inRedirect = true;
      //     infile = tokens_get_token(tokens, i + 1);
      //     i++; //want i to increment twice to skip the file arg
      //     continue;
      //   } 
      //   //REDIRECTION CHECKS END

      //   //PIPE CHECKS BEGIN
      //   if(*tokens_get_token(tokens, i) == '|') {
      //     pipeIndices[numPipes][1] = i - 1; 
      //     numPipes++;
      //     pipeIndices[numPipes][0] = i + 1
      //     continue; //don't want to add | to arg arr
      //   }
      //   //PIPE CHECKS END
      //   rest_of_args[argSize] = tokens_get_token(tokens, i); //rest of args contains all tokens
      //   argSize++;
      // }

      // //append null pointer to args
      // rest_of_args[argSize] = NULL;


// /* Helper fn to isolate code that runs the process */
// int execHelper(int** pipeIndices, char* rest_of_args, int numPipes) {
//   //argument vector and path for execv
//   //list of child processes / num pipes
//   //we close all pipe fds that a child process doesn't use and then we close the one we do use after we redirected stdin/stdout right
//     pid_t child_pid;
//     for(int i = 0; i <= numPipes; i++) {
//       //i keeps track of the process we're on
//       if ((child_pid = fork()) == 0) {
//         //START REDIRECTION
//         if (outRedirect) {
//           int outFD = open(outfile, O_CREAT | O_RDWR | O_TRUNC, 0666);
//           dup2(outFD, STDOUT_FILENO);
//           outRedirect = false;
//           close(outFD);
//         }
//         if (inRedirect) {
//           int inFD = open(infile, O_RDONLY);
//           dup2(inFD, STDIN_FILENO);
//           inRedirect = false;
//           close(inFD);
//         }
//         //END REDIRECTION

//         //START PATH RESOLUTION
//         char *path = getenv("PATH");
//         char *saveptr;
//         char* token = strtok_r(path, ":", &saveptr);
//         char* path_to_program = tokens_get_token(tokens, pipeIndices[0][0]);
//         while (token != NULL) {
//           if(*path_to_program == '/') {
//             execv(tokens_get_token(tokens, 0), rest_of_args);
//           }
//           char full_path[2048];
//           strcpy(full_path, token);
//           strcat(full_path, "/");
//           strcat(full_path, path_to_program);
//           execv(full_path, rest_of_args);
//           token = (char*) strtok_r(NULL, ":", &saveptr);
//         }
//         //END PATH RESOLUTION

//         //BEGIN PIPING
        
//         //END PIPIHNG
//       }
//     }
// }



// #include <ctype.h>
// #include <errno.h>
// #include <fcntl.h>
// #include <stdbool.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/types.h>
// #include <signal.h>
// #include <sys/wait.h>
// #include <termios.h>
// #include <unistd.h>

// #include "tokenizer.h"

// /* Convenience macro to silence compiler warnings about unused function parameters. */
// #define unused __attribute__((unused))

// /* Whether the shell is connected to an actual terminal or not. */
// bool shell_is_interactive;

// /* File descriptor for the shell input */
// int shell_terminal;

// /* Terminal mode settings for the shell */
// struct termios shell_tmodes;

// /* Process group id for the shell */
// pid_t shell_pgid;

// int cmd_exit(struct tokens* tokens);
// int cmd_help(struct tokens* tokens);
// int cmd_pwd(struct tokens* tokens);
// int cmd_cd(struct tokens* tokens);


// /* Built-in command functions take token array (see parse.h) and return int */
// typedef int cmd_fun_t(struct tokens* tokens);

// /* Built-in command struct and lookup table */
// typedef struct fun_desc {
//   cmd_fun_t* fun;
//   char* cmd;
//   char* doc;
// } fun_desc_t;

// fun_desc_t cmd_table[] = {
//     {cmd_help, "?", "show this help menu"},
//     {cmd_exit, "exit", "exit the command shell"},
//     {cmd_pwd, "pwd", "print current working directory"},
//     {cmd_cd, "cd", "change working directory"}
// };

// /* Prints a helpful description for the given command */
// int cmd_help(unused struct tokens* tokens) {
//   for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
//     printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
//   return 1;
// }

// /* Exits this shell */
// int cmd_exit(unused struct tokens* tokens) { exit(0); }

// int cmd_pwd(unused struct tokens* tokens) {
//   printf("%s\n", getcwd(NULL, 0));
//   return 0;
// }

// int cmd_cd(unused struct tokens* tokens) {
//   return chdir(tokens_get_token(tokens, 1));
// }

// /* Looks up the built-in command, if it exists. */
// int lookup(char cmd[]) {
//   for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
//     if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
//       return i;
//   return -1;
// }

// /* Intialization procedures for this shell */
// void init_shell() {
//   /* Our shell is connected to standard input. */
//   shell_terminal = STDIN_FILENO;

//   /* Check if we are running interactively */
//   shell_is_interactive = isatty(shell_terminal);

//   if (shell_is_interactive) {
//     /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
//      * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
//      * foreground, we'll receive a SIGCONT. */
//     while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
//       kill(-shell_pgid, SIGTTIN);

//     /* Saves the shell's process id */
//     shell_pgid = getpid();

//     /* Take control of the terminal */
//     tcsetpgrp(shell_terminal, shell_pgid);

//     /* Save the current termios to a variable, so it can be restored later. */
//     tcgetattr(shell_terminal, &shell_tmodes);
//   }
// }

// int main(unused int argc, unused char* argv[]) {
//   init_shell();

//   static char line[4096];
//   int line_num = 0;

//   /* Please only print shell prompts when standard input is not a tty */
//   if (shell_is_interactive)
//     fprintf(stdout, "%d: ", line_num);

//   while (fgets(line, 4096, stdin)) {
//     /* Split our line into words. */
//     struct tokens* tokens = tokenize(line);
    
//     /* Find which built-in function to run. */
//     int fundex = lookup(tokens_get_token(tokens, 0));
//     if (fundex >= 0) {
//       cmd_table[fundex].fun(tokens);
//     } else {
//       //START CHILD PROCESS
        
//       //END CHILD PROCESS
//       //START PARENT PROCESS

//       int status;
//       wait(&status);
//     }


// int childProc(tokens) {
//   pid_t child_pid;
//   if ((child_pid = fork()) == 0) {
//   //START SETUP
//   bool outRedirect = false;
//   bool inRedirect = false;
//   char* outfile;
//   char* infile;
//   int argSize = 0;
//   //PIPE SETUP
//   char* rest_of_args[tokens_get_length(tokens) + 1];
//   for(int i = 0; i < tokens_get_length(tokens); i++) {
//     //REDIRECTION CHECKS BEGIN
//     if (*tokens_get_token(tokens, i) == '>') {
//       outRedirect = true; 
//       outfile = tokens_get_token(tokens, i + 1);
//       i++; //want i to increment twice to skip the file arg
//       continue; 
//     }
//     if (*tokens_get_token(tokens, i) == '<') {
//       inRedirect = true;
//       infile = tokens_get_token(tokens, i + 1);
//       i++; //want i to increment twice to skip the file arg
//       continue;
//     } 
//     //REDIRECTION CHECKS END

//     //PIPE CHECKS BEGIN
//     if(*tokens_get_token(tokens, i) == '|') {
//       //if we encounter a pipe, recursively call child process
//       childProc(jjj)
//       continue; //don't want to add | to arg arr
//     }
//     rest_of_args[argSize] = tokens_get_token(tokens, i);
//     argSize++;
//   }

//     //PIPE CHECKS END
//   //append null pointer to args
//   rest_of_args[argSize] = NULL;
//   //END SETUP 
//   //START REDIRECTION
//   if (outRedirect) {
//     int outFD = open(outfile, O_CREAT | O_RDWR | O_TRUNC, 0666);
//     dup2(outFD, STDOUT_FILENO);
//     outRedirect = false;
//     close(outFD);
//   }
//   if (inRedirect) {
//     int inFD = open(infile, O_RDONLY);
//     dup2(inFD, STDIN_FILENO);
//     inRedirect = false;
//     close(inFD);
//   }
//   //END REDIRECTION

//   //START PATH RESOLUTION
//   char *path = getenv("PATH");
//   char *saveptr;
//   char* token = strtok_r(path, ":", &saveptr);
//   char* path_to_program = tokens_get_token(tokens, 0);
//   while (token != NULL) {
//     if(*path_to_program == '/') {
//       execv(tokens_get_token(tokens, 0), rest_of_args);
//     }
//     char full_path[2048];
//     strcpy(full_path, token);
//     strcat(full_path, "/");
//     strcat(full_path, path_to_program);
//     execv(full_path, rest_of_args);
//     token = (char*) strtok_r(NULL, ":", &saveptr);
//     //END PATH RESOLUTION
//   }
// }
//     if (shell_is_interactive)
//       /* Please only print shell prompts when standard input is not a tty */
//       fprintf(stdout, "%d: ", ++line_num);

//     /* Clean up memory */
//     tokens_destroy(tokens);
//   }

//   return 0;
// }

  //instead of only forking one time, set up array of children to fork that handles the case of no pipes
  //get the number of pipes, then set up the children pid array and use for loop to iterate thru and fork
  //wait n = children times
  //change rest_of_args to be an array of args for each process
  // int numPipes = 0;
  // char* pipe_args[tokens_get_length(tokens) + 1];