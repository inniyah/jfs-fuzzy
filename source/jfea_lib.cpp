  /***********************************************************************/
  /*                                                                     */
  /* jfea_lib.cpp  Version  2.02 Copyright (c) 1999-2000 Jan E. Mortensen*/
  /*                                                                     */
  /* JFS rule creator using evolutionary programing.                     */
  /*                                                                     */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                  */
  /*    Lollandsvej 35 3.tv.                                             */
  /*    DK-2000 Frederiksberg                                            */
  /*    Denmark                                                          */
  /*                                                                     */
  /***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfp_lib.h"
#include "jfea_lib.h"

/************************************************************************/
/* Beskrivelse af individerne                                           */
/************************************************************************/

struct jfea_stat_rec jfea_stat;

struct jfea_atom_desc
{
  unsigned char atype;     /* atom-type, see below.  */
  unsigned char aart;      /* use depends on atom-type, see below. */
  unsigned char val_1;     /* -"- */
  unsigned char val_2;     /* -"- */
};
#define JFEA_ATOM_SIZE sizeof(struct jfea_atom_desc)
/* where <atype> is one of: */
#define JFEA_AT_NONE         1  /* no expression (or no rule).              */
#define JFEA_AT_ADJECTIVE    2  /* expr = <var> is <adj>,                   */
                                /* where rel-adj = <val_1>.                 */
#define JFEA_AT_BETWEEN      3  /* expr = <var> between <a1> and <a2>,      */
                                /* rel-<a1> in <val_1> rel-<a2> in <val_2>. */
#define JFEA_AT_H_ADJECTIVE  4  /* expr = <var> <hedge> <adj>, where        */
                                /* <hedge> = <aart> and rel-<adj> = <val_1>.*/
#define JFEA_AT_H_BETWEEN    5  /* expr: <var> <hedge> between <a1> and <a2>*/
                                /* where <aart>=<hedge>, <val_1>=rel-<a1>   */
                                /* and <val_2>=rel<a2>.                     */
#define JFEA_AT_RELATION     6  /* expr: <var> <rel> <val>, where           */
                                /* <rel> = jea_rtab[<aart>], <val> =        */
                                /* relvalue(<val_1>).                       */
#define JFEA_AT_VAL_BETWEEN  7  /* expr: <var> <rel> <val_1> and            */
                                /*       <var> <rel> <val_2>, where         */
                                /* <rel> = jfea_rtab[<aart>].               */
#define JFEA_AT_INCDEC       8  /* expr: <inc/dec> <var> with <val>         */
                                /* <inc/dec>=decreas if <aart>=1, increase  */
                                /* else. <val> = relvalue(<val_1>).         */
#define JFEA_AT_IN           9  /* expr: <var> in (<a1>, <a2>,...), where   */
                                /* where var.a1 is in () if byte 0 of       */
                                /* <aart><val_1><val_2> is set and so on.   */
#define JFEA_AT_F_RELATION  10  /* expr: <var> <frel> <val>, where          */
                                /* <rel> = fuzzy-rels[<aart>], <val> =      */
                                /* relvalue(<val_1>).                       */
#define JFEA_AT_CMP         11  /* expr: <var> <rel> <var2>, where          */
                                /* <rel> = jfea_rtab[<aart>],               */
                                /* <var2>= jfea_a_descs[<val_1>].var_no.    */
#define JFEA_AT_F_CMP       12  /* expr <var> <frel> <var2>, where          */
                                /* <<frel> = fuzzy-rels[<aart>], <var2> =   */
                                /* jfea_a_descs[<val_1>].var_no.            */
#define JFEA_AT_OP          13  /* expr: <var><op><var2><rel><val>,  where  */
                                /* <aart>=16*<op>+<rel>, <val_1>=<var_2>,   */
                                /* <val> = relvalue(<val_2>).               */
#define JFEA_AT_DCMP        14  /* expr: <var> <rel> <var2>, where          */
                                /* <rel> = jfea_rtab[<aart>],               */
                                /* <var2>= jfea_a_descs[<val_1>].var_no.    */
#define JFEA_AT_F_DCMP      15  /* expr <var> <frel> <var2>, where          */
                                /* <<frel> = fuzzy-rels[<aart>], <var2> =   */
                                /* jfea_a_descs[<val_1>].var_no.            */

static struct jfea_atom_desc *jfea_inds  = NULL;

static long   jfea_ind_c = 40;    /* number of individuals.              */
static int    jfea_atom_c;        /* no of atoms pr individ.             */
static int    jfea_rule_c;         /* number of rules pr individual.     */
static int    jfea_rvar_c;        /* number of variables pr rule.        */
/*                     (jfea_atom_c = jfea_rule_c * jfea_rvar_c)         */
static int    jfea_slut;

static long jfea_cur_ind_no;  /* used by jfea_protect(),   */

struct jfea_score_desc
{ float        score;
  signed short ind_no;
  signed short iprotect;
};

#define JFI_SCORE_SIZE sizeof(struct jfea_score_desc)


struct jfea_pop_desc { struct jfea_score_desc *scores;
                      long  repro_c;
                      float c_q;       /* constants to rank-selection */
                      float c_r;       /* constants to rank-selection */
                      void *jfr_head;
                      int maximize;
                    };

static float (*jfea_judge)(void);

static struct jfea_pop_desc jfea_pop;  /* The population               */

static struct jfg_sprog_desc jfea_spdesc;

struct jfea_rm_ind_desc  /* remember info for repat-mutation */
{
  signed short ind_no;
  signed short scores_id;
  int rno;
  int vno;
  int succes;
  int direction;
};
static jfea_rm_ind_desc jfea_rm_ind;

/*********************************************************************/
/* Individual-description:                                           */
/*********************************************************************/


struct jfea_atomd_desc  /* describes an input/output variable.         */
                        /* used to convert an individual-rule of atoms */
                        /* to a jfs-statement, and to limit mutations. */
{ unsigned short fzvar_no;
  unsigned char  var_no;
  unsigned char  fzvar_c;
  unsigned char  domain_no;
  unsigned char  domain_c;       /* number of variables in with        */
                                 /* domain=<domaion_no>.               */
  unsigned char  steps;
  unsigned char  atypes_c;       /* no of l_atypes */
  unsigned char  l_atypes[16];   /* legal atypes   */
  float min_value;
  float max_value;
};
#define JFEA_MAX_VARS 64
static struct jfea_atomd_desc jfea_a_descs[JFEA_MAX_VARS];
/* jfea_a_descs[0] describes the output-variable,           */
/* jfea_a_descs[1] describes the first input-var and so on. */

#define JFEA_MAX_WORDS 128
static char *jfea_words[JFEA_MAX_WORDS];

static unsigned char *jfea_program_id;
static int jfea_delete_c;

static int jfea_fixed_rules = 0;
static int jfea_no_default  = 0; /* dont create rules with then-adjectiv  */
                                 /* == default adjectiv.                  */
static int jfea_default_no  = 0;

static unsigned char jfea_bit_tabel[] =
{
 1, 2, 4, 8, 16, 32, 64, 128
};

static unsigned char jfea_rel_tabel[] =
{
  JFS_ROP_EQ,    /* 0 */
  JFS_ROP_NEQ,   /* 1 */
  JFS_ROP_GT,    /* 2 */
  JFS_ROP_GEQ,   /* 3 */
  JFS_ROP_LT,    /* 4 */
  JFS_ROP_LEQ    /* 5 */
};

/*************************************************************************/
/* Diverse                                                               */
/*************************************************************************/

static struct jfg_tree_desc *jfea_tree;

struct jfea_error_rec jfea_error_desc;

static float jfea_jraseed; /* seed til intern random number generator */
static char jfea_empty[] = " ";

static int jfea_set_error(int no, char *etxt);
static unsigned char jfea_cmin(unsigned char a, unsigned char b);
static unsigned char jfea_cmax(unsigned char a, unsigned char b);
static int jfea_get_bit(struct jfea_atom_desc *atom, int bitno);
static void jfea_set_bit(struct jfea_atom_desc *atom, int bitno);
static void jfea_atom_cp(struct jfea_atom_desc *dest,
                         struct jfea_atom_desc *source);
static int jfea_var_no(char *text);
static int jfea_set_adesc(char *opword, char *vname, int input);
static int jfea_isoption(char *txt);
static int jfea_get_command(int argc);
//static void jfea_pl_create(char type, int address, int limit_c, int mode);
static int jfea_adesc_create(void);
static float jfea_rand_dget(void);
static float jfea_rand_iv_dget(float iinf, float isup);
static float jfea_float_val(unsigned char v, float fmin, float fmax,
                            unsigned char steps);
