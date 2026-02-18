#ifndef SHELL_H
#define SHELL_H

void prompt(void);
char **parse_line(char *line);
int execute_cmd(char **argv);
char *_which(char *command);
char *_getenv(const char *name);

#endif