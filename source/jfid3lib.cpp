  /*************************************************************************/
  /*                                                                       */
  /* jfid3lib.cpp Version  2.03  Copyright (c) 1998-2000 Jan E. Mortensen  */
  /*                                                                       */
  /* JFS Rule discover using ID3.                                          */
  /*                                                                       */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                    */
  /*    Lollandsvej 35 3.tv.                                               */
  /*    DK-2000 Frederiksberg                                              */
  /*    Denmark                                                            */
  /*                                                                       */
  /*************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfp_lib.h"
#include "jft_lib.h"
#include "jfid3lib.h"

static void *jfid3_head = NULL;

#define JFRD_VMAX 50

static float jfid3_ivalues[JFRD_VMAX];
static float jfid3_ovalues[JFRD_VMAX];
static float jfid3_expected[JFRD_VMAX];
static float jfid3_confidences[JFRD_VMAX];
static float jfid3_domsize[JFRD_VMAX];

#define JFID3_RC_SCORE 0
#define JFID3_RC_COUNT 1
static int jfid3_res_confl = JFID3_RC_SCORE;

/*****************************************************************************/
/* Data-structures to describe the variables in the 'extern jfrd'-statement, */
/* and the value of the current rule. The current rule is a rule copied to   */
/* the jfid3_if_vars cur-value, the jfid3_then_var's cur-value, and the      */
/* variables: jfid3_dscore, jfid3_conflict and jfid3_data_set_no.            */


struct jfid3_multi_desc { signed short var_no;
                          signed short cur;
                          signed short adjectiv_c;
                          unsigned short f_fzvar_no;
                       };

static struct jfid3_multi_desc jfid3_if_vars[JFRD_VMAX];
/* the input-variables in the 'extern jfrd'-statements */
static int jfid3_ff_if_vars = 0;  /* real size of the jfid3_if_vars-array */

static struct jfid3_multi_desc jfid3_then_var;

float jfid3_dscore;               /* score of current rule.              */
unsigned long jfid3_data_set_no;  /* the data-set number from which the  */
                                  /* current rule is created.            */
static char jfid3_conflict;       /* 1: the current rule conflicts with  */
                                  /*    another rule.                    */

/**********************************************************************/
/* Variables used in id3-reduction:                                   */

struct jfid3_stak_rec
           { unsigned short atribut; /* var = if_vars[atribut].var_no */
             unsigned short cur;     /* fzvar                         */
                                     /* = if_vars[atribut].f_fzvar+cur*/
           };

static int then_1_cur;

static struct jfid3_stak_rec jfid3_id3_stak[JFRD_VMAX];
/* describes the current branch in the id3-reduction-tree.             */

static int    jfid3_ff_id3_stak;

static float jfid3_id3_count[128];  /* used in selecting the branch-    */
                                    /* variable.                        */
static float jfid3_id3_M;

/************************************************************************/
/* variables to handle extern-statement:                                */

#define JFRD_WMAX 100
static const char *jfid3_words[JFRD_WMAX];

static struct jfg_sprog_desc     jfid3_pdesc;
static struct jfg_statement_desc jfid3_sdesc;

static signed long jfid3_ipcount; /* number of data-sets in file .          */


/***********************************************************************/
/* Variables used in inserting a rule into the program:                */

static unsigned char *jfid3_program_id;

static int jfid3_ins_rules;         /* number of rules inserted in program. */

#define JFRD_TREE_SIZE 100
static struct jfg_tree_desc jfid3_tree[JFRD_TREE_SIZE];

#define JFRD_MAX_TEXT 512
static char jfid3_text[JFRD_MAX_TEXT];

/**********************************************************************/
/* Variables to describe the rule-base:                               */

#define JFRD_CMP_EQ                 0
#define JFRD_CMP_NEQ                1
#define JFRD_CMP_INDEPENDENT        5
#define JFRD_CMP_CONTRADICTION      6


static unsigned char *jfid3_darea = NULL;   /* the rule-base.                */
static long jfid3_data_size = 10000; /* number of bytes allocated to         */
                                     /* rule-base (value from jfid3_run).    */
static long          jfid3_ff_darea = 0;    /* first-free s-rule.            */
static long          jfid3_drec_size;       /* size af en s_rule.            */

  /* En s_rule har foelgende indhold:  score (float).               */
  /*                                   data_set_no (long).          */
  /*                                   conflict (byte).             */
  /*                                   then_adj (byte).             */
  /*                                   var-1 fzvar-cur,             */
  /*                                   var-2 fzvar-cur,             */
  /*                                     .                          */

static int           jfid3_c_contradictions;


/************************************************************************/
/* Variables to history-file:                                           */

FILE *jfid3_hfile = NULL;
int jfid3_h_dset = 0;    /* 1: write info for all data-sets.            */
int jfid3_h_rules = 0;   /* 1: write rules,                             */
                         /* 2: write dataset-no, rules, scores.         */

/************************************************************************/
/* Diverse variable:                                                    */

static float jfid3_min_score   = -1.0;    /* rm rules with scores below. */

static char jfid3_t_jfrd[]     = "jfrd";
static char jfid3_t_input[]    = "input";
static char jfid3_t_output[]   = "output";
static char jfid3_empty[] = " ";

static char jfid3_da_fname[256];
static FILE *jfid3_sout;

#define JFE_WARNING 0
#define JFE_ERROR   1
#define JFE_FATAL   2

struct jfr_err_desc { int eno;
                      const char *text;
      };

static struct jfr_err_desc jfr_err_texts[] =
   {{  0, " "},
    {  1, "Cannot open file:"},
    {  2, "Error reading from file:"},
    {  3, "Error writing to file:"},
    {  4, "Not an jfr-file:"},
    {  5, "Wrong version:"},
    {  6, "Cannot allocate memory to:"},
    {  9, "Illegal number:"},
    { 10, "Value out of domain-range:"},
    { 11, "Unexpected EOF."},
    { 13, "Undefined adjectiv:"},
    {401, "jfp cannot insert this type of statement"},
    {402, "Statement to large."},
    {403, "Not enogh free memory to insert statement"},

    {504, "Syntax error in jfrd-statement:"},
    {505, "Too many variables in statement (max 50)."},
    {506, "Undefined variable:"},
    {507, "Illegal number of adjectives to variable ([1..127]:"},
    {508, "Out of memory in rule-array"},
    {514, "Tree not large enogh to hold statement"},
    {517, "Cannot create rules from data without a data-file"},
    {519, "Too many words in statement (max 255)."},
    {520, "No 'call jfrd'-statement in program"},
    {9999, "Unknown error!"},
   };

