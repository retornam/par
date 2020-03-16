/*********************/
/* reformat.c        */
/* for Par 1.10      */
/* Copyright 1993 by */
/* Adam M. Costello  */
/*********************/

/* This is ANSI C code. */


#include "reformat.h"  /* Makes sure we're consistent with the */
                       /* prototype. Also includes "errmsg.h". */
#include "buffer.h"    /* Also includes <stddef.h>.            */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#undef NULL
#define NULL ((void *) 0)


struct word {
  const char *chrs;       /* Pointer to the characters in the word */
                          /* (NOT terminated by '\0').             */
  struct word *prev,      /* Pointer to previous word.             */
              *next,      /* Pointer to next word.                 */
                          /* Supposing this word were the first... */
              *nextline;  /*   Pointer to first word in next line. */
  int score,              /*   Value of the objective function.    */
      length;             /* Length of this word.                  */
};

const char * const impossibility =
  "Impossibility #%d has occurred. Please report it.\n";


static int simplebreaks(struct word *head, struct word *tail, int L, int last)

/* Chooses line breaks in a list of struct words which  */
/* maximize the length of the shortest line. L is the   */
/* maximum line length. The last line counts as a line  */
/* only if last is non-zero. head must point to a dummy */
/* word, and tail must point to the last word, whose    */
/* next field must be NULL. Returns the length of the   */
/* shortest line on success, -1 if there is a word of   */
/* length greater than L, or L if there are no lines.   */

{
  struct word *w1, *w2;
  int linelen, score;

  if (!head->next) return L;

  for (w1 = tail, linelen = w1->length;
       w1 != head && linelen <= L;
       w1 = w1->prev, linelen += 1 + w1->length) {
    w1->score = last ? linelen : L;
    w1->nextline = NULL;
  }

  for ( ;  w1 != head;  w1 = w1->prev) {
    w1->score = -1;
    for (linelen = w1->length,  w2 = w1->next;
         linelen <= L;
         linelen += 1 + w2->length,  w2 = w2->next) {
      score = w2->score;
      if (linelen < score) score = linelen;
      if (score >= w1->score) {
        w1->nextline = w2;
        w1->score = score;
      }
    }
  }

  return head->next->score;
}


static void normalbreaks(struct word *head, struct word *tail,
                         int L, int fit, int last, errmsg_t errmsg)

/* Chooses line breaks in a list of struct words according to the  */
/* policy in "par.doc" for <just> = 0 (L is <L>, fit is <fit>, and */
/* last is <last>). head must point to a dummy word, and tail must */
/* point to the last word, whose next field must be NULL.          */
{
  struct word *w1, *w2;
  int tryL, shortest, score, target, linelen, extra, minlen;

  *errmsg = '\0';
  if (!head->next) return;

  target = L;

/* Determine minimum possible difference between  */
/* the lengths of the shortest and longest lines: */

  if (fit) {
    score = L + 1;
    for (tryL = L;  ;  --tryL) {
      shortest = simplebreaks(head,tail,tryL,last);
      if (shortest < 0) break;
      if (tryL - shortest < score) {
        target = tryL;
        score = target - shortest;
      }
    }
  }

/* Determine maximum possible length of the shortest line: */

  shortest = simplebreaks(head,tail,target,last);
  if (shortest < 0) {
    sprintf(errmsg,impossibility,1);
    return;
  }

/* Minimize the sum of the squares of the differences */
/* between target and the lengths of the lines:       */

  w1 = tail;
  do {
    w1->score = -1;
    for (linelen = w1->length,  w2 = w1->next;
         linelen <= target;
         linelen += 1 + w2->length,  w2 = w2->next) {
      extra = target - linelen;
      minlen = shortest;
      if (w2)
        score = w2->score;
      else {
        score = 0;
        if (!last) extra = minlen = 0;
      }
      if (linelen >= minlen  &&  score >= 0) {
        score += extra * extra;
        if (w1->score < 0  ||  score <= w1->score) {
          w1->nextline = w2;
          w1->score = score;
        }
      }
      if (!w2) break;
    }
    w1 = w1->prev;
  } while (w1 != head);

  if (head->next->score < 0)
    sprintf(errmsg,impossibility,2);
}


