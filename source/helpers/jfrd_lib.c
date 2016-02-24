  /**************************************************************************/
  /*                                                                        */
  /* jfrd_lib.c   Version  2.03  Copyright (c) 1998-2000 Jan E. Mortensen   */
  /*                                                                        */
  /* JFS to create a fuzzy system from data-sets using the Wang-Mendel      */
  /* method combined with an ad-hoc rule reduction method.                  */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/

#define JFE_WARNING 0
#define JFE_ERROR   1
#define JFE_FATAL   2

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfp_lib.h"
#include "jft_lib.h"
#include "jfrd_lib.h"

static void *jfrd_head = NULL;

#define JFRD_VMAX 50

static float jfrd_ivalues[JFRD_VMAX];
static float jfrd_ovalues[JFRD_VMAX];
static float jfrd_expected[JFRD_VMAX];
static float jfrd_confidences[JFRD_VMAX];

static char jfrd_da_fname[256];

static int jfrd_silent    = 0;
static int jfrd_batch     = 1;
static int jfrd_data_mode = 0;  /* 0: input, expected. 1: expected, input */

static long jfrd_data_size = 10000;
static long jfrd_prog_size = 20000;

static char jfrd_field_sep[256];   /* 0: use space, tab etc as field-seps, */
                                   /* other: use jfrd_field_sep's.         */

static int jfrd_res_confl = 0;
                          /* Conflict resolution-mode.             */
                          /* 0: score,                                    */
                          /* 1: count.                                    */

static int jfrd_max_time = 60;   /* stop rewind after                     */
                                 /* jfrd_max_tim minuttes                 */

static time_t jfrd_start_time;

static int jfrd_red_case = 0;    /* 1: use case-reduktion.                */

static int jfrd_use_weight = 0;  /* 1: 'ifw %<wgt>'-statements.           */
static float jfrd_weight   = 0.5;/* <wgt> in ifw %<wgt>'.                 */

static int jfrd_red_order = 0;   /* 0: id3-order,                        */
                                 /* 1: enter-order.                      */

static signed char    jfrd_red_mode = -1;          /* 0: no reduction,   */
                                                   /* 1: all,            */
                                                   /* 2: between,        */
                                                   /* 3: in,             */
                                                   /* 4: inbetween.      */

static int jfrd_def_fzvar = -1;  /* if default-reduction then default-fzvarno.*/

static signed long jfrd_ipcount;

struct jfrd_multi_desc { signed short var_no;
                         signed short cur;
                         signed short adjectiv_c;
                         unsigned short f_fzvar_no;
                         signed char    red_mode;  /* 0: ingen reduction,*/
                                                   /* 1: all,            */
                                                   /* 2: between,        */
                                                   /* 3: in.             */
                                                   /* 4: in-between.     */
                       };

static struct jfrd_multi_desc jfrd_if_vars[JFRD_VMAX];
static int jfrd_ff_if_vars = 0;

static struct jfrd_multi_desc jfrd_then_var;


#define JFRD_WMAX 100
static const char *jfrd_words[JFRD_WMAX];

static int jfrd_gains[JFRD_VMAX];

static struct jfg_sprog_desc     jfrd_pdesc;
static struct jfg_statement_desc jfrd_sdesc;

static char jfrd_empty[] = " ";

#define JFRD_TREE_SIZE 100

static struct jfg_tree_desc jfrd_tree[JFRD_TREE_SIZE];

#define JFRD_MAX_TEXT 512
static char jfrd_text[JFRD_MAX_TEXT];


#define JFRD_CMP_EQ                 0
#define JFRD_CMP_NEQ                1
#define JFRD_CMP_GT                 2
#define JFRD_CMP_LT                 3
#define JFRD_CMP_UNION              4
#define JFRD_CMP_INDEPENDENT        5
#define JFRD_CMP_DATA_CONTRADICTION 6
#define JFRD_CMP_RULE_CONTRADICTION 7


static unsigned char *jfrd_darea = NULL;
static long          jfrd_ff_darea = 0;    /* first-free s-rule.         */
static long          jfrd_drec_size;       /* size af en s_rule.         */
static float         jfrd_dscore;

  /* A s_rule contains:                score (float).               */
  /*                                   count (short) (=ip_no hvis   */
  /*                                   conflict_resolve=score).     */
  /*                                   conflict (byte).             */
  /*                                   then_adj (byte).             */
  /*                                   var-1 fzvar-cur (255=all)    */
  /*                                   var-2 fzvar-cur (255=all)    */
  /*                                     .                          */


static unsigned char *jfrd_c_rules;
static long          jfrd_ff_c_rules;
static long          jfrd_crule_size;
static long          jfrd_c_size;

  /* A c_rule contains:                score (float)            */
  /*                                   count (short)            */
  /*                                   conflict (byte)          */
  /*                                   then_adj (byte)          */
  /*                                   var-1 adj-1 (byte) 1=sat */
  /*                                   var-1 adj-2              */
  /*                                     .                      */
  /*                                   var-2 adj-1              */
  /*                                     .                      */

#define JFRD_MAX_CONTRADICTIONS 100
static unsigned long jfrd_contradictions[JFRD_MAX_CONTRADICTIONS];
static int           jfrd_ff_contradictions;
static int           jfrd_c_contradictions;

static unsigned char jfrd_case_expr[255];
static int           jfrd_case_id;       /* case_expr er for:           */
                                         /* jfrd_if_vars[jfrd_case_id]. */

static unsigned short jfrd_rule_count;
static char jfrd_conflict;

static unsigned char *jfrd_program_id;

static int jfrd_ins_rules;       /* number of rules inserted in program. */

/************************************************************************/
/* Variables to id3-reduction                                           */
/************************************************************************/

static float jfrd_id3_count[128];
static float jfrd_id3_M;

static FILE *sout;

static const char jfrd_t_jfrd[]     = "jfrd";
static const char jfrd_t_input[]    = "input";
static const char jfrd_t_output[]   = "output";

struct jfr_err_desc { int eno;
                      const char *text;
                    };

static struct jfr_err_desc jfr_err_texts[] =
    {  { 0, " "},
    {   1, "Cannot open file:"},
    {   2, "Error reading from file:"},
    {   3, "Error writing to file:"},
    {   4, "Not an jfr-file:"},
    {   5, "Wrong version:"},
    {   6, "Cannot allocate memory to:"},
    {   9, "Illegal number:"},
    {  10, "Value out of domain-range:"},
    {  11, "Unexpected EOF."},
    {  13, "Undefined adjectiv:"},
    { 401, "jfp cannot insert this type of statement"},
    { 402, "Statement to large."},
    { 403, "Not enogh free memory to insert statement"},

    { 501, "Illegal create-type:"},
    { 502, "Extern-statement ignored:"},
    { 504, "Syntax error in jfrd-statement:"},
    { 505, "Too many variables in statement (max 50)."},
    { 506, "Undefined variable:"},
    { 507, "Illegal number of adjectives to variable ([1..127]:"},
    { 508, "Out of memory in rule-array."},
    { 514, "Tree not large enogh to hold statement:"},
    { 517, "Cannot create rules from data without a data-file."},
    { 519, "Too many words in statement (max 255)."},
    { 520, "No 'extern jfrd'-statement in program."},
    { 521, "Not enogh free memory to in-reduction."},
    {9999, "Unknown error!"},
  };

