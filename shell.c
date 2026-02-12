#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

extern char **environ;

/**
 * main - simple UNIX shell
 *
 * Return: Always 0
 */
int main(void)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    pid_t pid;
    int status;
    char *argv[100];
    char *token;
    int i;

    while (1)
    {
        if (isatty(STDIN_FILENO))
        {
            printf("newshell$ ");
            fflush(stdout);
        }

        nread = getline(&line, &len, stdin);
        if (nread == -1)
        {
            free(line);
            exit(0);
        }

        if (nread > 0 && line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        i = 0;
        token = strtok(line, " \t");
        while (token != NULL && i < 99)
        {
            argv[i++] = token;
            token = strtok(NULL, " \t");
        }
        argv[i] = NULL;

        if (argv[0] == NULL)
            continue;

        /* Built-in: exit */
        if (strcmp(argv[0], "exit") == 0)
        {
            free(line);
            exit(0);
        }

        /* Built-in: env */
        if (strcmp(argv[0], "env") == 0)
        {
            for (i = 0; environ[i] != NULL; i++)
                printf("%s\n", environ[i]);
            continue;
        }

        pid = fork();

        if (pid == 0)
        {
            if (execve(argv[0], argv, environ) == -1)
            {
                fprintf(stderr, "Command not found\n");
                exit(1);
            }
        }
        else if (pid > 0)
        {
            wait(&status);
        }
        else
        {
            perror("fork");
        }
    }

    free(line);
    return (0);
}
