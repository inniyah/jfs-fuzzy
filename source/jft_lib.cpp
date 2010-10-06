  /*********************************************************************/
  /*                                                                   */
  /* jft_lib.c   Version 2.03  Copyright (c) 1999-2000 Jan E. Mortensen*/
  /*                                                                   */
  /* JFS-Tokeniser and penalty-functions. Tokenizer is user to         */
  /* read jfs-values from an ascii-file or from text-strings.          */
  /* Tokenizer-functions is used                                       */
  /*                                                                   */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                */
  /*    Lollandsvej 35 3.tv.                                           */
  /*    DK-2000 Frederiksberg                                          */
  /*    Denmark                                                        */
  /*                                                                   */
  /*********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "jft_lib.h"
#include "jfg_lib.h"

/************************************************************************/
/* tokenizer states:                                                    */

#define JFT_S_SPACE      0  /* in spaces between words.Start-state.     */
#define JFT_S_LETTER     1  /* in a word.                               */
#define JFT_S_COMMENT    2  /* in a comment.                            */
#define JFT_S_QOUTE      3  /* qoute                                    */
#define JFT_S_EOF        4  /* End state.                               */

/* actions */

#define JFT_A_NONE    0    /* no action.                               */
#define JFT_A_ADDLET  2    /* add letter.                              */
#define JFT_A_EWORD   3    /* end a word.                              */
#define JFT_A_QOUTE   4    /* start a word in qouetes. Not used ?      */
#define JFT_A_EEOF    5    /* unexpected eof.                          */


/*************************************************************************/
/* lokale variable                                                       */
/*************************************************************************/

static FILE *jft_tok_fp = NULL;  /* The file opened by jft_fopen().          */

static void *jft_jfr_head = NULL;

static int jft_fmode;       /* file-mode. One of JFT_FM (see jft_fopen()     */
                            /* in jft_lib.h.                                 */

#define JFT_MAX_TOKEN  258
/* static char jft_first_token[JFT_MAX_TOKEN];
   static int jft_token_read = 0;
*/
static struct jfg_sprog_desc jft_sprog;

static int jft_first = 1; /* 1: no values has been read (used to calculate */
                          /* jft_dset_desc.max_value ).                    */

static char jft_empty[2] = " ";

/*************************************************************************/
/* record definitions:                                                   */

#define JFT_FL_MAX_ARGS 256
struct jft_fl_arg_desc
{ unsigned char fltype; /* one of jFT_VT_ */
  unsigned char vno;    /* relative var-no */
  char vt_arg[8];       /* if JFT_VT_ARG   */
};
static struct jft_fl_arg_desc jft_fl_args[JFT_FL_MAX_ARGS];
static int jft_new_line_read = 0;   /* 1:new-line read, but line-no not changed */

/*************************************************************************/
/* finite-state automate-variables:                                      */

static char jft_chartypes[256];  /* chartypes[c] is c's character type       */
                             /* (chartpyes[c] in JFT_T_IGNORE...         */
                             /* JFT_T_DEFAULT.                           */

static int    jft_state;

struct jft_faut_rec { char chartype;
                      char action;
                      char new_state;
                    };

static int jft_sstate[] =   /* start of state i faut. faut[sstate[2]] is    */
                            /* first  faut-record in state 2 (comment).     */
    { 0,  /* JFT_S_SPACE                                          */
      5,  /* JFT_S_LETTER,                                        */
      10, /* JFT_S_COMMENT,                                       */
      13, /* JFT_S_QOUTE,                                         */
      16  /* JFT_S_EOF.                                           */
    };

static struct jft_faut_rec jft_faut[] =
{ {/* JFT_S_SPACE */ JFT_T_LETTER, JFT_A_ADDLET,JFT_S_LETTER}, /*  0 */
  {                  JFT_T_COMMENT,JFT_A_NONE,  JFT_S_COMMENT},/*  1 */
  {                  JFT_T_QOUTE,  JFT_A_NONE,  JFT_S_QOUTE},  /*  2 */
  {                  JFT_T_EOF,    JFT_A_NONE,  JFT_S_EOF},    /*  3 */
  {                  JFT_T_DEFAULT,JFT_A_NONE,  JFT_S_SPACE},  /*  4 */

