/*********************/
/* par.c             */
/* for Par 1.32      */
/* Copyright 1993 by */
/* Adam M. Costello  */
/*********************/

/* This is ANSI C code. */


#include "charset.h"   /* Also includes "errmsg.h". */
#include "buffer.h"    /* Also includes <stddef.h>. */
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


const char * const usagemsg =
"\n"
"Usage:\n"
"\n"
"par [help] [version] [B<op><set>] [P<op><set>] [Q<op><set>] [h[<hang>]]\n"
"    [p<prefix>] [s<suffix>] [w<width>] [c[<cap>]] [d[<div>]] [f[<fit>]]\n"
"    [g[<guess>]] [j[<just>]] [l[<last>]] [q[<quote>]] [R[<Report>]]\n"
"    [t[<touch>]]\n"
"\n"
"help       usage message             |"
                                  "Boolean parameters:\n"
"version    version number            |"
                                  "Option:   If 1:\n"
"B<op><set> as <op> is =/+/-,         |"
                                  "c<cap>    all words count as capitalized\n"
"           replace/augment/diminish  |"
                                  "d<div>    indentation delimits paragraphs\n"
"           body chars by <set>       |"
                                  "f<fit>    paragraphs are narrowed to\n"
"P<op><set> ditto for protective chars|"
                                  "          smoothen the right edge\n"
"Q<op><set> ditto for quote chars     |"
                                  "g<guess>  wide sentence breaks preserved\n"
"h<hang>    first <hang> lines of each|"
                                  "j<just>   paragraphs are justified\n"
"           paragraph not searched for|"
                                  "l<last>   last lines treated like others\n"
"           common prefixes & suffixes|"
                                  "q<quote>  vacant lines supplied between\n"
"p<prefix>  prefix length             |"
                                  "          different quote nesting levels\n"
"s<suffix>  suffix length             |"
                                  "R<Report> too-long words cause errors\n"
"w<width>   max output line length    |"
                                  "t<touch>  suffixes are moved left\n"
"\n"
"See par.doc or par.1 (the man page) for more information.\n"
;


static int digtoint(char c)

/* Returns the value represented by the digit c, or -1 if c is not a digit. */
{
  const char *p, * const digits = "0123456789";

  if (!c) return -1;
  p = strchr(digits,c);
  return  p  ?  p - digits  :  -1;

  /* We can't simply return c - '0' because this is ANSI C code,  */
  /* so it has to work for any character set, not just ones which */
  /* put the digits together in order.  Also, an array that could */
  /* be referenced as digtoint[c] might be bad because there's no */
  /* upper limit on CHAR_MAX.                                     */
}


static int strtoudec(const char *s, int *pn)

/* Converts the longest prefix of string s consisting of decimal   */
/* digits to an integer, which is stored in *pn.  Normally returns */
/* 1.  If *s is not a digit, then *pn is not changed, but 1 is     */
/* still returned.  If the integer represented is greater than     */
/* 9999, then *pn is not changed and 0 is returned.                */
{
  int n = 0, d;

  d = digtoint(*s);
  if (d < 0) return 1;

  do {
    if (n >= 1000) return 0;
    n = 10 * n + d;
    d = digtoint(*++s);
  } while (d >= 0);

  *pn = n;

  return 1;
}


static void parsearg(
  const char *arg, int *phelp, int *pversion, charset *bodychars,
  charset *protectchars, charset *quotechars, int *phang, int *pprefix,
  int *psuffix, int *pwidth, int *pcap, int *pdiv, int *pfit, int *pguess,
  int *pjust, int *plast, int *pquote, int *pReport, int *ptouch,
  errmsg_t errmsg
)
/* Parses the command line argument in *arg, setting the objects pointed to */
/* by the other pointers as appropriate. *phelp and *pversion are boolean   */
/* flags indicating whether the help and version options were supplied.     */
{
  const char *savearg = arg;
  charset *chars, *change;
  char oc;
  int n;

  *errmsg = '\0';

  if (*arg == '-') ++arg;

  if (!strcmp(arg, "help")) {
    *phelp = 1;
    return;
  }

  if (!strcmp(arg, "version")) {
    *pversion = 1;
    return;
  }

  if (*arg == 'B' || *arg == 'P' || *arg == 'Q' ) {
    chars =  *arg == 'B'  ?  bodychars    :
             *arg == 'P'  ?  protectchars :
          /* *arg == 'Q' */  quotechars   ;
    ++arg;
    if (*arg != '='  &&  *arg != '+'  &&  *arg != '-') goto badarg;
    change = parsecharset(arg + 1, errmsg);
    if (change) {
      if      (*arg == '=')   csswap(chars,change);
      else if (*arg == '+')   csadd(chars,change,errmsg);
      else  /* *arg == '-' */ csremove(chars,change,errmsg);
      freecharset(change);
    }
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
        if      (oc == 'w')   *pwidth  = n;
        else if (oc == 'p')   *pprefix = n;
        else  /* oc == 's' */ *psuffix = n;
      }
    }
    else {
      if (n > 1) goto badarg;
      if      (oc == 'c') *pcap    = n;
      else if (oc == 'd') *pdiv    = n;
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
  *phelp = 1;
}


static char **readlines(const charset *protectchars,
                        const charset *quotechars, int quote, errmsg_t errmsg)

