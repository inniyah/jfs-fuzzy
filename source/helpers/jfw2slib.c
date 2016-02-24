  /*********************************************************************/
  /*                                                                   */
  /* jfw2slib.cpp Version 2.01 Copyright (c) 1999-2000 Jan E. Mortensen*/
  /*                                                                   */
  /* JFS Invers Compiler-functions. Converts a JFW-file to a JFS-file. */
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
#include "jfr_gen.h"
#include "jfs_cons.h"
#include "jfs_text.h"
#include "jfw2slib.h"

#define JFI_CHPRLINE 78


static struct jfr_head_desc jf_head;
static struct jfw_head_desc jfw_head;
static char *jfc_ff_jfr;
static struct jfw_synonym_desc *jfw2s_synonyms;

static int jfw2s_write_vnos = 0; /* write variable-numbers. */
static int jfw2s_digits = 5;

#define JFI_OP_COMPACT 0
#define JFI_OP_ALL     1
static int jfw2s_op_mode = JFI_OP_ALL;  /* write all keywords */

static char *jfw2s_text = NULL;
static int   jfw2s_maxtext = 1024;

static int jfw2s_errno = 0;

static char jfw2s_spaces[] = "    ";
static char jfw2s_sspace[] = " ";

static FILE *jfw2s_op;
static FILE *jfw2s_ip;
static FILE *jfw2s_err;

static char jfw2s_t_quote[]    = "\"";
static char jfw2s_t_scolon[]   = ";";
static char jfw2s_t_colon[]    = ":";
static char jfw2s_t_defuz[]    = "defuz";
static char jfw2s_t_acut[]     = "acut";
static char jfw2s_t_f_comp[]   = "f_comp";
/* static char jfw2s_t_flag[]     = "flag"; */
static char jfw2s_t_default[]  = "default";
static char jfw2s_t_normal[]   = "normal";
static char jfw2s_t_base[]     = "base";
static char jfw2s_t_hedge[]    = "hedge";
static char jfw2s_t_hedgemode[]= "hedgemode";
static char jfw2s_t_conf[]     = "conf";
static char jfw2s_t_d_comp[]   = "d_comp";
static char jfw2s_t_center[]   = "center";
static char jfw2s_t_trapez[]   = "trapez";
static char jfw2s_t_or[]       = "or";
static char jfw2s_t_precedence[] = "precedence";
static char jfw2s_t_stack[]    = "jfg-stack";
static char jfw2s_t_text[]     = "text";
static char jfw2s_t_type[]     = "type";
static char jfw2s_t_min[]      = "min";
static char jfw2s_t_max[]      = "max";
static char jfw2s_t_domain[]   = "domain";
static char jfw2s_t_argument[] = "argument";
static char jfw2s_t_plf[]      = "plf";
static char jfw2s_t_size[]     = "size";



#define JFE_NONE    0
#define JFE_WARNING 1
#define JFE_ERROR   2
#define JFE_FATAL   3

static int jfw2s_errmode;

struct jfr_err_desc { int eno;
                      const char *text;
                    };


static struct jfr_err_desc jfr_err_texts[] =
        {    {   0, " "},
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
             { 301, "statement to long."},
             { 302, "jfg-tree to small to hold statement."},
             { 303, "Stack-overflow (jfg-stack)."},
             { 304, "program-id (pc) is not the start of a statement."},
             {  99, "unknown error."}
       };

static int jf_error(int errno, const char *name, int mode);
static void jf_ftoa(char *txt, float f, int pct);
static void jfw2s_write(char *text);
static void jfw2s_qwrite(char *text);
static void jfw2s_dwrite(float d, int pct);
static void jfw2s_plf_wwrite(float d, int pct, int exclusiv);
static void jfw2s_flush(void);
static void jfw2s_comment_write(int comment_no);
static void jfw2s_title_handle(void);
static void jfw2s_domains_handle(void);
static void jfw2s_vars_handle(int f_var_no, int var_c);
static void jfw2s_adj_write(int adj_no);
static void jfw2s_adjectives_handle(void);
static void jfw2s_hedges_handle(void);
static void jfw2s_operators_handle(void);
static void jfw2s_relations_handle(void);
static void jfw2s_arrays_handle(void);
static void jfw2s_synonyms_handle(void);
static int jfw_check(void *head);
static int jfw2s_close(void);
static int jfw_load(char *so_fname);
static void jfw2s_program_handle(void);


/*************************************************************************/
/* Hjaelpe-funktioner                                                    */
/*************************************************************************/