  {/* JFT_S_LETTER*/ JFT_T_LETTER, JFT_A_ADDLET,JFT_S_LETTER}, /*  5 */
  {                  JFT_T_COMMENT,JFT_A_EWORD, JFT_S_COMMENT},/*  6 */
  {                  JFT_T_QOUTE,  JFT_A_ADDLET,JFT_S_LETTER}, /*  7 */
  {                  JFT_T_EOF,    JFT_A_EWORD, JFT_S_EOF},    /*  8 */
  {                  JFT_T_DEFAULT,JFT_A_EWORD, JFT_S_SPACE},  /*  9 */

  {/* JFT_S_COMMNT*/ JFT_T_NL,     JFT_A_NONE,  JFT_S_SPACE},  /* 10 */
  {                  JFT_T_EOF,    JFT_A_NONE,  JFT_S_EOF},    /* 11 */
  {                  JFT_T_DEFAULT,JFT_A_NONE,  JFT_S_COMMENT},/* 12 */

  {/* JFT_S_QOUTE*/  JFT_T_QOUTE,  JFT_A_EWORD, JFT_S_SPACE},  /* 13 */
  {                  JFT_T_EOF,    JFT_A_EWORD, JFT_S_EOF},    /* 14 */
  {                  JFT_T_DEFAULT,JFT_A_ADDLET,JFT_S_QOUTE},  /* 15 */

  {/* JFT_S_EOF   */ JFT_T_DEFAULT,JFT_A_EEOF,  JFT_T_EOF}     /* 16 */
};

/**************************************************************************/
/* Penalty-matrix data:                                                   */

#define JFT_MAX_PENALTY 64
struct jft_penalty_desc { float op_value;
                          float exp_value;
                          float penalty;
                          signed short op_sign;
                          signed exp_sign;
                        };
static struct jft_penalty_desc jft_penalty_mt[JFT_MAX_PENALTY];
static int jft_penalty_c = 0;

/*************************************************************************/
/* Globale definitions:                                                  */

struct jft_error_record jft_error_desc;
struct jft_data_record jft_data_desc;
struct jft_dset_record jft_dset_desc;

/*************************************************************************/
/* function-definitions:                                                 */
/*************************************************************************/

static int jft_set_error(int error_no, char *arg);
static int jft_stricmp(char *a1, char *a2);
static int nchar(int *c);
static int jft_read_first_line(void);
static int jft_record_count(void);
static void jft_set_minmax(struct jft_data_record *dd, int var_no);

/*************************************************************************/
/* Lokale funktioner                                                     */
/*************************************************************************/

static int jft_set_error(int error_no, char *arg)
{
  jft_error_desc.error_no = error_no;
  jft_error_desc.line_no = jft_dset_desc.line_no;
  strcpy(jft_error_desc.carg, arg);
  return -1;
}

static int jft_stricmp(char *a1, char *a2)
{
  int m, res;

  res = -2;
  for (m = 0; res == -2; m++)
  { if (toupper(a1[m]) > toupper(a2[m]))
      res = 1;
    else
    if (toupper(a1[m]) < toupper(a2[m]))
      res = -1;
    else
    if (a1[m] == '\0')
      res = 0;
  }
  return res;
}

static int nchar(int *c)             /* reads next char (c) from file.   */
                                     /* returns charactertype.           */
{
  *c = getc(jft_tok_fp);
  if (*c == EOF)
    return JFT_T_EOF;
  return jft_chartypes[*c];
}


