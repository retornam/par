/*********************/
/* par.c             */
/* for Par 1.30      */
/* Copyright 1993 by */
/* Adam M. Costello  */
/*********************/

/* This is ANSI C code. */


#include "buffer.h"    /* Also includes <stddef.h> and "errmsg.h". */
#include "reformat.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#undef NULL
#define NULL ((void *) 0)

#ifdef DONTFREE
#define free(ptr)
#endif


const char * const progname = "par";
const char * const versionnum = "1.30";

struct charset {
  char *individuals;  /* Characters in this string are in the set.        */
  short flags;        /* Groups of characters can be included with flags. */
};

/* The following may be bitwise-OR'd together  */
/* to set the flags field of a struct charset: */

const short CS_UCASE = 1,  /* Includes all upper case letters. */
            CS_LCASE = 2,  /* Includes all lower case letters. */
            CS_DIGIT = 4,  /* Includes all decimal digits.     */
            CS_NUL   = 8;  /* Includes the NUL character.      */


static int incharset(char c, struct charset cset)

/* Returns 1 if c is in cset, 0 otherwise. */
{
  return     c && cset.individuals && strchr(cset.individuals, c)
         ||  cset.flags & CS_UCASE && isupper(c)
         ||  cset.flags & CS_LCASE && islower(c)
         ||  cset.flags & CS_DIGIT && isdigit(c)
         ||  cset.flags & CS_NUL   && !c
        ;
}


static int digtoint(char c)

/* Returns the value represented by the digit c, or -1 if c is not a digit. */
{
  return c == '0' ? 0 :
         c == '1' ? 1 :
         c == '2' ? 2 :
         c == '3' ? 3 :
         c == '4' ? 4 :
         c == '5' ? 5 :
         c == '6' ? 6 :
         c == '7' ? 7 :
         c == '8' ? 8 :
         c == '9' ? 9 :
         -1;

  /* We can't simply return c - '0' because this is ANSI C code, so */
  /* it has to work for any character set, not just ones which put  */
  /* the digits together in order.  Also, a lookup-table would be   */
  /* bad because there's no upper limit on CHAR_MAX.                */
}


static int hexdigtoint(char c)

/* Returns the value represented by the hexadecimal */
/* digit c, or -1 if c is not a hexadecimal digit.  */
{
  return c == 'A' || c == 'a' ? 10 :
         c == 'B' || c == 'b' ? 11 :
         c == 'C' || c == 'c' ? 12 :
         c == 'D' || c == 'd' ? 13 :
         c == 'E' || c == 'e' ? 14 :
         c == 'F' || c == 'f' ? 15 :
         digtoint(c);
}


static int strtoudec(const char *s, int *pn)

/* Converts the longest prefix of string s consisting of decimal   */
/* digits to an integer, which is stored in *pn.  Normally returns */
/* 1.  If *s is not a digit, then *pn is not changed, but 1 is     */
/* still returned.  If the integer represented is greater than     */
/* 9999, then *pn is not changed and 0 is returned.                */
{
  int n = 0;

  if (!isdigit(*s)) return 1;

  do {
    if (n >= 1000) return 0;
    n = 10 * n + digtoint(*s);
  } while (isdigit(*++s));

  *pn = n;

  return 1;
}


static void parsearg(
  const char *arg, int *phang, int *pprefix, int *psuffix, int *pwidth,
  int *pdiv, int *pfit, int *pguess, int *pjust, int *plast, int *pquote,
  int *pReport, int *ptouch, int *pversion, errmsg_t errmsg
)
/* Parses the command line argument in arg, setting *phang, *pprefix, */
/* *psuffix, *pwidth, *pdiv, *pfit, *pguess, *pjust, *plast, *pquote, */
/* *pReport, *ptouch, and/or *pversion as appropriate.                */
{
  const char *savearg = arg;
  char oc;
  int n;

  *errmsg = '\0';

  if (*arg == '-') ++arg;

  if (!strcmp(arg, "version")) {
    *pversion = 1;
    return;
  }

  if (isdigit(*arg)) {
    if (!strtoudec(arg, &n)) goto badarg;
    if (n <= 8) *pprefix = n;
    else *pwidth = n;
  }

  for (;;) {
    while (isdigit(*arg)) ++arg;
    oc = *arg;
    if (!oc) break;
    n = 1;
    if (!strtoudec(++arg, &n)) goto badarg;
    if (oc == 'h' || oc == 'p' || oc == 's' || oc == 'w') {
      if (oc == 'h') *phang = n;
      else {
        if (!isdigit(*arg)) goto badarg;
        if      (oc == 'w') *pwidth  = n;
        else if (oc == 'p') *pprefix = n;
        else  /*oc == 's'*/ *psuffix = n;
      }
    }
    else {
      if (n > 1) goto badarg;
      if      (oc == 'd') *pdiv    = n;
      else if (oc == 'f') *pfit    = n;
      else if (oc == 'g') *pguess  = n;
      else if (oc == 'j') *pjust   = n;
      else if (oc == 'l') *plast   = n;
      else if (oc == 'q') *pquote  = n;
      else if (oc == 'R') *pReport = n;
      else if (oc == 't') *ptouch  = n;
      else goto badarg;
    }
  }

  return;

badarg:
  sprintf(errmsg, "Bad argument: %.*s\n", errmsg_size - 16, savearg);
}


