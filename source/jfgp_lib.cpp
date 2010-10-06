
  /**************************************************************************/
  /*                                                                        */
  /* jfgp_lib.cpp  Version  2.03  Copyright (c) 1999-2000 Jan E. Mortensen  */
  /*                                                                        */
  /* JFS rule discover-functions using Genetic programing.                  */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <ctype.h>
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfp_lib.h"
#include "jfgp_lib.h"

struct jfgp_stat_rec jfgp_stat;

/************************************************************************/
/* GP-individ description                                               */
/************************************************************************/

struct jfgp_mulige_desc { /* Atom types                        */
                          unsigned char type;
                          unsigned char carg;
                          signed short sarg_1;
                          signed short sarg_2;
                       };

/* where <type> is one of: */

#define JFGP_MT_FZVAR   0  /* sarg_1 = var_no, sarg_2 = f_fzvar_no,       */
                           /* carg = fzvar_c.                             */

#define JFGP_MT_OP      1  /* carg = op_no,                               */

#define JFGP_MT_HEDGE   2  /* carg=hedgeno.                               */

#define JFGP_MT_BETWEEN 3  /* sarg_1 = var_no, sarg_2 = f_fzvar_no.       */
                           /* carg = fzvar_c                              */

#define JFGP_MT_UREL    4  /* carg = urel-no.                             */

#define JFGP_MT_DFUNC   5  /* carg in PLUS..LEQ (0..15).                  */

#define JFGP_MT_VAR     6  /* sarg_1 = var_no.                            */

#define JFGP_MT_SFUNC   7  /* carg in COS..CEIL (0.. 9).                  */

#define JFGP_MT_INT     8  /* sarg_1 = min_val, sarg_2 = antal (max_val = */
                           /* sarg_1 + sarg_2 - 1).                       */

#define JFGP_MT_FLOAT   9  /* sarg_1 : precision (ie number in            */
                           /* rand(sarg_1+1)/(sarg_1)).                   */

#define JFGP_MT_ARRAY  10  /* carg = array-no.                            */

#define JFGP_MT_ARGLIST 11 /* SPECIAL (kan kun oprettes indirekte).       */

#define JFGP_MT_IIF     12 /* iif-function.                               */

#define JFGP_MT_UFUNC   13 /* user-defined function                       */
                           /* carg=function-no, sarg_1 = arg_c.           */

#define JFGP_MT_UF_VAR  14 /* variable in user-defined function           */
                           /* sarg_1 = var_no (in function).              */
#define JFGP_MT_MAX 512

static int jfgp_ff_mt = 0;
static struct jfgp_mulige_desc jfgp_mulige[JFGP_MT_MAX];
     /* The atom-types from                          */
     /* then "call jfgp".                            */
     /* NB jfgp_mulige[jfgp_ff_mt] = JFGP_MT_ARGLIST */
     /*    (vaelges ikke til nye atomer).            */


struct jfgp_atom_desc {            /* an atom.                     */
       signed short  mref;         /* reference to jfgp_mulige.    */
                                   /* -1: free.                    */
       signed long  adr_1;
       signed long  adr_2;
       signed long  ref_c;   /* no of references to atom from  */
                             /* individs.                      */
                };

/* The use of <adr_1>, <adr_2> depends on jfgp_mulige[mref].type:          */

/* JFGP MT_FZVAR    adr_1 = cur_fzvno                                      */
/*                 (ie  fzvar_no = jfgp_mulige[mref].sarg_1 + adr_1).      */

/* JFGP_MT_OP       arg1 = jfgp_atoms[adr_1],                              */
/*                  arg2 = jfgp_atoms[adr 2].                              */

/* JFGP_MT_HEDGE    arg  = jfgp_atoms[adr_1].                              */

/* JFGP_MT_BETWEEN  adr_1 = from_cur, adr_2 = to_cur.                      */

/* JFGP_MT_UREL     arg1 = jfgp_atoms[adr_1],                              */
/*                  arg2 = jfgp_atoms[adr_2],                              */

/* JFGP_MT_DFUNC    arg1 = jfgp_atoms[adr_1],                              */
/*                  arg2 = jfgp_atoms[adr_2],                              */

/* JFGP_MT_VAR      var_no = jfgp_mulige[mref].sarg_1.                     */

/* JFGP_MT_SFUNC    arg = jfgp_atoms[adr_1].                               */

/* JFGP_MT_INT      value = jfgp_mulige[mref].sarg_1 + adr_1.              */

/* JFGP_MT_FLOAT    value = adr_1 / (jfgp_mulige[mref].sarg_1.             */

/* JFGP_MT_ARRAY    index = jfg_atoms[adr_1].                              */

/* JFGP_MT_ARGLIST  arg1  = jfgp_atoms[adr_1],                             */
/*                  arg2  = jfgp_atoms[adr_2].                             */

/* JFGP_MT_IIF      arg1  = jfgp_atoms[adr_1] (cond-expr),                 */
/*                  arg2  = jfgp_atoms[adr_2] (arglist(expr1, expr2)).     */

/* JFGP_MT_UFUNC    arg1  = jfgp_atoms[adr_1].                             */

/* JFGP_MT_UF_VAR   var_no = jfgp_mulige[mref].sarg_1.                     */

/* mref=-1          next_empty = jfgp_atoms[adr_1].                        */


struct jfgp_jfgp_stat_desc             /* describes a rule                 */
{
  unsigned char *pc;
  int f_mulige;
  int mulige_c;
  int type;              /* 0: if <expr> then <fzvar>,                     */
                         /* 1: <var> = <expr>,                             */
                         /* 2: case <expr>,                                */
                         /* 3: return <expr>                               */

  unsigned short no;     /* type=0: <fzvar-no>, 1,2: <var_no>.             */
  int terminal_c;
};
#define JFGP_STAT_COUNT 64
struct jfgp_jfgp_stat_desc jfgp_stats[JFGP_STAT_COUNT];
int jfgp_ff_stat;

struct jfgp_rule_head_desc             /* A single rule in an individ.     */

    { long         atom_no;  /* jfgp_atoms[atom_no] is rule. */
      signed short rsize;    /* rulesize (atom-count).       */
    };


#define JFGP_IS_ALIVE 0
#define JFGP_IS_DEAD  1
struct jfgp_ind_desc              /* Individ description                */
      { float        score;
        signed short tsize;       /* total size                         */
        signed short status;      /* JFGP_IS_ALIVE or JFGP_IS_DEAD      */
      };

#define JFGP_IND_SIZE sizeof(struct jfgp_ind_desc)


static struct jfgp_atom_desc      *jfgp_atoms;
static struct jfgp_ind_desc       *jfgp_inds;
static struct jfgp_rule_head_desc *jfgp_rule_heads;
                                    /* [ind_c * ff_stats_c]     */

struct jfgp_head_desc
{
   unsigned short    ind_c;
   unsigned short    alive_c;
   long              atom_c;
   unsigned short    free_list;       /* jfgp_atoms[free_list] is first   */
                                      /* free atom.                       */
   unsigned short    free_c;          /* no of atoms in free-list.        */

   void *jfr_head;
   float (*judge)(void);
   int (*compare)(float score_1, int size_1, float score_2, int size_2);
};

static struct jfgp_head_desc jfgp_head;

static signed short jfgp_count;  /* used to count the size of a rule.    */
static int jfgp_slut;

/**************************************************************************/
/* Program-data                                                           */
/**************************************************************************/

static struct jfg_sprog_desc  jfgp_spdesc;

#define JFGP_MAX_TREE 120

static int jfgp_no_deletes = 0;  /* no of rules to delete before insert. */

static struct jfg_tree_desc jfgp_tree[JFGP_MAX_TREE];
                                /* Used to insert rule.   */
static int jfgp_ff_tree;

static int jfgp_tournament_size = 5;

/*************************************************************************/
/* Div                                                                   */
/*************************************************************************/

#define JFGP_IT_INSERT  0
#define JFGP_IT_REPLACE 1
#define JFGP_IT_DELETE  2

struct jfgp_kw_desc { const char *name;
                      int  value;
                    };


static struct jfgp_kw_desc jfgp_kw_dfunc[] =
        { {"=",    JFS_ROP_EQ},
          {"==",   JFS_ROP_EQ},
          {"!=",   JFS_ROP_NEQ},
          {"<>",   JFS_ROP_NEQ},
          {">",    JFS_ROP_GT},
          {">=",   JFS_ROP_GEQ},
          {"<",    JFS_ROP_LT},
          {"<=",   JFS_ROP_LEQ},
          {"min",  JFS_DFU_MIN},
          {"max",  JFS_DFU_MAX},
          {"cut",  JFS_DFU_CUT},
          {"+",    JFS_DFU_PLUS},
          {"-",    JFS_DFU_MINUS},
          {"*",    JFS_DFU_PROD},
          {"/",    JFS_DFU_DIV},
          {"pow",  JFS_DFU_POW},
          {" ",   -1}
        };