static int jft_read_first_line(void)
{
  int res, m, fundet, i;
  char txt[256];
  char *atxt;
  struct jfg_var_desc vdesc;
  int fl_argc;

  fl_argc = 0;
  for (m = 0; m < JFT_FL_MAX_ARGS; m++)
  { jft_fl_args[m].fltype = JFT_VT_IGNORE;
    jft_fl_args[m].vno = 0;
  }

  res = jft_gettoken(txt);
  while (jft_dset_desc.line_no == 1 && res == 0)
  { /* split the token : value[<arg>] into value, arg  */
    fundet = 0;
    m = strlen(txt) - 1;
    if (txt[m] == '>')
    { txt[m] = '\0';
      while (m >= 0 && txt[m] != '<')
        m--;
      if (m >= 0)
      { txt[m] = '\0';
        if (jft_fl_args[fl_argc].fltype == JFT_VT_IGNORE)
        { atxt = &(txt[m + 1]);
          if (strlen(atxt) > 0 && strlen(atxt) < 8)
          { strcpy(jft_fl_args[fl_argc].vt_arg, atxt);
            if (strlen(atxt) == 1 && (atxt[0] == 'K' || atxt[0] == 'k'))
            { jft_fl_args[fl_argc].fltype = JFT_VT_KEY;
              jft_dset_desc.key = 1;
              fundet = 1;
            }
            else
              jft_fl_args[fl_argc].fltype = JFT_VT_ARG;
          }
        }
      }
    }
    for (m = 0; fundet == 0 && m < jft_sprog.var_c; m++)
    { jfg_var(&vdesc, jft_jfr_head, m);
      if (jft_stricmp(vdesc.name, txt) == 0)
      { if (m >= jft_sprog.f_ivar_no
            && m < jft_sprog.f_ivar_no + jft_sprog.ivar_c)
        { jft_fl_args[fl_argc].fltype = JFT_VT_INPUT;
          jft_fl_args[fl_argc].vno = m - jft_sprog.f_ivar_no;
          fundet = 1;
          jft_dset_desc.input = 1;
        }
        else
        if (m >= jft_sprog.f_ovar_no
            && m < jft_sprog.f_ovar_no + jft_sprog.ovar_c)
        { jft_fl_args[fl_argc].fltype = JFT_VT_EXPECTED;
          jft_fl_args[fl_argc].vno = m - jft_sprog.f_ovar_no;
          fundet = 1;
          jft_dset_desc.expected = 1;
        }
      }
    }
    fl_argc++;
    if (fl_argc >= JFT_FL_MAX_ARGS)
      return jft_set_error(15, jft_empty);
    res = jft_gettoken(txt);
  }

  if (jft_dset_desc.input == 1)
  { for (m = 0; m < jft_sprog.ivar_c; m++)
    { fundet = 0;
      for (i = 0; fundet == 0 && i < fl_argc; i++)
      { if (jft_fl_args[i].fltype == JFT_VT_INPUT && jft_fl_args[i].vno == m)
          fundet = 1;
      }
      if (fundet == 0)
      { jfg_var(&vdesc, jft_jfr_head, jft_sprog.f_ivar_no + m);
        res = jft_set_error(15, vdesc.name);
      }
    }
  }
  if (jft_dset_desc.expected == 1)
  { for (m = 0; m < jft_sprog.ovar_c; m++)
    { fundet = 0;
      for (i = 0; fundet == 0 && i < fl_argc; i++)
      { if (jft_fl_args[i].fltype == JFT_VT_EXPECTED && jft_fl_args[i].vno == m)
          fundet = 1;
      }
      if (fundet == 0)
      { jfg_var(&vdesc, jft_jfr_head, jft_sprog.f_ovar_no + m);
        res = jft_set_error(15, vdesc.name);
      }
    }
  }
  jft_dset_desc.record_size = fl_argc;
  jft_rewind(); /* because one token to much has been read */
  if (fl_argc == 0)
    res = jft_set_error(21, jft_empty);
  return res;
}

/*************************************************************************/
/* Externe funktioner                                                    */
/*************************************************************************/


