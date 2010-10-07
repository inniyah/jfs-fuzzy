  /*********************************************************************/
  /*                                                                   */
  /* jfpltlib.c   Version  2.02  Copyright (c) 2000 Jan E. Mortensen   */
  /*                                                                   */
  /* JFS library to write plot-info about a compiled jfs-program to    */
  /* a GNUPLOT-file.                                                   */
  /*                                                                   */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                */
  /*    Lollandsvej 35 3.tv.                                           */
  /*    DK-2000 Frederiksberg                                          */
  /*    Denmark                                                        */
  /*                                                                   */
  /*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfs_cons.h"
#include "jfs_text.h"
#include "jfpltlib.h"

static struct jfg_sprog_desc jfplt_spdesc;
static void *jfr_head;
static FILE *jfplt_sout;

static int jfplt_digits = 5;
static int jfplt_samples;
static int jfplt_2_gif = 0;
/* static char *jfplt_op_name; */
static char *jfplt_op_extension;
static char *jfplt_op_dir;
static char *jfplt_term_name;
static char jf_empty[] = " ";

static struct jfg_limit_desc jfplt_limits[256];

static const char **jfplt_data;
static int jfplt_data_c = 0;

static FILE *jfplt_op;

struct jfplt_error_rec jfplt_error_desc;

struct jfplt_minmax_desc
{
  float imin;
  float imax;
};
static struct jfplt_minmax_desc jfplt_minmax[128];
static int jfplt_ff_minmax = 0;

static int jfplt_error(int emode, int error_no, char *argument);
static void jf_ftoa(char *txt, float f);
static void jfplt_float(float f);
static int jfplt_in_data(char *name);
static void jfplt_set_output(char *name);
static void jfplt_one_plf(struct jfg_limit_desc *limits, int cur, int limit_c);
static int jfplt_init(char *initfname);
static void jfplt_f_hedges(void);
static void jfplt_hedges(void);
static void jfplt_f_relations(void);
static void jfplt_relations(void);
static void jfplt_op1_write(int op, float arg, char *a, char *b);
static void jfplt_f_operators(void);
static void jfplt_operators(void);
static void jfplt_f_adjectiv(struct jfg_adjectiv_desc *adesc,
                             unsigned short ano, int rano, int acount);
static void jfplt_f_dv_adjectiv(struct jfg_adjectiv_desc *adesc, int dv_type,
                          int dv_no, unsigned short ano, int rano, int acount);
static void jfplt_f_fuz_plt(void);
static void jfplt_find_minmax(float *imin, float *imax,
                              struct jfg_domain_desc *ddesc,
                              unsigned short f_ano, int a_count);
static void jfplt_fuz_plt(void);
static void jfplt_f_defuz_plt(void);
static void jfplt_defuz_plt(void);


/*************************************************************************/
/* Hjaelpe-funktioner                                                    */
/*************************************************************************/

static int jfplt_error(int emode, int error_no, char *argument)
{
  jfplt_error_desc.error_no = error_no;
  strcpy(jfplt_error_desc.argument, argument);
  jfplt_error_desc.error_mode = emode;
  return -1;
}

static void jf_ftoa(char *txt, float f)
{
  char it[30] = "   ";
  char *t;
  int m, dp, ep, dl, sign, at, slut, b;
  signed char cif, mente;

  if (f < 0.0)
  { f = -f;
    sign = -1;
  }
  else
    sign = 1;

  t = &(it[1]);
  sprintf(t, "%20.10f", f);
  dl = strlen(it);
  dp = dl - 1;
  while (it[dp] != '.')
    dp--;
  mente = 0;
  ep = dp + jfplt_digits - 1;
  for (m = dl - 1; m >= 0; m--)
  { if (it[m] != '.' && it[m] != ' ')
    { cif = (char) (it[m] - '0' + mente);
      if (cif == 10)
      { cif = 0;
	       mente = 1;
      }
      else
	       mente = 0;
      if (m > ep)
      { if (cif >= 5)
	         mente = 1;
	       it[m] = '\0';
      }
      else
	       it[m] = (char) (cif + '0');
    }
    else
    if (it[m] == ' ' && mente == 1)
    { it[m] = '1';
      mente = 0;
    }
  }

  m = strlen(it) - 1;
  slut = 0;
  while (slut == 0 && m > 0)
  { if (it[m] == '0')
      it[m] = '\0';
    else
    if (it[m] == '.')
    { it[m + 1] = '0';
      it[m + 2] = '\0';
      slut = 1;
    }
    else
      slut = 1;
    m--;
  }

  at = 0;
  if (sign == -1)
  { txt[at] = '-';
    at++;
  }
  b = at;
  for (m = 0; it[m] != '\0'; m++)
  { if (it[m] != ' ' && it[m] != '-')
    { txt[at] = it[m];
      at++;
    }
  }
  if (at == b)
  { if (sign == -1)
      at--;
    txt[at] = '0';
    at++;
  }
  txt[at] = '\0';
}

static void jfplt_float(float f)
{
  char txt[60];

  jf_ftoa(txt, f);
  fprintf(jfplt_op, "%s", txt);
}

static int jfplt_in_data(char *name)
{
  int found, m;

  if (jfplt_data_c == 0)
    found = 1;
  else
  { found = 0;
    for (m = 0; found == 0 && m < jfplt_data_c; m++)
    { if (strcmp(name, jfplt_data[m]) == 0)
        found = 1;
    }
  }
  return found;
}