static struct jfgp_kw_desc jfgp_kw_sfunc[] =
        { {"cos",   JFS_SFU_COS},
          {"sin",   JFS_SFU_SIN},
          {"tan",   JFS_SFU_TAN},
          {"acos",  JFS_SFU_ACOS},
          {"asin",  JFS_SFU_ASIN},
          {"atan",  JFS_SFU_ATAN},
          {"log",   JFS_SFU_LOG},
          {"fabs",  JFS_SFU_FABS},
          {"floor", JFS_SFU_FLOOR},
          {"ceil",  JFS_SFU_CEIL},
          {"sqr",   JFS_SFU_SQR},
          {"sqrt",  JFS_SFU_SQRT},
          {"wget",  JFS_SFU_WGET},
          {" ",   -1}
        };

static const char jfgp_t_gp[]        = "JFGP";
static const char jfgp_t_operators[] = "OPERATORS";
static const char jfgp_t_integer[]   = "INTEGER";
static const char jfgp_t_float[]     = "FLOAT";
static const char jfgp_t_hedges[]    = "HEDGES";
static const char jfgp_t_functions[] = "FUNCTIONS";
static const char jfgp_t_fzvars[]    = "FZVARS";
static const char jfgp_t_vars[]      = "VARS";
static const char jfgp_t_arrays[]    = "ARRAYS";
static const char jfgp_t_then[]      = "THEN";
static const char jfgp_t_iif[]       = "IIF";
static const char jfgp_t_assign[]    = "ASSIGN";
static const char jfgp_t_case[]      = "CASE";
static const char jfgp_t_return[]    = "RETURN";

static int jfgp_stricmp(const char *a1, const char *a2);
static int jfgp_find_var(const char *vname);
static int jfgp_find_ufvar(int function_no, const char *vname);
static int jfgp_hedge_find(const char *hname);
static int jfgp_urel_find(const char *urname);
static int jfgp_op_find(const char *oname);
static int jfgp_array_find(const char *aname);
static int jfgp_kw_find(struct jfgp_kw_desc *kws, const char *name);
static int jfgp_adesc_create(void);

static void jfgp_i_judge(int ind_no);

static int jfgp_random(int sup);
static int jfgp_ind_rand(int sign);

static int jfgp_gp_br_ins(int atom_no);
static int jfgp_gpi2p(long ind_no);
static int jfgp_i2p(long source_no);

static signed short jfgp_1_count(long ano);
static long jfgp_1_find(long ano, long iano);
static int jfgp_argc(int mtyp);
static int jfgp_scros(long ind_no);

static signed short jfgp_ir_change(long ano, signed short value);
static signed short jfgp_rule_change(signed short hno, signed short value);

static long jfgp_i_rrha(long ano, long iano);
static signed short jfgp_rrha_atom(signed short rhno);
static long jfgp_mutate(unsigned short stat_no,
                        long sano_1, long sano_2, long sano_3);
static int jfgp_uc_argc(int mno);
static long jfgp_u_crosover(long sano_1,  long sano_2);

static void   jfgp_ind_rm(signed short ino);
static signed short jfgp_ind_create(signed short indno);
static void jfgp_aterminal(int rno, long atom_no);
static int jfgp_pop_create(void);


/*************************************************************************/
/* Create mutaion-types (jfgp_mulige) from call-statement                */
/*************************************************************************/

static int jfgp_stricmp(const char *a1, const char *a2)
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

static int jfgp_find_var(const char *vname)
{
  int m, res;
  struct jfg_var_desc vdesc;

  res = -1;
  for (m = 0; res == -1 && m < jfgp_spdesc.var_c; m++)
  { jfg_var(&vdesc, jfgp_head.jfr_head, m);
    if (strcmp(vname, vdesc.name) == 0)
      res = m;
  }
  return res;
}

static int jfgp_find_ufvar(int function_no, const char *vname)
{
  int m, res;
  struct jfg_function_desc fdesc;
  struct jfg_func_arg_desc fadesc;

  res = -1;
  jfg_function(&fdesc, jfgp_head.jfr_head, function_no);
  for (m = 0; res == -1 && m < fdesc.arg_c; m++)
  { jfg_func_arg(&fadesc, jfgp_head.jfr_head, function_no, m);
    if (strcmp(vname, fadesc.name) == 0)
      res = m;
  }
  return res;
}

static int jfgp_hedge_find(const char *hname)
{
  int m, res;
  struct jfg_hedge_desc hdesc;

  res = -1;
  for (m = 0; res == -1 && m < jfgp_spdesc.hedge_c; m++)
  { jfg_hedge(&hdesc, jfgp_head.jfr_head, m);
    if (strcmp(hname, hdesc.name) == 0)
      res = m;
  }
  return res;
}

static int jfgp_ufunc_find(const char *ufname)
{
   int m, res;
   struct jfg_function_desc fdesc;

   res = -1;
   for (m = 0; res == -1 && m < jfgp_spdesc.function_c; m++)
   { jfg_function(&fdesc, jfgp_head.jfr_head, m);
     if (strcmp(ufname, fdesc.name) == 0)
       res = m;
   }
   return res;
}

static int jfgp_urel_find(const char *urname)
{
  int m, res;
  struct jfg_relation_desc urel;

  res = -1;
  for (m = 0; res == -1 && m < jfgp_spdesc.relation_c; m++)
  { jfg_relation(&urel, jfgp_head.jfr_head, m);
    if (strcmp(urname, urel.name) == 0)
      res = m;
  }
  return res;
}

static int jfgp_op_find(const char *oname)
{
  int m, res;
  struct jfg_operator_desc opdesc;

  res = -1;
  for (m = 0; res == -1 && m < jfgp_spdesc.operator_c; m++)
  { jfg_operator(&opdesc, jfgp_head.jfr_head, m);
    if (strcmp(oname, opdesc.name) == 0)
      res = m;
  }
  return res;
}

static int jfgp_array_find(const char *aname)
{
  int m, res;
  struct jfg_array_desc adesc;

  res = -1;
  for (m = 0; res == -1 && m < jfgp_spdesc.array_c; m++)
  { jfg_array(&adesc, jfgp_head.jfr_head, m);
    if (strcmp(aname, adesc.name) == 0)
      res = m;
  }
  return res;
}

static int jfgp_kw_find(struct jfgp_kw_desc *kws, const char *name)
{
  int m, res;

  res = -2;
  for (m = 0; res == -2; m++)
  { if (kws[m].value == -1)
      res = -1;
    else
    { if (strcmp(kws[m].name, name) == 0)
        res = kws[m].value;
    }
  }
  return res;
}

