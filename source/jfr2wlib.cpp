  /**************************************************************************/
  /*                                                                        */
  /* jfr2wlib.cpp   Version  2.00    Copyright (c) 1999 by Jan E. Mortensen */
  /*                                                                        */
  /* JFS Invers Compiler-functions. Converts a JFR-file to a JFW-file.      */
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
#include <math.h>
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfr_gen.h"
#include "jfs_text.h"
#include "jfs_cons.h"
#include "jfr2wlib.h"

#define JFI_CHPRLINE 78

static struct jfg_sprog_desc jfr2w_spdesc;
static void *jfr2w_head;
static struct jfr_head_desc *jfr_head;
static struct jfw_head_desc jfw_head;

static unsigned char *jfr2w_pc;

static int jfr2w_rule_no = 1;    /* nr aktuel rule */
static int jfr2w_digits = 5;
static int jfr2w_mode   = 0;  /* 0: Standard (infix, No extra parentesses), */
                            /* 2: prefix.                                 */
static int jfr2w_write_rules = 0; /* write rule numbers.                 */

static int jfr2w_sw_comments = 0; /* 1: insert '|' in switch-statements. */
static char *jfr2w_text = NULL;
static int   jfr2w_maxtext = 1024;

static int jfr2w_errno = 0;

static char jfr2w_spaces[] = "    ";
static char jfr2w_sspace[] = " ";

/* static struct jfg_limit_desc jfr2w_limits[256]; */

static struct jfg_tree_desc *jfr2w_tree;
static int    jfr2w_maxtree = 128;

static int    jfr2w_stacksize = 64;

static FILE *jfr2w_op = NULL;
static FILE *jfr2w_stdout = NULL;

/* static char jfr2w_t_quote[]    = "\""; */
static char jfr2w_t_scolon[]   = ";";
static char jfr2w_t_colon[]    = ":";
static char jfr2w_t_stack[]    = "jfg-stack";
static char jfr2w_t_bpar[]     = "(";
static char jfr2w_t_epar[]     = ")";
static char jfr2w_t_function[] = "function";
static char jfr2w_t_procedure[]= "procedure";


#define JFE_NONE    0
#define JFE_WARNING 1
#define JFE_ERROR   2
#define JFE_FATAL   3

static int jfr2w_errmode;

struct jfr_err_desc { int eno;
                      const char *text;
                    };

static struct jfr_err_desc jfr_err_texts[] =
 {
   {  0, " "},
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
   {301, "statement to long."},
   {302, "jfg-tree to small to hold statement."},
   {303, "Stack-overflow (jfg-stack)."},
   {304, "program-id (pc) is not the start of a statement."},
   {9999, "unknown error."}
 };

static int jf_error(int eno, char *name, int mode);
static void jf_ftoa(char *txt, float f, int pct);
static void jfr2w_write(char *text);
static void jfr2w_dwrite(float d, int pct);
static void jfr2w_flush(void);
static void jfr2w_comment_write(int comment_no);
static void jfr2w_block_handle(unsigned char *pc, int func_no);
static void jfr2w_function_handle(int fno);
static void jfr2w_program_handle(void);
static int jfr2w_copy_head(void);
static int jfr2w_close(void);
static void jfr2w_limits_handle(int f_limit_no, int limit_c);
static void jfr2w_plf_handle(void);

/*************************************************************************/
/* Hjaelpe-funktioner                                                    */
/*************************************************************************/

static int jf_error(int eno, char *name, int mode)
{
  int m, v, e;

  m = -1;
  if (jfr2w_errmode != JFE_FATAL)
  { e = 0;
    for (v = 0; e == 0; v++)
    { if (jfr_err_texts[v].eno == eno
       || jfr_err_texts[v].eno == 9999)
        e = v;
    }
    if (mode == JFE_WARNING)
    { fprintf(jfr2w_stdout, "WARNING %d: %s %s\n", eno, jfr_err_texts[e].text, name);
      m = 0;
      if (jfr2w_errmode == 0)
        jfr2w_errmode = 1;
    }
    else
    { if (eno != 0)
        fprintf(jfr2w_stdout, "*** error %d: %s %s\n", eno, jfr_err_texts[e].text, name);
      if (mode == JFE_FATAL)
      { jfr2w_errmode = JFE_FATAL;
        fprintf(jfr2w_stdout, "\n*** PROGRAM ABORTED! ***\n");
      }
      else
      { if (jfr2w_errmode < JFE_FATAL)
          jfr2w_errmode = JFE_ERROR;
      }
      m = -1;
    }
  }
  if (eno != 0)
    jfr2w_errno = eno;
  return m;
}