static void jfplt_set_output(char *name)
{
  if (jfplt_2_gif == 1)
  { fprintf(jfplt_op, "set output '");
    if (strlen(jfplt_op_dir) > 0)
#ifndef _WIN32
      fprintf(jfplt_op, "%s/",  jfplt_op_dir);
#else
      fprintf(jfplt_op, "%s\\", jfplt_op_dir);
#endif
    fprintf(jfplt_op, "%s.%s'\n", name, jfplt_op_extension);
  }
}

static void jfplt_one_plf(struct jfg_limit_desc *limits, int cur, int limit_c)
{
  float a, b;

  if (cur >= limit_c)
    jfplt_float(limits[limit_c - 1].value);
  else
  { if (limits[cur].exclusiv == 1)
      fprintf(jfplt_op, "(x < ");
    else
      fprintf(jfplt_op, "(x <= ");
    jfplt_float(limits[cur].limit);
    fprintf(jfplt_op, ") ? ");
    if (cur == 0)
      jfplt_float(limits[cur].value);
    else
    { if (limits[cur - 1].limit == limits[cur].limit)
        jfplt_float(limits[cur].value);
      else
      { a =  (limits[cur].value - limits[cur - 1].value)
           / ( limits[cur].limit - limits[cur - 1].limit);
        b = limits[cur - 1].value - a * limits[cur - 1].limit;
        jfplt_float(a);
        fprintf(jfplt_op, " * x + ");
        jfplt_float(b);
      }
    }
    fprintf(jfplt_op, " : ");
    jfplt_one_plf(limits, cur + 1, limit_c);
    /* fprintf(jfplt_op, ")"); */
  }
}

static int jfplt_init(char *initfname)
{
  FILE *fp;
  char txt[514];

  fp = NULL;
  fprintf(jfplt_op, "\n#Plots from the jfs-program: %s\n\n",
           jfplt_spdesc.title);
  fprintf(jfplt_op, "r01(x) = (x < 0.0) ? 0.0 : (x > 1.0) ? 1.0 : x\n");
  if (jfplt_2_gif == 1)
    fprintf(jfplt_op, "set terminal %s\n", jfplt_term_name);
  fprintf(jfplt_op, "\n");

  if (strlen(initfname) != 0)
  { fp = fopen(initfname, "r");
    if (fp == NULL)
      return jfplt_error(JPLT_EM_ERROR, 1, initfname);
  }
  if (fp != NULL)
  { while (fgets(txt, 512, fp) != NULL)
      fprintf(jfplt_op, "%s", txt);
    fclose(fp);
  }
  else
  { fprintf(jfplt_op, "set view 60,300,1,1\n");
    fprintf(jfplt_op, "set grid\n");
    fprintf(jfplt_op, "set isosamples 21\n");
    fprintf(jfplt_op, "set contour surface\n");
    fprintf(jfplt_op, "set samples %d\n", jfplt_samples);
  }
  fprintf(jfplt_op, "\n");
  return 0;
}

static void jfplt_f_hedges(void)
{
  unsigned short m;
  struct jfg_hedge_desc hdesc;

  fprintf(jfplt_op, "# Hedges:\n");
  for (m = 0; m < jfplt_spdesc.hedge_c; m++)
  { jfg_hedge(&hdesc, jfr_head, m);
    fprintf(jfplt_op, "# %s:\n", hdesc.name);
    fprintf(jfplt_op, "  h%d(x) = ", m);
    switch (hdesc.type)
    { case JFS_HT_NEGATE:
        fprintf(jfplt_op, "1.0 - x\n");
        break;
      case JFS_HT_POWER:
        fprintf(jfplt_op, "x ** " );
        jfplt_float(hdesc.hedge_arg);
        fprintf(jfplt_op, "\n");
        break;
      case JFS_HT_SIGMOID:
        fprintf(jfplt_op,
              "1.0 / ( 1.0 + 2.71828 ** (-(20.0 * x - 10) * ");
        jfplt_float(hdesc.hedge_arg);
        fprintf(jfplt_op, "))\n");
        break;
      case JFS_HT_ROUND:
        fprintf(jfplt_op, "( x >= ");
        jfplt_float(hdesc.hedge_arg);
        fprintf(jfplt_op, ") ? 1.0 : 0.0\n");
        break;
      case JFS_HT_YNOT:
        fprintf(jfplt_op, "(1.0 - x ** ");
        jfplt_float(hdesc.hedge_arg);
        fprintf(jfplt_op, ") ** (1.0 / ");
        jfplt_float(hdesc.hedge_arg);
        fprintf(jfplt_op, ")\n");
        break;
      case JFS_HT_BELL:
        fprintf(jfplt_op, "(x <= ");
        jfplt_float(hdesc.hedge_arg);
        fprintf(jfplt_op, ") ?  x ** (2.0 -  (x / " );
        jfplt_float(hdesc.hedge_arg);
        fprintf(jfplt_op, ")) : x ** ((x + " );
        jfplt_float(hdesc.hedge_arg);
        fprintf(jfplt_op, " - 2.0) / (2.0 * ");
        jfplt_float(hdesc.hedge_arg);
        fprintf(jfplt_op, " - 2.0))\n ");
        break;
      case JFS_HT_TCUT:
        fprintf(jfplt_op, "(x > ");
        jfplt_float( hdesc.hedge_arg);
        fprintf(jfplt_op, ") ? 1.0 : x\n");
        break;
      case JFS_HT_BCUT:
        fprintf(jfplt_op, "(x < ");
        jfplt_float( hdesc.hedge_arg);
        fprintf(jfplt_op, ") ? 0.0 : x\n");
        break;
      case JFS_HT_LIMITS:
        jfg_hlimits(jfplt_limits, jfr_head, m);
        jfplt_one_plf(jfplt_limits, 0, hdesc.limit_c);
        fprintf(jfplt_op, "\n");
        break;
    }
  }
  fprintf(jfplt_op, "\n");
}

