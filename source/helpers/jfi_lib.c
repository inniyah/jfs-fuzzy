  /***********************************************************************/
  /*                                                                     */
  /* jfi_lib.cpp  Version  2.01 Copyright (c) 1999-2000 Jan E. Mortensen */
  /*                                                                     */
  /* JFS improver-functions using evolutionary                           */
  /* programing.                                                         */
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
#include "jfi_lib.h"

/************************************************************************/
/* Beskrivelse af individerne                                           */
/************************************************************************/

struct jfi_stat_rec jfi_stat;

static float  *jfi_f_inds  = NULL; /* individerne (float-del).    */

static long   jfi_ind_c = 40;    /* antal individer                     */

static int    jfi_f_atom_c;      /* antal float-atomer pr individ.      */
static int    jfi_slut;

struct jfi_score_desc
         { float        score;
           signed short ind_no;
         };

#define JFI_SCORE_SIZE sizeof(struct jfi_score_desc)


struct jfi_pop_desc { struct jfi_score_desc *scores;
                      long  repro_c;
                      float c_q;       /* konstanter til rank-selection */
                      float c_r;
                      void *jfr_head;
                      int maximize;
                    };

static float (*jfi_judge)(void);

static struct jfi_pop_desc jfi_pop;  /* Grund-befolkningen           */

static struct jfg_sprog_desc jfi_spdesc;

/*********************************************************************/
/* Individ-beskrivelse med Konvertering indived/program-data.        */
/*********************************************************************/


#define JFI_ADJ_CENTER     0
#define JFI_ADJ_BASE       1

#define JFI_OPERATOR_ARG   2

#define JFI_NORMAL         3
#define JFI_DEFUZFUNC      4
#define JFI_ALFACUT        5

#define JFI_HEDGEARG       6

#define JFI_WEIGHT         7

#define JFI_ADJ_PLX        8
#define JFI_REL_PLX        9
#define JFI_HED_PLX       10

#define JFI_ADJ_PLY       11
#define JFI_REL_PLY       12
#define JFI_HED_PLY       13

#define JFI_PROGCONST     14

#define JFI_TRAP_START    15
#define JFI_TRAP_END      16

struct jfi_atomd_desc { unsigned short address;  /* see below  */
                        unsigned short function_no;
                        char           type;
                        unsigned char  arg;      /* see below  */
                      };

/* The meaning of the field depeends on the type:              */
/* JFI_ADJ_CENTER:    <address>=adjectiv-no, <arg>=1 if first adjectiv */
/*                    in a group else <arg>=0.                         */
/* JFI_ADJ_BASE:      <addres>=adjectiv-no. <arg>= 0 (val in ]0,inf[). */
/* JFI_OPERATOR_ARG:  <address>=operator_no. <arg>=0 (val in ]0,inf[,  */
/*                    or <arg>=2 (val in [0,1]).                       */
/* JFI_NORMARG:       <address>=var_no. arg=3 (val in ]0,1]).          */
/* JFI_DEFUZFUNC:     <adsress>=var-no. <arg>=2.                       */
/* JFI_ALFACUT:       <address>=var-no. <arg>=2 (val in [0,1]).        */
/* JFI_HEDGEARG:      <address>=hedge-no. arg in 0,2,3.                */
/* JFI_WEIGTH:        <function_no>=function_no (fucntion_c for main), */
/*                    <address>=pc (relativ to pc_start in function).  */
/*                     <arg>=2.                                        */
/* JFI_ADJ_PLX:       <address>=adjectiv-no. <arg>=pl-point-no.        */
/* JFI_REL_PLX:       <address>=relation-no. <arg>=pl-point-no.        */
/* JFI_HED_PLX:       <address>=hedge-no. <arg>=pl-point-no.           */
/* JFI_ADJ_PLY:       <address>=adjectiv-no. <arg>=pl-point-no.        */
/* JFI_REL_PLY:       <address>=relation-no. <arg>=pl-point-no.        */
/* JFI_HED_PLY:       <address>=hedge-no. <arg>=pl-point-no.           */
/* JFI_PROGCONST:     <function_no>=function_no (function_c for main), */
/*                    <address>=pc (relativ to function.pc_start).     */
/*                    <arg>=4   (val in ]-inf,inf[).                   */
/* JFI_TRAP_START     <address>=adjectiv-no,                           */
/* JFI_TRAP_END       <address>=adjectiv-no.                           */

#define JFI_ATOMD_SIZE sizeof(struct jfi_atomd_desc)

static struct jfi_atomd_desc *jfi_a_descs = NULL; /* a_descs[atom_c].   */

static struct jfg_limit_desc jfi_limits[128];



static float jfi_imut_pct = 30.0;
     /* pct mutationer ved skabelse af poputaion.  */


/*************************************************************************/
/* Diverse                                                               */
/*************************************************************************/

static float jfi_jraseed; /* seed til intern random number generator */

static void jfi_p_ad_create(int mode);
static void jfi_pl_create(char type, int address, int limit_c, int mode);
static void jfi_adesc_create(int mode);
static int jfi_random(int sup);
static float jfi_rand_dget(void);
static float jfi_rand_iv_dget(float iinf, float isup);
static int jfi_ind_rand(void);
static int jfi_p2i(long ind_no);
static void jfi_f2p(long atom_no, float val);
static void jfi_ai2p(long source_no, long atom_no);
static void jfi_i2p(long source_no);
static float jfi_mut_pl(int id, int limit_c);
static void jfi_mutate(int ind_no);
static int jfi_ind_rm(void);
static int jfi_cp_test(int cp);
static void jfi_ind_create(void);
static void jfi_bobl(void);
static void jfi_inds_create(void);
static void jfi_pop_create(void);