static int jfgp_adesc_create(void)
{
  const char *jfgp_words[128];
  int m, v, state, no, i, j;
  int ff_mt, funo, fv, terminal_c, slut, stype;
  unsigned char *pc;
  struct jfg_statement_desc sdesc;
  struct jfg_var_desc vdesc;
  struct jfg_function_desc fdesc;

  slut = 0;
  jfgp_ff_mt = 0;
  jfgp_ff_stat = 0;
  for (funo = 0; funo <= jfgp_spdesc.function_c; funo++)
  { if (funo == jfgp_spdesc.function_c)
      pc = jfgp_spdesc.pc_start;
    else
    { jfg_function(&fdesc, jfgp_head.jfr_head, funo);
      pc = fdesc.pc;
    }
    jfg_statement(&sdesc, jfgp_head.jfr_head, pc);
    while (!( (sdesc.type == JFG_ST_STEND && sdesc.sec_type == 2)
          ||  sdesc.type == JFG_ST_EOP))
    { if (sdesc.type == JFG_ST_IF && sdesc.sec_type == JFG_SST_EXTERN)
      { m = jfg_a_statement(jfgp_words, 128, jfgp_head.jfr_head, pc);
        if (m < 0)
          return 519;
        if (m < 5)
          return 904;
        v = 0; state = 0; terminal_c = 0;
        ff_mt = jfgp_ff_mt;
        while (v < m && state != 6)
        { switch (state)
          { case 0:
              if (jfgp_stricmp(jfgp_words[v], jfgp_t_gp) == 0)
              { slut = 1;
                state = 7; /* gp */
              }
              else
                state = 6;
              v++;
              break;
            case 7:
              if (jfgp_stricmp(jfgp_words[v], jfgp_t_operators) == 0)
                state = 8;
              else
              if (jfgp_stricmp(jfgp_words[v], jfgp_t_hedges) == 0)
                state = 8;
              else
              if (jfgp_stricmp(jfgp_words[v], jfgp_t_functions) == 0)
                state = 8;
              else
              if (jfgp_stricmp(jfgp_words[v], jfgp_t_fzvars) == 0)
                state = 10;
              else
              if (jfgp_stricmp(jfgp_words[v], jfgp_t_then) == 0)
              { stype = 0;
                state = 11;
              }
              else
              if (jfgp_stricmp(jfgp_words[v], jfgp_t_assign) == 0)
              { stype = 1;
                state = 11;
              }
              else
              if (jfgp_stricmp(jfgp_words[v], jfgp_t_case) == 0)
              { jfgp_stats[jfgp_ff_stat].pc = pc;
                jfgp_stats[jfgp_ff_stat].f_mulige = ff_mt;
                jfgp_stats[jfgp_ff_stat].mulige_c = jfgp_ff_mt - ff_mt;
                jfgp_stats[jfgp_ff_stat].type = 2;
                jfgp_stats[jfgp_ff_stat].terminal_c = terminal_c;
                jfgp_ff_stat++;
                state = 6;
                jfp_d_statement(jfgp_head.jfr_head, pc); /* NB Delete */
                sdesc.n_pc = pc;
                jfg_sprg(&jfgp_spdesc, jfgp_head.jfr_head);
                    /* delete might change pc program block */
              }
              else
              if (jfgp_stricmp(jfgp_words[v], jfgp_t_return) == 0)
              { jfgp_stats[jfgp_ff_stat].pc = pc;
                jfgp_stats[jfgp_ff_stat].f_mulige = ff_mt;
                jfgp_stats[jfgp_ff_stat].mulige_c = jfgp_ff_mt - ff_mt;
                jfgp_stats[jfgp_ff_stat].type = 3;
                jfgp_stats[jfgp_ff_stat].terminal_c = terminal_c;
                jfgp_ff_stat++;
                state = 6;
                jfp_d_statement(jfgp_head.jfr_head, pc); /* NB Delete */
                sdesc.n_pc = pc;
                jfg_sprg(&jfgp_spdesc, jfgp_head.jfr_head);
                    /* delete might change pc program block */
              }
              else
              if (jfgp_stricmp(jfgp_words[v], jfgp_t_vars) == 0)
                state = 13;
              else
              if (jfgp_stricmp(jfgp_words[v], jfgp_t_integer) == 0)
                state = 14;
              else
              if (jfgp_stricmp(jfgp_words[v], jfgp_t_float) == 0)
                state = 15;
              else
              if (jfgp_stricmp(jfgp_words[v], jfgp_t_arrays) == 0)
                state = 16;
              else
                return 902;
              v++;
              break;
            case 8:  /* functions, operators, hedges */
              if ((i = jfgp_op_find(jfgp_words[v])) != -1)
              { jfgp_mulige[jfgp_ff_mt].carg = i;
                jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_OP;
                v++;
                jfgp_ff_mt++;
              }
              else
              if ((i = jfgp_urel_find(jfgp_words[v])) != -1)
              { jfgp_mulige[jfgp_ff_mt].carg = i;
                jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_UREL;
                v++;
                jfgp_ff_mt++;
              }
              else
              if ((i = jfgp_kw_find(jfgp_kw_dfunc, jfgp_words[v])) != -1)
              { jfgp_mulige[jfgp_ff_mt].carg = i;
                jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_DFUNC;
                v++;
                jfgp_ff_mt++;
              }
              else
              if ((no = jfgp_hedge_find(jfgp_words[v])) != -1)
              { jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_HEDGE;
                jfgp_mulige[jfgp_ff_mt].carg = no;
                v++;
                jfgp_ff_mt++;
              }
              else
              if ((i = jfgp_kw_find(jfgp_kw_sfunc, jfgp_words[v])) != -1)
              { jfgp_mulige[jfgp_ff_mt].carg = i;
                jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_SFUNC;
                v++;
                jfgp_ff_mt++;
              }
              else
              if (jfgp_stricmp(jfgp_words[v], jfgp_t_iif) == 0)
              { jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_IIF;
                jfgp_mulige[jfgp_ff_mt].carg = 3;
                v++;
                jfgp_ff_mt++;
              }
              else
              if ((i = jfgp_ufunc_find(jfgp_words[v])) != -1)
              { jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_UFUNC;
                jfgp_mulige[jfgp_ff_mt].carg = i;
                jfg_function(&fdesc, jfgp_head.jfr_head, i);
                jfgp_mulige[jfgp_ff_mt].sarg_1 = fdesc.arg_c;
                v++;
                jfgp_ff_mt++;
              }
              else
                state = 7;
              break;
            case 10: /* fz vars */
              if ((no = jfgp_find_var(jfgp_words[v])) == -1)
                state = 7;
              else
              { jfg_var(&vdesc, jfgp_head.jfr_head, no);
                if (vdesc.fzvar_c == 0)
                  return 910;
                if (vdesc.fzvar_c > 2)
                { jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_BETWEEN;
                  jfgp_mulige[jfgp_ff_mt].sarg_1 = no;
                  jfgp_mulige[jfgp_ff_mt].sarg_2 = vdesc.f_fzvar_no;
                  jfgp_mulige[jfgp_ff_mt].carg = vdesc.fzvar_c;
                  jfgp_ff_mt++;
                  terminal_c++;
                }
                jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_FZVAR;
                jfgp_mulige[jfgp_ff_mt].sarg_1 = no;
                jfgp_mulige[jfgp_ff_mt].sarg_2 = vdesc.f_fzvar_no;
                jfgp_mulige[jfgp_ff_mt].carg = vdesc.fzvar_c;
                jfgp_ff_mt++;
                v++;
                terminal_c++;
              }
              break;
            case 11:    /* then, assign */
              if ((no = jfgp_find_var(jfgp_words[v])) == -1)
                return 7;
              if (terminal_c == 0)
                return 906;
              jfg_var(&vdesc, jfgp_head.jfr_head, no);
              if (stype == 0)
              { for (fv = 0; fv < vdesc.fzvar_c; fv++)
                { jfgp_stats[jfgp_ff_stat].pc = pc;
                  jfgp_stats[jfgp_ff_stat].f_mulige = ff_mt;
                  jfgp_stats[jfgp_ff_stat].mulige_c = jfgp_ff_mt - ff_mt;
                  jfgp_stats[jfgp_ff_stat].type = 0;
                  jfgp_stats[jfgp_ff_stat].no = vdesc.f_fzvar_no + fv;
                  jfgp_stats[jfgp_ff_stat].terminal_c = terminal_c;
                  jfgp_ff_stat++;
                }
              }
              else
              if (stype == 1)  /* var = <expr> */
              { jfgp_stats[jfgp_ff_stat].pc = pc;
                jfgp_stats[jfgp_ff_stat].f_mulige = ff_mt;
                jfgp_stats[jfgp_ff_stat].mulige_c = jfgp_ff_mt - ff_mt;
                jfgp_stats[jfgp_ff_stat].type = 1;
                jfgp_stats[jfgp_ff_stat].no = no;
                jfgp_stats[jfgp_ff_stat].terminal_c = terminal_c;
                jfgp_ff_stat++;
              }
              v++;
              if (m == v)
              { state = 6;
                jfp_d_statement(jfgp_head.jfr_head, pc); /* NB Delete */
                sdesc.n_pc = pc;
                jfg_sprg(&jfgp_spdesc, jfgp_head.jfr_head);
                    /* delete might change pc program block */
              }
              break;
            case 13: /* vars */
             if ((no = jfgp_find_var(jfgp_words[v])) != -1)
             { jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_VAR;
               jfgp_mulige[jfgp_ff_mt].sarg_1 = no;
               v++;
               jfgp_ff_mt++;
               terminal_c++;
             }
             else
             { if (funo < jfgp_spdesc.function_c)
               { if ((no = jfgp_find_ufvar(funo, jfgp_words[v])) != -1)
                 { jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_UF_VAR;
                   jfgp_mulige[jfgp_ff_mt].sarg_1 = no;
                   v++;
                   jfgp_ff_mt++;
                   terminal_c++;
                 }
               }
             }
             if (no == -1)
               state = 7;
             break;
           case 14: /* integer */
             jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_INT;
             i = atoi(jfgp_words[v]);
             v++;
             j = atoi(jfgp_words[v]);
             if (j < i || ((float) j) - ((float) i) > 32000.0)
               return 903;
             v++;
             jfgp_mulige[jfgp_ff_mt].sarg_1 = i;
             jfgp_mulige[jfgp_ff_mt].sarg_2 = j - i + 1;
             jfgp_ff_mt++;
             terminal_c++;
             state = 7;
             break;
           case 15: /* float */
             jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_FLOAT;
             i = atoi(jfgp_words[v]);
             v++;
             if (i <= 0 || i > 3)
               return 905;
             jfgp_mulige[jfgp_ff_mt].sarg_1 = 1;
             while (i > 0)
             { jfgp_mulige[jfgp_ff_mt].sarg_1 *= 10;
               i--;
             }
             jfgp_ff_mt++;
             terminal_c++;
             state = 7;
             break;
           case 16: /* arrays */
             if ((i = jfgp_array_find(jfgp_words[v])) != -1)
             { jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_ARRAY;
               jfgp_mulige[jfgp_ff_mt].carg = i;
               v++;
               jfgp_ff_mt++;
             }
             else
               state = 7;
             break;
          }
        }
        if (state != 6)
          return 7;
      }
      pc = sdesc.n_pc;
      jfg_statement(&sdesc, jfgp_head.jfr_head, pc);
    }
  }

  if (jfgp_ff_stat == 0)
  { if (slut == 0)
      return 901;
    else
      return 904;
  }
  jfgp_mulige[jfgp_ff_mt].type = JFGP_MT_ARGLIST;
  /* NB jfgp_ff_mt taelles ikke op. ARGLISTE kan ikke vaelges */

  return 0;
}

/*************************************************************************/
/* Generelle hjaelpe-funktioner                                          */
/*************************************************************************/


static void jfgp_i_judge(int ind_no)
{
  if (jfgp_i2p(ind_no) != 0)
    jfgp_ind_rm(ind_no);
  else
    jfgp_inds[ind_no].score = jfgp_head.judge();
}


/*************************************************************************/
/* Hjaelpe funktioner til crosover og mutation                           */
/*************************************************************************/