static int jfea_ind_rand(void);
static void jfea_i2p(long source_no);
static void jfea_new_atom(int ino, int rno, int vno);
static int jfea_ind_cmp(int i1_no, int i2_no);
static void jfea_ind_check(int ind_no);

static int jfea_mut_direction(int repeat);
static unsigned char jfea_byte_mut(unsigned char b, int maxval, int repeat);
static void jfea_mutate(int ind_no, int repeat);

static int jfea_ind_rm(void);
static void jfea_ind_create(int ind_no);
static void jfea_ind_breed(void);
static void jfea_bobl(void);
static void jfea_inds_create(void);
static void jfea_pop_create(void);


/* int cdecl matherr (struct exception *a)
   {
     a->retval = 0.0;
     return 1;
   }
*/


/*************************************************************************/
/* Generering af konverteringstabel jfea_a_descs                          */
/*************************************************************************/

static int jfea_set_error(int no, char *etxt)
{
  jfea_error_desc.error_no = no;
  strcpy(jfea_error_desc.argument, etxt);
  return no;
}

static unsigned char jfea_cmin(unsigned char a, unsigned char b)
{
  if (a < b)
    return a;
  return b;
}

static unsigned char jfea_cmax(unsigned char a, unsigned char b)
{
  if (a > b)
    return a;
  return b;
}

static int jfea_get_bit(struct jfea_atom_desc *atom, int bitno)
{
  int m;

  if (bitno < 8)
    m = (jfea_bit_tabel[bitno] & atom->val_2);
  else
  if (bitno < 16)
    m = (jfea_bit_tabel[bitno - 8] & atom->val_1);
  else
    m = (jfea_bit_tabel[bitno - 16] & atom->aart);
  if (m != 0)
    return 1;
  return 0;
}

static void jfea_set_bit(struct jfea_atom_desc *atom, int bitno)
{
  if (bitno < 8)
    atom->val_2 |= jfea_bit_tabel[bitno];
  else
  if (bitno < 16)
    atom->val_1 |= jfea_bit_tabel[bitno - 8];
  else
    atom->aart |= jfea_bit_tabel[bitno - 16];
}

static void jfea_atom_cp(struct jfea_atom_desc *dest,
                         struct jfea_atom_desc *source)
{
  dest->atype = source->atype;
  dest->aart  = source->aart;
  dest->val_1 = source->val_1;
  dest->val_2 = source->val_2;
}

static int jfea_var_no(char *text)
{
  int m, res;
  struct jfg_var_desc vdesc;

  res = -1;
  for (m = 0; res == -1 && m < jfea_spdesc.var_c; m++)
  { jfg_var(&vdesc, jfea_pop.jfr_head, m);
    if (strcmp(vdesc.name, text) == 0)
      res = m;
  }
  return res;
}

static int jfea_set_adesc(char *opword, char *vname, int input)
{
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  struct jfea_atomd_desc *adesc;
  int vno, minmax, op_adjective, op_float, op_between, m;
  char opw_float[4] = "vb";
  char opw_adjective[4] = "ab";

  vno = jfea_var_no(vname);
  if (vno == -1)
    return jfea_set_error(506, vname);
  if (jfea_rvar_c >= JFEA_MAX_VARS)
    return jfea_set_error(505, vname);

  if (input == 1)
  { adesc = &(jfea_a_descs[jfea_rvar_c]);
    adesc->l_atypes[0] = JFEA_AT_NONE;
    adesc->atypes_c = 1;
  }
  else
  { adesc = &(jfea_a_descs[0]);
    adesc->atypes_c = 0;
  }

  adesc->var_no = vno;
  jfg_var(&vdesc, jfea_pop.jfr_head, vno);
  jfg_domain(&ddesc, jfea_pop.jfr_head, vdesc.domain_no);
  adesc->fzvar_c = vdesc.fzvar_c;
  adesc->fzvar_no = vdesc.f_fzvar_no;
  adesc->steps = 255;
  adesc->domain_no = vdesc.domain_no;
  adesc->domain_c = 0;
  adesc->min_value = 0.0;
  adesc->max_value = 1.0;
  if ((ddesc.flags & JFS_DF_MINENTER) && (ddesc.flags & JFS_DF_MAXENTER))
  { adesc->min_value = ddesc.dmin;
    adesc->max_value = ddesc.dmax;
    minmax = 1;
    if (ddesc.type == JFS_DT_INTEGER
        && ddesc.dmax - ddesc.dmin > 0
        && ddesc.dmax - ddesc.dmin < 256)
      adesc->steps = (unsigned char) (1 + ddesc.dmax - ddesc.dmin);
  }
  else
    minmax = 0;
  if (opword == NULL)
  { if (vdesc.fzvar_c == 0)
    { if (minmax == 1)
        opword = opw_float;
      else
        return jfea_set_error(550, vname);
    }
    else
      opword = opw_adjective;
  }
  op_adjective = op_float = op_between = 0;
  for (m = 0; m < ((int) strlen(opword)); m++)
  { if (opword[m] == 'a')
    { op_adjective = 1;
      adesc->l_atypes[adesc->atypes_c] = JFEA_AT_ADJECTIVE;
      adesc->atypes_c++;
      if (adesc->fzvar_c == 0)
        return jfea_set_error(551, vname);
    }
    else
    if (opword[m] == 'v')
    { op_float = 1;
      adesc->l_atypes[adesc->atypes_c] = JFEA_AT_RELATION;
      adesc->atypes_c++;
      if (minmax == 0)
        return jfea_set_error(550, vname);
    }
  }

  if (input == 1)
  { for (m = 0; m < ((int) strlen(opword)); m++)
    { if (opword[m] == 'b')
      { op_between = 1;
        if (op_float == 1)
        { adesc->l_atypes[adesc->atypes_c] = JFEA_AT_VAL_BETWEEN;
          adesc->atypes_c++;
        }
        if (op_adjective == 1)
        { adesc->l_atypes[adesc->atypes_c] = JFEA_AT_BETWEEN;
          adesc->atypes_c++;
        }
      }
      else
      if (opword[m] == 'i')
      { if (op_adjective == 1)
        { if (adesc->fzvar_c > 24)
            return jfea_set_error(553, vname);
          adesc->l_atypes[adesc->atypes_c] = JFEA_AT_IN;
          adesc->atypes_c++;
        }
        else
          return jfea_set_error(554, vname);
      }
    }

    for (m = 0; m < ((int) strlen(opword)); m++)
    { if (opword[m] == 'h')
      { if (op_adjective == 1)
        { adesc->l_atypes[adesc->atypes_c] = JFEA_AT_H_ADJECTIVE;
          adesc->atypes_c++;
          if (op_between == 1)
          { adesc->l_atypes[adesc->atypes_c] = JFEA_AT_H_BETWEEN;
            adesc->atypes_c++;
          }
        }
      }
      else
      if (opword[m] == 'r')
      { if (jfea_spdesc.relation_c == 0)
          return jfea_set_error(557, jfea_empty);
        adesc->l_atypes[adesc->atypes_c] = JFEA_AT_F_RELATION;
        adesc->atypes_c++;
      }
      else
      if (opword[m] == 'c')
      { adesc->l_atypes[adesc->atypes_c] = JFEA_AT_CMP;
        adesc->atypes_c++;
      }
      else
      if (opword[m] == 'C')
      { adesc->l_atypes[adesc->atypes_c] = JFEA_AT_F_CMP;
        adesc->atypes_c++;
      }
      else
      if (opword[m] == 'd')
      { adesc->l_atypes[adesc->atypes_c] = JFEA_AT_DCMP;
        adesc->atypes_c++;
      }
      else
      if (opword[m] == 'D')
      { adesc->l_atypes[adesc->atypes_c] = JFEA_AT_F_DCMP;
        adesc->atypes_c++;
      }
      else
      if (opword[m] == 'o')
      { adesc->l_atypes[adesc->atypes_c] = JFEA_AT_OP;
        if (minmax == 0)
          return jfea_set_error(550, vname);
        adesc->atypes_c++;
      }
    }
    if (adesc->atypes_c == 0)
      return jfea_set_error(552, vname);

    jfea_rvar_c++;
  }
  else /* input == 0 */
  { for (m = 0; m < ((int) strlen(opword)); m++)
    { if (opword[m] == 'n')
      { adesc->l_atypes[adesc->atypes_c] = JFEA_AT_NONE;
        adesc->atypes_c++;
      }
      else
      if (opword[m] == 'i')
      { adesc->l_atypes[adesc->atypes_c] = JFEA_AT_INCDEC;
        adesc->atypes_c++;
      }
    }
    if (adesc->atypes_c == 0)
      return jfea_set_error(555, vname);
  }
  return 0;
}