void jft_init(void *jf_head)
{
  int m;

  for (m = 0; m < 256; m++)
  { jft_chartypes[m] = JFT_T_LETTER;
    if (isspace(m))
      jft_chartypes[m] = JFT_T_SPACE;
  };
  jft_chartypes['#']  = JFT_T_COMMENT;
  jft_chartypes['\n'] = JFT_T_NL;
  jft_chartypes['"']  = JFT_T_QOUTE;
  jft_chartypes['\''] = JFT_T_QOUTE;
  jft_error_desc.error_no = 0;
  jft_error_desc.line_no = 0;
  jft_error_desc.carg[0] = '\0';
  jft_jfr_head = jf_head;
  if (jft_jfr_head != NULL)
   jfg_sprg(&jft_sprog, jft_jfr_head);
}

void jft_dd_copy(struct jft_data_record *dest,
                 struct jft_data_record *source)
{
  dest->farg   = source->farg;
  dest->conf   = source->conf;
  dest->imin   = source->imin;
  dest->imax   = source->imax;
  dest->sarg   = source->sarg;
  dest->mode   = source->mode;
  strcpy(dest->token, source->token);
  dest->vtype  = source->vtype;
  strcpy(dest->vt_arg, source->vt_arg);
  dest->vno    = source->vno;
}

static int jft_record_count(void)
{
  int m, res;
  char txt[JFT_MAX_TOKEN];

  res = 0;
  jft_dset_desc.record_count = 0;
  while (res == 0)
  { for (m = 0; res == 0 && m < jft_dset_desc.record_size; m++)
    { res = jft_gettoken(txt);
      if (res != 0 && m != 0)
      { jft_rewind();
        return jft_set_error(11, jft_empty);
      }
    }
    if (res == 0)
      jft_dset_desc.record_count++;
  }
  jft_rewind();
  return jft_dset_desc.record_count;
}

int jft_fopen(char *fname, int fmode, int count_records)
{
  int res, fm, m, cm, c, fl_argc;

  res = 0;
  jft_state = 0;
  jft_first = 1;
  jft_new_line_read = 0;
  jft_dset_desc.line_no = 1;
  jft_dset_desc.record_no = 1;
  jft_dset_desc.field_no = -1;
  jft_dset_desc.record_count = 0;
  jft_dset_desc.record_size = 0;
  jft_dset_desc.expected = 0;
  jft_dset_desc.input = 0;
  jft_dset_desc.key = 0;
  jft_dset_desc.max_value = 0.0;
  jft_fmode = fmode;
  if (jft_fmode != JFT_FM_NONE && jft_jfr_head == NULL)
    return jft_set_error(4, jft_empty);
  if ((jft_tok_fp = fopen(fname, "r")) == NULL)
    return jft_set_error(1, fname);

  fl_argc = 0;
  if (jft_fmode == JFT_FM_FIRST_LINE)
    res = jft_read_first_line();
  else
  { fm = jft_fmode;
    for (c = 0; c < 3; c++)
    { cm = fm / 100;  /* cm = first_digit in jft_fmode */
      switch (cm)
      { case 0:
          break;
        case 1: /* input */
          for (m = 0; m < jft_sprog.ivar_c; m++)
          { jft_fl_args[fl_argc].fltype = JFT_VT_INPUT;
            jft_fl_args[fl_argc].vno = m;
            fl_argc++;
          }
          jft_dset_desc.input = 1;
          break;
        case 2: /* expected */
          for (m = 0; m < jft_sprog.ovar_c; m++)
          { jft_fl_args[fl_argc].fltype = JFT_VT_EXPECTED;
            jft_fl_args[fl_argc].vno = m;
            fl_argc++;
          }
          jft_dset_desc.expected = 1;
          break;
        case 3: /* key */
          jft_fl_args[fl_argc].fltype = JFT_VT_KEY;
          fl_argc++;
          jft_dset_desc.key = 1;
          break;
        default:
          return jft_set_error(17, jft_empty);
      }
      fm = 10 *(fm % 100); /* remove first digit, move digits left */
    }
    jft_dset_desc.record_size = fl_argc;
  }
  jft_dset_desc.field_no = -1;
  if (count_records == 1)
    jft_record_count();
  return res;
}