static int jfgp_random(int sup)
{
  int res;

  res = (int)(rand() * ((float) sup) / (RAND_MAX+1.0));
  /* res = rand() % sup; */
  return res;

}

static int jfgp_ind_rand(int sign)
{
  /* selects a random individ, if sign==-1 a bad individ is selected  */
  /* if sign==1 a good individ is selected.                           */
  int c, ino, i;

  ino = -1;
  for (c = 0; c < jfgp_tournament_size; )
  { i = jfgp_random(jfgp_head.ind_c);
    if (i != ino && jfgp_inds[i].status == JFGP_IS_ALIVE)
    { if (ino == -1)
        ino = i;
      else
      { if (jfgp_head.compare(jfgp_inds[i].score, jfgp_inds[i].tsize,
                              jfgp_inds[ino].score, jfgp_inds[ino].tsize)
            == sign)
          ino = i;
      }
      c++;
    }
  }
  return ino;
}

/*************************************************************************/
/* Konverteringsfunktioner                                               */
/*************************************************************************/

static int jfgp_gp_br_ins(long atom_no)  /* converts a rule to a jfg_tree */
{
  struct jfgp_atom_desc *atom;
  struct jfgp_mulige_desc *mul;
  int h, res;

  res = 0;
  if (jfgp_ff_tree + 3 >= JFGP_MAX_TREE)
    res = 1;
  if (res == 0)
  { atom = &(jfgp_atoms[atom_no]);
    mul = &(jfgp_mulige[atom->mref]);
    switch (mul->type)
    { case JFGP_MT_FZVAR:
        jfgp_tree[jfgp_ff_tree].type = JFG_TT_FZVAR;
        jfgp_tree[jfgp_ff_tree].sarg_1 = mul->sarg_2
                         + (unsigned short) atom->adr_1;
        jfgp_ff_tree++;
        break;
      case JFGP_MT_VAR:
        jfgp_tree[jfgp_ff_tree].type = JFG_TT_VAR;
        jfgp_tree[jfgp_ff_tree].sarg_1 = mul->sarg_1;
        jfgp_ff_tree++;
        break;
      case JFGP_MT_UF_VAR:
        jfgp_tree[jfgp_ff_tree].type = JFG_TT_UF_VAR;
        jfgp_tree[jfgp_ff_tree].sarg_1 = mul->sarg_1;
        jfgp_ff_tree++;
        break;
      case JFGP_MT_INT:
        jfgp_tree[jfgp_ff_tree].type = JFG_TT_CONST;
        jfgp_tree[jfgp_ff_tree].farg
           = ((float) mul->sarg_1) + (float) atom->adr_1;
        jfgp_tree[jfgp_ff_tree].op = 1;
        jfgp_ff_tree++;
        break;
      case JFGP_MT_FLOAT:
        jfgp_tree[jfgp_ff_tree].type = JFG_TT_CONST;
        jfgp_tree[jfgp_ff_tree].farg
           = ((float) atom->adr_1) / ((float) mul->sarg_1);
        jfgp_tree[jfgp_ff_tree].op = 1;
        jfgp_ff_tree++;
        break;
      case JFGP_MT_OP:
      case JFGP_MT_UREL:
      case JFGP_MT_DFUNC:
      case JFGP_MT_IIF:
      case JFGP_MT_ARGLIST:
        h = jfgp_ff_tree;
        if (mul->type == JFGP_MT_OP)
          jfgp_tree[h].type = JFG_TT_OP;
        else
        if (mul->type == JFGP_MT_UREL)
          jfgp_tree[h].type = JFG_TT_UREL;
        else
        if (mul->type == JFGP_MT_DFUNC)
          jfgp_tree[h].type = JFG_TT_DFUNC;
        else
        if (mul->type == JFGP_MT_IIF)
          jfgp_tree[h].type = JFG_TT_IIF;
        else
        if (mul->type == JFGP_MT_ARGLIST)
          jfgp_tree[h].type = JFG_TT_ARGLIST;
        jfgp_tree[h].op = mul->carg;
        jfgp_tree[h].sarg_1 = jfgp_ff_tree + 1;
        jfgp_ff_tree++;
        res = jfgp_gp_br_ins(atom->adr_1);
        jfgp_tree[h].sarg_2 = jfgp_ff_tree;
        if (res == 0)
          res = jfgp_gp_br_ins(atom->adr_2);
        break;
      case JFGP_MT_HEDGE:
        jfgp_tree[jfgp_ff_tree].type = JFG_TT_HEDGE;
        jfgp_tree[jfgp_ff_tree].op = mul->carg;
        jfgp_tree[jfgp_ff_tree].sarg_1 = jfgp_ff_tree + 1;
        jfgp_ff_tree++;
        res = jfgp_gp_br_ins(atom->adr_1);
        break;
      case JFGP_MT_UFUNC:
        jfgp_tree[jfgp_ff_tree].type = JFG_TT_UFUNC;
        jfgp_tree[jfgp_ff_tree].op = mul->carg;
        jfgp_tree[jfgp_ff_tree].sarg_1 = jfgp_ff_tree + 1;
        jfgp_ff_tree++;
        res = jfgp_gp_br_ins(atom->adr_1);
        break;
      case JFGP_MT_SFUNC:
        jfgp_tree[jfgp_ff_tree].type = JFG_TT_SFUNC;
        jfgp_tree[jfgp_ff_tree].op = mul->carg;
        jfgp_tree[jfgp_ff_tree].sarg_1 = jfgp_ff_tree + 1;
        jfgp_ff_tree++;
        res = jfgp_gp_br_ins(atom->adr_1);
        break;
      case JFGP_MT_ARRAY:
        jfgp_tree[jfgp_ff_tree].type = JFG_TT_ARVAL;
        jfgp_tree[jfgp_ff_tree].op = mul->carg;
        jfgp_tree[jfgp_ff_tree].sarg_1 = jfgp_ff_tree + 1;
        jfgp_ff_tree++;
        res = jfgp_gp_br_ins(atom->adr_1);
        break;
      case JFGP_MT_BETWEEN:
        if (atom->adr_1 != atom->adr_2)
        { jfgp_tree[jfgp_ff_tree].type = JFG_TT_BETWEEN;
          jfgp_tree[jfgp_ff_tree].sarg_1 = mul->sarg_1;
          jfgp_tree[jfgp_ff_tree].sarg_2 = atom->adr_1;
          jfgp_tree[jfgp_ff_tree].op     = atom->adr_2;
        }
        else
        { jfgp_tree[jfgp_ff_tree].type = JFG_TT_FZVAR;
          jfgp_tree[jfgp_ff_tree].sarg_1 = mul->sarg_2
                           + (unsigned short) atom->adr_1;
        }
        jfgp_ff_tree++;
        break;
     }
  }
  return res;
}


static int jfgp_i2p(long ind_no)
{
  /* kopierer et individ til program */
  int m, r, res;
  struct jfg_statement_desc jfgp_sdesc;
  struct jfgp_rule_head_desc *ind_rules;
  const char *argv[2];
  unsigned char *pc;

  argv[0] = jfgp_t_gp;
  res = 0;
  /* deleting statements from last copyed individ. By deleting from the  */
  /* front statements are puled onto there original pc.                  */
  for (m = jfgp_ff_stat - jfgp_no_deletes; m < jfgp_ff_stat; m++)
    jfp_d_statement(jfgp_head.jfr_head, jfgp_stats[m].pc);

  jfgp_no_deletes = 0;
  ind_rules = &(jfgp_rule_heads[ind_no * jfgp_ff_stat]);
  for (r = jfgp_ff_stat - 1; res == 0 && r >= 0; r--)
  { jfgp_ff_tree = 0;
    pc = jfgp_stats[r].pc;
    res = jfgp_gp_br_ins(ind_rules[r].atom_no);
    if (jfgp_ff_tree < JFGP_MAX_TREE)
      jfgp_tree[jfgp_ff_tree].type = JFG_TT_TRUE;
    else
      res = 1;
    if (res == 0)
    { if (jfgp_stats[r].type == 0)   /* if <expr> then <fzvar> */
      { jfgp_sdesc.flags = 0;
        jfgp_sdesc.type = JFG_ST_IF;
        jfgp_sdesc.sec_type = JFG_SST_FZVAR;
        jfgp_sdesc.sarg_1 = jfgp_stats[r].no;
        res = jfp_i_tree(jfgp_head.jfr_head, &pc, &jfgp_sdesc,
                         jfgp_tree, 0, 0, 0, argv, 0);
      }
      else
      if (jfgp_stats[r].type == 1)    /* <var> = <expr> */
      { jfgp_sdesc.flags = 0;
        jfgp_sdesc.type = JFG_ST_IF;
        jfgp_sdesc.sec_type = JFG_SST_VAR;
        jfgp_sdesc.sarg_1 = jfgp_stats[r].no;
        res = jfp_i_tree(jfgp_head.jfr_head, &pc, &jfgp_sdesc,
                         jfgp_tree, jfgp_ff_tree, 0, 0, argv, 0);
      }
      else
      if (jfgp_stats[r].type == 2)    /* case <expr> */
      { jfgp_sdesc.flags = 0;
        jfgp_sdesc.type = JFG_ST_CASE;
        res = jfp_i_tree(jfgp_head.jfr_head, &pc, &jfgp_sdesc,
                         jfgp_tree, 0, 0, 0, argv, 0);
      }
      else
      if (jfgp_stats[r].type == 3)    /* return <expr> */
      { jfgp_sdesc.flags = 0;
        jfgp_sdesc.type = JFG_ST_IF;
        jfgp_sdesc.sec_type = JFG_SST_RETURN;
        res = jfp_i_tree(jfgp_head.jfr_head, &pc, &jfgp_sdesc,
                         jfgp_tree, jfgp_ff_tree, 0, 0, argv, 0);
      }
      if (res != 0)
        return res;
      else
       jfgp_no_deletes++;
    }
  }
  return res;
}