/*************************************************************************/
/* Generering af konverteringstabel jfi_a_descs                          */
/*************************************************************************/

static void jfi_p_ad_create(int mode)   /* mode=0: tael, 1:indsaet */
{                                       /* konvertering af rules-del */
  unsigned char *pc;
  unsigned char *pc_start;
  unsigned char *epc;
  struct jfg_statement_desc sdesc;
  struct jfg_function_desc fdesc;
  struct jfg_oc_desc oc;
  int m;

  for (m = 0; m <= jfi_spdesc.function_c; m++)  /* ekstra m: main */
  { if (m < jfi_spdesc.function_c)
    { jfg_function(&fdesc, jfi_pop.jfr_head, m);
      pc = fdesc.pc;
    }
    else
      pc = jfi_spdesc.pc_start;
    pc_start = pc;
    jfg_statement(&sdesc, jfi_pop.jfr_head, pc);
    while (!(sdesc.type == JFG_ST_EOP
             || (sdesc.type == JFG_ST_STEND && sdesc.sec_type == 2)))
    { switch (sdesc.type)
      { case JFG_ST_IF:
          if (sdesc.flags == 3)    /* weight og learn weight */
          { if (mode == 1)
            { jfi_a_descs[jfi_f_atom_c].type = JFI_WEIGHT;
              jfi_a_descs[jfi_f_atom_c].function_no = m;
              jfi_a_descs[jfi_f_atom_c].address
                      = pc - pc_start;
              jfi_a_descs[jfi_f_atom_c].arg = 2;
            }
            jfi_f_atom_c++;
          }
          break;
      }
      pc = sdesc.n_pc;
      jfg_statement(&sdesc, jfi_pop.jfr_head, pc);
    }
  }


  for (m = 0; m <= jfi_spdesc.function_c; m++)  /* ekstra m: main */
  { if (m < jfi_spdesc.function_c)
    { jfg_function(&fdesc, jfi_pop.jfr_head, m);
      pc = fdesc.pc;
    }
    else
      pc = jfi_spdesc.pc_start;
    pc_start = pc;
    jfg_statement(&sdesc, jfi_pop.jfr_head, pc);
    while (sdesc.type != JFG_ST_EOP)
    { switch (sdesc.type)
      { case JFG_ST_IF:
          epc = pc;
          jfg_oc(&oc, jfi_pop.jfr_head, epc);
          while (oc.type != JFG_TT_EOE)
          { if (oc.type == JFG_TT_CONST)
            { if (oc.op == 1)  /* % */
              { if (mode == 1)
                { jfi_a_descs[jfi_f_atom_c].type = JFI_PROGCONST;
                  jfi_a_descs[jfi_f_atom_c].address
                    = epc - pc_start;
                  jfi_a_descs[jfi_f_atom_c].function_no = m;  
                  jfi_a_descs[jfi_f_atom_c].arg = 4;
                }
                jfi_f_atom_c++;
              }
            }
            epc = oc.n_pc;
            jfg_oc(&oc, jfi_pop.jfr_head, epc);
          }
          break;
      }
      pc = sdesc.n_pc;
      jfg_statement(&sdesc, jfi_pop.jfr_head, pc);
    }
  }
}

static void jfi_pl_create(char type, int address, int limit_c, int mode)
{
  int m;

  for (m = 0; m < limit_c; m++)
  { if (jfi_limits[m].flags & JFS_LF_IX)
    { if (mode == 1)
      { jfi_a_descs[jfi_f_atom_c].type = type;
        jfi_a_descs[jfi_f_atom_c].address = address;
        jfi_a_descs[jfi_f_atom_c].arg = m;
      }
      jfi_f_atom_c++;
    }
    if (jfi_limits[m].flags & JFS_LF_IY)
    { if (mode == 1)
      { if (type == JFI_ADJ_PLX)
          jfi_a_descs[jfi_f_atom_c].type = JFI_ADJ_PLY;
        else
        if (type == JFI_REL_PLX)
          jfi_a_descs[jfi_f_atom_c].type = JFI_REL_PLY;
        else
          jfi_a_descs[jfi_f_atom_c].type = JFI_HED_PLY;
        jfi_a_descs[jfi_f_atom_c].address = address;
        jfi_a_descs[jfi_f_atom_c].arg = m;
      }
      jfi_f_atom_c++;
    }
  }
}