static int jfea_isoption(char *txt)
{
  if (txt[0] == '-')
    return 1;
  return 0;
}

static int jfea_get_command(int argc)
{
  int m, v, res;
  struct jfg_var_desc vdesc;
  int state;   /*  0: start                       */
               /*  1: jfrd                        */
               /*  2: input                       */
               /*  3: output                      */
               /*  4: slut                        */

  char *opword;

  opword = NULL;
  jfea_rvar_c = 1;  /* number 0: output-var */

  state = 0;
  m = 0;
  while (state != -1)
  { if (m >= argc)
    { if (state != 4)
        return jfea_set_error(504, jfea_empty);
    }
    switch (state)
    { case 0:
        if (strcmp(jfea_words[m], "jfrd") == 0)
        { state = 1;
          m++;
        }
        else
          return jfea_set_error(504, jfea_words[m]);
        break;
      case 1:  /* jfrd */
        if (strcmp(jfea_words[m], "input") == 0)
        { state = 2;
          m++;
        }
        else
          return jfea_set_error(504, jfea_words[m]);
        break;
      case 2:  /* input */
        if (strcmp(jfea_words[m], "output") == 0)
        { state = 3;
          m++;
        }
        else
        if (jfea_isoption(jfea_words[m]) == 1)
        { m++;
          opword = jfea_words[m];
          m++;
          state = 5;
        }
        else
        { opword = NULL;
          state = 5;
        }
        break;
      case 5:  /* foer input-var */
        if (strcmp(jfea_words[m], "output") == 0)
        { state = 3;
          m++;
        }
        else
        { if ((res = jfea_set_adesc(opword, jfea_words[m], 1)) != 0)
            return res;
          m++;
          state = 2;
        }
        break;
      case 3:  /* efter output */
        if (jfea_rvar_c == 1)
        { for (v = 0; v < jfea_spdesc.ivar_c; v++)
          { jfg_var(&vdesc, jfea_pop.jfr_head, jfea_spdesc.f_ivar_no + v);
            if ((res = jfea_set_adesc(opword, vdesc.name, 1)) != 0)
              return res;
          }
          if (jfea_rvar_c == 1)
            return jfea_set_error(556, jfea_empty);
        }
        opword = NULL;
        if (jfea_isoption(jfea_words[m]) == 1)
        { m++;
          opword = jfea_words[m];
          m++;
        }
        if ((res = jfea_set_adesc(opword, jfea_words[m], 0)) != 0)
           return res;
        if (jfea_no_default == 1)
        { jfg_var(&vdesc, jfea_pop.jfr_head, jfea_a_descs[0].var_no);
          if (vdesc.default_type != 2)
            return jfea_set_error(522, jfea_empty);
          if (vdesc.fzvar_c == 1)
            return jfea_set_error(560, vdesc.name);
          jfea_default_no = vdesc.default_no;
        }
        m++;
        state = 4;
        break;
      case 4:
        state = -1; /* slut */
        if (m < argc)
          return jfea_set_error(504, jfea_words[m]);
        break;
    }
  }
  /* calculate domain_c : */
  for (m = 1; m < jfea_rvar_c; m++)
  { for (v = 1; v < jfea_rvar_c; v++)
    { if (jfea_a_descs[m].domain_no == jfea_a_descs[v].domain_no)
        jfea_a_descs[m].domain_c++;
    }
  }
  return 0;
}

static int jfea_adesc_create(void)
{
  int slut, fno, m, res;
  unsigned char *pc;
  struct jfg_statement_desc sdesc;
  struct jfg_function_desc fdesc;

  slut = 0;
  for (fno = 0; fno <= jfea_spdesc.function_c; fno++)
  { if (fno == jfea_spdesc.function_c)
      pc = jfea_spdesc.pc_start;
    else
    { jfg_function(&fdesc, jfea_pop.jfr_head, fno);
      pc = fdesc.pc;
    }
    jfg_statement(&sdesc, jfea_pop.jfr_head, pc);
    while (slut == 0
            &&
            !(sdesc.type == JFG_ST_EOP
              || (sdesc.type == JFG_ST_STEND && sdesc.sec_type == 2)))
    { if (sdesc.type == JFG_ST_IF && sdesc.sec_type == JFG_SST_EXTERN)
      { m = jfg_a_statement(jfea_words, JFEA_MAX_WORDS, jfea_pop.jfr_head, pc);
        if (m < 0)
          return jfea_set_error(519, jfea_empty);
        if ((res = jfea_get_command(m)) == 0)
        { jfea_program_id = pc;
          jfea_delete_c = 1;
          slut = 1;
        }
        else
          return res;
      }
      else
      { pc = sdesc.n_pc;
        jfg_statement(&sdesc, jfea_pop.jfr_head, pc);
      }
    }
  }
  if (slut == 1)
    return 0;
  else
    return jfea_set_error(520, jfea_empty);
}

/*************************************************************************/
/* Hjaelpe funktioner til crosover og mutation                           */
/*************************************************************************/

int jfea_random(int sup)
{
  int res;

  res = (int)(rand() * ((float) sup) / (RAND_MAX+1.0));
  /* res = rand() % sup; */
  return res;
}

static float jfea_rand_dget(void)
{
  float husk, new_seed;

  husk = jfea_jraseed * 24298.0 + 99991.0;

  new_seed = husk - floor(husk / 199017.0) * 199017.0;
  if (new_seed == jfea_jraseed)
  { printf(" ERROR in random numbers !!!. jraseed: %f\n\a\a", new_seed);
    new_seed = 1.0 * jfea_random(30000);
  }
  jfea_jraseed = new_seed;
  return (jfea_jraseed / 199017.0);
}

static float jfea_rand_iv_dget(float iinf, float isup)
{
  float f;

  f = jfea_rand_dget();
  return  f * (isup - iinf) + iinf;
}

static int jfea_ind_rand(void)
{
  int m;
  float r, ssum;

  r = jfea_rand_dget();
  ssum = 0.0;
  for (m = 0; m < jfea_ind_c; m++)
  { ssum += jfea_pop.c_q - jfea_pop.c_r * (float) m;
    if (r < ssum)
      return m;
  }
  return 0;
}


/*************************************************************************/
/* Konverteringsfunktioner                                               */
/*************************************************************************/

static float jfea_float_val(unsigned char v, float fmin, float fmax,
                            unsigned char steps)
{
  float f;

  f = ((float) v) / ((float) steps);
  f = fmin + f * (fmax - fmin) ;
  return f;
}

