  /************************************************************************/
  /*                                                                      */
  /* jfr2clib.c   Version  2.05  Copyright (c) 1999-2001 Jan E. Mortensen */
  /*                                                                      */
  /* JFS library to convert a compiled jfs-program to                     */
  /* C-sourcecode.                                                        */
  /*                                                                      */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                   */
  /*    Lollandsvej 35 3.tv.                                              */
  /*    DK-2000 Frederiksberg                                             */
  /*    Denmark                                                           */
  /*                                                                      */
  /************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfs_cons.h"
#include "jfs_text.h"

static struct jfg_sprog_desc jfr2c_spdesc;
static void *jfr_head;
static FILE *jfr2c_sout;

static int jfr2c_digits;
static int jfr2c_non_protected;
static int jfr2c_non_rounded;
static int jfr2c_use_minmax;
static int jfr2c_use_relations;
static int jfr2c_use_inline;
static int jfr2c_conf_func;
static int jfr2c_use_conf;
static int jfr2c_use_double;

#define JFI_OPT_SPEED 0
#define JFI_OPT_SPACE 1
#define JFI_OPT_FAM   2
static int jfr2c_optimize = JFI_OPT_SPEED;

static char jfr2c_spaces[] = "    ";
static char jf_empty[] = " ";
/* static char *jfr2c_t_info; */

static struct jfg_limit_desc jfr2c_limits[256];


struct pladr_desc { short first;
                    short last;
                    char  type;  /* 0: hedge, 1: relation,             */
                                 /* 2: adjectiv.                       */
                    unsigned char id;
                  };
/* Bruges til at finde et objekts pl-funktion i arrayene x og y.       */

#define JFI_PL_ADR_MAX 256
struct pladr_desc jfr2c_pl_adr[JFI_PL_ADR_MAX];
int jfr2c_ff_pl_adr;
int jfr2c_plf_excl;  /* use exclusiv */

/* Data til at afgoere om der behoeves fuz/defuz-funcs for variable */
struct varuse_desc { char rvar;   /* read domain-var */
                     char wvar;   /* write domain-var */
                     char rfzvar; /* read fuzzy-var */
                     char wfzvar; /* write fuzy-var */
                     char conf_sum; /* use conf-sum */
                     char expr_use; /* 1: fzvar, 2: domain-var, 3: both */
                     char vround;   /* use var-round-functions */
                     char fuzed;    /* fuzification is done for this var */
                     char defuzed;  /* defuzification is done for this var */
                   };
#define JFI_VARUSE_MAX 256
struct varuse_desc jfr2c_varuse[JFI_VARUSE_MAX];


/* Hvilke 1-arg-funktioner, 2-arg-funktioner, var-funcs benyttes */
int jfr2c_sfunc_use[20];
int jfr2c_dfunc_use[20]; /* 0: dont use dfunc              */
                       /* 1: define and use dfunc as df% */
                       /* 2: use dfunc as ((e1) op (e2)) */
                       /* 3: use dfunc as func(e1, e2)   */
int jfr2c_vfunc_use[20];
int jfr2c_fop_use[255];  /*  0: don't use operator,          */
                       /*  1: define and use fop as o%,    */
                       /*  2: use as ((e1) op (e2)),       */
                       /*  3: use as func(e1, e2),         */
                       /* <0: use op-no -opno instead.     */
int jfr2c_switch_use;
int jfr2c_max_levels[256];

int jfr2c_iif_use;
int jfr2c_fixed_glw;

#define JFI_LTYPES_MAX 64
unsigned char jfr2c_ltypes[JFI_LTYPES_MAX]; /* 0: switch, 1: while */
int jfr2c_ff_ltypes;

struct jfr2c_between_desc { int var_no;
                          int rano_1;
                          int rano_2;
                        };
#define JFI_BETWEEN_MAX 256
struct jfr2c_between_desc jfr2c_betweens[JFI_BETWEEN_MAX];
int jfr2c_ff_betweens;

static struct jfg_tree_desc *jfr2c_tree;
static int    jfr2c_maxtree;

static int    jfr2c_stacksize;

static int jfr2c_errcount;

#define JFI_MAX_WORDS 32
static const char *jfr2c_words[JFI_MAX_WORDS];

#define JFR2C_MAX_TEXT 1024

static FILE *jfr2c_op;
static FILE *jfr2c_dcl;
static FILE *jfr2c_hfile;
static const char *jfr2c_fixed[] =
{ " ",
  "static float rmm(float v, float mi, float ma)", /* rounds v to [mi, ma] */
  "{ float r; r = v;",
  "  if (r < mi) r = mi;",
  "  if (r > ma) r = ma;",
  "  return r;",
  "}",
  " ",
  "static float cut(float a, float v)",
  "{ if (a<v) return 0.0; else return a;",
  "}",
  " ",
  "static float r01(float v)",
  "{ if (v < 0.0) return 0.0;",
  "  if (v > 1.0) return 1.0;",
  "   return v;",
  "}",
  " ",
  "<!-- JFS END -->"
};

static const char *jfr2c_i_fixed[] =
{ " ",
  "inline static float rmm(float v, float mi, float ma)",
  "{ float r; r = v;",
  "  if (r < mi) r = mi;",
  "  if (r > ma) r = ma;",
  "  return r;",
  "}",
  " ",
  "inline static float cut(float a, float v)",
  "{ if (a<v) return 0.0; else return a;",
  "}",
  " ",
  "inline static float r01(float v)",
  "{ if (v < 0.0) return 0.0;",
  "  if (v > 1.0) return 1.0;",
  "   return v;",
  "}",
  " ",
  "<!-- JFS END -->"
};

static const char *jfr2c_t_minmax[] =
{
  "static float rmin(float v, float mi)",
  "{",
  "  if (v < mi) return v; else return mi;",
  "}",
  " ",
  "static float rmax(float v, float ma)",
  "{",
  "  if (v > ma) return v; else return ma;",
  "}",
  " ",
  "<!-- JFS END -->",
};

static const char *jfr2c_it_minmax[] =
{
  "inline static float rmin(float v, float mi)",
  "{",
  "  if (v < mi) return v; else return mi;",
  "}",
  " ",
  "inline static float rmax(float v, float ma)",
  "{",
  "  if (v > ma) return v; else return ma;",
  "}",
  " ",
  "<!-- JFS END -->",
};

static const char *jfr2c_t_iif[] =
{
  "static float iif(float c,float e1, float e2)",
  "{ return c*e1+(1.0-c)*e2;",
  "}",
  " ",
  "<!-- JFS END -->",
};

static const char *jfr2c_t_plcalc[] =
{
  "static float pl_calc(float xv, int first, int last)",
  "{ float  a, r; int m;",
  "  if (xv < plf[first].x || (xv == plf[first].x && plf[first].excl == 0))",
  "    r = plf[first].y; ",
  "  else if (xv > plf[last].x) r = plf[last].y; ",
  "  else",
  "  { for (m = first; m < last; m++)",
  "    { if (xv < plf[m+1].x || (xv == plf[m+1].x && plf[m+1].excl==0))",
  "      { if (plf[m].x == plf[m+1].x)",
  "          r = plf[m+1].y;",
  "        else",
  "        {  a = (plf[m+1].y-plf[m].y) / (plf[m+1].x-plf[m].x);",
  "           r = a * xv + plf[m].y-a*plf[m].x;",
  "        }",
  "        break;",
  "  } } }",
  "  return r;",
  "}",
  " ",
  "<!-- JFS END -->",
};

static const char *jfr2c_t_splcalc[] =      /* if no exclusive in plf's use */
{
  "static float pl_calc(float xv, int first, int last)",
  "{ float  a, r; int m;",
  "  if (xv <= plf[first].x)",
  "    r = plf[first].y; ",
  "  else if (xv >= plf[last].x) r = plf[last].y; ",
  "  else",
  "  { for (m = first; m < last; m++)",
  "    { if (xv <= plf[m+1].x)",
  "      {  a = (plf[m+1].y-plf[m].y) / (plf[m+1].x-plf[m].x);",
  "         r = a * xv + plf[m].y-a*plf[m].x;",
  "        break;",
  "  } } }",
  "  return r;",
  "}",
  " ",
  "<!-- JFS END -->",
};

static char jfr2c_t_real[16] = "float";
static const char jfr2c_t_float[]  = "float";
static const char jfr2c_t_double[] = "double";
static const char jfr2c_t_end[]   = "<!-- JFS END -->";
static const char jfr2c_t_stack[] = "stack";
static const char jfr2c_t_c[]     = "c";

#define JFE_WARNING 0
#define JFE_ERROR   1

struct jfr_err_desc { int eno;
                      const char *text;
                    };

struct jfr_err_desc jfr_err_texts[] = {
	{   0, " "},
	{   1, "Cannot open file:"},
	{   2, "Error reading from file:"},
	{   3, "Error writing to file:"},
	{   4, "Not a jfr-file:"},
	{   5, "Wrong version:"},
	{   6, "Cannot allocate memory to:"},
	{   9, "Illegal number:"},
	{  10, "Value out of domain-range:"},
	{  11, "Unexpected EOF."},
	{  13, "Undefined adjective:"},
	{ 301, "statement to long."},
	{ 302, "jfg-tree to small to hold statement."},
	{ 303, "Stack-overflow (jfg-stack)."},
	{ 304, "program-id (pc) is not the start of a statement."},
	{1002, "Too many domain-variables. Max 100."},
	{1007, "Too many block-levels (switch/while-blocks). Max 64."},
	{1008, "Extern-statement ignored."},
	{1009, "Too many different between-expresions. Max 255."},
	{1010, "Text-string to long. Max 1024 chars."},
	{9999, "unknown error."}
};

static int jf_error(int eno, const char *name, int mode);
static void jf_ftoa(char *txt, float f);
static int jfr2c_subst(char *d, const char *s, const char *ds, const char *ss);
static void jfr2c_ext_subst(char *d, const char *e);
static void jfr2c_sub_print(FILE *fp, const char *t);
static void jfr2c_float(FILE *fp, float f);
static void jfr2c_pfloat(float f);
static void jfr2c_fdefuzed_init(void);
static int jfr2c_pl_adr_add(int typ, int id, int ant);
static int jfr2c_pl_find(int typ, int id);
static int jfr2c_adj_parent(int *res, int ano);
static int jfr2c_pl_write(void);  /* write constant arrays to pl-functions */
static void jfr2c_var_write(void);
static void jfr2c_array_write(void);
static void jfr2c_fzvar_write(void);
static void jfr2c_hedges_write(void);
static void jfr2c_relations_write(void);
static void jfr2c_op1_write(int op, float arg);
static void jfr2c_operators_write(void);
static void jfr2c_sfunc_write(void);
static void jfr2c_dfunc_write(void);
static void jfr2c_vfunc_write(int vno, int fno);
static void jfr2c_decl_check(void);
static void jfr2c_set_fop(int op_no);
static int jfr2c_expr_check(unsigned short id);
static int jfr2c_program_check(void);
static void jfr2c_between_write(void);
static void jfr2c_vround_write(void);
static void jfr2c_fuz_write(void);  /* fuzzification-functions */
static void jfr2c_defuz_write(void);
static void jfr2c_leaf_fuzdefuz_check(unsigned short id);
static void jfr2c_fuzdefuz_check(unsigned char *pc);
static void jfr2c_leaf_write(int id);
static void jfr2c_init_write(char *funcname);
static void jfr2c_rules_write(char *funcname);
static void jfr2c_jfs_write(char *funcname);
static void jfr2c_t_write(const char **txts);
static int jfr2c_program_write(char *funcname, char *progname, char *hname);
static int jfr2c_file_append(char *dfname, char *sfname);


/*************************************************************************/
/* Hjaelpe-funktioner                                                    */
/*************************************************************************/