int jft_rewind(void)
{
  int res, m;
  char dummy[JFT_MAX_TOKEN];

  if (jft_tok_fp == NULL)
    return jft_set_error(2, jft_empty);
  rewind(jft_tok_fp);
  jft_state = 0;
  jft_dset_desc.line_no = 1;
  jft_dset_desc.record_no = 1;
  jft_dset_desc.field_no = -1;
  jft_first = 1;
  jft_new_line_read = 0;
  if (jft_fmode == JFT_FM_FIRST_LINE)
  { res = 0;
    for (m = 0; res == 0 && m < jft_dset_desc.record_size; m++)
      res = jft_gettoken(dummy);
  }
  jft_dset_desc.field_no = -1;
  jft_dset_desc.record_no = 1;
  return 0;
}

void jft_char_type(int char_no, int ctype)
{
  if (char_no < 256 && ctype < 8)
    jft_chartypes[char_no] = ctype;
}

void jft_close(void)
{
  if (jft_tok_fp != NULL)
    fclose(jft_tok_fp);
  jft_tok_fp = NULL;
}

int jft_atof(float *f, char *a)
{
  int m;
  char c;
  int state;  /* 0: before sign,                    */
              /* 1: afersign, before decimal-point, */
              /* 2: after decimal point.            */

  state = 0;
  *f = 0.0;
  for (m = 0; a[m] != '\0'; m++)
  { if (a[m] == ',')  /* Both comma and point is decimal-char */
      a[m] = '.';
    c = a[m];
    switch (state)
    { case 0:
        if (c != ' ')
        { if (c == '-')
            state = 1;
          else
          if (c == '.')
            state = 2;
          else
          if (isdigit(c))
            state = 1;
          else
            return jft_set_error(9, a);
        }
        break;
      case 1:
        if (!isdigit(c))
        { if (c == '.')
            state = 2;
          else
            return jft_set_error(9, a);
        }
        break;
      case 2:
        if (!isdigit(c))
          return jft_set_error(9, a);
        break;
    }
  }
  if (state == 0)
    *f = 0.0;
  else
    *f = atof(a);
  return 0;
}


static void jft_set_minmax(struct jft_data_record *dd, int var_no)
{
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  struct jfg_adjectiv_desc adesc;

  jfg_var(&vdesc, jft_jfr_head, var_no);
  jfg_domain(&ddesc, jft_jfr_head, vdesc.domain_no);
  if (ddesc.flags & JFS_DF_MINENTER)
    dd->imin = ddesc.dmin;
  else
  { if (vdesc.fzvar_c >= 2)
    { jfg_adjectiv(&adesc, jft_jfr_head, vdesc.f_adjectiv_no);
      dd->imin = adesc.center;
    }
    else
    { if (ddesc.flags & JFS_DF_MAXENTER)
        dd->imin = ddesc.dmax - 1.0;
      else
        dd->imin = 0.0;
    }
  }
  if (ddesc.flags & JFS_DF_MAXENTER)
    dd->imax = ddesc.dmax;
  else
  { if (vdesc.fzvar_c >= 2)
    { jfg_adjectiv(&adesc, jft_jfr_head,
                    vdesc.f_adjectiv_no + vdesc.fzvar_c - 1);
      dd->imax = adesc.center;
    }
    else
      dd->imax = dd->imin + 1.0;
  }
}