static int jf_error(int eno, const char *name, int mode);
static int jfrd_fl_ip_get(struct jft_data_record *dd);
static int jfrd_ip_get(void);
static int jfrd_var_no(const char *text);
static int jfrd_get_command(int argc);
static unsigned char *jfrd_ins_s_rule(unsigned char *progid);
static unsigned char *jfrd_ins_c_rule(unsigned char *progid, long srno);
static int jfrd_oom(void);
static void jfrd_s_get(int dno);
static void jfrd_s_put(int dno);
static void jfrd_s_copy(int dno, int sno);
static void jfrd_s_update(int dno);
static void jfrd_c_update(int dno);
static void jfrd_c_copy(long dno, long sno);
static void jfrd_c_rm(long dno);
static void jfrd_sc_copy(long cno, long sno);
static int jfrd_ch_cmp(unsigned char ch1, unsigned char ch2);
static int jfrd_s_cmp(int dno1, int dno2);
static int jfrd_sc_cmp(int sno, int cno);
static int jfrd_c_test(unsigned long cno);
static int jfrd_rcheck(unsigned long *cruleno, unsigned long adr);
static void jfrd_resolve_contradiction(unsigned long rno1, unsigned long rno2);
static int jfrd_closest_adjectiv(int ifvar_no);
static float jfrd_id3_i(float ant);
static void jfrd_id3_chose(void);
static int jfrd_red_all(void);
static void jfrd_call(void);
static void jfrd_no_call(void);
static int jfrd_in_contradictions(void);
static void jfrd_ip2rule(unsigned long rno);
static void jfrd_conflict_add(unsigned long rno);
static int jfrd_rewind_posible(void);
static void jfrd_rewind(void);
static int jfrd_check_cdata(unsigned long rno);
static void jfrd_red_contra(void);
static unsigned long jfrd_s2c(unsigned long sno);
static int jfrd_red_in(void);
static void jfrd_red_def(void);
static int jfrd_case_cmp(unsigned long cno, unsigned short clevel);
static int jfrd_case_get(unsigned long cno, int id);
static int jfrd_case_find(unsigned short clevel);
static void jfrd_case_rm(unsigned long cno);
static unsigned char *jfrd_case_ins(unsigned char *programid);
static void jfrd_s_ins_rules(void);
static void jfrd_insert_rules(void);
static int jfrd_data(unsigned char *program_id);


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
  { fprintf(sout, "WARNING %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    m = 0;
  }
  else
  { if (eno != 0)
      fprintf(sout, "*** error %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    if (mode == JFE_FATAL)
    { if (eno != 0)
        fprintf(sout, "\n*** PROGRAM ABORTED! ***\n");
      jft_close();
      if (jfrd_head != NULL)
        jfr_close(jfrd_head);
      if (jfrd_darea != NULL)
        free(jfrd_darea);
      jfg_free();
      jfr_free();
      exit(1);
    }
    m = -1;
  }
  return m;
}


/************************************************************************/
/* Funktioner til indlaesning fra fil                                   */
/************************************************************************/

static int jfrd_fl_ip_get(struct jft_data_record *dd)
{
  int m;
  char txt[256];

  m = jft_getdata(dd);
  if (m != 0)
  { if (m != 11) /* eof */
    { sprintf(txt, " %s in file: %s line %d.",
              jft_error_desc.carg, jfrd_da_fname, jft_error_desc.line_no);
      jf_error(jft_error_desc.error_no, txt, JFE_ERROR);
      m = 0;
    }
  }
  return m;
}

static int jfrd_ip_get(void)
{
  int slut, m;
  struct jft_data_record dd;

  slut = 0;
  for (m = 0; slut == 0 && m < jft_dset_desc.record_size; m++)
  { slut = jfrd_fl_ip_get(&dd);
    if (slut == 1 && m != 0)
       return jf_error(11, jfrd_empty, JFE_ERROR);
    if (slut == 0)
    { if (dd.vtype == JFT_VT_EXPECTED)
        jfrd_expected[dd.vno] = dd.farg;
      else
      if (dd.vtype == JFT_VT_INPUT)
      { jfrd_ivalues[dd.vno] = dd.farg;
        if (dd.mode == JFT_DM_MISSING)
          jfrd_confidences[dd.vno] = 0.0;
        else
          jfrd_confidences[dd.vno] = 1.0;
      }
    }
  }
  return slut;
}

/**********************************************************************/
/* funktions to decoding 'extern jfrd...'-statement                   */
/**********************************************************************/

static int jfrd_var_no(const char *text)
{
  int m, res;
  struct jfg_var_desc vdesc;

  res = -1;
  for (m = 0; res == -1 && m < jfrd_pdesc.var_c; m++)
  { jfg_var(&vdesc, jfrd_head, m);
    if (strcmp(vdesc.name, text) == 0)
      res = m;
  }
  return res;
}

static int jfrd_get_command(int argc)
{
  int m, vno;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;

  int state;
       /*  0: start                       */
       /*  1: jfrd                        */
       /*  2: input                       */
       /*  3: output                      */
       /*  4: slut                        */

  jfrd_ff_if_vars = 0;

  m = -1;
  state = 0;

  while (state != -1)
  { m++;
    if (m >= argc)
    { if (state != 4)
     return jf_error(504, jfrd_empty, JFE_ERROR);
    }
    switch (state)
    { case 0:
        if (strcmp(jfrd_words[m], jfrd_t_jfrd) == 0)
          state = 1;
        else
          return jf_error(502, jfrd_words[m], JFE_WARNING);
        break;
      case 1:  /* jfrd */
        if (strcmp(jfrd_words[m], jfrd_t_input) == 0)
          state = 2;
        else
          return jf_error(504, jfrd_words[m], JFE_ERROR);
        break;
      case 2:  /* input */
        if (strcmp(jfrd_words[m], jfrd_t_output) == 0)
          state = 3;
        else
        if (jfrd_words[m][0] == '-')
          m++;
        else
        { vno = jfrd_var_no(jfrd_words[m]);
          if (vno == -1)
            return jf_error(506, jfrd_words[m], JFE_ERROR);
          if (jfrd_ff_if_vars >= JFRD_VMAX)
            return jf_error(505, jfrd_words[m], JFE_ERROR);

          jfrd_if_vars[jfrd_ff_if_vars].var_no = vno;
          jfg_var(&vdesc, jfrd_head, vno);
          jfrd_if_vars[jfrd_ff_if_vars].adjectiv_c = vdesc.fzvar_c;
          jfrd_if_vars[jfrd_ff_if_vars].f_fzvar_no = vdesc.f_fzvar_no;
          if (vdesc.fzvar_c == 0 || vdesc.fzvar_c > 254)
            return jf_error(507, jfrd_words[m], JFE_ERROR);
          if (jfrd_red_mode == -1)
          { jfrd_if_vars[jfrd_ff_if_vars].red_mode = 2;
            jfg_domain(&ddesc, jfrd_head, vdesc.domain_no);
            if (ddesc.type == JFS_DT_CATEGORICAL)
              jfrd_if_vars[jfrd_ff_if_vars].red_mode = 3;
            if (ddesc.type == JFS_DT_INTEGER)
              jfrd_if_vars[jfrd_ff_if_vars].red_mode = 4;
          }
          else
            jfrd_if_vars[jfrd_ff_if_vars].red_mode = jfrd_red_mode;
          jfrd_ff_if_vars++;
        }
        break;
      case 3:
        if (jfrd_words[m][0] == '-')
          m++;
        else
        { vno = jfrd_var_no(jfrd_words[m]);
          if (vno == -1)
            return jf_error(506, jfrd_words[m], JFE_ERROR);
          jfrd_then_var.var_no = vno;
          jfg_var(&vdesc, jfrd_head, vno);
          jfrd_then_var.adjectiv_c = vdesc.fzvar_c;
          jfrd_then_var.f_fzvar_no = vdesc.f_fzvar_no;
          if (vdesc.fzvar_c == 0 || vdesc.fzvar_c > 254)
            return jf_error(507, jfrd_words[m], JFE_ERROR);
          state = 4;
        }
        break;
      case 4:
        state = -1; /* slut */
        if (m < argc)
          return jf_error(504, jfrd_words[m], JFE_ERROR);
        break;
    }
  }
  if (jfrd_ff_if_vars == 0)
  { for (jfrd_ff_if_vars = 0; jfrd_ff_if_vars < jfrd_pdesc.ivar_c;
         jfrd_ff_if_vars++)
    { vno = jfrd_pdesc.f_ivar_no + jfrd_ff_if_vars;
      if (jfrd_ff_if_vars >= JFRD_VMAX)
        return jf_error(505, jfrd_words[m], JFE_ERROR);
      jfrd_if_vars[jfrd_ff_if_vars].var_no = vno;
      jfg_var(&vdesc, jfrd_head, vno);
      jfrd_if_vars[jfrd_ff_if_vars].adjectiv_c = vdesc.fzvar_c;
      jfrd_if_vars[jfrd_ff_if_vars].f_fzvar_no = vdesc.f_fzvar_no;
      if (vdesc.fzvar_c == 0 || vdesc.fzvar_c > 254)
        return jf_error(507, jfrd_words[m], JFE_ERROR);
      if (jfrd_red_mode == -1)
      { jfrd_if_vars[jfrd_ff_if_vars].red_mode = 2;
        jfg_domain(&ddesc, jfrd_head, vdesc.domain_no);
        if (ddesc.type == JFS_DT_CATEGORICAL)
          jfrd_if_vars[jfrd_ff_if_vars].red_mode = 3;
        if (ddesc.type == JFS_DT_INTEGER)
          jfrd_if_vars[jfrd_ff_if_vars].red_mode = 4;
      }
      else
        jfrd_if_vars[jfrd_ff_if_vars].red_mode = jfrd_red_mode;
     }
  }
  return 0;
}

/***********************************************************************/
/* Insert rules in program                                             */
/***********************************************************************/

static unsigned char *jfrd_ins_s_rule(unsigned char *progid)
/* insert current simple rule in program */
{
  int res, ff_tree, v, first;
  unsigned char *pc;
  const char *dummy[2] = { NULL, NULL };

  pc = progid;
  ff_tree = 0;
  first = 1;
  for (v = 0; v < jfrd_ff_if_vars; v++)
  { if (jfrd_if_vars[v].cur != 255)
    { if (ff_tree + 2 >= JFRD_TREE_SIZE)
      { jf_error(514, jfrd_empty, JFE_ERROR);
        return pc;
      }
      jfrd_tree[ff_tree].type = JFG_TT_FZVAR;
      jfrd_tree[ff_tree].sarg_1 = jfrd_if_vars[v].cur
      + jfrd_if_vars[v].f_fzvar_no;
      ff_tree++;
      if (first == 0)
      { jfrd_tree[ff_tree].type = JFG_TT_OP;
        jfrd_tree[ff_tree].op = JFS_ONO_AND;
        jfrd_tree[ff_tree].sarg_1 = ff_tree - 2;
        jfrd_tree[ff_tree].sarg_2 = ff_tree - 1;
        ff_tree++;
      }
      first = 0;
    }
  }
  if (first == 1)                           /* Ingen if-arguments!  */
  { jfrd_tree[ff_tree].type = JFG_TT_TRUE;
    ff_tree++;
  }
  jfrd_sdesc.type = JFG_ST_IF;
  jfrd_sdesc.sec_type = JFG_SST_FZVAR;
  jfrd_sdesc.flags = 0;
  if (jfrd_use_weight == 1)
  { jfrd_sdesc.flags += 3;
    jfrd_sdesc.farg = jfrd_weight;
  }
  jfrd_sdesc.sarg_1 = jfrd_then_var.f_fzvar_no + jfrd_then_var.cur;

  res = jfp_i_tree(jfrd_head, &pc, &jfrd_sdesc,
                   jfrd_tree, ff_tree - 1, 0, 0,
                   dummy, 0);
  if (res != 0)
    jf_error(res, jfrd_empty, JFE_ERROR);
  return pc;
}

static unsigned char *jfrd_ins_c_rule(unsigned char *progid, long srno)
{
  int res, ff_tree, v, first_and, first_or;
  int rm_leave, count, im;
  long sr, b;
  unsigned char *pc;
  unsigned char cthen;
  unsigned char vfrom, vto;
  const char *dummy[2] = { NULL, NULL };

  pc = progid;
  ff_tree = 0;
  first_and = 1;

  sr = jfrd_crule_size * srno;
  sr += sizeof(float) + sizeof(short) + 1;
  cthen = jfrd_c_rules[sr];
  sr++;

  for (v = 0; v < jfrd_ff_if_vars; v++)
  { count = 0; im = 0; vfrom = vto = 0;
    for (b = 0; b < jfrd_if_vars[v].adjectiv_c; b++)
    { if (jfrd_c_rules[sr + b] == 1)
      { count++;
        if (im == 0 || im == 2)
        { im++;
          vfrom = b;
        }
        vto = b;
      }
      else
      { if (im == 1)
       im = 2;
      }
    }
    if (count != jfrd_if_vars[v].adjectiv_c)
    { if (im <= 2 && vfrom != vto &&
          (jfrd_if_vars[v].red_mode == 2 || jfrd_if_vars[v].red_mode == 4))
        /* brug between */
      { if (ff_tree + 3 >= JFRD_TREE_SIZE)
        { jf_error(514, jfrd_empty, JFE_ERROR);
          return pc;
        }
        jfrd_tree[ff_tree].type   = JFG_TT_BETWEEN;
        jfrd_tree[ff_tree].sarg_1 = jfrd_if_vars[v].var_no;
        jfrd_tree[ff_tree].sarg_2 = vfrom;
        jfrd_tree[ff_tree].op     = vto;
        ff_tree++;
        sr += (unsigned long) jfrd_if_vars[v].adjectiv_c;
      }
      else
      { first_or = 1;
        for (b = 0; b < jfrd_if_vars[v].adjectiv_c; b++)
        { if (jfrd_c_rules[sr] == 1)
          { if (ff_tree + 3 >= JFRD_TREE_SIZE)
            { jf_error(514, jfrd_empty, JFE_ERROR);
              return pc;
            }
            jfrd_tree[ff_tree].type = JFG_TT_FZVAR;
            jfrd_tree[ff_tree].sarg_1 = b + jfrd_if_vars[v].f_fzvar_no;
            ff_tree++;
            if (first_or == 0)
            { jfrd_tree[ff_tree].type = JFG_TT_OP;
              jfrd_tree[ff_tree].op = JFS_ONO_OR;
              jfrd_tree[ff_tree].sarg_1 = ff_tree - 2;
              jfrd_tree[ff_tree].sarg_2 = ff_tree - 1;
              ff_tree++;
            }
            first_or = 0;
          }
          sr++;
        }
      }
      if (first_and == 1)
     rm_leave = ff_tree - 1;
      else
      { jfrd_tree[ff_tree].type = JFG_TT_OP;
        jfrd_tree[ff_tree].op = JFS_ONO_AND;
        jfrd_tree[ff_tree].sarg_1 = rm_leave;
        jfrd_tree[ff_tree].sarg_2 = ff_tree - 1;
        rm_leave = ff_tree;
        ff_tree++;
      }
      first_and = 0;
    }
    else
      sr += (unsigned long) jfrd_if_vars[v].adjectiv_c;
  }
  if (first_and == 1)
  { jfrd_tree[ff_tree].type = JFG_TT_TRUE;
    ff_tree++;
  }
  jfrd_sdesc.type = JFG_ST_IF;
  jfrd_sdesc.sec_type = JFG_SST_FZVAR;
  jfrd_sdesc.flags = 0;
  if (jfrd_use_weight == 1)
  { jfrd_sdesc.flags += 3;
    jfrd_sdesc.farg = jfrd_weight;
  }
  jfrd_sdesc.sarg_1 = jfrd_then_var.f_fzvar_no + cthen;

  res = jfp_i_tree(jfrd_head, &pc, &jfrd_sdesc,
                   jfrd_tree, ff_tree - 1, 0, 0,
                   dummy, 0);
  if (res != 0)
    jf_error(res, jfrd_empty, JFE_ERROR);
  return pc;
}

/*************************************************************************/
/* discover                                                              */
/*************************************************************************/

static int jfrd_oom(void)
{
  int res;

  if ((jfrd_ff_darea + 5) * jfrd_drec_size > jfrd_data_size)
    res = 1;
  else
    res = 0;
  return res;
}

static void jfrd_s_get(int dno) /* copies s-rule to jfrd-if_vars etc   */
{
  unsigned long adr;
  int m;

  adr = ((unsigned long) dno) * jfrd_drec_size;

  memcpy((char *) &jfrd_dscore,
         (char *) &(jfrd_darea[adr]), sizeof(float));
  adr += sizeof(float);


  memcpy((char *) &jfrd_rule_count,
         (char *) &(jfrd_darea[adr]), sizeof(short));
  adr += sizeof(short);

  jfrd_conflict = jfrd_darea[adr];
  adr++;

  jfrd_then_var.cur = jfrd_darea[adr];
  adr++;
  for (m = 0; m < jfrd_ff_if_vars; m++)
  { jfrd_if_vars[m].cur = jfrd_darea[adr];
    adr++;
  }
}

static void jfrd_s_put(int dno)   /* copies if_vars-rule til s_rule <dno>. */
{
  unsigned long adr;
  int m;

  adr = ((unsigned long) dno) * jfrd_drec_size;

  memcpy((char *) &(jfrd_darea[adr]),
         (char *) &jfrd_dscore, sizeof(float));
  adr += sizeof(float);


  memcpy((char *) &(jfrd_darea[adr]),
       (char *) &jfrd_rule_count, sizeof(short));
  adr += sizeof(short);

  jfrd_darea[adr] = jfrd_conflict;
  adr++;
  jfrd_darea[adr] = jfrd_then_var.cur;
  adr++;
  for (m = 0; m < jfrd_ff_if_vars; m++)
  { jfrd_darea[adr] = jfrd_if_vars[m].cur;
    adr++;
  }
}

static void jfrd_s_copy(int dno, int sno)
{
  jfrd_s_get(sno);
  jfrd_s_put(dno);
}

static void jfrd_s_update(int dno)
{
  unsigned long adr;
  float f;
  short s;

  adr = dno * jfrd_drec_size;
  memcpy((char *) &f,
         (char *) &(jfrd_darea[adr]), sizeof(float));
  if (jfrd_dscore < f)
    memcpy((char *) &(jfrd_darea[adr]),
           (char *) &jfrd_dscore, sizeof(float));
  adr += sizeof(float);
  memcpy((char *) &s,
         (char *) &(jfrd_darea[adr]), sizeof(short));
  if (jfrd_res_confl == 1)
    s += jfrd_rule_count;
  memcpy((char *) &(jfrd_darea[adr]),
         (char *) &s, sizeof(short));
}

static void jfrd_c_update(int dno)
{
  unsigned long adr;
  float f;
  short s;

  adr = dno * jfrd_crule_size;
  memcpy((char *) &f,
         (char *) &(jfrd_c_rules[adr]), sizeof(float));
  if (jfrd_dscore < f)
    memcpy((char *) &(jfrd_c_rules[adr]),
           (char *) &jfrd_dscore, sizeof(float));
  adr += sizeof(float);
  memcpy((char *) &s,
         (char *) &(jfrd_c_rules[adr]), sizeof(short));
  if (jfrd_res_confl == 1)
    s += jfrd_rule_count;
  memcpy((char *) &(jfrd_c_rules[adr]),
         (char *) &s, sizeof(short));
}

static void jfrd_c_copy(long dno, long sno)
{
  unsigned long s_adr;
  unsigned long d_adr;
  int v, a;

  s_adr = sno * jfrd_crule_size;
  d_adr = dno * jfrd_crule_size;
  memcpy((char *) &(jfrd_c_rules[d_adr]),
         (char *) &(jfrd_c_rules[s_adr]), sizeof(float));
  s_adr += sizeof(float);
  d_adr += sizeof(float);

  memcpy((char *) &(jfrd_c_rules[d_adr]),
         (char *) &(jfrd_c_rules[s_adr]), sizeof(short));
  s_adr += sizeof(short);
  d_adr += sizeof(short);

  jfrd_c_rules[d_adr] = jfrd_c_rules[s_adr];
  s_adr++; d_adr++;

  jfrd_c_rules[d_adr] = jfrd_c_rules[s_adr];
  s_adr++; d_adr++;

  for (v = 0; v < jfrd_ff_if_vars; v++)
  { for (a = 0; a < jfrd_if_vars[v].adjectiv_c; a++)
    { jfrd_c_rules[d_adr] = jfrd_c_rules[s_adr];
      s_adr++; d_adr++;
    }
  }
}

static void jfrd_c_rm(long dno)
{
  jfrd_c_copy(dno, jfrd_ff_c_rules - 1);
  jfrd_ff_c_rules--;
}

static void jfrd_sc_copy(long cno, long sno)
{
  unsigned long s_adr;
  unsigned long c_adr;
  int v;
  unsigned char a;

  s_adr = sno * jfrd_drec_size;
  c_adr = cno * jfrd_crule_size;
  memcpy((char *) &(jfrd_c_rules[c_adr]),
         (char *) &(jfrd_darea[s_adr]), sizeof(float));
  s_adr += sizeof(float);
  c_adr += sizeof(float);

  memcpy((char *) &(jfrd_c_rules[c_adr]),
         (char *) &(jfrd_darea[s_adr]), sizeof(short));
  s_adr += sizeof(short);
  c_adr += sizeof(short);

  jfrd_c_rules[c_adr] = jfrd_darea[s_adr];
  s_adr++;
  c_adr++;

  jfrd_c_rules[c_adr] = jfrd_darea[s_adr];  /* then-cur */
  s_adr++; c_adr++;

  for (v = 0; v < jfrd_ff_if_vars; v++)
  { for (a = 0; a < jfrd_if_vars[v].adjectiv_c; a++)
    { if (a == jfrd_darea[s_adr] || jfrd_darea[s_adr] == 255)
        jfrd_c_rules[c_adr] = 1;
      else
        jfrd_c_rules[c_adr] = 0;
      c_adr++;
    }
    s_adr++;
  }
}


static int jfrd_ch_cmp(unsigned char ch1, unsigned char ch2)
{
  int res;

  res = JFRD_CMP_NEQ;
  if (ch1 == ch2)
    res = JFRD_CMP_EQ;
  else
  if (ch1 == 255)
    res = JFRD_CMP_GT;
  else
  if (ch2 == 255)
    res = JFRD_CMP_LT;

  return res;
}

static int jfrd_s_cmp(int dno1, int dno2)
{
  unsigned long adr1, adr2;
  int m, res, ccmp;
  unsigned char then1, then2;
  int allrule;

  allrule = 0;
  adr1 = (((unsigned long) dno1) * jfrd_drec_size) + sizeof(float);
  adr1 += (unsigned long) (sizeof(short) + 1);
  adr2 = (((unsigned long) dno2) * jfrd_drec_size) + sizeof(float);
  adr2 += (unsigned long) (sizeof(short) + 1);
  then1 = jfrd_darea[adr1];
  then2 = jfrd_darea[adr2];
  adr1++;
  adr2++;
  res = JFRD_CMP_EQ;
  for (m = 0; res != JFRD_CMP_NEQ && m < jfrd_ff_if_vars; m++)
  { ccmp = jfrd_ch_cmp(jfrd_darea[adr1], jfrd_darea[adr2]);
    if (jfrd_darea[adr1] == 255 || jfrd_darea[adr2] == 255)
      allrule = 1;
    if (ccmp == JFRD_CMP_NEQ)
      res = JFRD_CMP_NEQ;
    else
    { if (ccmp != res && res != JFRD_CMP_UNION && ccmp != JFRD_CMP_EQ)
      { if (res == JFRD_CMP_EQ)
          res = ccmp;
        else
          res = JFRD_CMP_UNION;
      }
    }
    adr1++;
    adr2++;
  }

  if (then1 == then2)
  { if (res == JFRD_CMP_NEQ || res == JFRD_CMP_UNION)
      res = JFRD_CMP_INDEPENDENT;
  }
  else
  { if (res == JFRD_CMP_NEQ)
      res = JFRD_CMP_INDEPENDENT;
    else
    { if (allrule == 0)
        res = JFRD_CMP_DATA_CONTRADICTION;
      else
        res = JFRD_CMP_RULE_CONTRADICTION;
    }
  }
  return res;
}

static int jfrd_sc_cmp(int sno, int cno)
{
  unsigned long s_adr, c_adr;
  int m, res, ccmp, count, match;
  unsigned char s_then, c_then;
  unsigned char a;

  s_adr = (((unsigned long) sno) * jfrd_drec_size) + sizeof(float);
  s_adr += (unsigned long) (sizeof(short) + 1);
  s_then = jfrd_darea[s_adr];
  s_adr++;
  c_adr = (((unsigned long) cno) * jfrd_crule_size);
  c_adr += (unsigned long) (sizeof(float) + sizeof(short) + 1);
  c_then = jfrd_c_rules[c_adr];
  c_adr++;

  res = JFRD_CMP_EQ;
  for (m = 0; res != JFRD_CMP_NEQ && m < jfrd_ff_if_vars; m++)
  { count = 0; match = 0;
    for (a = 0; a < jfrd_if_vars[m].adjectiv_c; a++)
    { if (jfrd_c_rules[c_adr] == 1)
      { count++;
        if (a == jfrd_darea[s_adr])
          match = 1;
      }
      c_adr++;
    }
    if (jfrd_darea[s_adr] == 255)
    { if (count == jfrd_if_vars[m].adjectiv_c)
        ccmp = JFRD_CMP_EQ;
      else
        ccmp = JFRD_CMP_GT;
    }
    else
    { if (match == 1)
      { if (count == 1)
          ccmp = JFRD_CMP_EQ;
        else
          ccmp = JFRD_CMP_LT;
      }
      else
     ccmp = JFRD_CMP_NEQ;
    }

    if (ccmp == JFRD_CMP_NEQ)
      res = JFRD_CMP_NEQ;
    else
    { if (ccmp != res && res != JFRD_CMP_UNION && ccmp != JFRD_CMP_EQ)
      { if (res == JFRD_CMP_EQ)
          res = ccmp;
        else
          res = JFRD_CMP_UNION;
      }
    }
    s_adr++;
  }

  if (s_then == c_then)
  { if (res == JFRD_CMP_NEQ || res == JFRD_CMP_UNION)
      res = JFRD_CMP_INDEPENDENT;
  }
  else
  { if (res == JFRD_CMP_NEQ)
      res = JFRD_CMP_INDEPENDENT;
    else
      res = JFRD_CMP_RULE_CONTRADICTION;
  }
  return res;
}

static int jfrd_c_test(unsigned long cno)
{
  unsigned long m;
  int res;

  res = JFRD_CMP_INDEPENDENT;
  for (m = 0; res != JFRD_CMP_RULE_CONTRADICTION && m < jfrd_ff_darea; m++)
  { res = jfrd_sc_cmp(m, cno);
  }
  return res;
}

static int jfrd_rcheck(unsigned long *cruleno, unsigned long adr)
{
  int res, r;
  unsigned long m;

  res = JFRD_CMP_INDEPENDENT;
  for (m = 0; m < jfrd_ff_darea; m++)
  { if (m != adr)
    { r = jfrd_s_cmp(adr, m);
      if (r == JFRD_CMP_EQ)
      { res = r;
        *cruleno = m;
      }
      else
      if (r == JFRD_CMP_LT && res != JFRD_CMP_EQ)
      { res = r;
        *cruleno = m;
      }
      else
      if (r == JFRD_CMP_DATA_CONTRADICTION && res != JFRD_CMP_EQ &&
          res != JFRD_CMP_LT)
      { res = r;
        *cruleno = m;
      }
      else
      if (r == JFRD_CMP_RULE_CONTRADICTION && res != JFRD_CMP_EQ &&
          res != JFRD_CMP_LT && res != JFRD_CMP_DATA_CONTRADICTION)
      { res = r;
        *cruleno = m;
      }
    }
  }
  return res;
}

static void jfrd_resolve_contradiction(unsigned long rno1, unsigned long rno2)
{
  unsigned long adr1, adr2;
  float score_1, score_2;
  short count_1, count_2;

  adr1 = ((unsigned long) rno1) * jfrd_drec_size;
  memcpy((char *) &score_1,
         (char *) &(jfrd_darea[adr1]), sizeof(float));
  adr1 += sizeof(float);
  memcpy((char *) &count_1,
         (char *) &(jfrd_darea[adr1]), sizeof(short));
  adr1 += sizeof(short);

  adr2 = ((unsigned long) rno2) * jfrd_drec_size;
  memcpy((char *) &score_2,
         (char *) &(jfrd_darea[adr2]), sizeof(float));
  adr2 += sizeof(float);
  memcpy((char *) &count_2,
         (char *) &(jfrd_darea[adr2]), sizeof(short));
  adr2 += sizeof(short);

  if (jfrd_res_confl == 0)
  { if (score_1 <= score_2)
    { jfrd_s_copy(rno2, jfrd_ff_darea - 1);
      jfrd_ff_darea--;
      jfrd_conflict_add(count_2);
    }
    else
    { jfrd_s_copy(rno1, rno2);
      jfrd_ff_darea--;   /* MANGLER: conflict-add af alle = rno1 */
    }
  }
  else
  { jfrd_darea[adr1] = 1;
    jfrd_darea[adr2] = 1;
  }
  jfrd_c_contradictions++;
}

static int jfrd_closest_adjectiv(int ifvar_no)
{
  int m, best_cur = 0;
  float best_value, value;
  struct jfg_var_desc vdesc;

  best_value = 0.0;
  jfg_var(&vdesc, jfrd_head, jfrd_if_vars[ifvar_no].var_no);
  for (m = 0; m < jfrd_if_vars[ifvar_no].adjectiv_c; m++)
  { value = jfr_fzvget(jfrd_if_vars[ifvar_no].f_fzvar_no + m);
    if (m == 0 || value > best_value)
    { best_value = value;
      best_cur = m;
    }
  }
  jfrd_dscore += (1.0 - best_value);
  return best_cur;
}


static float jfrd_id3_i(float ant)
{
  float res, p;
  int m;

  res = 0.0;
  if (ant != 0.0)
  { for (m = 0; m < jfrd_then_var.adjectiv_c; m++)
    { p = jfrd_id3_count[m] / ant;
      if (p != 0.0)
        res += p * jfrd_id3_M * log(p);
    }
    res = -res;
  }
  return res;
}

static void jfrd_id3_chose(void)
{
  int a, m, v, worst;
  float it, antal, worst_score, gain, ant2;
  float id3_score[JFRD_VMAX];
  struct jfg_var_desc vdesc;

  antal = 0.0;
  for (a = 0; a < jfrd_then_var.adjectiv_c; a++)
    jfrd_id3_count[a] = 0.0;
  for (a = 0; a < jfrd_ff_darea; a++)
  { jfrd_s_get(a);
    jfrd_id3_count[jfrd_then_var.cur] += 1.0;
    antal += 1.0;
  }
  it = jfrd_id3_i(antal);

  for (v = 0; v < jfrd_ff_if_vars; v++)
  { gain = it;
    for (m = 0; m < jfrd_if_vars[v].adjectiv_c; m++)
    { for (a = 0; a < jfrd_then_var.adjectiv_c; a++)
       jfrd_id3_count[a] = 0.0;
      ant2 = 0.0;
      for (a = 0; a < jfrd_ff_darea; a++)
      { jfrd_s_get(a);
        if (jfrd_if_vars[v].cur == m)
        { jfrd_id3_count[jfrd_then_var.cur] += 1.0;
          ant2 += 1.0;
        }
      }
      if (antal != 0.0)
        gain -= ant2 / antal * jfrd_id3_i(ant2);
      else
        gain = 0.0; /* never! */
    }
    id3_score[v] = gain;
  }

  if (jfrd_silent == 0)
  { fprintf(sout, "\n    Informational gain:\n");
    for (m = 0; m < jfrd_ff_if_vars; m++)
    { jfg_var(&vdesc, jfrd_head, jfrd_if_vars[m].var_no);
      fprintf(sout, "      %16s %6.2f\n", vdesc.name, id3_score[m]);
    }
  }

  worst_score = 0.0;
  for (m = 0; m < jfrd_ff_if_vars; m++)
  { worst = -1;
    for (a = 0; a < jfrd_ff_if_vars; a++)
    { if (id3_score[a] != -1.0)
      { if (worst == -1 || id3_score[a] < worst_score)
        { worst = a;
          worst_score = id3_score[a];
        }
      }
    }
    id3_score[worst] = -1.0;
    jfrd_gains[m] = worst;
  }
}

static int jfrd_red_all(void)
{
  int v, reduced, conflict, res, reduce_c, delete_c;
  unsigned char husk;
  int i, r, r2, np;
  float factor;

  reduce_c = delete_c = 0; np = 1;
  if (jfrd_red_order == 1)
  { for (v = 0; v < jfrd_ff_if_vars; v++)
      jfrd_gains[v] = v;
  }
  else
    jfrd_id3_chose();

  if (jfrd_silent == 0)
    fprintf(sout, "\n    all-reduction                        end\n    ");
  for (r = 0; r < jfrd_ff_darea; r++)
  {
    reduced = 0;
    jfrd_s_get(r);
    for (i = 0; i < jfrd_ff_if_vars; i++)   /* omvendt raekkefoelge ? */
    { v = jfrd_gains[i];
      if (jfrd_if_vars[v].red_mode != 0)
      { husk = jfrd_if_vars[v].cur;
        jfrd_if_vars[v].cur = 255;
        jfrd_s_put(r);
        conflict = 0;
        for (r2 = 0; conflict == 0 && r2 < jfrd_ff_darea; r2++)
        { if (r2 != r)
          { res = jfrd_s_cmp(r, r2);
            if (res == JFRD_CMP_RULE_CONTRADICTION)
              conflict = 1;
          }
        }
        if (conflict == 0)
        { reduced = 1;
          jfrd_if_vars[v].cur = 255;
          jfrd_s_put(r);
        }
        else
        { jfrd_if_vars[v].cur = husk;
          jfrd_s_put(r);
        }
      } /* for i in vars */
    }
    if (reduced == 1)
    { reduce_c++;
      for (r2 = r + 1; r2 < jfrd_ff_darea;  )
      { res = jfrd_s_cmp(r, r2);
        if (res == JFRD_CMP_GT)
        { jfrd_s_get(r2);
          jfrd_s_update(r);
          jfrd_s_get(jfrd_ff_darea - 1); /* sidste rule */
          jfrd_s_put(r2);
          jfrd_ff_darea--;
          delete_c++;
        }
        else
        { if (res != JFRD_CMP_NEQ && res != JFRD_CMP_INDEPENDENT)
            fprintf(sout, "res=%d\n", res);
          r2++;
        }
      }
    } /* reduced == 1 */
    if (jfrd_silent == 0 && jfrd_ff_darea != 0)
    { factor = 40.0 / ((float) jfrd_ff_darea);
      if (r * factor >= np)
      { while (r * factor > np)
        { fprintf(sout, ".");
          np++;
        }
      }
    }
  }
  if (jfrd_silent == 0)
  { while (np <= 40)
    { fprintf(sout, ".");
      np++;
    }
    fprintf(sout,
        "\n    all-reduction finished. %d rules deleted. %d rules reduced.\n",
         delete_c, reduce_c);
  }
  return 0;
}

static void jfrd_call(void)
{
  int m;

  jfrd_dscore = 0.0;
  for (m = 0; m < jfrd_ff_if_vars; m++)
  { jfrd_if_vars[m].cur = jfrd_closest_adjectiv(m);
  }
}

static void jfrd_no_call(void)
{
  return ;
}

static int jfrd_in_contradictions(void)
{
  int m, e;

  if (jfrd_ff_contradictions < JFRD_MAX_CONTRADICTIONS)
    e = jfrd_ff_contradictions;
  else
    e = JFRD_MAX_CONTRADICTIONS;
  for (m = 0; m < e; m++)
  { if (jfrd_contradictions[m] == jfrd_ipcount)
      return 1;
  }
  return 0;
}

static void jfrd_ip2rule(unsigned long rno)
{
  float dist, best_dist;
  int m, v, best_no = 0;

  best_dist = 0.0;
  jfr_arun(jfrd_ovalues, jfrd_head, jfrd_ivalues, jfrd_confidences,
    jfrd_call, NULL, NULL);

  for (m = 0; m < jfrd_then_var.adjectiv_c; m++)
  { jfrd_then_var.cur = m;
    jfrd_ins_s_rule(jfrd_program_id);
    jfr_arun(jfrd_ovalues, jfrd_head, jfrd_ivalues, jfrd_confidences,
      jfrd_no_call, NULL, NULL);
    dist = 0.0;
    for (v = 0; v < jfrd_pdesc.ovar_c; v++)
      dist += fabs(jfrd_ovalues[v] - jfrd_expected[v]);
    if (m == 0 || dist < best_dist)
    { best_no = m;
      best_dist = dist;
    }
    jfp_d_statement(jfrd_head, jfrd_program_id);
  }
  jfrd_then_var.cur = best_no;
  jfrd_dscore += best_dist;
  jfrd_conflict = 0;
  if (jfrd_res_confl == 0)
    jfrd_rule_count = jfrd_ipcount;
  else
    jfrd_rule_count = 1;
  jfrd_s_put(rno);
}

static void jfrd_conflict_add(unsigned long rno)
{
  int m, e, fundet;

  if (jfrd_ff_contradictions < JFRD_MAX_CONTRADICTIONS)
    e = jfrd_ff_contradictions;
  else
    e = JFRD_MAX_CONTRADICTIONS;
  fundet = 0;

  for (m = 0; fundet == 0 && m < e; m++)
  { if (jfrd_contradictions[m] == rno)
      fundet = 1;
  }
  if (fundet == 0)
  { if (jfrd_ff_contradictions < JFRD_MAX_CONTRADICTIONS)
      jfrd_contradictions[jfrd_ff_contradictions] = rno;
    jfrd_ff_contradictions++;
  }
}

static int jfrd_rewind_posible(void)
{
  int res;
  time_t c_time;

  res = 1;
  c_time = time(NULL);
  if ((c_time - jfrd_start_time) / 60 >= jfrd_max_time)
    res = 0;
  if (jfrd_ff_contradictions >= JFRD_MAX_CONTRADICTIONS)
    res = 0;
  return res;
}

static void jfrd_rewind(void)
{
  jft_rewind();
  jfrd_ipcount = 0;
}

static int jfrd_check_cdata(unsigned long rno)
{
  unsigned long rm_ipcount;
  int res, rule_type;

  res = JFRD_CMP_RULE_CONTRADICTION;
  rm_ipcount = jfrd_ipcount;
  jfrd_rewind();
  while (jfrd_ipcount < rm_ipcount)
  { jfrd_ip_get();
    jfrd_ipcount++;
    jfrd_ip2rule(jfrd_ff_darea);
    rule_type = jfrd_s_cmp(rno, jfrd_ff_darea);
    if (rule_type == JFRD_CMP_DATA_CONTRADICTION)
      res = rule_type;
  }
  return res;
}

static void jfrd_red_contra(void)
{
  unsigned long n1, n2;
  unsigned short n1_count;
  int rm_rules;
  int ens;

  rm_rules = 0;
  if (jfrd_res_confl == 1)  /* count */
  { for (n1 = 0; n1 < jfrd_ff_darea; n1++)
    { jfrd_s_get(n1);
      n1_count = jfrd_rule_count;
      if (jfrd_conflict == 1)
      { for (n2 = n1 + 1; n2 < jfrd_ff_darea;  )
        { jfrd_s_get(n2);
          if (jfrd_conflict == 1)
          { ens = jfrd_s_cmp(n1, n2);
            if (ens == JFRD_CMP_DATA_CONTRADICTION)
            { if (jfrd_rule_count > n1_count)
              { n1_count = jfrd_rule_count;
                jfrd_s_copy(n1, n2);
              }
              jfrd_s_copy(n2, jfrd_ff_darea - 1);
              jfrd_ff_darea--;
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
  }
  if (jfrd_silent == 0)
    fprintf(sout, "\n    Contradictions resolved. %d rules removed.\n",
     rm_rules);
}

static unsigned long jfrd_s2c(unsigned long sno)
{
  signed long cno;

  if ((jfrd_ff_c_rules + 2) * jfrd_crule_size >= jfrd_c_size)
    cno = -1;
  else
  {  jfrd_sc_copy(jfrd_ff_c_rules, sno);
     cno = jfrd_ff_c_rules;
     jfrd_ff_c_rules++;
  }
  return cno;
}

static int jfrd_red_in(void)
{
   long m,  sno, cno, bp, bm, cno_bno;
   int okp, okm, i, r, v, iv, np;
   float factor;

   jfrd_c_rules = &(jfrd_darea[jfrd_ff_darea * jfrd_drec_size]);
   jfrd_c_size = jfrd_data_size - (jfrd_ff_darea * jfrd_drec_size);
   np = 1;
   if (jfrd_silent == 0)
     fprintf(sout, "\n    in-reduction                         end\n    ");

   jfrd_ff_c_rules = 0;
   jfrd_crule_size = sizeof(float) + sizeof(short) + 1 + 1;
   for (m = 0; m < jfrd_ff_if_vars; m++)
     jfrd_crule_size += jfrd_if_vars[m].adjectiv_c;

  for (sno = 0; sno < jfrd_ff_darea; sno++)
  { jfrd_s_get(sno);
    jfrd_conflict = 0;
    jfrd_s_put(sno);
  }
  for (sno = 0; sno < jfrd_ff_darea; sno++)
  { jfrd_s_get(sno);
    if (jfrd_conflict == 0)
    { cno = jfrd_s2c(sno);
      if (cno == -1)
      { fprintf(sout, "\n");
     return jf_error(521, jfrd_empty, JFE_ERROR);
      }
      for (i = 0; i < jfrd_ff_if_vars; i++)   /* omvendt raekkefoelge ? */
      { v = jfrd_gains[i];
        cno_bno = cno * jfrd_crule_size + (long) (sizeof(float)
                 + sizeof(short) + 2);
        if (jfrd_if_vars[v].red_mode > 1)
        { if (jfrd_if_vars[v].cur != 255)
          { bp = bm = jfrd_if_vars[v].cur;
            okp = okm = 1;
            for (iv = 0; iv < v; iv++)
              cno_bno += (long) jfrd_if_vars[iv].adjectiv_c;
            while (okp == 1 || okm == 1)
            { bp++;
              if (bp >= jfrd_if_vars[v].adjectiv_c)
                okp = 0;
              /* else */
              if (okp == 1)
              { if (jfrd_c_rules[cno_bno + bp] == 0)
                { jfrd_c_rules[cno_bno + bp] = 1;
                  if (jfrd_c_test(cno) == JFRD_CMP_RULE_CONTRADICTION)
                  { jfrd_c_rules[cno_bno + bp] = 0;
                    if (jfrd_if_vars[v].red_mode == 2)  /* between */
                      okp = 0;
                  }
                }
              }
              if (bm <= 0)
                okm = 0;
              /* else */
              if (okm == 1)
              { bm--;
                if (jfrd_c_rules[cno_bno + bm] == 0)
                { jfrd_c_rules[cno_bno + bm] = 1;
                  if (jfrd_c_test(cno) == JFRD_CMP_RULE_CONTRADICTION)
                  { jfrd_c_rules[cno_bno + bm] = 0;
                    if (jfrd_if_vars[v].red_mode == 2)
                      okm = 0;
                  }
                }
              }
            }
          }
        }
      } /* for (i */
      for (m = 0; m < jfrd_ff_darea; m++)
      { r = jfrd_sc_cmp(m, cno);
        if (r == JFRD_CMP_LT || r == JFRD_CMP_EQ)
        { jfrd_s_get(m);
          jfrd_conflict = 1;
          jfrd_s_put(m);
        }
      }
    }
    if (jfrd_silent == 0 && jfrd_ff_darea != 0)
    { factor = 40.0 / ((float) jfrd_ff_darea);
      if (sno * factor >= np)
      { while (sno * factor > np)
        { fprintf(sout, ".");
          np++;
        }
      }
    }
  }
  /* reduce c-rules */

  if (jfrd_silent == 0)
  { while (np <= 40)
    { fprintf(sout, ".");
      np++;
    }
    fprintf(sout,
         "\n    in-reduction finished. %ld rules deleted.\n",
         jfrd_ff_darea - jfrd_ff_c_rules);
  }

  return 0;
}

static void jfrd_red_def(void)
{
  int m, best_ant, ant_slettet;
  short ant[256];
  long adr;
  long r;
  unsigned char cthen;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adjdesc;

  jfrd_def_fzvar = -1;
  best_ant = ant_slettet = 0;
  for (m = 0; m < jfrd_then_var.adjectiv_c; m++)
    ant[m] = 0;
  for (r = 0; r < jfrd_ff_c_rules; r++)
  { adr =  r * jfrd_crule_size + sizeof(float) + sizeof(short) + 1;
    cthen = jfrd_c_rules[adr];
    ant[cthen] = ant[cthen] + 1;
  }
  for (m = 0; m < jfrd_then_var.adjectiv_c; m++)
  { if (m == 0 || ant[m] > best_ant)
    { jfrd_def_fzvar = m;
      best_ant = ant[m];
    }
  }
  if (jfrd_def_fzvar >= 0)
  { jfg_var(&vdesc, jfrd_head, jfrd_then_var.var_no);
    jfg_adjectiv(&adjdesc, jfrd_head, vdesc.f_adjectiv_no + jfrd_def_fzvar);
    vdesc.default_val = adjdesc.center;
    vdesc.default_no = jfrd_def_fzvar;
    vdesc.default_type = 2; /* Rettes til adjectiv */
    jfp_var(jfrd_head, jfrd_then_var.var_no, &vdesc);
    for (r = 0; r < jfrd_ff_c_rules;  )
    { adr = r * jfrd_crule_size + sizeof(float) + sizeof(short) + 1;
      if (jfrd_c_rules[adr] == jfrd_def_fzvar)
      { jfrd_c_rm(r);
        ant_slettet++;
      }
      else
        r++;
    }
    if (jfrd_silent == 0)
      fprintf(sout,
              "\n    default-reduction finished. %d rules deleted.\n",
              ant_slettet);

  }
}

static int jfrd_case_cmp(unsigned long cno, unsigned short clevel)
{
  int res, m;
  unsigned long adr;
  unsigned short rlevel;

  adr = cno * jfrd_crule_size + sizeof(float);
  memcpy((char *) &rlevel,
         (char *) &(jfrd_c_rules[adr]), sizeof(short));
  if (rlevel != clevel)
    res = 0;
  else
  { res = 1;
    adr += (long) sizeof(short) + 2;
    for (m = 0; m < jfrd_case_id; m++)
      adr += (unsigned long) jfrd_if_vars[m].adjectiv_c;
    for (m = 0; res == 1 && m < jfrd_if_vars[jfrd_case_id].adjectiv_c; m++)
    { if (jfrd_c_rules[adr] != jfrd_case_expr[m])
        res = 0;
      adr++;
    }
  }
  return res;
}

static int jfrd_case_get(unsigned long cno, int id)
{
  unsigned long adr;
  int m, notall;

  notall = 0;
  adr = cno * jfrd_crule_size + sizeof(float) + sizeof(short) + 2;
  for (m = 0; m < id; m++)
    adr += (long) jfrd_if_vars[m].adjectiv_c;
  for (m = 0; m < jfrd_if_vars[id].adjectiv_c; m++)
  { jfrd_case_expr[m] = jfrd_c_rules[adr];
    if (jfrd_c_rules[adr] == 0)
      notall = 1;
    adr++;
  }
  jfrd_case_id = id;
  return notall;
}

static int jfrd_case_find(unsigned short clevel)
{
  unsigned long cno, cno2, best_cno, adr;
  int m, best_id;
  unsigned short count, best_count, rlevel;

  best_count = 0; best_cno = 0; best_id = 0;
  for (cno = 0; cno < jfrd_ff_c_rules; cno++)
  { adr = cno * jfrd_crule_size + sizeof(float);
    memcpy((char *) &rlevel,
           (char *) &(jfrd_c_rules[adr]), sizeof(short));
    if (clevel == rlevel)
    { for (m = 0; m < jfrd_ff_if_vars; m++)
      { count = 0;
        if (jfrd_case_get(cno, m) == 1)
        { for (cno2 = 0; cno2 < jfrd_ff_c_rules; cno2++)
            count += jfrd_case_cmp(cno2, clevel);
        }
        if (count > best_count)
        { best_count = count;
          best_cno = cno;
          best_id = m;
        }
      }
    }
  }
  jfrd_case_get(best_cno, best_id);
  return best_count;
}

static void jfrd_case_rm(unsigned long cno)
{
  unsigned long adr;
  unsigned short rlevel;
  int m;

  adr = cno * jfrd_crule_size + sizeof(float);
  memcpy((char *) &rlevel,
         (char *) &(jfrd_c_rules[adr]), sizeof(short));
  rlevel++;
  memcpy((char *) &(jfrd_c_rules[adr]),
         (char *) &rlevel, sizeof(short));
  adr += sizeof(short) + 2;
  for (m = 0; m < jfrd_case_id; m++)
    adr += jfrd_if_vars[m].adjectiv_c;
  for (m = 0; m < jfrd_if_vars[jfrd_case_id].adjectiv_c; m++)
  { jfrd_c_rules[adr] = 1;
    adr++;
  }
}

static unsigned char *jfrd_case_ins(unsigned char *programid)
{
  unsigned char *pc;
  int ff, m, first_or, res, im, b;
  unsigned char vfrom, vto;
  const char *dummy[2] = { NULL, NULL };

  pc = programid;
  jfrd_sdesc.type = JFG_ST_CASE;
  jfrd_sdesc.flags = 0;
  first_or = 1;
  ff = 0;

  im = 0; vfrom = vto = 0;
  for (b = 0; b < jfrd_if_vars[jfrd_case_id].adjectiv_c; b++)
  { if (jfrd_case_expr[b] == 1)
    { if (im == 0 || im == 2)
      { im++;
        vfrom = b;
      }
      vto = b;
    }
    else
    { if (im == 1)
        im = 2;
    }
  }
  if (im <= 2 && vfrom != vto &&
      (jfrd_if_vars[jfrd_case_id].red_mode == 2
       || jfrd_if_vars[jfrd_case_id].red_mode == 4))
       /* brug between */
  { jfrd_tree[ff].type   = JFG_TT_BETWEEN;
    jfrd_tree[ff].sarg_1 = jfrd_if_vars[jfrd_case_id].var_no;
    jfrd_tree[ff].sarg_2 = vfrom;
    jfrd_tree[ff].op     = vto;
    ff++;
  }
  else
  { for (m = 0; m < jfrd_if_vars[jfrd_case_id].adjectiv_c; m++)
    { if (jfrd_case_expr[m] == 1)
      { jfrd_tree[ff].type = JFG_TT_FZVAR;
        jfrd_tree[ff].sarg_1 = jfrd_if_vars[jfrd_case_id].f_fzvar_no + m;
        ff++;
        if (first_or == 0)
        { jfrd_tree[ff].type = JFG_TT_OP;
          jfrd_tree[ff].op   = JFS_ONO_OR;
          jfrd_tree[ff].sarg_1 = ff - 2;
          jfrd_tree[ff].sarg_2 = ff - 1;
          ff++;
        }
        first_or = 0;
      }
    }
  }
  res = jfp_i_tree(jfrd_head, &pc, &jfrd_sdesc, jfrd_tree, ff - 1, 0, 0,
     dummy, 0);
  if (res != 0)
    jf_error(res, jfrd_empty, JFE_ERROR);

  return pc;
}

static void jfrd_s_ins_rules(void)
{
  int m;
  unsigned char *pc;

  pc = jfrd_program_id;
  for (m = 0; pc != NULL && m < jfrd_ff_darea; m++)
  { jfrd_s_get(m);
    pc = jfrd_ins_s_rule(pc);
    jfrd_ins_rules++;
  }
}

static void jfrd_insert_rules(void)
{
  unsigned long m, adr;
  unsigned short clevel, rlevel;
  unsigned char *pc;
  int a, casecount, res;
  unsigned char cthen;
  const char *dummy[2] = { NULL, NULL };

  pc = jfrd_program_id;

  clevel = 0;
  for (m = 0; m < jfrd_ff_c_rules; m++)         /* count angiver nu level */
  { adr = m * jfrd_crule_size + sizeof(float);
    memcpy((char *) &(jfrd_c_rules[adr]),
           (char *) &clevel, sizeof(short));
  }
  while (jfrd_ff_c_rules > 0)
  { casecount = 1;
    if (jfrd_red_case == 1)
      casecount = jfrd_case_find(clevel);
    if (casecount > 1)
    { jfrd_sdesc.type = JFG_ST_SWITCH;
      jfrd_sdesc.flags = 0;
      res = jfp_i_tree(jfrd_head, &pc, &jfrd_sdesc, jfrd_tree, 0, 0, 0,
                       dummy, 0);
      if (res != 0)
     jf_error(res, jfrd_empty, JFE_ERROR);
      pc = jfrd_case_ins(pc);
      for (m = 0; m < jfrd_ff_c_rules; m++)
      { if (jfrd_case_cmp(m, clevel) == 1)
          jfrd_case_rm(m);
      }
      clevel++;
    }
    else
    { for (a = 0; a < jfrd_then_var.adjectiv_c; a++)
      { for (m = 0; m < jfrd_ff_c_rules;  )
        { adr = m * jfrd_crule_size + sizeof(float);
          memcpy((char *) &rlevel,
                 (char *) &(jfrd_c_rules[adr]), sizeof(short));
          adr += sizeof(short) + 1;
          cthen = jfrd_c_rules[adr];
          if (cthen == a && rlevel == clevel)
          { pc = jfrd_ins_c_rule(pc, m);
            jfrd_ins_rules++;
            jfrd_c_rm(m);
          }
          else
            m++;
        }
      }
      if (clevel > 0)
      { clevel--;
        jfrd_sdesc.type = JFG_ST_STEND;
        jfrd_sdesc.sec_type = 0;
        res = jfp_i_tree(jfrd_head, &pc, &jfrd_sdesc, jfrd_tree, 0, 0, 0,
                         dummy, 0);
        if (res != 0)
          jf_error(res, jfrd_empty, JFE_ERROR);
      }
    }
  }
  while (clevel > 0)
  { clevel--;
    jfrd_sdesc.type = JFG_ST_STEND;
    jfrd_sdesc.sec_type = 0;
    res = jfp_i_tree(jfrd_head, &pc, &jfrd_sdesc, jfrd_tree, 0, 0, 0,
                     dummy, 0);
    if (res != 0)
      jf_error(res, jfrd_empty, JFE_ERROR);
  }

}

static int jfrd_data(unsigned char *program_id)
{
  int rule_type, ctype;
  unsigned long cruleno = 0;

  jfrd_program_id = program_id;
  if ((jfrd_darea = (unsigned char *) malloc(jfrd_data_size)) == NULL)
    return jf_error(6, jfrd_empty, JFE_FATAL);
  jfrd_ipcount = 0;
  jfrd_ff_darea = 0;
  jfrd_ff_contradictions = 0;
  jfrd_c_contradictions = 0;
  jfrd_ins_rules = 0;

  jfrd_drec_size = sizeof(float) + sizeof(short) +
     sizeof(unsigned char) * (jfrd_ff_if_vars + 1 + 1);
  if (jfrd_drec_size % 2 == 1)
    jfrd_drec_size++;

  jfrd_start_time = time(NULL);

  while (jfrd_ip_get() == 0)
  { jfrd_ipcount++;
    if (jfrd_in_contradictions() == 0)
    { if (jfrd_oom() == 0)
      { jfrd_ip2rule(jfrd_ff_darea);
        jfrd_ff_darea++;
        rule_type = jfrd_rcheck(&cruleno, jfrd_ff_darea - 1);
        if (rule_type == JFRD_CMP_DATA_CONTRADICTION)
        { jfrd_resolve_contradiction(cruleno, jfrd_ff_darea - 1);
        }
        else
        if (rule_type == JFRD_CMP_RULE_CONTRADICTION)
        { if (jfrd_rewind_posible() == 1)
          { ctype = jfrd_check_cdata(jfrd_ff_darea - 1);
            if (ctype == JFRD_CMP_DATA_CONTRADICTION)
            { jfrd_conflict_add(jfrd_ipcount);
              jfrd_ff_darea--;
            }
            else /* rule contradiction */
            { jfrd_s_copy(cruleno, jfrd_ff_darea - 1);
              jfrd_ff_darea--;
              jfrd_red_contra();
              if (jfrd_silent == 0)
                fprintf(sout,
           "         Rewinding. Cause: rule contradiction. %d data-sets read.\n",
                        (int) jfrd_ipcount);
              jfrd_rewind();
            }
          }
        }
        else
        if (rule_type == JFRD_CMP_EQ || rule_type == JFRD_CMP_LT)
        { jfrd_s_update(cruleno);
          jfrd_ff_darea--;
        }
        /* else the rule is inserted */
        if (jfrd_oom() == 1)
        { if (jfrd_silent == 0)
            fprintf(sout,
       "         Data reduction. Cause: Out of memory. %d data-sets read.\n",
                    (int) jfrd_ipcount);
          jfrd_red_contra();
          jfrd_red_all();
        }
      }
    }
  } /* while */
  if (jfrd_silent == 0)
    fprintf(sout, "\n    %d rules created from %d data-sets.\n",
         (int) jfrd_ff_darea, (int) jfrd_ipcount);
  jfp_d_statement(jfrd_head, program_id);
  jfrd_red_contra();
  jfrd_red_all();
  if (jfrd_red_in() == 0)
  { if (jfrd_def_fzvar != -1)
      jfrd_red_def();
    jfrd_insert_rules();
  }
  else
   jfrd_s_ins_rules();

  if (jfrd_silent == 0)
    fprintf(sout,
     "\n    %d rules inserted. Created from %d data-sets. %d contraditions.\n",
           (int) jfrd_ins_rules,  (int) jfrd_ipcount,
           (int) jfrd_c_contradictions);


  free(jfrd_darea);
  jfrd_darea = NULL;
  jfrd_ff_darea = 0;
  return 0;
}

int jfrd_run(char *op_fname, char *ip_fname, char *sout_fname, char *da_fname,
             int data_mode, char *field_sep, long prog_size, long data_size,
             int max_time, int red_mode, int red_weight, float weight_value,
             int red_case, int res_confl_mode, int red_order, int def_fzvar,
             int append_mode, int silent, int batch)
{
  int m, slut;
  unsigned char *pc;
  char txt[80];

  sout = stdout;
  if (sout_fname != NULL && strlen(sout_fname) != 0)
  { if (append_mode == 1)
      sout = fopen(sout_fname, "a");
    else
      sout = fopen(sout_fname, "w");
    if (sout == NULL)
      sout = stdout;
  }
  fprintf(sout, "\n  JFRD version 2.03 \n\n");
  jfrd_data_mode = data_mode;
  strcpy(jfrd_da_fname, da_fname);
  strcpy(jfrd_field_sep, field_sep);
  jfrd_prog_size = prog_size;
  jfrd_data_size = data_size;
  jfrd_max_time = max_time;
  jfrd_red_mode = red_mode;
  jfrd_use_weight = red_weight;
  jfrd_weight = weight_value;
  jfrd_red_case = red_case;
  jfrd_res_confl = res_confl_mode;
  jfrd_red_order = red_order;
  jfrd_def_fzvar = def_fzvar;
  jfrd_silent    = silent;
  jfrd_batch     = batch;

  m = jfr_init(0);
  if (m != 0)
    jf_error(m, jfrd_empty, JFE_FATAL);
  m = jfr_aload(&jfrd_head, ip_fname, jfrd_prog_size);
  if (m != 0)
    jf_error(m, ip_fname, JFE_FATAL);
  m = jfg_init(JFG_PM_NORMAL, 64, 4);
  if (m != 0)
    jf_error(m, jfrd_empty, JFE_FATAL);
  m = jfp_init(0);
  if (m != 0)
    jf_error(m, jfrd_empty, JFE_FATAL);

  jft_init(jfrd_head);
  for (m = 0; m < strlen(jfrd_field_sep); m++)
    jft_char_type(jfrd_field_sep[m], JFT_T_SPACE);
  m = jft_fopen(da_fname, data_mode, 0);
  if (m != 0)
    return jf_error(m, da_fname, JFE_FATAL);

  jfrd_id3_M = 1.0 / log(2.0);

  jfg_sprg(&jfrd_pdesc, jfrd_head);

  if (jfrd_silent == 0)
    fprintf(sout, "\n  Rule discovery startet\n");
  pc = jfrd_pdesc.pc_start; slut = 0;
  jfg_statement(&jfrd_sdesc, jfrd_head, pc);
  while (slut == 0 && jfrd_sdesc.type != JFG_ST_EOP)
  { if (jfrd_sdesc.type == JFG_ST_IF && jfrd_sdesc.sec_type == JFG_SST_EXTERN)
    { jfg_t_statement(jfrd_text, JFRD_MAX_TEXT, 2,
                jfrd_tree, JFRD_TREE_SIZE, jfrd_head, -1, pc);
      if (jfrd_silent == 0)
        fprintf(sout, "\n  Now handling statement (in %s):\n  %s\n",
                      ip_fname, jfrd_text);
      m = jfg_a_statement(jfrd_words, JFRD_WMAX, jfrd_head, pc);
      if (m < 0)
        return jf_error(519, jfrd_text, JFE_FATAL);
      if (jfrd_get_command(m) == 0)
      { jfrd_data(pc);
        slut = 1;
      }
      else
        pc = jfrd_sdesc.n_pc;
    }
    else
      pc = jfrd_sdesc.n_pc;
    jfg_statement(&jfrd_sdesc, jfrd_head, pc);
  }

  if (slut == 0)
    return jf_error(520, jfrd_empty, JFE_FATAL);

  m = jfp_save(op_fname, jfrd_head);
  if (m != 0)
    jf_error(m, op_fname, JFE_FATAL);

  jfr_close(jfrd_head);

  jft_close();
  jfg_free();
  jfp_free();
  jfr_free();
  if (jfrd_silent == 0)
  { fprintf(sout, "\n  Rule discovery completed.\n");
    fprintf(sout, "  Changed program written to: %s\n\n", op_fname);
  }  
  if (batch == 0)
  { printf("Press RETURN ...");
    fgets(txt, 78, stdin);
  }
  if (sout != stdout)
    fclose(sout);
  return 0;
}