static struct charset getcharset(
  const char *varname, const char *defaultval, errmsg_t errmsg
)
/* Returns the set of characters defined by the environment variable named */
/* *varname, according to the syntax for PARBODY described in par.doc.     */
/* If *varname is not defined, the value *defaultval is used instead.      */
/* On failure, the individuals field of the return value will be NULL.     */
{
  const char *val, *p;
  struct buffer *cbuf = NULL;
  char ch;
  struct charset cs = { NULL, 0 };

  val = getenv(varname);
  if (!val) val = defaultval;

  cbuf = newbuffer(sizeof (char), errmsg);
  if (*errmsg) goto gcscleanup;
  for (p = val;  *p;  ++p)
    if (*p == '_') {
      ++p;
      if (*p == '_' || *p == 's' || *p == 'x') {
        if      (*p == '_') ch = '_';
        else if (*p == 's') ch = ' ';
        else /* *p == 'x' */ {
          if (!isxdigit(p[1]) || !isxdigit(p[2])) goto gcsbadval;
          ch = 16 * hexdigtoint(p[1]) + hexdigtoint(p[2]);
          p += 2;
        }
        if (ch) {
          additem(cbuf, &ch, errmsg);
          if (*errmsg) goto gcscleanup;
        }
        else
          cs.flags |= CS_NUL;
      }
      else {
        if      (*p == 'A') cs.flags |= CS_UCASE;
        else if (*p == 'a') cs.flags |= CS_LCASE;
        else if (*p == '0') cs.flags |= CS_DIGIT;
        else goto gcsbadval;
      }
    }
    else {
      additem(cbuf,p,errmsg);
      if (*errmsg) goto gcscleanup;
    }
  ch = '\0';
  additem(cbuf, &ch, errmsg);
  if (*errmsg) goto gcscleanup;
  cs.individuals = copyitems(cbuf,errmsg);

gcscleanup:

  if (cbuf) freebuffer(cbuf);
  return cs;

gcsbadval:

  sprintf(errmsg, "Bad %s: %.*s\n", varname,
          errmsg_size - 8 - strlen(varname), val);
  goto gcscleanup;
}


static char **readlines(struct charset protectchars,
                        struct charset quotechars, int quote, errmsg_t errmsg)