static int jf_error(int eno, const char *name, int mode)
{
  int m, v, e;

  e = 0;
  m = -1;
  if (jfw2s_errmode != JFE_FATAL)
  { for (v = 0; e == 0; v++)
    { if (jfr_err_texts[v].eno == eno
          || jfr_err_texts[v].eno == 999)
        e = v;
    }
    if (mode == JFE_WARNING)
    { fprintf(jfw2s_err, "WARNING %d: %s %s\n",
                       eno, jfr_err_texts[e].text, name);
      if (jfw2s_errmode == 0)
        jfw2s_errmode = JFE_WARNING;
      m = 0;
    }
    else
    { fprintf(jfw2s_err, "*** error %d: %s %s\n",
                       eno, jfr_err_texts[e].text, name);
      if (mode == JFE_FATAL)
      { fprintf(jfw2s_err, "\n*** PROGRAM ABORTED! ***\n");
        jfw2s_errmode = JFE_FATAL;
      }
      else
        jfw2s_errmode = JFE_ERROR;
      m = -1;
    }
    if (eno != 0)
      jfw2s_errno = eno;
  }
  return m;
}


static void jf_ftoa(char *txt, float f, int pct)
{
  char it[30] = "   ";
  char *t;
  int m, cif, mente, dp, ep, dl, sign, at, slut, b, mnul;

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
  ep = dp + jfw2s_digits - 1;
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
    { it[m] = '\0';
      slut = 1;
    }
    else
      slut = 1;
    m--;
  }

  at = 0;
  if (pct != 0)
  { txt[at] = '%';
    at++;
  }
  /* check om lig -0. Hvis ja ret til 0 (sign:=1) */
  mnul = 1;
  for (m = 0; it[m] != '\0'; m++)
  { if (it[m] != ' ' && it[m] != '0')
      mnul = 0;
  }
  if (mnul == 1)
     sign = 1;

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

/*************************************************************************/
/* Udskrivningsfunktoner                                                 */
/*************************************************************************/

static void jfw2s_write(char *text)
           /* Writes to buffer. If buffer is full                   */
           /* then write buffer to file, write newline and spaces   */
           /* to buffer.                                            */
{
  int len;
  int espace;

  espace = 1;
  len = strlen(jfw2s_text);
  if (len + strlen(text) + 1 > JFI_CHPRLINE)
  { fprintf(jfw2s_op, "%s\n", jfw2s_text);
    strcpy(jfw2s_text, jfw2s_spaces);
    strcat(jfw2s_text, jfw2s_spaces);
  }
  else
  { if (len == 0)
      strcat(jfw2s_text, jfw2s_spaces);
    else
    { if (strcmp(text, jfw2s_t_colon) == 0 || jfw2s_text[len - 1] == ':')
        espace = 0;
      if (text[0] == ',' || text[0] == ')' || jfw2s_text[len - 1] == '(')
        espace = 0;
      if (espace == 1)
        strcat(jfw2s_text, jfw2s_sspace);
    }
  }
  strcat(jfw2s_text, text);
}


static void jfw2s_qwrite(char *text)
     /* som jfw2s_write, men teksten    */
     /* skrives i gaaesoejne.         */
{
  char tmp[100];

  strcpy(tmp, jfw2s_t_quote);
  strcat(tmp, text);
  strcat(tmp, jfw2s_t_quote);
  jfw2s_write(tmp);
}

static void jfw2s_dwrite(float d, int pct)
{
  char txt[30];

  jf_ftoa(txt, d, pct);
  jfw2s_write(txt);
}

static void jfw2s_plf_wwrite(float d, int pct, int exclusiv)
{
  char txt[32];
  int s;

  jf_ftoa(txt, d, pct);
  if (exclusiv != 0)
  { s = strlen(txt);
    txt[s] = 'x';
    txt[s + 1] = '\0';
  }
  jfw2s_write(txt);
}

static void jfw2s_flush(void)           /* Udskriver bufferen efterfulgt af  */
                                      /* et semikoloen og et linieskift.   */
{
  fprintf(jfw2s_op, "%s%s\n", jfw2s_text, jfw2s_t_scolon);
  jfw2s_text[0] = '\0';
}


/*************************************************************************/
/* Blok-funktioner                                                       */
/*************************************************************************/

static void jfw2s_comment_write(int comment_no)
{
  int m, res;
  struct jfr_comment_desc *com;

  res = 0;
  if (comment_no >= 0)
  { com = &(jf_head.comments[comment_no]);
    for (m = 0; res == 0 && m < com->comment_c; m++)
    { if (m >= jfw2s_maxtext - 1)
        res = 301;
      else
        jfw2s_text[m] = jf_head.comment_block[com->f_comment_id + m];
    }
    if (res == 0)
    { jfw2s_text[m] = '\0';
      fprintf(jfw2s_op, "/*%s*/\n", jfw2s_text);
    }
    else
      jf_error(res, " in comment", JFE_WARNING);
  }
  jfw2s_text[0] = '\0';
}

