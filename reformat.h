/*********************/
/* reformat.h        */
/* for Par 1.20      */
/* Copyright 1993 by */
/* Adam M. Costello  */
/*********************/

/* This is ANSI C code. */


#include "errmsg.h"


char **reformat(
  const char * const *inlines, const char * const *endline,
  int hang, int prefix, int suffix, int width, int fit,
  int just, int last, int touch, errmsg_t errmsg
);
  /* inlines is an array of pointers to input lines, up to    */
  /* but not including endline. The other parameters are the  */
  /* variables of the same name as described in "par.doc".    */
  /* reformat(inlines, endline, hang, prefix, suffix, width,  */
  /* just, last, min, errmsg) returns a NULL-terminated array */
  /* of pointers to output lines containing the reformatted   */
  /* paragraph, according to the specification in "par.doc".  */
  /* None of the integer parameters may be negative. Returns  */
  /* NULL on failure.                                         */
