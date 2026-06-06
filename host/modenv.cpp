#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>

char *modify_env(const char *curenv, const char *name, const char *value)
{

   const char *src = curenv;
   char *newenv = NULL;
   bool found = false;
   int size = 0;
   int nlen = strlen(name);
   int vlen = strlen(value);
   while(*src) {
       int slen = strlen(src);
       if(!_strnicmp(src, name, nlen) && (src[nlen] == '=')) {
           newenv = (char *)realloc((void *)newenv, size + nlen + vlen + 2);
           size += sprintf(newenv + size, "%s=%s", name, value) + 1;
           found = true;
       } else {
           newenv = (char *)realloc((void *)newenv, size + slen + 1);
           strcpy(newenv + size, src);
           size += slen + 1;
       }
       src += slen + 1;
   }
   if(!found) {
       newenv = (char *)realloc((void *)newenv, size + nlen + vlen + 2);
       size += sprintf(newenv + size, "%s=%s", name, value) + 1;
   }
   newenv = (char *)realloc((void *)newenv, size + 1);
   newenv[size] = 0;
   return newenv;
}

char *add_to_path(const char *curenv, const char *newdir)
{
   const char *src = curenv;
   char *newpath, *newenv;
   const char *path = NULL;
   while(*src) {
       if(!_strnicmp(src, "PATH", 4) && (src[4] == '=')) {
           path = src + 5;
           break;
       }
       src += strlen(src) + 1;
   }
   if(path) {
       int plen = strlen(path);
       newpath = (char *)malloc(plen + strlen(newdir) + 2);
       char *semi = path[plen - 1] == ';' ? "" : ";";
       sprintf(newpath, "%s%s%s", path, semi, newdir);
   } else {
       newpath = (char *)malloc(strlen(newdir) + 1);
       strcpy(newpath, newdir);
   }
   newenv = modify_env(curenv, "PATH", newpath);
   free(newpath);
   return newenv;
}