static void jfplt_hedges(void)
{
  unsigned short m;
  struct jfg_hedge_desc hdesc;

  fprintf(jfplt_op, "set xlabel \"X\" 0\n");
  fprintf(jfplt_op, "set ylabel \"\" 0\n");
  for (m = 0; m < jfplt_spdesc.hedge_c; m++)
  { jfg_hedge(&hdesc, jfr_head, m);
    if (jfplt_in_data(hdesc.name) == 1)
    { jfplt_set_output(hdesc.name);
      fprintf(jfplt_op, "set title \"%s(x)\"\n", hdesc.name);
      fprintf(jfplt_op, "plot [0.0:1.0] [0.0:1.0] h%d(x) notitle\n", m);
      if (jfplt_2_gif == 0)
        fprintf(jfplt_op, "pause -1 \"Hit RETURN to continue\"\n");
    }
  }
  fprintf(jfplt_op, "\n");
}

static void jfplt_f_relations(void)
{
  struct jfg_relation_desc rdesc;
  unsigned short m;

  fprintf(jfplt_op, "#Relations:\n");
  for (m = 0; m < jfplt_spdesc.relation_c; m++)
  { jfg_relation(&rdesc, jfr_head, m);
    if (jfplt_in_data(rdesc.name) == 1)
    { fprintf(jfplt_op, "  rplf%d(x) = ", m);
      if ((rdesc.flags & JFS_RF_HEDGE) != 0)
        fprintf(jfplt_op, "h%d(", rdesc.hedge_no);
      jfg_rlimits(jfplt_limits, jfr_head, m);
      jfplt_one_plf(jfplt_limits, 0, rdesc.limit_c);
      if ((rdesc.flags & JFS_RF_HEDGE) != 0)
        fprintf(jfplt_op, ")");
      fprintf(jfplt_op,"\n");
      fprintf(jfplt_op, "r%d(x,y) = rplf%d(x - y)\n", m, m);
    }
  }
  fprintf(jfplt_op, "\n");
}

static void jfplt_relations(void)
{
  unsigned short m;
  float imax;
  struct jfg_relation_desc rdesc;

  fprintf(jfplt_op, "set xlabel \"X\" 0\n");
  fprintf(jfplt_op, "set ylabel \"Y\" 0\n");
  for (m = 0; m < jfplt_spdesc.relation_c; m++)
  { jfg_relation(&rdesc, jfr_head, m);
    if (jfplt_in_data(rdesc.name) == 1)
    { jfplt_set_output(rdesc.name);
      jfg_rlimits(jfplt_limits, jfr_head, m);
      fprintf(jfplt_op, "set title \"X %s Y\"\n", rdesc.name);
      imax = jfplt_limits[rdesc.limit_c - 1].limit - jfplt_limits[0].limit;
      fprintf(jfplt_op,
              "splot [0.0:%1.4f] [0.0:%1.4f] [0.0:1.0] r%d(x, y) notitle\n",
              imax, imax, m);
      if (jfplt_2_gif == 0)
        fprintf(jfplt_op, "pause -1 \"Hit RETURN to continue\"\n");
    }
  }
  fprintf(jfplt_op, "\n");
}