static void jf_ftoa(char *txt, float f, int pct)
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
  ep = dp + jfr2w_digits - 1;
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

static void jfr2w_write(char *text)
           /* Skriver til buffer. Hvis buffer */
           /* er fuld skrives den ud, der     */
           /* skiftes linie og der skrives    */
           /* blanktegn. */
{
  int len;
  int espace;

  espace = 1;
  len = strlen(jfr2w_text);
  if (len + strlen(text) + 1 > JFI_CHPRLINE)
  { fprintf(jfr2w_op, "%s\n", jfr2w_text);
    strcpy(jfr2w_text, jfr2w_spaces);
    strcat(jfr2w_text, jfr2w_spaces);
  }
  else
  { if (len == 0)
      strcat(jfr2w_text, jfr2w_spaces);
    else
    { if (strcmp(text, jfr2w_t_colon) == 0 || jfr2w_text[len - 1] == ':')
        espace = 0;
      if (text[0] == ',' || text[0] == ')' || jfr2w_text[len - 1] == '(')
        espace = 0;
      if (espace == 1)
        strcat(jfr2w_text, jfr2w_sspace);
    }
  }
  strcat(jfr2w_text, text);
}

static void jfr2w_dwrite(float d, int pct)
{
  char txt[30];

  jf_ftoa(txt, d, pct);
  jfr2w_write(txt);
}


static void jfr2w_flush(void)           /* Udskriver bufferen efterfulgt af  */
                                      /* et semikoloen og et linieskift.   */
{
  fprintf(jfr2w_op, "%s%s\n", jfr2w_text, jfr2w_t_scolon);
  jfr2w_text[0] = '\0';
}


/*************************************************************************/
/* Blok-funktioner                                                       */
/*************************************************************************/

static void jfr2w_comment_write(int comment_no)
{
  if (comment_no >= 0)
  { jfg_comment(jfr2w_text, jfr2w_maxtext, jfr2w_head, comment_no);
    fprintf(jfr2w_op, "/*%s*/\n", jfr2w_text);
    jfr2w_text[0] = '\0';
  }
}


static void jfr2w_block_handle(unsigned char *pc, int func_no)
{
  struct jfg_statement_desc stat_info;
  struct jfg_function_desc fdesc;
  int res, p, pos, slut;

  if (func_no >= 0)
    jfg_function(&fdesc, jfr2w_head, func_no);
  jfr2w_pc = pc;
  if (func_no >= 0)
    pos = 6;
  else
    pos = 4;
  jfr2w_rule_no = 1;
  slut = 0;
  jfg_statement(&stat_info, jfr2w_head, jfr2w_pc);
  while (stat_info.type != JFG_ST_EOP && stat_info.n_pc != jfr2w_pc
         && slut == 0)
  { if (stat_info.type == JFG_ST_STEND || stat_info.type == JFG_ST_CASE
        || stat_info.type == JFG_ST_DEFAULT)
      pos -= 2;
    if (pos < 4)
      pos = 4;


    res = jfg_t_statement(jfr2w_text, jfr2w_maxtext, pos,
                          jfr2w_tree, jfr2w_maxtree,
                          jfr2w_head, func_no, jfr2w_pc);
    if (res != 0)
      jf_error(res, jfr2w_text, JFE_ERROR);
    else
    { if (jfr2w_write_rules == 1)
      { if (func_no >= 0)
          fprintf(jfr2w_op, "    # rule %s:%d:\n", fdesc.name, jfr2w_rule_no);
        else
          fprintf(jfr2w_op, "  # rule main:%d:\n", jfr2w_rule_no);
        jfr2w_rule_no++;
      }
      if (jfr2w_sw_comments == 1)
      { for (p = 4; p < pos; p += 2)
          jfr2w_text[p] = '|';
      }
      fprintf(jfr2w_op, "%s\n", jfr2w_text);
      jfr2w_comment_write(stat_info.comment_no);
    }
    if (stat_info.type == JFG_ST_CASE || stat_info.type == JFG_ST_SWITCH
        || stat_info.type == JFG_ST_WHILE
        || stat_info.type == JFG_ST_DEFAULT)
      pos += 2;
    if (pos > 50)
      pos = 50;
    if (stat_info.type == JFG_ST_STEND && stat_info.sec_type == 2)
      slut = 1;
    else
    { jfr2w_pc = stat_info.n_pc;
      jfg_statement(&stat_info, jfr2w_head, jfr2w_pc);
    }
  }
}