static void justbreaks(
  struct word *head, struct word *tail, int L, int last, errmsg_t errmsg
)
/* Chooses line breaks in a list of struct words according     */
/* to the policy in "par.doc" for <just> = 1 (L is <L> and     */
/* last is <last>). head must point to a dummy word, and tail  */
/* must point to the last word, whose next field must be NULL. */
{
  struct word *w1, *w2;
  int numgaps, extra, score, gap, maxgap, numbiggaps;

  *errmsg = '\0';
  if (!head->next) return;

/* Determine the minimum possible largest inter-word gap: */

  w1 = tail;
  do {
    w1->score = L;
    for (numgaps = 0, extra = L - w1->length, w2 = w1->next;
         extra >= 0;
         ++numgaps, extra -= 1 + w2->length, w2 = w2->next) {
      gap = numgaps ? (extra + numgaps - 1) / numgaps : L;
      if (w2)
        score = w2->score;
      else {
        score = 0;
        if (!last) gap = 0;
      }
      if (gap > score) score = gap;
      if (score < w1->score) {
        w1->nextline = w2;
        w1->score = score;
      }
      if (!w2) break;
    }
    w1 = w1->prev;
  } while (w1 != head);

  maxgap = head->next->score;
  if (maxgap >= L) {
    strcpy(errmsg, "Cannot justify.\n");
    return;
  }

/* Minimize the sum of the squares of the numbers   */
/* of extra spaces required in each inter-word gap: */

  w1 = tail;
  do {
    w1->score = -1;
    for (numgaps = 0, extra = L - w1->length, w2 = w1->next;
         extra >= 0;
         ++numgaps, extra -= 1 + w2->length, w2 = w2->next) {
      gap = numgaps ? (extra + numgaps - 1) / numgaps : L;
      if (w2)
        score = w2->score;
      else {
        if (!last) {
          w1->nextline = NULL;
          w1->score = 0;
          break;
        }
        score = 0;
      }
      if (gap <= maxgap && score >= 0) {
        numbiggaps = extra % numgaps;
        score += (extra / numgaps) * (extra + numbiggaps) + numbiggaps;
        /* The above may not look like the sum of the squares of the numbers */
        /* of extra spaces required in each inter-word gap, but trust me, it */
        /* is. It's easier to prove graphically than algebraicly.            */
        if (w1->score < 0  ||  score <= w1->score) {
          w1->nextline = w2;
          w1->score = score;
        }
      }
      if (!w2) break;
    }
    w1 = w1->prev;
  } while (w1 != head);

  if (head->next->score < 0)
    sprintf(errmsg,impossibility,3);
}


