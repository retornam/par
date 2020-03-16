/*********************/
/* reformat.h        */
/* for Par 1.10      */
/* Copyright 1993 by */
/* Adam M. Costello  */
/*********************/

/* This is ANSI C code. */


#include "errmsg.h"


char **reformat(
  const char * const *inlines, int hang, int prefix, int suffix,
  int width, int fit, int just, int last, int touch, errmsg_t errmsg
);
  /* inlines is a NULL-terminated array of pointers to input */
  /* lines. The other parameters are the variables of the    */
  /* same name as described in "par.doc". reformat(inlines,  */
  /* hang, prefix, suffix, width, just, last, min, errmsg)   */
  /* returns a NULL-terminated array of pointers to output   */
  /* lines containing the reformatted paragraph, according   */
  /* to the specification in "par.doc". None of the integer  */
  /* parameters may be negative. Returns NULL on failure.    */
