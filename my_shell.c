#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

char s[100] = {0};

int NumOfTokens;
int bg_pid[64];
bool bg_stat[64];
int bg_count = 0;
int fc;
int startShell = 1;
int f;

char line[MAX_INPUT_SIZE];

void init_bg_var()
{

  int k;
  for (k = 0; k < 64; k++)
  {
    bg_pid[k] = -1;
    bg_stat[k] = false;
  }
}

void check_bg_process_finish()
{
  int status;
  int pid;
  int i;
  for (i = 0; i < bg_count; i++)
  {
    if (bg_stat[i] == true)
    {
      pid = waitpid(bg_pid[i], &status, WNOHANG);

      if (pid == bg_pid[i])
      {
        printf("SHELL : Background Process Finished PID : %d \n", bg_pid[i]);
        bg_stat[i] = false;
      }
    }
  }
}

char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for (i = 0; i < strlen(line); i++)
  {

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t')
    {
      token[tokenIndex] = '\0';
      if (tokenIndex != 0)
      {
        tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
        strcpy(tokens[tokenNo++], token);
        tokenIndex = 0;
      }
    }
    else
    {
      token[tokenIndex++] = readChar;
    }
  }

  NumOfTokens = tokenNo;
  free(token);
  tokens[tokenNo] = NULL;
  return tokens;
}

void handle_sigint()
{
  char **tokens;

  tokens = tokenize(line);
  if (tokens[0] == NULL)
    printf("\n SHELL : SIGINT detected!,to continue press enter \n");
  else
  {
    kill(fc, SIGTERM);
    int wc, status;
    wc = waitpid(fc, &status, 0);
    if (wc == fc)
      printf("\nSHELL : Foreground Process Terminated PID : %d \n", fc);
  }
}

int find_bg_count()
{
  int i;
  for (i = 0; i < 64; i++)
    if (bg_stat[i] == false)
      return i;
}

int main(int argc, char *argv[])
{
  init_bg_var();
  // char line[MAX_INPUT_SIZE];
  char first = '\0';
  int wc;
  char **tokens;
  int i;

  int DirChangeFlag = 0;

  int status;

  struct sigaction sa;
  sa.sa_handler = &handle_sigint;
  sa.sa_flags = SA_RESTART;

  sigaction(SIGINT, &sa, NULL);

  // signal(SIGINT, &handle_sigint);

  while (startShell)
  {
    /* BEGIN: TAKING INPUT */

    if (bg_count != 0)
      check_bg_process_finish();

    bzero(line, sizeof(line));
    usleep(500000);
    printf("CS744@siddhant:~%s$ ", getcwd(s, 100));
    scanf("%[^\n]", line);
    getchar();

    line[strlen(line)] = '\n'; // terminate with new line
    tokens = tokenize(line);

    if (tokens[0] == NULL)
    {
      continue;
    }

    // Forking a new process every iteration
    fc = fork();

    if (fc < 0)
    {
      // fork failed; exit
      fprintf(stderr, "fork failed\n");
      exit(1);
    }
    else if (fc == 0)
    { // child (new process)

      if ((strcmp(tokens[0], "cd") == 0) || (strcmp(tokens[0], "exit") == 0))
      {
        _exit(1);
      }
      else
      {
        if (strcmp(tokens[NumOfTokens - 1], "&") == 0)
        {
          tokens[NumOfTokens - 1] = NULL;

          f = setpgid(getpid(), getpid());
          if (f != 0)
          {
            printf("Value ret by setpgid : %d\n", f);
            printf("Value of errno       : %d\n", errno);
          }

          printf("process pushed to background PID : %d and PGID : %d\n", getpid(), getpgid(getpid()));
        }

        execvp(tokens[0], tokens);
        printf("command execution failed (cmd entered does not exist)!!\n");
        _exit(1);
      }
    }
    else
    { // parent goes down this path (main)

      if (strcmp(tokens[NumOfTokens - 1], "&") == 0)
      {

        if (bg_count == 63)
          bg_count = find_bg_count();

        bg_pid[bg_count] = fc;

        bg_stat[bg_count] = true;

        bg_count++;

        fc = -13;

        continue;
      }

      wc = waitpid(fc, &status, 0);
      // if (wc == fc)

      if (strcmp(tokens[0], "exit") == 0)
      {
        startShell = 0;
        for (i = 0; i < bg_count; i++)
        {

          if (bg_stat[i] == true)
          {

            kill(bg_pid[i], SIGTERM);
            int s1;
            int wc1 = waitpid(bg_pid[i], &s1, 0);

            if (wc1 == bg_pid[i])
              printf("Exit cmd : Background process terminated  PID : %d \n", bg_pid[i]);
          }
        }
      }
      if (strcmp(tokens[0], "cd") == 0)
      {
        // printf("no. of tokens : %d \n", NumOfTokens);
        if (NumOfTokens <= 2)
        {
          DirChangeFlag = chdir(tokens[1]);
          if (DirChangeFlag != 0)
            printf("\"%s\" Specified Path Does not exist or is a File !!\n", tokens[1]);
        }
        else
        {

          printf("SHELL cd : too many arguments\n");
        }
      }

      // Freeing the allocated memory
      for (i = 0; tokens[i] != NULL; i++)
      {
        free(tokens[i]);
      }
      free(tokens);
    }
  }
  return 0;
}
