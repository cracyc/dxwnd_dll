#ifndef modenv_h
#define modenv_h

char *modify_env(char *newenv, const char *curenv, const char *name, const char *value);
char *add_to_path(const char *curenv, const char *newdir);

#endif