static void jfi_adesc_create(int mode)
{
        /* mode = 0: tael,  */
        /*        1: insaet.*/
  unsigned short m;
  int fundet, v;
  struct jfg_adjectiv_desc adesc;
  struct jfg_var_desc      vdesc;
  struct jfg_hedge_desc    hdesc;
  struct jfg_domain_desc   ddesc;
  struct jfg_operator_desc odesc;
  struct jfg_relation_desc reldesc;

  jfi_f_atom_c = 0;

  for (m = 0; m < jfi_spdesc.adjectiv_c; m++)
  { jfg_adjectiv(&adesc, jfi_pop.jfr_head, m);
    if ((adesc.flags & JFS_AF_ICENTER) != 0)
    { if (mode == 1)
      { jfi_a_descs[jfi_f_atom_c].type = JFI_ADJ_CENTER;
        jfi_a_descs[jfi_f_atom_c].address = m;
        jfi_a_descs[jfi_f_atom_c].arg = 0;

        /* Undersoeg om dette adjectiv er foerste eller sidste i sin gruppe*/
        jfg_domain(&ddesc, jfi_pop.jfr_head, adesc.domain_no);
        if (ddesc.adjectiv_c > 0 &&
            m >= ddesc.f_adjectiv_no &&
            m < ddesc.f_adjectiv_no + ddesc.adjectiv_c)
        { if (m == ddesc.f_adjectiv_no)
            jfi_a_descs[jfi_f_atom_c].arg = 1;
        }
        else
        { fundet = 0;
          for (v = 0; fundet == 0 && v < jfi_spdesc.var_c; v++)
          { jfg_var(&vdesc, jfi_pop.jfr_head, v);
            if (vdesc.fzvar_c > 0 &&
              m >= vdesc.f_adjectiv_no &&
              m < vdesc.f_adjectiv_no + vdesc.fzvar_c)
            { if (m == vdesc.f_adjectiv_no)
                jfi_a_descs[jfi_f_atom_c].arg = 1;
              fundet = 1;
            }
          }
        }
      }
      jfi_f_atom_c++;
    }
  }
  for (m = 0; m < jfi_spdesc.adjectiv_c; m++)
  { jfg_adjectiv(&adesc, jfi_pop.jfr_head, m);
    if ((adesc.flags & JFS_AF_ISTRAPEZ) != 0)
    { if (mode == 1)
      { jfi_a_descs[jfi_f_atom_c].type = JFI_TRAP_START;
        jfi_a_descs[jfi_f_atom_c].address = m;
        jfi_a_descs[jfi_f_atom_c].arg = 0;
      }
      jfi_f_atom_c++;
    }
    if ((adesc.flags & JFS_AF_IETRAPEZ) != 0)
    { if (mode == 1)
      { jfi_a_descs[jfi_f_atom_c].type = JFI_TRAP_END;
        jfi_a_descs[jfi_f_atom_c].address = m;
        jfi_a_descs[jfi_f_atom_c].arg = 0;
      }
      jfi_f_atom_c++;
    }
  }

  for (m = 0; m < jfi_spdesc.adjectiv_c; m++)
  { jfg_adjectiv(&adesc, jfi_pop.jfr_head, m);
    if ((adesc.flags & JFS_AF_IBASE) != 0)
    { if (mode == 1)
      { jfi_a_descs[jfi_f_atom_c].type = JFI_ADJ_BASE;
        jfi_a_descs[jfi_f_atom_c].address = m;
        jfi_a_descs[jfi_f_atom_c].arg = 1;
      }
      jfi_f_atom_c++;
    }
    jfg_alimits(jfi_limits, jfi_pop.jfr_head, m);
    jfi_pl_create(JFI_ADJ_PLX, m, adesc.limit_c, mode);
  }

  for (m = 0; m < jfi_spdesc.var_c; m++)
  { jfg_var(&vdesc, jfi_pop.jfr_head, m);
    if ((vdesc.flags & JFS_VF_IACUT) != 0)
    { if (mode == 1)
      { jfi_a_descs[jfi_f_atom_c].type = JFI_ALFACUT;
        jfi_a_descs[jfi_f_atom_c].address = m;
        jfi_a_descs[jfi_f_atom_c].arg = 2;
      }
      jfi_f_atom_c++;
    }
    if ((vdesc.flags & JFS_VF_IDEFUZ) != 0)
    { if (mode == 1)
      { jfi_a_descs[jfi_f_atom_c].type = JFI_DEFUZFUNC;
        jfi_a_descs[jfi_f_atom_c].address = m;
        jfi_a_descs[jfi_f_atom_c].arg = 2;
      }
      jfi_f_atom_c++;
    }
    if ((vdesc.flags & JFS_VF_INORMAL) != 0)
    { if (mode == 1)
      { jfi_a_descs[jfi_f_atom_c].type = JFI_NORMAL;
        jfi_a_descs[jfi_f_atom_c].address = m;
        jfi_a_descs[jfi_f_atom_c].arg = 2;
      }
      jfi_f_atom_c++;
    }
  }

  for (m = 0; m < jfi_spdesc.hedge_c; m++)
  { jfg_hedge(&hdesc, jfi_pop.jfr_head, m);
    if ((hdesc.flags & JFS_HF_IARG) != 0 && hdesc.type != JFS_HT_LIMITS)
    { if (mode == 1)
      { jfi_a_descs[jfi_f_atom_c].type = JFI_HEDGEARG;
        jfi_a_descs[jfi_f_atom_c].address = m;
        if (hdesc.type == JFS_HT_POWER || hdesc.type == JFS_HT_YNOT)
          jfi_a_descs[jfi_f_atom_c].arg = 1;
        else
        if (hdesc.type == JFS_HT_BELL)
          jfi_a_descs[jfi_f_atom_c].arg = 3;
        else
          jfi_a_descs[jfi_f_atom_c].arg = 2;
      }
      jfi_f_atom_c++;
    }
    if ((hdesc.flags & JFS_HF_IARG) != 0 && hdesc.type == JFS_HT_LIMITS)
    { jfg_hlimits(jfi_limits, jfi_pop.jfr_head, m);
      jfi_pl_create(JFI_HED_PLX, m, hdesc.limit_c, mode);
    }
  }

  for (m = 0; m < jfi_spdesc.operator_c; m++)
  { jfg_operator(&odesc, jfi_pop.jfr_head, m);
    if ((odesc.flags & JFS_OF_IARG) != 0)
    { if (mode == 1)
      { jfi_a_descs[jfi_f_atom_c].type = JFI_OPERATOR_ARG;
        jfi_a_descs[jfi_f_atom_c].address = m;
        if (odesc.op_2 == odesc.op_1)
          jfi_a_descs[jfi_f_atom_c].arg = 0;
        else
          jfi_a_descs[jfi_f_atom_c].arg = 2;
      }
      jfi_f_atom_c++;
    }
  }

  for (m = 0; m < jfi_spdesc.relation_c; m++)
  { jfg_relation(&reldesc, jfi_pop.jfr_head, m);
    jfg_rlimits(jfi_limits, jfi_pop.jfr_head, m);
    jfi_pl_create(JFI_REL_PLX, m, reldesc.limit_c, mode);
  }
  jfi_p_ad_create(mode);
}


