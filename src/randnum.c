/*
  randnum.c
  
  By Bill Kendrick
  kendrick@zippy.sonoma.edu
  http://zippy.sonoma.edu/kendrick/
  
  September 8, 1996 - December 3, 1996 / July 8, 1998
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "randnum.h"

void randinit()
{
  long z;
  struct timeval tp;
  
  gettimeofday(&tp, NULL);
  
  z = tp.tv_sec + tp.tv_usec;
  
  srand(z);
}


int randnum(int mod)
{
  int v;
  
  v = rand();
  
  return(v % mod);
}