static void jfw2s_title_handle(void)
{
  if (strlen(jf_head.title) != 0)
    fprintf(jfw2s_op, "\ntitle \"%s\";\n", jf_head.title);
  jfw2s_comment_write(jf_head.comment_no);
  /* jfw2s_flush(); */
}

static void jfw2s_domains_handle(void)
{
  int m, first, skriv;
  struct jfr_domain_desc *dom_info;

  first = 1;
  for (m = 0; m < jf_head.domain_c; m++)
  { dom_info = &(jf_head.domains[m]);
    skriv = 1;
    if (jfw2s_op_mode == JFI_OP_COMPACT)
    { if (m == JFS_DNO_FBOOL && dom_info->dmin == 0.0
          && dom_info->dmax == 1.0 && dom_info->adjectiv_c == 0
          && dom_info->type == JFS_DT_FLOAT && dom_info->flags == 3
          && dom_info->unit[0] == '\0')
        skriv = 0;
      if (m == JFS_DNO_FLOAT && dom_info->type == JFS_DT_FLOAT
          && dom_info->flags == 0)
        skriv = 0;
    }
    if (skriv == 1)
    { if (first == 1)
      { fprintf(jfw2s_op, "\ndomains\n");
        first = 0;
      }
      jfw2s_write(dom_info->name);
      if (strlen(dom_info->unit) != 0)
      { if (jfw2s_op_mode == JFI_OP_ALL)
          jfw2s_write(jfw2s_t_text);
        jfw2s_qwrite(dom_info->unit);
      }
      if (jfw2s_op_mode == JFI_OP_ALL || dom_info->type != JFS_DT_FLOAT)
      { if (jfw2s_op_mode == JFI_OP_ALL)
          jfw2s_write(jfw2s_t_type);
        jfw2s_write(jfs_t_dts[dom_info->type]);
      }
      if ((dom_info->flags & JFS_DF_MINENTER) != 0)
      { if (jfw2s_op_mode == JFI_OP_ALL)
          jfw2s_write(jfw2s_t_min);
        jfw2s_dwrite(dom_info->dmin, 0);
      }
      if ((dom_info->flags & JFS_DF_MAXENTER) != 0)
      {  if (jfw2s_op_mode == JFI_OP_ALL || dom_info->flags == JFS_DF_MAXENTER)
           jfw2s_write(jfw2s_t_max);
         jfw2s_dwrite(dom_info->dmax, 0);
      }
      jfw2s_flush();
      jfw2s_comment_write(dom_info->comment_no);
    }
  }
}

