/*
charset.c
last touched in Par 1.53.0
last meaningful change in Par 1.53.0
Copyright 1993, 2001, 2020 Adam M. Costello

This is ANSI C code (C89).

Because this is ANSI C code, we can't assume that char has only 8 bits.
Therefore, we can't use bit vectors to represent sets without the risk
of consuming large amounts of memory.  Therefore, this code is much more
complicated than might be expected.

The issues regarding char and unsigned char are relevant to the
use of the ctype.h functions, and the interpretation of the _xhh
sequence.  See the comments near the beginning of par.c.

*/


#include "charset.h"  /* Makes sure we're consistent with the prototypes. */

#include "buffer.h"
#include "errmsg.h"

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef NULL
#define NULL ((void *) 0)

#ifdef DONTFREE
#define free(ptr)
#endif


typedef unsigned char csflag_t;

struct charset {
  char *inlist;    /* Characters in inlist are in the set.                */
  char *outlist;   /* Characters in outlist are not in the set.           */
                   /* inlist and outlist must have no common characters.  */
                   /* inlist and outlist may be NULL, which acts like "". */
  csflag_t flags;  /* Characters in neither list are in the set if they   */
                   /* belong to any of the classes indicated by flags.    */
};

/* The following may be bitwise-OR'd together */
/* to set the flags field of a charset:       */

static const csflag_t
  CS_UCASE = 1,   /* Includes all upper case letters.   */
  CS_LCASE = 2,   /* Includes all lower case letters.   */
  CS_NCASE = 4,   /* Includes all neither case letters. */
  CS_DIGIT = 8,   /* Includes all decimal digits.       */
  CS_SPACE = 16,  /* Includes all space characters.     */
  CS_NUL   = 32;  /* Includes the NUL character.        */


static int appearsin(char c, const char *str)

/* Returns 0 if c is '\0' or str is NULL or c     */
/* does not appear in *str.  Otherwise returns 1. */
{
  return c && str && strchr(str,c);
}


static int hexdigtoint(char c)

/* Returns the value represented by the hexadecimal */
/* digit c, or -1 if c is not a hexadecimal digit.  */
{
  const char *p, * const hexdigits = "0123456789ABCDEFabcdef";
  int n;

  if (!c) return -1;
  p = strchr(hexdigits, *(unsigned char *)&c);
  if (!p) return -1;
  n = p - hexdigits;
  if (n >= 16) n -= 6;
  return n;

  /* We can't do things like c - 'A' because we can't */
  /* depend on the order of the characters in ANSI C. */
  /* Nor can we do things like hexdigtoint[c] because */
  /* we don't know how large such an array might be.  */
}


charset *parsecharset(const char *str, errmsg_t errmsg)
{
  charset *cset = NULL;
  buffer *cbuf = NULL;
  const char *p, * const singleescapes = "_sbqQx";
  int hex1, hex2;
  char ch;

  cset = malloc(sizeof (charset));
  if (!cset) {
    strcpy(errmsg,outofmem);
    goto pcserror;
  }
  cset->inlist = cset->outlist = NULL;
  cset->flags = 0;

  cbuf = newbuffer(sizeof (char), errmsg);
  if (*errmsg) goto pcserror;

  for (p = str;  *p;  ++p)
    if (*p == '_') {
      ++p;
      if (appearsin(*p, singleescapes)) {
        if      (*p == '_') ch = '_' ;
        else if (*p == 's') ch = ' ' ;
        else if (*p == 'b') ch = '\\';
        else if (*p == 'q') ch = '\'';
        else if (*p == 'Q') ch = '\"';
        else /*  *p == 'x'  */ {
          hex1 = hexdigtoint(p[1]);
          hex2 = hexdigtoint(p[2]);
          if (hex1 < 0  ||  hex2 < 0) goto pcsbadstr;
          *(unsigned char *)&ch = 16 * hex1 + hex2;
          p += 2;
        }
        if (!ch)
          cset->flags |= CS_NUL;
        else {
          additem(cbuf, &ch, errmsg);
          if (*errmsg) goto pcserror;
        }
      }
      else {
        if      (*p == 'A') cset->flags |= CS_UCASE;
        else if (*p == 'a') cset->flags |= CS_LCASE;
        else if (*p == '@') cset->flags |= CS_NCASE;
        else if (*p == '0') cset->flags |= CS_DIGIT;
        else if (*p == 'S') cset->flags |= CS_SPACE;
        else goto pcsbadstr;
      }
    }
    else {
      additem(cbuf,p,errmsg);
      if (*errmsg) goto pcserror;
    }
  ch = '\0';
  additem(cbuf, &ch, errmsg);
  if (*errmsg) goto pcserror;
  cset->inlist = copyitems(cbuf,errmsg);
  if (*errmsg) goto pcserror;

pcscleanup:

  if (cbuf) freebuffer(cbuf);
  return cset;

pcsbadstr:

  sprintf(errmsg, "Bad charset syntax: %.*s\n", errmsg_size - 22, str);

pcserror:

  if (cset) freecharset(cset);
  cset = NULL;
  goto pcscleanup;
}


