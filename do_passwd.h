#ifndef   __DO_PASSWD_H
#define   __DO_PASSWD_H

#include <fcntl.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>



int   passwd_write(char* pdate) ;
char* passwd_read() ;
int   passwd_open() ;

#endif