static void jfw2s_vars_handle(int f_var_no, int var_c)
{
  int m;
  struct jfr_var_desc *var_info;
  struct jfr_domain_desc *domain_info;
  struct jfr_operator_desc *op;
  struct jfr_adjectiv_desc *adesc;

  for (m = 0; m < var_c; m++)
  { var_info = &(jf_head.vars[f_var_no + m]);
    if (jfw2s_write_vnos == 1)
      fprintf(jfw2s_op, "  # variable no %d:\n", f_var_no + m);
    jfw2s_write(var_info->name);
    if (strlen(var_info->text) > 0)
    { if (jfw2s_op_mode == JFI_OP_ALL)
        jfw2s_write(jfw2s_t_text);
      jfw2s_qwrite(var_info->text);
    }
    domain_info = &(jf_head.domains[var_info->domain_no]);
    if (jfw2s_op_mode == JFI_OP_ALL)
      jfw2s_write(jfw2s_t_domain);
    jfw2s_write(domain_info->name);

    if (var_info->defuz_1 != JFS_VD_CENTROID
        || var_info->defuz_2 != var_info->defuz_1
        || jfw2s_op_mode == JFI_OP_ALL)
    { jfw2s_write(jfw2s_t_defuz);
      jfw2s_write(jfs_t_vds[var_info->defuz_1]);
      if (var_info->defuz_2 != var_info->defuz_1)
      { jfw2s_write(jfs_t_vds[var_info->defuz_2]);
        jfw2s_dwrite(var_info->defuz_arg, var_info->flags & JFS_VF_IDEFUZ);
      }
    }
    op = &(jf_head.operators[var_info->f_comp]);
    if (strcmp(op->name, jfw2s_t_or) != 0 || jfw2s_op_mode == JFI_OP_ALL)
    { jfw2s_write(jfw2s_t_f_comp);
      jfw2s_write(op->name);
    }
    if (var_info->d_comp != JFS_VCF_NEW || jfw2s_op_mode == JFI_OP_ALL)
    { jfw2s_write(jfw2s_t_d_comp);
      jfw2s_write(jfs_t_vcfs[var_info->d_comp]);
    }
    if ((var_info->flags & JFS_VF_NORMAL))
    { jfw2s_write(jfw2s_t_normal);
      if (var_info->no_arg <= 0.0)
        jfw2s_write(jfw2s_t_conf);
      else
        jfw2s_dwrite(var_info->no_arg, var_info->flags & JFS_VF_INORMAL);
    }
    if (var_info->acut != 0.0)
    { jfw2s_write(jfw2s_t_acut);
      jfw2s_dwrite(var_info->acut, var_info->flags & JFS_VF_IACUT);
    }
    if (var_info->default_type != 0)
    { jfw2s_write(jfw2s_t_default);
      if (var_info->default_type == 1)
        jfw2s_dwrite(var_info->default_val, 0);
      else
      { if (var_info->fzvar_c == 0)
          adesc = &(jf_head.adjectives[domain_info->f_adjectiv_no
                                        + var_info->default_no]);
        else
          adesc = &(jf_head.adjectives[var_info->f_adjectiv_no
                                        + var_info->default_no]);
        jfw2s_write(adesc->name);
      }
    }
    if (var_info->argument != 0)
    { jfw2s_write(jfw2s_t_argument);
      jfw2s_dwrite(var_info->argument, 0);
    }
    jfw2s_flush();
    jfw2s_comment_write(var_info->comment_no);
  }
}

static void jfw2s_adj_write(int adj_no)
{
  int lim;
  struct jfr_adjectiv_desc *adjectiv;
  struct jfr_hedge_desc *hedge;
  struct jfr_limit_desc *limdesc;

  adjectiv = &(jf_head.adjectives[adj_no]);
  jfw2s_write(adjectiv->name);
  if (adjectiv->limit_c != 0)
  { if (jfw2s_op_mode == JFI_OP_ALL)
      jfw2s_write(jfw2s_t_plf);
    for (lim = 0; lim < adjectiv->limit_c; lim++)
    { limdesc = &(jf_head.limits[adjectiv->f_limit_no + lim]);
      jfw2s_plf_wwrite(limdesc->limit,
                     limdesc->flags & JFS_LF_IX,
                     limdesc->exclusiv);
      jfw2s_write(jfw2s_t_colon);
      jfw2s_dwrite(limdesc->b,
                 limdesc->flags & JFS_LF_IY);
    }
  }

  if (adjectiv->flags & JFS_AF_CENTER)
  { if (jfw2s_op_mode == JFI_OP_ALL || adjectiv->limit_c > 0)
      jfw2s_write(jfw2s_t_center);
    jfw2s_dwrite(adjectiv->center, adjectiv->flags & JFS_AF_ICENTER);
  }
  if ((adjectiv->flags & JFS_AF_BASE) != 0)
  { jfw2s_write(jfw2s_t_base);
    jfw2s_dwrite(adjectiv->base, adjectiv->flags & JFS_AF_IBASE);
  }
  if (adjectiv->flags & JFS_AF_TRAPEZ)
  { jfw2s_write(jfw2s_t_trapez);
    jfw2s_dwrite(adjectiv->trapez_start, adjectiv->flags & JFS_AF_ISTRAPEZ);
    jfw2s_dwrite(adjectiv->trapez_end, adjectiv->flags & JFS_AF_IETRAPEZ);
  }
  if ((adjectiv->flags & JFS_AF_HEDGE) != 0)
  { jfw2s_write(jfw2s_t_hedge);
    hedge = &(jf_head.hedges[adjectiv->h1_no]);
    jfw2s_write(hedge->name);
    if (adjectiv->h2_no != adjectiv->h1_no)
    { hedge = &(jf_head.hedges[adjectiv->h2_no]);
      jfw2s_write(hedge->name);
    }
  }
  jfw2s_flush();

/*  if (jfw2s_write_center == 1)
  { if ((adjectiv.flags & JFG_AF_CENTER) == 0
      || (adjectiv.flags & JFG_AF_BASE) == 0)
    { fprintf(jfw2s_op, "      #");
      if ((adjectiv.flags & JFG_AF_CENTER) == 0)
      { jf_ftoa(txt, adjectiv.center, 0);
     fprintf(jfw2s_op, " calculated center: %s", txt);
      }
      if ((adjectiv.flags & JFG_AF_BASE) == 0)
      { jf_ftoa(txt, adjectiv.base, 0);
     fprintf(jfw2s_op, " calculated base: %s", txt);
      }
      fprintf(jfw2s_op, "\n");
    }
  }
*/
  jfw2s_comment_write(adjectiv->comment_no);
}