static void jfr2w_function_handle(int fno)
{
  int m, first;
  struct jfg_function_desc fdesc;
  struct jfg_func_arg_desc farg;
  char t[80];

  jfr2w_text[0] = '\0';
  jfg_function(&fdesc, jfr2w_head, fno);
  if (fdesc.type == JFS_FT_PROCEDURE)
    jfr2w_write(jfr2w_t_procedure);
  else
    jfr2w_write(jfr2w_t_function);

  strcpy(t, fdesc.name);
  strcat(t, jfr2w_t_bpar);
  jfr2w_write(t);

  first = 1;
  for (m = 0; m < fdesc.arg_c; m++)
  { jfg_func_arg(&farg, jfr2w_head, fno, m);
    if (first == 0)
    { t[0] = ','; t[1] = ' '; t[2] = '\0';
    }
    else
      t[0] = '\0';
    first = 0;
    strcat(t, farg.name);
    jfr2w_write(t);
  }
  jfr2w_write(jfr2w_t_epar);
  jfr2w_flush();
  jfr2w_comment_write(fdesc.comment_no);
  jfr2w_block_handle(fdesc.pc, fno);
  fprintf(jfr2w_op, "\n");
}

static void jfr2w_program_handle(void)
{
  int m;

  for (m = 0; m < jfr2w_spdesc.function_c; m++)
    jfr2w_function_handle(m);
  jfr2w_block_handle(jfr2w_spdesc.pc_start, -1);
}


static int jfr2w_copy_head(void)
{
  char ctekst[4] = "jfw";
  int size;

  strcpy(jfw_head.check, ctekst);
  jfw_head.version = 200;

  strcpy(jfw_head.title, jfr_head->title);
  jfw_head.comment_no = jfr_head->comment_no;
  jfw_head.f_prog_line_no = 0;
  jfw_head.synonym_c  = 0;
  jfw_head.domain_c   = jfr_head->domain_c;
  jfw_head.adjectiv_c = jfr_head->adjectiv_c;
  jfw_head.ivar_c     = jfr_head->ivar_c;
  jfw_head.ovar_c     = jfr_head->ovar_c;
  jfw_head.lvar_c     = jfr_head->lvar_c;
  jfw_head.var_c      = jfr_head->var_c;
  jfw_head.array_c    = jfr_head->array_c;
  jfw_head.limit_c    = jfr_head->limit_c;
  jfw_head.hedge_c    = jfr_head->hedge_c;
  jfw_head.relation_c = jfr_head->relation_c;
  jfw_head.operator_c = jfr_head->operator_c;
  jfw_head.arg_1_c    = jfr_head->arg_1_c = 0;
  jfw_head.arg_2_c    = jfr_head->arg_2_c = 0;

  jfw_head.comment_c  = jfr_head->f_prog_comment;
  if (jfr_head->f_prog_comment > 0)
    jfw_head.com_block_c
      = jfr_head->comments[jfr_head->f_prog_comment - 1].f_comment_id
       + jfr_head->comments[jfr_head->f_prog_comment - 1].comment_c;

  else
    jfw_head.com_block_c = 0;

  size = JFR_COMMENT_SIZE * jfw_head.comment_c
   + JFR_DOMAIN_SIZE * jfw_head.domain_c
   + JFR_ADJECTIV_SIZE * jfw_head.adjectiv_c
   + JFR_VAR_SIZE * jfw_head.var_c
   + JFR_ARRAY_SIZE * jfw_head.array_c
   + JFR_LIMIT_SIZE * jfw_head.limit_c
   + JFR_HEDGE_SIZE * jfw_head.hedge_c
   + JFR_RELATION_SIZE * jfw_head.relation_c
   + JFR_OPERATOR_SIZE * jfw_head.operator_c;


  if (fwrite((char *) &jfw_head, JFW_HEAD_SIZE, 1, jfr2w_op) != 1)
    return jf_error(3, jfr2w_spaces, JFE_FATAL);

  if (fwrite((char *) jfr_head->domains,
             size, 1, jfr2w_op) != 1)
    return jf_error(3, jfr2w_spaces, JFE_FATAL);

  if (fwrite((char *) jfr_head->comment_block,
             jfw_head.com_block_c, 1, jfr2w_op) != 1)
    return jf_error(3, jfr2w_spaces, JFE_FATAL);
  return 0;
}

static int jfr2w_close(void)
{
  if (jfr2w_text != NULL)
    free(jfr2w_text);
  if (jfr2w_tree != NULL)
    free(jfr2w_tree);
  if (jfr2w_head != NULL)
    jfr_close(jfr2w_head);
  if (jfr2w_op != NULL)
    fclose(jfr2w_op);
  jfg_free();
  jfr_free();
  return JFE_FATAL;
}

