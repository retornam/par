/*********************/
/* errmsg.h          */
/* for Par 1.10      */
/* Copyright 1993 by */
/* Adam M. Costello  */
/*********************/

/* This is ANSI C code. */


#ifndef ERRMSG_H
#define ERRMSG_H


#define errmsg_size 163  /* This will never decrease, but may increase. */

typedef char errmsg_t[errmsg_size];

/* Any function which takes the argument errmsg_t errmsg must, before */
/* returning, either set errmsg[0] to '\0' (indicating success), or   */
/* write an error message string into errmsg, (indicating failure),   */
/* being careful not to overrun the space.                            */


extern const char * const outofmem;  /* "Out of memory.\n" */


#endif
