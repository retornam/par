/*
reformat.h
last touched in Par 1.53.0
last meaningful change in Par 1.53.0
Copyright 1993, 2020 Adam M. Costello

This is ANSI C code (C89).

*/


#include "charset.h"
#include "errmsg.h"


char **reformat(
  const char * const *inlines, const char * const *endline, int afp, int fs,
  int hang, int prefix, int suffix, int width, int cap, int fit, int guess,
  int just, int last, int Report, int touch, const charset *terminalchars,
  errmsg_t errmsg
);
  /* inlines is an array of pointers to input lines, up to but not  */
  /* including endline.  inlines and endline must not be equal.     */
  /* terminalchars is the set of terminal characters as described   */
  /* in "par.doc".  The other parameters are variables described in */
  /* "par.doc".  reformat(inlines, endline, afp, fs, hang, prefix,  */
  /* suffix, width, cap, fit, guess, just, last, Report, touch,     */
  /* terminalchars, errmsg) returns a NULL-terminated array of      */
  /* pointers to output lines containing the reformatted paragraph, */
  /* according to the specification in "par.doc".  None of the      */
  /* integer parameters may be negative.  Returns NULL on failure.  */