/*************************************************************************/
/* Hjaelpe funktioner til crosover og mutation                           */
/*************************************************************************/

static int jfi_random(int sup)
{
  int res;

  res = (int)(rand() * ((float) sup) / (RAND_MAX+1.0));
  /* res = rand() % sup; */
  return res;
}

static float jfi_rand_dget(void)
{
  float husk, new_seed;

  husk = jfi_jraseed * 24298.0 + 99991.0;

  new_seed = husk - floor(husk / 199017.0) * 199017.0;
  if (new_seed == jfi_jraseed)
  { printf(" ERROR in random numbers !!!. jraseed: %f\n\a\a", new_seed);
    new_seed = 1.0 * jfi_random(30000);
  }
  jfi_jraseed = new_seed;
  return (jfi_jraseed / 199017.0);
}

static float jfi_rand_iv_dget(float iinf, float isup)
{
  float f;

  f = jfi_rand_dget();
  return  f * (isup - iinf) + iinf;
}

static int jfi_ind_rand(void)
{
  int m;
  float r, ssum;

  r = jfi_rand_dget();
  ssum = 0.0;
  for (m = 0; m < jfi_ind_c; m++)
  { ssum += jfi_pop.c_q - jfi_pop.c_r * (float) m;
    if (r < ssum)
      return m;
  }
  return 0;
}


/*************************************************************************/
/* Konverteringsfunktioner                                               */
/*************************************************************************/

static int jfi_p2i(long ind_no)
{
  /* kopier et jfr_program til et individ. */

  long m, dadr;
  int  adr, fno;
  int cu_pl_adj, cu_pl_hed, cu_pl_rel;
  unsigned char *pc_start;
  unsigned char arg;
  struct jfg_adjectiv_desc  adesc;
  struct jfg_var_desc       vdesc;
  struct jfg_hedge_desc     hdesc;
  struct jfg_operator_desc  odesc;
  struct jfg_statement_desc sdesc;
  struct jfg_function_desc  fdesc;
  struct jfg_oc_desc        oc;

  dadr = ind_no * jfi_f_atom_c;

  cu_pl_adj = cu_pl_hed = cu_pl_rel = -1;

  for (m = 0; m < jfi_f_atom_c; m++, dadr++)
  { adr = jfi_a_descs[m].address;
    arg = jfi_a_descs[m].arg;
    switch (jfi_a_descs[m].type)
    { case JFI_ADJ_CENTER:
        jfg_adjectiv(&adesc, jfi_pop.jfr_head, adr);
        jfi_f_inds[dadr] = adesc.center;
        break;
      case JFI_TRAP_START:
        jfg_adjectiv(&adesc, jfi_pop.jfr_head, adr);
        jfi_f_inds[dadr] = adesc.trapez_start;
        break;
      case JFI_TRAP_END:
        jfg_adjectiv(&adesc, jfi_pop.jfr_head, adr);
        jfi_f_inds[dadr] = adesc.trapez_end;
        break;
      case JFI_ADJ_BASE:
        jfg_adjectiv(&adesc, jfi_pop.jfr_head, adr);
        jfi_f_inds[dadr] = adesc.base;
        break;
      case JFI_NORMAL:
        jfg_var(&vdesc, jfi_pop.jfr_head, adr);
        jfi_f_inds[dadr] = vdesc.no_arg;
        break;
      case JFI_DEFUZFUNC:
        jfg_var(&vdesc, jfi_pop.jfr_head, adr);
        jfi_f_inds[dadr] = vdesc.defuz_arg;
        break;
      case JFI_ALFACUT:
        jfg_var(&vdesc, jfi_pop.jfr_head, adr);
        jfi_f_inds[dadr] = vdesc.acut;
        break;
      case JFI_HEDGEARG:
        jfg_hedge(&hdesc, jfi_pop.jfr_head, adr);
        jfi_f_inds[dadr] = hdesc.hedge_arg;
        break;
      case JFI_OPERATOR_ARG:
        jfg_operator(&odesc, jfi_pop.jfr_head, adr);
        jfi_f_inds[dadr] = odesc.op_arg;
        break;
      case JFI_WEIGHT:
        fno = jfi_a_descs[m].function_no;
        if (fno == jfi_spdesc.function_c)
          pc_start = jfi_spdesc.pc_start;
        else
        { jfg_function(&fdesc, jfi_pop.jfr_head, fno);
          pc_start = fdesc.pc;
        }
        jfg_statement(&sdesc, jfi_pop.jfr_head,
                      pc_start + adr);
        jfi_f_inds[dadr] = sdesc.farg;
        break;
      case JFI_HED_PLX:
        if (adr != cu_pl_hed)
          jfg_hlimits(jfi_limits, jfi_pop.jfr_head, adr);
        cu_pl_hed = adr;
        jfi_f_inds[dadr] = jfi_limits[arg].limit;
        break;
      case JFI_HED_PLY:
        if (adr != cu_pl_hed)
          jfg_hlimits(jfi_limits, jfi_pop.jfr_head, adr);
        cu_pl_hed = adr;
        jfi_f_inds[dadr] = jfi_limits[arg].value;
        break;
      case JFI_REL_PLX:
        if (adr != cu_pl_rel)
          jfg_rlimits(jfi_limits, jfi_pop.jfr_head, adr);
        cu_pl_rel = adr;
        jfi_f_inds[dadr] = jfi_limits[arg].limit;
        break;
      case JFI_REL_PLY:
        if (adr != cu_pl_rel)
          jfg_rlimits(jfi_limits, jfi_pop.jfr_head, adr);
        cu_pl_rel = adr;
        jfi_f_inds[dadr] = jfi_limits[arg].value;
        break;
      case JFI_ADJ_PLX:
        if (adr != cu_pl_adj)
          jfg_alimits(jfi_limits, jfi_pop.jfr_head, adr);
        cu_pl_adj = adr;
        jfi_f_inds[dadr] = jfi_limits[arg].limit;
        break;
      case JFI_ADJ_PLY:
        if (adr != cu_pl_adj)
          jfg_alimits(jfi_limits, jfi_pop.jfr_head, adr);
        cu_pl_adj = adr;
        jfi_f_inds[dadr] = jfi_limits[arg].value;
        break;
      case JFI_PROGCONST:
        fno = jfi_a_descs[m].function_no;
        if (fno == jfi_spdesc.function_c)
          pc_start = jfi_spdesc.pc_start;
        else
        { jfg_function(&fdesc, jfi_pop.jfr_head, fno);
          pc_start = fdesc.pc;
        }
        jfg_oc(&oc, jfi_pop.jfr_head, pc_start + adr);
        jfi_f_inds[dadr] = oc.farg;
        break;
    }
  }
  return 0;
}