static void jfw2s_adjectives_handle(void)
{
  int m, a, first;
  struct jfr_domain_desc *domain;
  struct jfr_var_desc *var;

  first = 1;
  if (jf_head.adjectiv_c > 0)
  { fprintf(jfw2s_op, "\nadjectives\n");
    for (m = 0; m < jf_head.domain_c; m++)
    { domain = &(jf_head.domains[m]);
      for (a = 0; a < domain->adjectiv_c; a++)
      { if (a == 0 && first == 0)
          fprintf(jfw2s_op, "\n");
        first = 0;
        jfw2s_write(domain->name);
        jfw2s_adj_write(domain->f_adjectiv_no + a);
      }
    }
    for (m = 0; m < jf_head.var_c; m++)
    { var = &(jf_head.vars[m]);
      domain = &(jf_head.domains[var->domain_no]);
      if (domain->adjectiv_c == 0
          || var->f_adjectiv_no != domain->f_adjectiv_no)
      { for (a = 0; a < var->fzvar_c; a++)
        { if (a == 0 && first == 0)
            fprintf(jfw2s_op, "\n");
          first = 0;
          if (jfw2s_write_vnos == 1)
            fprintf(jfw2s_op, "  # fuzzy variable no %d:\n",
                            var->f_fzvar_no + a);
          jfw2s_write(var->name);
          jfw2s_adj_write(var->f_adjectiv_no + a);
        }
        /* { if (jfw2s_write_vnos == 1)
             { for (a = 0; a < var->fzvar_c; a++)
               { jfg_adjectiv(&adjective, jfr_head, var.f_adjectiv_no + a);
                 fprintf(jfw2s_op, "  # fuzzy variable no %d: %s %s\n",
                      var.f_fzvar_no + a, var.name, adjective.name);
               }
          }
           }
        */
      }
    }
  }
}

static void jfw2s_hedges_handle(void)
{
  int m, lim, first, skriv;
  struct jfr_hedge_desc *hedge;
  struct jfr_limit_desc *lim_desc;

  first = 1;
  if (jf_head.hedge_c > 0)
  { for (m = 0; m < jf_head.hedge_c; m++)
    { hedge = &(jf_head.hedges[m]);
      skriv = 1;
      if (m == JFS_HNO_NOT && hedge->type == JFS_HT_NEGATE
          && jfw2s_op_mode == JFI_OP_COMPACT)
        skriv = 0;
      if (skriv == 1)
      { if (first == 1)
        { fprintf(jfw2s_op, "\nhedges\n");
          first = 0;
        }
        jfw2s_write(hedge->name);
        if (hedge->type == JFS_HT_LIMITS)
        { if (jfw2s_op_mode == JFI_OP_ALL)
            jfw2s_write(jfw2s_t_plf);
          for (lim = 0; lim < hedge->limit_c; lim++)
          { lim_desc = &(jf_head.limits[lim + hedge->f_limit_no]);
            jfw2s_plf_wwrite(lim_desc->limit,
                           lim_desc->flags & JFS_LF_IX,
                           lim_desc->exclusiv);
            jfw2s_write(jfw2s_t_colon);
            jfw2s_dwrite(lim_desc->b,
                       lim_desc->flags & JFS_LF_IY);
          }
        }
        else
        { jfw2s_write(jfs_t_hts[hedge->type]);
          if (hedge->type != JFS_HT_NEGATE)
            jfw2s_dwrite(hedge->hedge_arg, hedge->flags & JFS_HF_IARG);
        }
        jfw2s_flush();
        jfw2s_comment_write(hedge->comment_no);
      }
    }
  }
}