static void jfea_i2p(long source_no)
{
  /* kopierer et individ til jfr_progammet. */

  long adr, radr;
  int r, v, husk_ft, ft, tt, lt, count, b,
      first_bit, rm_count, op, steps, ival;
  float mi, ma, tv;
  unsigned char then_no;
  unsigned char *pc;
  struct jfg_statement_desc stat;
  struct jfea_atomd_desc *adesc;
  struct jfea_atom_desc *atom;

  then_no = 0;
  for (r = 0; r < jfea_delete_c; r++)
    jfp_d_statement(jfea_pop.jfr_head, jfea_program_id);
  jfea_delete_c = 0;
  pc = jfea_program_id;
  adr = source_no * jfea_atom_c;
  for (r = 0; r < jfea_rule_c; r++)
  { ft = 0;
    adesc = &(jfea_a_descs[0]);
    if (jfea_inds[adr].atype != JFEA_AT_NONE)
    { stat.type = JFG_ST_IF;
      stat.flags = 0;
      if (jfea_inds[adr].atype == JFEA_AT_ADJECTIVE)
      { stat.sec_type = JFG_SST_FZVAR;
        then_no = jfea_inds[adr].val_1;
        stat.sarg_1 = adesc->fzvar_no + then_no;
      }
      else
      if (jfea_inds[adr].atype == JFEA_AT_RELATION)
      { stat.sec_type = JFG_SST_VAR;
        stat.sarg_1 = adesc->var_no;
        jfea_tree[ft].type = JFG_TT_CONST;
        jfea_tree[ft].farg
            = jfea_float_val(jfea_inds[adr].val_1,
                             adesc->min_value, adesc->max_value,
                             adesc->steps - 1);
        ft++;
      }
      else
      if (jfea_inds[adr].atype == JFEA_AT_INCDEC)
      { stat.sec_type = JFG_SST_INC;
         if (jfea_inds[adr].aart == 1)
           stat.flags += 4;
        stat.sarg_1 = adesc->var_no;
        jfea_tree[ft].type = JFG_TT_CONST;
        jfea_tree[ft].farg
            = jfea_float_val(jfea_inds[adr].val_1, 0.0, 1.0, 255);
        ft++;
      }

      radr = adr + 1;
      count = 0;
      rm_count = 0;
      lt = ft;
      for (v = 1; v < jfea_rvar_c; v++)
      { atom = &(jfea_inds[radr]);
        adesc = &(jfea_a_descs[v]);
        radr++;
        switch (atom->atype)
        { case JFEA_AT_NONE:
            break;
          case JFEA_AT_ADJECTIVE:
            jfea_tree[ft].type = JFG_TT_FZVAR;
            jfea_tree[ft].sarg_1 = adesc->fzvar_no + atom->val_1;
            tt = ft;
            ft++;
            count++;
            break;
          case JFEA_AT_BETWEEN:
            jfea_tree[ft].type = JFG_TT_BETWEEN;
            jfea_tree[ft].sarg_1 = jfea_a_descs[v].var_no;
            jfea_tree[ft].sarg_2 = atom->val_1;
            jfea_tree[ft].op = atom->val_2;
            tt = ft;
            ft++;
            count++;
            break;
          case JFEA_AT_H_ADJECTIVE:
            jfea_tree[ft].type = JFG_TT_FZVAR;
            jfea_tree[ft].sarg_1 = adesc->fzvar_no + atom->val_1;
            ft++;
            jfea_tree[ft].type = JFG_TT_HEDGE;
            jfea_tree[ft].op = atom->aart;
            jfea_tree[ft].sarg_1 = ft - 1;
            tt = ft;
            ft++;
            count++;
            break;
          case JFEA_AT_H_BETWEEN:
            jfea_tree[ft].type = JFG_TT_BETWEEN;
            jfea_tree[ft].sarg_1 = jfea_a_descs[v].var_no;
            jfea_tree[ft].sarg_2 = atom->val_1;
            jfea_tree[ft].op = atom->val_2;
            ft++;
            jfea_tree[ft].type = JFG_TT_HEDGE;
            jfea_tree[ft].op = atom->aart;
            jfea_tree[ft].sarg_1 = ft - 1;
            tt = ft;
            ft++;
            count++;
            break;
          case JFEA_AT_RELATION:
            jfea_tree[ft].type = JFG_TT_VAR;
            jfea_tree[ft].sarg_1 = adesc->var_no;
            ft++;
            jfea_tree[ft].type = JFG_TT_CONST;
            jfea_tree[ft].op = 0;
            jfea_tree[ft].farg
                = jfea_float_val(atom->val_1, adesc->min_value,
                                 adesc->max_value, adesc->steps - 1);
            ft++;
            jfea_tree[ft].type = JFG_TT_DFUNC;
            jfea_tree[ft].op = jfea_rel_tabel[atom->aart];
            jfea_tree[ft].sarg_1 = ft - 2;
            jfea_tree[ft].sarg_2 = ft - 1;
            tt = ft;
            ft++;
            count++;
            break;
          case JFEA_AT_OP:
            jfea_tree[ft].type = JFG_TT_VAR;
            jfea_tree[ft].sarg_1 = adesc->var_no;
            ft++;
            jfea_tree[ft].type = JFG_TT_VAR;
            jfea_tree[ft].sarg_1 = jfea_a_descs[atom->val_1].var_no;
            ft++;
            jfea_tree[ft].type = JFG_TT_DFUNC;
            op = atom->aart / 16;
            if (op == 0)
            { jfea_tree[ft].op = JFS_DFU_PLUS;
              mi = adesc->min_value + jfea_a_descs[atom->val_1].min_value;
              ma = adesc->max_value + jfea_a_descs[atom->val_1].max_value;
            }
            else
            if (op == 1)
            { jfea_tree[ft].op = JFS_DFU_MINUS;
              mi = adesc->min_value - jfea_a_descs[atom->val_1].max_value;
              ma = adesc->max_value - jfea_a_descs[atom->val_1].min_value;
            }
            else
            { jfea_tree[ft].op = JFS_DFU_PROD;
              mi = ma = adesc->min_value * jfea_a_descs[atom->val_1].min_value;
              tv = adesc->min_value * jfea_a_descs[atom->val_1].max_value;
              if (tv < mi) mi = tv;
              if (tv > ma) ma = tv;
              tv = adesc->max_value * jfea_a_descs[atom->val_1].min_value;
              if (tv < mi) mi = tv;
              if (tv > ma) ma = tv;
              tv = adesc->max_value * jfea_a_descs[atom->val_1].max_value;
              if (tv < mi) mi = tv;
              if (tv > ma) ma = tv;
            }
            steps = (int) (ma - mi + 1);
            if (steps < adesc->steps)
              steps = adesc->steps;
            if (steps < jfea_a_descs[atom->val_1].steps)
              steps = jfea_a_descs[atom->val_1].steps;
            if (steps > 255)
              steps = 255;
            ival = (int) ((((float) atom->val_2) / ((float) adesc->steps - 1.0))
                          * (float) (steps));
            jfea_tree[ft].sarg_1 = ft - 2;
            jfea_tree[ft].sarg_2 = ft - 1;
            ft++;
            jfea_tree[ft].type = JFG_TT_CONST;
            jfea_tree[ft].op = 0;
            jfea_tree[ft].farg = jfea_float_val(ival, mi, ma, steps - 1);
            ft++;
            jfea_tree[ft].type = JFG_TT_DFUNC;
            jfea_tree[ft].op = jfea_rel_tabel[atom->aart & 7];
            jfea_tree[ft].sarg_1 = ft - 2;
            jfea_tree[ft].sarg_2 = ft - 1;
            tt = ft;
            ft++;
            count++;
            break;
          case JFEA_AT_CMP:
          case JFEA_AT_DCMP:
            jfea_tree[ft].type = JFG_TT_VAR;
            jfea_tree[ft].sarg_1 = adesc->var_no;
            ft++;
            jfea_tree[ft].type = JFG_TT_VAR;
            jfea_tree[ft].op = 0;
            jfea_tree[ft].sarg_1 = jfea_a_descs[atom->val_1].var_no;
            ft++;
            jfea_tree[ft].type = JFG_TT_DFUNC;
            jfea_tree[ft].op = jfea_rel_tabel[atom->aart];
            jfea_tree[ft].sarg_1 = ft - 2;
            jfea_tree[ft].sarg_2 = ft - 1;
            tt = ft;
            ft++;
            count++;
            break;
          case JFEA_AT_F_CMP:
          case JFEA_AT_F_DCMP:
            jfea_tree[ft].type = JFG_TT_VAR;
            jfea_tree[ft].sarg_1 = adesc->var_no;
            ft++;
            jfea_tree[ft].type = JFG_TT_VAR;
            jfea_tree[ft].op = 0;
            jfea_tree[ft].sarg_1 = jfea_a_descs[atom->val_1].var_no;
            ft++;
            jfea_tree[ft].type = JFG_TT_UREL;
            jfea_tree[ft].op = atom->aart;
            jfea_tree[ft].sarg_1 = ft - 2;
            jfea_tree[ft].sarg_2 = ft - 1;
            tt = ft;
            ft++;
            count++;
            break;
          case JFEA_AT_F_RELATION:
            jfea_tree[ft].type = JFG_TT_VAR;
            jfea_tree[ft].sarg_1 = adesc->var_no;
            ft++;
            jfea_tree[ft].type = JFG_TT_CONST;
            jfea_tree[ft].op = 0;
            jfea_tree[ft].farg
                = jfea_float_val(atom->val_1, adesc->min_value,
                                 adesc->max_value, adesc->steps - 1);
            ft++;
            jfea_tree[ft].type = JFG_TT_UREL;
            jfea_tree[ft].op = atom->aart;
            jfea_tree[ft].sarg_1 = ft - 2;
            jfea_tree[ft].sarg_2 = ft - 1;
            tt = ft;
            ft++;
            count++;
            break;
          case JFEA_AT_VAL_BETWEEN:
            /* var >= val_1 and var <= val_2: */
            jfea_tree[ft].type = JFG_TT_VAR;
            jfea_tree[ft].sarg_1 = adesc->var_no;
            ft++;
            jfea_tree[ft].type = JFG_TT_CONST;
            jfea_tree[ft].op = 0;
            jfea_tree[ft].farg
                = jfea_float_val(atom->val_1, adesc->min_value,
                                 adesc->max_value, adesc->steps - 1);
            ft++;
            jfea_tree[ft].type = JFG_TT_DFUNC;
            jfea_tree[ft].op = JFS_ROP_GEQ;
            jfea_tree[ft].sarg_1 = ft - 2;
            jfea_tree[ft].sarg_2 = ft - 1;
            husk_ft = ft;
            ft++;
            jfea_tree[ft].type = JFG_TT_VAR;
            jfea_tree[ft].sarg_1 = adesc->var_no;
            ft++;
            jfea_tree[ft].type = JFG_TT_CONST;
            jfea_tree[ft].op = 0;
            jfea_tree[ft].farg
                = jfea_float_val(atom->val_2, adesc->min_value,
                                 adesc->max_value, adesc->steps - 1);
            ft++;
            jfea_tree[ft].type = JFG_TT_DFUNC;
            jfea_tree[ft].op = JFS_ROP_LEQ;
            jfea_tree[ft].sarg_1 = ft - 2;
            jfea_tree[ft].sarg_2 = ft - 1;
            ft++;
            jfea_tree[ft].type = JFG_TT_OP;
            jfea_tree[ft].op = JFS_ONO_AND;
            jfea_tree[ft].sarg_1 = husk_ft;
            jfea_tree[ft].sarg_2 = ft - 1;
            tt = ft;
            ft++;
            count++;
            break;
          case JFEA_AT_IN:
            first_bit = 1;
            for (b = 0; b < adesc->fzvar_c; b++)
            { if (jfea_get_bit(atom, b) == 1)
              { jfea_tree[ft].type = JFG_TT_FZVAR;
                jfea_tree[ft].sarg_1 = adesc->fzvar_no + b;
                ft++;
                if (first_bit == 0)
                { jfea_tree[ft].type = JFG_TT_OP;
                  jfea_tree[ft].op = JFS_ONO_OR;
                  jfea_tree[ft].sarg_1 = ft - 2;
                  jfea_tree[ft].sarg_2 = ft - 1;
                  ft++;
                }
                first_bit = 0;
              }
            }
            if (first_bit == 0)
            { tt = ft - 1;
              count++;
            }
            break;
        }
        if (count > 1 && count != rm_count)
        { jfea_tree[ft].type = JFG_TT_OP;
          jfea_tree[ft].op = JFS_ONO_AND;
          jfea_tree[ft].sarg_1 = lt;
          jfea_tree[ft].sarg_2 = tt;
          lt = ft;
          ft++;
        }
        else
        if (count == 1)
          lt = tt;
        rm_count = count;
      }
      if (count == 0)
      { jfea_tree[ft].type = JFG_TT_TRUE;
        lt = ft;
      }
      if (jfp_i_tree(jfea_pop.jfr_head, &pc, &stat, jfea_tree,
                     lt, 0, 0, jfea_words, 0)
          == 0)
        jfea_delete_c++;
    }
    adr += jfea_rvar_c;
  }
}