/*************************************************************************/
/* Cros-over funktioner                                                  */
/*************************************************************************/

static signed short jfgp_1_count(long ano)
{
  /* returns no of atoms in (sub)-trae, with ref_c == 1.                  */

  signed short count;
  struct jfgp_atom_desc *atom;
  struct jfgp_mulige_desc *mulig;

  atom = &(jfgp_atoms[ano]);
  mulig = &(jfgp_mulige[atom->mref]);

  if (atom->ref_c == 1)
    count = 1;
  else
    count = 0;
  switch (mulig->type)
  { case JFGP_MT_FZVAR:
    case JFGP_MT_VAR:
    case JFGP_MT_UF_VAR:
    case JFGP_MT_INT:
    case JFGP_MT_FLOAT:
    case JFGP_MT_BETWEEN:
      break;
    case JFGP_MT_UREL:
    case JFGP_MT_DFUNC:
    case JFGP_MT_OP:
    case JFGP_MT_IIF:
    case JFGP_MT_ARGLIST:
      count += jfgp_1_count(atom->adr_1);
      count += jfgp_1_count(atom->adr_2);
      break;
    case JFGP_MT_SFUNC:
    case JFGP_MT_HEDGE:
    case JFGP_MT_ARRAY:
    case JFGP_MT_UFUNC:
      count += jfgp_1_count(atom->adr_1);
      break;
  }
  return count;
}


static long jfgp_1_find(long ano, long iano)
{
  struct jfgp_atom_desc *atom;
  long dno;

  /* finds atom no <iano> in (sub)-tree ano, where only nodes with     */
  /* ref_c == 1 is counted.                                            */
  atom = &(jfgp_atoms[ano]);
  if (atom->ref_c == 1)
    jfgp_count++;

  dno = -1;
  if (iano == jfgp_count)
    return ano;

  switch (jfgp_mulige[atom->mref].type)
  { case JFGP_MT_FZVAR:
    case JFGP_MT_VAR:
    case JFGP_MT_UF_VAR:
    case JFGP_MT_INT:
    case JFGP_MT_FLOAT:
    case JFGP_MT_BETWEEN:
      break;
    case JFGP_MT_UREL:
    case JFGP_MT_DFUNC:
    case JFGP_MT_OP:
    case JFGP_MT_IIF:
    case JFGP_MT_ARGLIST:
      dno = jfgp_1_find(atom->adr_1, iano);
      if (dno < 0)
        dno = jfgp_1_find(atom->adr_2, iano);
      break;
    case JFGP_MT_SFUNC:
    case JFGP_MT_HEDGE:
    case JFGP_MT_UFUNC:
    case JFGP_MT_ARRAY:
      dno = jfgp_1_find(atom->adr_1, iano);
      break;
  }
  return dno;
}

static int jfgp_argc(int mtyp)
{
  /* returns the number of arguments for a given mulige-type */
  /* returns 0 for MT_VAR, -1 for MT_FZVAR etc               */
  int res;

  res = -1;
  if (mtyp == JFGP_MT_OP || mtyp == JFGP_MT_UREL || mtyp == JFGP_MT_DFUNC
      || mtyp == JFGP_MT_IIF || mtyp == JFGP_MT_ARGLIST)
    res = 2;
  else
  if (mtyp == JFGP_MT_HEDGE || mtyp == JFGP_MT_SFUNC
      || mtyp == JFGP_MT_ARRAY || mtyp == JFGP_MT_UFUNC)
    res = 1;
  else
  if (mtyp == JFGP_MT_VAR)
    res = 0;
  else
  if (mtyp == JFGP_MT_UF_VAR)
    res = 0;
  /* else (JFGP_MT_FZVAR, JFGP_MT_BETWEEN, JFGP_MT_INT, JFGP_MT_FLOAT) */
  /*   -1 */

  return res;
}


static int jfgp_scros(long ind_no)
{
  /* mutation by changing of a single atom with ref_c == 1. If no nodes */
  /* with ref_c return 0, else return 1.                                */

  int r, rno, res, i, m, argc, slut;
  long a;
  int retning;
  struct jfgp_atom_desc *atom;
  struct jfgp_mulige_desc *mul;
  struct jfgp_rule_head_desc *rhead;

  res = 0;  /* no change */
  rno = jfgp_random(jfgp_ff_stat);
  rhead = &(jfgp_rule_heads[ind_no * jfgp_ff_stat + rno]);
  r = jfgp_1_count(rhead->atom_no);
  if (r > 0)
  { res = 1;
    a = jfgp_random(r) + 1;
    retning = jfgp_random(2);
    if (retning == 0)
      retning = -1;
    jfgp_count = 0;
    a = jfgp_1_find(rhead->atom_no, a);
    atom = &(jfgp_atoms[a]);
    mul = &(jfgp_mulige[atom->mref]);
    switch (mul->type)
    { case JFGP_MT_FZVAR:
        atom->adr_1 += retning;
        if (atom->adr_1 < 0 || atom->adr_1 >= mul->carg)
          atom->adr_1 = jfgp_random(mul->carg);
        break;
      case JFGP_MT_BETWEEN:
        if (jfgp_random(2) == 0)
        { atom->adr_1 += retning;
          if (atom->adr_1 < 0 || atom->adr_1 > atom->adr_2)
            atom->adr_1 = jfgp_random(atom->adr_2 + 1);
        }
        else
          atom->adr_2 = jfgp_random(mul->carg - atom->adr_1) + atom->adr_1;
        break;
      case JFGP_MT_INT:
        atom->adr_1 += retning;
        if (atom->adr_1 < 0 || atom->adr_1 >= mul->sarg_2)
          atom->adr_1 = jfgp_random(mul->sarg_2);
        break;
      case JFGP_MT_FLOAT:
        atom->adr_1 += retning;
        if (atom->adr_1 < 0 || atom->adr_1 > mul->sarg_1)
          atom->adr_1 = jfgp_random(mul->sarg_1 + 1);
        break;
      case JFGP_MT_VAR:
      case JFGP_MT_UF_VAR:
      case JFGP_MT_IIF:
      case JFGP_MT_ARGLIST:
        res = 0; /* no change */
        break;
      default:
        argc = jfgp_argc(mul->type);
        r = jfgp_random(jfgp_ff_mt) + 1;
        m = 0; i = 0; slut = 0;
        while (slut == 0)
        { if (jfgp_argc(jfgp_mulige[m].type) == argc)
            i++;
          if (i == r)
          { atom->mref = m;
            slut = 1;
          }
          m++;
          if (m >= jfgp_ff_mt)
            m = 0;
        }
        break;
     }
  }
  if (res == 1)
  { jfgp_stat.ind_size = jfgp_inds[ind_no].tsize;
    jfgp_stat.old_score = jfgp_inds[ind_no].score;
    jfgp_stat.sum_score -= jfgp_inds[ind_no].score;
    jfgp_i_judge(ind_no);
    if (jfgp_inds[ind_no].status == JFGP_IS_ALIVE)
      jfgp_stat.sum_score += jfgp_inds[ind_no].score;
  }
  return res;
}

/*************************************************************************/
/* mutation funktioner                                                   */
/*************************************************************************/


static signed short jfgp_ir_change(long ano, signed short value)
{
  /* changes ref-count for the (sub)-tree starting in ano with value */
  /* (value is -1 or 1). Return number of nodes in tree.             */

  signed short count;
  struct jfgp_atom_desc *atom;

  atom = &(jfgp_atoms[ano]);
  switch (jfgp_mulige[atom->mref].type)
  { case JFGP_MT_FZVAR:
    case JFGP_MT_VAR:
    case JFGP_MT_UF_VAR:
    case JFGP_MT_INT:
    case JFGP_MT_FLOAT:
    case JFGP_MT_BETWEEN:
      count = 1;
      break;
    case JFGP_MT_UREL:
    case JFGP_MT_DFUNC:
    case JFGP_MT_OP:
    case JFGP_MT_IIF:
    case JFGP_MT_ARGLIST:
      count = 1;
      count += jfgp_ir_change(atom->adr_1, value);
      count += jfgp_ir_change(atom->adr_2, value);
      break;
    case JFGP_MT_SFUNC:
    case JFGP_MT_HEDGE:
    case JFGP_MT_ARRAY:
    case JFGP_MT_UFUNC:
      count = 1;
      count += jfgp_ir_change(atom->adr_1, value);
      break;
  }
  atom->ref_c += value;
  if (atom->ref_c == 0)
  { atom->mref = -1;
    atom->adr_1 = jfgp_head.free_list;
    jfgp_head.free_list = ano;
    jfgp_head.free_c++;
  }
  return count;
}


