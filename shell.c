#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "shell.h"

extern char **environ;

/**
 * find_command - search command in PATH
 * @command: command name
 *
 * Return: full path if found, otherwise NULL
 */
char *find_command(char *command)
{
	char *path_env;
	char *path_copy;
	char *dir;
	static char full_path[1024];
	int i;

	if (strchr(command, '/'))
	{
		if (access(command, X_OK) == 0)
			return (command);
		return (NULL);
	}

	path_env = NULL;
	i = 0;

	while (environ[i] != NULL)
	{
		if (strncmp(environ[i], "PATH=", 5) == 0)
		{
			path_env = environ[i] + 5;
			break;
		}
		i++;
	}

	if (path_env == NULL || path_env[0] == '\0')
		return (NULL);

	path_copy = strdup(path_env);
	if (path_copy == NULL)
		return (NULL);

	dir = strtok(path_copy, ":");

	while (dir != NULL)
	{
		snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);

		if (access(full_path, X_OK) == 0)
		{
			free(path_copy);
			return (full_path);
		}

		dir = strtok(NULL, ":");
	}

	free(path_copy);
	return (NULL);
}

/**
 * builtin_env - print all environment variables
 */
void builtin_env(void)
{
	int i;

	i = 0;
	while (environ[i] != NULL)
	{
		printf("%s\n", environ[i]);
		i++;
	}
}

/**
 * main - simple shell
 *
 * Return: last command exit status
 */
int main(void)
{
	char *line;
	size_t len;
	ssize_t nread;
	pid_t pid;
	int status;
	char *argv[100];
	char *token;
	int i;
	char *cmd_path;

	line = NULL;
	len = 0;
	status = 0;

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
			exit(status);
		}

		if (line[nread - 1] == '\n')
			line[nread - 1] = '\0';

		i = 0;
		token = strtok(line, " \t");

		while (token != NULL && i < 99)
		{
			argv[i] = token;
			i++;
			token = strtok(NULL, " \t");
		}
		argv[i] = NULL;

		if (argv[0] == NULL)
			continue;

		if (strcmp(argv[0], "exit") == 0)
		{
			free(line);
			exit(status);
		}

		if (strcmp(argv[0], "env") == 0)
		{
			builtin_env();
			status = 0;
			continue;
		}

		cmd_path = find_command(argv[0]);

		if (cmd_path == NULL)
		{
			fprintf(stderr, "./hsh: 1: %s: not found\n", argv[0]);
			status = 127;
			continue;
		}

		pid = fork();

		if (pid == -1)
		{
			perror("fork");
			status = 1;
			continue;
		}

		if (pid == 0)
		{
			if (execve(cmd_path, argv, environ) == -1)
			{
				perror("execve");
				exit(1);
			}
		}
		else
		{
			waitpid(pid, &status, 0);
			if (WIFEXITED(status))
				status = WEXITSTATUS(status);
		}
	}

	free(line);
	return (status);
}