/*************************************************************************/
/* mutation/crosover funktioner                                          */
/*************************************************************************/

static void jfea_new_atom(int ino, int rno, int vno)
{
  long adr;
  int m, ra, vc;
  struct jfea_atom_desc *atom;
  struct jfea_atomd_desc *adesc;

  adr = ino * jfea_atom_c + rno * jfea_rvar_c + vno;
  atom = &(jfea_inds[adr]);
  adesc = &(jfea_a_descs[vno]);
  atom->atype = adesc->l_atypes[jfea_random(adesc->atypes_c)];
  switch (atom->atype)
  { case JFEA_AT_NONE:
      atom->aart = 0;
      atom->val_1 = 0;
      atom->val_2 = 0;
      break;
    case JFEA_AT_ADJECTIVE:
      atom->aart = 0;
      atom->val_1 = jfea_random(adesc->fzvar_c);
      atom->val_2 = 0;
      break;
    case JFEA_AT_BETWEEN:
      atom->aart = 0;
      atom->val_1 = jfea_random(adesc->fzvar_c);
      atom->val_2 = jfea_random(adesc->fzvar_c);
      break;
    case JFEA_AT_H_ADJECTIVE:
      atom->aart = jfea_random(jfea_spdesc.hedge_c);
      atom->val_1 = jfea_random(adesc->fzvar_c);
      atom->val_2 = 0;
      break;
    case JFEA_AT_H_BETWEEN:
      atom->aart = jfea_random(jfea_spdesc.hedge_c);
      atom->val_1 = jfea_random(adesc->fzvar_c);
      atom->val_2 = jfea_random(adesc->fzvar_c);
      break;
    case JFEA_AT_RELATION:
      atom->aart = jfea_random(6);
      atom->val_1 = jfea_random(adesc->steps);
      atom->val_2 = 0;
      break;
    case JFEA_AT_OP:
      atom->aart = jfea_random(6) + 16 * jfea_random(3);
      atom->val_1 = 1 + jfea_random(jfea_rvar_c - 1);
      atom->val_2 = jfea_random(adesc->steps);
      break;
    case JFEA_AT_CMP:
      atom->aart = jfea_random(6);
      atom->val_1 = 1 + jfea_random(jfea_rvar_c - 1);
      atom->val_2 = 0;
      break;
    case JFEA_AT_DCMP:
      atom->aart = jfea_random(6);
      ra = jfea_random(adesc->domain_c);
      vc = 0;
      for (m = 1; m < jfea_rvar_c; m++)
      { if (adesc->domain_no == jfea_a_descs[m].domain_no)
        { if (vc == ra)
          { atom->val_1 = m;
            break;
          }
          vc++;
        }
      }
      atom->val_2 = 0;
      break;
    case JFEA_AT_F_CMP:
      atom->aart = jfea_random(jfea_spdesc.relation_c);
      atom->val_1 = 1 + jfea_random(jfea_rvar_c - 1);
      atom->val_2 = 0;
      break;
    case JFEA_AT_F_DCMP:
      atom->aart = jfea_random(jfea_spdesc.relation_c);
      ra = jfea_random(adesc->domain_c);
      vc = 0;
      for (m = 1; m < jfea_rvar_c; m++)
      { if (adesc->domain_no == jfea_a_descs[m].domain_no)
        { if (vc == ra)
          { atom->val_1 = m;
            break;
          }
          vc++;
        }
      }
      atom->val_2 = 0;
      break;
    case JFEA_AT_F_RELATION:
      atom->aart = jfea_random(jfea_spdesc.relation_c);
      atom->val_1 = jfea_random(adesc->steps);
      atom->val_2 = 0;
      break;
    case JFEA_AT_VAL_BETWEEN:
      atom->aart = 0;
      atom->val_1 = jfea_random(adesc->steps);
      atom->val_2 = jfea_random(adesc->steps);
      break;
    case JFEA_AT_INCDEC:
      atom->aart = jfea_random(2);
      atom->val_1 = jfea_random(256);
      atom->val_2 = 0;
      break;
    case JFEA_AT_IN:
      atom->aart = 0; atom->val_1 = 0; atom->val_2 = 0;
      for (m = 0; m < adesc->fzvar_c; m++)
      { if (jfea_random(2) == 0)
          jfea_set_bit(atom, m);
      }
      break;
  }
}

static int jfea_ind_cmp(int i1_no, int i2_no)
{
  int dif, m;
  struct jfea_atom_desc *atom_1;
  struct jfea_atom_desc *atom_2;

  dif = 0;
  atom_1 = &(jfea_inds[i1_no * jfea_atom_c]);
  atom_2 = &(jfea_inds[i2_no * jfea_atom_c]);
  for (m = 0; dif == 0 && m < jfea_atom_c; m++)
  { if (atom_1->atype != atom_2->atype
        || atom_1->aart != atom_2->aart
        || atom_1->val_1 != atom_2->val_1
        || atom_1->val_2 != atom_2->val_2)
      dif = 1;
    atom_1++;
    atom_2++;
  }
  return dif;
}