static signed short jfgp_rule_change(signed short hno, signed short value)
{
  struct jfgp_rule_head_desc *rhead;
  signed short count;

  /* changes ref_c for rule-header no <hno> with <value> (1 or -1), and */
  /* calculates and returns number of atoms in rule-head.               */

  rhead = &(jfgp_rule_heads[hno]);
  count = jfgp_ir_change(rhead->atom_no, value);
  rhead->rsize = count;
  return count;
}

static long jfgp_i_rrha(long ano, long iano)
{
  /* hjaelpefunktion til jfgp_rrha (bruger globale var jfgp_count) */

  struct jfgp_atom_desc *atom;
  long dno;

  dno = -1;
  jfgp_count++;

  atom = &(jfgp_atoms[ano]);

  if (ano < 0 || ano >= jfgp_head.atom_c || atom->mref > 10 ||
      jfgp_mulige[atom->mref].type > 10)
    dno = -1;
  if (iano == jfgp_count)
  { if (jfgp_mulige[atom->mref].type == JFGP_MT_ARGLIST)
    { dno = atom->adr_1;
      return dno;
    }
    else
      return ano;
  }

  switch (jfgp_mulige[atom->mref].type)
  { case JFGP_MT_FZVAR:
    case JFGP_MT_VAR:
    case JFGP_MT_UF_VAR:
    case JFGP_MT_INT:
    case JFGP_MT_FLOAT:
    case JFGP_MT_BETWEEN:
      break;
    case JFGP_MT_UREL:
    case JFGP_MT_DFUNC:
    case JFGP_MT_OP:
    case JFGP_MT_IIF:
    case JFGP_MT_ARGLIST:
      dno = jfgp_i_rrha(atom->adr_1, iano);
      if (dno < 0)
        dno = jfgp_i_rrha(atom->adr_2, iano);
      break;
    case JFGP_MT_SFUNC:
    case JFGP_MT_ARRAY:
    case JFGP_MT_HEDGE:
    case JFGP_MT_UFUNC:
      dno = jfgp_i_rrha(atom->adr_1, iano);
      break;
  }
  return dno;
}

static long jfgp_rrha_atom(long rhno)
{
  /* returnerer atom-no paa et tilfaeldigt atom i reglen rhno */

  struct jfgp_rule_head_desc *rhead;
  long ano, iano;

  rhead = &(jfgp_rule_heads[rhno]);
  iano = jfgp_random(rhead->rsize) + 1;
  jfgp_count = 0;
  ano = jfgp_i_rrha(rhead->atom_no, iano);
  return ano;
}


static long jfgp_mutate(unsigned short stat_no,
                                long sano_1,
                                long sano_2,
                                long sano_3)
{
  /* creates and returns a new atom. The atom is created randomly from   */
  /* mulige-types. If the type has to reference other atoms then sano_1, */
  /* sano_2, sano_3 is used.                                             */

  long m, dno, sno1, sno2, sno3, nno, argno;
  struct jfgp_atom_desc *atom;
  struct jfgp_mulige_desc *mul;
  int isize, x;

  sno1 = sano_1;
  sno2 = sano_2;
  sno3 = sano_3;
  if (jfgp_head.free_c <= 10)
    return -1;
  dno = jfgp_head.free_list;
  jfgp_head.free_list = jfgp_atoms[dno].adr_1;
  jfgp_head.free_c--;

  isize = jfgp_random(2);
  x = 0;
  atom = &(jfgp_atoms[dno]);
  do
  {
    m = jfgp_random(jfgp_stats[stat_no].mulige_c)
         + jfgp_stats[stat_no].f_mulige;
    mul = &(jfgp_mulige[m]);
    x++;
  }
  while (x < 100 && isize > 0  && (mul->type == JFGP_MT_FZVAR
                                   || mul->type == JFGP_MT_BETWEEN
                                   || mul->type == JFGP_MT_VAR
                                   || mul->type == JFGP_MT_UF_VAR
                                   || mul->type == JFGP_MT_INT
                                   || mul->type == JFGP_MT_FLOAT));
  switch (mul->type)
  { case JFGP_MT_FZVAR:
      atom->adr_1 = jfgp_random(mul->carg);
      break;
    case JFGP_MT_VAR:
    case JFGP_MT_UF_VAR:
      break;
    case JFGP_MT_INT:
      atom->adr_1 = jfgp_random(mul->sarg_2);
      break;
    case JFGP_MT_FLOAT:
      atom->adr_1 = jfgp_random(mul->sarg_1 + 1);
      break;
    case JFGP_MT_OP:
    case JFGP_MT_UREL:
    case JFGP_MT_DFUNC:
      atom->adr_1 = sno1;
      atom->adr_2 = sno2;
      break;
    case JFGP_MT_IIF:
      atom->adr_1 = sno1;
      nno = jfgp_head.free_list;
      atom->adr_2 = nno;
      jfgp_head.free_list = jfgp_atoms[nno].adr_1;
      jfgp_head.free_c--;
      jfgp_atoms[nno].mref = jfgp_ff_mt;  /* ARGLIST */
      jfgp_atoms[nno].adr_1 = sno2;
      jfgp_atoms[nno].adr_2 = sno3;
      jfgp_atoms[nno].ref_c = 0;
      break;
    case JFGP_MT_UFUNC:
      if (mul->sarg_1 == 1)
        atom->adr_1 = sno1;
      else
      { atom->adr_1 = jfgp_head.free_list;
        for (argno = 0; argno < mul->sarg_1; argno++)
        { nno = jfgp_head.free_list;
          jfgp_head.free_list = jfgp_atoms[nno].adr_1;
          jfgp_head.free_c--;
          jfgp_atoms[nno].mref = jfgp_ff_mt;  /* ARGLIST */
          jfgp_atoms[nno].adr_1 = sno1;
          if (argno + 2 == mul->sarg_1)
          { jfgp_atoms[nno].adr_2 = sno2;
            argno++;
          }
          else
            jfgp_atoms[nno].adr_2 = jfgp_head.free_list;
          jfgp_atoms[nno].ref_c = 0;
        }
      }
      break;
    case JFGP_MT_HEDGE:
    case JFGP_MT_SFUNC:
    case JFGP_MT_ARRAY:
      atom->adr_1 = sno1;
      break;
    case JFGP_MT_BETWEEN:
      atom->adr_1 = jfgp_random(mul->carg);
      atom->adr_2 = atom->adr_1
                    + jfgp_random(mul->carg - atom->adr_1);
      break;
  }
  atom->mref = m;
  atom->ref_c = 0;
  return dno;
}

static int jfgp_uc_argc(int mno)
{
  /* help-function to jfgp_u_crosover.                       */
  /* returns the number of arguments for a given mulige.     */
  /* returns 0 for types uniform_cros cannot handle (iif,    */
  /* user-definde-functions).                                */

  int res, mtyp;

  mtyp = jfgp_mulige[mno].type;
  res = 0;
  if (mtyp == JFGP_MT_OP || mtyp == JFGP_MT_UREL || mtyp == JFGP_MT_DFUNC)
    res = 2;
  else
  if (mtyp == JFGP_MT_HEDGE || mtyp == JFGP_MT_SFUNC || mtyp == JFGP_MT_ARRAY)
    res = 1;
  return res;
}

static long jfgp_u_crosover(long sano_1,  long sano_2)
{
  /* uniform crosover between sano_1 and sano_2. Returns new atom. */

  long ac1, ac2, dno;
  struct jfgp_atom_desc *atom;
  struct jfgp_atom_desc *s_atom;
  struct jfgp_mulige_desc *mul;

  ac1 = jfgp_uc_argc(jfgp_atoms[sano_1].mref);
  ac2 = jfgp_uc_argc(jfgp_atoms[sano_2].mref);
  if (ac1 == 0 || ac1 != ac2)
  { if (jfgp_random(2) == 0)
      return sano_1;
    else
      return sano_2;
  }
  else
  { if (jfgp_head.free_c < 10)
      return -1;
    dno = jfgp_head.free_list;
    jfgp_head.free_list = jfgp_atoms[dno].adr_1;
    jfgp_head.free_c--;

    atom = &(jfgp_atoms[dno]);
    if (jfgp_random(2) == 0)
      s_atom = &(jfgp_atoms[sano_1]);
    else
      s_atom = &(jfgp_atoms[sano_2]);
    mul = &(jfgp_mulige[s_atom->mref]);
    atom->mref = s_atom->mref;
    atom->ref_c = 0;
    switch (mul->type)
    { case JFGP_MT_OP:
      case JFGP_MT_UREL:
      case JFGP_MT_DFUNC:
        atom->adr_1 = jfgp_u_crosover(jfgp_atoms[sano_1].adr_1,
                                      jfgp_atoms[sano_2].adr_1);
        if (atom->adr_1 != -1)
          atom->adr_2 = jfgp_u_crosover(jfgp_atoms[sano_1].adr_2,
                                        jfgp_atoms[sano_2].adr_2);
        if (atom->adr_1 == -1 || atom->adr_2 == -1)
        { /* error. free reserved node: */
          atom->mref = -1;
          atom->adr_1 = jfgp_head.free_list;
          jfgp_head.free_list = dno;
          jfgp_head.free_c++;
          dno = -1;
        }
        break;
      case JFGP_MT_HEDGE:
      case JFGP_MT_SFUNC:
      case JFGP_MT_ARRAY:
        atom->adr_1 = jfgp_u_crosover(jfgp_atoms[sano_1].adr_1,
                                      jfgp_atoms[sano_2].adr_1);
        if (atom->adr_1 == -1)
        { /* error. free reserved node: */
          atom->mref = -1;
          atom->adr_1 = jfgp_head.free_list;
          jfgp_head.free_list = dno;
          jfgp_head.free_c++;
          dno = -1;
        }
        break;
      default:
        dno = -1;
        break;
    }
  }
  return dno;
}

