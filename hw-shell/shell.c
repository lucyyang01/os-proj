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
      bool outRedirect = false;
      bool inRedirect = false;
      char* outfile;
      char* infile;
      int argSize = 0;
      char* rest_of_args[tokens_get_length(tokens) + 1];
      for(int i = 0; i < tokens_get_length(tokens); i++) {
         if (*tokens_get_token(tokens, i) == '>') {
          outRedirect = true; 
          outfile = tokens_get_token(tokens, i + 1);
          i++; //want i to increment twice to skip the file arg
          continue; 
        } else if (*tokens_get_token(tokens, i) == '<') {
          inRedirect = true;
          infile = tokens_get_token(tokens, i + 1);
          i++; //want i to increment twice to skip the file arg
          continue;
        } 
        rest_of_args[argSize] = tokens_get_token(tokens, i);
        argSize++;
      }
      //append null pointer to args
      rest_of_args[argSize] = NULL;
      //END SETUP 

      //child code execution
      pid_t child_pid;
      if ((child_pid = fork()) == 0) {
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

      } else {
        int status;
        wait(&status);
      }
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}

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
//       /* REPLACE this to run commands as programs. */
//       //fprintf(stdout, "This shell doesn't know how to run programs.\n");
//       // bool outRedirect = false;
//       // bool inRedirect = false;
//       // bool inNeedToClose = false;
//       // bool outNeedToClose = false;
//       // //char* process;
//       // char* outfile;
//       // char* infile;
//       // int argSize = 0;

//       // char* rest_of_args[tokens_get_length(tokens) + 1];
//       // for(int i = 0; i < tokens_get_length(tokens); i++) {
//         //redirect output to files that don't yet exist
//         //skip redirector and file
//         //keep track of current size of rest of args, if u see anything that's not a carr
//         //>,< always surrounded by spaces


//         // if (*tokens_get_token(tokens, i) == '>') {
//         //   i++;
//         //   outRedirect = true; 
//         //   outfile = tokens_get_token(tokens, i + 1);
//         //   continue; //uncomdy
//         // } else if (*tokens_get_token(tokens, i) == '<') {
//         //   //feed contents of file to stdin 
//         //   i++;
//         //   inRedirect = true;
//         //   infile = tokens_get_token(tokens, i + 1);
//         //   continue;
//         // } 
//         // i++;

//       //   argSize++;
//       //   rest_of_args[argSize] = tokens_get_token(tokens, i);
//       //   //argSize++;
//       // }
//       // //append null pointer to args
//       // //use size variable to null terminate arg array
//       // rest_of_args[argSize + 1] = NULL;
//       pid_t child_pid;
//       // int outFD;
//       // int inFD;
//       //hello
//       //child code execu
//       if ((child_pid = fork()) == 0) {
//         //START REDIRECTION
//         // if (outRedirect) {
//         //   outFD = open(outfile, O_CREAT | O_RDWR | O_TRUNC, 0666);
//         //   if (outFD == -1) {
//         //     printf("%s\n", outfile);
//         //     perror("outfd error");
//         //   }
//         //   if (dup2(outFD, STDOUT_FILENO) == -1) {
//         //     perror("dup2 error");
//         //   }
//         //   outRedirect = false;
//         //   outNeedToClose = true;
//         // }
//         // if (inRedirect) {
//         //   inFD = open(infile, O_RDONLY);
//         //   if (inFD == -1) {
//         //     printf("%s\n", infile);
//         //     perror("outfd error");
//         //   }
//         //   if (dup2(inFD, STDIN_FILENO) == -1) {
//         //     perror("dup2 error");
//         //   }
//         //   inRedirect = false;
//         //   inNeedToClose = true;
//         // }
//         //END REDIRECTION
       

//         //START PATH RESOLUTION
//         // char *path = getenv("PATH");
//         // char* path_to_program = tokens_get_token(tokens, 0);

//         char* rest_of_args[tokens_get_length(tokens) + 1];
//         for(int i = 0; i < tokens_get_length(tokens); i++) {
//           rest_of_args[i] = tokens_get_token(tokens, i);
//         }
//         //append null pointer to args
//         rest_of_args[tokens_get_length(tokens)] = NULL;
//         //child code execution
        
//         char *path = getenv("PATH");
//         char* path_to_program = tokens_get_token(tokens, 0);
//         char *saveptr;
//         char* token = strtok_r(path, ":", &saveptr);
//         //while token not null or exec keeps failing
//         while ((token = (char*) strtok_r(NULL, ":", &saveptr))) {
//           char full_path[2048];
//           strcat(token, "/");
//           strcat(token, path_to_program);
//           strcpy(full_path, token); //use slashes when concatenating paths
//           execv(full_path, rest_of_args);
//         }
//       }
        
//        else {
//         // if(inNeedToClose) {
//         //   close(inFD);
//         // }
//         // if(outNeedToClose) {
//         //   close(outFD);
//         // }
//         int status;
//         wait(&status);
//       }
//     }

//     if (shell_is_interactive)
//       /* Please only print shell prompts when standard input is not a tty */
//       fprintf(stdout, "%d: ", ++line_num);

//     /* Clean up memory */
//     tokens_destroy(tokens);
//   }

//   return 0;
// }