void freecharset(charset *cset)
{
  if (cset->inlist) free(cset->inlist);
  if (cset->outlist) free(cset->outlist);
  free(cset);
}


int csmember(char c, const charset *cset)
{
  unsigned char uc;

  if (appearsin(c, cset->inlist )) return 1;
  if (appearsin(c, cset->outlist)) return 0;
  uc = *(unsigned char *)&c;

  /* The logic for the CS_?CASE flags is a little convoluted,  */
  /* but avoids calling islower() or isupper() more than once. */

  if (cset->flags & CS_NCASE) {
    if ( isalpha(uc) &&
         (cset->flags & CS_LCASE || !islower(uc)) &&
         (cset->flags & CS_UCASE || !isupper(uc))    ) return 1;
  }
  else {
    if ( (cset->flags & CS_LCASE && islower(uc)) ||
         (cset->flags & CS_UCASE && isupper(uc))    ) return 1;
  }

  return (cset->flags & CS_DIGIT && isdigit(uc)) ||
         (cset->flags & CS_SPACE && isspace(uc)) ||
         (cset->flags & CS_NUL   && !c         )    ;
}


static charset *csud(
  int u, const charset *cset1, const charset *cset2, errmsg_t errmsg
)
/* Returns the union of cset1 and cset2 if u is 1, or the set    */
/* difference cset1 - cset2 if u is 0.  Returns NULL on failure. */
{
  charset *csu;
  buffer *inbuf = NULL, *outbuf = NULL;
  char *lists[4], **list, *p, nullchar = '\0';

  csu = malloc(sizeof (charset));
  if (!csu) {
    strcpy(errmsg,outofmem);
    goto csuderror;
  }
  inbuf = newbuffer(sizeof (char), errmsg);
  if (*errmsg) goto csuderror;
  outbuf = newbuffer(sizeof (char), errmsg);
  if (*errmsg) goto csuderror;
  csu->inlist = csu->outlist = NULL;
  csu->flags =  u  ?  cset1->flags |  cset2->flags
                   :  cset1->flags & ~cset2->flags;

  lists[0] = cset1->inlist;
  lists[1] = cset1->outlist;
  lists[2] = cset2->inlist;
  lists[3] = cset2->outlist;

  for (list = lists;  list < lists + 4;  ++list)
    if (*list) {
      for (p = *list;  *p;  ++p)
        if (u  ?  csmember(*p, cset1) ||  csmember(*p, cset2)
               :  csmember(*p, cset1) && !csmember(*p, cset2)) {
          if (!csmember(*p, csu)) {
            additem(inbuf,p,errmsg);
            if (*errmsg) goto csuderror;
          }
        }
        else
          if (csmember(*p, csu)) {
            additem(outbuf,p,errmsg);
            if (*errmsg) goto csuderror;
          }
    }

  additem(inbuf, &nullchar, errmsg);
  if (*errmsg) goto csuderror;
  additem(outbuf, &nullchar, errmsg);
  if (*errmsg) goto csuderror;
  csu->inlist = copyitems(inbuf,errmsg);
  if (*errmsg) goto csuderror;
  csu->outlist = copyitems(outbuf,errmsg);
  if (*errmsg) goto csuderror;

csudcleanup:

  if (inbuf) freebuffer(inbuf);
  if (outbuf) freebuffer(outbuf);
  return csu;

csuderror:

  if (csu) freecharset(csu);
  csu = NULL;
  goto csudcleanup;
}


charset *csunion(const charset *cset1, const charset *cset2, errmsg_t errmsg)
{
  return csud(1,cset1,cset2,errmsg);
}


charset *csdiff(const charset *cset1, const charset *cset2, errmsg_t errmsg)
{
  return csud(0,cset1,cset2,errmsg);
}


void csadd(charset *cset1, const charset *cset2, errmsg_t errmsg)
{
  charset *csu;

  csu = csunion(cset1,cset2,errmsg);
  if (*errmsg) return;
  csswap(csu,cset1);
  freecharset(csu);
}


void csremove(charset *cset1, const charset *cset2, errmsg_t errmsg)
{
  charset *csu;

  csu = csdiff(cset1,cset2,errmsg);
  if (*errmsg) return;
  csswap(csu,cset1);
  freecharset(csu);
}


charset *cscopy(const charset *cset, errmsg_t errmsg)
{
  charset emptycharset = { NULL, NULL, 0 };

  return csunion(cset, &emptycharset, errmsg);
}


void csswap(charset *cset1, charset *cset2)
{
  charset tmp;

  tmp = *cset1;
  *cset1 = *cset2;
  *cset2 = tmp;
}