static void jfr2w_limits_handle(int f_limit_no, int limit_c)
{
  int m;
  struct jfr_limit_desc *lim;

  for (m = 0; m < limit_c; m++)
  { lim = &(jfr_head->limits[f_limit_no + m]);
    lim->b = lim->a * lim->limit + lim->b;
  }
}

static void jfr2w_plf_handle(void)
{
  int m;
  struct jfr_hedge_desc *hedge;
  struct jfr_relation_desc *rel;
  struct jfr_adjectiv_desc *adj;

  for (m = 0; m < jfr_head->hedge_c; m++)
  { hedge = &(jfr_head->hedges[m]);
    if (hedge->type == JFS_HT_LIMITS)
      jfr2w_limits_handle(hedge->f_limit_no, hedge->limit_c);
  }
  for (m = 0; m < jfr_head->relation_c; m++)
  { rel = &(jfr_head->relations[m]);
    jfr2w_limits_handle(rel->f_limit_no, rel->limit_c);
  }
  for (m = 0; m < jfr_head->adjectiv_c; m++)
  { adj = &(jfr_head->adjectives[m]);
    if (adj->limit_c > 0)
      jfr2w_limits_handle(adj->f_limit_no, adj->limit_c);
  }
}

int jfr2w_conv(char *de_fname, char *so_fname, char *sout_fname,
               int out_mode,  int ndigits,
               int maxtext, int maxtree, int maxstack,
               int rule_nos, int smode, int sw_comments)
{
  int m;

  jfr2w_stdout = stdout;
  if (sout_fname != NULL && sout_fname[0] != '\0')
  { if (out_mode == 0)
      jfr2w_stdout = fopen(sout_fname, "w");
    else
      jfr2w_stdout = fopen(sout_fname, "a");
    if (jfr2w_stdout == NULL)
    { jfr2w_stdout = stdout;
      fprintf(jfr2w_stdout, "Cannot open %s for output.\n");
    }
  }
  jfr2w_errmode = JFE_NONE;
  jfr_init(0);
  if ((m = jfr_load(&jfr2w_head, so_fname)) != 0)
  { jf_error(m, so_fname, JFE_FATAL);
    return jfr2w_close();
  }

  if ((jfr2w_op = fopen(de_fname, "wb")) == NULL)
  { jf_error(1, de_fname, JFE_FATAL);
    return jfr2w_close();
  }

  jfr_head = (struct jfr_head_desc *) jfr2w_head;

  if (maxtext != 0)
    jfr2w_maxtext = maxtext;
  else
    jfr2w_maxtext = 512;
  jfr2w_text = (char *) malloc(jfr2w_maxtext);
  if (jfr2w_text == NULL)
  { jf_error(6, jfr2w_spaces, JFE_FATAL);
    return jfr2w_close();
  }
  jfr2w_text[0] = '\0';

  if (maxtree != 0)
    jfr2w_maxtree = maxtree;
  else
    jfr2w_maxtree = 128;
  m = sizeof(struct jfg_tree_desc) * jfr2w_maxtree;
  jfr2w_tree = (struct jfg_tree_desc *) malloc(m);
  if (jfr2w_tree == NULL)
  { jf_error(6, jfr2w_spaces, JFE_FATAL);
    return jfr2w_close();
  }

  if (ndigits != 0)
    jfr2w_digits = ndigits;
  else
    jfr2w_digits = 5;

  jfr2w_write_rules = rule_nos;
  jfr2w_mode = smode;
  if (maxstack != 0)
    jfr2w_stacksize = maxstack;
  else
    jfr2w_stacksize = 64;
  jfr2w_sw_comments = sw_comments;
  jfr2w_errno = 0;

  m = jfg_init(jfr2w_mode, jfr2w_stacksize, jfr2w_digits);
  if (m != 0)
  { jf_error(m, jfr2w_t_stack, JFE_FATAL);
    return jfr2w_close();
  }

  jfg_sprg(&jfr2w_spdesc, jfr2w_head);

  jfr2w_plf_handle();

  jfr2w_copy_head();

  if (jfr2w_op != NULL)
    fclose(jfr2w_op);
  if ((jfr2w_op = fopen(de_fname, "a")) == NULL)
  { jf_error(1, de_fname, JFE_FATAL);
    return jfr2w_close();
  }

  jfr2w_program_handle();
  fclose(jfr2w_op);
  jfr_close(jfr2w_head);
  jfg_free();
  jfr_free();
  return jfr2w_errmode;
}