/* Reads lines from stdin until EOF, or until a line beginning with a */
/* protective character is encountered (in which case the protective  */
/* character is pushed back onto the input stream), or until a blank  */
/* line is encountered (in which case the newline is pushed back onto */
/* the input stream).  Returns a NULL-terminated array of pointers to */
/* individual lines, stripped of their newline characters.  Every NUL */
/* character is stripped, and every white character is changed to a   */
/* space unless it is a newline.  If quote is 1, vacant lines will be */
/* inserted as described for the q option in par.doc.  Returns NULL   */
/* on failure.                                                        */
{
  struct buffer *cbuf = NULL, *lbuf = NULL;
  int c, empty, blank, nonquote, oldnonquote = 0, qplen;
  char ch, *ln = NULL, nullchar = '\0', *nullline = NULL, *qpstart, *qpend,
       *oldqpstart = &nullchar, *oldqpend = &nullchar, *p, *op, *vln = NULL,
       **lines = NULL;

  *errmsg = '\0';

  cbuf = newbuffer(sizeof (char), errmsg);
  if (*errmsg) goto rlcleanup;
  lbuf = newbuffer(sizeof (char *), errmsg);
  if (*errmsg) goto rlcleanup;

  for (empty = blank = 1;  ; ) {
    c = getchar();
    if (c == EOF) break;
    if (c == '\n') {
      if (blank) {
        ungetc(c,stdin);
        break;
      }
      additem(cbuf, &nullchar, errmsg);
      if (*errmsg) goto rlcleanup;
      ln = copyitems(cbuf,errmsg);
      if (*errmsg) goto rlcleanup;
      if (quote) {
        qpstart = ln;
        for (qpend = qpstart;
             *qpend && incharset(*qpend, quotechars);
             ++qpend);
        nonquote = *qpend != '\0';
        while (qpend > qpstart && qpend[-1] == ' ') --qpend;
        for (p = qpstart, op = oldqpstart;
             p < qpend && op < oldqpend && *p == *op;
             ++p, ++op);
        if (   (p < qpend && op == oldqpend  ||  p == qpend && op < oldqpend)
            && nonquote && oldnonquote) {
          qplen = p - qpstart;
          vln = malloc((qplen + 1) * sizeof (char));
          if (!vln) {
            strcpy(errmsg,outofmem);
            goto rlcleanup;
          }
          strncpy(vln,qpstart,qplen);
          vln[qplen] = '\0';
          additem(lbuf, &vln, errmsg);
          if (*errmsg) goto rlcleanup;
          vln = NULL;
        }
        oldqpstart = qpstart;
        oldqpend = qpend;
        oldnonquote = nonquote;
      }
      additem(lbuf, &ln, errmsg);
      if (*errmsg) goto rlcleanup;
      ln = NULL;
      clearbuffer(cbuf);
      empty = blank = 1;
    }
    else {
      if (empty) {
        if (incharset((char) c,protectchars)) {
          ungetc(c,stdin);
          break;
        }
        empty = 0;
      }
      if (!c) continue;
      if (isspace(c)) c = ' ';
      else blank = 0;
      ch = c;
      additem(cbuf, &ch, errmsg);
      if (*errmsg) goto rlcleanup;
    }
  }

  if (!blank) {
    additem(cbuf, &nullchar, errmsg);
    if (*errmsg) goto rlcleanup;
    ln = copyitems(cbuf,errmsg);
    if (*errmsg) goto rlcleanup;
    additem(lbuf, &ln, errmsg);
    if (*errmsg) goto rlcleanup;
    ln = NULL;
  }

  additem(lbuf, &nullline, errmsg);
  if (*errmsg) goto rlcleanup;
  lines = copyitems(lbuf,errmsg);

rlcleanup:

  if (cbuf) freebuffer(cbuf);
  if (lbuf) {
    if (!lines)
      for (;;) {
        lines = nextitem(lbuf);
        if (!lines) break;
        free(*lines);
      }
    freebuffer(lbuf);
  }
  if (ln) free(ln);
  if (vln) free(vln);

  return lines;
}


static void compresuflen(
  const char * const *lines, const char * const *endline,
  struct charset bodychars, int pre, int suf, int *ppre, int *psuf
)
/* lines is an array of strings, up to but not including endline.  */
/* Writes into *ppre and *psuf the comprelen and comsuflen of the  */
/* lines in lines.  Assumes that they have already been determined */
/* to be at least pre and suf. endline must not equal lines.       */
{
  const char *start, *end, * const *line, *p1, *p2, *start2;

  start = *lines;
  for (end = start + pre;  *end && !incharset(*end, bodychars);  ++end);
  for (line = lines + 1;  line < endline;  ++line) {
    for (p1 = start + pre, p2 = *line + pre;
         p1 < end && *p1 == *p2;
         ++p1, ++p2);
    end = p1;
  }
  *ppre = end - start;

  start2 = *lines + *ppre;
  for (end = start2;  *end;  ++end);
  for (start = end - suf;
       start > start2 && !incharset(start[-1], bodychars);
       --start);
  for (line = lines + 1;  line < endline;  ++line) {
    start2 = *line + *ppre;
    for (p2 = start2;  *p2;  ++p2);
    for (p1 = end - suf, p2 -= suf;
         p1 > start && p2 > start2 && p1[-1] == p2[-1];
         --p1, --p2);
    start = p1;
  }
  while (end - start >= 2 && *start == ' ' && start[1] == ' ') ++start;
  *psuf = end - start;
}