int jft_atov(struct jft_data_record *dd, int var_no, char *a)
{
  int m, res, colon, h;
  float f;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  struct jfg_adjectiv_desc adesc;

  res = 0;
  dd->conf = 1.0;
  strcpy(dd->token, a);
  jfg_var(&vdesc, jft_jfr_head, var_no);
  if (var_no >= jft_sprog.f_ivar_no
      && var_no < jft_sprog.f_ivar_no + jft_sprog.ivar_c)
  { dd->vtype = JFT_VT_INPUT;
    dd->vno = var_no - jft_sprog.f_ivar_no;
  }
  else
  if (var_no >= jft_sprog.f_ovar_no
      && var_no < jft_sprog.f_ovar_no + jft_sprog.ovar_c)
  { dd->vtype = JFT_VT_EXPECTED;
    dd->vno = var_no - jft_sprog.f_ovar_no;
  }
  else
  if (var_no >= jft_sprog.f_lvar_no
      && var_no <= jft_sprog.f_lvar_no + jft_sprog.lvar_c)
  { dd->vtype = JFT_VT_LOCAL;
    dd->vno = var_no - jft_sprog.f_lvar_no;
  }

  if (a[0] == '\0' || a[0] == '?')
  { dd->mode = JFT_DM_MISSING;
    dd->farg = vdesc.default_val;
    dd->conf = 0.0;
  }
  else
  if (a[0] == '*')
  { dd->mode = JFT_DM_INTERVAL;
    dd->farg = vdesc.default_val;
    if (a[1] == '\0' || a[1] == 'm')  /* arg= '*' or '*m' */
    { if (a[1] == '\0')
        dd->sarg = -1;   /* no count */
      else
        dd->sarg = -2;   /* multil count */
      jft_set_minmax(dd, var_no);
      if (vdesc.fzvar_c < 1)
      { dd->sarg = (int) (dd->imax - dd->imin + 1.0);
        if (dd->sarg < 2)
          dd->sarg = 11;
      }
    }
    else /* arg = *count[:begin:end] */
    { m = 1;
      while (isdigit(a[m]))
        m++;
      if (a[m] == '\0' || a[m] == ' ') /* arg = *count  */
      { a[m] = '\0';
        dd->sarg = atoi(&(a[1]));
        jft_set_minmax(dd, var_no);
        if (dd->sarg < 2)
        { dd->sarg = 2;
          res = 9;
        }
      }
      else /* arg = *count:begin:end  */
      if (a[m] == ':')
      { dd->mode = JFT_DM_MMINTERVAL;
        a[m] = '\0';
        dd->sarg = atoi(&(a[1]));
        if (dd->sarg < 2)
          dd->sarg = 2;
        m++;
        h = m;
        while (a[m] != ':')
        { if (a[m] == '\0')
            res = 14;
          else
            m++;
        }
        if (res == 0)
        { a[m] = '\0';
          res = jft_atof(&f, &(a[h]));
        }
        if (res == 0)
        { dd->imin = f;
          m++;
          res = jft_atof(&f, &(a[m]));
          if (res == 0)
            dd->imax = f;
        }
      }
      else
        res = 9;
    }
  }
  else /* value[:conf] or adjective:[conf] */
  { colon = -1;
    for (m = 0; colon == -1 && m < ((int)strlen(a)); m++)
    { if (a[m] == ':')
      { colon = m;
        a[m] = '\0';
      }
    }
    res = jft_atof(&f, a);
    if (res == 0)
    { dd->mode = JFT_DM_NUMBER;
      jfg_domain(&ddesc, jft_jfr_head, vdesc.domain_no);
      if ((ddesc.flags & JFS_DF_MINENTER) != 0 && f < ddesc.dmin)
      { f = ddesc.dmin;
        res = 10;
      }
      else
      if ((ddesc.flags & JFS_DF_MAXENTER) != 0 && f > ddesc.dmax)
      { f = ddesc.dmax;
        res = 10;
      }
      dd->farg = f;
    }
    else
    { res = 13;
      for (m = 0; res == 13 && m < vdesc.fzvar_c; m++)
      { jfg_adjectiv(&adesc, jft_jfr_head, vdesc.f_adjectiv_no + m);
        if (strcmp(a, adesc.name) == 0)
        { res = 0;
          dd->mode = JFT_DM_ADJECTIV;
          dd->sarg = m;
          dd->farg = adesc.center;
        }
      }
    }
    if (colon != -1)
    { res = jft_atof(&f, &(a[colon + 1]));
      if (res == 0)
      { if (f <= 1.0)
          dd->conf = f;
        else
          res = 10;
      }
    }
  }
  if (res == 0)
  { if (jft_first == 1 || dd->farg > jft_dset_desc.max_value)
    { jft_first = 0;
      jft_dset_desc.max_value = dd->farg;
    }
  }
  if (res != 0)
  { dd->mode = JFT_DM_ERROR;
    jft_set_error(res, dd->token);
    return -1;
  }
  return 0;
}