static int jf_error(int eno, const char *name, int mode)
{
  int m, v, e;

  e= 0;
  for (v = 0; e == 0; v++)
  { if (jfr_err_texts[v].eno == eno
	    || jfr_err_texts[v].eno == 9999)
      e = v;
  }
  if (mode == JFE_WARNING)
  { fprintf(jfr2c_sout, "WARNING %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    m = 0;
  }
  else
  { if (eno != 0)
      fprintf(jfr2c_sout, "*** error %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    jfr2c_errcount++;
    m = -1;
  }
  return m;
}

static int jfr2c_subst(char *d, const char *s, const char *ds, const char *ss)
{
  /* copies <s> to <d> with all occurences of <ss> replaced by <ds> */

  int st, dt, tt, ss_len, ds_len;
  char ttxt[JFR2C_MAX_TEXT];

  dt = tt = 0;
  ss_len = strlen(ss);
  ds_len = strlen(ds);
  for (st = 0; s[st] != '\0'; st++)
  { if (s[st] == ss[tt])
    { ttxt[tt] = s[st];
      tt++;
      if (tt == ss_len)
      { dt += ds_len;
        if (dt >= JFR2C_MAX_TEXT)
          return jf_error(1010, d, JFE_ERROR);
        strcat(d, ds);
        tt = 0;
      }
    }
    else
    { if (tt > 0)
      { dt += tt;
        if (dt >= JFR2C_MAX_TEXT)
          return jf_error(1010, d, JFE_ERROR);
        ttxt[tt] = '\0';
        strcat(d, ttxt);
        tt = 0;
      }
      if (dt >= JFR2C_MAX_TEXT)
        return jf_error(1010, d, JFE_ERROR);
      d[dt] = s[st];
      dt++;
      d[dt] = '\0';
    }
  }
  if (tt > 0)
  { dt += tt;
    if (dt >= JFR2C_MAX_TEXT)
      return jf_error(1010, d, JFE_ERROR);
    ttxt[tt] = '\0';
    strcat(d, ttxt);
  }
  d[dt] = '\0';
  return 0;
}

static void jfr2c_ext_subst(char *d, const char *e)
{
  int m, fundet;
  char punkt[] = ".";

  fundet = 0;
  for (m = strlen(d) - 1; m >= 0 && fundet == 0 ; m--)
  { if (d[m] == '.')
    { fundet = 1;
 	    d[m] = '\0';
    }
  }
  if (strlen(e) != 0)
    strcat(d, punkt);
  strcat(d, e);
}

static void jfr2c_sub_print(FILE *fp, const char *t)
{
  char ttxt[JFR2C_MAX_TEXT];

  if (jfr2c_use_double == 1)
  { jfr2c_subst(ttxt, t, "double", "float");
    fprintf(fp, "%s", ttxt);
  }
  else
    fprintf(fp, "%s", t);
}

static void jf_ftoa(char *txt, float f)
{
  char it[30] = "   ";
  char *t;
  int m, cif, mente, dp, ep, dl, sign, at, slut, b;

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
  ep = dp + jfr2c_digits - 1;
  for (m = dl - 1; m >= 0; m--)
  { if (it[m] != '.' && it[m] != ' ')
    { cif = it[m] - '0' + mente;
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
	       it[m] = cif + '0';
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

static void jfr2c_float(FILE *fp, float f)
{
  char txt[60];

  jf_ftoa(txt, f);
  fprintf(fp, "%s", txt);
}

static void jfr2c_pfloat(float f)
{
  char txt[60];

  jf_ftoa(txt, f);
  if (txt[0]=='-')
    fprintf(jfr2c_op, "(%s)", txt);
  else
    fprintf(jfr2c_op, "%s", txt);
}

static void jfr2c_fdefuzed_init(void)
{
  int m;

  for (m = 0; m < 256; m++)
    jfr2c_varuse[m].fuzed = jfr2c_varuse[m].defuzed = 0;
}

static int jfr2c_pl_adr_add(int typ, int id, int ant)
{
  if (jfr2c_ff_pl_adr >= JFI_PL_ADR_MAX)
    return jf_error(1001, jfr2c_spaces, JFE_ERROR);
  jfr2c_pl_adr[jfr2c_ff_pl_adr].type = typ;
  jfr2c_pl_adr[jfr2c_ff_pl_adr].id = id;
  if (jfr2c_ff_pl_adr == 0)
    jfr2c_pl_adr[jfr2c_ff_pl_adr].first = 0;
  else
    jfr2c_pl_adr[jfr2c_ff_pl_adr].first
      = jfr2c_pl_adr[jfr2c_ff_pl_adr - 1].last + 1;
  jfr2c_pl_adr[jfr2c_ff_pl_adr].last
    = jfr2c_pl_adr[jfr2c_ff_pl_adr].first + ant - 1;
  jfr2c_ff_pl_adr++;
  return 0;
}

static int jfr2c_pl_find(int typ, int id)
{
  int m;

  for (m = 0; m < jfr2c_ff_pl_adr; m++)
  { if (jfr2c_pl_adr[m].type == typ && jfr2c_pl_adr[m].id == id)
      return m;
  }
  return -1;
}

static int jfr2c_adj_parent(int *res, int ano)
{
  int pt, fundet, m;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;

  fundet = 0;
  for (m = 0; fundet == 0 && m < jfr2c_spdesc.domain_c; m++)
  { jfg_domain(&ddesc, jfr_head, m);
    if (ano >= ddesc.f_adjectiv_no
        && ano < ddesc.f_adjectiv_no + ddesc.adjectiv_c)
    { fundet = 1;
      pt = 0;
      *res = m;
    }
  }
  for (m = 0; fundet == 0 && m < jfr2c_spdesc.var_c; m++)
  { jfg_var(&vdesc, jfr_head, m);
    if (ano >= vdesc.f_adjectiv_no
        && ano < vdesc.f_adjectiv_no + vdesc.fzvar_c)
    { fundet = 1;
      pt = 1;
      *res = m;
    }
  }
  return pt;
}

static int jfr2c_pl_write(void)  /* write constant arrays to pl-functions */
{
  int m, i, pt, first, li, no = 0, res;
  struct jfg_hedge_desc hdesc;
  struct jfg_relation_desc rdesc;
  struct jfg_adjectiv_desc adesc;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  char ttxt[256];

  /* First check if exclusiv is used: */
  res = 0;
  for (m = 0; res == 0 && m < jfr2c_spdesc.hedge_c; m++)
  { jfg_hedge(&hdesc, jfr_head, m);
    if (hdesc.type == JFS_HT_LIMITS)
    { if (jfr2c_plf_excl == 0)
      { jfg_hlimits(jfr2c_limits, jfr_head, m);
        for (li = 0; li < hdesc.limit_c; li++)
        { if (jfr2c_limits[li].exclusiv == 1)
            jfr2c_plf_excl = 1;
        }
      }
      res = jfr2c_pl_adr_add(0, m, hdesc.limit_c);
    }
  }
  for (m = 0; res == 0 && m < jfr2c_spdesc.relation_c; m++)
  { jfg_relation(&rdesc, jfr_head, m);
    if (jfr2c_plf_excl == 0)
    { jfg_rlimits(jfr2c_limits, jfr_head, m);
      for (li = 0; li < rdesc.limit_c; li++)
      { if (jfr2c_limits[li].exclusiv == 1)
          jfr2c_plf_excl = 1;
      }
    }
    res = jfr2c_pl_adr_add(1, m, rdesc.limit_c);
  }
  for (m = 0; res == 0 && m < jfr2c_spdesc.adjectiv_c; m++)
  { jfg_adjectiv(&adesc, jfr_head, m);
    if (adesc.limit_c > 0)
    { if (jfr2c_plf_excl == 0)
      { jfg_alimits(jfr2c_limits, jfr_head, m);
        for (li = 0; li < adesc.limit_c; li++)
        { if (jfr2c_limits[li].exclusiv == 1)
            jfr2c_plf_excl = 1;
        }
      }
      res = jfr2c_pl_adr_add(2, m, adesc.limit_c);
    }
  }
  if (res != 0)
    return res;
  /* then write the plfs: */
  if (jfr2c_plf_excl == 0)
    sprintf(ttxt, "struct plf_desc { %s x; %s y;};\n", jfr2c_t_real, jfr2c_t_real);
  else
    sprintf(ttxt, "struct plf_desc { %s x; %s y; short excl;};\n",
                  jfr2c_t_real, jfr2c_t_real);
  jfr2c_sub_print(jfr2c_dcl, ttxt);
  if (jfr2c_ff_pl_adr != 0)
    fprintf(jfr2c_dcl, "static struct plf_desc plf[%d] = \n{\n",
		    jfr2c_pl_adr[jfr2c_ff_pl_adr - 1].last + 1);
  first = 1;
  for (m = 0; m < jfr2c_ff_pl_adr; m++)
  { switch (jfr2c_pl_adr[m].type)
    { case 0:
        jfg_hlimits(jfr2c_limits, jfr_head, jfr2c_pl_adr[m].id);
        jfg_hedge(&hdesc, jfr_head, jfr2c_pl_adr[m].id);
        sprintf(ttxt, "/* hedge %s: */\n", hdesc.name);
        break;
      case 1:
        jfg_rlimits(jfr2c_limits, jfr_head, jfr2c_pl_adr[m].id);
        jfg_relation(&rdesc, jfr_head, jfr2c_pl_adr[m].id);
        sprintf(ttxt, "/* relation %s: */\n", rdesc.name);
        break;
      case 2:
        jfg_adjectiv(&adesc, jfr_head, jfr2c_pl_adr[m].id);
        pt = jfr2c_adj_parent(&no, jfr2c_pl_adr[m].id);
        if (pt == 0)
        { jfg_domain(&ddesc, jfr_head, no);
          sprintf(ttxt, "/* domain-adjective: %s %s: */\n",
                  ddesc.name, adesc.name);
        }
        else
        { jfg_var(&vdesc, jfr_head, no);
          sprintf(ttxt, "/* var-adjective: %s %s: */\n",
                  vdesc.name, adesc.name);
        }
        jfg_alimits(jfr2c_limits, jfr_head, jfr2c_pl_adr[m].id);
        break;
    }
    for (i = 0; i <= jfr2c_pl_adr[m].last - jfr2c_pl_adr[m].first; i++)
    { if (first == 0)
   	    fprintf(jfr2c_dcl, ",\n");
      if (i == 0)
        fprintf(jfr2c_dcl, " %s", ttxt);
      first = 0;
      fprintf(jfr2c_dcl, "   {");
      jfr2c_float(jfr2c_dcl, jfr2c_limits[i].limit);
      fprintf(jfr2c_dcl, ", ");
      jfr2c_float(jfr2c_dcl, jfr2c_limits[i].value);
      if (jfr2c_plf_excl == 1)
      { fprintf(jfr2c_dcl, ", ");
        fprintf(jfr2c_dcl, "%d", jfr2c_limits[i].exclusiv);
      }
      fprintf(jfr2c_dcl, "}");
    }
  }
  if (jfr2c_ff_pl_adr != 0)
    fprintf(jfr2c_dcl, "};\n\n");
  return res;
}

static void jfr2c_var_write(void)
{
  struct jfg_var_desc vdesc;
  int m;

  fprintf(jfr2c_dcl,
"/*---------------------------------------------------------------------*/\n");
  fprintf(jfr2c_dcl,
"/* Domain variables:                                                   */\n\n");
  for (m = 0; m < jfr2c_spdesc.var_c; m++)
  { jfg_var(&vdesc, jfr_head, m);
    fprintf(jfr2c_dcl, "static %s v%d", jfr2c_t_real, m);
    if (jfr2c_use_conf == 1)
      fprintf(jfr2c_dcl, ", c%d", m);
    if (vdesc.d_comp == JFS_VCF_AVG)
    { jfr2c_varuse[m].conf_sum = 1;
      fprintf(jfr2c_dcl, ", cs%d", m);
    }
    if (jfr2c_optimize == JFI_OPT_SPEED)
      fprintf(jfr2c_dcl, "; static char vs%d", m);
    fprintf(jfr2c_dcl, "; /* %s */\n", vdesc.name);
  }
  fprintf(jfr2c_dcl, "\n");
}

static void jfr2c_array_write(void)
{
  struct jfg_array_desc adesc;
  int m;

  if (jfr2c_spdesc.array_c > 0)
  {  fprintf(jfr2c_dcl,
"/*---------------------------------------------------------------------*/\n");
    fprintf(jfr2c_dcl,
"/* arrays:                                                             */\n\n");
  }
  for (m = 0; m < jfr2c_spdesc.array_c; m++)
  { jfg_array(&adesc, jfr_head, m);
    fprintf(jfr2c_dcl, "static %s a%d[%d]; /* %s */\n",
		          jfr2c_t_real, m, adesc.array_c, adesc.name);
  }
  fprintf(jfr2c_dcl, "\n");
}

static void jfr2c_fzvar_write(void)
{
  int m;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;
  struct jfg_fzvar_desc fzvdesc;

  fprintf(jfr2c_dcl,
"/*---------------------------------------------------------------------*/\n");
  fprintf(jfr2c_dcl,
"/* Fuzzy variables:                                                   */\n\n");

  for (m = 0; m < jfr2c_spdesc.fzvar_c; m++)
  { jfg_fzvar(&fzvdesc, jfr_head, m);
    jfg_var(&vdesc, jfr_head, fzvdesc.var_no);
    jfg_adjectiv(&adesc, jfr_head,
                 vdesc.f_adjectiv_no + (m - vdesc.f_fzvar_no));
    fprintf(jfr2c_dcl, "static %s f%d; /* %s %s */\n",
                       jfr2c_t_real, m, vdesc.name, adesc.name);
  }
  fprintf(jfr2c_dcl, "\n");
}

static void jfr2c_hedges_write(void)
{
  int m, id;
  struct jfg_hedge_desc hdesc;

  fprintf(jfr2c_op,
"/*---------------------------------------------------------------------*/\n");
  fprintf(jfr2c_op,
"/* Hedges:                                                             */\n\n");
  for (m = 0; m < jfr2c_spdesc.hedge_c; m++)
  { jfg_hedge(&hdesc, jfr_head, m);
    if (hdesc.type != JFS_HT_LIMITS && jfr2c_non_rounded == 0)
    { fprintf(jfr2c_op, "static %s h%d(%s v) /* %s */\n",
                         jfr2c_t_real, m, jfr2c_t_real, hdesc.name);
      fprintf(jfr2c_dcl,"static %s h%d(%s v);\n",
                         jfr2c_t_real, m, jfr2c_t_real);
      fprintf(jfr2c_op, "{\n");
      fprintf(jfr2c_op, "  %s a;\n   a=r01(v);\n", jfr2c_t_real);
    }
    else
    { fprintf(jfr2c_op, "static %s h%d(%s a) /* %s */\n",
                        jfr2c_t_real, m, jfr2c_t_real, hdesc.name);
      fprintf(jfr2c_dcl,"static %s h%d(%s a);\n",
                        jfr2c_t_real, m, jfr2c_t_real);
      fprintf(jfr2c_op, "{\n");
    }
    switch (hdesc.type)
    { case JFS_HT_NEGATE:
        fprintf(jfr2c_op, "  return (1.0-a);\n};\n");
        break;
      case JFS_HT_POWER:
        fprintf(jfr2c_op, "  return pow(a," );
        jfr2c_float(jfr2c_op, hdesc.hedge_arg);
        fprintf(jfr2c_op, ");\n};\n");
        break;
      case JFS_HT_SIGMOID:
        fprintf(jfr2c_op,
              "  return (1.0/(1.0+pow(2.71828,-(20.0*a-10)*");
        jfr2c_float(jfr2c_op, hdesc.hedge_arg);
        fprintf(jfr2c_op, ")));\n};\n");
        break;
      case JFS_HT_ROUND:
        fprintf(jfr2c_op, "  if (a>=");
        jfr2c_float(jfr2c_op, hdesc.hedge_arg);
        fprintf(jfr2c_op, ") return 1.0;\n else return 0.0;\n};\n");
        break;
      case JFS_HT_YNOT:
        fprintf(jfr2c_op, "  return pow(1.0-pow(a,");
        jfr2c_float(jfr2c_op, hdesc.hedge_arg);
        fprintf(jfr2c_op, "),1.0/");
        jfr2c_float(jfr2c_op, hdesc.hedge_arg);
        fprintf(jfr2c_op, ");\n};\n");
        break;
      case JFS_HT_BELL:
        fprintf(jfr2c_op, "  if (a<=");
        jfr2c_float(jfr2c_op, hdesc.hedge_arg);
        fprintf(jfr2c_op, ") return pow(a,2.0-(a/" );
        jfr2c_float(jfr2c_op, hdesc.hedge_arg);
        fprintf(jfr2c_op, "));\n   else return pow(a,(a+" );
        jfr2c_pfloat(hdesc.hedge_arg);
        fprintf(jfr2c_op, "-2.0)/(2.0*");
        jfr2c_float(jfr2c_op, hdesc.hedge_arg);
        fprintf(jfr2c_op, "-2.0));\n};\n ");
        break;
      case JFS_HT_TCUT:
        fprintf(jfr2c_op, "  if (a > ");
        jfr2c_float(jfr2c_op, hdesc.hedge_arg);
        fprintf(jfr2c_op, ") return 1.0; else return a;\n};\n");
        break;
      case JFS_HT_BCUT:
        fprintf(jfr2c_op, "  if (a<");
        jfr2c_float(jfr2c_op, hdesc.hedge_arg);
        fprintf(jfr2c_op, ") return 0.0; else return a;\n};\n");
        break;
      case JFS_HT_LIMITS:
        fprintf(jfr2c_op, "  return pl_calc(a," );
        id = jfr2c_pl_find(0, m);
        fprintf(jfr2c_op, "%d,%d);\n};\n", jfr2c_pl_adr[id].first,
                          jfr2c_pl_adr[id].last);
        break;
    }
    fprintf(jfr2c_op, "\n");
  }
}

static void jfr2c_relations_write(void)
{
  struct jfg_relation_desc rdesc;
  int m, id;

  if (jfr2c_spdesc.relation_c > 0)
  {   fprintf(jfr2c_op,
"/*---------------------------------------------------------------------*/\n");
    fprintf(jfr2c_op,
"/* Relations:                                                          */\n\n");
  }
  for (m = 0; m < jfr2c_spdesc.relation_c; m++)
  { jfg_relation(&rdesc, jfr_head, m);
    fprintf(jfr2c_op, "static %s r%d(%s x, %s y) /* %s */\n",
            jfr2c_t_real, m, jfr2c_t_real, jfr2c_t_real, rdesc.name);
    fprintf(jfr2c_op, "{\n  return %d", m);
    fprintf(jfr2c_dcl,"static %s r%d(%s x, %s y);\n",
            jfr2c_t_real, m, jfr2c_t_real, jfr2c_t_real);
    if ((rdesc.flags & JFS_RF_HEDGE) != 0)
      fprintf(jfr2c_op, "h%d(", rdesc.hedge_no);
    id = jfr2c_pl_find(1, m);
    fprintf(jfr2c_op, "pl_calc(x-y,%d,%d)",
		    jfr2c_pl_adr[id].first, jfr2c_pl_adr[id].last);
    if ((rdesc.flags & JFS_RF_HEDGE) != 0)
      fprintf(jfr2c_op, ");\n};\n");
    else
      fprintf(jfr2c_op,";\n};\n\n");
  }
}

static void jfr2c_op1_write(int op, float arg)
{
  switch (op)
  { case JFS_FOP_MIN:
      if (jfr2c_use_minmax == 1)
        fprintf(jfr2c_op, "r = min(a, b);\n");
      else
        fprintf(jfr2c_op, "if (a<b) r=a;\n else r=b;\n");
      break;
    case JFS_FOP_MAX:
      if (jfr2c_use_minmax == 1)
        fprintf(jfr2c_op, "r = max(a, b);\n");
      else
        fprintf(jfr2c_op, "if (a>b) r=a;\n else r=b;\n");
      break;
    case JFS_FOP_PROD:
      fprintf(jfr2c_op, "r=a*b;\n");
      break;
    case JFS_FOP_PSUM:
      fprintf(jfr2c_op, "r=a+b-a*b;\n");
      break;
    case JFS_FOP_AVG:
      fprintf(jfr2c_op, "r=(a+b)/2.0;\n");
      break;
    case JFS_FOP_BSUM:
      fprintf(jfr2c_op, "r=r01(a+b);\n");
      break;
    case JFS_FOP_SIMILAR:
      fprintf(jfr2c_op, "if (a==b) r=1.0; else {if (a<b) r=a/b; else r=b/a;}\n");
      break;
    case JFS_FOP_NEW:
      fprintf(jfr2c_op, "r=b;\n");
      break;
    case JFS_FOP_MXOR:
      fprintf(jfr2c_op, "if (a>=b) r=a-b;\n else r=b-a;\n");
      break;
    case JFS_FOP_SPTRUE:
      fprintf(jfr2c_op,
	          "r=(2.0*a-1.0)*(2.0*a-1.0)*(2.0*b-1.0)*(2.0*b-1.0);\n");
      break;
    case JFS_FOP_SPFALSE:
      fprintf(jfr2c_op,
	      "r=1.0-(2.0*a-1.0)*(2.0*a-1.0)*(2.0*b-1.0)*(2.0*b-1.0);\n");
      break;
    case JFS_FOP_SMTRUE:
      fprintf(jfr2c_op, "r=1.0-16.0*(a-a*a)*(b-b*b);\n");
      break;
    case JFS_FOP_SMFALSE:
      fprintf(jfr2c_op, "r=16.0*(a-a*a)*(b-b*b);\n");
      break;
    case JFS_FOP_R0:
      fprintf(jfr2c_op, "r=0.0;\n");
      break;
    case JFS_FOP_R1:
      fprintf(jfr2c_op, "r=a*b-a-b+1;\n");
      break;
    case JFS_FOP_R2:
      fprintf(jfr2c_op, "r=b-a*b;\n");
      break;
    case JFS_FOP_R3:
      fprintf(jfr2c_op, "r=1.0-a;\n");
      break;
    case JFS_FOP_R4:
      fprintf(jfr2c_op, "r=a-a*b;\n");
      break;
    case JFS_FOP_R5:
      fprintf(jfr2c_op, "r=1.0-b;\n");
      break;
    case JFS_FOP_R6:
      fprintf(jfr2c_op, "r=a+b-2.0*a*b;\n");
      break;
    case JFS_FOP_R7:
      fprintf(jfr2c_op, "r=1.0-a*b;\n");
      break;
    case JFS_FOP_R8:
      fprintf(jfr2c_op, "r=a*b;\n");
      break;
    case JFS_FOP_R9:
      fprintf(jfr2c_op, "r=1.0-a-b+2.0*a*b;\n");
      break;
    case JFS_FOP_R10:
      fprintf(jfr2c_op, "r=b;\n");
      break;
    case JFS_FOP_R11:
      fprintf(jfr2c_op, "r=1.0-a+a*b;\n");
      break;
    case JFS_FOP_R12:
      fprintf(jfr2c_op, "r=a;\n");
      break;
    case JFS_FOP_R13:
      fprintf(jfr2c_op, "r=a*b-b+1.0;\n");
      break;
    case JFS_FOP_R14:
      fprintf(jfr2c_op, "r=a+b-a*b;\n");
      break;
    case JFS_FOP_R15:
      fprintf(jfr2c_op, "r=1.0;\n");
      break;
    case JFS_FOP_HAMAND:
      fprintf(jfr2c_op, "r=(a*b)/(");
      jfr2c_float(jfr2c_op, arg);
      fprintf(jfr2c_op, "+(1.0-");
      jfr2c_pfloat(arg);
      fprintf(jfr2c_op, ")*(a+b-a*b));\n");
      break;
    case JFS_FOP_HAMOR:
      fprintf(jfr2c_op, "r=(a+b-(2.0-");
      jfr2c_pfloat(arg);
      fprintf(jfr2c_op, ")*a*b)/(1.0-(1.0-");
      jfr2c_pfloat(arg);
      fprintf(jfr2c_op, ")*a*b);\n");
      break;
    case JFS_FOP_YAGERAND:
      fprintf(jfr2c_op, "r=pow(pow(1.0-a,");
      jfr2c_float(jfr2c_op, arg);
      fprintf(jfr2c_op, ")+pow(1.0-b,");
      jfr2c_float(jfr2c_op, arg);
      fprintf(jfr2c_op, "),1.0/");
      jfr2c_float(jfr2c_op, arg);
      fprintf(jfr2c_op, ");\nif (r>1.0) r=0.0;\n else r=1.0-r;\n");
      break;
    case JFS_FOP_YAGEROR:
      fprintf(jfr2c_op, "r=pow(pow(a,");
      jfr2c_float(jfr2c_op, arg);
      fprintf(jfr2c_op, ")+pow(b,");
      jfr2c_float(jfr2c_op, arg);
      fprintf(jfr2c_op, "),1.0/");
      jfr2c_float(jfr2c_op, arg);
      fprintf(jfr2c_op, ");\nif (r>1.0) r=1.0;\n");
      break;
    case JFS_FOP_BUNION:
      fprintf(jfr2c_op, "r=a+b-1.0; if (r<0.0) r=0.0;\n");
      break;
    default:
      break;
  }
}

static void jfr2c_operators_write(void)
{
  struct jfg_operator_desc odesc;
  int m;

  fprintf(jfr2c_op,
"/*---------------------------------------------------------------------*/\n");
  fprintf(jfr2c_op,
"/* Fuzzy operators:                                                    */\n\n");

  for (m = 0; m < jfr2c_spdesc.operator_c; m++)
  { if (jfr2c_fop_use[m] == 1)
    { jfg_operator(&odesc, jfr_head, m);
      if (jfr2c_non_rounded == 0 || odesc.hedge_mode == JFS_OHM_ARG1
          || odesc.hedge_mode == JFS_OHM_ARG2
          || odesc.hedge_mode == JFS_OHM_ARG12)
      { fprintf(jfr2c_op, "static %s o%d(%s x, %s y) /* %s */\n",
                jfr2c_t_real, m, jfr2c_t_real, jfr2c_t_real, odesc.name);
  		    fprintf(jfr2c_dcl,"static %s o%d(%s x, %s y);\n",
                jfr2c_t_real, m, jfr2c_t_real, jfr2c_t_real);
        fprintf(jfr2c_op, "{ %s r, a, b", jfr2c_t_real);
      }
      else
      { fprintf(jfr2c_op, "static %s o%d(%s a, %s b) /* %s */\n",
                jfr2c_t_real, m, jfr2c_t_real, jfr2c_t_real, odesc.name);
  		    fprintf(jfr2c_dcl,"static %s o%d(%s a, %s b);\n",
                jfr2c_t_real, m, jfr2c_t_real, jfr2c_t_real);
        fprintf(jfr2c_op, "{ %s r", jfr2c_t_real);
      }
      if (odesc.op_2 != JFS_FOP_NONE && odesc.op_2 != odesc.op_1)
        fprintf(jfr2c_op, ", t;\n");
      else
        fprintf(jfr2c_op, ";\n");
      if (odesc.hedge_mode == JFS_OHM_ARG1 || odesc.hedge_mode == JFS_OHM_ARG12)
        fprintf(jfr2c_op, "  a=h%d(x);", odesc.hedge_no);
      else
      { if (jfr2c_non_rounded == 0)
          fprintf(jfr2c_op, "  a=r01(x);\n");
        else
        if (odesc.hedge_mode == JFS_OHM_ARG2)
          fprintf(jfr2c_op, "  a=x;\n");
      }
      if (odesc.hedge_mode == JFS_OHM_ARG2 || odesc.hedge_mode == JFS_OHM_ARG12)
        fprintf(jfr2c_op, "  b=h%d(y);", odesc.hedge_no);
      else
      { if (jfr2c_non_rounded == 0)
          fprintf(jfr2c_op, "  b=r01(y);\n");
        else
        if (odesc.hedge_mode == JFS_OHM_ARG1)
          fprintf(jfr2c_op, "  b=y;\n");
      }
      jfr2c_op1_write(odesc.op_1, odesc.op_arg);
      if (odesc.op_2 != JFS_FOP_NONE && odesc.op_2 != odesc.op_1)
      { fprintf(jfr2c_op, "  t=r;\n");
        jfr2c_op1_write(odesc.op_2, odesc.op_arg);
        fprintf(jfr2c_op, "  r=t*(1.0 - ");
        jfr2c_float(jfr2c_op, odesc.op_arg);
        fprintf(jfr2c_op, ") + r * ");
        jfr2c_pfloat(odesc.op_arg);
        fprintf(jfr2c_op, ";\n");
      }

      if (odesc.hedge_mode == JFS_OHM_POST)
        fprintf(jfr2c_op, "  r=h%d(r);\n", odesc.hedge_no);
      fprintf(jfr2c_op, "  return r;\n};\n\n");
    }
  }
}

static void jfr2c_sfunc_write(void)
{
  int m;

  fprintf(jfr2c_op,
"/*---------------------------------------------------------------------*/\n");
  fprintf(jfr2c_op,
"/* Predefined functions and operators:                                 */\n\n");

  for (m = 0; m < 20; m++)
  { if (jfr2c_sfunc_use[m] == 1)
    { fprintf(jfr2c_op, "static %s s%d(%s a) /* %s */\n",
              jfr2c_t_real, m, jfr2c_t_real, jfs_t_sfus[m]);
      fprintf(jfr2c_dcl,"static %s s%d(%s a);\n",
              jfr2c_t_real, m, jfr2c_t_real);
      fprintf(jfr2c_op, "{  %s r;\n  r=0.0;\n  ", jfr2c_t_real);
      switch (m)
      { case JFS_SFU_TAN:
          fprintf(jfr2c_op, "r=cos(a);if (r!=0.0) r=sin(a) / r;");
          break;
        case JFS_SFU_ACOS:
          fprintf(jfr2c_op, "if (a>=-1.0 && a<=1.0) r=acos(a);");
          break;
        case JFS_SFU_ASIN:
          fprintf(jfr2c_op, "if (a>=-1.0 && a<=1.0) r=asin(a);");
          break;
        case JFS_SFU_LOG:
          fprintf(jfr2c_op, "if (a>0) r=log(a);");
          break;
        case JFS_SFU_NEGATE:
          fprintf(jfr2c_op, "r=-a;");
          break;
        case JFS_SFU_RANDOM:
          fprintf(jfr2c_op, "r = (((%s) rand()) / ((%s) RAND_MAX)) * a;",
                  jfr2c_t_real, jfr2c_t_real);
          break;
        case JFS_SFU_SQRT:
          fprintf(jfr2c_op, "if (a>=0) r=sqrt(a);");
          break;
        case JFS_SFU_WGET:
          fprintf(jfr2c_op, "if (glw>a) r=glw;");
          break;
      }
      fprintf(jfr2c_op, "return r;\n};\n\n");
    }
  }
}

static void jfr2c_dfunc_write(void)
{
  int m;

  for (m = 0; m < 20; m++)
  { if (jfr2c_dfunc_use[m] == 1)
    { fprintf(jfr2c_op, "static %s d%d(%s a, %s b) /* %s */\n",
              jfr2c_t_real, m, jfr2c_t_real, jfr2c_t_real, jfs_t_dfus[m]);
      fprintf(jfr2c_dcl,"static %s d%d(%s a, %s b);\n",
              jfr2c_t_real, m, jfr2c_t_real, jfr2c_t_real);
      fprintf(jfr2c_op, "{ %s r;\n", jfr2c_t_real);
      fprintf(jfr2c_op, "  r=0.0;\n  ");
      switch (m)
      { case JFS_DFU_DIV:
          fprintf(jfr2c_op, "if (b!=0.0) r=a/b;");
          break;
        case JFS_DFU_POW:
          fprintf(jfr2c_op,
                  "if ((a==0 && b<0) || (a<0 && b != floor(b)))\n");
          fprintf(jfr2c_op, "r=0.0; else r=pow(a,b);");
          break;
        case JFS_DFU_MIN:
          fprintf(jfr2c_op, "if (a<b) r=a; else r=b;");
          break;
        case JFS_DFU_MAX:
          fprintf(jfr2c_op, "if (a>b) r=a; else r=b;");
          break;
        case JFS_DFU_CUT:
          fprintf(jfr2c_op, "if (a>b) r=a; else r=0.0;");
          break;
        case JFS_ROP_EQ:
          fprintf(jfr2c_op, "if (a==b) r=1.0;");
          break;
        case JFS_ROP_NEQ:
          fprintf(jfr2c_op, "if (a!=b) r=1.0;");
          break;
        case JFS_ROP_GT:
          fprintf(jfr2c_op, "if (a>b) r=1.0;");
          break;
        case JFS_ROP_GEQ:
          fprintf(jfr2c_op, "if (a>=b) r=1.0;");
          break;
        case JFS_ROP_LT:
          fprintf(jfr2c_op, "if (a<b) r=1.0;");
          break;
        case JFS_ROP_LEQ:
          fprintf(jfr2c_op, "if (a<=b) r=1.0;");
          break;
      }
      fprintf(jfr2c_op, "\n  return r;\n};\n\n");
    }
  }
}

static void jfr2c_vfunc_write(int vno, int fno)
{
  /* skriver en var-function ind i et expresion (kaldes fra write_leaf */
  int a;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;

  jfg_var(&vdesc, jfr_head, vno);
  switch (fno)
  { case JFS_VFU_DNORMAL:
      jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
      if ((ddesc.flags & JFS_DF_MINENTER) != 0
	         && (ddesc.flags & JFS_DF_MAXENTER) != 0)
      { if (ddesc.dmin == ddesc.dmax)
	         jfr2c_pfloat(ddesc.dmin);
        else
        { /* ((v%-dmin)/(dmax-dmin)) */
          fprintf(jfr2c_op, "((v%d-", vno);
          jfr2c_pfloat(ddesc.dmin);
          fprintf(jfr2c_op, ")/");
          jfr2c_float(jfr2c_op, ddesc.dmax - ddesc.dmin);
          fprintf(jfr2c_op, ")");
        }
      }
      else
        fprintf(jfr2c_op, "v%d", vno);
      break;
    case JFS_VFU_M_FZVAR:
      for (a = 0; a < vdesc.fzvar_c - 1; a++)
   	  { if (jfr2c_use_minmax == 1)
          fprintf(jfr2c_op, "max(f%d,", vdesc.f_fzvar_no + a);
        else
          fprintf(jfr2c_op, "rmax(f%d,", vdesc.f_fzvar_no + a);
      }
      fprintf(jfr2c_op, "f%d", vdesc.f_fzvar_no + vdesc.fzvar_c - 1);
      for (a = 0; a < vdesc.fzvar_c - 1; a++)
	      fprintf(jfr2c_op, ")");
      break;
    case JFS_VFU_S_FZVAR:
      fprintf(jfr2c_op, "(");
      for (a = 0; a < vdesc.fzvar_c; a++)
      { if (a != 0)
          fprintf(jfr2c_op, "+");
        fprintf(jfr2c_op, "f%d", vdesc.f_fzvar_no + a);
      }
      fprintf(jfr2c_op, ")");
      break;
    case JFS_VFU_DEFAULT:
      jfr2c_pfloat(vdesc.default_val);
      break;
    case JFS_VFU_CONFIDENCE:
      fprintf(jfr2c_op, "c%d", vno);
      break;
  }
}

static void jfr2c_decl_check(void)
{
  struct jfg_var_desc vdesc;
  int m;

  for (m = 0; m < jfr2c_spdesc.var_c; m++)
  { jfg_var(&vdesc, jfr_head, m);
    if (vdesc.d_comp != JFS_VCF_NEW)
      jfr2c_use_conf = 1;
    if ((vdesc.flags & JFS_VF_NORMAL)
        && vdesc.no_arg != 0.0)
      jfr2c_use_conf = 1;
  }
}

static void jfr2c_set_fop(int op_no)
{
  struct jfg_operator_desc odesc;

  jfg_operator(&odesc, jfr_head, op_no);
  if (jfr2c_fop_use[op_no] == 0)
  { jfr2c_fop_use[op_no] = 1;
    if (odesc.hedge_mode == JFS_OHM_NONE
        && odesc.op_1 == odesc.op_2
        && jfr2c_non_rounded == 1)
    { switch (odesc.op_1)
      { case JFS_FOP_MIN:
        case JFS_FOP_MAX:
          jfr2c_fop_use[op_no] = 3;
          break;
        case JFS_FOP_PROD:
        case JFS_FOP_R8:
          jfr2c_fop_use[op_no] = 2;
          break;
      }
    }
  }
}

static int jfr2c_expr_check(unsigned short id)
{
  struct jfg_tree_desc *leaf;
  struct jfg_fzvar_desc fzvdesc;
  int m;

  leaf = &(jfr2c_tree[id]);
  switch (leaf->type)
  { case JFG_TT_OP:
      jfr2c_set_fop(leaf->op);
      jfr2c_expr_check(leaf->sarg_1);
      jfr2c_expr_check(leaf->sarg_2);
      break;
    case JFG_TT_UREL:
    case JFG_TT_ARGLIST:
      jfr2c_expr_check(leaf->sarg_1);
      jfr2c_expr_check(leaf->sarg_2);
      break;
    case JFG_TT_IIF:
      jfr2c_iif_use = 1;
      jfr2c_expr_check(leaf->sarg_1);
      jfr2c_expr_check(leaf->sarg_2);
      break;
    case JFG_TT_HEDGE:
    case JFG_TT_UFUNC:
    case JFG_TT_ARVAL:
      jfr2c_expr_check(leaf->sarg_1);
      break;
    case JFG_TT_SFUNC:
      if (jfr2c_sfunc_use[leaf->op] == 0)
        jfr2c_sfunc_use[leaf->op] = 1;
      jfr2c_expr_check(leaf->sarg_1);
      break;
    case JFG_TT_DFUNC:
      if (jfr2c_dfunc_use[leaf->op] == 0)
        jfr2c_dfunc_use[leaf->op] = 1;
      jfr2c_expr_check(leaf->sarg_1);
      jfr2c_expr_check(leaf->sarg_2);
      break;
    case JFG_TT_VAR:
      jfr2c_varuse[leaf->sarg_1].rvar = 1;
      break;
    case JFG_TT_FZVAR:
      jfg_fzvar(&fzvdesc, jfr_head, leaf->sarg_1);
      jfr2c_varuse[fzvdesc.var_no].rfzvar = 1;
      break;
    case JFG_TT_BETWEEN:
      jfr2c_varuse[leaf->sarg_1].rvar = 1;
      for (m = 0; m < jfr2c_ff_betweens; m++)
      { if (jfr2c_betweens[m].var_no == leaf->sarg_1
	           && jfr2c_betweens[m].rano_1 == leaf->sarg_2
	           && jfr2c_betweens[m].rano_2 == leaf->op)
       	  break;
      }
      if (m == jfr2c_ff_betweens)
      { if (jfr2c_ff_betweens >= JFI_BETWEEN_MAX)
        { jf_error(1009, jfr2c_spaces, JFE_ERROR);
          return -1;
        }
        jfr2c_betweens[jfr2c_ff_betweens].var_no = leaf->sarg_1;
        jfr2c_betweens[jfr2c_ff_betweens].rano_1 = leaf->sarg_2;
        jfr2c_betweens[jfr2c_ff_betweens].rano_2 = leaf->op;
        jfr2c_ff_betweens++;
      }
      break;
    case JFG_TT_VFUNC:
      jfr2c_vfunc_use[leaf->op] = 1;
      if (leaf->op == JFS_VFU_DNORMAL)
	       jfr2c_varuse[leaf->sarg_1].rvar = 1;
      if (leaf->op == JFS_VFU_M_FZVAR || leaf->op == JFS_VFU_S_FZVAR)
	       jfr2c_varuse[leaf->sarg_1].rfzvar = 1;
      if (leaf->op == JFS_VFU_CONFIDENCE)
      { jfr2c_varuse[leaf->sarg_1].rvar = 1;
        jfr2c_use_conf = 1;
      }
      break;
    default:
      break;
  }
  return 0;
}

static int jfr2c_program_check(void)
{
  int m, fu, res, clevel;
  unsigned short c, i, e;
  /* struct jfg_var_desc vdesc; */
  struct jfg_statement_desc sdesc;
  struct jfg_var_desc vdesc;
  struct jfg_fzvar_desc fzvdesc;
  struct jfg_function_desc fudesc;
  unsigned char *pc;

  for (m = 0; m < 20; m++)
  { jfr2c_sfunc_use[m] = 0;
    jfr2c_dfunc_use[m] = 0;
    jfr2c_vfunc_use[m] = 0;
  }
  jfr2c_sfunc_use[JFS_SFU_COS] = jfr2c_sfunc_use[JFS_SFU_SIN]
  = jfr2c_sfunc_use[JFS_SFU_ATAN] = jfr2c_sfunc_use[JFS_SFU_FABS]
  = jfr2c_sfunc_use[JFS_SFU_FLOOR] = jfr2c_sfunc_use[JFS_SFU_CEIL]
  = jfr2c_sfunc_use[JFS_SFU_SQR] = -1;
  jfr2c_dfunc_use[JFS_DFU_PLUS] = jfr2c_dfunc_use[JFS_DFU_MINUS]
  = jfr2c_dfunc_use[JFS_DFU_PROD] = 2; /* use operator-notation */
  if (jfr2c_non_protected == 1)
  { jfr2c_sfunc_use[JFS_SFU_TAN] = jfr2c_sfunc_use[JFS_SFU_ACOS]
    = jfr2c_sfunc_use[JFS_SFU_ASIN] = jfr2c_sfunc_use[JFS_SFU_LOG]
    = jfr2c_sfunc_use[JFS_SFU_NEGATE] = jfr2c_sfunc_use[JFS_SFU_SQRT] = -1;
    jfr2c_dfunc_use[JFS_DFU_DIV] = 2;
    jfr2c_dfunc_use[JFS_DFU_POW] = 3;
  }
  if (jfr2c_use_relations == 1)
  { jfr2c_dfunc_use[JFS_ROP_EQ] = jfr2c_dfunc_use[JFS_ROP_NEQ]
    = jfr2c_dfunc_use[JFS_ROP_GT] = jfr2c_dfunc_use[JFS_ROP_GEQ]
    = jfr2c_dfunc_use[JFS_ROP_LT] = jfr2c_dfunc_use[JFS_ROP_LEQ] = 2;
  }
  if (jfr2c_use_minmax == 1)
    jfr2c_dfunc_use[JFS_DFU_MIN] = jfr2c_dfunc_use[JFS_DFU_MAX] = 3;
  for (m = 0; m < 255; m++)
    jfr2c_fop_use[m] = 0;
  jfr2c_switch_use = 0;
  jfr2c_iif_use = 0;
  jfr2c_ff_betweens = 0;
  jfr2c_fixed_glw = 1;

  for (m = 0; m <jfr2c_spdesc.var_c; m++)
  { jfg_var(&vdesc, jfr_head, m);
    jfr2c_set_fop(vdesc.f_comp);
    jfr2c_varuse[m].rvar   = 0;
    jfr2c_varuse[m].wvar   = 0;
    jfr2c_varuse[m].rfzvar = 0;
    jfr2c_varuse[m].wfzvar = 0;
    if (m >= jfr2c_spdesc.f_ivar_no
   	    && m < jfr2c_spdesc.f_ivar_no + jfr2c_spdesc.ivar_c)
      jfr2c_varuse[m].wvar   = 1;
    if (m >= jfr2c_spdesc.f_ovar_no
	       && m < jfr2c_spdesc.f_ovar_no + jfr2c_spdesc.ovar_c)
      jfr2c_varuse[m].rvar = 1;
  }

  for (fu = 0; fu < jfr2c_spdesc.function_c + 1; fu++)
  { if (fu < jfr2c_spdesc.function_c)
    { jfg_function(&fudesc, jfr_head, fu);
      pc = fudesc.pc;
      jfr2c_fixed_glw = 0;
    }
    else
      pc = jfr2c_spdesc.pc_start;
    jfr2c_max_levels[fu] = 0;
    clevel = 0;
    jfg_statement(&sdesc, jfr_head, pc);
    while (sdesc.type != JFG_ST_EOP
	          && (!(sdesc.type == JFG_ST_STEND && sdesc.sec_type == 2)))
    { switch (sdesc.type)
      { case JFG_ST_IF:
          if (sdesc.flags != 0)
            jfr2c_set_fop(JFS_ONO_WEIGHTOP);
          res = jfg_if_tree(jfr2c_tree, jfr2c_maxtree, &c, &i, &e,
                            jfr_head, pc);
          if (res != 0)
            return jf_error(res, jfr2c_spaces, JFE_ERROR);
          switch (sdesc.sec_type)
          { case JFG_SST_FZVAR:
              jfg_fzvar(&fzvdesc, jfr_head, sdesc.sarg_1);
              jfr2c_varuse[fzvdesc.var_no].rfzvar = 1;  /* i comp-operator */
              jfr2c_varuse[fzvdesc.var_no].wfzvar = 1;
              jfr2c_expr_check(c);
               break;
            case JFG_SST_VAR:
            case JFG_SST_INC:
              jfr2c_varuse[sdesc.sarg_1].rvar = 1;  /* i comp-operator */
              jfr2c_varuse[sdesc.sarg_1].wvar = 1;
              jfr2c_expr_check(c);
              jfr2c_expr_check(e);
              break;
            case JFG_SST_ARR:
              jfr2c_expr_check(c);
              jfr2c_expr_check(i);
              jfr2c_expr_check(e);
              break;
            case JFG_SST_PROCEDURE:
            case JFG_SST_RETURN:
            case JFG_SST_FUARG:
              jfr2c_expr_check(c);
              jfr2c_expr_check(e);
              break;
            default:
              jfr2c_expr_check(c);

          }
          break;
        case JFG_ST_CASE:
          jfr2c_fixed_glw = 0;
          res = jfg_if_tree(jfr2c_tree, jfr2c_maxtree, &c, &i, &e, jfr_head, pc);
          if (res != 0)
            return jf_error(res, jfr2c_spaces, JFE_ERROR);
          jfr2c_expr_check(c);
          jfr2c_fop_use[JFS_ONO_CASEOP] = 1;
          break;
        case JFG_ST_SWITCH:
          jfr2c_fixed_glw = 0;
          jfr2c_switch_use = 1;
          clevel++;
          if (clevel > jfr2c_max_levels[fu])
            jfr2c_max_levels[fu] = clevel;
          break;
        case JFG_ST_WHILE:
          jfr2c_fixed_glw = 0;
          jfr2c_switch_use = 1;
          jfr2c_fop_use[JFS_ONO_WHILEOP] = 1;
          clevel++;
          if (clevel > jfr2c_max_levels[fu])
            jfr2c_max_levels[fu] = clevel;
          res = jfg_if_tree(jfr2c_tree, jfr2c_maxtree, &c, &i, &e, jfr_head, pc);
          if (res != 0)
            return jf_error(res, jfr2c_spaces, JFE_ERROR);
          jfr2c_expr_check(c);
          break;
        case JFG_ST_STEND:
          jfr2c_fixed_glw = 0;
          if (sdesc.sec_type != 2)
            clevel--;
          break;
        case JFG_ST_DEFAULT:
          jfr2c_fixed_glw = 0;
          jfr2c_fop_use[JFS_ONO_CASEOP] = 1;
          break;
        case JFG_ST_WSET:
          jfr2c_fixed_glw = 0;
          res = jfg_if_tree(jfr2c_tree, jfr2c_maxtree, &c, &i, &e, jfr_head, pc);
          if (res != 0)
            return jf_error(res, jfr2c_spaces, JFE_ERROR);
          jfr2c_expr_check(c);
          break;
        default:
          jfr2c_fixed_glw = 0;
          break;
      }
      pc = sdesc.n_pc;
      jfg_statement(&sdesc, jfr_head, pc);
    }
  }
  return 0;
}

static void jfr2c_between_write(void)
{
  int m, id;
  struct jfr2c_between_desc *bd;
  struct jfg_adjectiv_desc adesc1;
  struct jfg_adjectiv_desc adesc2;
  struct jfg_adjectiv_desc adesc_tmp;
  struct jfg_var_desc vdesc;

  for (m = 0; m < jfr2c_ff_betweens; m++)
  { bd = &(jfr2c_betweens[m]);
    jfg_var(&vdesc, jfr_head, bd->var_no);
    jfg_adjectiv(&adesc1, jfr_head, vdesc.f_adjectiv_no + bd->rano_1);
    jfg_adjectiv(&adesc2, jfr_head, vdesc.f_adjectiv_no + bd->rano_2);
    fprintf(jfr2c_op, "static %s b%d_%d_%d(void)\n{ %s r;\n",
      		    jfr2c_t_real, bd->var_no, bd->rano_1, bd->rano_2, jfr2c_t_real);
    fprintf(jfr2c_dcl,"static %s b%d_%d_%d(void);\n",
            jfr2c_t_real, bd->var_no, bd->rano_1, bd->rano_2);
    fprintf(jfr2c_op, "if (v%d<", bd->var_no);
    jfr2c_float(jfr2c_op, adesc1.center);
    fprintf(jfr2c_op, ") {");
    if (adesc1.limit_c > 0)
    { id = jfr2c_pl_find(2, vdesc.f_adjectiv_no + bd->rano_1);
      fprintf(jfr2c_op, "r=pl_calc(v%d,%d,%d);\n",
		      bd->var_no,
		      jfr2c_pl_adr[id].first, jfr2c_pl_adr[id].last);
    }
    else
    { if (bd->rano_1 == 0)
	    fprintf(jfr2c_op, " r=1.0;\n");
      else
      { /* {if (vval <= pre) fzv=0 else fzv=(vval-pre)/(center-pre);} */
        fprintf(jfr2c_op, "if (v%d<=",bd->var_no);
        jfg_adjectiv(&adesc_tmp, jfr_head,
                     vdesc.f_adjectiv_no + bd->rano_1 - 1);
        jfr2c_float(jfr2c_op, adesc_tmp.center);
        fprintf(jfr2c_op, ") r = 0.0;\n else r = (v%d - ",
                        bd->var_no);
        jfr2c_pfloat(adesc_tmp.center);
        fprintf(jfr2c_op, ") / ");
        jfr2c_float(jfr2c_op, adesc1.center-adesc_tmp.center);
        fprintf(jfr2c_op, ";");
      }
      if (adesc1.flags & JFS_AF_HEDGE)
      { fprintf(jfr2c_op, "r = h%d(r);\n",
		    	adesc1.h1_no);
      }
    }
    fprintf(jfr2c_op, ";}\n else if (v%d > ", bd->var_no);
    jfr2c_float(jfr2c_op, adesc2.center);
    fprintf(jfr2c_op, ") {");

    if (adesc2.limit_c > 0)
    { id = jfr2c_pl_find(2, vdesc.f_adjectiv_no + bd->rano_2);
      fprintf(jfr2c_op, "r = pl_calc(v%d,%d,%d);\n",
		       bd->var_no,
		       jfr2c_pl_adr[id].first, jfr2c_pl_adr[id].last);
    }
    else
    { if (bd->rano_2 == vdesc.fzvar_c - 1)
        fprintf(jfr2c_op, "r=1.0;\n");
      else
      { /* {if (vval>=post) fzv=0 else fzv=(post-vval)/(post-center);} */
        jfg_adjectiv(&adesc_tmp, jfr_head,
                 vdesc.f_adjectiv_no + bd->rano_2 +1);
        fprintf(jfr2c_op, "if (v%d >= ",bd->var_no);
        jfr2c_float(jfr2c_op, adesc_tmp.center);
        fprintf(jfr2c_op, ") r = 0.0;\n else r = (");
        jfr2c_float(jfr2c_op, adesc_tmp.center);
        fprintf(jfr2c_op, "-v%d) / ", bd->var_no);
        jfr2c_float(jfr2c_op, adesc_tmp.center-adesc2.center);
        fprintf(jfr2c_op, ";\n");
      }
      if (adesc2.flags & JFS_AF_HEDGE)
      { fprintf(jfr2c_op, "r = h%d(r);\n", adesc2.h2_no);
      }
    }
    fprintf(jfr2c_op, "}\n else r = 1.0; return r;\n}\n\n");
  }
}

static void jfr2c_fuz_write(void)  /* fuzzification-functions */
{
  int vno, a, id;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;
  struct jfg_adjectiv_desc adesc2;

  fprintf(jfr2c_op,
"/*---------------------------------------------------------------------*/\n");
  fprintf(jfr2c_op,
"/* Fuzzification-functions:                                            */\n\n");
  for (vno = 0; vno < jfr2c_spdesc.var_c; vno++)
  { if (jfr2c_varuse[vno].wvar == 1 && jfr2c_varuse[vno].rfzvar == 1)
    { jfg_var(&vdesc, jfr_head, vno);
      fprintf(jfr2c_op, "static void fu%d(void) /* %s */\n", vno, vdesc.name);
      fprintf(jfr2c_op, "{\n");
      if ((vdesc.flags & JFS_VF_NORMAL) != 0 && vdesc.fzvar_c > 0)
        fprintf(jfr2c_op, "  %s ns;\n", jfr2c_t_real);
      if (jfr2c_optimize == JFI_OPT_SPACE)
        fprintf(jfr2c_op, "  if (v%d!=h) {\n",vno);
      fprintf(jfr2c_dcl,"static void fu%d(void);\n", vno);
      for (a = 0; a < vdesc.fzvar_c; a++)
      { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
        if (adesc.limit_c > 0)
        { id = jfr2c_pl_find(2, vdesc.f_adjectiv_no + a);
          fprintf(jfr2c_op, "f%d=pl_calc(v%d,%d,%d);\n",
                vdesc.f_fzvar_no + a, vno,
                jfr2c_pl_adr[id].first, jfr2c_pl_adr[id].last);
        }
        else
        { fprintf(jfr2c_op, "if (v%d <= ", vno);
          jfr2c_float(jfr2c_op, adesc.trapez_start);
          if (a == 0)
            fprintf(jfr2c_op, ") f%d=1.0; \n",vdesc.f_fzvar_no + a);
          else
          { /* {if (vval <= pre) fzv=0;\n else fzv=(vval-pre)/(center-pre);} */
            fprintf(jfr2c_op, ") {if (v%d <= ",vno);
            jfg_adjectiv(&adesc2, jfr_head, vdesc.f_adjectiv_no + a - 1);
            jfr2c_float(jfr2c_op, adesc2.trapez_end);
            fprintf(jfr2c_op, ") f%d = 0.0;\n else f%d = (v%d-",
                   vdesc.f_fzvar_no + a,
                   vdesc.f_fzvar_no + a, vno);
            jfr2c_pfloat(adesc2.trapez_end);
            fprintf(jfr2c_op, ") / ");
            jfr2c_float(jfr2c_op, adesc.trapez_start-adesc2.trapez_end);
            fprintf(jfr2c_op,";} ");
          }
          if (adesc.trapez_start != adesc.trapez_end)
          { /* else if (vval <= trapez_end) fzv = 1.0;  */
            fprintf(jfr2c_op, "else if (v%d <= ", vno);
            jfr2c_float(jfr2c_op, adesc.trapez_end);
            fprintf(jfr2c_op, ") f%d = 1.0;\n ", vdesc.f_fzvar_no + a);
          }
          fprintf(jfr2c_op, "else ");
          if (a == vdesc.fzvar_c - 1)
            fprintf(jfr2c_op, "f%d = 1.0;\n", vdesc.f_fzvar_no + a);
          else
          { /* {if (vval>=post) fzv=0\n else fzv=(post-vval)/(post-center);} */
            jfg_adjectiv(&adesc2, jfr_head, vdesc.f_adjectiv_no + a +1);
            fprintf(jfr2c_op, "{ if (v%d >= ",vno);
            jfr2c_float(jfr2c_op, adesc2.trapez_start);
            fprintf(jfr2c_op, ") f%d=0.0;\n else f%d = (",
              vdesc.f_fzvar_no + a, vdesc.f_fzvar_no + a);
            jfr2c_float(jfr2c_op, adesc2.trapez_start);
            fprintf(jfr2c_op, " - v%d) / ", vno);
            jfr2c_float(jfr2c_op, adesc2.trapez_start-adesc.trapez_end);
            fprintf(jfr2c_op, ";};\n");
          }
        }
        if (adesc.flags & JFS_AF_HEDGE)
        { if (adesc.h2_no == adesc.h1_no)
            fprintf(jfr2c_op, "f%d=h%d(f%d);\n",
                    vdesc.f_fzvar_no + a, adesc.h1_no,
                    vdesc.f_fzvar_no + a);
          else
          { fprintf(jfr2c_op, "if (v%d <= ", vno);
            jfr2c_float(jfr2c_op, adesc.center);
            fprintf(jfr2c_op, ") f%d = h%d(f%d);\n else f%d = h%d(f%d);\n",
                     vdesc.f_fzvar_no + a, adesc.h1_no,
                     vdesc.f_fzvar_no + a, vdesc.f_fzvar_no + a,
                     adesc.h2_no, vdesc.f_fzvar_no + a);
          }
        }
      }
      if ((vdesc.flags & JFS_VF_NORMAL) != 0 && vdesc.fzvar_c > 0)
      { fprintf(jfr2c_op, "ns = ");
        for (a = 0; a < vdesc.fzvar_c; a++)
        { if (a != 0)
            fprintf(jfr2c_op, " + ");
          fprintf(jfr2c_op, "f%d", vdesc.f_fzvar_no + a);
        }
        fprintf(jfr2c_op, ";\n");
        fprintf(jfr2c_op, "if (ns != 0.0) {");
        if (vdesc.no_arg > 0.0)
        { fprintf(jfr2c_op, "ns = ");
          jfr2c_float(jfr2c_op, vdesc.no_arg);
          fprintf(jfr2c_op, " / ns;\n");
        }
        else
          fprintf(jfr2c_op, "ns = c%d / ns\n;", vno);
        for (a = 0; a < vdesc.fzvar_c; a++)
          fprintf(jfr2c_op, "f%d *= ns\n;", vdesc.f_fzvar_no + a);
        fprintf(jfr2c_op, "};");
      }
      if (jfr2c_optimize == JFI_OPT_SPACE)
        fprintf(jfr2c_op, "}\n");
      if (jfr2c_optimize == JFI_OPT_SPEED)
        fprintf(jfr2c_op, "vs%d = 0;\n", vno);
      fprintf(jfr2c_op, "};\n\n");
    }
  } /* for var */
}


static void jfr2c_vround_write(void)
{
  int a, vno;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;
  struct jfg_domain_desc ddesc;

  fprintf(jfr2c_op,
"/*---------------------------------------------------------------------*/\n");
  fprintf(jfr2c_op,
"/* Variable-rounding functions:                                        */\n\n");
  for (vno = 0; vno < jfr2c_spdesc.var_c; vno++)
  { if (jfr2c_varuse[vno].wvar == 1 || jfr2c_varuse[vno].rvar == 1)
    { jfg_var(&vdesc, jfr_head, vno);
      jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
      if (ddesc.type == JFS_DT_FLOAT && jfr2c_non_rounded == 1)
        jfr2c_varuse[vno].vround = 0;
      else
      { jfr2c_varuse[vno].vround = 1;
        fprintf(jfr2c_op, "static void vr%d(void) /* %s */\n{", vno, vdesc.name);
        fprintf(jfr2c_dcl,"static void vr%d(void);\n", vno);
        switch (ddesc.type)
        { case JFS_DT_INTEGER:
            fprintf(jfr2c_op, "double t, r;\n");
            fprintf(jfr2c_op, "t = modf((double) v%d, &r);", vno);
            fprintf(jfr2c_op, "if (t >= 0.5) r = r + 1.0;\n");
            fprintf(jfr2c_op, "else if (t <= -0.5) r = r - 1.0;");
            fprintf(jfr2c_op, "v%d=(%s) r;\n", vno, jfr2c_t_real);
            break;
          case JFS_DT_CATEGORICAL:
            jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no);
            fprintf(jfr2c_op, "  %s res, bdist;\n  res=", jfr2c_t_real);
            jfr2c_float(jfr2c_op, adesc.center);
            fprintf(jfr2c_op, ";  bdist=fabs(v%d-", vno);
            jfr2c_pfloat(adesc.center);
            fprintf(jfr2c_op, ");\n");
            for (a = 1; a <vdesc.fzvar_c; a++)
            { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
              fprintf(jfr2c_op, "  if (fabs(v%d - ", vno);
              jfr2c_pfloat(adesc.center);
              fprintf(jfr2c_op, ") < bdist)\n  {  res = ");
              jfr2c_float(jfr2c_op, adesc.center);
              fprintf(jfr2c_op, "; bdist = fabs(v%d - ",vno);
              jfr2c_pfloat(adesc.center);
              fprintf(jfr2c_op, ");\n  }\n");
            }
            fprintf(jfr2c_op, "  v%d = res;\n", vno);
            break;
          default:
            break;
        }
        if (jfr2c_non_rounded == 0)
        { if (ddesc.flags & JFS_DF_MINENTER)
          { if (ddesc.flags & JFS_DF_MAXENTER)
            { fprintf(jfr2c_op, "  v%d=rmm(v%d,", vno, vno);
              jfr2c_float(jfr2c_op, ddesc.dmin);
              fprintf(jfr2c_op, ",");
              jfr2c_float(jfr2c_op, ddesc.dmax);
            }
            else
            { if (jfr2c_use_minmax == 1)
                fprintf(jfr2c_op, "v%d = max(v%d, ", vno, vno);
              else
                fprintf(jfr2c_op, "v%d = rmax(v%d, ", vno, vno);
              jfr2c_float(jfr2c_op, ddesc.dmin);
            }
            fprintf(jfr2c_op, ");");
          }
          else
          { if (ddesc.flags & JFS_DF_MAXENTER)
            { if (jfr2c_use_minmax == 1)
                fprintf(jfr2c_op, "v%d = min(v%d, ", vno, vno);
              else
                fprintf(jfr2c_op, "v%d = rmin(v%d, ", vno, vno);
              jfr2c_float(jfr2c_op, ddesc.dmax);
              fprintf(jfr2c_op, ");");
            }
          }
        }
        fprintf(jfr2c_op, "\n}\n\n");
      }
    }
  }
}

static void jfr2c_defuz_write(void)
{
  int a, d, defuz_func, vno;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;
  struct jfg_domain_desc ddesc;

  fprintf(jfr2c_op,
"/*---------------------------------------------------------------------*/\n");
  fprintf(jfr2c_op,
"/* Defuzification functions:                                           */\n\n");
  for (vno = 0; vno < jfr2c_spdesc.var_c; vno++)
  { if (jfr2c_varuse[vno].wfzvar == 1 && jfr2c_varuse[vno].rvar == 1)
    { jfg_var(&vdesc, jfr_head, vno);
      /* s=f0+f1+..+fn;             */
      if (jfr2c_optimize == JFI_OPT_SPACE)
      {  fprintf(jfr2c_op,
           	     "static void df%d(%s n) /* %s */\n{ %s s",
                 vno, jfr2c_t_real, vdesc.name, jfr2c_t_real);
         fprintf(jfr2c_dcl, "static void df%d(%s n);\n", vno, jfr2c_t_real);
      }
      if (jfr2c_optimize == JFI_OPT_SPEED)
      {  fprintf(jfr2c_op, "static void df%d(void) /* %s */\n{ %s s",
                 vno, vdesc.name, jfr2c_t_real);
         fprintf(jfr2c_dcl, "static void df%d(void);\n", vno);
      }
      if (vdesc.defuz_1 != vdesc.defuz_2)
        fprintf(jfr2c_op, ", tv");
      if (vdesc.defuz_1 == JFS_VD_CENTROID || vdesc.defuz_2 == JFS_VD_CENTROID)
        fprintf(jfr2c_op, ", tael, naevn");
      if (vdesc.defuz_1 == JFS_VD_CMAX || vdesc.defuz_2 == JFS_VD_CMAX
          || vdesc.defuz_1 == JFS_VD_FMAX || vdesc.defuz_2 == JFS_VD_FMAX
          || vdesc.defuz_1 == JFS_VD_LMAX || vdesc.defuz_2 == JFS_VD_LMAX)
        fprintf(jfr2c_op, ", f, b, e;\n");
      else
        fprintf(jfr2c_op, ";\n");
      if (jfr2c_optimize == JFI_OPT_SPACE)
        fprintf(jfr2c_op, " if (n!=h)\n{");
      fprintf(jfr2c_op, " s = ");
      for (a = 0; a < vdesc.fzvar_c; a++)
      { if (a != 0)
	      fprintf(jfr2c_op, " + ");
        if (vdesc.acut == 0.0)
          fprintf(jfr2c_op, "f%d", vdesc.f_fzvar_no + a);
        else
        { fprintf(jfr2c_op, "cut(f%d, ", vdesc.f_fzvar_no + a);
          jfr2c_float(jfr2c_op, vdesc.acut);
          fprintf(jfr2c_op, ")");
        }
      }
      fprintf(jfr2c_op, ";\n    if (s==0.0)\n {");
      if (jfr2c_use_conf == 1)
        fprintf(jfr2c_op, " c%d = 0.0;", vno);
      fprintf(jfr2c_op," v%d = ", vno);
      jfr2c_float(jfr2c_op, vdesc.default_val);
      fprintf(jfr2c_op, ";}\n else {\n");

      /* c=max(f0,max(f1,..));   */
      if (jfr2c_use_conf == 1)
      { fprintf(jfr2c_op, " c%d = ", vno);
        for (a = 0; a < vdesc.fzvar_c - 1; a++)
        { if (jfr2c_use_minmax == 1)
            fprintf(jfr2c_op, "max(f%d, ", vdesc.f_fzvar_no + a);
          else
            fprintf(jfr2c_op, "rmax(f%d, ", vdesc.f_fzvar_no + a);
        }
        fprintf(jfr2c_op, "f%d", vdesc.f_fzvar_no + vdesc.fzvar_c - 1);
        for (a = 0; a < vdesc.fzvar_c - 1; a++)
          fprintf(jfr2c_op, ")");
        fprintf(jfr2c_op, ";\n");
      }
      defuz_func = vdesc.defuz_1;
      for (d = 0; d < 2; d++)
      { switch (defuz_func)
        { case JFS_VD_CENTROID:
            fprintf(jfr2c_op, "    tael = ");
            for (a = 0; a < vdesc.fzvar_c; a++)
            { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
              if (a != 0)
                fprintf(jfr2c_op, " + ");
              jfr2c_pfloat(0.5*adesc.base*adesc.center);
              if (vdesc.acut == 0)
                fprintf(jfr2c_op, " * f%d / s", vdesc.f_fzvar_no + a);
              else
              { fprintf(jfr2c_op, " * cut(f%d,", vdesc.f_fzvar_no + a);
                jfr2c_float(jfr2c_op, vdesc.acut);
                fprintf(jfr2c_op, ") / s");
              }
            }
            fprintf(jfr2c_op, ";\n    naevn = ");
            for (a = 0; a < vdesc.fzvar_c; a++)
            { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
              if (a != 0)
                fprintf(jfr2c_op, " + ");
              jfr2c_pfloat(0.5*adesc.base);
              if (vdesc.acut == 0)
                fprintf(jfr2c_op, " * f%d / s", vdesc.f_fzvar_no + a);
              else
              { fprintf(jfr2c_op, " * cut(f%d, ", vdesc.f_fzvar_no + a);
                jfr2c_float(jfr2c_op, vdesc.acut);
                fprintf(jfr2c_op, ") / s");
              }
            }
            fprintf(jfr2c_op, ";\n if (naevn == 0.0)\n{");
            if (jfr2c_use_conf == 1)
              fprintf(jfr2c_op, " c%d=0.0;", vno);
            fprintf(jfr2c_op, " v%d = ", vno);
            jfr2c_float(jfr2c_op, vdesc.default_val);
            fprintf(jfr2c_op, ";}\nelse\n v%d = tael / naevn;\n", vno);
            break;
          case JFS_VD_AVG:
            fprintf(jfr2c_op, " v%d = ", vno);
            for (a = 0; a < vdesc.fzvar_c; a++)
            { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
              if (a != 0)
                fprintf(jfr2c_op, " + ");
              jfr2c_pfloat(adesc.center);
              fprintf(jfr2c_op, " * ");
              if (vdesc.acut == 0.0)
                fprintf(jfr2c_op, "f%d / s",vdesc.f_fzvar_no + a);
              else
              { fprintf(jfr2c_op, "cut(f%d, ", vdesc.f_fzvar_no + a);
                jfr2c_float(jfr2c_op, vdesc.acut);
                fprintf(jfr2c_op, ") / s");
              }
            }
            fprintf(jfr2c_op, ";");
            break;
          case JFS_VD_CMAX:
          case JFS_VD_FMAX:
          case JFS_VD_LMAX:
            jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no);
            fprintf(jfr2c_op, "     f = f%d;", vdesc.f_fzvar_no);
            if (defuz_func != JFS_VD_LMAX)
            { fprintf(jfr2c_op, " b = ");
              jfr2c_float(jfr2c_op, adesc.center);
              fprintf(jfr2c_op, ";");
            }
            if (defuz_func != JFS_VD_FMAX)
            { fprintf(jfr2c_op, " e = ");
              jfr2c_float(jfr2c_op, adesc.center);
              fprintf(jfr2c_op, ";");
            }
            for (a = 1; a < vdesc.fzvar_c; a++)
            { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
              fprintf(jfr2c_op, "if (f%d > f) {f = f%d;",
                      vdesc.f_fzvar_no + a, vdesc.f_fzvar_no + a);
              if (defuz_func != JFS_VD_LMAX)
              { fprintf(jfr2c_op, "b = ");
                jfr2c_float(jfr2c_op, adesc.center);
                fprintf(jfr2c_op, ";");
              }
              if (defuz_func != JFS_VD_FMAX)
              { fprintf(jfr2c_op, "e = ");
                jfr2c_float(jfr2c_op, adesc.center);
                fprintf(jfr2c_op, ";");
              }
              fprintf(jfr2c_op, "};\n");
              if (defuz_func != JFS_VD_FMAX)
              { fprintf(jfr2c_op, "if (f%d==f) e=",vdesc.f_fzvar_no + a);
                jfr2c_float(jfr2c_op, adesc.center);
                fprintf(jfr2c_op, ";\n");
              }
            }
            if (defuz_func == JFS_VD_CMAX)
              fprintf(jfr2c_op, "    v%d = ( b + e) / 2.0;\n", vno);
            if (defuz_func == JFS_VD_FMAX)
              fprintf(jfr2c_op, "    v%d = b;\n", vno);
            if (defuz_func == JFS_VD_LMAX)
              fprintf(jfr2c_op, "    v%d = e;\n", vno);
            break;
          case JFS_VD_FIRST:
            jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
            if ((ddesc.flags & JFS_DF_MINENTER) != 0
                && (ddesc.flags & JFS_DF_MAXENTER) != 0)
            { /* v=f*(dmax-dmin)+dmin; */
              fprintf(jfr2c_op, "    v%d=f%d*(", vno, vdesc.f_fzvar_no);
              jfr2c_float(jfr2c_op, ddesc.dmax);
              fprintf(jfr2c_op, "-");
              jfr2c_pfloat(ddesc.dmin);
              fprintf(jfr2c_op, ")+");
              jfr2c_pfloat(ddesc.dmin);
              fprintf(jfr2c_op, ";\n");
            }
            else
              fprintf(jfr2c_op, "    v%d=f%d;\n",vno, vdesc.f_fzvar_no + a);
            break;
	       }
        if (d == 0)
        { if (vdesc.defuz_2 == vdesc.defuz_1)
            d = 7;
          else
          { fprintf(jfr2c_op, "    tv = v%d;\n", vno);
            defuz_func = vdesc.defuz_2;
          }
        }
        else
        { fprintf(jfr2c_op, "    v%d = tv * (1.0 - ", vno);
          jfr2c_pfloat(vdesc.defuz_arg);
          fprintf(jfr2c_op, ") + v%d * ", vno);
          jfr2c_pfloat(vdesc.defuz_arg);
          fprintf(jfr2c_op, ";\n");
        }
      }
      if (jfr2c_varuse[vno].vround == 1)
        fprintf(jfr2c_op, "    vr%d();\n", vno); 
      fprintf(jfr2c_op, " }\n");                 /* end of else-block */
      if (jfr2c_optimize == JFI_OPT_SPACE)
        fprintf(jfr2c_op, "}\n");
      if (jfr2c_optimize == JFI_OPT_SPEED)
        fprintf(jfr2c_op, "vs%d = 0;\n", vno);
      fprintf(jfr2c_op, "};\n\n");
    }
  } /* for (m in vars */
}

/***************************************************************************/
/* Check statement to for needed cals to fuz, defuz if speed-optimizes:    */

static void jfr2c_leaf_fuzdefuz_check(unsigned short id)
{
  struct jfg_tree_desc *leaf;
  struct jfg_fzvar_desc fzvdesc;

  leaf = &(jfr2c_tree[id]);
  switch (leaf->type)
  { case JFG_TT_OP:
    case JFG_TT_UREL:
    case JFG_TT_ARGLIST:
    case JFG_TT_IIF:
    case JFG_TT_DFUNC:
      jfr2c_leaf_fuzdefuz_check(leaf->sarg_1);
      jfr2c_leaf_fuzdefuz_check(leaf->sarg_2);
      break;
    case JFG_TT_HEDGE:
    case JFG_TT_UFUNC:
    case JFG_TT_ARVAL:
    case JFG_TT_SFUNC:
      jfr2c_leaf_fuzdefuz_check(leaf->sarg_1);
      break;
    case JFG_TT_VAR:
      jfr2c_varuse[leaf->sarg_1].expr_use |= 2;
      break;
    case JFG_TT_FZVAR:
      jfg_fzvar(&fzvdesc, jfr_head, leaf->sarg_1);
      jfr2c_varuse[fzvdesc.var_no].expr_use |= 1;
      break;
    case JFG_TT_BETWEEN:
      jfr2c_varuse[leaf->sarg_1].expr_use |= 1;
      break;
    case JFG_TT_VFUNC:
      if (leaf->op == JFS_VFU_DNORMAL)
	       jfr2c_varuse[leaf->sarg_1].expr_use |= 1;
      if (leaf->op == JFS_VFU_M_FZVAR || leaf->op == JFS_VFU_S_FZVAR)
	       jfr2c_varuse[leaf->sarg_1].expr_use |= 3;
      break;
    default:
      break;
  }
}

static void jfr2c_fuzdefuz_check(unsigned char *pc)
{
  int m, res;
  unsigned short c, i, e;
  struct jfg_statement_desc sdesc;
  struct jfg_fzvar_desc fzvdesc;

  jfg_statement(&sdesc, jfr_head, pc);
  for (m = 0; m < JFI_VARUSE_MAX; m++)
    jfr2c_varuse[m].expr_use = 0;

  switch (sdesc.type)
  { case JFG_ST_IF:
      res = jfg_if_tree(jfr2c_tree, jfr2c_maxtree, &c, &i, &e,
                        jfr_head, pc);
      if (res != 0)
        jf_error(res, jfr2c_spaces, JFE_ERROR);
      switch (sdesc.sec_type)
      { case JFG_SST_FZVAR:
          jfg_fzvar(&fzvdesc, jfr_head, sdesc.sarg_1);
          jfr2c_varuse[fzvdesc.var_no].expr_use |= 1;
          jfr2c_leaf_fuzdefuz_check(c);
          break;
        case JFG_SST_VAR:
        case JFG_SST_INC:
          jfr2c_varuse[sdesc.sarg_1].expr_use |= 2;
          jfr2c_leaf_fuzdefuz_check(c);
          jfr2c_leaf_fuzdefuz_check(e);
          break;
        case JFG_SST_ARR:
          jfr2c_leaf_fuzdefuz_check(c);
          jfr2c_leaf_fuzdefuz_check(i);
          jfr2c_leaf_fuzdefuz_check(e);
          break;
        case JFG_SST_PROCEDURE:
        case JFG_SST_RETURN:
        case JFG_SST_FUARG:
          jfr2c_leaf_fuzdefuz_check(c);
          jfr2c_leaf_fuzdefuz_check(e);
          break;
        default:
          jfr2c_leaf_fuzdefuz_check(c);
      }
      break;
    case JFG_ST_CASE:
      res = jfg_if_tree(jfr2c_tree, jfr2c_maxtree, &c, &i, &e, jfr_head, pc);
      if (res != 0)
        jf_error(res, jfr2c_spaces, JFE_ERROR);
      jfr2c_leaf_fuzdefuz_check(c);
      break;
    case JFG_ST_WHILE:
      res = jfg_if_tree(jfr2c_tree, jfr2c_maxtree, &c, &i, &e, jfr_head, pc);
      if (res != 0)
        jf_error(res, jfr2c_spaces, JFE_ERROR);
      jfr2c_leaf_fuzdefuz_check(c);
      break;
    case JFG_ST_WSET:
      res = jfg_if_tree(jfr2c_tree, jfr2c_maxtree, &c, &i, &e, jfr_head, pc);
      if (res != 0)
        jf_error(res, jfr2c_spaces, JFE_ERROR);
      jfr2c_leaf_fuzdefuz_check(c);
      break;
    default:
      break;
  }
  for (m = 0; m < JFI_VARUSE_MAX; m++)
  { if ((jfr2c_varuse[m].expr_use == 1 || jfr2c_varuse[m].expr_use == 3)
        && jfr2c_varuse[m].wvar == 1
        && jfr2c_varuse[m].rfzvar == 1
        && jfr2c_varuse[m].fuzed == 0)
    { fprintf(jfr2c_op, " if (vs%d == 1) fu%d();\n", m, m);
      jfr2c_varuse[m].fuzed = 1;
    }
    if ((jfr2c_varuse[m].expr_use == 2 || jfr2c_varuse[m].expr_use == 3)
        && jfr2c_varuse[m].wfzvar == 1
        && jfr2c_varuse[m].rvar == 1
        && jfr2c_varuse[m].defuzed == 0)
    { fprintf(jfr2c_op, " if (vs%d == 2) df%d();\n", m, m);
      jfr2c_varuse[m].defuzed = 1;
    }
  }
}

/**************************************************************************/
/* write statements:                                                      */

static void jfr2c_leaf_write(int id)
{
  struct jfg_tree_desc *leaf;
  struct jfg_array_desc adesc;
  struct jfg_operator_desc odesc;

  leaf = &(jfr2c_tree[id]);
  switch (leaf->type)
  { case JFG_TT_OP:
      jfg_operator(&odesc, jfr_head, leaf->op);
      if (jfr2c_fop_use[leaf->op] == 1)
      { fprintf(jfr2c_op, "o%d(", leaf->op);
        jfr2c_leaf_write(leaf->sarg_1);
        fprintf(jfr2c_op, ",");
        jfr2c_leaf_write(leaf->sarg_2);
        fprintf(jfr2c_op, ")");
      }
      else
      if (jfr2c_fop_use[leaf->op] == 2)
      { fprintf(jfr2c_op, "((");
        jfr2c_leaf_write(leaf->sarg_1);
        fprintf(jfr2c_op, ")) ");
        if (odesc.op_1 == JFS_FOP_PROD || odesc.op_1 == JFS_FOP_R8)
          fprintf(jfr2c_op, "* ((");
        jfr2c_leaf_write(leaf->sarg_2);
        fprintf(jfr2c_op, "))");
      }
      else
      { if (odesc.op_1 == JFS_FOP_MIN)
        { if (jfr2c_use_minmax)
            fprintf(jfr2c_op, "min(");
          else
            fprintf(jfr2c_op, "rmin(");
        }
        else
        if (odesc.op_1 == JFS_FOP_MAX)
        { if (jfr2c_use_minmax)
            fprintf(jfr2c_op, "max(");
          else
            fprintf(jfr2c_op, "rmax(");
        }
        jfr2c_leaf_write(leaf->sarg_1);
        fprintf(jfr2c_op, ",");
        jfr2c_leaf_write(leaf->sarg_2);
        fprintf(jfr2c_op, ")");
      }
      break;
    case JFG_TT_HEDGE:
      fprintf(jfr2c_op, "h%d(", leaf->op);
      jfr2c_leaf_write(leaf->sarg_1);
      fprintf(jfr2c_op, ")");
      break;
    case JFG_TT_UREL:
      fprintf(jfr2c_op, "r%d(", leaf->op);
      jfr2c_leaf_write(leaf->sarg_1);
      fprintf(jfr2c_op, ",");
      jfr2c_leaf_write(leaf->sarg_2);
      fprintf(jfr2c_op, ")");
      break;
    case JFG_TT_SFUNC:
      if (jfr2c_sfunc_use[leaf->op] == 1)
        fprintf(jfr2c_op, "s%d(", leaf->op);
      else
      { switch (leaf->op)
        { case JFS_SFU_COS:
            fprintf(jfr2c_op, "cos(");
            break;
          case JFS_SFU_SIN:
            fprintf(jfr2c_op, "sin(");
            break;
          case JFS_SFU_TAN:
            fprintf(jfr2c_op, "tan(");
            break;
          case JFS_SFU_ACOS:
            fprintf(jfr2c_op, "acos(");
            break;
          case JFS_SFU_ASIN:
            fprintf(jfr2c_op, "asin(");
            break;
          case JFS_SFU_ATAN:
            fprintf(jfr2c_op, "atan(");
            break;
          case JFS_SFU_LOG:
            fprintf(jfr2c_op, "log(");
            break;
          case JFS_SFU_FABS:
            fprintf(jfr2c_op, "fabs(");
            break;
          case JFS_SFU_FLOOR:
            fprintf(jfr2c_op, "floor(");
            break;
          case JFS_SFU_CEIL:
            fprintf(jfr2c_op, "ceil(");
            break;
          case JFS_SFU_NEGATE:
            fprintf(jfr2c_op, "-(");
            break;
          case JFS_SFU_SQR:
            fprintf(jfr2c_op, "sqr(");
            break;
          case JFS_SFU_SQRT:
            fprintf(jfr2c_op, "sqrt(");
            break;
        }
      }
      jfr2c_leaf_write(leaf->sarg_1);
      fprintf(jfr2c_op, ")");
      break;
    case JFG_TT_DFUNC:
      if (jfr2c_dfunc_use[leaf->op] == 1)     /* call d%(e1,e2) */
      { fprintf(jfr2c_op, "d%d(", leaf->op);
        jfr2c_leaf_write(leaf->sarg_1);
        fprintf(jfr2c_op, ",");
        jfr2c_leaf_write(leaf->sarg_2);
        fprintf(jfr2c_op, ")");
      }
      else
      if (jfr2c_dfunc_use[leaf->op] == 2)     /* ((e1) op (e2)) */
      {  fprintf(jfr2c_op, "((");
         jfr2c_leaf_write(leaf->sarg_1);
         fprintf(jfr2c_op, ") ");
         switch (leaf->op)
         {  case JFS_DFU_PLUS:
              fprintf(jfr2c_op, "+");
              break;
            case JFS_DFU_MINUS:
              fprintf(jfr2c_op, "-");
              break;
            case JFS_DFU_PROD:
              fprintf(jfr2c_op, "*");
              break;
            case JFS_DFU_DIV:
              fprintf(jfr2c_op, "/");
              break;
            case JFS_ROP_EQ:
              fprintf(jfr2c_op, "==");
              break;
            case JFS_ROP_NEQ:
             fprintf(jfr2c_op, "!=");
             break;
           case JFS_ROP_GT:
             fprintf(jfr2c_op, ">");
             break;
           case JFS_ROP_GEQ:
             fprintf(jfr2c_op, ">=");
             break;
           case JFS_ROP_LT:
             fprintf(jfr2c_op, "<");
             break;
           case JFS_ROP_LEQ:
             fprintf(jfr2c_op, "<=");
             break;
         }
         fprintf(jfr2c_op, " (");
         jfr2c_leaf_write(leaf->sarg_2);
         fprintf(jfr2c_op, "))");
      }
      else          /* 3: func(e1,e2) */
      { switch (leaf->op)
        { case JFS_DFU_POW:
            fprintf(jfr2c_op, "pow(");
            break;
          case JFS_DFU_MIN:
      	     if (jfr2c_use_minmax == 1)
              fprintf(jfr2c_op, "min(");
            else
              fprintf(jfr2c_op, "rmin(");  
            break;
          case JFS_DFU_MAX:
   	        if (jfr2c_use_minmax == 1)
              fprintf(jfr2c_op, "max(");
            else
              fprintf(jfr2c_op, "rmax(");
            break;
        }
        jfr2c_leaf_write(leaf->sarg_1);
        fprintf(jfr2c_op, ", ");
        jfr2c_leaf_write(leaf->sarg_2);
        fprintf(jfr2c_op, ")");
      }
      break;
    case JFG_TT_CONST:
      jfr2c_pfloat(leaf->farg);
      break;
    case JFG_TT_VAR:
      fprintf(jfr2c_op, "v%d", leaf->sarg_1);
      break;
    case JFG_TT_FZVAR:
      fprintf(jfr2c_op, "f%d", leaf->sarg_1);
      break;
    case JFG_TT_TRUE:
      fprintf(jfr2c_op, "1.0");
      break;
    case JFG_TT_FALSE:
      fprintf(jfr2c_op, "0.0");
      break;
    case JFG_TT_BETWEEN:
      fprintf(jfr2c_op, "b%d_%d_%d()",
	      leaf->sarg_1, leaf->sarg_2, leaf->op);
      break;
    case JFG_TT_VFUNC:
      jfr2c_vfunc_write(leaf->sarg_1, leaf->op);
      break;
    case JFG_TT_UFUNC:
      fprintf(jfr2c_op, "uf%d(", leaf->op);
      jfr2c_leaf_write(leaf->sarg_1);
      fprintf(jfr2c_op, ")");
      break;
    case JFG_TT_ARGLIST:
      jfr2c_leaf_write(leaf->sarg_1);
      fprintf(jfr2c_op,", ");
      jfr2c_leaf_write(leaf->sarg_2);
      break;
    case JFG_TT_UF_VAR:
      fprintf(jfr2c_op,"lv%d", leaf->sarg_1);
      break;
    case JFG_TT_IIF:
      fprintf(jfr2c_op,"iif(");
      jfr2c_leaf_write(leaf->sarg_1);
      fprintf(jfr2c_op, ",");
      jfr2c_leaf_write(leaf->sarg_2);
      fprintf(jfr2c_op, ")");
      break;
    case JFG_TT_ARVAL:
      fprintf(jfr2c_op, "a%d[(int) rmm(", leaf->op);
      jfg_array(&adesc, jfr_head, leaf->op);
      jfr2c_leaf_write(leaf->sarg_1);
      fprintf(jfr2c_op, ",0,%d)]", adesc.array_c - 1);
      break;
  }
}

static void jfr2c_init_write(char *funcname)
{
  int m, a, fu;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  /* struct jfg_adjectiv_desc adesc; */
  struct jfg_array_desc ardesc;

  fprintf(jfr2c_op,
"/*---------------------------------------------------------------------*/\n");
  fprintf(jfr2c_op,
"/* Main functions:                                                     */\n\n");

  fprintf(jfr2c_hfile, "void %s(%s *op, %s *ip",
                       funcname, jfr2c_t_real, jfr2c_t_real);
  fprintf(jfr2c_op,    "void %s(%s *op, %s *ip",
                       funcname, jfr2c_t_real, jfr2c_t_real);
  if (jfr2c_conf_func == 1)
  {
    fprintf(jfr2c_hfile, ", %s *conf", jfr2c_t_real);
    fprintf(jfr2c_op,    ", %s *conf", jfr2c_t_real);
  }
  fprintf(jfr2c_hfile, ");\n");
  fprintf(jfr2c_op, ")\n{\n");
  fprintf(jfr2c_op, "  %s t, r;\n", jfr2c_t_real);
  if (jfr2c_spdesc.array_c > 0)
    fprintf(jfr2c_op, "int ac;\n");

  fu = jfr2c_spdesc.function_c; /* main-function */
  if (jfr2c_switch_use == 1 && jfr2c_max_levels[fu] > 0)
    fprintf(jfr2c_op, "%s sw[%d];\n %s rw[%d];\n",
            jfr2c_t_real, jfr2c_max_levels[fu] + 1,
            jfr2c_t_real, jfr2c_max_levels[fu] + 1);


  for (m = 0; m < jfr2c_spdesc.var_c; m++)
  { jfg_var(&vdesc, jfr_head, m);
    if (m >= jfr2c_spdesc.f_ivar_no
	    && m < jfr2c_spdesc.f_ivar_no + jfr2c_spdesc.ivar_c)
    { jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
      if (m == jfr2c_spdesc.f_ivar_no)
      { fprintf(jfr2c_hfile,
                "\n\n/*---------------------------------------------*/\n");
        fprintf(jfr2c_hfile,
                    "/* INPUT:                                      */\n\n");
      }
      fprintf(jfr2c_hfile, "/*  ip[%d] = %s */\n",
                           m - jfr2c_spdesc.f_ivar_no, vdesc.name);
      fprintf(jfr2c_op, "  v%d = ip[%d];", m, m - jfr2c_spdesc.f_ivar_no);
      if (jfr2c_use_conf == 1)
      { if (jfr2c_conf_func == 1)
          fprintf(jfr2c_op, " c%d = conf[%d]; ",
                          m, m - jfr2c_spdesc.f_ivar_no);
        else
          fprintf(jfr2c_op, " c%d = 1.0; ", m);
      }
      if (jfr2c_varuse[m].conf_sum == 1)
        fprintf(jfr2c_op, "cs%d = 0.0;\n", m);
      else
        fprintf(jfr2c_op, "\n");
    }
    else
    { fprintf(jfr2c_op, "  v%d = ", m);
      jfr2c_float(jfr2c_op, vdesc.default_val);
      fprintf(jfr2c_op, ";");
      if (jfr2c_use_conf == 1)
        fprintf(jfr2c_op, " c%d = 0.0;", m);
      if (jfr2c_varuse[m].conf_sum == 1)
        fprintf(jfr2c_op, "cs%d = 0.0;\n", m);
      else
        fprintf(jfr2c_op, "\n");
      for (a = 0; a < vdesc.fzvar_c; a++)
      {	fprintf(jfr2c_op, "f%d = 0.0;", vdesc.f_fzvar_no + a);
        if (a > 0 && a % 5 == 0)
          fprintf(jfr2c_op, "\n  ");
      }
      fprintf(jfr2c_op, "\n");
      if (m >= jfr2c_spdesc.f_ovar_no
          && m < jfr2c_spdesc.f_ovar_no + jfr2c_spdesc.ovar_c)
      { if (m == jfr2c_spdesc.f_ovar_no)
        { fprintf(jfr2c_hfile,
                "\n\n/*---------------------------------------------*/\n");
          fprintf(jfr2c_hfile,
                    "/* OUTPUT:                                     */\n\n");
        }
        fprintf(jfr2c_hfile, "/*  op[%d] = %s */\n",
                              m - jfr2c_spdesc.f_ovar_no, vdesc.name);
      }
    }
  }
  for (m = 0; m < jfr2c_spdesc.array_c; m++)
  { jfg_array(&ardesc, jfr_head, m);
    fprintf(jfr2c_op, "for (ac=0; ac < %d; ac++) a%d[ac]=0.0;\n",
                     ardesc.array_c, m);
  }
  fprintf(jfr2c_op, "glw = 1.0; rmw = 1.0;\n");
}

static void jfr2c_rules_write(char *funcname)
{
  unsigned short c, i, e;
  unsigned char *pc;
  int a, fu, first, use_rmw, assign_statement, rm_optimize, stno;
  struct jfg_var_desc vdesc;
  struct jfg_statement_desc sdesc;
  struct jfg_fzvar_desc fzvdesc;
  struct jfg_function_desc fudesc;
  struct jfg_array_desc adesc;
  struct jfg_operator_desc odesc;

  rm_optimize = jfr2c_optimize;
  for (fu = 0; fu < jfr2c_spdesc.function_c + 1; fu++)
  { jfr2c_optimize = rm_optimize;
    jfr2c_fdefuzed_init();
    if (fu < jfr2c_spdesc.function_c)
    { jfr2c_optimize = JFI_OPT_SPACE; /* funcs/procedures always space-optimized*/
      jfg_function(&fudesc, jfr_head, fu);
      pc = fudesc.pc;
      first = 1;
      fprintf(jfr2c_op, "static %s uf%d(", jfr2c_t_real, fu);
      fprintf(jfr2c_dcl,"static %s uf%d(", jfr2c_t_real, fu);
      for (a = 0; a < fudesc.arg_c; a++)
      { if (first == 0)
        { fprintf(jfr2c_op, ",");
          fprintf(jfr2c_dcl,",");
        }
        first = 0;
        fprintf(jfr2c_op, "%s lv%d", jfr2c_t_real, a);
        fprintf(jfr2c_dcl,"%s lv%d", jfr2c_t_real, a);
      }
      fprintf(jfr2c_op, ") /* %s */\n{  %s rfw, r;\n",
                        fudesc.name, jfr2c_t_real);
      fprintf(jfr2c_dcl,");\n");
      if (jfr2c_switch_use == 1 && jfr2c_max_levels[fu] > 0)
        fprintf(jfr2c_op, "%s sw[%d];\n %s rw[%d];\n",
                    jfr2c_t_real, jfr2c_max_levels[fu] + 1,
                    jfr2c_t_real,jfr2c_max_levels[fu] + 1);
      fprintf(jfr2c_op, "rfw = glw;\n");
    }
    else
    { jfr2c_init_write(funcname);
      pc = jfr2c_spdesc.pc_start;

      jfr2c_ff_ltypes = 0;
      for (a = 0; a < jfr2c_spdesc.ivar_c; a++)
      { jfg_var(&vdesc, jfr_head, jfr2c_spdesc.f_ivar_no + a);
        if (vdesc.fzvar_c > 0 && jfr2c_varuse[jfr2c_spdesc.f_ivar_no + a].wvar == 1
            && jfr2c_varuse[jfr2c_spdesc.f_ivar_no + a].rfzvar == 1)
        { if (jfr2c_optimize == JFI_OPT_SPACE)
            fprintf(jfr2c_op, "  h = v%d + 1.0; fu%d();\n",
                    jfr2c_spdesc.f_ivar_no + a, jfr2c_spdesc.f_ivar_no + a);
          if (jfr2c_optimize == JFI_OPT_SPEED)
            fprintf(jfr2c_op, "vs%d = 1;\n", jfr2c_spdesc.f_ivar_no + a);
        }
      }
      fprintf(jfr2c_op, "\n");
    }
    jfg_statement(&sdesc, jfr_head, pc);
    stno = 1;
    while (sdesc.type != JFG_ST_EOP &&
       	   (!(sdesc.type == JFG_ST_STEND && sdesc.sec_type == 2)))
    { fprintf(jfr2c_op, "/* statement %d: */ \n", stno);
      if (jfr2c_optimize == JFI_OPT_SPEED)
        jfr2c_fuzdefuz_check(pc);
      switch (sdesc.type)
      { case JFG_ST_IF:
          jfg_if_tree(jfr2c_tree, jfr2c_maxtree, &c, &i, &e, jfr_head, pc);
          if ((sdesc.flags & 1) == 0 && jfr2c_tree[c].type == JFG_TT_TRUE)
          { use_rmw = 0;
            assign_statement = 1;
          }
          else
          { assign_statement = 0;
            if (jfr2c_fixed_glw == 1)
              use_rmw = 0;
            else
              use_rmw = 1;
            /* glw=r01(weigt_op(glw, lweigth)*expr); */
            if (use_rmw == 1)
              fprintf(jfr2c_op, " rmw = glw;");
            fprintf(jfr2c_op, " glw = ");
            if (jfr2c_non_rounded == 0)
              fprintf(jfr2c_op, "r01(");
            if ((sdesc.flags & 1) != 0)
            { jfg_operator(&odesc, jfr_head, JFS_ONO_WEIGHTOP);
              if (jfr2c_fop_use[JFS_ONO_WEIGHTOP] == 1)
              { fprintf(jfr2c_op, "o%d(glw,", JFS_ONO_WEIGHTOP);
                jfr2c_float(jfr2c_op, sdesc.farg);
                fprintf(jfr2c_op, ")");
              }
              else
              if (jfr2c_fixed_glw == 1)
                jfr2c_float(jfr2c_op, sdesc.farg);
              else
              if (jfr2c_fop_use[JFS_ONO_WEIGHTOP] == 2)
              { fprintf(jfr2c_op, "(glw * ");
                jfr2c_float(jfr2c_op, sdesc.farg);
                fprintf(jfr2c_op, ")");
              }
              else
              { if (odesc.op_1 == JFS_FOP_MIN)
                { if (jfr2c_use_minmax == 1)
                    fprintf(jfr2c_op, "min(");
                  else
                    fprintf(jfr2c_op, "rmin(");
                }
                else
                if (odesc.op_1 == JFS_FOP_MAX)
                { if (jfr2c_use_minmax == 1)
                    fprintf(jfr2c_op, "max(");
                  else
                    fprintf(jfr2c_op, "rmax(");
                }
                fprintf(jfr2c_op, "glw, ");
                jfr2c_float(jfr2c_op, sdesc.farg);
                fprintf(jfr2c_op, ")");
              }
              fprintf(jfr2c_op, " * (");
            }
            else
            { if (jfr2c_fixed_glw == 0)
                fprintf(jfr2c_op, "glw *");
              fprintf(jfr2c_op, " (");
            }
            jfr2c_leaf_write(c);
            if (jfr2c_non_rounded == 0)
              fprintf(jfr2c_op, ")");
            fprintf(jfr2c_op, ");\n");
          }
          /* if (glw!=0.0) {...*/
          if (!(assign_statement == 1 && jfr2c_fixed_glw == 1))
            fprintf(jfr2c_op, "  if (glw != 0.0)\n  { ");

          switch (sdesc.sec_type)
          { case JFG_SST_FZVAR:
              jfg_fzvar(&fzvdesc, jfr_head, sdesc.sarg_1);
              jfg_var(&vdesc, jfr_head, fzvdesc.var_no);
              jfg_operator(&odesc, jfr_head, vdesc.f_comp);
              fprintf(jfr2c_op, " h = f%d ;f%d = ",
                      sdesc.sarg_1, sdesc.sarg_1);
              if (jfr2c_fop_use[vdesc.f_comp] == 1)
              { fprintf(jfr2c_op, "o%d(f%d, glw);", vdesc.f_comp, sdesc.sarg_1);
              }
              else
              if (jfr2c_fop_use[vdesc.f_comp] == 2)
              { fprintf(jfr2c_op, "(f%d * glw);", sdesc.sarg_1);
              }
              else
              { if (odesc.op_1 == JFS_FOP_MIN)
                { if (jfr2c_use_minmax)
                    fprintf(jfr2c_op, "min(");
                  else
                    fprintf(jfr2c_op, "rmin(");
                }
                else
                if (odesc.op_1 == JFS_FOP_MAX)
                { if (jfr2c_use_minmax == 1)
                    fprintf(jfr2c_op, "max(");
                  else
                    fprintf(jfr2c_op, "rmax(");
                }
                fprintf(jfr2c_op, "f%d, glw);", sdesc.sarg_1);
              }
              if (jfr2c_varuse[fzvdesc.var_no].wfzvar == 1
                  && jfr2c_varuse[fzvdesc.var_no].rvar == 1)
              { if (jfr2c_optimize == JFI_OPT_SPACE)
                  fprintf(jfr2c_op, "df%d(f%d);",fzvdesc.var_no, sdesc.sarg_1);
                else
                if (jfr2c_optimize == JFI_OPT_SPEED)
                { fprintf(jfr2c_op, " if (f%d != h) vs%d = 2;\n",
                                   sdesc.sarg_1, fzvdesc.var_no);
                  jfr2c_varuse[fzvdesc.var_no].defuzed = 0;
                }
              }
              if (!(assign_statement == 1 && jfr2c_fixed_glw == 1))
                fprintf(jfr2c_op, "  };\n");
              if (use_rmw == 1)
                fprintf(jfr2c_op, " glw = rmw;\n");
              break;
            case JFG_SST_VAR:
              jfg_var(&vdesc, jfr_head, sdesc.sarg_1);
              fprintf(jfr2c_op, "  h = v%d; r = ", sdesc.sarg_1);
              jfr2c_leaf_write(e);
              fprintf(jfr2c_op, ";\n");
              switch(vdesc.d_comp)
              { case JFS_VCF_NEW:
                  fprintf(jfr2c_op, "  v%d = r;", sdesc.sarg_1);
                  if (jfr2c_use_conf == 1)
                    fprintf(jfr2c_op, " c%d = glw;", sdesc.sarg_1);
                  break;
                case JFS_VCF_AVG:
                  fprintf(jfr2c_op, "  r = v%d * cs%d + r * glw; cs%d += glw;",
                          sdesc.sarg_1, sdesc.sarg_1, sdesc.sarg_1);
                  fprintf(jfr2c_op, "  v%d = r / cs%d;\n",
                          sdesc.sarg_1, sdesc.sarg_1);
                  break;
                case JFS_VCF_MAX:
                  fprintf(jfr2c_op, "  if (glw > c%d)\n  { v%d = r; c%d = glw;};\n",
                          sdesc.sarg_1, sdesc.sarg_1, sdesc.sarg_1);
                  break;
              }
              if (jfr2c_varuse[sdesc.sarg_1].vround == 1)
                fprintf(jfr2c_op, "  vr%d();", sdesc.sarg_1);
              if (jfr2c_varuse[sdesc.sarg_1].wvar == 1
                  && jfr2c_varuse[sdesc.sarg_1].rfzvar == 1)
              { if (jfr2c_optimize == JFI_OPT_SPACE)
                  fprintf(jfr2c_op, "  fu%d();", sdesc.sarg_1);
                else
                if (jfr2c_optimize == JFI_OPT_SPEED)
                { fprintf(jfr2c_op, " if (v%d != h) vs%d = 1;\n",
                                  sdesc.sarg_1, sdesc.sarg_1);
                  jfr2c_varuse[sdesc.sarg_1].fuzed = 0;
                }
              }
              if (!(assign_statement == 1 && jfr2c_fixed_glw == 1))
                fprintf(jfr2c_op, "}\n");
              if (use_rmw == 1)
                fprintf(jfr2c_op, " glw = rmw;\n");
              break;
            case JFG_SST_ARR:
              fprintf(jfr2c_op, "  a%d[(int) rmm(", sdesc.sarg_1);
              jfr2c_leaf_write(i);
              jfg_array(&adesc, jfr_head, sdesc.sarg_1);
              fprintf(jfr2c_op, ",0.0,");
              jfr2c_float(jfr2c_op, adesc.array_c - 1.0);
              fprintf(jfr2c_op, ")] = ");
              jfr2c_leaf_write(e);
              fprintf(jfr2c_op, ";\n");
              if (!(assign_statement == 1 && jfr2c_fixed_glw == 1))
                 fprintf(jfr2c_op, "}\n");
              if (use_rmw == 1)
                fprintf(jfr2c_op, " glw = rmw;\n");
              break;
            case JFG_SST_INC:
              jfg_var(&vdesc, jfr_head, sdesc.sarg_1);
              fprintf(jfr2c_op, "  h = v%d; r = ", sdesc.sarg_1);
              jfr2c_leaf_write(e);
              fprintf(jfr2c_op, ";\n");
              if (sdesc.flags & 4)
                fprintf(jfr2c_op, "  v%d -= r * glw;\n", sdesc.sarg_1);
              else
                fprintf(jfr2c_op, "  v%d += r* glw;\n", sdesc.sarg_1);
              if (jfr2c_use_conf == 1)
                fprintf(jfr2c_op, "  if (glw > c%d) c%d = glw;\n",
                        sdesc.sarg_1, sdesc.sarg_1);
              if (jfr2c_varuse[sdesc.sarg_1].vround == 1)
                fprintf(jfr2c_op, "  vr%d();", sdesc.sarg_1);
              if (jfr2c_varuse[sdesc.sarg_1].wvar == 1
                  && jfr2c_varuse[sdesc.sarg_1].rfzvar == 1)
              { if (jfr2c_optimize == JFI_OPT_SPACE)
                  fprintf(jfr2c_op, " fu%d();\n", sdesc.sarg_1);
                else
                if (jfr2c_optimize == JFI_OPT_SPEED)
                { fprintf(jfr2c_op, " if (v%d != h) vs%d = 1;\n",
                                  sdesc.sarg_1, sdesc.sarg_1);
                  jfr2c_varuse[sdesc.sarg_1].fuzed = 0;
                }
              }
              if (!(assign_statement == 1 && jfr2c_fixed_glw == 1))
                 fprintf(jfr2c_op, "}\n");
              if (use_rmw == 1)
                fprintf(jfr2c_op, " glw = rmw;\n");
              break;
            case JFG_SST_EXTERN:
              a = jfg_a_statement(jfr2c_words, JFI_MAX_WORDS, jfr_head, pc);
              if (a > 0)
              { if (strcmp(jfr2c_words[0], jfr2c_t_c) == 0)
                { if (a > 1)
                  { char *w;
                    w = (char *)(malloc (strlen(jfr2c_words[1]) + 1));
                    if (w != NULL) strcpy(w, jfr2c_words[1]);
                    if (w[0] == '"')
                      w++;
                    if (w[strlen(w) - 1] == '"')
                    w[strlen(w) - 1] = '\0';
                    fprintf(jfr2c_op, "%s;\n", w);
                    free(w);
                  }
                }
                else
                 jf_error(1008, jfr2c_words[0], JFE_WARNING);
              }
              if (!(assign_statement == 1 && jfr2c_fixed_glw == 1))
                fprintf(jfr2c_op," }\n");
              if (use_rmw == 1)
                fprintf(jfr2c_op, " glw = rmw;\n");
              jfr2c_fdefuzed_init();
              break;
            case JFG_SST_CLEAR:
              jfg_var(&vdesc, jfr_head, sdesc.sarg_1);
              fprintf(jfr2c_op, "  v%d = ", sdesc.sarg_1);
              jfr2c_float(jfr2c_op, vdesc.default_val);
              fprintf(jfr2c_op, ";");
              if (jfr2c_use_conf == 1)
                fprintf(jfr2c_op, " c%d = 0.0; ", sdesc.sarg_1);
              if (jfr2c_varuse[sdesc.sarg_1].conf_sum == 1)
                 fprintf(jfr2c_op, "cs%d = 0.0;\n", sdesc.sarg_1);
              else
                fprintf(jfr2c_op, "\n");

              for (a = 0; a < vdesc.fzvar_c; a++)
                fprintf(jfr2c_op, "f%d = 0.0; ", vdesc.f_fzvar_no + a);
              if (!(assign_statement == 1 && jfr2c_fixed_glw == 1))
                fprintf(jfr2c_op, " }\n");
              if (use_rmw == 1)
                fprintf(jfr2c_op, " glw = rmw;\n");
              jfr2c_fdefuzed_init();
              break;
            case JFG_SST_PROCEDURE:
              jfr2c_leaf_write(e);
              if (!(assign_statement == 1 && jfr2c_fixed_glw == 1))
                fprintf(jfr2c_op, ";};\n");
              if (use_rmw == 1)
                fprintf(jfr2c_op, " glw = rmw;\n");
              break;
            case JFG_SST_RETURN:
              fprintf(jfr2c_op, "  glw = rfw;\n  return ");
              jfr2c_leaf_write(e);
              if (!(assign_statement == 1 && jfr2c_fixed_glw == 1))
                fprintf(jfr2c_op, ";};\n");
              if (use_rmw == 1)
                fprintf(jfr2c_op, " glw = rmw;\n");
              break;
            case JFG_SST_FUARG:
              fprintf(jfr2c_op, "  lv%d", sdesc.sarg_1);
              fprintf(jfr2c_op, " = ");
              jfr2c_leaf_write(e);
              if (!(assign_statement == 1 && jfr2c_fixed_glw == 1))
                fprintf(jfr2c_op, ";}\n");
              if (use_rmw == 1)
                fprintf(jfr2c_op, " glw = rmw;\n");
              break;
          }
          break;
        case JFG_ST_CASE:
           jfg_if_tree(jfr2c_tree, jfr2c_maxtree, &c, &i, &e, jfr_head, pc);
           fprintf(jfr2c_op, "  glw = ");
           jfr2c_leaf_write(c);
           fprintf(jfr2c_op, ";");
           fprintf(jfr2c_op, "  sw[%d] += glw;", jfr2c_ff_ltypes - 1);
           fprintf(jfr2c_op, "  glw = o%d(rw[%d], glw);", JFS_ONO_CASEOP,
                   jfr2c_ff_ltypes - 1);
           break;
        case JFG_ST_WSET:
          jfg_if_tree(jfr2c_tree, jfr2c_maxtree, &c, &i, &e, jfr_head, pc);
          fprintf(jfr2c_op, "  if (glw != 0.0) glw = ");
          jfr2c_leaf_write(c);
          fprintf(jfr2c_op, ";\n");
          break;
        case JFG_ST_DEFAULT:
   	      if (jfr2c_use_minmax == 1)
            fprintf(jfr2c_op, "  glw = o%d(rw[%d],max(0.0, 1.0 - sw[%d]));\n",
                     JFS_ONO_CASEOP, jfr2c_ff_ltypes - 1,
                     jfr2c_ff_ltypes - 1);
          else
            fprintf(jfr2c_op, "  glw = o%d(rw[%d],rmax(0.0, 1.0 - sw[%d]));\n",
                     JFS_ONO_CASEOP, jfr2c_ff_ltypes - 1,
                     jfr2c_ff_ltypes - 1);
           break;
        case JFG_ST_STEND:
           jfr2c_ff_ltypes--;
           if (jfr2c_ltypes[jfr2c_ff_ltypes] == 0) /* switch statement */
             fprintf(jfr2c_op, "  glw = rw[%d];\n", jfr2c_ff_ltypes);
           else    /* while-statement */
             fprintf(jfr2c_op, "  glw = rw[%d];}\nglw = rw[%d];\n",
                             jfr2c_ff_ltypes, jfr2c_ff_ltypes);
           break;
        case JFG_ST_SWITCH:
           jfr2c_ltypes[jfr2c_ff_ltypes] = 0;
           fprintf(jfr2c_op, "  rw[%d] = glw;\n", jfr2c_ff_ltypes);
           fprintf(jfr2c_op, "  sw[%d] = 0.0;\n", jfr2c_ff_ltypes);
           jfr2c_ff_ltypes++;
           if (jfr2c_ff_ltypes >= JFI_LTYPES_MAX)
             jf_error(1007, jfr2c_spaces, JFE_ERROR);
           break;
        case JFG_ST_WHILE:
           jfr2c_ltypes[jfr2c_ff_ltypes] = 1;
           fprintf(jfr2c_op, "  rw[%d] = glw; rmw = glw;\n", jfr2c_ff_ltypes);
           fprintf(jfr2c_op, "  while (1 == 1)\n  {\n");
           fprintf(jfr2c_op, "    glw = o%d(glw,(", JFS_ONO_WHILEOP);
           jfg_if_tree(jfr2c_tree, jfr2c_maxtree, &c, &i, &e, jfr_head, pc);
           jfr2c_leaf_write(c);
           fprintf(jfr2c_op, "));\n    if ( glw == 0.0) break;\n");
           jfr2c_ff_ltypes++;
           if (jfr2c_ff_ltypes >= JFI_LTYPES_MAX)
             jf_error(1007, jfr2c_spaces, JFE_ERROR);
           jfr2c_fdefuzed_init();
           break;
        default:
           break;
      }
      fprintf(jfr2c_op, "\n");
      stno++;
      pc = sdesc.n_pc;
      jfg_statement(&sdesc, jfr_head, pc);
    }
    if (fu < jfr2c_spdesc.function_c)
    { fprintf(jfr2c_op, "  glw = rfw;\n  return 0.0;\n");
      fprintf(jfr2c_op, "}\n");
    }
  }
}

static void jfr2c_jfs_write(char *funcname)
{
  int m, vno;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;

  jfr2c_rules_write(funcname);

  for (m = 0; m < jfr2c_spdesc.ovar_c; m++)
  { vno = jfr2c_spdesc.f_ovar_no + m;
    jfg_var(&vdesc, jfr_head, vno);
    jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
    if (jfr2c_optimize == JFI_OPT_SPEED)
    { if (jfr2c_varuse[vno].wfzvar == 1
                  && jfr2c_varuse[vno].rvar == 1)
        fprintf(jfr2c_op, " if (vs%d == 2) df%d();\n", vno, vno);
    }
    fprintf(jfr2c_op, "op[%d]=v%d;\n", m ,vno);
  }
  fprintf(jfr2c_op, "};\n\n");
}

static void jfr2c_t_write(const char **txts)
{
  int m, slut;
  char tmpt[1024];

  slut = 0;
  for (m = 0; slut == 0; m++)
  { if (strcmp(txts[m], jfr2c_t_end) == 0)
      slut = 1;
    else
    { if (jfr2c_use_double == 1)
      { jfr2c_subst(tmpt, txts[m], "double", "float");
        fprintf(jfr2c_op, "%s\n", tmpt);
      }
      else
        fprintf(jfr2c_op, "%s\n", txts[m]);
    }
  }
}

static int jfr2c_program_write(char *funcname, char *progfname, char *hfname)
{
  int m, res, fundet;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;

  jfr2c_decl_check();
  jfr2c_program_check();

  fprintf(jfr2c_hfile,
"/*---------------------------------------------------------------------*/\n");
  fprintf(jfr2c_hfile, "/* %s  */\n", hfname);
  fprintf(jfr2c_hfile, "/* generated by JFR2C */\n");
  fprintf(jfr2c_hfile,
"/*---------------------------------------------------------------------*/\n\n");

  fprintf(jfr2c_dcl,
"/*---------------------------------------------------------------------*/\n");
  fprintf(jfr2c_dcl, "/* %s  */\n", progfname);
  fprintf(jfr2c_dcl, "/* generated by JFR2C */\n");
  fprintf(jfr2c_dcl,
"/*---------------------------------------------------------------------*/\n\n");

  fprintf(jfr2c_dcl, "#include <stdlib.h>\n");
  fprintf(jfr2c_dcl, "#include <math.h>\n\n");

  /* Write constants and variable-declarations */
  fprintf(jfr2c_dcl, "static %s glw, rmw, h; \n\n", jfr2c_t_real);
  res = jfr2c_pl_write();  /* constants in pl-functions */
  if (res != 0)
    return res;
  jfr2c_var_write(); /* domain variables */
  jfr2c_array_write();
  jfr2c_fzvar_write(); /* fuzzy variables */

  fprintf(jfr2c_dcl,
"\n/*---------------------------------------------------------------*/\n");
  fprintf(jfr2c_dcl,
"/* Function declarations:                                        */\n\n");

  if (jfr2c_use_inline == 0)
  { fprintf(jfr2c_dcl, "static %s rmm(%s v, %s mi, %s ma);\n",
                        jfr2c_t_real, jfr2c_t_real, jfr2c_t_real, jfr2c_t_real);
    fprintf(jfr2c_dcl, "static %s cut(%s a, %s v);\n",
                        jfr2c_t_real, jfr2c_t_real, jfr2c_t_real);
    fprintf(jfr2c_dcl, "static %s r01(%s v);\n", jfr2c_t_real, jfr2c_t_real);
    jfr2c_t_write(jfr2c_fixed);
  }
  else
  { fprintf(jfr2c_dcl, "inline static %s rmm(%s v, %s mi, %s ma);\n",
                       jfr2c_t_real, jfr2c_t_real, jfr2c_t_real, jfr2c_t_real);
    fprintf(jfr2c_dcl, "inline static %s cut(%s a, %s v);\n",
                       jfr2c_t_real, jfr2c_t_real, jfr2c_t_real);
    fprintf(jfr2c_dcl, "inline static %s r01(%s v);\n",
                       jfr2c_t_real, jfr2c_t_real);
    jfr2c_t_write(jfr2c_i_fixed);
  }

  if (jfr2c_use_minmax == 0)
  { if (jfr2c_use_inline == 0)
    { fprintf(jfr2c_dcl, "static %s rmin(%s v, %s mi);\n",
                         jfr2c_t_real, jfr2c_t_real, jfr2c_t_real);
      fprintf(jfr2c_dcl, "static %s rmax(%s v, %s ma);\n",
                         jfr2c_t_real, jfr2c_t_real, jfr2c_t_real);
      jfr2c_t_write(jfr2c_t_minmax);
    }
    else
    { fprintf(jfr2c_dcl, "inline static %s rmin(%s v, %s mi);\n",
                         jfr2c_t_real, jfr2c_t_real, jfr2c_t_real);
      fprintf(jfr2c_dcl, "inline static %s rmax(%s v, %s ma);\n",
                         jfr2c_t_real, jfr2c_t_real, jfr2c_t_real);
      jfr2c_t_write(jfr2c_it_minmax);
    }
  }
  fundet = 0;
  for (m = 0; fundet == 0 && m < jfr2c_spdesc.ivar_c; m++)
  { jfg_var(&vdesc, jfr_head, jfr2c_spdesc.f_ivar_no + m);
    jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
    if (   (ddesc.flags & JFS_DF_MINENTER) != 0
        || (ddesc.flags & JFS_DF_MAXENTER) != 0)
      fundet = 1;
  }

  if (jfr2c_ff_pl_adr > 0)
  { if (jfr2c_plf_excl == 1)
      jfr2c_t_write(jfr2c_t_plcalc);
    else
      jfr2c_t_write(jfr2c_t_splcalc);
  }

  jfr2c_hedges_write();
  jfr2c_relations_write();

  jfr2c_sfunc_write();
  jfr2c_dfunc_write();
  jfr2c_operators_write();
  if (jfr2c_iif_use == 1)
    jfr2c_t_write(jfr2c_t_iif);
  jfr2c_between_write();
  jfr2c_vround_write();
  jfr2c_fuz_write();
  jfr2c_defuz_write();
  jfr2c_jfs_write(funcname);
  return 0;
}

static int jfr2c_file_append(char *dfname, char *sfname)
{
  char buf[300];
  FILE *ip;
  FILE *op;
  int res, m, t;

  res = 0;
  if ((op = fopen(dfname, "a")) == NULL)
    return jf_error(1, dfname, JFE_ERROR);
  if ((ip = fopen(sfname, "r")) == NULL)
    return jf_error(1, sfname, JFE_ERROR);
  m = 1;
  while (m > 0 && res == 0)
  { m = fread(buf, 1, 255, ip);
    if (m > 0)
    { t = fwrite(buf, 1, m, op);
      if (t != m)
        res = jf_error(2, dfname, JFE_ERROR);
    }
  }
  fclose(ip);
  fclose(op);
  return 0;
}

int jfr2c_conv(char *opfname, char *func_name, char *ipfname,
               int precision, int non_protected, int non_rounded,
               int use_relations, int use_minmax, int use_inline,
               int optimize, int conf_func,
               int use_double,
   	           int maxtree, int maxstack, FILE *sout)
{
  int m, res;
  void *head;
  char ophname[256];
  char tmpfname[256];

  jfr2c_sout = sout;
  head = NULL;
  jfr2c_op = NULL;
  jfr2c_dcl = NULL;
  jfr2c_hfile = NULL;
  jfr2c_errcount = 0;
  jfr2c_ff_pl_adr = 0;
  jfr2c_plf_excl = 0;

  strcpy(ophname, opfname);
  jfr2c_ext_subst(ophname, "h");
  strcpy(tmpfname, opfname);
  jfr2c_ext_subst(tmpfname, "tmp");

  if ((m = jfr_init(0)) != 0)
   return jf_error(m, jf_empty, JFE_ERROR);

  if ((m = jfr_load(&head, ipfname)) != 0)
    return jf_error(m, ipfname, JFE_ERROR);

  if ((jfr2c_dcl = fopen(opfname, "w")) == NULL)
    return jf_error(1, opfname, JFE_ERROR);

  if ((jfr2c_op = fopen(tmpfname, "w")) == NULL)
    return jf_error(1, tmpfname, JFE_ERROR);

  if ((jfr2c_hfile = fopen(ophname, "w")) == NULL)
    return jf_error(1, ophname, JFE_ERROR);


  jfr_head = head;
  jfr2c_digits = precision;
  jfr2c_non_protected = non_protected;
  jfr2c_non_rounded = non_rounded;
  jfr2c_use_relations = use_relations;
  jfr2c_use_minmax = use_minmax;
  jfr2c_use_inline = use_inline;
  jfr2c_optimize = optimize;
  jfr2c_conf_func = conf_func;
  jfr2c_use_conf = jfr2c_conf_func == 1;
  jfr2c_use_double = use_double;
  if (use_double == 1)
    strcpy(jfr2c_t_real, jfr2c_t_double);
  else
    strcpy(jfr2c_t_real, jfr2c_t_float);
  jfr2c_maxtree = maxtree;
  m = sizeof(struct jfg_tree_desc) * jfr2c_maxtree;
  jfr2c_tree = (struct jfg_tree_desc *) malloc(m);
  if (jfr2c_tree == NULL)
    return jf_error(6, jfr2c_spaces, JFE_ERROR);

  jfr2c_stacksize = maxstack;

  m = jfg_init(0, jfr2c_stacksize, jfr2c_digits);
  if (m != 0)
    return jf_error(m, jfr2c_t_stack, JFE_ERROR);

  jfg_sprg(&jfr2c_spdesc, jfr_head);

  res = jfr2c_program_write(func_name, opfname, ophname);

  jfg_free();
  if (jfr2c_op != NULL)
    fclose(jfr2c_op);
  jfr2c_op = NULL;
  if (jfr2c_hfile != NULL)
    fclose(jfr2c_hfile);
  jfr2c_hfile = NULL;
  if (jfr2c_dcl != NULL)
    fclose(jfr2c_dcl);
  jfr2c_dcl = NULL;

  if (res == 0)
  { jfr2c_file_append(opfname, tmpfname);
    remove(tmpfname);
  }

  jfr_close(&head);
  jfr_free();

  if (jfr2c_tree != NULL)
    free(jfr2c_tree);
  return jfr2c_errcount;
}