static void jfi_f2p(long atom_no, float val)
{
  /* Retter jfr-program. saetter atom nr <atom_no> <f> */
  /* retter ikke i individer.                          */

  struct jfg_adjectiv_desc  adesc;
  struct jfg_var_desc       vdesc;
  struct jfg_hedge_desc     hdesc;
  struct jfg_operator_desc  odesc;
  struct jfg_statement_desc sdesc;
  struct jfg_function_desc  fdesc;
  struct jfi_atomd_desc *atbesk;
  struct jfg_oc_desc        oc;
  unsigned char *pc_start;

  atbesk = &(jfi_a_descs[atom_no]);
  switch (atbesk->type)
  { case JFI_ADJ_CENTER:
      jfg_adjectiv(&adesc, jfi_pop.jfr_head, atbesk->address);
      adesc.center = val;
      jfp_adjectiv(jfi_pop.jfr_head, atbesk->address, &adesc);
      break;
    case JFI_TRAP_START:
      jfg_adjectiv(&adesc, jfi_pop.jfr_head, atbesk->address);
      adesc.trapez_start = val;
      jfp_adjectiv(jfi_pop.jfr_head, atbesk->address, &adesc);
      break;
    case JFI_TRAP_END:
      jfg_adjectiv(&adesc, jfi_pop.jfr_head, atbesk->address);
      adesc.trapez_end = val;
      jfp_adjectiv(jfi_pop.jfr_head, atbesk->address, &adesc);
      break;
    case JFI_ADJ_BASE:
      jfg_adjectiv(&adesc, jfi_pop.jfr_head, atbesk->address);
      adesc.base = val;
      jfp_adjectiv(jfi_pop.jfr_head, atbesk->address, &adesc);
      break;
    case JFI_NORMAL:
      jfg_var(&vdesc, jfi_pop.jfr_head, atbesk->address);
      vdesc.no_arg = val;
      jfp_var(jfi_pop.jfr_head, atbesk->address, &vdesc);
      break;
    case JFI_DEFUZFUNC:
      jfg_var(&vdesc, jfi_pop.jfr_head, atbesk->address);
      vdesc.defuz_arg = val;
      jfp_var(jfi_pop.jfr_head, atbesk->address, &vdesc);
      break;
    case JFI_ALFACUT:
      jfg_var(&vdesc, jfi_pop.jfr_head, atbesk->address);
      vdesc.acut = val;
      jfp_var(jfi_pop.jfr_head, atbesk->address, &vdesc);
      break;
    case JFI_HEDGEARG:
      jfg_hedge(&hdesc, jfi_pop.jfr_head, atbesk->address);
      hdesc.hedge_arg = val;
      jfp_hedge(jfi_pop.jfr_head, atbesk->address, &hdesc);
      break;
    case JFI_OPERATOR_ARG:
      jfg_operator(&odesc, jfi_pop.jfr_head, atbesk->address);
      odesc.op_arg = val;
      jfp_operator(jfi_pop.jfr_head, atbesk->address, &odesc);
      break;
    case JFI_WEIGHT:
      if (atbesk->function_no == jfi_spdesc.function_c)
        pc_start = jfi_spdesc.pc_start;
      else
      { jfg_function(&fdesc, jfi_pop.jfr_head, atbesk->function_no);
        pc_start = fdesc.pc;
      }
      jfg_statement(&sdesc, jfi_pop.jfr_head,
                    pc_start +  atbesk->address);
      sdesc.farg = val;
      jfp_statement(pc_start + atbesk->address, &sdesc);
      break;
    case JFI_PROGCONST:
      if (atbesk->function_no == jfi_spdesc.function_c)
        pc_start = jfi_spdesc.pc_start;
      else
      { jfg_function(&fdesc, jfi_pop.jfr_head, atbesk->function_no);
        pc_start = fdesc.pc;
      }
      jfg_oc(&oc, jfi_pop.jfr_head,
             pc_start + atbesk->address);
      oc.farg = val;
      jfp_u_oc(jfi_pop.jfr_head,
               pc_start + atbesk->address, &oc);
      break;
    case JFI_ADJ_PLX:
      jfg_alimits(jfi_limits, jfi_pop.jfr_head, atbesk->address);
      jfi_limits[atbesk->arg].limit = val;
      jfp_alimits(jfi_pop.jfr_head, atbesk->address, jfi_limits);
      break;
    case JFI_ADJ_PLY:
      jfg_alimits(jfi_limits, jfi_pop.jfr_head, atbesk->address);
      jfi_limits[atbesk->arg].value = val;
      jfp_alimits(jfi_pop.jfr_head, atbesk->address, jfi_limits);
      break;
    case JFI_HED_PLX:
      jfg_hlimits(jfi_limits, jfi_pop.jfr_head, atbesk->address);
      jfi_limits[atbesk->arg].limit = val;
      jfp_hlimits(jfi_pop.jfr_head, atbesk->address, jfi_limits);
      break;
    case JFI_HED_PLY:
      jfg_hlimits(jfi_limits, jfi_pop.jfr_head, atbesk->address);
      jfi_limits[atbesk->arg].value = val;
      jfp_hlimits(jfi_pop.jfr_head, atbesk->address, jfi_limits);
      break;
    case JFI_REL_PLX:
      jfg_rlimits(jfi_limits, jfi_pop.jfr_head, atbesk->address);
      jfi_limits[atbesk->arg].limit = val;
      jfp_rlimits(jfi_pop.jfr_head, atbesk->address, jfi_limits);
      break;
    case JFI_REL_PLY:
      jfg_rlimits(jfi_limits, jfi_pop.jfr_head, atbesk->address);
      jfi_limits[atbesk->arg].value = val;
      jfp_rlimits(jfi_pop.jfr_head, atbesk->address, jfi_limits);
      break;

  }
}