static void delimit(
  const char * const *lines, const char * const *endline,
  struct charset bodychars, int div, int pre, int suf, char *tags
)
/* lines is an array of strings, up to but not including endline.     */
/* Sets each character in the parallel array tags to 'f', 'p', or     */
/* 'v' according to whether the corresponding line in lines is the    */
/* first line of a paragraph, some other line in a paragraph, or a    */
/* vacant line, respectively, depending on the values of bodychars    */
/* and div, according to "par.doc".  It is assumed that the comprelen */
/* and comsuflen of the lines in lines have already been determined   */
/* to be at least pre and suf, respectively.                          */
{
  const char * const *line, *end, *p, * const *nextline;
  char *tag, *nexttag;
  int anyvacant = 0, status;

  if (endline == lines) return;

  if (endline == lines + 1) {
    *tags = 'f';
    return;
  }

  compresuflen(lines, endline, bodychars, pre, suf, &pre, &suf);

  line = lines;
  tag = tags;
  do {
    *tag = 'v';
    for (end = *line;  *end;  ++end);
    end -= suf;
    for (p = *line + pre;  p < end;  ++p)
      if (*p != ' ') {
        *tag = 'p';
        break;
      }
    if (*tag == 'v') anyvacant = 1;
    ++line;
    ++tag;
  } while (line < endline);

  if (anyvacant) {
    line = lines;
    tag = tags;
    do {
      if (*tag == 'v') {
        ++line;
        ++tag;
        continue;
      }

      for (nextline = line + 1, nexttag = tag + 1;
           nextline < endline && *nexttag == 'p';
           ++nextline, ++nexttag);

      delimit(line,nextline,bodychars,div,pre,suf,tag);

      line = nextline;
      tag = nexttag;
    } while (line < endline);

    return;
  }

  if (!div) {
    *tags = 'f';
    return;
  }

  line = lines;
  tag = tags;
  status = ((*lines)[pre] == ' ');
  do {
    if (((*line)[pre] == ' ') == status)
      *tag = 'f';
    ++line;
    ++tag;
  } while (line < endline);
}


static void setaffixes(
  const char * const *inlines, const char * const *endline,
  struct charset bodychars, struct charset quotechars,
  int hang, int quote, int *pprefix, int *psuffix
)
/* inlines is an array of strings, up to but not including   */
/* endline.  If either of *pprefix, *psuffix is less than 0, */
/* sets it to a default value based on inlines, bodychars,   */
/* quotechars, hang, and quote, according to "par.doc".      */
{
  int numin, pre, suf;
  const char *start, *p;

  numin = endline - inlines;

  if ((*pprefix < 0 || *psuffix < 0) && numin > hang + 1)
    compresuflen(inlines + hang, endline, bodychars, 0, 0, &pre, &suf);

  if (*pprefix < 0)
    if (quote && numin == hang + 1) {
      start = inlines[hang];
      for (p = start;  *p && incharset(*p, quotechars);  ++p);
      *pprefix = p - start;
    }
    else *pprefix = numin > hang + 1  ?  pre  :  0;

  if (*psuffix < 0)
    *psuffix = numin > hang + 1  ?  suf  :  0;
}


static void freelines(char **lines)
/* Frees the elements of lines, and lines itself. */
/* lines is a NULL-terminated array of strings.   */
{
  char **line;

  for (line = lines;  *line;  ++line)
    free(*line);

  free(lines);
}