static void jfplt_op1_write(int op, float arg, char *a, char *b)
{
  switch (op)
  { case JFS_FOP_MIN:
        fprintf(jfplt_op, "(%s < %s) ? %s : %s ", a, b, a, b);
      break;
    case JFS_FOP_MAX:
        fprintf(jfplt_op, "(%s > %s) ? %s : %s ", a, b, a, b);
      break;
    case JFS_FOP_PROD:
      fprintf(jfplt_op, " %s * %s ", a, b);
      break;
    case JFS_FOP_PSUM:
      fprintf(jfplt_op, "%s + %s - %s * %s ", a, b, a, b);
      break;
    case JFS_FOP_AVG:
      fprintf(jfplt_op, " (%s + %s) / 2.0 ", a, b);
      break;
    case JFS_FOP_BSUM:
      fprintf(jfplt_op, "r01(%s + %s) ", a, b);
      break;
    case JFS_FOP_SIMILAR:
      fprintf(jfplt_op, " (%s == %s) ? 1.0 : (%s < %s) ? %s / %s : %s / %s ",
                           a,     b,           a,   b,   a,    b,  b,   a);
      break;
    case JFS_FOP_NEW:
      fprintf(jfplt_op, "%s ", b);
      break;
    case JFS_FOP_MXOR:
      fprintf(jfplt_op, "(%s >= %s) ? %s - %s : %s - %s ", a, b, a, b ,b, a);
      break;
    case JFS_FOP_SPTRUE:
      fprintf(jfplt_op,
	          "(2.0*%s-1.0)*(2.0*%s-1.0)*(2.0*%s-1.0)*(2.0*%s-1.0) ", a, a, b, b);
      break;
    case JFS_FOP_SPFALSE:
      fprintf(jfplt_op,
	      "1.0-(2.0*%s-1.0)*(2.0*%s-1.0)*(2.0*%s-1.0)*(2.0*%s-1.0) ", a, a, b, b);
      break;
    case JFS_FOP_SMTRUE:
      fprintf(jfplt_op, "1.0 - 16.0*(%s - %s * %s) * ( %s - %s * %s) ",
                                      a, a, a,          b, b, b);
      break;
    case JFS_FOP_SMFALSE:
      fprintf(jfplt_op, "16.0*(%s - %s * %s)*(%s - %s * %s) ", a, a, a, b, b, b);
      break;
    case JFS_FOP_R0:
      fprintf(jfplt_op, "0.0 ");
      break;
    case JFS_FOP_R1:
      fprintf(jfplt_op, "%s * %s - %s - %s + 1 ", a, b, a, b);
      break;
    case JFS_FOP_R2:
      fprintf(jfplt_op, "%s - %s * %s ", b, a, b);
      break;
    case JFS_FOP_R3:
      fprintf(jfplt_op, "1.0 - %s ", a);
      break;
    case JFS_FOP_R4:
      fprintf(jfplt_op, "%s - %s * %s ", a, a, b);
      break;
    case JFS_FOP_R5:
      fprintf(jfplt_op, "1.0 - %s ", b);
      break;
    case JFS_FOP_R6:
      fprintf(jfplt_op, "%s + %s - 2.0 * %s * %s ", a, b, a, b);
      break;
    case JFS_FOP_R7:
      fprintf(jfplt_op, "1.0 - %s * %s ", a, b);
      break;
    case JFS_FOP_R8:
      fprintf(jfplt_op, "%s * %s ", a, b);
      break;
    case JFS_FOP_R9:
      fprintf(jfplt_op, "1.0 - %s - %s + 2.0 * %s * %s ", a, b, a, b);
      break;
    case JFS_FOP_R10:
      fprintf(jfplt_op, "%s ", b);
      break;
    case JFS_FOP_R11:
      fprintf(jfplt_op, "1.0 - %s + %s * %s ", a, a, b);
      break;
    case JFS_FOP_R12:
      fprintf(jfplt_op, "%s ", a);
      break;
    case JFS_FOP_R13:
      fprintf(jfplt_op, "%s * %s - %s + 1.0 ", a, b, b);
      break;
    case JFS_FOP_R14:
      fprintf(jfplt_op, "%s + %s - %s * %s ", a, b, a, b);
      break;
    case JFS_FOP_R15:
      fprintf(jfplt_op, "1.0 ");
      break;
    case JFS_FOP_HAMAND:
      fprintf(jfplt_op, "(%s*%s)/(", a, b);
      jfplt_float( arg);
      fprintf(jfplt_op, "+(1.0-");
      jfplt_float(arg);
      fprintf(jfplt_op, ")*(%s+%s-%s*%s)) ", a, b, a, b);
      break;
    case JFS_FOP_HAMOR:
      fprintf(jfplt_op, "(%s+%s-(2.0-", a, b);
      jfplt_float(arg);
      fprintf(jfplt_op, ") * %s * %s)/(1.0-(1.0-", a, b);
      jfplt_float(arg);
      fprintf(jfplt_op, ") * %s * %s) ", a, b);
      break;
    case JFS_FOP_YAGERAND:
      fprintf(jfplt_op, "r01(1.0 - (((1.0-%s)**", a);
      jfplt_float( arg);
      fprintf(jfplt_op, "+(1.0-%s)**", b);
      jfplt_float( arg);
      fprintf(jfplt_op, ")**(1.0/");
      jfplt_float( arg);
      fprintf(jfplt_op, ")))");
      break;
    case JFS_FOP_YAGEROR:
      fprintf(jfplt_op, "r01((%s**", a);
      jfplt_float( arg);
      fprintf(jfplt_op, "+%s**", b);
      jfplt_float( arg);
      fprintf(jfplt_op, ")**(1.0/");
      jfplt_float( arg);
      fprintf(jfplt_op, "))");
      break;
    case JFS_FOP_BUNION:
      fprintf(jfplt_op, "r01(%s+%s-1.0 ", a, b);
      break;
    default:
      break;
  }
}