char **reformat(const char * const *inlines, int hang,
                int prefix, int suffix, int width, int fit,
                int just, int last, int touch, errmsg_t errmsg)
{
  int numin, numout, affix, L, linelen, numgaps, extra, phase;
  const char * const *line, **suffixes = NULL, **suf, *end, *p1, *p2;
  char *q1, *q2, **outlines = NULL;
  struct word dummy, *head, *tail, *w1, *w2;
  struct buffer *pbuf = NULL;

/* Initialization: */

  *errmsg = '\0';
  dummy.next = dummy.prev = NULL;
  head = tail = &dummy;

/* Count the input lines: */

  for (line = inlines;  *line;  ++line);
  numin = line - inlines;

/* Allocate space for pointers to the suffixes: */

  if (numin) {
    suffixes = malloc(numin * sizeof (const char *));
    if (!suffixes) {
      strcpy(errmsg,outofmem);
      goto rfcleanup;
    }
  }

/* Set the pointers to the suffixes, and create the words: */

  affix = prefix + suffix;
  L = width - prefix - suffix;

  for (line = inlines, suf = suffixes;  *line;  ++line, ++suf) {
    for (end = *line;  *end;  ++end);
    if (end - *line < affix) {
      sprintf(errmsg,
              "Line %d shorter than <prefix> + <suffix> = %d + %d = %d\n",
              line - inlines + 1, prefix, suffix, affix);
      goto rfcleanup;
    }
    end -= suffix;
    *suf = end;
    p1 = *line + prefix;
    for (;;) {
      while (p1 < end && *p1 == ' ') ++p1;
      if (p1 == end) break;
      p2 = p1;
      while (p2 < end && *p2 != ' ') ++p2;
      if (p2 - p1 > L) p2 = p1 + L;
      w1 = malloc(sizeof (struct word));
      if (!w1) {
        strcpy(errmsg,outofmem);
        goto rfcleanup;
      }
      w1->next = NULL;
      w1->prev = tail;
      tail = tail->next = w1;
      w1->chrs = p1;
      w1->length = p2 - p1;
      p1 = p2;
    }
  }

/* Expand first word if preceeded only by spaces: */

  w1 = head->next;
  if (w1) {
    p1 = *inlines + prefix;
    for (p2 = p1;  *p2 == ' ';  ++p2);
    if (w1->chrs == p2) {
      w1->chrs = p1;
      w1->length += p2 - p1;
    }
  }

/* Choose line breaks according to policy in "par.doc": */

  if (just) justbreaks(head,tail,L,last,errmsg);
  else normalbreaks(head,tail,L,fit,last,errmsg);
  if (*errmsg) goto rfcleanup;

/* If touch is non-zero, change L to be the length of the longest line: */

  if (!just && touch) {
    L = 0;
    w1 = head->next;
    while (w1) {
      for (linelen = w1->length, w2 = w1->next;
           w2 != w1->nextline;
           linelen += 1 + w2->length, w2 = w2->next);
      if (linelen > L) L = linelen;
      w1 = w2;
    }
  }

/* Construct the lines: */

  pbuf = newbuffer(sizeof (char *), errmsg);
  if (*errmsg) goto rfcleanup;

  numout = 0;
  w1 = head->next;
  while (numout < hang || w1) {
    if (w1)
      for (w2 = w1->next, numgaps = 0, extra = L - w1->length;
           w2 != w1->nextline;
           ++numgaps, extra -= 1 + w2->length, w2 = w2->next);
    linelen = suffix  ||  just && (w2 || last) ?
                L + affix :
                w1 ? prefix + L - extra : prefix;
    q1 = malloc((linelen + 1) * sizeof (char));
    if (!q1) {
      strcpy(errmsg,outofmem);
      goto rfcleanup;
    }
    additem(pbuf, &q1, errmsg);
    if (*errmsg) goto rfcleanup;
    ++numout;
    q2 = q1 + prefix;
    if      (numout <= numin) memcpy(q1, inlines[numout - 1], prefix);
    else if (numin > hang)    memcpy(q1, inlines[numin - 1], prefix);
    else                      while (q1 < q2) *q1++ = ' ';
    q1 = q2;
    if (w1) {
      phase = numgaps / 2;
      for (w2 = w1;  ; ) {
        memcpy(q1, w2->chrs, w2->length);
        q1 += w2->length;
        w2 = w2->next;
        if (w2 == w1->nextline) break;
        *q1++ = ' ';
        if (just && (w1->nextline || last)) {
          phase += extra;
          while (phase >= numgaps) {
            *q1++ = ' ';
            phase -= numgaps;
          }
        }
      }
    }
    q2 += linelen - affix;
    while (q1 < q2) *q1++ = ' ';
    q2 = q1 + suffix;
    if      (numout <= numin) memcpy(q1, suffixes[numout - 1], suffix);
    else if (numin)           memcpy(q1, suffixes[numin - 1], suffix);
    else                      while(q1 < q2) *q1++ = ' ';
    *q2 = '\0';
    if (w1) w1 = w1->nextline;
  }

  q1 = NULL;
  additem(pbuf, &q1, errmsg);
  if (*errmsg) goto rfcleanup;

  outlines = copyitems(pbuf,errmsg);

rfcleanup:

  if (suffixes) free(suffixes);

  while (tail != head) {
    tail = tail->prev;
    free(tail->next);
  }

  if (pbuf) {
    if (!outlines)
      for (;;) {
        outlines = nextitem(pbuf);
        if (!outlines) break;
        free(*outlines);
      }
    freebuffer(pbuf);
  }

  return outlines;
}