static void jfw2s_operators_handle(void)
{
  int m, first, skriv;
  struct jfr_operator_desc *op;
  struct jfr_hedge_desc *hedge;
  char t[10];


  first = 1;
  if (jf_head.operator_c > 0)
  { for (m = 0; m < jf_head.operator_c; m++)
    { op = &(jf_head.operators[m]);
      skriv = 1;
      if (jfw2s_op_mode == JFI_OP_COMPACT)
      { if (m == JFS_ONO_CASEOP && op->op_1 == JFS_FOP_PROD
            && op->hedge_mode == JFS_OHM_NONE
            && op->op_2 == JFS_FOP_PROD && op->flags == 0)
          skriv = 0;
        if (m == JFS_ONO_WEIGHTOP && op->op_1 == JFS_FOP_PROD
            && op->hedge_mode == JFS_OHM_NONE
            && op->op_2 == JFS_FOP_PROD && op->flags == 0)
          skriv = 0;
        if (m == JFS_ONO_AND && op->op_1 == JFS_FOP_MIN
            && op->hedge_mode == JFS_OHM_NONE
            && op->op_2 == JFS_FOP_MIN && op->flags == 4
            && op->precedence == 30)
          skriv = 0;
        if (m == JFS_ONO_OR && op->op_1 == JFS_FOP_MAX
            && op->hedge_mode == JFS_OHM_NONE
            && op->op_2 == JFS_FOP_MAX && op->flags == 4
            && op->precedence == 20)
          skriv = 0;
        if (m == JFS_ONO_WHILEOP && op->op_1 == JFS_FOP_PROD
            && op->hedge_mode == JFS_OHM_NONE
            && op->op_2 == JFS_FOP_PROD && op->flags == 0)
          skriv = 0;
      }
      if (skriv == 1)
      { if (first == 1)
        { fprintf(jfw2s_op, "\noperators\n");
          first = 0;
        }
        jfw2s_write(op->name);
        if (jfw2s_op_mode == JFI_OP_ALL)
          jfw2s_write(jfw2s_t_type);
        jfw2s_write(jfs_t_fops[op->op_1]);
        if (op->op_2 != op->op_1)
          jfw2s_write(jfs_t_fops[op->op_2]);
        if (op->op_2 != op->op_1 ||
            (op->op_1 >= JFS_FOP_HAMAND
             && op->op_1 <= JFS_FOP_YAGEROR))
          jfw2s_dwrite(op->op_arg, op->flags & JFS_OF_IARG);

        if (op->hedge_mode != JFS_OHM_NONE)
        { if (op->hedge_mode != JFS_OHM_POST)
          { jfw2s_write(jfw2s_t_hedgemode);
            jfw2s_write(jfs_t_oph_modes[op->hedge_mode]);
          }
          jfw2s_write(jfw2s_t_hedge);
          hedge = &(jf_head.hedges[op->hedge_no]);
          jfw2s_write(hedge->name);
        }
        if (op->precedence != 19)
        { jfw2s_write(jfw2s_t_precedence);
          sprintf(t, "%d", (int) op->precedence);
          jfw2s_write(t);
        }
        jfw2s_flush();
        jfw2s_comment_write(op->comment_no);
      }
    }
  }
}

static void jfw2s_relations_handle(void)
{
  int m, lim;
  struct jfr_relation_desc *relation;
  struct jfr_hedge_desc *hedge;
  struct jfr_limit_desc *limdesc;

  if (jf_head.relation_c > 0)
  { fprintf(jfw2s_op, "\nrelations\n");
    for (m = 0; m < jf_head.relation_c; m++)
    { relation = &(jf_head.relations[m]);
      jfw2s_write(relation->name);
      if (jfw2s_op_mode == JFI_OP_ALL)
        jfw2s_write(jfw2s_t_plf);
      for (lim = 0; lim < relation->limit_c; lim++)
      { limdesc = &(jf_head.limits[relation->f_limit_no + lim]);
        jfw2s_plf_wwrite(limdesc->limit,
                       limdesc->flags & JFS_LF_IX,
                       limdesc->exclusiv);
     jfw2s_write(jfw2s_t_colon);
     jfw2s_dwrite(limdesc->b,
             limdesc->flags & JFS_LF_IY);
      }
      if ((relation->flags & JFS_RF_HEDGE) != 0)
      { jfw2s_write(jfw2s_t_hedge);
        hedge = &(jf_head.hedges[relation->hedge_no]);
        jfw2s_write(hedge->name);
      }
      jfw2s_flush();
      jfw2s_comment_write(relation->comment_no);
    }
  }
}

static void jfw2s_arrays_handle(void)
{
  int m;
  struct jfr_array_desc *adesc;
  char t[30];

  if (jf_head.array_c > 0)
  { fprintf(jfw2s_op, " \narrays\n");
    for (m = 0; m < jf_head.array_c; m++)
    { adesc = &(jf_head.arrays[m]);
      jfw2s_write(adesc->name);
      jfw2s_write(jfw2s_t_size);
      sprintf(t, "%d", (int) adesc->array_c);
      jfw2s_write(t);
      jfw2s_flush();
      jfw2s_comment_write(adesc->comment_no);
    }
  }
}