static void jfplt_f_operators(void)
{
  struct jfg_operator_desc odesc;
  unsigned short m;
  char arg1[32];
  char arg2[32];

  fprintf(jfplt_op, "#Operators:\n");
  for (m = 0; m < jfplt_spdesc.operator_c; m++)
  { jfg_operator(&odesc, jfr_head, m);
    if (jfplt_in_data((char *) odesc.name) == 1)
    { fprintf(jfplt_op, "#  %s:\n", odesc.name);
      fprintf(jfplt_op, "  o%d(a, b) = ", m);
      if (odesc.hedge_mode == JFS_OHM_POST)
        fprintf(jfplt_op, "h%d(", odesc.hedge_no);
      if (odesc.op_2 != JFS_FOP_NONE && odesc.op_2 != odesc.op_1)
      {  jfplt_float(1.0 - odesc.op_arg);
         fprintf(jfplt_op, " * ( ");
      }
      if (odesc.hedge_mode == JFS_OHM_ARG1 || odesc.hedge_mode == JFS_OHM_ARG12)
        sprintf(arg1, "h%d(a)", odesc.hedge_no);
      else
        strcpy(arg1, "a");
      if (odesc.hedge_mode == JFS_OHM_ARG2 || odesc.hedge_mode == JFS_OHM_ARG12)
        sprintf(arg2, "h%d(b)", odesc.hedge_no);
      else
        strcpy(arg2, "b");

      jfplt_op1_write(odesc.op_1, odesc.op_arg, arg1, arg2);

      if (odesc.op_2 != JFS_FOP_NONE && odesc.op_2 != odesc.op_1)
      { fprintf(jfplt_op, ") +  ");
        jfplt_float(odesc.op_arg);
        fprintf(jfplt_op, " * (");
        jfplt_op1_write(odesc.op_2, odesc.op_arg, arg1, arg2);
        fprintf(jfplt_op, ")");
      }
      if (odesc.hedge_mode == JFS_OHM_POST)
        fprintf(jfplt_op, ")");
      fprintf(jfplt_op, "\n");
    }
  }
  fprintf(jfplt_op, "\n");
}

static void jfplt_operators(void)
{
  struct jfg_operator_desc odesc;
  unsigned short m;

  fprintf(jfplt_op, "set xlabel \"X\" 0\n");
  fprintf(jfplt_op, "set ylabel \"Y\" 0\n");
  for (m = 0; m < jfplt_spdesc.operator_c; m++)
  { jfg_operator(&odesc, jfr_head, m);
    if (jfplt_in_data((char *) odesc.name) == 1)
    { jfplt_set_output((char *) odesc.name);
      fprintf(jfplt_op, "set title \"X %s Y\"\n", odesc.name);
      fprintf(jfplt_op, "splot [0.0:1.0] [0.0:1.0] [0.0:1.0] o%d(x,y) notitle\n",
               m);
      if (jfplt_2_gif == 0)
        fprintf(jfplt_op, "pause -1 \"Hit RETURN to continue\"\n");
    }
  }
  fprintf(jfplt_op, "\n");
}

static void jfplt_f_adjectiv(struct jfg_adjectiv_desc *adesc,
                             unsigned short ano, int rano, int acount)
{
  int m;
  struct jfg_adjectiv_desc adesc2;

  m = 0;
  if (adesc->limit_c > 0)
  { jfg_alimits(jfplt_limits, jfr_head, ano);
    jfplt_one_plf(jfplt_limits, 0, adesc->limit_c);
  }
  else
  { if (rano != 0)
    { jfg_adjectiv(&adesc2, jfr_head, (unsigned short) (ano - 1));
      jfplt_limits[m].limit = adesc2.trapez_end;
      jfplt_limits[m].value = 0.0;
      jfplt_limits[m].exclusiv = 0;
      m++;
    }
    jfplt_limits[m].limit = adesc->trapez_start;
    jfplt_limits[m].value = 1.0;
    jfplt_limits[m].exclusiv = 0;
    m++;
    if (adesc->trapez_start != adesc->trapez_end)
    { jfplt_limits[m].limit = adesc->trapez_end;
      jfplt_limits[m].value = 1.0;
      jfplt_limits[m].exclusiv = 0;
      m++;
    }
    if (rano < acount - 1)
    { jfg_adjectiv(&adesc2, jfr_head, (unsigned short) (ano + 1));
      jfplt_limits[m].limit = adesc2.trapez_start;
      jfplt_limits[m].value = 0.0;
      jfplt_limits[m].exclusiv = 0;
      m++;
    }
    jfplt_one_plf(jfplt_limits, 0, m);
  }
}

static void jfplt_f_dv_adjectiv(struct jfg_adjectiv_desc *adesc, int dv_type,
                              int dv_no, unsigned short ano, int rano, int acount)
{
  char txt[64];

  if (dv_type == 0)
    sprintf(txt, "d%da%d", dv_no, rano);
  else
    sprintf(txt, "v%da%d", dv_no, rano);
  if ((adesc->flags & JFS_AF_HEDGE) && adesc->h1_no != adesc->h2_no)
  { fprintf(jfplt_op, "%splf(x) = ", txt);
    jfplt_f_adjectiv(adesc, ano, rano, acount);
    fprintf(jfplt_op, "\n");
  }
  fprintf(jfplt_op, "%s(x) = ", txt);
  if (adesc->flags & JFS_AF_HEDGE)
  { if (adesc->h1_no == adesc->h2_no)
    { fprintf(jfplt_op, "h%d(", adesc->h1_no);
      jfplt_f_adjectiv(adesc, ano, rano, acount);
      fprintf(jfplt_op, ")");
    }
    else
    { fprintf(jfplt_op, "(x > ");
      jfplt_float( adesc->center);
      fprintf(jfplt_op, ") ? h%d(%splf(x)) : h%d(%splf(x))",
              adesc->h2_no, txt, adesc->h1_no, txt);
    }
  }
  else
    jfplt_f_adjectiv(adesc, ano, rano, acount);
  fprintf(jfplt_op, "\n");
}

