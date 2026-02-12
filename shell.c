#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>

extern char **environ;

static char *get_path(void)
{
    int i = 0;

    while (environ[i])
    {
        if (strncmp(environ[i], "PATH=", 5) == 0)
            return (environ[i] + 5);
        i++;
    }
    return NULL;
}

static int file_exists(char *path)
{
    struct stat st;

    if (stat(path, &st) == 0)
        return 1;
    return 0;
}

/* Resolve command using PATH. Returns:
 * - command itself if it already contains '/'
 * - malloc'ed full path if found in PATH
 * - NULL if not found
 */
static char *resolve_path(char *command)
{
    char *path = get_path();
    char *path_copy, *dir, *full_path;
    int len;

    if (!command)
        return NULL;

    if (strchr(command, '/'))
    {
        if (file_exists(command))
            return command;
        return NULL;
    }

    if (!path)
        return NULL;

    path_copy = strdup(path);
    if (!path_copy)
        return NULL;

    dir = strtok(path_copy, ":");
    while (dir)
    {
        len = (int)strlen(dir) + (int)strlen(command) + 2;
        full_path = malloc(len);
        if (!full_path)
        {
            free(path_copy);
            return NULL;
        }

        sprintf(full_path, "%s/%s", dir, command);

        if (file_exists(full_path))
        {
            free(path_copy);
            return full_path; /* malloc'ed */
        }

        free(full_path);
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL;
}

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
    char *cmd_path;
    int last_status = 0; /* <-- IMPORTANT */

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
            exit(last_status); /* optional: exit with last status */
        }

        if (nread > 0 && line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        i = 0;
        token = strtok(line, " \t");
        while (token && i < 99)
        {
            argv[i++] = token;
            token = strtok(NULL, " \t");
        }
        argv[i] = NULL;

        if (!argv[0])
            continue;

        /* ===== BUILTINS ===== */

        if (strcmp(argv[0], "exit") == 0)
        {
            free(line);
            exit(last_status); /* <-- FIX: exit with last command status */
        }

        if (strcmp(argv[0], "env") == 0)
        {
            for (i = 0; environ[i]; i++)
                printf("%s\n", environ[i]);
            last_status = 0;
            continue;
        }

        /* ===== PATH RESOLUTION ===== */

        cmd_path = resolve_path(argv[0]);
        if (!cmd_path)
        {
            /* command not found => usual status is 127 */
            fprintf(stderr, "%s: not found\n", argv[0]);
            last_status = 127;
            continue; /* fork edilmir */
        }

        pid = fork();
        if (pid == 0)
        {
            execve(cmd_path, argv, environ);
            perror("execve");
            exit(126);
        }
        else if (pid > 0)
        {
            wait(&status);

            if (WIFEXITED(status))
                last_status = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                last_status = 128 + WTERMSIG(status);
            else
                last_status = 1;
        }
        else
        {
            perror("fork");
            last_status = 1;
        }

        if (cmd_path != argv[0])
            free(cmd_path);
    }

    free(line);
    return 0;
}