static int jf_error(int errno, const char *name, int mode);
static int jfid3_fl_ip_get(struct jft_data_record *dd);
static int jfid3_ip_get();
static int jfid3_var_no(const char *text);
static int jfid3_get_command(int argc);
static unsigned char *jfid3_ins_s_rule(unsigned char *progid);
static int jfid3_oom(void);
static void jfid3_s_get(int dno);
static void jfid3_s_put(int dno);
static void jfid3_s_copy(int dno, int sno);
static void jfid3_s_update(int dno);
static int jfid3_ch_cmp(unsigned char ch1, unsigned char ch2);
static int jfid3_s_cmp(int dno1, int dno2);
static int jfid3_rcheck(unsigned long *cruleno, unsigned long adr);
static void jfid3_resolve_contradiction(unsigned long rno1, unsigned long rno2);
static int jfid3_closest_adjectiv(int ifvar_no);
static float jfid3_id3_i(float ant);
static int jfid3_in_leaf(void);
static int jfid3_empty_leaf(void);
static int jfid3_in_stack(int id);
static int jfid3_id3_chose(void);
static void jfid3_call(void);
static void jfid3_no_call(void);
static void jfid3_ip2rule(unsigned long rno);
static void jfid3_red_contra(void);
static void jfid3_rm_rules(void);
static int jfid3_tilbage(void);
static int jfid3_red_id3(void);
static int jfid3_data(void);
static void jfid3_set_domsize(void);
static void jfid3_hist_rules(void);