int jft_gettoken(char *t)
{
  int fundet, m , ctype, akt_char;
  int res, ff_sent;

  res = 1;
  ff_sent = 0;
  t[0] = '\0';
  while (jft_state != JFT_S_EOF && res == 1)
  {
    ctype = nchar(&akt_char);
    if (ctype != JFT_T_IGNORE)
    { fundet = 0;
      if (jft_new_line_read == 1)
      { jft_dset_desc.line_no++;
        jft_new_line_read = 0;
      }
      for (m = jft_sstate[jft_state]; fundet == 0; m++)
      { if (jft_faut[m].chartype == ctype
            || jft_faut[m].chartype == JFT_T_DEFAULT)
        { switch (jft_faut[m].action)
          { case JFT_A_NONE:
             break;
            case JFT_A_ADDLET:
              t[ff_sent] = akt_char;
              ff_sent++;
              break;
            case JFT_A_EWORD:
              t[ff_sent] = '\0';
              ff_sent++;
              res = 0;
              break;
            case JFT_A_EEOF:
              t[ff_sent] = '\0';
              res = 11;
              break;
          }
          jft_state = jft_faut[m].new_state;
          fundet = 1;
          if (ff_sent >= JFT_MAX_TOKEN)
          { t[JFT_MAX_TOKEN - 1] = '\0';
            return jft_set_error(18, t);
          }
        }
      }
      if (ctype == JFT_T_NL)
        jft_new_line_read = 1;
    }
  }
  jft_dset_desc.field_no++;
  if (jft_dset_desc.field_no >= jft_dset_desc.record_size)
  { jft_dset_desc.field_no = 0;
    jft_dset_desc.record_no++;
  }
  if (res == 1)
    res = 11;
  if (res == 11 && strlen(t) > 0)
    res = 0;
  return res;
}

int jft_getvar(struct jft_data_record *dd, int var_no)
{
  int res;
  char tok[256];

  res = jft_gettoken(tok);
  if (res == 0)
    res = jft_atov(dd, var_no, tok);
  return res;
}


int jft_getdata(struct jft_data_record *dd)
{
/* Reads the next value from the file opened by jft_fopen() into the    */
/* data-description <dd>.                                               */
/* return  0: succes,                                                   */
/*        11: eof,                                                      */
/*        -1: error, see <jft_error_desc> for details.                  */

  int res;

  res = jft_gettoken(dd->token);
  dd->vtype = jft_fl_args[jft_dset_desc.field_no].fltype;
  dd->vt_arg[0] = '\0';
  if (res == 0)
  { switch (jft_fl_args[jft_dset_desc.field_no].fltype)
    { case JFT_VT_IGNORE:
      case JFT_VT_KEY:
        break;
      case JFT_VT_ARG:
        strcpy(dd->vt_arg, jft_fl_args[jft_dset_desc.field_no].vt_arg);
        break;
      case JFT_VT_INPUT:
        res = jft_atov(dd, jft_sprog.f_ivar_no
                           + jft_fl_args[jft_dset_desc.field_no].vno, dd->token);
        break;
      case JFT_VT_EXPECTED:
        res = jft_atov(dd, jft_sprog.f_ovar_no
                           + jft_fl_args[jft_dset_desc.field_no].vno, dd->token);
        break;
    }
  }

  return res;
}