static void jfgp_aterminal(int rno, long atom_no)
{
  /* selects a random terninam for rule number <rno> and places it in    */
  /* <atom_no>.                                                          */

  int r, m, i, t, id;
  struct jfgp_mulige_desc *mul;
  struct jfgp_atom_desc *atom;

  r = jfgp_random(jfgp_stats[rno].terminal_c);
  m = 0;
  for (i = 0; 1 == 1; i++)
  { t = jfgp_mulige[i + jfgp_stats[rno].f_mulige].type;
    if (t == JFGP_MT_FZVAR || t == JFGP_MT_VAR || t == JFGP_MT_INT
        || t == JFGP_MT_FLOAT || t == JFGP_MT_BETWEEN
        || t == JFGP_MT_UF_VAR)
    { if (m == r)
      { id = i + jfgp_stats[rno].f_mulige;
        break;
      }
      m++;
      if (i >= jfgp_ff_mt)
        return ;
    }
  }
  mul = &(jfgp_mulige[id]);
  atom = &(jfgp_atoms[atom_no]);
  switch (mul->type)
  { case JFGP_MT_FZVAR:
      atom->adr_1 = jfgp_random(mul->carg);
      break;
    case JFGP_MT_VAR:
    case JFGP_MT_UF_VAR:
      break;
    case JFGP_MT_INT:
      atom->adr_1 = jfgp_random(mul->sarg_2);
      break;
    case JFGP_MT_FLOAT:
      atom->adr_1 = jfgp_random(mul->sarg_1 + 1);
      break;
    case JFGP_MT_BETWEEN:
      atom->adr_1 = jfgp_random(mul->carg);
      atom->adr_2 = atom->adr_1
                    + jfgp_random(mul->carg - atom->adr_1);
      break;
  }
  atom->mref = id;
}

static void jfgp_ind_rm(signed short sno)
{
  /* removes individ number <sno>.                */

  int m;

  if (jfgp_inds[sno].status == JFGP_IS_ALIVE)
  { for (m = 0; m < jfgp_ff_stat; m++)
    { jfgp_rule_change(sno * jfgp_ff_stat + m, -1);
      jfgp_rule_heads[sno * jfgp_ff_stat + m].rsize = 0;
    }
    jfgp_inds[sno].status = JFGP_IS_DEAD;
    jfgp_stat.sum_score -= jfgp_inds[sno].score;
    jfgp_inds[sno].tsize = 0;
    jfgp_inds[sno].score = 0.0;
    jfgp_head.alive_c--;
    jfgp_stat.alive_c = jfgp_head.alive_c;
    jfgp_stat.free_c = jfgp_head.free_c;
  }
}


static signed short jfgp_ind_create(signed short ind_no)
{
  /* creates a new individual. <ind_no> is a dead individual */

  int r, ok, s1_ind_no, s2_ind_no, s3_ind_no,
      one_replace, this_replace;
  long m, ano1, ano2, ano3, dno;
  struct jfgp_rule_head_desc *gp1_ind_rules;
  struct jfgp_rule_head_desc *gp2_ind_rules;
  struct jfgp_rule_head_desc *gp3_ind_rules;
  struct jfgp_rule_head_desc *gpd_ind_rules;
  struct jfgp_ind_desc *child;

  child = &(jfgp_inds[ind_no]);
  jfgp_stat.old_score = child->score;

  /* select parents: */
  s1_ind_no = jfgp_ind_rand(1);
  s2_ind_no = jfgp_ind_rand(1);
  s3_ind_no = jfgp_ind_rand(1);

  gp1_ind_rules = &(jfgp_rule_heads[s1_ind_no * jfgp_ff_stat]);
  gp2_ind_rules = &(jfgp_rule_heads[s2_ind_no * jfgp_ff_stat]);
  gp3_ind_rules = &(jfgp_rule_heads[s3_ind_no * jfgp_ff_stat]);
  gpd_ind_rules = &(jfgp_rule_heads[ind_no * jfgp_ff_stat]);

  /* choose between one-replace and multi-replace: */
  if (jfgp_random(3) == 0 && jfgp_ff_stat > 1)
  { one_replace = 1;
    this_replace = jfgp_random(jfgp_ff_stat);
  }
  else
    one_replace = 0;

  for (m = 0; m < jfgp_ff_stat; m++)
  { ok = 0;
    while (ok == 0)
    { ok = 1;
      r = jfgp_random(8);
      if (jfgp_ff_stat == 1 && r < 2)
        r = 5;
      if (one_replace == 1 && m != this_replace)
        r = 0;
      if ((r >= 4)
          && gp1_ind_rules[m].rsize + gp2_ind_rules[m].rsize + 5
             > JFGP_MAX_TREE)
        r = r - 3;
      switch (r)
      { case 0:  /* dest-rule = ind-1-rule */
          gpd_ind_rules[m].atom_no = gp1_ind_rules[m].atom_no;
          break;
        case 1: /* dest-rule = ind-2-rule */
          gpd_ind_rules[m].atom_no = gp2_ind_rules[m].atom_no;
          break;
        case 2:  /* dest-rule = atom in ind-1-rule */
          gpd_ind_rules[m].atom_no
          = jfgp_rrha_atom(s1_ind_no * jfgp_ff_stat + m);
          break;
        case 3:  /* dest-rule = atom in ind-2 rule */
          gpd_ind_rules[m].atom_no
            = jfgp_rrha_atom(s2_ind_no * jfgp_ff_stat + m);
          break;
        case 4:    /* dest-rule = mutate(atom in ind-1, atom in ind-2,  */
                   /*                    atom in ind-3)                 */
          ano1 = jfgp_rrha_atom(s1_ind_no * jfgp_ff_stat + m);
          ano2 = jfgp_rrha_atom(s2_ind_no * jfgp_ff_stat + m);
          ano3 = jfgp_rrha_atom(s3_ind_no * jfgp_ff_stat + m);
          dno  = jfgp_mutate(m, ano1, ano2, ano3);
          if (dno == -1)
            ok = 0;
          else
            gpd_ind_rules[m].atom_no = dno;
          break;
        case 5:    /* uniform crosover */
          ano1 = gp1_ind_rules[m].atom_no;
          ano2 = gp2_ind_rules[m].atom_no;
          dno = jfgp_u_crosover(ano1, ano2);
          if (dno == -1)
            ok = 0;
          else
            gpd_ind_rules[m].atom_no = dno;
          break;
        case 6:    /* dest-rule = mutate(ind-1-rule, ind-2-rule, ind-3-rule */
          ano1 = gp1_ind_rules[m].atom_no;
          ano2 = gp2_ind_rules[m].atom_no;
          ano3 = gp3_ind_rules[m].atom_no;
          dno = jfgp_mutate(m, ano1, ano2, ano3);
          if (dno == -1)
            ok = 0;
          else
            gpd_ind_rules[m].atom_no = dno;
          break;
        case 7:   /* dest-rule = mutate(ind-1-rule, atom in ind-2, ind-3-rule*/
          ano1 = gp1_ind_rules[m].atom_no;
          ano2 = jfgp_rrha_atom(s2_ind_no * jfgp_ff_stat + m);
          ano3 = gp3_ind_rules[m].atom_no;
          dno = jfgp_mutate(m, ano1, ano2, ano3);
          if (dno == -1)
            ok = 0;
          else
            gpd_ind_rules[m].atom_no = dno;
          break;
        default:
          ok = 0;
          break;
      }
    }
    child->tsize += jfgp_rule_change(ind_no * jfgp_ff_stat + m, 1);
  }
  child->status = JFGP_IS_ALIVE;
  jfgp_head.alive_c++;

  jfgp_stat.ind_size = child->tsize;
  jfgp_stat.free_c = jfgp_head.free_c;
  jfgp_stat.alive_c = jfgp_head.alive_c;
  jfgp_i_judge(ind_no);
  if (jfgp_inds[ind_no].status == JFGP_IS_ALIVE)
    jfgp_stat.sum_score += child->score;
  return ind_no;
}

static void jfgp_re_judge(void)
{
  int i;

  jfgp_stat.sum_score = 0.0;
  for (i = 0; i < jfgp_head.ind_c; i++)
  { if (jfgp_inds[i].status == JFGP_IS_ALIVE)
    { jfgp_i_judge(i);
      if (jfgp_inds[i].status == JFGP_IS_ALIVE)
        jfgp_stat.sum_score += jfgp_inds[i].score;
    }
  }
}

/****************************************************************************/
/* Hjaelpe-funktioner til jfgp_init:                                        */