/* Reads lines from stdin until EOF, or until a line beginning with a */
/* protective character is encountered (in which case the protective  */
/* character is pushed back onto the input stream), or until a blank  */
/* line is encountered (in which case the newline is pushed back onto */
/* the input stream).  Returns a NULL-terminated array of pointers to */
/* individual lines, stripped of their newline characters.  Every NUL */
/* character is stripped, and every white character is changed to a   */
/* space unless it is a newline.  If quote is 1, vacant lines will be */
/* supplied as described for the q option in par.doc.  Returns NULL   */
/* on failure.                                                        */
{
  buffer *cbuf = NULL, *lbuf = NULL;
  int c, empty, blank, nonquote, oldnonquote = 0, vlnlen;
  char ch, *ln = NULL, nullchar = '\0', *nullline = NULL, *qpend,
       *oldln = &nullchar, *oldqpend = &nullchar, *p, *op, *vln = NULL,
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
        for (qpend = ln;
             *qpend && csmember(*qpend, quotechars);
             ++qpend);
        nonquote = *qpend != '\0';
        while (qpend > ln && qpend[-1] == ' ') --qpend;
        for (p = ln, op = oldln;
             p < qpend && op < oldqpend && *p == *op;
             ++p, ++op);
        if (    p < qpend && op == oldqpend && oldnonquote
            ||  p == qpend && op < oldqpend && nonquote   )
          if (oldnonquote && nonquote) {
            vlnlen = p - ln;
            vln = malloc((vlnlen + 1) * sizeof (char));
            if (!vln) {
              strcpy(errmsg,outofmem);
              goto rlcleanup;
            }
            strncpy(vln,ln,vlnlen);
            vln[vlnlen] = '\0';
            additem(lbuf, &vln, errmsg);
            if (*errmsg) goto rlcleanup;
            vln = NULL;
          }
          else
            if      (oldnonquote) ln[oldqpend - oldln] = '\0';
            else if (nonquote)    oldln[qpend - ln]    = '\0';
        oldln = ln;
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
        if (csmember((char) c, protectchars)) {
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
  const charset *bodychars, int pre, int suf, int *ppre, int *psuf
)
/* lines is an array of strings, up to but not including endline.  */
/* Writes into *ppre and *psuf the comprelen and comsuflen of the  */
/* lines in lines.  Assumes that they have already been determined */
/* to be at least pre and suf. endline must not equal lines.       */
{
  const char *start, *end, * const *line, *p1, *p2, *start2;

  start = *lines;
  for (end = start + pre;  *end && !csmember(*end, bodychars);  ++end);
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
       start > start2 && !csmember(start[-1], bodychars);
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
  const charset *bodychars, int div, int pre, int suf, char *tags
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
  const charset *bodychars, const charset *quotechars,
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
      for (p = start;  *p && csmember(*p, quotechars);  ++p);
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
  int help = 0, version = 0, hang = 0, prefix = -1, suffix = -1, width = 72,
      cap = 0, div = 0, fit = 0, guess = 0, just = 0, last = 0, quote = 0,
      Report = 0, touch = -1, prefixbak, suffixbak, c;
  charset *bodychars = NULL, *protectchars = NULL, *quotechars = NULL;
  char *parinit = NULL, *arg, **inlines = NULL, **endline,
       *tags = NULL, **firstline, *firsttag, *end, **nextline, *nexttag,
       **outlines = NULL, **line;
  const char *env, * const whitechars = " \f\n\r\t\v";
  errmsg_t errmsg = { '\0' };

/* Process environment variables: */

  env = getenv("PARBODY");
  if (!env) env = "";
  bodychars = parsecharset(env,errmsg);
  if (*errmsg) {
    help = 1;
    goto parcleanup;
  }

  env = getenv("PARPROTECT");
  if (!env) env = "";
  protectchars = parsecharset(env,errmsg);
  if (*errmsg) {
    help = 1;
    goto parcleanup;
  }

  env = getenv("PARQUOTE");
  if (!env) env = "> ";
  quotechars = parsecharset(env,errmsg);
  if (*errmsg) {
    help = 1;
    goto parcleanup;
  }

  env = getenv("PARINIT");
  if (env) {
    parinit = malloc((strlen(env) + 1) * sizeof (char));
    if (!parinit) {
      strcpy(errmsg,outofmem);
      goto parcleanup;
    }
    strcpy(parinit,env);
    arg = strtok(parinit,whitechars);
    while (arg) {
      parsearg(arg, &help, &version, bodychars, protectchars,
               quotechars, &hang, &prefix, &suffix, &width, &cap, &div,
               &fit, &guess, &just, &last, &quote, &Report, &touch, errmsg);
      if (*errmsg || help || version) goto parcleanup;
      arg = strtok(NULL,whitechars);
    }
    free(parinit);
    parinit = NULL;
  }

/* Process command line arguments: */

  while (*++argv) {
    parsearg(*argv, &help, &version, bodychars, protectchars,
             quotechars, &hang, &prefix, &suffix, &width, &cap, &div,
             &fit, &guess, &just, &last, &quote, &Report, &touch, errmsg);
    if (*errmsg || help || version) goto parcleanup;
  }

  if (touch < 0) touch = fit || last;
  prefixbak = prefix;
  suffixbak = suffix;

/* Main loop: */

  for (;;) {
    for (;;) {
      c = getchar();
      if (csmember((char) c, protectchars))
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
                  width, cap, fit, guess, just, last, Report, touch, errmsg);
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

  if (bodychars) freecharset(bodychars);
  if (protectchars) freecharset(protectchars);
  if (quotechars) freecharset(quotechars);
  if (parinit) free(parinit);
  if (inlines) freelines(inlines);
  if (tags) free(tags);
  if (outlines) freelines(outlines);

  if (*errmsg) printf("par error:\n%.*s", errmsg_size, errmsg);
  if (version) puts("par 1.32");
  if (help)    fputs(usagemsg,stdout);

  exit(*errmsg ? EXIT_FAILURE : EXIT_SUCCESS);
}