int jft_getrecord(float *ipvalues, float *ipconfs, float *expvalues,
                  char *key)
{
/* Reads the next record into <ipvalues>, <ipconfs> (confidences input),*/
/* <exp_values> (hvis expected) and the text-value of key-field is      */
/* written to <key>.                                                    */
/* return   0: sucees,                                                  */
/*         11: eof,                                                     */
/*         -1: error.                                                   */

  int m, res;
  struct jft_data_record dd;

  res = 0;
  for (m = 0; res == 0 && m < jft_dset_desc.record_size; m++)
  { res = jft_getdata(&dd);
    if (res == 11 && m != 0)
      res = jft_set_error(11, jft_empty);
    if (res == 0)
    { if (dd.vtype == JFT_VT_INPUT)
      { if (ipvalues != NULL)
          ipvalues[dd.vno] = dd.farg;
        if (ipconfs != NULL)
          ipconfs[dd.vno] = dd.conf;
      }
      else
      if (dd.vtype == JFT_VT_EXPECTED && expvalues != NULL)
        expvalues[dd.vno] = dd.farg;
      else
      if (dd.vtype == JFT_VT_KEY && key != NULL)
        strcpy(key, dd.token);
    }
  }
  return res;
};

/************************************************************************/
/* penalty-functions:                                                   */

int jft_penalty_read(char *fname)
{
  struct jft_data_record dd;
  int m, state;
  char token[256];
  char *txt;

  jft_init(jft_jfr_head);
  if ((m = jft_fopen(fname, JFT_FM_NONE, 0)) != 0)
    return -1;
  if (jft_sprog.ovar_c != 1)
  { jft_close();
    return jft_set_error(19, jft_empty);
  }
  state = 0;
  while (state >= 0)
  { m = jft_gettoken(token);
    txt = token;
    if (m == 11 && state == 0)
      state = -1;
    else
    { if (m != 0)
        return -1;
      if (jft_penalty_c >= JFT_MAX_PENALTY)
        return jft_set_error(20, jft_empty);
      switch (state)
      { case 0:   /* output */
          if (token[0] == '-' || token[0] == '!')
          { txt++;
            jft_penalty_mt[jft_penalty_c].op_sign = -1;
          }
          else
            jft_penalty_mt[jft_penalty_c].op_sign = 1;
          m = jft_atov(&dd, jft_sprog.f_ovar_no, txt);
          jft_penalty_mt[jft_penalty_c].op_value = dd.farg;
          break;
        case 1:   /* expected */
          if (token[0] == '-' || token[0] == '!')
          { txt++;
            jft_penalty_mt[jft_penalty_c].exp_sign = -1;
          }
          else
            jft_penalty_mt[jft_penalty_c].exp_sign = 1;
          m = jft_atov(&dd, jft_sprog.f_ovar_no, txt);
          jft_penalty_mt[jft_penalty_c].exp_value = dd.farg;
          break;
        case 2:   /* penalty-value */
          m = jft_atof(&(jft_penalty_mt[jft_penalty_c].penalty), txt);
          jft_penalty_c++;
          break;
      }
      if (m != 0)
        return -1;
      state += 1;
      if (state > 2)
        state = 0;
    }
  }
  jft_close();
  return jft_penalty_c;
}

float jft_penalty_calc(float op_value, float exp_value)
{
  float dist;
  int m;

  dist = 0.0;
  if (jft_penalty_c != 0)
  { for (m = 0; m < jft_penalty_c; m++)
    { if (   (jft_penalty_mt[m].op_sign == 1
              && fabs(op_value - jft_penalty_mt[m].op_value) < 0.001)
          || (jft_penalty_mt[m].op_sign == -1
              && fabs(op_value - jft_penalty_mt[m].op_value) >= 0.001))
      { if (   (jft_penalty_mt[m].exp_sign == 1
                && fabs(exp_value - jft_penalty_mt[m].exp_value) < 0.001)
            || (jft_penalty_mt[m].exp_sign == -1
                && fabs(exp_value - jft_penalty_mt[m].exp_value) >= 0.001))
        { dist = jft_penalty_mt[m].penalty;
          break;
        }
      }
    }
  }
  return dist;
}