main(int argc, const char * const *argv)
{
  int hang = 0, prefix = -1, suffix = -1, width = 72, div = 0, fit = 0,
      guess = 0, just = 0, last = 0, quote = 0, Report = 0, touch = -1,
      version = 0, prefixbak, suffixbak, c;
  char *parinit, *picopy = NULL, *arg, **inlines = NULL, **endline,
       *tags = NULL, **firstline, *firsttag, *end, **nextline, *nexttag,
       **outlines = NULL, **line;
  const char * const whitechars = " \f\n\r\t\v";
  struct charset bodychars = { NULL, 0 }, protectchars = { NULL, 0 },
                 quotechars = { NULL, 0 };
  errmsg_t errmsg = { '\0' };

/* Process PARINIT environment variable: */

  parinit = getenv("PARINIT");
  if (parinit) {
    picopy = malloc((strlen(parinit) + 1) * sizeof (char));
    if (!picopy) {
      strcpy(errmsg,outofmem);
      goto parcleanup;
    }
    strcpy(picopy,parinit);
    arg = strtok(picopy,whitechars);
    while (arg) {
      parsearg(arg, &hang, &prefix, &suffix, &width, &div, &fit, &guess,
               &just, &last, &quote, &Report, &touch, &version, errmsg);
      if (*errmsg || version) goto parcleanup;
      arg = strtok(NULL,whitechars);
    }
    free(picopy);
    picopy = NULL;
  }

/* Process command line arguments: */

  while (*++argv) {
    parsearg(*argv, &hang, &prefix, &suffix, &width, &div, &fit,
             &guess, &just, &last, &quote, &Report, &touch, &version, errmsg);
    if (*errmsg || version) goto parcleanup;
  }

  if (touch < 0) touch = fit || last;
  prefixbak = prefix;
  suffixbak = suffix;

/* Process other environment variables: */

  bodychars = getcharset("PARBODY", "", errmsg);
  if (*errmsg) goto parcleanup;

  protectchars = getcharset("PARPROTECT", "", errmsg);
  if (*errmsg) goto parcleanup;

  quotechars = getcharset("PARQUOTE", "> ", errmsg);
  if (*errmsg) goto parcleanup;

/* Main loop: */

  for (;;) {
    for (;;) {
      c = getchar();
      if (incharset((char) c, protectchars))
        while (c != '\n' && c != EOF) {
          putchar(c);
          c = getchar();
        }
      if (c != '\n') break;
      putchar(c);
    }
    if (c == EOF) break;
    ungetc(c,stdin);

    inlines = readlines(protectchars,quotechars,quote,errmsg);
    if (*errmsg) goto parcleanup;

    for (endline = inlines;  *endline;  ++endline);
    if (endline == inlines) {
      free(inlines);
      inlines = NULL;
      continue;
    }

    tags = malloc((endline - inlines) * sizeof(char));
    if (!tags) {
      strcpy(errmsg,outofmem);
      goto parcleanup;
    }

    delimit((const char * const *) inlines,
            (const char * const *) endline, bodychars, div, 0, 0, tags);

    firstline = inlines;
    firsttag = tags;
    do {
      if (*firsttag == 'v') {
        for (end = *firstline;  *end;  ++end);
        while (end > *firstline && end[-1] == ' ') --end;
        *end = '\0';
        puts(*firstline);
        ++firsttag;
        ++firstline;
        continue;
      }

      for (nexttag = firsttag + 1, nextline = firstline + 1;
           nextline < endline && *nexttag == 'p';
           ++nexttag, ++nextline);

      prefix = prefixbak;
      suffix = suffixbak;
      setaffixes((const char * const *) firstline,
                 (const char * const *) nextline,
                 bodychars, quotechars, hang, quote, &prefix, &suffix);
      if (width <= prefix + suffix) {
        sprintf(errmsg,
                "<width> (%d) <= <prefix> (%d) + <suffix> (%d)\n",
                width, prefix, suffix);
        goto parcleanup;
      }

      outlines =
        reformat((const char * const *) firstline,
                 (const char * const *) nextline, hang, prefix, suffix,
                  width, fit, guess, just, last, Report, touch, errmsg);
      if (*errmsg) goto parcleanup;

      for (line = outlines;  *line;  ++line)
        puts(*line);

      freelines(outlines);
      outlines = NULL;

      firsttag = nexttag;
      firstline = nextline;
    } while (firstline < endline);

    free(tags);
    tags = NULL;

    freelines(inlines);
    inlines = NULL;
  }

parcleanup:

  if (picopy) free(picopy);
  if (bodychars.individuals) free(bodychars.individuals);
  if (protectchars.individuals) free(protectchars.individuals);
  if (quotechars.individuals) free(quotechars.individuals);
  if (inlines) freelines(inlines);
  if (tags) free(tags);
  if (outlines) freelines(outlines);

  if (*errmsg) {
    printf("%s error:\n%.*s", progname, errmsg_size, errmsg);
    exit(EXIT_FAILURE);
  }

  if (version) printf("%s %s\n", progname, versionnum);

  exit(EXIT_SUCCESS);
}