static void jfplt_f_fuz_plt(void)  /* fuzzification-functions */
{
  unsigned short m, a;
  unsigned short ano;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  struct jfg_adjectiv_desc adesc;

  fprintf(jfplt_op,"# Fuzzification-functions:\n");
  for (m = 0; m < jfplt_spdesc.domain_c; m++)
  { jfg_domain(&ddesc, jfr_head, m);
    if (jfplt_in_data(ddesc.name) == 1 && ddesc.adjectiv_c > 0)
    { fprintf(jfplt_op, "# %s:\n", ddesc.name);
      for (a = 0; a < ddesc.adjectiv_c; a++)
      { ano = (unsigned short) (ddesc.f_adjectiv_no + a);
        jfg_adjectiv(&adesc, jfr_head, ano);
        jfplt_f_dv_adjectiv(&adesc, 0, m, ano, a, ddesc.adjectiv_c);
      }
    }
  }
  for (m = 0; m < jfplt_spdesc.var_c; m++)
  { jfg_var(&vdesc, jfr_head, m);
    if (jfplt_in_data(vdesc.name) == 1 && vdesc.fzvar_c > 0)
    { jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
      if (ddesc.adjectiv_c == 0 || vdesc.f_adjectiv_no != ddesc.f_adjectiv_no
          || (vdesc.flags & JFS_VF_NORMAL) || jfplt_data_c > 0)
      { fprintf(jfplt_op, "# %s:\n", vdesc.name);
        for (a = 0; a < vdesc.fzvar_c; a++)
        { ano = (unsigned short) (vdesc.f_adjectiv_no + a);
          jfg_adjectiv(&adesc, jfr_head, ano);
          jfplt_f_dv_adjectiv(&adesc, 1, m, ano, a, vdesc.fzvar_c);
        }
        if (vdesc.flags & JFS_VF_NORMAL)
        { fprintf(jfplt_op, "vasum%d(x) = ", m);
          for (a = 0; a < vdesc.fzvar_c; a++)
          { if (a != 0)
              fprintf(jfplt_op, " + ");
            fprintf(jfplt_op, "v%da%d(x)", m, a);
          }
          fprintf(jfplt_op, "\n");
        }
      }
    }
  }
  fprintf(jfplt_op, "\n");
}

static void jfplt_find_minmax(float *imin, float *imax,
                              struct jfg_domain_desc *ddesc,
                              unsigned short f_ano, int a_count)
{
  float mi, ma;
  int first_min, first_max;
  unsigned short m, a;
  struct jfg_adjectiv_desc adesc;

  mi = 0.0; ma = 1.0;
  first_min = first_max = 1;
  if (ddesc->flags & JFS_DF_MINENTER)
  { mi = ddesc->dmin;
    first_min = 0;
  }
  if (ddesc->flags & JFS_DF_MAXENTER)
  { ma = ddesc->dmax;
    first_max = 0;
  }
  for (a = 0; a < a_count; a++)
  { jfg_adjectiv(&adesc, jfr_head, (unsigned short) (f_ano + a));
    if (first_min == 1 || adesc.trapez_start < mi)
    { mi = adesc.trapez_start;
      first_min = 0;
    }
    if (first_max == 1 || adesc.trapez_end > ma)
    { ma = adesc.trapez_end;
      first_max = 0;
    }
    if (adesc.limit_c > 0)
    { jfg_alimits(jfplt_limits, jfr_head, (unsigned short) (f_ano + a));
      for (m = 0; m < adesc.limit_c; m++)
      { if (first_min == 1 || jfplt_limits[m].limit < mi)
        { mi = jfplt_limits[m].limit;
          first_min = 0;
        }
        if (first_max == 1 || jfplt_limits[m].limit > ma)
        { ma = jfplt_limits[m].limit;
          first_max = 0;
        }
      }
    }
  }
  *imin = mi;
  *imax = ma;
}