static void jfea_ind_check(int ind_no)
{
  long adr, aadr;
  int r, a, fino, b, bcount, bno;
  struct jfea_atom_desc *atom;

  adr = ind_no * jfea_atom_c;
  fino = 0;
  for (r = 0; r < jfea_rule_c; r++)
  { aadr = adr;
    for (a = 0; a < jfea_rvar_c; a++)
    { atom = &(jfea_inds[aadr]);
      if (a == 0)
      { if (atom->atype == JFEA_AT_ADJECTIVE)
        { if (jfea_fixed_rules == 1)
          { if (jfea_no_default == 1 && fino == jfea_default_no)
              fino++;
            if (fino >= jfea_a_descs[0].fzvar_c)
              fino = 0;
            atom->val_1 = fino;
            fino++;
            if (fino >= jfea_a_descs[0].fzvar_c)
              fino = 0;
          }
          else
          { if (jfea_no_default == 1)
            { while (fino == jfea_default_no)
                fino = jfea_random(jfea_a_descs[0].fzvar_c);
              atom->val_1 = fino;
            }
          }
        }
      }
      else
      { if (atom->atype == JFEA_AT_BETWEEN || atom->atype == JFEA_AT_H_BETWEEN)
        { if (atom->val_1 > atom->val_2
              || (atom->val_1 == 0
                  && atom->val_2 == jfea_a_descs[a].fzvar_c - 1))
          { atom->atype = JFEA_AT_NONE;
            atom->aart = 0;
            atom->val_1 = 0;
            atom->val_2 = 0;
          }
          else
          if (atom->val_1 == atom->val_2)
          { atom->val_2 = 0;
            if (atom->atype == JFEA_AT_BETWEEN)
              atom->atype = JFEA_AT_ADJECTIVE;
            else
              atom->atype = JFEA_AT_H_ADJECTIVE;
          }
        }
        else
        if (atom->atype == JFEA_AT_VAL_BETWEEN)
        { if (atom->val_1 > atom->val_2
              || (atom->val_1 == 0 && atom->val_2 == jfea_a_descs[a].steps - 1))
          { atom->atype = JFEA_AT_NONE;
            atom->aart = 0;
            atom->val_1 = 0;
            atom->val_2 = 0;
          }
        }
        else
        if (atom->atype == JFEA_AT_IN)
        { bcount = 0;
          for (b = 0; b < jfea_a_descs[a].fzvar_c; b++)
          { if (jfea_get_bit(atom, b) != 0)
            { bcount++;
              bno = b;
            }
          }
          if (bcount == 0 || bcount == jfea_a_descs[a].fzvar_c - 1)
          { atom->atype = JFEA_AT_NONE;
            atom->aart = 0;
            atom->val_1 = 0;
            atom->val_2 = 0;
          }
          else
          if (bcount == 1)
          { atom->atype == JFEA_AT_ADJECTIVE;
            atom->aart = 0;
            atom->val_1 = bno;
            atom->val_2 = 0;
          }
        }
      }
      aadr++;
    }
    adr += jfea_rvar_c;
  }
}

static int jfea_mut_direction(int repeat)
{
  int di;

  if (repeat == 1)
    di = jfea_rm_ind.direction;
  else
    di = 0;
  while (di == 0)
    di = jfea_random(3) - 1;
  jfea_rm_ind.direction = di;
  return di;
}

static unsigned char jfea_byte_mut(unsigned char b, int maxval, int repeat)
{
  int m, di;
  unsigned char res;

  di = jfea_mut_direction(repeat);
  m = ((int) b) + di;
  if (m < 0)
    m = 0;
  if (m >= maxval)
    m = maxval - 1;
  res = m;
  return res;
}

static void jfea_mutate(int ind_no, int repeat)
{
  int m, rno, vno, di;
  struct jfea_atom_desc *atom;
  struct jfea_atomd_desc *adesc;

  if (repeat == 1)
  { rno = jfea_rm_ind.rno;
    vno = jfea_rm_ind.vno;
  }
  else
  { rno = jfea_random(jfea_rule_c);
    vno = jfea_random(jfea_rvar_c);
    jfea_rm_ind.rno = rno;
    jfea_rm_ind.vno = vno;
  }
  atom = &(jfea_inds[ind_no * jfea_atom_c + rno * jfea_rvar_c + vno]);
  adesc = &(jfea_a_descs[vno]);
  switch (atom->atype)
  { case JFEA_AT_NONE:
      jfea_new_atom(ind_no, rno, vno);
      break;
    case JFEA_AT_ADJECTIVE:
      atom->val_1 = jfea_byte_mut(atom->val_1, adesc->fzvar_c, repeat);
      break;
    case JFEA_AT_BETWEEN:
      if (jfea_random(2) == 0)
        atom->val_1 = jfea_byte_mut(atom->val_1, adesc->fzvar_c, repeat);
      else
        atom->val_2 = jfea_byte_mut(atom->val_2, adesc->fzvar_c, repeat);
      break;
    case JFEA_AT_H_ADJECTIVE:
      if (jfea_random(2) == 0)
        atom->val_1 = jfea_byte_mut(atom->val_1, adesc->fzvar_c,repeat);
      else
        atom->aart = jfea_byte_mut(atom->aart, jfea_spdesc.hedge_c, repeat);
      break;
    case JFEA_AT_H_BETWEEN:
      m = jfea_random(3);
      if (m == 0)
        atom->val_1 = jfea_byte_mut(atom->val_1, adesc->fzvar_c, repeat);
      else
      if (m == 1)
        atom->val_2 = jfea_byte_mut(atom->val_2, adesc->fzvar_c, repeat);
      else
        atom->aart = jfea_byte_mut(atom->aart, jfea_spdesc.hedge_c, repeat);
      break;
    case JFEA_AT_RELATION:
      m = jfea_random(3);
      if (m == 0)
      { atom->val_1 = jfea_byte_mut(atom->val_1, jfea_rvar_c, repeat);
        if (atom->val_1 == 0)
          atom->val_1 = 1;
      }
      else
      if (m == 1)
        atom->val_2 = jfea_byte_mut(atom->val_2, adesc->steps, repeat);
      else
        atom->aart = jfea_random(6) + 16 * jfea_random(3);
      break;
    case JFEA_AT_CMP:
      if (jfea_random(2) == 0)
      { atom->val_1 = jfea_byte_mut(atom->val_1, jfea_rvar_c, repeat);
        if (atom->val_1 == 0)
          atom->val_1 = 1;
      }
      else
        atom->aart = jfea_byte_mut(atom->aart, 6, repeat);
      break;
    case JFEA_AT_DCMP:
      if (jfea_random(2) == 0)
      { di = jfea_mut_direction(repeat);
        m = atom->val_1 + di;
        while (1 == 1)
        { if (m <= 0)
            m = jfea_rvar_c - 1;
          if (m >= jfea_rvar_c)
            m = 1;
          if (adesc->domain_no == jfea_a_descs[m].domain_no)
          {  atom->val_1 = m;
             break;
          }
            m += di;
        }
      }
      else
        atom->aart = jfea_byte_mut(atom->aart, 6, repeat);
      break;
    case JFEA_AT_F_CMP:
      if (jfea_random(2) == 0)
      { atom->val_1 = jfea_byte_mut(atom->val_1, jfea_rvar_c, repeat);
        if (atom->val_1 == 0)
          atom->val_1 = 1;
      }
      else
        atom->aart = jfea_byte_mut(atom->aart, jfea_spdesc.relation_c, repeat);
      break;
    case JFEA_AT_F_DCMP:
      if (jfea_random(2) == 0)
      { di = jfea_mut_direction(repeat);
        m = atom->val_1 + di;
        while (1 == 1)
        { if (m <= 0)
            m = jfea_rvar_c - 1;
          if (m >= jfea_rvar_c)
            m = 1;
          if (adesc->domain_no == jfea_a_descs[m].domain_no)
          {  atom->val_1 = m;
             break;
          }
            m += di;
        }
      }
      else
        atom->aart = jfea_byte_mut(atom->aart, jfea_spdesc.relation_c, repeat);
      break;
    case JFEA_AT_F_RELATION:
      if (jfea_random(2) == 0)
        atom->val_1 = jfea_byte_mut(atom->val_1, adesc->steps, repeat);
      else
        atom->aart = jfea_byte_mut(atom->aart, jfea_spdesc.relation_c, repeat);
      break;
    case JFEA_AT_VAL_BETWEEN:
      if (jfea_random(2) == 0)
        atom->val_1 = jfea_byte_mut(atom->val_1, adesc->steps, repeat);
      else
        atom->val_2 = jfea_byte_mut(atom->val_2, adesc->steps, repeat);
      break;
    case JFEA_AT_INCDEC:
      if (jfea_random(2) == 0)
        atom->val_1 = jfea_byte_mut(atom->val_1, 256, repeat);
      else
        atom->aart = jfea_random(2);
      break;
    case JFEA_AT_IN:
      if (jfea_random(2) == 0)
        jfea_set_bit(atom, jfea_random(adesc->fzvar_c));
      else
      { m = jfea_random(adesc->fzvar_c);
        if (m < 8)
          atom->val_2 &= 256 - jfea_bit_tabel[m];
        else
        if (m < 16)
          atom->val_1 &= 256 - jfea_bit_tabel[m - 8];
        else
          atom->val_1 &= 256 - jfea_bit_tabel[m - 16];
      }
      break;
  }
}