static void jfw2s_synonyms_handle(void)
{
  int m;

  if (jfw_head.synonym_c > 0)
  { fprintf(jfw2s_op, "\nsynonyms\n");
    for (m = 0; m < jfw_head.synonym_c; m++)
    { jfw2s_write(jfw2s_synonyms[m].name);
      if (strlen(jfw2s_synonyms[m].value) > 0)
        jfw2s_write(jfw2s_synonyms[m].value);
      jfw2s_flush();
    }
  }
}

static int jfw_check(void *head)
{
  struct jfw_head_desc *jfw_head;

  if (head == NULL)
    return 4;
  jfw_head = (struct jfw_head_desc *) head;
  if (jfw_head->check[0] != 'j' ||
      jfw_head->check[1] != 'f' ||
      jfw_head->check[2] != 'w')
    return 4;
  if (jfw_head->version / 100 != 2)
    return 5;
  return 0;
}

static int jfw2s_close(void)
{
  if (jfw2s_text != NULL)
    free(jfw2s_text);
  fclose(jfw2s_op);
  fclose(jfw2s_ip);
  if (jfw2s_err != stdout)
    fclose(jfw2s_err);
  return JFE_FATAL;
}

static int jfw_load(char *so_fname)
{
  int size, ar, res;

  jfw2s_ip = fopen(so_fname, "rb");
  if (jfw2s_ip == NULL)
    return jf_error(1, so_fname, JFE_FATAL);

  ar = fread(&jfw_head, 1, JFW_HEAD_SIZE, jfw2s_ip);
  if (ar < 0)
    return jf_error(2, jfw2s_spaces, JFE_FATAL);
  if (ar == JFW_HEAD_SIZE)
  { res = jfw_check(&jfw_head);
    if (res != 0)
      return res;
  }
  else
    return jf_error(4, jfw2s_spaces, JFE_FATAL);

  strcpy(jf_head.title, jfw_head.title);
  jf_head.comment_no  = jfw_head.comment_no;
  jf_head.domain_c    = jfw_head.domain_c;
  jf_head.adjectiv_c  = jfw_head.adjectiv_c;
  jf_head.ivar_c      = jfw_head.ivar_c;
  jf_head.ovar_c      = jfw_head.ovar_c;
  jf_head.lvar_c      = jfw_head.lvar_c;
  jf_head.var_c       = jfw_head.var_c;
  jf_head.array_c     = jfw_head.array_c;
  jf_head.limit_c     = jfw_head.limit_c;
  jf_head.relation_c  = jfw_head.relation_c;
  jf_head.hedge_c     = jfw_head.hedge_c;
  jf_head.operator_c  = jfw_head.operator_c;
  jf_head.comment_c   = jfw_head.comment_c;
  jf_head.f_prog_comment = jfw_head.comment_c;
  jf_head.com_block_c = jfw_head.com_block_c;
  jf_head.arg_1_c     = jfw_head.arg_1_c;
  jf_head.arg_2_c     = jfw_head.arg_2_c;
  jf_head.function_c  = 0;
  jf_head.funcarg_c   = 0;
  jf_head.funccode_c  = 0;
  jf_head.program_c   = 0;
  jf_head.f_lvar_no   = 0;
  jf_head.f_ivar_no   = jfw_head.lvar_c;
  jf_head.f_ovar_no   = jf_head.f_ivar_no + jfw_head.ivar_c;

  size =   jfw_head.synonym_c    * JFW_SYNONYM_SIZE
  + jfw_head.domain_c     * JFR_DOMAIN_SIZE
  + jfw_head.adjectiv_c   * JFR_ADJECTIV_SIZE
  + jfw_head.var_c        * JFR_VAR_SIZE
  + jfw_head.array_c      * JFR_ARRAY_SIZE
  + jfw_head.limit_c      * JFR_LIMIT_SIZE
  + jfw_head.hedge_c      * JFR_HEDGE_SIZE
  + jfw_head.relation_c   * JFR_RELATION_SIZE
  + jfw_head.operator_c   * JFR_OPERATOR_SIZE
  + jfw_head.comment_c    * JFR_COMMENT_SIZE
  + jfw_head.com_block_c;

  jfc_ff_jfr = (char *) malloc(size);
  if (jfc_ff_jfr == NULL)
    return jf_error(6, jfw2s_spaces, JFE_FATAL);

  ar = fread(jfc_ff_jfr, 1, size, jfw2s_ip);
  if (ar != size)
    return jf_error(2, jfw2s_spaces, JFE_FATAL);
  jfw2s_synonyms = (struct jfw_synonym_desc *) jfc_ff_jfr;

  jfc_ff_jfr += jfw_head.synonym_c * JFW_SYNONYM_SIZE;

  jf_head.domains = (struct jfr_domain_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfw_head.domain_c * JFR_DOMAIN_SIZE;
  jf_head.adjectives = (struct jfr_adjectiv_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfw_head.adjectiv_c * JFR_ADJECTIV_SIZE;
  jf_head.vars = (struct jfr_var_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfw_head.var_c * JFR_VAR_SIZE;
  jf_head.arrays = (struct jfr_array_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfw_head.array_c * JFR_ARRAY_SIZE;
  jf_head.limits = (struct jfr_limit_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfw_head.limit_c * JFR_LIMIT_SIZE;
  jf_head.hedges = (struct jfr_hedge_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfw_head.hedge_c * JFR_HEDGE_SIZE;
  jf_head.relations = (struct jfr_relation_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfw_head.relation_c * JFR_RELATION_SIZE;
  jf_head.operators = (struct jfr_operator_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfw_head.operator_c * JFR_OPERATOR_SIZE;
  jf_head.comments = (struct jfr_comment_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfw_head.comment_c * JFR_COMMENT_SIZE;
  jf_head.comment_block = (unsigned char *) jfc_ff_jfr;
  jfc_ff_jfr += jfw_head.com_block_c;

  return 0;
}

static void jfw2s_program_handle(void)
{
  char c;

  c = 0;
  while ((c = fgetc(jfw2s_ip)) != EOF)
  { if (c == 10)
      putc('\n', jfw2s_op);                /* NB: Virker kun i DOS !! */
    else
    if (c != 13)
      putc(c, jfw2s_op);
  }
}

int jfw2s_conv(char *de_fname, char *so_fname, char *err_fname,
               int ndigits, int maxtext, int out_mode,
               int var_nos, int op_mode)
{
  int m;

  jfw2s_errmode = JFE_NONE;
  jfw2s_op = jfw2s_ip = NULL;
  jfw2s_text = NULL;
  jfw2s_err = stdout;
  if (!(strlen(err_fname) == 0 || err_fname == NULL))
  { if (out_mode == 0)
      jfw2s_err = fopen(err_fname, "w");
    else
      jfw2s_err = fopen(err_fname, "a");
    if (jfw2s_err == NULL)
    { jfw2s_err = stdout;
      jf_error(1, err_fname, JFE_ERROR);
    }
  }

  if ((m = jfw_load(so_fname)) != 0)
    return jfw2s_close();

  if ((jfw2s_op = fopen(de_fname, "w")) == NULL)
  { jf_error(1, de_fname, JFE_FATAL);
    return jfw2s_close();
  }

  if (maxtext != 0)
    jfw2s_maxtext = maxtext;
  else
    jfw2s_maxtext = 512;
  jfw2s_text = (char *) malloc(jfw2s_maxtext);
  if (jfw2s_text == NULL)
  { jf_error(6, jfw2s_spaces, JFE_FATAL);
    return jfw2s_close();
  }

  jfw2s_text[0] = '\0';

  if (ndigits != 0)
    jfw2s_digits = ndigits;
  else
    jfw2s_digits = 5;
  jfw2s_write_vnos     = var_nos;
  jfw2s_op_mode        = op_mode;
  jfw2s_errno = 0;

  if (m != 0)
  { jf_error(m, jfw2s_t_stack, JFE_FATAL);
    return jfw2s_close();
  }

  jfw2s_title_handle();
  jfw2s_synonyms_handle();
  jfw2s_hedges_handle();
  jfw2s_operators_handle();
  jfw2s_relations_handle();
  jfw2s_domains_handle();
  if (jf_head.ivar_c > 0)
  { fprintf(jfw2s_op, "\ninput\n");
    jfw2s_vars_handle(jf_head.f_ivar_no, jf_head.ivar_c);
  }
  if (jf_head.ovar_c > 0)
  { fprintf(jfw2s_op, "\noutput\n");
    jfw2s_vars_handle(jf_head.f_ovar_no, jf_head.ovar_c);
  }

  if (jf_head.lvar_c > 0)
  { fprintf(jfw2s_op, "\nlocal\n");
    jfw2s_vars_handle(jf_head.f_lvar_no, jf_head.lvar_c);
  }
  jfw2s_adjectives_handle();
  jfw2s_arrays_handle();

  fprintf(jfw2s_op, "\nprogram\n");
  jfw2s_program_handle();

  jfw2s_close();
  return jfw2s_errmode;
}