static void jfplt_fuz_plt(void)
{
  unsigned short m, a;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  struct jfg_adjectiv_desc adesc;
  float imin, imax, normal_value;

  for (m = 0; m < jfplt_spdesc.domain_c; m++)
  { jfg_domain(&ddesc, jfr_head, m);
    if (jfplt_in_data(ddesc.name) == 1 && ddesc.adjectiv_c > 0)
    { jfplt_set_output(ddesc.name);
      fprintf(jfplt_op, "set xlabel \"\" 0\n");
      fprintf(jfplt_op, "set ylabel \"\" 0\n");
      fprintf(jfplt_op, "set title \"%s fuzzificate\"\n", ddesc.name);
      jfplt_find_minmax(&imin, &imax, &ddesc,
                        ddesc.f_adjectiv_no, ddesc.adjectiv_c);
      fprintf(jfplt_op, "plot [");
      jfplt_float( imin);
      fprintf(jfplt_op, ":");
      jfplt_float( imax);
      fprintf(jfplt_op, "] [0.0:1.0] ");
      for (a = 0; a < ddesc.adjectiv_c; a++)
      { jfg_adjectiv(&adesc, jfr_head,
                     (unsigned short) (ddesc.f_adjectiv_no + a));
        if (a != 0)
          fprintf(jfplt_op, ",");
        fprintf(jfplt_op, "d%da%d(x) title \"%s\"", m, a, adesc.name);
      }
      fprintf(jfplt_op, "\n");
      if (jfplt_2_gif == 0)
        fprintf(jfplt_op, "pause -1 \"Hit RETURN to continue\"\n");
    }
  }
  fprintf(jfplt_op, "\n");
  for (m = 0; m < jfplt_spdesc.var_c; m++)
  { jfg_var(&vdesc, jfr_head, m);
    if (jfplt_in_data(vdesc.name) == 1 && vdesc.fzvar_c > 0)
    { jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
      if (ddesc.adjectiv_c == 0 || vdesc.f_adjectiv_no != ddesc.f_adjectiv_no
          || (vdesc.flags & JFS_VF_NORMAL) || jfplt_data_c > 0)
      { jfplt_set_output(vdesc.name);
        fprintf(jfplt_op, "set xlabel \"\" 0\n");
        fprintf(jfplt_op, "set ylabel \"\" 0\n");
        fprintf(jfplt_op, "set title \"%s fuzzificate\"\n", vdesc.name);
        jfplt_find_minmax(&imin, &imax, &ddesc, vdesc.f_adjectiv_no,
                          vdesc.fzvar_c);
        fprintf(jfplt_op, "plot [");
        jfplt_float( imin);
        fprintf(jfplt_op, ":");
        jfplt_float( imax);
        fprintf(jfplt_op, "] [0.0:1.0] ");
        for (a = 0; a < vdesc.fzvar_c; a++)
        { jfg_adjectiv(&adesc, jfr_head,
                       (unsigned short) (vdesc.f_adjectiv_no + a));
          if (a != 0)
            fprintf(jfplt_op, ",");
          if (vdesc.flags & JFS_VF_NORMAL)
          { normal_value = vdesc.no_arg;
            if (normal_value == 0.0)  /* normal conf */
              normal_value = 1.0;
            fprintf(jfplt_op, "((vasum%d(x) == 0.0) ? 1.0 : ", m);
            jfplt_float( normal_value);
            fprintf(jfplt_op, " / vasum%d(x)) * ", m);
          }
          fprintf(jfplt_op, "v%da%d(x) title \"%s\"",
                  m, a, adesc.name);
        }
        fprintf(jfplt_op, "\n");
        if (jfplt_2_gif == 0)
          fprintf(jfplt_op, "pause -1 \"Hit RETURN to continue\"\n");
      }
    }
  }
  fprintf(jfplt_op, "\n");
}

static void jfplt_f_defuz_plt(void)
{
  int a, dupli;
  unsigned short vno, v;
  float base, af, bf, imin, imax;
  struct jfg_var_desc vdesc;
  struct jfg_var_desc v2desc;
  struct jfg_domain_desc ddesc;
  struct jfg_adjectiv_desc adesc;

  fprintf(jfplt_op, "#defuzification:\n");
  for (vno = 0; vno < jfplt_spdesc.var_c; vno++)
  { jfg_var(&vdesc, jfr_head, vno);
    if (jfplt_in_data(vdesc.name) == 1 && vdesc.fzvar_c > 0)
    { dupli = 0;
      for (v = 0; dupli == 0 && v < vno; v++)
      { jfg_var(&v2desc, jfr_head, v);
        if (vdesc.f_adjectiv_no == v2desc.f_adjectiv_no
            && vdesc.defuz_1 == v2desc.defuz_1 && vdesc.defuz_2 == v2desc.defuz_2
            && vdesc.defuz_arg == v2desc.defuz_arg)
          dupli = 1;
      }
      if (dupli == 0 || jfplt_data_c > 0)
      { fprintf(jfplt_op, "# %s:\n", vdesc.name);
        if (vdesc.defuz_1 != JFS_VD_CENTROID && vdesc.defuz_2 != JFS_VD_CENTROID)
        { jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
          jfplt_find_minmax(&imin, &imax, &ddesc, vdesc.f_adjectiv_no,
                            vdesc.fzvar_c);
          base = (imax - imin) / ((float) vdesc.fzvar_c);
        }
        for (a = 0; a < vdesc.fzvar_c; a++)
        { fprintf(jfplt_op, "df%da%d(x) = ", vno, a);
          jfg_adjectiv(&adesc, jfr_head, (unsigned short) (vdesc.f_adjectiv_no + a));
          if (vdesc.defuz_1 == JFS_VD_CENTROID || vdesc.defuz_2 == JFS_VD_CENTROID)
            base = adesc.base / 2.0;
          fprintf(jfplt_op, "(x < ");
          jfplt_float( adesc.center - base);
          fprintf(jfplt_op, " ) ? 0.0 : ");
          if (adesc.center - base < imin)
            imin = adesc.center - base;

          fprintf(jfplt_op, "(x <= ");
          jfplt_float( adesc.center);
          fprintf(jfplt_op, " ) ? ");
          af = 1.0 / (adesc.center - (adesc.center - base));
          bf = -af * (adesc.center - base);
          fprintf(jfplt_op, " x * ");
          jfplt_float( af);
          fprintf(jfplt_op, " + ");
          jfplt_float( bf);
          fprintf(jfplt_op, " : ");

          fprintf(jfplt_op, "(x <= ");
          jfplt_float( adesc.center + base);
          if (adesc.center + base > imax)
            imax = adesc.center + base;
          fprintf(jfplt_op, " ) ? ");
          af = -1.0 / (adesc.center + base - adesc.center);
          bf = 1.0 - af * adesc.center;
          fprintf(jfplt_op, " x * ");
          jfplt_float( af);
          fprintf(jfplt_op, " + ");
          jfplt_float( bf);
          fprintf(jfplt_op, " : 0.0\n");
        }
        jfplt_minmax[jfplt_ff_minmax].imin = imin;
        jfplt_minmax[jfplt_ff_minmax].imax = imax;
        jfplt_ff_minmax++;
      }
    }
  }
  fprintf(jfplt_op, "\n");
}