static int jfea_ind_rm(void)
{
  int sno;

  sno = 0;
  while (sno <= 0)
  { sno = jfea_ind_c - jfea_ind_rand() - 1;
    if (jfea_pop.scores[sno].iprotect == 1)
      sno = 0;
  }
  jfea_stat.old_score = jfea_pop.scores[sno].score;
  return sno;
}


static void jfea_ind_create(int ind_no)
{
  int r, v, arule;
  long adr;
  struct jfea_atom_desc *atom;

  for (r = 0; r < jfea_rule_c; r++)
  { arule = jfea_random(4);
    for (v = 0; v < jfea_rvar_c; v++)
    { if (v == 0 || arule >= 2)
        jfea_new_atom(ind_no, r, v);
      else
      { adr = ind_no * jfea_atom_c + r * jfea_rvar_c + v;
        atom = &(jfea_inds[adr]);
        atom->atype = JFEA_AT_NONE;
        atom->aart = 0;
        atom->val_1 = 0;
        atom->val_2 = 0;
      }
      if (arule == 1)
        jfea_new_atom(ind_no, r, 1 + jfea_random(jfea_rvar_c - 1));
    }
  }
}

static void jfea_ind_breed(void)
{
  struct jfea_score_desc *child;
  int d_ind_no, m, s1_ind_no, s2_ind_no, cp1, cp2, s1no;
  int a, d_sno, method, dif, am_atoms, e, r, p, rno, vno;
  struct jfea_atom_desc *s1_ind;
  struct jfea_atom_desc *s2_ind;
  struct jfea_atom_desc *d_ind;
  struct jfea_atom_desc tval;

  d_sno = jfea_ind_rm();

  child = &(jfea_pop.scores[d_sno]);
  d_ind_no = child->ind_no;

  if (jfea_rm_ind.succes == 1)
  { m = jfea_rm_ind.scores_id;
    while (m == d_sno)
      m = jfea_ind_rand();
  }
  else
  { m = d_sno;
    while (m == d_sno)
      m = jfea_ind_rand();
  }
  s1no = m;
  s1_ind_no = jfea_pop.scores[m].ind_no;
  jfea_stat.p1_score = jfea_pop.scores[m].score;

  m = d_sno;
  if (jfea_random(4) == 0)
  { m = d_sno;
    while (m == d_sno || m == s1no)
      m = jfea_random(jfea_ind_c);
  }
  else
  { while (m == d_sno || m == s1no)
      m = jfea_ind_rand();
  }
  s2_ind_no = jfea_pop.scores[m].ind_no;
  jfea_stat.p2_score = jfea_pop.scores[m].score;

  jfea_rm_ind.ind_no = d_ind_no;
  s1_ind = &(jfea_inds[s1_ind_no * jfea_atom_c]);
  s2_ind = &(jfea_inds[s2_ind_no * jfea_atom_c]);
  d_ind =  &(jfea_inds[d_ind_no * jfea_atom_c]);

  /*^cp1 = 0; cp2 = jfea_atom_c - 1; */
  if (jfea_rm_ind.succes == 1)
    method = 7;  /* repeat mutation */
  else
  { method = 7;
    while (method == 7) /* select something different! (qad) */
      method = jfea_random(10);
  }
  jfea_rm_ind.succes = 0;
  switch (method)
  { case 0: /* crosover: */
      cp1 = jfea_random(jfea_atom_c);
      cp2 = jfea_random(jfea_atom_c - cp1) + cp1;
      for (m = 0; m < jfea_atom_c; m++)
      { if (m >= cp1 && m < cp2)
          jfea_atom_cp(&tval, &(s2_ind[m]));
        else
          jfea_atom_cp(&tval, &(s1_ind[m]));
        jfea_atom_cp(&(d_ind[m]), &tval);
      }
      break;
    case 1:  /* sum-cros */
      for (m = 0; m < jfea_atom_c; m++)
      { if (s1_ind[m].atype == s2_ind[m].atype)
        { tval.atype = s1_ind[m].atype;
          tval.aart = s1_ind[m].aart;
          tval.val_1 = (((int) s1_ind[m].val_1) + ((int) s2_ind[m].val_1)) / 2;
          tval.val_2 = (((int) s1_ind[m].val_2) + ((int) s2_ind[m].val_2)) / 2;
        }
        else
          jfea_atom_cp(&tval, &(s1_ind[m]));
       jfea_atom_cp(&(d_ind[m]), &tval);
      }
      break;
    case 2: /* mutation */
      for (m = 0; m < jfea_atom_c; m++)
      { jfea_atom_cp(&tval, &(s1_ind[m]));
        jfea_atom_cp(&(d_ind[m]), &tval);
      }
      rno = jfea_random(jfea_rule_c);
      vno = jfea_random(jfea_rvar_c);
      jfea_new_atom(d_ind_no, rno, vno);
      break;
    case 3: /* creation */
      jfea_ind_create(d_ind_no);
      break;
    case 4: /* point cross */
      for (m = 0; m < jfea_atom_c; m++)
      { if (jfea_random(4) == 0)
          jfea_atom_cp(&tval, &(s1_ind[m]));
        else
          jfea_atom_cp(&tval, &(s2_ind[m]));
        jfea_atom_cp(&(d_ind[m]), &tval);
      }
      break;
    case 5: /* rule cros */
      m = 0;
      for (r = 0; r < jfea_rule_c; r++)
      { if (jfea_random(4) == 0)
          p = 1;
        else
          p = 0;
        for (e = 0; e < jfea_rvar_c; e++)
        { if (p == 0)
            jfea_atom_cp(&tval, &(s1_ind[m]));
          else
            jfea_atom_cp(&tval, &(s2_ind[m]));
          jfea_atom_cp(&(d_ind[m]), &tval);
        }
        m++;
      }
      break;
    case 6: /* stepmutation */
      for (m = 0; m < jfea_atom_c; m++)
      { jfea_atom_cp(&tval, &(s1_ind[m]));
        jfea_atom_cp(&(d_ind[m]), &tval);
      }
      jfea_mutate(d_ind_no, 0);
      jfea_rm_ind.succes = 1; /* temporary, rm by score-check. */
      break;
    case 7: /* repeat.mutation */
      for (m = 0; m < jfea_atom_c; m++)
      { jfea_atom_cp(&tval, &(s1_ind[m]));
        jfea_atom_cp(&(d_ind[m]), &tval);
      }
      jfea_mutate(d_ind_no, 1);
      jfea_rm_ind.succes = 1;
      break;
    case 8:  /* max-cros */
      for (m = 0; m < jfea_atom_c; m++)
      { if (s1_ind[m].atype == s2_ind[m].atype)
        { tval.atype = s1_ind[m].atype;
          tval.aart = s1_ind[m].aart;
          tval.val_1 = jfea_cmin(s1_ind[m].val_1, s2_ind[m].val_1);
          tval.val_2 = jfea_cmax(s1_ind[m].val_2, s2_ind[m].val_2);
        }
        else
        { if (s2_ind[m].atype == JFEA_AT_NONE)
            jfea_atom_cp(&tval, &(s2_ind[m]));
          else
            jfea_atom_cp(&tval, &(s1_ind[m]));
        }
       jfea_atom_cp(&(d_ind[m]), &tval);
      }
      break;
    case 9: /* min-cros */
      for (m = 0; m < jfea_atom_c; m++)
      { if (s1_ind[m].atype == s2_ind[m].atype)
        { tval.atype = s1_ind[m].atype;
          tval.aart = s1_ind[m].aart;
          tval.val_1 = jfea_cmax(s1_ind[m].val_1, s2_ind[m].val_1);
          tval.val_2 =jfea_cmin(s1_ind[m].val_2, s2_ind[m].val_2);
        }
        else
          jfea_atom_cp(&tval, &(s1_ind[m]));
       jfea_atom_cp(&(d_ind[m]), &tval);
      }
      break;
  }
  jfea_ind_check(d_ind_no);
  dif = jfea_ind_cmp(d_ind_no, s1_ind_no);
  if (dif == 1)
    dif = jfea_ind_cmp(d_ind_no, s2_ind_no);
  if (dif == 0)
  { method = 2;
    jfea_rm_ind.succes = 0;
    am_atoms = jfea_random(5) + 1;
    for (a = 0; a < am_atoms; a++)
    { rno = jfea_random(jfea_rule_c);
      vno = jfea_random(jfea_rvar_c);
      jfea_new_atom(d_ind_no, rno, vno);
    }
    jfea_ind_check(d_ind_no);
  }
  jfea_i2p(d_ind_no);
  jfea_cur_ind_no = d_ind_no;
  jfea_stat.method = method;
  child->score = jfea_judge();

  if (  (jfea_pop.maximize == 0 && child->score >= jfea_stat.p1_score)
      || (jfea_pop.maximize == 1 && child->score <= jfea_stat.p1_score))
    jfea_rm_ind.succes = 0;
}