static int jfgp_pop_create(void)
{
  /* Create the initial population */
  int m, r, rh, worst_id;
  long ff_atom;
  int x, d_ind_no, s1_ind_no, s2_ind_no, s3_ind_no, ok;
  long ano1, ano2, ano3, dno;

  struct jfgp_rule_head_desc *gp1_ind_rules;
  struct jfgp_rule_head_desc *gp2_ind_rules;
  struct jfgp_rule_head_desc *gp3_ind_rules;
  struct jfgp_rule_head_desc *gpd_ind_rules;

  ff_atom = 0;
  jfgp_head.alive_c = 0;
  jfgp_stat.sum_score = 0.0;
  for (m = 0;  m < jfgp_head.ind_c; m++)
  { jfgp_inds[m].score = 0.0;
    rh = m * jfgp_ff_stat;
    if (ff_atom + jfgp_ff_stat + 10 > jfgp_head.atom_c)
    { jfgp_inds[m].status = JFGP_IS_DEAD;
    }
    else
    { for (r = 0; r < jfgp_ff_stat; r++)
      { /* for each rulehead create a rule containing a single terminal */
        jfgp_rule_heads[rh].atom_no = ff_atom;
        jfgp_rule_heads[rh].rsize = 1;
        jfgp_atoms[ff_atom].ref_c = 1;
        jfgp_aterminal(r, ff_atom);
        ff_atom++;
        rh++;
      }
      jfgp_inds[m].tsize = jfgp_ff_stat;
      jfgp_inds[m].status = JFGP_IS_ALIVE;
      jfgp_stat.ind_size = jfgp_inds[m].tsize;
      jfgp_head.alive_c++;
      jfgp_stat.alive_c = jfgp_head.alive_c;
      jfgp_stat.free_c = jfgp_head.atom_c - ff_atom;
    }
  }
  /* The rest of the atoms are placed in the free list    */
  jfgp_head.free_c = 0;
  jfgp_head.free_list = ff_atom;
  for (m = ff_atom; m < jfgp_head.atom_c; m++)
  { jfgp_atoms[m].mref  = -1;    /* empty */
    jfgp_atoms[m].ref_c  = 0;
    jfgp_atoms[m].adr_1 = m + 1;
    jfgp_atoms[m].adr_2 = 0;
    jfgp_head.free_c++;
  }

  /* Create random mutations of the individuals  */
  ok = 1;
  for (x = 0; ok == 1 && x < jfgp_head.ind_c * 10; x++)
  { d_ind_no = jfgp_ind_rand(-1);
    jfgp_ind_rm(d_ind_no);
    s1_ind_no = jfgp_ind_rand(1);
    s2_ind_no = jfgp_ind_rand(1);
    s3_ind_no = jfgp_ind_rand(1);
    gp1_ind_rules = &(jfgp_rule_heads[s1_ind_no * jfgp_ff_stat]);
    gp2_ind_rules = &(jfgp_rule_heads[s2_ind_no * jfgp_ff_stat]);
    gp3_ind_rules = &(jfgp_rule_heads[s3_ind_no * jfgp_ff_stat]);
    gpd_ind_rules = &(jfgp_rule_heads[d_ind_no * jfgp_ff_stat]);
    for (m = 0; ok == 1 && m < jfgp_ff_stat; m++)
    { ano1 = gp1_ind_rules[m].atom_no;
      ano2 = gp2_ind_rules[m].atom_no;
      ano3 = gp3_ind_rules[m].atom_no;
      dno = jfgp_mutate(m, ano1, ano2, ano3);
      if (dno == -1)
        ok = 0;
      else
      { gpd_ind_rules[m].atom_no = dno;
        jfgp_inds[d_ind_no].tsize += jfgp_rule_change(d_ind_no * jfgp_ff_stat + m, 1);
      }
    }
    if (ok == 1)
    { jfgp_inds[d_ind_no].status = JFGP_IS_ALIVE;
      jfgp_head.alive_c++;
    }
  }

  jfgp_stat.free_c = jfgp_head.free_c;
  jfgp_stat.alive_c = jfgp_head.alive_c;


  /* judge the individuals */
  for (m = 0; m < jfgp_head.ind_c; m++)
  { if (jfgp_inds[m].status == JFGP_IS_ALIVE)
    { jfgp_stat.ind_size = jfgp_inds[m].tsize;
      jfgp_stat.old_score = 0.0;
      jfgp_i_judge(m);
      jfgp_stat.sum_score += jfgp_inds[m].score;
      if (m == 0)
        worst_id = m;
      else
      { if (jfgp_head.compare(jfgp_inds[m].score, 1,
                              jfgp_inds[worst_id].score, 1) == -1)
          worst_id = m;
      }
    }
  }
  /* the score of dead individs are set equal the worst score */
  for (m = 0; m < jfgp_head.ind_c; m++)
  { if (jfgp_inds[m].status == JFGP_IS_DEAD)
      jfgp_inds[m].score = jfgp_inds[worst_id].score;
  }
  return 0;
}

/************************************************************************/
/* externe funktioner:                                                  */

int jfgp_run(float (* judge)(void),
             int (*compare)(float score_1, int size_1,
                            float score_2, int size_2),
             int tournament_size)
{
  int m, res, ic_type, ino;

  jfgp_no_deletes = 0;
  if (tournament_size > 0)
    jfgp_tournament_size = tournament_size;
  jfgp_head.judge = judge;
  jfgp_head.compare = compare;
  if ((m = jfgp_pop_create()) != 0)
    return m;

  jfgp_slut = 0;
  while (jfgp_slut == 0)
  { ic_type = JFGP_IT_REPLACE;
    if (   ((float) jfgp_head.free_c) / ((float) jfgp_head.atom_c) < 0.25
        && ((float) jfgp_head.alive_c) / ((float) jfgp_head.ind_c) > 0.5)
      ic_type = JFGP_IT_DELETE;
    if (   ((float) jfgp_head.free_c) / ((float) jfgp_head.atom_c) >= 0.25
        && jfgp_head.alive_c < jfgp_head.ind_c)
      ic_type = JFGP_IT_INSERT;
    switch (ic_type)
    { case JFGP_IT_REPLACE:
        res = 0;
        if (jfgp_random(4) == 0)
        { ino = jfgp_ind_rand(-1);
          res = jfgp_scros(ino);
        }
        if (res == 0)
        { ino = jfgp_ind_rand(-1);
          jfgp_ind_rm(ino);
          jfgp_ind_create(ino);
        }
        break;
      case JFGP_IT_DELETE:
        ino = jfgp_ind_rand(-1);
        jfgp_ind_rm(ino);
        break;
      case JFGP_IT_INSERT:
        for (ino = 0; ino < jfgp_head.ind_c; ino++)
        { if (jfgp_inds[ino].status == JFGP_IS_DEAD)
          { jfgp_ind_create(ino);
            break;
          }
        }
        break;
    }
  }

  /* Find best individ and copy it to program: */
  ino = -1;
  for (m = 0; m < jfgp_head.ind_c; m++)
  { if (jfgp_inds[m].status == JFGP_IS_ALIVE)
    { if (ino== -1)
        ino = m;
      else
      { if (jfgp_head.compare(jfgp_inds[m].score, jfgp_inds[m].tsize,
                              jfgp_inds[ino].score, jfgp_inds[ino].tsize)
            == 1)
          ino = m;
      }
    }
  }
  jfgp_i2p(ino);
  return 0;
}


void jfgp_stop(void)
{
  jfgp_slut = 1;
}

int jfgp_init(void *jfr_head, long atom_c, int ind_c)
{
  long size;
  int  res;

  jfgp_atoms      = NULL;
  jfgp_inds       = NULL;
  jfgp_rule_heads = NULL;

  jfgp_head.ind_c    = ind_c;
  jfgp_head.atom_c   = atom_c;
  jfgp_head.free_c   = 0;
  jfgp_head.jfr_head = jfr_head;

  jfg_sprg(&jfgp_spdesc, jfr_head);

  if ((res = jfgp_adesc_create()) != 0)
    return res;

  jfgp_no_deletes = 0;

  /* Memory alloc of DAG:                                                */
  size = jfgp_head.atom_c * sizeof(struct jfgp_atom_desc);
  if ((jfgp_atoms = (struct jfgp_atom_desc *) malloc(size)) == NULL)
    return 6;

 /* Memory alloc of rule-heads:                                         */
  size = jfgp_ff_stat * jfgp_head.ind_c * sizeof(struct jfgp_rule_head_desc);
  if ((jfgp_rule_heads
       = (struct jfgp_rule_head_desc *) malloc(size)) == NULL)
  { free(jfgp_atoms);
    return 6;
  }

  /* Memory alloc of individs:                                        */
  size = jfgp_head.ind_c * sizeof(struct jfgp_ind_desc);
  if ((jfgp_inds = (struct jfgp_ind_desc *) malloc(size)) == NULL)
  { free(jfgp_rule_heads);
    free(jfgp_atoms);
    return 6;
  }
  return 0;
}

void jfgp_free(void)
{
  if (jfgp_inds != NULL)
    free(jfgp_inds);
  if (jfgp_rule_heads != NULL)
    free(jfgp_rule_heads);
  if (jfgp_atoms != NULL)
    free(jfgp_atoms);
}

