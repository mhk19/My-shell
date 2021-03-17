#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MHKSH_TOK_BUFSIZE 64
#define MHKSH_TOK_DELIM " \t\r\n\a"

int mhksh_cd (char **args);
int mhksh_help (char **args);
int mhksh_exit (char **args);
int mhksh_echo (char **args);
int mhksh_history (char **args);
int size_of_history = 0;

char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "echo",
    "history"
};

int (*builtin_func[]) (char **) = {
    &mhksh_cd,
    &mhksh_help,
    &mhksh_exit,
    &mhksh_echo,
    &mhksh_history
};

int mhksh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

char **history;

int mhksh_cd (char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "mhksh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
        perror("mhksh");
        }
    }
    return 1;
}

int mhksh_help(char **args)
{
  int i;
  printf("This is Mahak's shell\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < mhksh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int mhksh_echo(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "mhksh: expected argument to \"echo\"\n");
    } else {
        char *dest;
        dest = malloc(MHKSH_TOK_BUFSIZE * sizeof(char));
        memcpy(dest, args[1]+1, strlen(args[1])-2);
        printf("%s \n", dest);
    }
    return 1;
}

int mhksh_history(char **args) {
    for (int i=0 ; i<size_of_history ; i++) {
        printf("%s", history[i]);
    }
    return 1;
}

int mhksh_exit(char **args)
{
  return 0;
}

int mhksh_launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    if (execvp(args[0], args) == -1) {
      perror("mhksh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    perror("mhksh");
  } else {
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int mhksh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    return 1;
  }

  for (i = 0; i < mhksh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return mhksh_launch(args);
}

char **mhksh_split_line(char *line)
{
  int bufsize = MHKSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "mhksh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, MHKSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += MHKSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "mhksh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, MHKSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

char *mhksh_read_line(void)
{
  char *line = NULL;
  ssize_t bufsize = 0;

  if (getline(&line, &bufsize, stdin) == -1){
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);
    } else  {
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }

  return line;
}

void mhksh_loop(void)
{
    char *line;
    char **args;
    int status = 1;
    int bufsize = MHKSH_TOK_BUFSIZE;
    int position = 0;
    history = malloc(1*sizeof(char*));

    while (status) {
        printf("> ");
        size_of_history++;
        line = mhksh_read_line();
        bufsize += strlen(line);
        history = realloc(history, size_of_history*sizeof(char*));
        history[position] = malloc(strlen(line)*sizeof(line));
        strcpy(history[position], line);
        position++;
        args = mhksh_split_line(line);
        status = mhksh_execute(args);

        free(line);
        free(args);
    }
}

int main(int argc, char **argv)
{
  mhksh_loop();
  return EXIT_SUCCESS;
}