/****************************************************************************/
/* Hjaelpe-funktioner til jfl_load                                          */
/****************************************************************************/


static void jfea_bobl(void)
{
   int m;
   int midt, rm_iprotect;
   float rm_score;
   short rm_ind_no;

   for (m = jfea_ind_c - 1; m > 0; m--)
   { if (   (jfea_pop.maximize == 0
             && jfea_pop.scores[m].score < jfea_pop.scores[m - 1].score)
         || (jfea_pop.maximize == 1
             && jfea_pop.scores[m].score > jfea_pop.scores[m - 1].score))
     { rm_score    = jfea_pop.scores[m - 1].score;
       rm_ind_no   = jfea_pop.scores[m - 1].ind_no;
       rm_iprotect = jfea_pop.scores[m - 1].iprotect;
       jfea_pop.scores[m - 1].score  = jfea_pop.scores[m].score;
       jfea_pop.scores[m - 1].ind_no = jfea_pop.scores[m].ind_no;
       jfea_pop.scores[m - 1].iprotect = jfea_pop.scores[m].iprotect;
       jfea_pop.scores[m].score      = rm_score;
       jfea_pop.scores[m].ind_no     = rm_ind_no;
       jfea_pop.scores[m].iprotect   = rm_iprotect;
       if (jfea_rm_ind.ind_no == jfea_pop.scores[m].ind_no)
         jfea_rm_ind.scores_id = m;
     }
   }

   if (jfea_pop.repro_c >= jfea_ind_c)
     midt = jfea_ind_c / 2;
   else
     midt = jfea_pop.repro_c / 2;
   jfea_stat.median_score = jfea_pop.scores[midt].score;
   jfea_stat.worst_score = jfea_pop.scores[jfea_ind_c - 1].score;
}

static void jfea_inds_create(void)
{                                /* skaber start-individerne */
  int i;

  for (i = 0; i < jfea_ind_c; i++)
  { jfea_ind_create(i);
    jfea_ind_check(i);
  }
}


static void jfea_pop_create(void)  /* judges initial generation */
{
  int m, midt;

  jfea_pop.c_q = 2.0 / ((float) jfea_ind_c);
  jfea_pop.c_r =   (2.0 * jfea_pop.c_q - 2.0 / ((float) jfea_ind_c))
                / (((float) jfea_ind_c) - 1.0);

  jfea_pop.repro_c = 0;
  for (m = 0; m < jfea_ind_c; m++)
  { jfea_pop.scores[m].ind_no = m;
    jfea_pop.scores[m].iprotect = 0;
    jfea_i2p(m);
    if (jfea_pop.repro_c >= jfea_ind_c)
      midt = jfea_ind_c / 2;
    else
      midt = jfea_pop.repro_c / 2;
    jfea_stat.median_score = jfea_pop.scores[midt].score;
    jfea_cur_ind_no = m;
    jfea_pop.scores[m].score = jfea_judge();
    jfea_pop.repro_c++;
  }
  jfea_bobl();
}



int jfea_init(void *jfr_head, int ind_c, int rule_c, int fixed_rules,
              int no_default)
{
  unsigned long size;
  int res, a, as, m, rs;
  struct jfea_atomd_desc *adesc;
  
  jfea_jraseed = (float) jfea_random(30000);
  jfea_fixed_rules = fixed_rules;
  jfea_pop.scores = NULL;
  jfea_ind_c = ind_c;
  jfea_rule_c = rule_c;
  jfea_no_default = no_default;
  jfea_pop.jfr_head = jfr_head;
  jfea_stat.worst_score = 0.0;
  jfea_stat.p1_score = 0.0;
  jfea_stat.p2_score = 0.0;
  jfea_stat.method = 3;
  jfg_sprg(&jfea_spdesc, jfr_head);
  /* analyse jfr-program, create conversion-table adesc */
  res = jfea_adesc_create();
  if (res != 0)
    return res;
  if (jfea_rvar_c > 0)
  { if (jfea_rule_c == 0)
    { jfea_rule_c = jfea_a_descs[0].fzvar_c;
      if (jfea_no_default == 1)
        jfea_rule_c--;
    }
    jfea_atom_c = jfea_rvar_c * jfea_rule_c;
    size = jfea_atom_c * jfea_ind_c * JFEA_ATOM_SIZE;
    if ((jfea_inds = (struct jfea_atom_desc *) malloc(size)) == NULL)
      return jfea_set_error(6, "individuals");
  }
  else
    return jfea_set_error(802, jfea_empty);

  size = jfea_ind_c * JFI_SCORE_SIZE;
  if ((jfea_pop.scores = (struct jfea_score_desc *) malloc(size)) == NULL)
  { free(jfea_inds);
    return jfea_set_error(6, "score table");
  }

  /* calculate size of jfg_tree: */
  rs = jfea_rvar_c + 4;
  for (m = 1; m < jfea_rvar_c; m++)
  { adesc = &(jfea_a_descs[m]);
    as = 3;
    for (a = 0; a < adesc->atypes_c; a++)
    { if (adesc->l_atypes[a] == JFEA_AT_VAL_BETWEEN && as < 7)
        as = 7;
      else
      if (adesc->l_atypes[a] == JFEA_AT_OP && as < 5)
        as = 5;
      else
      if (adesc->l_atypes[a] == JFEA_AT_IN)
      { if (adesc->fzvar_c > as)
          as = adesc->fzvar_c;
      }
    }
    rs += as;
  }
  if (rs < 128)
    rs = 128;
  size = sizeof(jfg_tree_desc) * rs;
  if ((jfea_tree = (struct jfg_tree_desc *) malloc(size)) == NULL)
  { free(jfea_pop.scores);
    free(jfea_inds);
    return jfea_set_error(6, "konversion-tree");
  }
  return 0;
}

void jfea_free(void)
{
  if (jfea_pop.scores != NULL)
  { free(jfea_pop.scores);
    jfea_pop.scores = NULL;
  }
  if (jfea_inds != NULL)
  { free(jfea_inds);
    jfea_inds = NULL;
  }
}

int jfea_run(float (*judge)(void), int maximize)
{
  jfea_slut = 0;
  jfea_rm_ind.succes = 0;
  jfea_pop.maximize = maximize;
  jfea_judge = judge;
  jfea_inds_create();
  jfea_pop_create();

  while (jfea_slut == 0)
  { jfea_ind_breed();
    jfea_pop.repro_c++;
    jfea_bobl();
  }
  jfea_i2p(jfea_pop.scores[0].ind_no);

  return 0;
}

void jfea_stop(void)
{
  jfea_slut = 1;
}

long jfea_protect(void)
{
  int m;

  for (m = 0; m < jfea_ind_c; m++)
  { if (jfea_pop.scores[m].ind_no == jfea_cur_ind_no)
    { jfea_pop.scores[m].iprotect = 1;
      break;
    }
  }
  return jfea_cur_ind_no;
}

void jfea_un_protect(long ind_id)
{
  int m;

  for (m = 0; m < jfea_ind_c; m++)
  { if (jfea_pop.scores[m].ind_no == ind_id)
    { jfea_pop.scores[m].iprotect = 0;
      return;
    }
  }
}


void jfea_ind2jfr(long ind_id)
{
  jfea_cur_ind_no = ind_id;
  jfea_i2p(ind_id);
}