static int jf_error(int eno, const char *name, int mode)
{
  int m, v, e;

  e = 0;
  for (v = 0; e == 0; v++)
  { if (jfr_err_texts[v].eno == eno
        || jfr_err_texts[v].eno == 9999)
      e = v;
  }
  if (mode == JFE_WARNING)
  { fprintf(jfid3_sout, "WARNING %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    m = 0;
  }
  else
  { if (eno != 0)
      fprintf(jfid3_sout, "*** error %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    if (mode == JFE_FATAL)
    { if (eno != 0)
        fprintf(jfid3_sout, "\n*** PROGRAM ABORTED! ***\n");
    }
    m = -1;
  }
  return m;
}

/************************************************************************/
/* Funktioner til indlaesning fra fil                                   */
/************************************************************************/

static int jfid3_fl_ip_get(struct jft_data_record *dd)
{
  int m;
  char txt[256];

  m = jft_getdata(dd);
  if (m != 0)
  { if (m != 11) /* eof */
    { sprintf(txt, " %s in file: %s line %d.",
              jft_error_desc.carg, jfid3_da_fname, jft_error_desc.line_no);
      jf_error(jft_error_desc.error_no, txt, JFE_ERROR);
      m = 0;
    }
  }
  return m;
}

static int jfid3_ip_get()
{
  int slut, m;
  struct jft_data_record dd;

  slut = 0;
  for (m = 0; slut == 0 && m < jft_dset_desc.record_size; m++)
  { slut = jfid3_fl_ip_get(&dd);
    if (slut == 1 && m != 0)
       return jf_error(11, jfid3_empty, JFE_ERROR);
    if (slut == 0)
    { if (dd.vtype == JFT_VT_EXPECTED)
        jfid3_expected[dd.vno] = dd.farg;
      else
      if (dd.vtype == JFT_VT_INPUT)
      { jfid3_ivalues[dd.vno] = dd.farg;
        if (dd.mode == JFT_DM_MISSING)
          jfid3_confidences[dd.vno] = 0.0;
        else
          jfid3_confidences[dd.vno] = 1.0;
      }
    }
  }
  return slut;
}

/**********************************************************************/
/* functions to decode an 'extern jfrd'-statement                     */
/**********************************************************************/

static int jfid3_var_no(const char *text)
{
  int m, res;
  struct jfg_var_desc vdesc;

  res = -1;
  for (m = 0; res == -1 && m < jfid3_pdesc.var_c; m++)
  { jfg_var(&vdesc, jfid3_head, m);
    if (strcmp(vdesc.name, text) == 0)
      res = m;
  }
  return res;
}

static int jfid3_get_command(int argc)
{
  int m, vno;
  struct jfg_var_desc vdesc;

  int state;   /*  0: start                       */
               /*  1: jfrd                        */
               /*  2: input                       */
               /*  3: output                      */
               /*  4: slut                        */

  jfid3_ff_if_vars = 0;
  m = -1;
  state = 0;

  while (state != -1)
  { m++;
    if (m >= argc)
    { if (state != 4)
        return jf_error(504, jfid3_empty, JFE_ERROR);
    }
    switch (state)
    { case 0:
        if (strcmp(jfid3_words[m], jfid3_t_jfrd) == 0)
          state = 1;
        else
          return -1; /*jf_error(502, jfid3_words[m], JFE_ERROR); */
        break;
      case 1:  /* jfrd */
        if (strcmp(jfid3_words[m], jfid3_t_input) == 0)
          state = 2;
        else
          return jf_error(504, jfid3_words[m], JFE_ERROR);
        break;
      case 2:  /* input */
        if (strcmp(jfid3_words[m], jfid3_t_output) == 0)
          state = 3;
        else
        if (jfid3_words[m][0] == '-')
          m++;
        else
        { vno = jfid3_var_no(jfid3_words[m]);
          if (vno == -1)
            return jf_error(506, jfid3_words[m], JFE_ERROR);
          if (jfid3_ff_if_vars >= JFRD_VMAX)
            return jf_error(505, jfid3_words[m], JFE_ERROR);

          jfid3_if_vars[jfid3_ff_if_vars].var_no = vno;
          jfg_var(&vdesc, jfid3_head, vno);
          jfid3_if_vars[jfid3_ff_if_vars].adjectiv_c = vdesc.fzvar_c;
          jfid3_if_vars[jfid3_ff_if_vars].f_fzvar_no = vdesc.f_fzvar_no;
          if (vdesc.fzvar_c == 0 || vdesc.fzvar_c > 254)
            return jf_error(507, jfid3_words[m], JFE_ERROR);
          jfid3_ff_if_vars++;
        }
        break;
      case 3:
        if (jfid3_words[m][0] == '-')
          m++;
        else
        { vno = jfid3_var_no(jfid3_words[m]);
          if (vno == -1)
            return jf_error(506, jfid3_words[m], JFE_ERROR);
          jfid3_then_var.var_no = vno;
          jfg_var(&vdesc, jfid3_head, vno);
          jfid3_then_var.adjectiv_c = vdesc.fzvar_c;
          jfid3_then_var.f_fzvar_no = vdesc.f_fzvar_no;
          if (vdesc.fzvar_c == 0 || vdesc.fzvar_c > 254)
            return jf_error(507, jfid3_words[m], JFE_ERROR);
          state = 4;
        }
        break;
      case 4:
        state = -1; /* slut */
        if (m < argc)
          return jf_error(504, jfid3_words[m], JFE_ERROR);
        break;
    }
  }
  if (jfid3_ff_if_vars == 0)
  { for (jfid3_ff_if_vars = 0; jfid3_ff_if_vars < jfid3_pdesc.ivar_c;
         jfid3_ff_if_vars++)
    {  vno = jfid3_pdesc.f_ivar_no + jfid3_ff_if_vars;
       if (jfid3_ff_if_vars >= JFRD_VMAX)
         return jf_error(505, jfid3_words[m], JFE_ERROR);

       jfid3_if_vars[jfid3_ff_if_vars].var_no = vno;
       jfg_var(&vdesc, jfid3_head, vno);
       jfid3_if_vars[jfid3_ff_if_vars].adjectiv_c = vdesc.fzvar_c;
       jfid3_if_vars[jfid3_ff_if_vars].f_fzvar_no = vdesc.f_fzvar_no;
       if (vdesc.fzvar_c == 0 || vdesc.fzvar_c > 254)
         return jf_error(507, jfid3_words[m], JFE_ERROR);
    }
  }
  return 0;
}

/***********************************************************************/
/* Insert tree-statement                                               */
/***********************************************************************/


static unsigned char *jfid3_ins_s_rule(unsigned char *progid)
/* insert current simple rule in program. Used to insert statements to */
/* find then-value.                                                    */
{
  int res, ff_tree, v, first;
  unsigned char *pc;
  const char *dummy[2] = { NULL, NULL };

  pc = progid;
  ff_tree = 0;
  first = 1;
  for (v = 0; v < jfid3_ff_if_vars; v++)
  { if (jfid3_if_vars[v].cur != 255)     /* always in v 2.00 of jfid3! */
    { if (ff_tree + 2 >= JFRD_TREE_SIZE)
      { jf_error(514, jfid3_empty, JFE_ERROR);
        return pc;
      }
      jfid3_tree[ff_tree].type = JFG_TT_FZVAR;
      jfid3_tree[ff_tree].sarg_1 = jfid3_if_vars[v].cur
                      + jfid3_if_vars[v].f_fzvar_no;
      ff_tree++;
      if (first == 0)
      { jfid3_tree[ff_tree].type = JFG_TT_OP;
        jfid3_tree[ff_tree].op = JFS_ONO_AND;
        jfid3_tree[ff_tree].sarg_1 = ff_tree - 2;
        jfid3_tree[ff_tree].sarg_2 = ff_tree - 1;
        ff_tree++;
      }
      first = 0;
    }
  }
  if (first == 1)                           /* No if-arguments! (never) */
  { jfid3_tree[ff_tree].type = JFG_TT_TRUE;
    ff_tree++;
  }
  jfid3_sdesc.type = JFG_ST_IF;
  jfid3_sdesc.sec_type = JFG_SST_FZVAR;
  jfid3_sdesc.flags = 0;
  jfid3_sdesc.sarg_1 = jfid3_then_var.f_fzvar_no + jfid3_then_var.cur;

  res = jfp_i_tree(jfid3_head, &pc, &jfid3_sdesc,
                   jfid3_tree, ff_tree - 1, 0, 0,
                   dummy, 0);
  if (res != 0)
    jf_error(res, jfid3_empty, JFE_ERROR);
  return pc;
}


/*************************************************************************/
/* Functions to handl rule-base:                                         */

static int jfid3_oom(void)
{
  /* return: 1: if out-of-memory (not enogh memory to insert rule),      */
  /*        0: enogh memory.                                             */
  int res;

  if ((jfid3_ff_darea + 5) * jfid3_drec_size > jfid3_data_size)
    res = 1;
  else
    res = 0;
  return res;
}

static void jfid3_s_get(int dno) /* copies rule-base rule no <dno>  to  */
                                 /* current rule.                       */
{
  unsigned long adr;
  int m;

  adr = ((unsigned long) dno) * jfid3_drec_size;

  memcpy((char *) &jfid3_dscore,
          (char *) &(jfid3_darea[adr]), sizeof(float));
  adr += sizeof(float);


  memcpy((char *) &jfid3_data_set_no,
         (char *) &(jfid3_darea[adr]), sizeof(long));
  adr += sizeof(long);

  jfid3_conflict = jfid3_darea[adr];
  adr++;

  jfid3_then_var.cur = jfid3_darea[adr];
  adr++;
  for (m = 0; m < jfid3_ff_if_vars; m++)
  { jfid3_if_vars[m].cur = jfid3_darea[adr];
    adr++;
  }
}

static void jfid3_s_put(int dno)   /* copies current rule to rule-base */
                                   /* rule number <dno>.               */
{
  unsigned long adr;
  int m;

  adr = ((unsigned long) dno) * jfid3_drec_size;

  memcpy((char *) &(jfid3_darea[adr]),
         (char *) &jfid3_dscore, sizeof(float));
  adr += sizeof(float);


  memcpy((char *) &(jfid3_darea[adr]),
         (char *) &jfid3_data_set_no, sizeof(long));
  adr += sizeof(long);

  jfid3_darea[adr] = jfid3_conflict;
  adr++;
  jfid3_darea[adr] = jfid3_then_var.cur;
  adr++;
  for (m = 0; m < jfid3_ff_if_vars; m++)
  { jfid3_darea[adr] = jfid3_if_vars[m].cur;
    adr++;
  }
}

static void jfid3_s_copy(int dno, int sno)
{
  /* copy rule-base rule <sno> to rule-base rule <dno>.        */
  jfid3_s_get(sno);
  jfid3_s_put(dno);
}

static void jfid3_s_update(int dno)
{
  /* updates rule-base no <dno> from current rule. It is assumed the */
  /* rules are identical (except score, data-set-no).                */

  unsigned long adr;
  unsigned long rm_adr;
  float rb_score, new_score;
  unsigned long rb_data_set_no, new_data_set_no;

  adr = dno * jfid3_drec_size;
  rm_adr = adr;
  memcpy((char *) &rb_score,
         (char *) &(jfid3_darea[adr]), sizeof(float));
  adr += sizeof(float);
  memcpy((char *) &rb_data_set_no,
         (char *) &(jfid3_darea[adr]), sizeof(long));

  if (jfid3_res_confl == JFID3_RC_SCORE)
  { if (jfid3_dscore > rb_score)
    { new_score = jfid3_dscore;
      new_data_set_no = jfid3_data_set_no;
      if (jfid3_h_dset == 1)
        fprintf(jfid3_hfile, "  %4d  %8.4f  replace(%d)\n",
                             jfid3_ipcount, jfid3_dscore, rb_data_set_no);
    }
    else
    { new_score = rb_score;
      new_data_set_no = rb_data_set_no;
      if (jfid3_h_dset == 1)
        fprintf(jfid3_hfile, "  %4d  %8.4f  ignore_eq(%d)\n",
                             jfid3_ipcount, jfid3_dscore, rb_data_set_no);
    }
  }
  else
  { new_score = rb_score + jfid3_dscore;
    new_data_set_no = rb_data_set_no;
    if (jfid3_h_dset == 1)
      fprintf(jfid3_hfile, "  %4d  %8.4f  update(%d)\n",
                           jfid3_ipcount, jfid3_dscore, rb_data_set_no);
  }

  adr = rm_adr;
  memcpy((char *) &(jfid3_darea[adr]),
         (char *) &new_score, sizeof(float));
  adr += sizeof(float);
  memcpy((char *) &(jfid3_darea[adr]),
         (char *) &new_data_set_no, sizeof(long));
}


static int jfid3_ch_cmp(unsigned char ch1, unsigned char ch2)
{
  int res;

  res = JFRD_CMP_NEQ;
  if (ch1 == ch2)
    res = JFRD_CMP_EQ;
  return res;
}

static int jfid3_s_cmp(int dno1, int dno2)
{
  /* compares the rules in rule-base <dno1> and <dno2>    */

  unsigned long adr1, adr2;
  int m, res, ccmp;
  unsigned char then1, then2;

  adr1 = (((unsigned long) dno1) * jfid3_drec_size) + sizeof(float);
  adr1 += (unsigned long) (sizeof(long) + 1);
  adr2 = (((unsigned long) dno2) * jfid3_drec_size) + sizeof(float);
  adr2 += (unsigned long) (sizeof(long) + 1);
  then1 = jfid3_darea[adr1];
  then2 = jfid3_darea[adr2];
  adr1++;
  adr2++;
  res = JFRD_CMP_EQ;
  for (m = 0; res != JFRD_CMP_NEQ && m < jfid3_ff_if_vars; m++)
  { ccmp = jfid3_ch_cmp(jfid3_darea[adr1], jfid3_darea[adr2]);
    if (ccmp == JFRD_CMP_NEQ)
      res = JFRD_CMP_NEQ;
    adr1++;
    adr2++;
  }

  if (then1 == then2)
  { if (res == JFRD_CMP_NEQ)
      res = JFRD_CMP_INDEPENDENT;
  }
  else
  { if (res == JFRD_CMP_NEQ)
      res = JFRD_CMP_INDEPENDENT;
    else
      res = JFRD_CMP_CONTRADICTION;
  }
  return res;
}


static int jfid3_rcheck(unsigned long *cruleno, unsigned long adr)
{
  /* compares rule number <adr> with all other rules in the rule-base. */
  /* Returns: independent, eq or contradiction. If eq or contradition  */
  /*          then <cruleno> returns the rule-number it is equal or    */
  /*          contradict.                                              */

  int res, r;
  unsigned long m;

  res = JFRD_CMP_INDEPENDENT;
  for (m = 0; m < jfid3_ff_darea; m++)
  { if (m != adr)
    { r = jfid3_s_cmp(adr, m);
      if (r == JFRD_CMP_EQ)
      { res = r;
        *cruleno = m;
      }
      else
      if (r == JFRD_CMP_CONTRADICTION && res != JFRD_CMP_EQ)
      { res = r;
        *cruleno = m;
      }
    }
  }
  return res;
}

static void jfid3_resolve_contradiction(unsigned long rno1, unsigned long rno2)
{
  /* resolve the contradiction between the rule-base rules <rno1> and */
  /* <rno2>, where <rno2> is asumed to be the last rule in the base?  */
  /* A rule is deleted if RC_SCORE.                                   */

  unsigned long adr1, adr2;
  float score_1, score_2;
  unsigned long dset_no_1, dset_no_2;

  adr1 = ((unsigned long) rno1) * jfid3_drec_size;
  memcpy((char *) &score_1,
         (char *) &(jfid3_darea[adr1]), sizeof(float));
  adr1 += sizeof(float);
  memcpy((char *) &dset_no_1,
         (char *) &(jfid3_darea[adr1]), sizeof(long));
  adr1 += sizeof(long);

  adr2 = ((unsigned long) rno2) * jfid3_drec_size;
  memcpy((char *) &score_2,
         (char *) &(jfid3_darea[adr2]), sizeof(float));
  adr2 += sizeof(float);
  memcpy((char *) &dset_no_2,
         (char *) &(jfid3_darea[adr1]), sizeof(long));
  adr2 += sizeof(long);

  if (jfid3_res_confl == JFID3_RC_SCORE)
  { if (score_1 >= score_2)
    { /* remove rule-2 : */
      if (jfid3_h_dset == 1)
        fprintf(jfid3_hfile, "  %4d  %8.4f  ignore_conflict(%d)\n",
                              jfid3_ipcount, score_2, dset_no_1);
      jfid3_s_copy(rno2, jfid3_ff_darea - 1);
      jfid3_ff_darea--;
    }
    else
    { /* remove rule-1 (by replacing in with rule-2): */
      if (jfid3_h_dset == 1)
        fprintf(jfid3_hfile, "  %4d  %8.4f  replace_conflict(%d)\n",
                             jfid3_ipcount, score_2, dset_no_1);
      jfid3_s_copy(rno1, rno2);
      jfid3_ff_darea--;
    }
  }
  else
  { jfid3_darea[adr1] = 1;  /* set conflict to true */
    jfid3_darea[adr2] = 1;
    if (jfid3_h_dset == 1)
      fprintf(jfid3_hfile, "  %4d  %8.4f  insert_conflict(%d)\n",
                           jfid3_ipcount, score_2, dset_no_1);
  }
  jfid3_c_contradictions++;
}

static int jfid3_closest_adjectiv(int ifvar_no)
{
  int m, best_cur;
  float best_value, value;
  struct jfg_var_desc vdesc;

  best_value = 0.0;
  jfg_var(&vdesc, jfid3_head, jfid3_if_vars[ifvar_no].var_no);
  for (m = 0; m < jfid3_if_vars[ifvar_no].adjectiv_c; m++)
  { value = jfr_fzvget(jfid3_if_vars[ifvar_no].f_fzvar_no + m);
    if (m == 0 || value > best_value)
    { best_value = value;
      best_cur = m;
    }
  }
  jfid3_dscore *= best_value;
  return best_cur;
}

/**************************************************************************/
/* Functions used in id3-reduction:                                       */

static float jfid3_id3_i(float ant)
{
  float res, p;
  int m;

  res = 0.0;
  if (ant != 0.0)
  { for (m = 0; m < jfid3_then_var.adjectiv_c; m++)
    { p = jfid3_id3_count[m] / ant;
      if (p != 0.0)
        res += p * jfid3_id3_M * log(p);
    }
    res = -res;
  }
  return res;
}

static int jfid3_in_leaf(void)
                   /* return 1 if aktuel rule is in the leave described */
                   /* by id3_stak.                                      */
{
  int m, id;
  int res;

  res = 1;
  for (m = 0; res == 1 && m < jfid3_ff_id3_stak; m++)
  { id = jfid3_id3_stak[m].atribut;
    if (jfid3_if_vars[id].cur != jfid3_id3_stak[m].cur)
      res = 0;
  }
  return res;
}

static int jfid3_empty_leaf(void)  /* return 1 if no rule in actual leaf */
{
  int res, m;

  res = 1;
  for (m = 0; res == 1 && m < jfid3_ff_darea; m++)
  { jfid3_s_get(m);
    if (jfid3_in_leaf() == 1)
      res = 0;
  }
  return res;
}

static int jfid3_in_stack(int id)
{
  int res, m;

  res = 0;
  for (m = 0; res == 0 && m < jfid3_ff_id3_stak; m++)
  { if (jfid3_id3_stak[m].atribut == id)
      res = 1;
  }
  return res;
}

static int jfid3_id3_chose(void)
{
  int a, m, v, best;
  float it, antal, best_score, gain, ant2;
  float id3_score[JFRD_VMAX];

  then_1_cur = -2;
  antal = 0.0;
  for (a = 0; a < jfid3_then_var.adjectiv_c; a++)
    jfid3_id3_count[a] = 0.0;
  for (a = 0; a < jfid3_ff_darea; a++)
  { jfid3_s_get(a);
    if (jfid3_in_leaf() == 1)
    { jfid3_id3_count[jfid3_then_var.cur] += 1.0;
      antal += 1.0;
      if (then_1_cur == -2)
        then_1_cur = jfid3_then_var.cur;
      else
      if (then_1_cur != jfid3_then_var.cur)
        then_1_cur = -1;
    }
  }
  it = jfid3_id3_i(antal);

  for (v = 0; v < jfid3_ff_if_vars; v++)
  { if (jfid3_in_stack(v) == 1)
      gain = -1.0;
    else
    { gain = it;
      for (m = 0; m < jfid3_if_vars[v].adjectiv_c; m++)
      { for (a = 0; a < jfid3_then_var.adjectiv_c; a++)
          jfid3_id3_count[a] = 0.0;
        ant2 = 0.0;
        for (a = 0; a < jfid3_ff_darea; a++)
        { jfid3_s_get(a);
          if (jfid3_in_leaf() == 1)
          { if (jfid3_if_vars[v].cur == m)
            { jfid3_id3_count[jfid3_then_var.cur] += 1.0;
              ant2 += 1.0;
            }
          }
        }
        if (antal != 0.0)
          gain -= ant2 / antal * jfid3_id3_i(ant2);
        else
          gain = 0.0; /* burde aldrig ske */
      }
    }
    id3_score[v] = gain;
  }
  best_score = -1.0;
  for (v = 0; v < jfid3_ff_if_vars; v++)
  { if (id3_score[v] > best_score)
    { best = v;
      best_score = id3_score[v];
    }
  }
  return best;
}


static void jfid3_call(void)
{
  int m;

  jfid3_dscore = 1.0;
  for (m = 0; m < jfid3_ff_if_vars; m++)
  { jfid3_if_vars[m].cur = jfid3_closest_adjectiv(m);
  }
}

static void jfid3_no_call(void)
{
  return ;
}

static void jfid3_ip2rule(unsigned long rno)
{
  float d, dist, best_dist;
  int m, v, best_no;

  best_dist = 0.0;
  jfr_arun(jfid3_ovalues, jfid3_head, jfid3_ivalues, jfid3_confidences,
           jfid3_call, NULL, NULL);

  for (m = 0; m < jfid3_then_var.adjectiv_c; m++)
  { jfid3_then_var.cur = m;
    jfid3_ins_s_rule(jfid3_program_id);
    jfr_arun(jfid3_ovalues, jfid3_head, jfid3_ivalues, jfid3_confidences,
             jfid3_no_call, NULL, NULL);
    dist = 0.0;
    for (v = 0; v < jfid3_pdesc.ovar_c; v++)
    {  d = fabs(jfid3_ovalues[v] - jfid3_expected[v]) / jfid3_domsize[v];
       if (d > 1.0)
         d = 1.0;
       d = 1.0 - d;     /* smallest dist is best. */
       dist += d;
    }
    dist /= jfid3_pdesc.ovar_c;
    if (m == 0 || dist > best_dist)
    { best_no = m;
      best_dist = dist;
    }
    jfp_d_statement(jfid3_head, jfid3_program_id);
  }
  jfid3_then_var.cur = best_no;

  jfid3_dscore *= best_dist;
  jfid3_conflict = 0;
  jfid3_data_set_no = jfid3_ipcount;

  if (jfid3_res_confl == JFID3_RC_COUNT)
    jfid3_dscore = 1.0;
  jfid3_s_put(rno);
}


static void jfid3_red_contra(void)
{
  unsigned long n1, n2, n1_data_set_no;
  float n1_score, cc;
  int rm_rules;
  int ens;

  rm_rules = 0;
  cc = 0.0;
  if (jfid3_res_confl == JFID3_RC_COUNT)
  { jfid3_c_contradictions = 0;
    for (n1 = 0; n1 < jfid3_ff_darea; n1++)
    { jfid3_s_get(n1);
      n1_score = jfid3_dscore;
      n1_data_set_no = jfid3_data_set_no;
      if (jfid3_conflict == 1)
      { for (n2 = n1 + 1; n2 < jfid3_ff_darea;  )
        { jfid3_s_get(n2);
          if (jfid3_conflict == 1)
          { ens = jfid3_s_cmp(n1, n2);
            if (ens == JFRD_CMP_CONTRADICTION)
            { if (jfid3_dscore > n1_score)
              { if (jfid3_h_dset == 1)
                  fprintf(jfid3_hfile, "  remove(%d) contradict(%d)\n",
                                        n1_data_set_no, jfid3_data_set_no);
                cc += n1_score;
                n1_score = jfid3_dscore;
                n1_data_set_no = jfid3_data_set_no;
                jfid3_s_copy(n1, n2);
              }
              else
              { if (jfid3_h_dset == 1)
                  fprintf(jfid3_hfile, "  remove(%d) contradict(%d)\n",
                                        jfid3_data_set_no, n1_data_set_no);
                cc += jfid3_dscore;
              }
              jfid3_s_copy(n2, jfid3_ff_darea - 1);
              jfid3_ff_darea--;
              rm_rules++;
            }
            else
              n2++;
          }
          else
            n2++;
        }
      }
    }
    fprintf(jfid3_sout, "\n    Contradictions resolved. %d rules removed.\n",
                         rm_rules);
    jfid3_c_contradictions = (int) cc;  /* ?? */
  }
}

static void jfid3_rm_rules(void)
{
  float ascore;
  unsigned long n1;
  int rm_rules, rm_rule_count;

  ascore = 0.0;
  rm_rule_count = jfid3_ff_darea;
  rm_rules = 0;
  for (n1 = 0; n1 < jfid3_ff_darea; )
  { jfid3_s_get(n1);
    ascore += jfid3_dscore;
    if (jfid3_dscore < jfid3_min_score)
    { if (jfid3_h_dset == 1)
        fprintf(jfid3_hfile, "  remove(%d) reason_score(%8.4f)\n",
                             jfid3_data_set_no, jfid3_dscore);
      jfid3_s_copy(n1, jfid3_ff_darea - 1);
      jfid3_ff_darea--;
      rm_rules++;
    }
    else
      n1++;
  }
  fprintf(jfid3_sout, "\n    Avg rule-score: %8.4f.\n",
                      ascore / ((float) rm_rule_count));
  fprintf(jfid3_sout, "\n    Score-reduction. %d rules removed.\n",
                      rm_rules);
}

static void jfid3_hist_rules(void)
{
  int n1, v;
  float ascore;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;

  ascore = 0.0;
  for (n1 = 0; n1 < jfid3_ff_darea; n1++)
  { jfid3_s_get(n1);
    ascore += jfid3_dscore;
    if (jfid3_h_rules > 0)
    { for (v = 0; v < jfid3_ff_if_vars; v++)
      { jfg_var(&vdesc, jfid3_head, jfid3_if_vars[v].var_no);
        jfg_adjectiv(&adesc, jfid3_head,
                     vdesc.f_adjectiv_no + jfid3_if_vars[v].cur);
        fprintf(jfid3_hfile, "%s ", adesc.name);
      }
      jfg_var(&vdesc, jfid3_head, jfid3_then_var.var_no);
      jfg_adjectiv(&adesc, jfid3_head, vdesc.f_adjectiv_no + jfid3_then_var.cur);
      fprintf(jfid3_hfile, "%s ", adesc.name);
      if (jfid3_h_rules == 2)
        fprintf(jfid3_hfile, "%4d %8.4f", jfid3_data_set_no, jfid3_dscore);
      fprintf(jfid3_hfile, "\n");
    }
  }
  if (jfid3_ff_darea > 0)
    fprintf(jfid3_sout, "\n    Avg rule-score: %8.4f\n",
                    ascore / ((float) jfid3_ff_darea));
}

static int jfid3_tilbage(void)
{
  int slut, rettet, top, id, res;
  const char *dummy[2] = { NULL, NULL };

  slut = 0;
  rettet = 0;
  while (slut == 0 && rettet == 0)
  { top = jfid3_ff_id3_stak - 1;
    id = jfid3_id3_stak[top].atribut;
    if (jfid3_id3_stak[top].cur >= jfid3_if_vars[id].adjectiv_c)
    { jfid3_ff_id3_stak--;
      if (jfid3_ff_id3_stak == 0)
        slut = 1;
      jfid3_sdesc.type = JFG_ST_STEND;
      jfid3_sdesc.sec_type = 0;
      res = jfp_i_tree(jfid3_head, &jfid3_program_id, &jfid3_sdesc,
                       jfid3_tree, 0, 0, 0, dummy, 0);
      if (res != 0)
        return jf_error(res, jfid3_empty, JFE_FATAL);
    }
    else
    { jfid3_id3_stak[top].cur++;
      if (jfid3_empty_leaf() == 0)
      { jfid3_sdesc.type = JFG_ST_CASE;
        jfid3_sdesc.flags = 1;
        jfid3_tree[0].type = JFG_TT_FZVAR;
        jfid3_tree[0].sarg_1 = jfid3_if_vars[id].f_fzvar_no
                               + jfid3_id3_stak[top].cur;
        res = jfp_i_tree(jfid3_head, &jfid3_program_id, &jfid3_sdesc,
                         jfid3_tree, 0, 0, 0,
                         dummy, 0);
        if (res != 0)
          return jf_error(res, jfid3_empty, JFE_FATAL);
        rettet = 1;
      }
    }
  }
  return slut;
}

static int jfid3_red_id3(void)
{
  int slut, res, id;
  int node_c, leaf_c;
  const char *dummy[2] = { NULL, NULL };

  node_c = leaf_c = 0;
  jfid3_ff_id3_stak = 0;
  slut = 0;
  while (slut == 0)
  { id = jfid3_id3_chose();
    if (then_1_cur >= 0)
    { jfid3_sdesc.type = JFG_ST_IF;
      jfid3_sdesc.sec_type = JFG_SST_FZVAR;
      jfid3_sdesc.flags = 0;
      jfid3_sdesc.sarg_1 = jfid3_then_var.f_fzvar_no + then_1_cur;
      jfid3_tree[0].type = JFG_TT_TRUE;
      res = jfp_i_tree(jfid3_head, &jfid3_program_id, &jfid3_sdesc,
                       jfid3_tree, 0, 0, 0,
                       dummy, 0);
      if (res != 0)
        return jf_error(res, jfid3_empty, JFE_FATAL);
      leaf_c++;
      slut = jfid3_tilbage();
    }
    else
    if (then_1_cur == -2)
      slut = jfid3_tilbage();
    else
    { jfid3_id3_stak[jfid3_ff_id3_stak].atribut = id;
      jfid3_id3_stak[jfid3_ff_id3_stak].cur = 0;
      jfid3_sdesc.type = JFG_ST_SWITCH;
      jfid3_sdesc.sarg_1 = jfid3_if_vars[id].var_no;
      jfid3_sdesc.flags = 1;
      res = jfp_i_tree(jfid3_head, &jfid3_program_id, &jfid3_sdesc,
                       jfid3_tree, 0, 0, 0,
                       dummy, 0);
      if (res != 0)
        return jf_error(res, jfid3_empty, JFE_FATAL);
      node_c++;
      jfid3_ff_id3_stak++;
      while (jfid3_empty_leaf() == 1)
        jfid3_id3_stak[jfid3_ff_id3_stak - 1].cur++;
      jfid3_sdesc.type = JFG_ST_CASE;
      jfid3_sdesc.flags = 1;
      jfid3_tree[0].type = JFG_TT_FZVAR;
      jfid3_tree[0].sarg_1 = jfid3_if_vars[id].f_fzvar_no
                            + jfid3_id3_stak[jfid3_ff_id3_stak - 1].cur;
      res = jfp_i_tree(jfid3_head, &jfid3_program_id, &jfid3_sdesc,
                       jfid3_tree, 0, 0, 0,
                       dummy, 0);
      if (res != 0)
        return jf_error(res, jfid3_empty, JFE_FATAL);
    }
  }
  fprintf(jfid3_sout,
            "\n    Tree-reduction finished. Inserted %d nodes, %d leaves.\n",
            node_c, leaf_c);
  return 0;
}


static int jfid3_data(void)
{
  int oomc;
  int rule_type;
  unsigned long cruleno;

  if ((jfid3_darea = (unsigned char *) malloc(jfid3_data_size)) == NULL)
    return 6;
  jfid3_ipcount = 0;
  jfid3_ff_darea = 0;
  jfid3_c_contradictions = 0;
  jfid3_ins_rules = 0;
  oomc = 0;

  jfid3_drec_size = sizeof(float) + sizeof(long) +
                    sizeof(unsigned char) * (jfid3_ff_if_vars + 1 + 1);
  if (jfid3_drec_size % 2 == 1)
    jfid3_drec_size++;

  if (jfid3_h_dset == 1)
    fprintf(jfid3_hfile, "dataset    score  action\n");
  while (jfid3_ip_get() == 0)
  { jfid3_ipcount++;
    if (jfid3_oom() == 0)
    { jfid3_ip2rule(jfid3_ff_darea);
      jfid3_ff_darea++;
      rule_type = jfid3_rcheck(&cruleno, jfid3_ff_darea - 1);
      if (rule_type == JFRD_CMP_CONTRADICTION)
      { jfid3_resolve_contradiction(cruleno, jfid3_ff_darea - 1);
      }
      else
      if (rule_type == JFRD_CMP_EQ)
      { jfid3_s_update(cruleno);
        jfid3_ff_darea--;
      }
      else
      { if (jfid3_h_dset == 1)
          fprintf(jfid3_hfile, "  %4d  %8.4f  insert\n",
                               jfid3_ipcount, jfid3_dscore);
      }
    }
    else
      oomc++;
  } /* while */
  fprintf(jfid3_sout, "\n    Data read from %s.\n", jfid3_da_fname);
  fprintf(jfid3_sout, "      %d rules created from %d data-sets.\n",
          (int) jfid3_ff_darea, (int) jfid3_ipcount);
  if (oomc != 0)
    fprintf(jfid3_sout,
               "\n    Not enough memory to all data-samples. %d rejected\n",
            oomc);
  jfp_d_statement(jfid3_head, jfid3_program_id);
  jfid3_red_contra();
  fprintf(jfid3_sout,
          "\n    %d contradictions\n", (int) jfid3_c_contradictions);
  if (jfid3_min_score > 0.0)
    jfid3_rm_rules();
  jfid3_hist_rules();
  jfid3_red_id3();

  jfid3_ff_darea = 0;
  return 0;
}

/* static int jfid3_no_call(void)
{
   fprintf(jfid3_sout, "Warning: Call-funktion not executed!\n");
   return 0;
}
*/

static void jfid3_set_domsize(void)
{
  int m;
  float dmin, dmax;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;

  for (m = 0; m < jfid3_pdesc.ovar_c; m++)
  { jfid3_domsize[m] = 0.0;
    jfg_var(&vdesc, jfid3_head, jfid3_pdesc.f_ovar_no + m);
    if (vdesc.fzvar_c > 1)
    { jfg_adjectiv(&adesc, jfid3_head, vdesc.f_adjectiv_no);
      dmin = adesc.center;
      jfg_adjectiv(&adesc, jfid3_head, vdesc.f_adjectiv_no + vdesc.fzvar_c - 1);
      dmax = adesc.center;
      jfid3_domsize[m] = dmax - dmin;
    }
    if (jfid3_domsize[m] <= 0)
      jfid3_domsize[m] = 1.0;
  }
}


int jfid3_run(char *op_fname, char *ip_fname, char *da_fname, char *field_sep,
              int f_mode, long prog_size, long data_size, int res_confl,
              char *hfile_name, int h_dsets, int h_rules, float min_score,
              char *sout_fname, int append, int batch)
{
  int m;
  unsigned char *pc;
  int slut;
  char txt[80];

  jfid3_sout = stdout;
  if (sout_fname != NULL && strlen(sout_fname) != 0)
  { if (append == 1)
      jfid3_sout = fopen(sout_fname, "a");
    else
      jfid3_sout = fopen(sout_fname, "w");
    if (jfid3_sout == NULL)
    { jfid3_sout = stdout;
      printf("Cannot open %s for writing.\n", sout_fname);
    }
  }
  fprintf(jfid3_sout, "\nJFID3 version 2.0\n");
  m = jfr_init(0);
  if (m != 0)
    return jf_error(m, jfid3_empty, JFE_FATAL);
  m = jfr_aload(&jfid3_head, ip_fname, prog_size);
  if (m != 0)
    return jf_error(m, ip_fname, JFE_FATAL);
  m = jfg_init(JFG_PM_NORMAL, 64, 4);
  if (m != 0)
    return jf_error(m, jfid3_empty, JFE_FATAL);
  m = jfp_init(0);
  if (m != 0)
    return jf_error(m, jfid3_empty, JFE_FATAL);

  strcpy(jfid3_da_fname, da_fname);
  jfid3_data_size = data_size;
  jfid3_res_confl = res_confl;
  jfid3_min_score = min_score;
  jfid3_h_rules = h_rules;
  jfid3_h_dset = h_dsets;
  if (jfid3_h_dset == 1 || jfid3_h_rules != 0)
  { jfid3_hfile = fopen(hfile_name, "w");
    if (jfid3_hfile == NULL)
      return jf_error(1, hfile_name, JFE_FATAL);
  }

  jft_init(jfid3_head);
  for (m = 0; m < strlen(field_sep); m++)
    jft_char_type(field_sep[m], JFT_T_SPACE);

  m = jft_fopen(da_fname, f_mode, 0);
  if (m != 0)
    return jf_error(m, jfid3_empty, JFE_FATAL);

  jfid3_id3_M = 1.0 / log(2.0);
  jfg_sprg(&jfid3_pdesc, jfid3_head);
  jfid3_set_domsize();

  fprintf(jfid3_sout, "\n  Rule discovery startet\n");
  pc = jfid3_pdesc.pc_start; slut = 0;
  jfg_statement(&jfid3_sdesc, jfid3_head, pc);
  while (slut == 0 && jfid3_sdesc.type != JFG_ST_EOP)
  { if (jfid3_sdesc.type == JFG_ST_IF
        && jfid3_sdesc.sec_type == JFG_SST_EXTERN)
    {
      jfg_t_statement(jfid3_text, JFRD_MAX_TEXT, 2,
                      jfid3_tree, JFRD_TREE_SIZE, jfid3_head, -1, pc);
      fprintf(jfid3_sout, "\n  Now handling statement (in %s):\n  %s\n",
                           ip_fname, jfid3_text);
      m = jfg_a_statement(jfid3_words, JFRD_WMAX, jfid3_head, pc);
      if (m < 0)
        return jf_error(519, jfid3_empty, JFE_FATAL);
      if (jfid3_get_command(m) == 0)
      { jfid3_program_id = pc;
        m = jfid3_data();
        if (m != 0)
          return jf_error(m, jfid3_empty, JFE_FATAL);
        slut = 1;
      }
      else
        pc = jfid3_sdesc.n_pc;
    }
    else
      pc = jfid3_sdesc.n_pc;
    jfg_statement(&jfid3_sdesc, jfid3_head, pc);
  }
  if (slut == 0)
    jf_error(520, ip_fname, JFE_FATAL);

  m = jfp_save(op_fname, jfid3_head);
  if (m != 0)
    return jf_error(m, op_fname, JFE_FATAL);

  jft_close();
  jfr_close(jfid3_head);
  jfg_free();
  jfp_free();
  jfr_free();
  fprintf(jfid3_sout, "\n  Rule discovery completed. Program written to: %s\n\n",
                      op_fname);
  if (jfid3_darea != NULL)
   free(jfid3_darea);
  if (batch == 0)
  { printf("Press RETURN ...");
    fgets(txt, 78, stdin);
  }
  return 0;
}