static void jfi_ai2p(long source_no, long atom_no)
{
  /* kopierer et atom fra individ til jfr_progam. */

  float val;

  val = jfi_f_inds[source_no * jfi_f_atom_c + atom_no];
  jfi_f2p(atom_no, val);
}


static void jfi_i2p(long source_no)
{
  /* kopierer et individ til jfr_progammet. */

  long m;

  for (m = 0; m < jfi_f_atom_c; m++)
    jfi_ai2p(source_no, m);
}


/*************************************************************************/
/* mutation/crosover funktioner                                          */
/*************************************************************************/

static float jfi_mut_pl(int id, int limit_c)
{
  float center, pre, post, res;

  center = jfi_limits[id].limit;
  pre = post = center;
  if (id > 0)
    pre = jfi_limits[id - 1].limit;
  if (id + 1 < limit_c)
    post = jfi_limits[id + 1].limit;
  if (pre >= center)
    pre = center - (post - center);
  if (post <= center)
    post = center + (center - pre);
  res = jfi_rand_iv_dget(pre, post);
  jfi_limits[id].limit = res;
  return res;
}

static void jfi_mutate(int ind_no)
       /* forudsaetter aktuelle program = ind_no. Retter baade i program */
       /* og i individ.                                                  */
{
  int ano;
  float *ind_adr;
  float center, pre, post;
  struct jfg_adjectiv_desc adesc;
  struct jfg_hedge_desc hdesc;
  struct jfg_relation_desc rdesc;
  struct jfg_domain_desc ddesc;

  ind_adr = &(jfi_f_inds[ind_no * jfi_f_atom_c]);
  ano = jfi_random(jfi_f_atom_c);
  switch (jfi_a_descs[ano].type)
  { case JFI_ADJ_CENTER:
      jfg_adjectiv(&adesc, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      center = adesc.center;
      pre = post = center;
      jfg_domain(&ddesc, jfi_pop.jfr_head, adesc.domain_no);
      if (jfi_a_descs[ano].address > 0)
      { jfg_adjectiv(&adesc, jfi_pop.jfr_head, jfi_a_descs[ano].address - 1);
        pre = adesc.center;
      }
      if (jfi_a_descs[ano].address + 1 < jfi_spdesc.adjectiv_c)
      { jfg_adjectiv(&adesc, jfi_pop.jfr_head, jfi_a_descs[ano].address + 1);
        post = adesc.center;
      }
      if (pre >= center)
        pre = center - (post - center);
      if (post <= center)
        post = center + (center - pre);

      if ((ddesc.flags & JFS_DF_MINENTER) != 0
          && pre < ddesc.dmin)
        pre = ddesc.dmin;
      if ((ddesc.flags & JFS_DF_MAXENTER) != 0
          && post > ddesc.dmax)
        post = ddesc.dmax;
      ind_adr[ano] = jfi_rand_iv_dget(pre, post);
      break;
    case JFI_TRAP_START:
      jfg_adjectiv(&adesc, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      center = adesc.trapez_start;
      pre = center;
      post = adesc.trapez_end;
      jfg_domain(&ddesc, jfi_pop.jfr_head, adesc.domain_no);
      if (jfi_a_descs[ano].address > 0)
      { jfg_adjectiv(&adesc, jfi_pop.jfr_head, jfi_a_descs[ano].address - 1);
        pre = adesc.trapez_end;
      }
      if (pre >= center)
        pre = center - (post - center);

      if ((ddesc.flags & JFS_DF_MINENTER) != 0
          && pre < ddesc.dmin)
        pre = ddesc.dmin;
      ind_adr[ano] = jfi_rand_iv_dget(pre, post);
      break;
    case JFI_TRAP_END:
      jfg_adjectiv(&adesc, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      center = adesc.trapez_end;
      pre = adesc.trapez_start;
      post = center;
      jfg_domain(&ddesc, jfi_pop.jfr_head, adesc.domain_no);
      if (jfi_a_descs[ano].address + 1 < jfi_spdesc.adjectiv_c)
      { jfg_adjectiv(&adesc, jfi_pop.jfr_head, jfi_a_descs[ano].address + 1);
        post = adesc.trapez_start;
      }
      if (post <= center)
        post = center + (center - pre);

      if ((ddesc.flags & JFS_DF_MAXENTER) != 0
          && post > ddesc.dmax)
        post = ddesc.dmax;
      ind_adr[ano] = jfi_rand_iv_dget(pre, post);
      break;
    case JFI_ADJ_PLY:
      jfg_adjectiv(&adesc, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      jfg_alimits(jfi_limits, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      ind_adr[ano] = jfi_rand_dget();
      break;
    case JFI_ADJ_PLX:
      jfg_adjectiv(&adesc, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      jfg_alimits(jfi_limits, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      ind_adr[ano] = jfi_mut_pl(jfi_a_descs[ano].arg, adesc.limit_c);
      break;
    case JFI_HED_PLY:
      jfg_hedge(&hdesc, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      jfg_hlimits(jfi_limits, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      ind_adr[ano] = jfi_rand_dget();
      break;
    case JFI_HED_PLX:
      jfg_hedge(&hdesc, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      jfg_hlimits(jfi_limits, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      ind_adr[ano] = jfi_mut_pl(jfi_a_descs[ano].arg, hdesc.limit_c);
      break;
    case JFI_REL_PLY:
      jfg_relation(&rdesc, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      jfg_rlimits(jfi_limits, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      ind_adr[ano] = jfi_rand_dget();
      break;
    case JFI_REL_PLX:
      jfg_relation(&rdesc, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      jfg_rlimits(jfi_limits, jfi_pop.jfr_head, jfi_a_descs[ano].address);
      ind_adr[ano] = jfi_mut_pl(jfi_a_descs[ano].arg, rdesc.limit_c);
      break;
    default:
      if (jfi_a_descs[ano].arg == 0 ||
          jfi_a_descs[ano].arg == 1)
        ind_adr[ano] = ind_adr[ano] * jfi_rand_iv_dget(0.0, 2.0);
      else
      if (jfi_a_descs[ano].arg == 4)
        ind_adr[ano] = ind_adr[ano] * jfi_rand_iv_dget(-2.0, 2.0);
      else
        ind_adr[ano] = jfi_rand_dget();
      break;
  }
  jfi_f2p(ano, ind_adr[ano]);
}


static int jfi_ind_rm(void)
{
  int sno;

  sno = jfi_ind_c - jfi_ind_rand() - 1;
  if (sno <= 0)
    sno = 1;
  jfi_stat.old_score = jfi_pop.scores[sno].score;
  return sno;
}

static int jfi_cp_test(int cp)
    /* tester om cros-over kan laves i punkt cp */
{
  int res;

  res = 1; /* ja */

  if (jfi_a_descs[cp].type == JFI_ADJ_CENTER
      && jfi_a_descs[cp].arg == 0)
    res = 0;
  if (jfi_a_descs[cp].type == JFI_ADJ_PLX
       || jfi_a_descs[cp].type == JFI_ADJ_PLY
       || jfi_a_descs[cp].type == JFI_HED_PLX
       || jfi_a_descs[cp].type == JFI_HED_PLY
       || jfi_a_descs[cp].type == JFI_REL_PLX
       || jfi_a_descs[cp].type == JFI_REL_PLY)
  { if (cp > 0)
    { if (jfi_a_descs[cp].type == jfi_a_descs[cp - 1].type
          && jfi_a_descs[cp].arg == jfi_a_descs[cp - 1].arg)
        res = 0;
    }
    if (jfi_a_descs[cp].type == JFI_TRAP_END && cp > 0)
      res = 0;
  }
  return res;
}

static void jfi_ind_create(void)
{
  struct jfi_score_desc *child;
  int d_ind_no, m, s1_ind_no, s2_ind_no, cp1, cp2;
  int a, d_sno, method, s1_dif, s2_dif, am_atoms;
  float *s1_ind;
  float *s2_ind;
  float *d_ind;
  float r, tval = 0;

  d_sno = jfi_ind_rm();

  child = &(jfi_pop.scores[d_sno]);
  d_ind_no = child->ind_no;

  m = jfi_ind_rand();
  s1_ind_no = jfi_pop.scores[m].ind_no;
  jfi_stat.p1_score = jfi_pop.scores[m].score;
  m = jfi_ind_rand();
  s2_ind_no = jfi_pop.scores[m].ind_no;
  jfi_stat.p2_score = jfi_pop.scores[m].score;

  s1_ind = &(jfi_f_inds[s1_ind_no * jfi_f_atom_c]);
  s2_ind = &(jfi_f_inds[s2_ind_no * jfi_f_atom_c]);
  d_ind =  &(jfi_f_inds[d_ind_no * jfi_f_atom_c]);

  cp1 = 0; cp2 = jfi_f_atom_c - 1;
  method = jfi_random(3);
  if (method == 0)
  { cp1 = jfi_random(jfi_f_atom_c);
    while (cp1 >= 0 && jfi_cp_test(cp1) == 0)
      cp1--;
    cp2 = jfi_random(jfi_f_atom_c - cp1) + cp1;
    while (cp2 >= 0 && jfi_cp_test(cp2) == 0)
      cp2--;
  }
  r = jfi_rand_dget();

  s1_dif = 0; s2_dif = 0;
  for (m = 0; m < jfi_f_atom_c; m++)
  { if (method == 0)             /* crosover */
    { if (m >= cp1 && m < cp2)
        tval = s2_ind[m];
      else
        tval = s1_ind[m];
    }
    else
    if (method == 1)             /* sum-cros */
    { tval = r * s1_ind[m] + (1.0 - r) * s2_ind[m];
    }
    else
    if (method == 2)
      tval = s1_ind[m];          /* mutation */

    d_ind[m] = tval;
    if (d_ind[m] != s1_ind[m])
      s1_dif = 1;
    if (d_ind[m] != s2_ind[m])
      s2_dif = 1;
  }

  jfi_i2p(d_ind_no);

  if (method == 2 || s1_dif == 0 || s2_dif == 0)
  { method = 2;
    am_atoms = jfi_random(5) + 1;
    for (a = 0; a < am_atoms; a++)
      jfi_mutate(d_ind_no);
  }

  jfi_p2i(d_ind_no);
  jfi_i2p(d_ind_no);  /* Hvorfor ?? */
  jfi_stat.method = method;
  child->score = jfi_judge();
}


/****************************************************************************/
/* Hjaelpe-funktioner til jfl_load                                          */
/****************************************************************************/


static void jfi_bobl(void)
{
   int m;
   int midt;
   float rm_score;
   short rm_ind_no;

   for (m = jfi_ind_c - 1; m > 0; m--)
   { if (   (jfi_pop.maximize == 0
             && jfi_pop.scores[m].score < jfi_pop.scores[m - 1].score)
         || (jfi_pop.maximize == 1
             && jfi_pop.scores[m].score > jfi_pop.scores[m - 1].score))
     { rm_score    = jfi_pop.scores[m - 1].score;
       rm_ind_no   = jfi_pop.scores[m - 1].ind_no;
       jfi_pop.scores[m - 1].score  = jfi_pop.scores[m].score;
       jfi_pop.scores[m - 1].ind_no = jfi_pop.scores[m].ind_no;
       jfi_pop.scores[m].score      = rm_score;
       jfi_pop.scores[m].ind_no     = rm_ind_no;
     }
   }
   if (jfi_pop.repro_c >= jfi_ind_c)
     midt = jfi_ind_c / 2;
   else
     midt = jfi_pop.repro_c / 2;
   jfi_stat.median_score
      = jfi_pop.scores[midt].score;
   jfi_stat.worst_score = jfi_pop.scores[jfi_ind_c - 1].score;
}

static void jfi_inds_create(void)
{                                /* skaber start-individerne */
  int m, a, ant;

  jfi_p2i(0);
  ant = 0;
  if (jfi_imut_pct > 0.0)
    ant = (int) ((((float) jfi_f_atom_c) / 100.0) * jfi_imut_pct);
  if (ant == 0)
    ant = 5;
  for (m = 1; m < jfi_ind_c; m++)
  { jfi_i2p(0);
    jfi_p2i(m);
    for (a = 0; a < ant; a++)
      jfi_mutate(m);
    ant++;
    jfi_p2i(m);
  }
}


static void jfi_pop_create(void)  /* judges initial generation */
{
  int m;

  jfi_pop.c_q = 2.0 / ((float) jfi_ind_c);
  jfi_pop.c_r =   (2.0 * jfi_pop.c_q - 2.0 / ((float) jfi_ind_c))
                / (((float) jfi_ind_c) - 1.0);

  for (m = 0; m < jfi_ind_c; m++)
  { jfi_pop.scores[m].ind_no = m;
    jfi_i2p(m);
    jfi_pop.scores[m].score = jfi_judge();
  }
  jfi_bobl();
}



int jfi_init(void *jfr_head, int ind_c)
{
  unsigned long size;

  jfi_jraseed = (float) jfi_random(30000);

  jfi_pop.scores = NULL;
  jfi_ind_c = ind_c;
  jfi_pop.jfr_head = jfr_head;
  jfi_stat.worst_score = 0.0;
  jfi_stat.p1_score = 0.0;
  jfi_stat.p2_score = 0.0;
  jfi_stat.method = 2;
  jfg_sprg(&jfi_spdesc, jfr_head);
  /* anayse jfr-program, create conversion-table adesc */
  jfi_adesc_create(0);
  if (jfi_f_atom_c > 0)
  { size = jfi_f_atom_c * JFI_ATOMD_SIZE;
    if ((jfi_a_descs = (struct jfi_atomd_desc *) malloc(size)) == NULL)
      return 6;
  }
  else
    return 802;

  jfi_adesc_create(1);

  /* Allocate space to individ-data */
  size = jfi_f_atom_c * sizeof(float) * jfi_ind_c;
  if ((jfi_f_inds = (float *) malloc(size)) == NULL)
  { free(jfi_a_descs);
    return 6;
  }
  size = jfi_ind_c * JFI_SCORE_SIZE;
  if ((jfi_pop.scores = (struct jfi_score_desc *) malloc(size)) == NULL)
  { free(jfi_a_descs);
    free(jfi_f_inds);
    return 6;
  }
  return 0;
}

void jfi_free(void)
{
  if (jfi_pop.scores != NULL)
  { free(jfi_pop.scores);
    jfi_pop.scores = NULL;
  }
  if (jfi_f_inds != NULL)
  { free(jfi_f_inds);
    jfi_f_inds = NULL;
  }
  if (jfi_a_descs != NULL)
  { free(jfi_a_descs);
    jfi_a_descs = NULL;
  }
}

int jfi_run(float (*judge)(void), int maximize)
{
  jfi_slut = 0;
  jfi_pop.maximize = maximize;
  jfi_judge = judge;
  jfi_inds_create();
  jfi_pop_create();

  while (jfi_slut == 0)
  { jfi_ind_create();
    jfi_bobl();
  }
  jfi_i2p(jfi_pop.scores[0].ind_no);

  return 0;
}

void jfi_stop(void)
{
  jfi_slut = 1;
}