static void jfplt_defuz_plt(void)
{
  unsigned short m, a, v;
  int dupli;
  struct jfg_var_desc vdesc;
  struct jfg_var_desc v2desc;
  struct jfg_domain_desc ddesc;
  struct jfg_adjectiv_desc adesc;
  float imin, imax;
  char txt[64];

  jfplt_ff_minmax = 0;
  for (m = 0; m < jfplt_spdesc.var_c; m++)
  { jfg_var(&vdesc, jfr_head, m);
    strcpy(txt, vdesc.name);
    strcat(txt, "_df");
    if (jfplt_in_data(vdesc.name) == 1 && vdesc.fzvar_c > 0)
    { dupli = 0;
      for (v = 0; dupli == 0 && v < m; v++)
      { jfg_var(&v2desc, jfr_head, v);
        if (vdesc.f_adjectiv_no == v2desc.f_adjectiv_no
            && vdesc.defuz_1 == v2desc.defuz_1 && vdesc.defuz_2 == v2desc.defuz_2
            && vdesc.defuz_arg == v2desc.defuz_arg)
          dupli = 1;
      }
      if (jfplt_data_c > 0 || dupli == 0)
      { jfplt_set_output(txt);
        jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
        fprintf(jfplt_op, "set xlabel \"\" 0\n");
        fprintf(jfplt_op, "set ylabel \"\" 0\n");
        fprintf(jfplt_op, "set title \"%s defuzzificate\"\n", vdesc.name);
        /* jfplt_minmax(&imin, &imax, &ddesc, vdesc.f_adjectiv_no, vdesc.fzvar_c); */
        imin = jfplt_minmax[jfplt_ff_minmax].imin;
        imax = jfplt_minmax[jfplt_ff_minmax].imax;
        fprintf(jfplt_op, "plot [");
        jfplt_float(imin);
        fprintf(jfplt_op, ":");
        jfplt_float(imax);
        fprintf(jfplt_op, "] [0.0:1.0] ");
        for (a = 0; a < vdesc.fzvar_c; a++)
        { jfg_adjectiv(&adesc, jfr_head,
                       (unsigned short) (vdesc.f_adjectiv_no + a));
          if (a != 0)
            fprintf(jfplt_op, ",");
          fprintf(jfplt_op, "df%da%d(x) title \"%s\"", m, a, adesc.name);
        }
        fprintf(jfplt_op, "\n");
        if (jfplt_2_gif == 0)
          fprintf(jfplt_op, "pause -1 \"Hit RETURN to continue\"\n");
        jfplt_ff_minmax++;
      }
    }
  }
  fprintf(jfplt_op, "\n");
}

int jfplt_plot(struct jfplt_param_desc *params)
{
  int m;
  void *head;

  jfplt_sout = params->sout;
  head = NULL;
  jfplt_op = NULL;
  jfplt_error_desc.error_mode = JPLT_EM_NONE;

  if ((m = jfr_init(0)) != 0)
   return jfplt_error(JPLT_EM_ERROR, m, jf_empty);

  if ((m = jfr_load(&head, params->ipfname)) != 0)
    return jfplt_error(JPLT_EM_ERROR, m, params->ipfname);

  if ((jfplt_op = fopen(params->opfname, "w")) == NULL)
    return jfplt_error(JPLT_EM_ERROR, 1, params->opfname);

  jfr_head = head;
  jfplt_digits = params->digits;
  jfplt_data_c = params->data_c;
  jfplt_data = params->data;
  jfplt_samples = params->samples;
  jfplt_term_name = params->term_name;
  if (strlen(jfplt_term_name) > 0)
    jfplt_2_gif = 1;
  else
    jfplt_2_gif = 0;
  jfplt_op_dir = params->op_dir;
  jfplt_op_extension = params->op_extension;
  jfplt_ff_minmax = 0;

  m = jfg_init(0, 64, jfplt_digits);
  if (m != 0)
    return jfplt_error(JPLT_EM_ERROR, m, jf_empty);

  jfg_sprg(&jfplt_spdesc, jfr_head);

  m = jfplt_init(params->initfname);
  if (m != 0)
    return m;
  jfplt_f_hedges();
  if (params->plt_hedges == 1)
    jfplt_hedges();
  if (params->plt_relations == 1)
  { jfplt_f_relations();
    jfplt_relations();
  }
  if (params->plt_operators == 1)
  { jfplt_f_operators();
    jfplt_operators();
  }
  if (params->plt_fuz == 1)
  { jfplt_f_fuz_plt();
    jfplt_fuz_plt();
  }
  if (params->plt_defuz == 1)
  { jfplt_f_defuz_plt();
    jfplt_defuz_plt();
  }
  fprintf(jfplt_op, "\nset output\n");

  jfg_free();
  if (jfplt_op != NULL)
    fclose(jfplt_op);
  jfplt_op = NULL;

  jfr_close(&head);
  jfr_free();

  return 0;
}







