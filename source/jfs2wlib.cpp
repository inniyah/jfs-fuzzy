  /***********************************************************************/
  /*                                                                     */
  /* jfs2wlib.cpp Version 2.03  Copyright (c) 1999-2000 Jan E. Mortensen */
  /*                                                                     */
  /* JFS Compiler-functions. Converts from JFS-file til JFW-file.        */
  /*                                                                     */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                  */
  /*    Lollandsvej 35 3.tv.                                             */
  /*    DK-2000 Frederiksberg                                            */
  /*    Denmark                                                          */
  /*                                                                     */
  /***********************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "jfr_gen.h"
#include "jfs_cons.h"
#include "jfs_text.h"

/******************************************************************/
/* Variable til hukkomelsesstyring                                */
/******************************************************************/

static int jfc_pass = 1;  /* 1: first pass, only allocate size,   */
                          /* 2: real compile.                     */

static void *jfc_memory             = NULL;
static unsigned char *jfc_ff_memory;
static unsigned char *jfc_ff_jfr    = NULL;
static FILE *jfc_source             = NULL;
static FILE *jfc_erfile             = NULL;
static FILE *jfc_dest               = NULL;

/***************************************************************/
/* variable for tokeniser                                      */
/***************************************************************/

#define JFC_MAXSENT 512
#define JFC_MAXBUF  512

static char jfc_ttypes[256];          /* tokentypes for hvert tegn */

static char jfc_sent[JFC_MAXSENT + 1];  /* aktuelle sourcesaetning    */
static int  jfc_ff_sent;

static char jfc_buf[JFC_MAXBUF];  /* input buffer.                    */
static int  jfc_cu_buf;           /* aktual char in input buffer.     */
static int  jfc_ff_buf;           /* first free in input buffer.      */
static int  jfc_eof;              /* 1: eof in sourcefil.             */
static char jfc_cu_source;        /* current char in sourcen.         */

static int  jfc_maxargline = 1024;
static char *jfc_argline = NULL;  /* tokens i current statement.      */
static int  jfc_argl_ff;          /* first free in argl_ff.           */

struct jfc_argv_desc { char *arg;    /* pointer to jfc_argline */
                                     /* (if token).            */
                       char  type;   /* 1:  token  */
                                     /* 2:  %      */
                                     /* 3:  ;      */
                                     /* 4:  "      */
                                     /* 5:  :      */
                                     /* 6:  (      */
                                     /* 7:  )      */
                                     /* 8: nl      */
                                     /* 9: #       */
                                     /*10: token (aldrig?) */
                                     /*11: -       */
                                     /*12: ,       */
                                     /*13: comment */
                       char qoute;   /* 1: token i "" */
       };

static int jfc_margc = 256;
static struct jfc_argv_desc *jfc_argv = NULL;/* words in current statement.*/
static int jfc_argc;  /* number of words in current statement. */
static int jfc_targc; /* jfc_argc - %'er.              */

static int jfc_line_count;  /* numer of lines (not number of statemets). */

static struct jfw_synonym_desc *jfc_synonyms = NULL;
static int  jfc_ff_synonyms = 0;

static int  jfc_comment_id = 0;

/************************************************************************/
/* Variable til default-names i var-declarations (grisekode)            */
/************************************************************************/

struct jfc_def_name_desc { char name[16];
                         };
#define JFC_DEF_NAMES_SIZE 127

static struct jfc_def_name_desc jfc_def_names[JFC_DEF_NAMES_SIZE];
static int jfc_ff_def_names = 0;

/************************************************************************/
/*  Variables til afkodning af keywords                                 */
/************************************************************************/


static const char jfc_t_hedge[]     = "hedge";
static const char jfc_t_conf[]      = "conf";
static const char jfc_t_is[]        = "is";
static const char jfc_t_type[]      = "type";
static const char jfc_t_plf[]       = "plf";
static const char jfc_t_confidence[]= "confidence";

struct jfc_t_table_desc { const char *name;
                          int  argc;
                        };

static const char jfc_dn_fbool[]      = "fbool";      /* 0 */
static const char jfc_dn_float[]      = "float";      /* 1 */

static const char jfc_hn_not[]        = "not";        /* 0 */

static const char jfc_on_caseop[]     = "caseop";     /* 0 */
static const char jfc_on_weightop[]   = "weightop";   /* 1 */
static const char jfc_on_and[]        = "and";        /* 2 */
static const char jfc_on_or[]         = "or";         /* 3 */
static const char jfc_on_whileop[]    = "whileop";    /* 4 */

static struct jfc_t_table_desc jfc_t_domargs[] =
     {{ "text",         0},  /* 0 */
      { "type",         0},  /* 1 */
      { "min",          0},  /* 2 */
      { "max",          0},  /* 3 */
      { " ", -1 }
     };

static struct jfc_t_table_desc jfc_t_adjargs[] =
     { {"plf",           0},  /* 0 */
       {"base",          1},  /* 1 */
       {"center",        1},  /* 2 */
       {"hedge",         1},  /* 3 */
       {"trapez",        2},  /* 4 */
       {" ", -1}
     };

static struct jfc_t_table_desc jfc_t_varargs[] =
     { {"acut",          1},  /* 0 */
       {"argument",      1},  /* 1 */
       {"normal",        1},  /* 2 */
       {"default",       1},  /* 3 */
       {"defuz",         1},  /* 4 */
       {"f_comp",        1},  /* 5 */
       {"d_comp",        1},  /* 6 */
       {"text",          1},  /* 7 */
       {"domain",        1},  /* 8 */
       {" ", -1}
     };


static struct jfc_t_table_desc jfc_t_arrargs[] =
     { {"size",          0},  /* 0 */
       {" ", -1}
     };


static struct jfc_t_table_desc jfc_t_opargs[] =
     { {"hedge",     1},     /* 0 */
       {"precedence",1},     /* 1 */
       {"type",      1},     /* 2 */
       {"hedgemode", 1},     /* 3 */
       {" " ,       -1}
     };

static struct jfc_t_table_desc jfc_t_operators[] =
   { {"#",        0},       /*  0 = op_one */
     {"min",      0},       /*  1 */
     {"max",      0},       /*  2 */
     {"prod",     0},       /*  3 */
     {"psum",     0},       /*  4 */
     {"avg",      0},       /*  5 */
     {"bsum",     0},       /*  6 */
     {"new",      0},       /*  7 */
     {"mxor",     0},       /*  8 */
     {"sptrue",   0},       /*  9 */
     {"spfalse",  0},       /* 10 */
     {"smtrue",   0},       /* 11 */
     {"smfalse",  0},       /* 12 */
     {"r0",       0},       /* 13 */
     {"r1",       0},       /* 14 */
     {"r2",       0},       /* 15 */
     {"r3",       0},       /* 16 */
     {"r4",       0},       /* 17 */
     {"r5",       0},       /* 18 */
     {"r6",       0},       /* 19 */
     {"r7",       0},       /* 20 */
     {"r8",       0},       /* 21 */
     {"r9",       0},       /* 22 */
     {"r10",      0},       /* 23 */
     {"r11",      0},       /* 24 */
     {"r12",      0},       /* 25 */
     {"r13",      0},       /* 26 */
     {"r14",      0},       /* 27 */
     {"r15",      0},       /* 28 */
     {"ham_and",  1},       /* 29 */
     {"ham_or",   1},       /* 30 */
     {"yager_and",1},       /* 31 */
     {"yager_or", 1},       /* 32 */
     {"bunion",   0},       /* 33 */
     {"similar",  0},       /* 34 */
     {" ", -1}
     };

static struct jfc_t_table_desc jfc_t_blocks[] =
    { {"title",        0},  /*  0 */
      {"domains",      0},  /*  1 */
      {"adjectives",   0},  /*  2 */
      {"input",        1},  /*  3 */
      {"output",       1},  /*  4 */
      {"local",        1},  /*  5 */
      {"hedges",       0},  /*  6 */
      {"relations",    0},  /*  7 */
      {"rules",        0},  /*  8 */
      {"synonyms",     0},  /*  9 */
      {"program",      0},  /* 10 */
      {"operators",    0},  /* 11 */
      {"arrays",       0},  /* 12 */
      {" ", -1}
    };


struct jfc_t_func_desc { char *name;
                         unsigned char a_type;
                         unsigned char arg;
                         signed char argc;
                       };


/**********************************************************************/
/* Diverse variable                                                   */
/**********************************************************************/

/* Error-types : */
#define JFE_NONE    0
#define JFE_WARNING 1
#define JFE_ERROR   2
#define JFE_FATAL   3

#define JFC_MAXERRS 25

static struct jfr_head_desc jfc_ihead; /* jfr-head, destination.      */
static struct jfr_head_desc *jfc_head; /* pointer to jfc_ihead.       */

static int jfc_err_count;      /* antal fejl under oversaettelsen.    */

static char jfc_empty[] = " "; /* tom streng til fejlmeddellelser.    */


static int jfc_gl_error_mode;  /* global error mode */

static const char *jfc_err_modes[] =
   { "NONE",
     "WARNING",
     "ERROR",
     "FATAL ERROR"
   };

static int jfc_err_line_no = -1;
static int jfc_err_message_mode; /* 0: normal error-messages,           */
                                 /* 1: compact error-messages.          */

struct jfc_err_desc { int eno;
                      const char *text;
                    };

static struct jfc_err_desc jfc_err_texts[] =
    {
     {0, " "},
     {1, "Error opening file:"},
     {2, "Error reading from file:"},
     {3, "Error writing to file:"},
     {6, "Cannot allocate memory to:"},
     {7, "Syntax error in:"},
     {8, "Out of memory"},
     {9, "Illegal number:"},
    {11, "Unexpected EOF."},

   {103, "Adjectiv value out of domain range:"},
   {104, "Argument-id < 0:"},
   {105, "Too many words in sentence."},
   {106, "Too many domains (max 255)."},
   {107, "Too many limits in pl-function (max 255)."},
   {108, "Too many relations (max 255)."},
   {109, "Unknown operator:"},
   {110, "Undefined domain:"},
   {111, "Unknown hedge-mode:"},
   {113, "Undefined hedge:"},
   {114, "Too many operators (max 255)."},
   {115, "Illegal second operator-type:"},
   {120, "Unknown hedge type:"},
   {121, "Constant out of range [0.0, 1.0]:"},
   {122, "Constant out of range ]0.0, inf[:"},
   {123, "Constant out of range ]0.0, 1.0[:"},
   {124, "Illegal precedence [1, 255]:"},
   {125, "Unknown operator-type:"},
   {126, "Colon (:) expected."},
   {127, "Wrong number off arguments."},
   {128, "Multiple declaration of:"},
   {129, "Too many hedges (max 255)."},
   {131, "Unknown defuzificationfunction: "},
   {133, "Too many errors."},
   {134, "Text or number expected."},
   {142, "Sentence to long."},
   {144, "Illegal placed comment. Comment ignored."},
   {146, "Unknown domain-type:"},
   {147, "too many arrays."},
   {148, "Illegal array-size:"},
   {149, "Too many (max 128) adjectives bound to:"},
   {151, "Too many default-names (max 128)"},
   {152, "Unknown default-adjectiv in variable:"},
   {155, "Trapez start-point > end-point."},
   {156, "Both trapez and plf in adjectiv."},
   {157, "Domain_min > domain_max."},
   {158, "No plf-function bound to:"},
   {159, "No operator-type specified"},
  {9999, "Unknown error"}
};

/* hjaelpe - funktioner */

  static int jfc_error(int eno, char *name, int mode);
  static int jfc_tcut(int argno, int alen);
  static float jfc_atof(int v);
  static int jf_atof(float *d, char *a);
  static int jfc_afind(char *tab[], int argc, char *arg);

  static int jfc_table_find(int *argc, struct jfc_t_table_desc *tab,
                            char *arg);
  static int jfc_getline(void);
  static void jfc_skipline(void);
  static int jfc_gettoken(void);
  static int jfc_getchar(void);

  static int jfc_domain_find(char *name);
  static int jfc_array_find(char *arrname);
  static int jfc_adjectiv_find(int varno, char *name);
  static int jfc_var_find(char *name);
  static int jfc_hedge_find(char *name);
  static int jfc_operator_find(char *name);
  static int jfc_relation_find(char *name);

  static unsigned char *jfc_malloc(int size);

/* jfc_comp */
  static int jfc_comp(void);
  static int jfc_plf_insert(int *id);
  static void jfc_comment_handle(int cstate);
  static void jfc_init(void);
  static int jfc_syn_ins(void);
  static int jfc_domain_ins(void);
  static void jfc_first_domains(void);
  static int jfc_adjectiv_ins(void);
  static int jfc_var_ins(int var_type);
  static void jfc_first_vars(void);
  static int jfc_hedge_ins(void);
  static void jfc_first_hedges(void);
  static int jfc_relation_ins(void);
  static int jfc_operator_ins(void);
  static void jfc_first_operators(void);
  static int jfc_array_ins(void);
  static int jfc_progline_ins(void);
  static void jfc_var_def_handle(void);

  static int jfc_close(void);
  static void jfc_mem_set(void);

  static int jfc_save(char *fname);

/*************************************************************************/
/* Generelle hjaelpefunktoner                                            */
/*************************************************************************/

static int jfc_error(int eno, char *name, int mode)
{
  int m, v;
  //char *t;

  m = -1;
  if (jfc_gl_error_mode != JFE_FATAL)
  { m = 0;
    jfc_sent[jfc_ff_sent] = '\0';
    if (jfc_err_count > JFC_MAXERRS)
    { eno = 133;
      mode = JFE_FATAL;
    }
    for (v = 0; 1 == 1; v++)
    { if (jfc_err_texts[v].eno == eno || jfc_err_texts[v].eno == 9999)
        break;
    }
    if (jfc_line_count != jfc_err_line_no)  /* only one error pr line */
    { if (jfc_err_message_mode == 0)
        fprintf(jfc_erfile, "%s %d: %s %s\n", jfc_err_modes[mode],
                            eno, jfc_err_texts[v].text, name);
      else
        fprintf(jfc_erfile, "%s#%d#%d#%s %s#\n", jfc_err_modes[mode],
                            eno, jfc_line_count, jfc_err_texts[v].text, name);
      if (mode != JFE_WARNING)
      { jfc_err_count++;
        m = -1;
      }
      if (jfc_err_message_mode == 0)
        fprintf(jfc_erfile, "in line %d:\n%s\n", jfc_line_count, jfc_sent);
    }
    jfc_err_line_no = jfc_line_count;
    if (mode == JFE_FATAL)
    { jfc_gl_error_mode = JFE_FATAL;
      fprintf(jfc_erfile, "\n*** COMPILATION ABORTED! ***\n");
    }
  }
  return m;
}

static int jfc_tcut(int argno, int alen)
{
  if (argno >= jfc_argc)
  { jfc_argv[argno].arg = jfc_empty;
    return jfc_error(127, jfc_empty, JFE_ERROR);
  }
  if (jfc_argv[argno].type != 1)
  { jfc_argv[argno].arg = jfc_empty;
    return jfc_error(134, jfc_empty, JFE_ERROR);
  }
  if (strlen(jfc_argv[argno].arg) >= alen)
    jfc_argv[argno].arg[alen -1] = '\0';
  return 0;
}

static int jf_atof(float *d, char *a)
{
  int m;
  char c;
  int state;  /* 0: foer fortegn,  */
       /* 1: efter fortegn, */
       /* 2: efter komma,   */

  state = 0;
  *d = 0.0;
  for (m = 0; a[m] != '\0'; m++)
  { if (a[m] == ',')
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
            return 1;
        }
        break;
      case 1:
        if (!isdigit(c))
        { if (c == '.')
            state = 2;
          else
            return 1;
        }
        break;
      case 2:
        if (!isdigit(c))
          return 1;
        break;
    }
  }
  if (state == 0)
    *d = 0.0;
  else
    *d = atof(a);
  return 0;
}


static float jfc_atof(int v)
{
  float f;

  if (v >= jfc_argc)
  { jfc_error(127, jfc_empty, JFE_ERROR);
    return 0.0;
  }
  if (jfc_argv[v].type != 1)
  { jfc_error(134, jfc_empty, JFE_ERROR);
    return 0.0;
  }

  if (jf_atof(&f, jfc_argv[v].arg) != 0)
    jfc_error(9, jfc_argv[v].arg, JFE_ERROR);
  return f;
}

static int jfc_afind(char *tab[], int argc, char *arg)
{
  int m;

  for (m = 0; m < argc; m++)
  { if (strcmp(arg, tab[m]) == 0)
      return m;
  }
  return -1;
}

static int jfc_table_find(int *argc, struct jfc_t_table_desc *tab, char *arg)
{
  int m;

  for (m = 0; tab[m].argc != -1; m++)
  { if (strcmp(arg, tab[m].name) == 0)
    { *argc = tab[m].argc;
      return m;
    }
  }
  *argc = 0;
  return -1;
}

static int jfc_domain_find(char *name)
{
  int m;

  if (strcmp(name, jfc_dn_fbool) == 0)
    return 0;
  if (strcmp(name, jfc_dn_float) == 0)
    return 1;
  if (jfc_pass == 2)
  for (m = 0; m < jfc_head->domain_c; m++)
  { if (strcmp(name, jfc_head->domains[m].name) == 0)
      return m;
  }
  return -1;
}

static int jfc_array_find(char *arrname)
{
  int m;

  if (jfc_pass == 2)
  for (m = 0; m < jfc_head->array_c; m++)
  { if (strcmp(arrname, jfc_head->arrays[m].name) == 0)
      return m;
  }
  return -1;
}

static int jfc_adjectiv_find(int var_no, char *adjname)
{
  /* NB: henter fra variable hvis fzvar_c > 0 ellers fra domain */

  int m, adjno;
  struct jfr_var_desc *v;
  struct jfr_domain_desc *d;

  v = &(jfc_head->vars[var_no]);
  if (jfc_pass == 2)
  { if (v->fzvar_c > 0)
    { for (m = 0; m < v->fzvar_c; m++)
      { adjno = v->f_adjectiv_no + m;
        if (strcmp(adjname, jfc_head->adjectives[adjno].name) == 0)
          return adjno;
      }
    }
    else
    { d = &(jfc_head->domains[v->domain_no]);
      for (m = 0; m < d->adjectiv_c; m++)
      { adjno = d->f_adjectiv_no + m;
        if (strcmp(adjname, jfc_head->adjectives[adjno].name) == 0)
          return adjno;
      }
    }
  }
  return -1;
}

static int jfc_var_find(char *name)
{
  int m;

  for (m = 0; m < jfc_head->ivar_c; m++)
  { if (strcmp(name, jfc_head->vars[jfc_head->f_ivar_no + m].name) == 0)
      return jfc_head->f_ivar_no + m;
  }
  for (m = 0; m < jfc_head->ovar_c; m++)
  { if (strcmp(name, jfc_head->vars[jfc_head->f_ovar_no + m].name) == 0)
      return jfc_head->f_ovar_no + m;
  }
  for (m = 0; m < jfc_head->lvar_c; m++)
  { if (strcmp(name, jfc_head->vars[jfc_head->f_lvar_no + m].name) == 0)
      return jfc_head->f_lvar_no + m;
  }
  return -1;
}

static int jfc_hedge_find(char *name)
{
  int m;

  if (strcmp(name, jfc_hn_not) == 0)
    return 0;
  if (jfc_pass == 2)
  for (m = 0; m < jfc_head->hedge_c; m++)
  { if (strcmp(jfc_head->hedges[m].name, name) == 0)
      return m;
  }
  return -1;
}

static int jfc_operator_find(char *name)
{
  int m;

  if (jfc_pass == 2)
  for (m = 0; m < jfc_head->operator_c; m++)
  { if (strcmp(jfc_head->operators[m].name, name) == 0)
      return m;
  }
  if (strcmp(name, jfc_on_caseop) == 0)
    return 0;
  if (strcmp(name, jfc_on_weightop) == 0)
    return 1;
  if (strcmp(name, jfc_on_and) == 0)
    return 2;
  if (strcmp(name, jfc_on_or) == 0)
    return 3;
  if (strcmp(name, jfc_on_whileop) == 0)
    return 4;

  return -1;
}

static int jfc_relation_find(char *name)
{
  int m;

  if (jfc_pass == 2)
  for (m = 0; m < jfc_head->relation_c; m++)
  { if (strcmp(jfc_head->relations[m].name, name) == 0)
      return m;
  }
  return -1;
}

static unsigned char *jfc_malloc(int size)
{
  /* Intern malloc. Allocates memory in jfm_memory.  */

  unsigned char *adr;

  if (jfc_ff_jfr + size >= jfc_ff_memory)
  { adr = (unsigned char *) jfc_memory;
    jfc_error(8, jfc_empty, JFE_FATAL);
  }
  else
  { adr = jfc_ff_jfr;
    jfc_ff_jfr += size;
  }
  return adr;
}


/*************************************************************************/
/* Laesning af input, opsplitning i linier og tokens                     */
/*************************************************************************/

static int jfc_getchar(void)  /* reads a char into jfc_cu_source */
{
  if (jfc_cu_buf >= jfc_ff_buf)
  { jfc_ff_buf = fread(jfc_buf, 1, JFC_MAXBUF, jfc_source);
    if (jfc_ff_buf <= 0)
    { jfc_eof = 1;
      return -1;         /* end of file */
    }
    jfc_cu_buf = 0;
  }
  jfc_cu_source = jfc_buf[jfc_cu_buf];
  if (jfc_ff_sent < JFC_MAXSENT)
  { jfc_sent[jfc_ff_sent] = jfc_cu_source;
    jfc_ff_sent++;
  }
  jfc_cu_buf++;
  return 0;
}

static void jfc_skipline(void)
{
  int fundet;

  fundet = 0;
  while (fundet == 0)
  { if (jfc_eof == 1)
      fundet = 1;
    else
    if (jfc_ttypes[jfc_cu_source] == 8)
      fundet = 1;
    else
      jfc_getchar();
  }
}

static int jfc_gettoken(void)
{
  int fundet, sf, s;
  int istate;
  /* char *adr;  */

  fundet = 0; istate = 0;
  if (jfc_argc >= jfc_margc - 1)
  { jfc_error(105, jfc_empty, JFE_ERROR);
    jfc_argc = jfc_margc - 2;
    jfc_targc = 0;
    jfc_argl_ff = 0;
  }
  jfc_argv[jfc_argc].arg = jfc_empty;
  jfc_argv[jfc_argc].qoute = 0;
  while (fundet == 0)
  { if (jfc_argl_ff >= jfc_maxargline)
    { jfc_error(142, jfc_empty, JFE_ERROR);
      jfc_argl_ff = jfc_maxargline - 1;
    }
    if (jfc_eof == 1)
      fundet = -1;
    else
    { switch (istate)
      { case 0:          /* space */
          switch (jfc_ttypes[jfc_cu_source])
          { case 0:
              break;
            case 1:
              jfc_argline[jfc_argl_ff] = jfc_cu_source;
              jfc_argv[jfc_argc].type = 1;
              jfc_argv[jfc_argc].arg = &(jfc_argline[jfc_argl_ff]);
              jfc_argl_ff++;
              istate = 1;
              break;
            case 4:       /* " */
              jfc_argline[jfc_argl_ff] = '\0';
              jfc_argv[jfc_argc].type = 1;
              jfc_argv[jfc_argc].qoute = 1;
              jfc_argv[jfc_argc].arg = &(jfc_argline[jfc_argl_ff]);
              istate = 4;
              break;
            case 10:      /*  +,*,/,>,<,=,!  */
              jfc_argline[jfc_argl_ff] = jfc_cu_source;
              jfc_argv[jfc_argc].type = 1;
              jfc_argv[jfc_argc].arg = &(jfc_argline[jfc_argl_ff]);
              jfc_argl_ff++;
              istate = 2;
              break;
            default:
              jfc_argv[jfc_argc].type = jfc_ttypes[jfc_cu_source];
              if (jfc_ttypes[jfc_cu_source] == 3)   /* ; */
                jfc_cu_source = '\0';
              fundet = 1;
              break;
          }
          jfc_getchar();
          break;
        case 1:     /* i token */
          if (jfc_ttypes[jfc_cu_source] == 1)
          { jfc_argline[jfc_argl_ff] = jfc_cu_source;
            jfc_argl_ff++;
            jfc_getchar();
          }
          else
          { jfc_argline[jfc_argl_ff] = '\0';
            jfc_argl_ff++;
            sf = -1;
            for (s = 0; sf == -1 && s < jfc_ff_synonyms; s++)
            { if (strcmp(jfc_synonyms[s].name, jfc_argv[jfc_argc].arg) == 0)
                sf = s;
            }
            if (sf != -1)
            { if (strlen(jfc_synonyms[sf].value) == 0)
                istate = 0;
              else
              { if (jfc_argl_ff + strlen(jfc_synonyms[sf].value) + 1
                     >= jfc_maxargline)
                 { jfc_error(142, jfc_empty, JFE_ERROR);
                   jfc_argl_ff = jfc_maxargline - 1;
                 }
                 else
                 { strcpy(&(jfc_argline[jfc_argl_ff]),
                   jfc_synonyms[sf].value);
                   jfc_argv[jfc_argc].arg = &(jfc_argline[jfc_argl_ff]);
                   jfc_argl_ff += strlen(jfc_synonyms[sf].value);
                   jfc_argline[jfc_argl_ff] = '\0';
                   jfc_argl_ff++;
                   fundet = 1;
                 }
              }
            }
            else
              fundet = 1;
            if (sf == -1 && strcmp(jfc_argv[jfc_argc].arg, jfc_t_is) == 0)
            { istate = 0;
              fundet = 0;
            }
          }
          break;
        case 2:     /* i operator */
          if (jfc_ttypes[jfc_cu_source] == 10)
          { jfc_argline[jfc_argl_ff] = jfc_cu_source;
            jfc_argl_ff++;
            jfc_getchar();
            if (jfc_argl_ff == 2)
            { if (jfc_argline[0] == '/' && jfc_argline[1] == '*')
              { istate = 5;
                jfc_argv[jfc_argc].type = 13;
                jfc_argv[jfc_argc].arg = &(jfc_argline[2]);
              }
            }
            if (jfc_argl_ff >= 2)
            { if (jfc_argline[jfc_argl_ff - 2] == '/'
                  && jfc_argline[jfc_argl_ff - 1] == '/')
              { jfc_argl_ff -= 2;
                jfc_skipline();
                istate = 0;
              }
            }
          }
          else
          { jfc_argline[jfc_argl_ff] = '\0';
            jfc_argl_ff++;
            fundet = 1;
          }
          break;
        case 4:          /* i " "  */
          if (jfc_ttypes[jfc_cu_source] != 4)
            jfc_argline[jfc_argl_ff] = jfc_cu_source;
          else
          { jfc_argline[jfc_argl_ff] = '\0';
            fundet = 1;
          }
          jfc_argl_ff++;
          jfc_getchar();
          break;
        case 5:          /* i comment */
          jfc_argline[jfc_argl_ff] = jfc_cu_source;
          if (jfc_cu_source == '*')
            istate = 6;
          jfc_argl_ff++;
          if (jfc_ttypes[jfc_cu_source] == 8)  /* nl */
            jfc_line_count++;
          jfc_getchar();
          break;
        case 6:        /* i comment efter '*'  */
          jfc_argline[jfc_argl_ff] = jfc_cu_source;
          jfc_argl_ff++;
          if (jfc_cu_source == '/')
          { fundet = 1;
            jfc_argline[jfc_argl_ff - 2] = '\0';
            jfc_argl_ff++;
          }
          else
          { istate = 5;
            if (jfc_ttypes[jfc_cu_source] == 8)
              jfc_line_count++;
          }
          jfc_getchar();
          break;
      } /* switch */
    }
  } /* while */

  return fundet;
}

static int jfc_getline(void)
{
  int m, fundet, dum;

  fundet = 0;
  while (fundet == 0)
  { for (m = 0; m < jfc_argc; m++)
      jfc_argv[m].arg = jfc_empty;
    jfc_argc = 0;
    jfc_argl_ff = 0;
    jfc_targc = 0;
    jfc_ff_sent = 0;
    while (fundet == 0)
    { m = jfc_gettoken();
      if (m == -1)
        fundet = -1;
      else
      if (jfc_argv[jfc_argc].type == 9)   /*  '#'  */
        jfc_skipline();
      else
      if (jfc_argv[jfc_argc].type == 3)   /* ';'   */
      { if (jfc_argc > 0)
          fundet = 1;
      }
      else
      if (jfc_argv[jfc_argc].type == 8)   /* nl    */
        jfc_line_count++;
      else
      if (jfc_argv[jfc_argc].type == 13)  /* comment */
        fundet = 1;
      else
      { if (jfc_argv[jfc_argc].type != 2) /* % */
          jfc_targc++;
        if (jfc_argv[jfc_argc].type == 1
            && jfc_argv[jfc_argc].qoute == 0 /* hvis token .. */
            && jfc_argc == 0)
        { if (jfc_table_find(&dum, jfc_t_blocks,
                             jfc_argv[jfc_argc].arg) != -1) /* og blok */
          { jfc_argv[jfc_argc + 1].type = 3;
            fundet = 1;
          }
        }
        jfc_argc++;
      }
    }
  }  /* while (fundet == 0) */

  return fundet;
}

static int jfc_plf_insert(int *id)
{
  int tilst, slut, lcount, tflag;
  char *s;
  // float x, y, hx, hy;

  tilst = 0; slut = 0; lcount = 0;
  jfc_head->limits[jfc_head->limit_c].flags = 0;
  jfc_head->limits[jfc_head->limit_c].exclusiv = 0;
  if (strcmp(jfc_argv[*id].arg, jfc_t_plf) == 0)
    (*id)++;
  while (slut == 0 && *id < jfc_argc)
  { switch (tilst)
    { case 0:     /* x  */
        tflag = 0;
        if (jfc_argv[*id].type == 2)
          tflag = 1;
        if (*id + 2 + tflag >= jfc_argc)
          slut = 1;
        if (slut == 0)
        { if (jfc_argv[*id + 1 + tflag].type != 5)
            slut = 1;
        }
        if (slut == 0)
        { if (jfc_argv[*id].type == 2)
          { (*id)++;
            jfc_head->limits[jfc_head->limit_c].flags |= JFS_LF_IX;
          }
          if (jfc_argv[*id].type == 1)
          { s = jfc_argv[*id].arg;
            if (s[strlen(s) - 1] == 'x')
            { s[strlen(s) - 1] = '\0';
              jfc_head->limits[jfc_head->limit_c].exclusiv = 1;
            }
            jfc_head->limits[jfc_head->limit_c].limit = jfc_atof(*id);
          }
          else
            jfc_error(9, jfc_argv[*id].arg, JFE_ERROR);
          (*id)++;
          tilst = 1;
        }
        break;
      case 1:   /* : */
        if (jfc_argv[*id].type != 5)    /* ':' */
          jfc_error(126, jfc_empty, JFE_ERROR);
        tilst = 2;
        (*id)++;
        break;
      case 2:   /* y */
        if (jfc_argv[*id].type == 2)
        { (*id)++;
          jfc_head->limits[jfc_head->limit_c].flags |= JFS_LF_IY;
        }
        jfc_head->limits[jfc_head->limit_c].b = jfc_atof(*id);  /* .value */
        jfc_head->limit_c++;
        lcount++;
        (*id)++;
        jfc_head->limits[jfc_head->limit_c].flags = 0;
        jfc_head->limits[jfc_head->limit_c].exclusiv = 0;
        tilst = 0;
        break;
    }
  } /* while */
  if (tilst != 0)
  { jfc_error(7, jfc_empty, JFE_ERROR);
  }
  if (lcount == 0)
  { jfc_error(7, jfc_empty, JFE_ERROR);
    (*id)++;
  }
  if (lcount > 255)
  { jfc_error(107, jfc_empty, JFE_ERROR);
    lcount = 255;
  }
  return lcount;
}

static int jfc_syn_ins(void)
{
  struct jfw_synonym_desc *syn;

  if (jfc_argc < 1 || jfc_argc > 2)
    return jfc_error(127, jfc_empty, JFE_ERROR);
  if (jfc_pass == 1)
  { syn = (struct jfw_synonym_desc *) jfc_malloc(JFW_SYNONYM_SIZE);
    if (jfc_gl_error_mode == JFE_FATAL)
      return -1;
    jfc_tcut(0, 32),
    strcpy(syn->name, jfc_argv[0].arg);
    if (jfc_argc == 2)
    { jfc_tcut(1, 96);
      strcpy(syn->value, jfc_argv[1].arg);
    }
    else
      syn->value[0] = '\0';
  }
  jfc_ff_synonyms++;
  return 0;
}

static void jfc_comment_handle(int cstate)
{
  struct jfr_comment_desc *comment;
  int comment_no, s;
  char *adr;

  comment_no = jfc_head->comment_c;
  comment = &(jfc_head->comments[comment_no]);
  jfc_head->comment_c++;
  s = strlen(jfc_argv[0].arg) + 1;
  if (jfc_pass == 1)
    jfc_head->com_block_c += s;
  else
  { comment->comment_c = s;
    comment->f_comment_id = jfc_head->com_block_c;
    adr = (char *) (jfc_head->comment_block + jfc_head->com_block_c);
    jfc_head->com_block_c += s;
    strcpy(adr, jfc_argv[0].arg);
    switch (cstate)
    { case 0:
        jfc_head->comment_no = comment_no;
        break;
      case 1:
        jfc_head->domains[jfc_comment_id].comment_no = comment_no;
        break;
      case 2:
        jfc_head->adjectives[jfc_comment_id].comment_no
          = comment_no;
        break;
      case 3:
      case 4:
      case 5:
        jfc_head->vars[jfc_comment_id].comment_no = comment_no;
        break;
      case 6:
        jfc_head->hedges[jfc_comment_id].comment_no = comment_no;
        break;
      case 7:
        jfc_head->relations[jfc_comment_id].comment_no = comment_no;
        break;
      case 11:
        jfc_head->operators[jfc_comment_id].comment_no
          = comment_no;
        break;
      case 12:
        jfc_head->arrays[jfc_comment_id].comment_no = comment_no;
        break;
    }
  }
}

static int jfc_domain_ins(void)
{
  struct jfr_domain_desc *dom;
  int v, m, dummy, dno, kw;

  jfc_comment_id = -1;
  jfc_tcut(0, 16);
  if (jfc_pass == 1)
  { if (   strcmp(jfc_argv[0].arg, jfc_dn_fbool) != 0
        && strcmp(jfc_argv[0].arg, jfc_dn_float) != 0)
      jfc_head->domain_c++;
  }
  else
  { if ((dno = jfc_domain_find(jfc_argv[0].arg)) != -1)
    { if (dno > 1)
        return jfc_error(128, jfc_argv[0].arg, JFE_ERROR);
      else
        dom = &(jfc_head->domains[dno]);
    }
    else
    { dno = jfc_head->domain_c;
      dom = &(jfc_head->domains[jfc_head->domain_c]);
      if (jfc_head->domain_c >= 255)
        return jfc_error(106, jfc_empty, JFE_ERROR);
      jfc_head->domain_c++;
      dom->unit[0] = '\0';
      dom->dmin = 0.0; dom->dmax = 1.0;
      dom->adjectiv_c = 0;
      dom->comment_no = -1;
      dom->type = 0; dom->flags = 0;
    }
    jfc_comment_id = dno;
    strcpy(dom->name, jfc_argv[0].arg);
    for (v = 1; v < jfc_argc; v++)
    { kw = jfc_table_find(&dummy, jfc_t_domargs, jfc_argv[v].arg);
      if (kw == -1)
      { if (jfc_argv[v].qoute == 1)
          kw = 0; /* text */
        else
        if (jfc_afind(jfs_t_dts, JFS_DT_COUNT, jfc_argv[v].arg) != -1)
          kw = 1; /* type */
        else
        if ((dom->flags & JFS_DF_MINENTER) == 0)
          kw = 2;  /* dmin */
        else
          kw = 3;  /* dmax */
      }
      else
        v++;
      switch (kw)
      { case 0: /* text */
          jfc_tcut(v, 40);
          strcpy(dom->unit, jfc_argv[v].arg);
          break;
        case 1: /* type */
          m = jfc_afind(jfs_t_dts, JFS_DT_COUNT, jfc_argv[v].arg);
          if (m >= 0)
            dom->type = m;
          else
            jfc_error(146, jfc_argv[v].arg, JFE_ERROR);
          break;
        case 2: /* min */
          dom->dmin = jfc_atof(v);
          dom->flags |= JFS_DF_MINENTER;
          break;
        case 3: /* max */
          dom->dmax = jfc_atof(v);
          dom->flags |= JFS_DF_MAXENTER;
          break;
      }
    }
    if ((dom->flags & 3) == 3 && dom->dmin >= dom->dmax)
      return jfc_error(157, jfc_empty, JFE_ERROR);
  }
  return 0;
}

static void jfc_first_domains(void)
{
  struct jfr_domain_desc *dom;

  dom = &(jfc_head->domains[JFS_DNO_FBOOL]);
  dom->unit[0] = '\0';
  dom->dmin = 0.0; dom->dmax = 1.0;
  dom->f_adjectiv_no = 0;
  dom->adjectiv_c = 0;
  dom->type = JFS_DT_FLOAT;
  dom->comment_no = -1;
  dom->flags = JFS_DF_MINENTER + JFS_DF_MAXENTER;
  strcpy(dom->name, jfc_dn_fbool);

  dom = &(jfc_head->domains[JFS_DNO_FLOAT]);
  dom->unit[0] = '\0';
  dom->dmin = 0.0; dom->dmax = 1.0;
  dom->f_adjectiv_no = 0;
  dom->adjectiv_c = 0;
  dom->type = JFS_DT_FLOAT;
  dom->comment_no = -1;
  dom->flags = 0;
  strcpy(dom->name, jfc_dn_float);
}


static int jfc_adjectiv_ins(void)
{
  struct jfr_adjectiv_desc *adjectiv;
  struct jfr_domain_desc *domain;
  struct jfr_var_desc *var, *v2;
  int v, kw, m, adj_no, domain_no, var_no, tflag, dummy;
  float f;

  jfc_comment_id = -1;
  if (jfc_pass == 1)
  { jfc_head->adjectiv_c++;
    for (v = 1; v < jfc_argc; v++)
    { if (jfc_argv[v].type == 5)  /* : */
        jfc_head->limit_c++;
    }
  }
  else
  { var_no = -1;
    if (jfc_targc < 2)
      return jfc_error(127, jfc_empty, JFE_ERROR);
    jfc_tcut(0, 16);
    domain_no = jfc_domain_find(jfc_argv[0].arg);
    if (domain_no == -1)
    { var_no = jfc_var_find(jfc_argv[0].arg);
      if (var_no == -1)
        return jfc_error(110, jfc_argv[0].arg, JFE_ERROR);
      var = &(jfc_head->vars[var_no]);
      if (var->fzvar_c == 0)
        var->f_adjectiv_no = jfc_head->adjectiv_c;
      adj_no = var->f_adjectiv_no + var->fzvar_c;
      if (var->fzvar_c >= 127)
        jfc_error(149, var->name, JFE_ERROR);
      var->fzvar_c++;
      domain_no = var->domain_no;
      domain = &(jfc_head->domains[domain_no]);
    }
    else
    { domain = &(jfc_head->domains[domain_no]);
      if (domain->adjectiv_c == 0)
        domain->f_adjectiv_no = jfc_head->adjectiv_c;
      adj_no = domain->f_adjectiv_no + domain->adjectiv_c;
      if (domain->adjectiv_c >= 127)
        jfc_error(149, domain->name, JFE_ERROR);
      domain->adjectiv_c++;
    }
    v = 1;
    jfc_tcut(v, 16);

    if (adj_no < jfc_head->adjectiv_c)
    { for (m = jfc_head->adjectiv_c; m > adj_no; m--)
        memcpy(&(jfc_head->adjectives[m]), &(jfc_head->adjectives[m - 1]),
               JFR_ADJECTIV_SIZE);
      for (m = 0; m < jfc_head->domain_c; m++)
      { if (jfc_head->domains[m].f_adjectiv_no >= adj_no)
          jfc_head->domains[m].f_adjectiv_no++;
      }
      for (m = 0; m < jfc_head->var_c; m++)
      { v2 = &(jfc_head->vars[m]);
        if (v2->f_adjectiv_no >= adj_no)
          v2->f_adjectiv_no++;
      }
    }

    adjectiv = &(jfc_head->adjectives[adj_no]);
    jfc_comment_id = adj_no;
    jfc_head->adjectiv_c++;

    adjectiv->base = adjectiv->center = 0.0;  /* ret i LIMITS-ins f.alle! */
    adjectiv->trapez_start = adjectiv->trapez_end = 0.0;
    strcpy(adjectiv->name, jfc_argv[v].arg);
    if (var_no != -1)
      adjectiv->var_no = var_no;
    else
    { adjectiv->flags = 0;
      adjectiv->var_no = 0;
    }
    adjectiv->limit_c = 0;
    adjectiv->comment_no = -1;
    adjectiv->f_limit_no = 0;
    adjectiv->domain_no = domain_no;
    adjectiv->h1_no = adjectiv->h2_no = 0;   /* none */
    v++;

    while (v < jfc_argc)
    { tflag = 0;
      kw = jfc_table_find(&dummy, jfc_t_adjargs, jfc_argv[v].arg);
      if (kw != -1)
        v++;
      else
      { if (jfc_argv[v].type == 2)
        { tflag = 1;
          v++;
        }
        if (jfc_argv[v + 1].type == 5)   /* ':', dvs multi limits vaerdier. */
          kw = 0; /* plf */
        else
        if (jfc_argv[v].type == 1)
        { if (jf_atof(&f, jfc_argv[v].arg) == 0)
            kw = 2; /* center */
          else
            kw = 3; /* hedge */
        }
      }
      v -= tflag;
      switch (kw)
      { case 0:  /* plf */
          adjectiv->f_limit_no = jfc_head->limit_c;
          adjectiv->limit_c = jfc_plf_insert(&v);
          break;
        case 1: /* base */
          if (jfc_argv[v].type == 2)
          { adjectiv->flags |= JFS_AF_IBASE;
            v++;
          }
          adjectiv->base = jfc_atof(v);
          adjectiv->flags |= JFS_AF_BASE;
          v++;
          break;
        case 2: /* center */
          if (jfc_argv[v].type == 2)
          { v++;
            adjectiv->flags |= JFS_AF_ICENTER;
          }
          adjectiv->flags |= JFS_AF_CENTER;
          adjectiv->center = jfc_atof(v);
          if (adjectiv->center < domain->dmin)
          { if ((domain->flags & JFS_DF_MINENTER) != 0)
              jfc_error(103, jfc_argv[v].arg, JFE_ERROR);
          }
          if (adjectiv->center > domain->dmax)
          { if ((domain->flags & JFS_DF_MAXENTER) != 0)
              jfc_error(103, jfc_argv[v].arg, JFE_ERROR);
          }
          v++;
          break;
        case 3:  /* hedge */
          m = jfc_hedge_find(jfc_argv[v].arg);
          if (m == -1)
            return jfc_error(113, jfc_argv[v].arg, JFE_ERROR);
          if (adjectiv->flags & JFS_AF_HEDGE)
            adjectiv->h2_no = m;
          else
          { adjectiv->h1_no = m;
            adjectiv->h2_no = m;
            adjectiv->flags |= JFS_AF_HEDGE;
          }
          v++;
          break;
        case 4:  /* trapez */
          adjectiv->flags |= JFS_AF_TRAPEZ;
          if (jfc_argv[v].type == 2)
          { adjectiv->flags |= JFS_AF_ISTRAPEZ;
            v++;
          }
          adjectiv->trapez_start = jfc_atof(v);
          if (adjectiv->trapez_start < domain->dmin)
          { if ((domain->flags & JFS_DF_MINENTER) != 0)
              jfc_error(103, jfc_argv[v].arg, JFE_ERROR);
          }
          v++;
          if (jfc_argv[v].type == 2)
          { adjectiv->flags |= JFS_AF_IETRAPEZ;
            v++;
          }
          adjectiv->trapez_end = jfc_atof(v);

          if (adjectiv->trapez_end > domain->dmax)
          { if ((domain->flags & JFS_DF_MAXENTER) != 0)
              jfc_error(103, jfc_argv[v].arg, JFE_ERROR);
          }
          if (adjectiv->trapez_start > adjectiv->trapez_end)
            jfc_error(155, jfc_empty, JFE_ERROR);
          v++;
          break;
        default:
          return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
      }
    } /* while */
    if ((adjectiv->flags & JFS_AF_TRAPEZ)
        && adjectiv->limit_c > 0)  /* trapez and plf */
      jfc_error(156, jfc_empty, JFE_ERROR);
  }
  return 0;
}

static int jfc_var_ins(int ip_type)
{
  int m, v, vno, ano, argc, domno;
  struct jfr_var_desc *var;
  float f;

  jfc_comment_id = -1;

  jfc_tcut(0, 16);
  if ((vno = jfc_var_find(jfc_argv[0].arg)) != -1)
    return jfc_error(128, jfc_argv[0].arg, JFE_ERROR);

  if (ip_type == 0)
  { vno = jfc_head->f_ivar_no + jfc_head->ivar_c;
    jfc_head->ivar_c++;
  }
  else
  if (ip_type == 1)
  { vno = jfc_head->f_ovar_no + jfc_head->ovar_c;
    jfc_head->ovar_c++;
  }
  else
  { vno = jfc_head->f_lvar_no + jfc_head->lvar_c;
    jfc_head->lvar_c++;
  }
  jfc_comment_id = vno;
  jfc_head->var_c++;

  if (jfc_pass == 2)
  { var = &(jfc_head->vars[vno]);
    strcpy(var->name, jfc_argv[0].arg);
    strcpy(var->text, var->name);
    var->vtype = ip_type;
    var->value = var->acut = var->conf = 0.0;
    var->no_arg = 1.0;
    var->defuz_arg = 0.0;
    var->defuz_1 = var->defuz_2 = JFS_VD_CENTROID;
    var->flags  = 0;
    var->argument = 0;
    var->f_adjectiv_no = var->f_fzvar_no = var->fzvar_c = 0;
    var->comment_no = -1;
    var->default_type = JFS_VDT_NONE;
    var->default_val = 0.0;
    var->default_no = 0;
    var->f_comp = JFS_ONO_OR;
    var->d_comp = JFS_VCF_NEW;
    var->domain_no = JFS_DNO_FLOAT;
    var->status = ip_type;

    v = 1;
    while (v < jfc_argc)
    { if ((ano = jfc_table_find(&argc, jfc_t_varargs, jfc_argv[v].arg))
          == -1)
      { if (jfc_argv[v].qoute == 1)
          ano = 7; /* text */
        else
          ano = 8; /* domain */
      }
      else
        v++;
      switch (ano)
      { case 0:                                 /* acut */
          if (jfc_argv[v].type == 2)
          { var->flags |= JFS_VF_IACUT;
            v++;
          }
          var->acut = jfc_atof(v);
          if (var->acut > 1.0 || var->acut < 0.0)
            jfc_error(121, jfc_argv[v].arg, JFE_ERROR);
          v++;
          break;
        case 1:                                 /* argument */
          m = atoi(jfc_argv[v].arg);
          if (m < 0)
            return jfc_error(104, jfc_empty, JFE_ERROR);
          var->argument = m;
          v++;
          break;
        case 2:                                 /* normal */
          var->flags |= JFS_VF_NORMAL;
          if (jfc_argv[v].type == 2)
          { var->flags |= JFS_VF_INORMAL;
            v++;
          }
          if (jfc_argv[v].type == 1)
          { if (jf_atof(&f, jfc_argv[v].arg) == 0)
            { if (f > 0.0 && f <= 1.0)
              { var->no_arg = f;
                v++;
              }
              else
              { if ((var->flags & JFS_VF_INORMAL) != 0)
                  return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
                var->no_arg = 1.0;
              }
            }
            else
            if (strcmp(jfc_argv[v].arg, jfc_t_conf) == 0
                || strcmp(jfc_argv[v].arg, jfc_t_confidence) == 0)
            { var->no_arg = -1.0;
              if ((var->flags & JFS_VF_INORMAL) != 0)
                return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
              v++;
            }
            else
              jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
          }
          else
            var->no_arg = 1.0;
          break;
        case 3:                                 /* default */
          if (jf_atof(&f, jfc_argv[v].arg) == 0)
          { var->default_val = f;
            var->default_type = JFS_VDT_VALUE;
          }
          else
          { if (jfc_ff_def_names >= JFC_DEF_NAMES_SIZE)
              jfc_error(151, jfc_argv[v].arg, JFE_ERROR);
            else
            { strcpy(jfc_def_names[jfc_ff_def_names].name,
                     jfc_argv[v].arg);
              var->default_type = JFS_VDT_ADJECTIV;
              var->default_no = jfc_ff_def_names;
              jfc_ff_def_names++;
            }
          }
          v++;
          break;
        case 4:                                /* defuz */
          m = jfc_afind(jfs_t_vds, JFS_VD_COUNT, jfc_argv[v].arg);
          if (m == -1)
            return jfc_error(131, jfc_argv[v].arg, JFE_ERROR);
          var->defuz_1 = m;
          var->defuz_2 = m;
          v++;
          m = jfc_afind(jfs_t_vds, JFS_VD_COUNT, jfc_argv[v].arg);
          if (m != -1)
          { var->defuz_2 = m;
            v++;
            if (jfc_argv[v].type == 2)
            { var->flags |= JFS_VF_IDEFUZ;
              v++;
            }
            var->defuz_arg = jfc_atof(v);
            if (var->defuz_arg < 0.0 || var->defuz_arg > 1.0)
              jfc_error(121, jfc_argv[v].arg, JFE_ERROR);
            v++;
          }
          break;
        case 5:                                 /* f_comp */
          m = jfc_operator_find(jfc_argv[v].arg);
          if (m == -1)
            return jfc_error(109, jfc_argv[v].arg, JFE_ERROR);
          var->f_comp = m;
          v++;
          break;
        case 6:                                 /* d_comp */
          m = jfc_afind(jfs_t_vcfs, JFS_VCF_COUNT, jfc_argv[v].arg);
          if (m == -1)
            return jfc_error(109, jfc_argv[v].arg, JFE_ERROR);
          var->d_comp = m;
          v++;
          break;
        case 7:                                 /* text */
          jfc_tcut(1, 60);
          strcpy(var->text, jfc_argv[v].arg);
          v++;
          break;
        case 8:                                 /* domain */
          jfc_tcut(v, 16);
          if ((domno = jfc_domain_find(jfc_argv[v].arg)) == -1)
            return jfc_error(110, jfc_argv[v].arg, JFE_ERROR);
          var->domain_no = domno;
          v++;
          break;
      }
    }
    if (var->default_type == JFS_VDT_NONE)
    { if (jfc_head->domains[var->domain_no].flags & JFS_DF_MINENTER)
        var->default_val = jfc_head->domains[var->domain_no].dmin;
      else
        var->default_val = 0.0;
    }
  }
  return 0;
}

static int jfc_hedge_ins(void)
{
  int v, m, ac, hno;
  struct jfr_hedge_desc *hedge;

  jfc_comment_id = -1;
  if (jfc_head->hedge_c >= 255)
    return jfc_error(129, jfc_empty, JFE_ERROR);
  if (jfc_targc < 2)
    return jfc_error(127, jfc_empty, JFE_ERROR);
  v = 0;
  jfc_tcut(0, 16);
  if ((hno = jfc_hedge_find(jfc_argv[v].arg)) != -1)
  { if (hno > 0)
      return jfc_error(128, jfc_argv[v].arg, JFE_ERROR);
    hedge = &(jfc_head->hedges[hno]);
    jfc_comment_id = hno;
  }
  else
  { hedge = &(jfc_head->hedges[jfc_head->hedge_c]);
    jfc_comment_id = jfc_head->hedge_c;
    jfc_head->hedge_c++;
  }
  if (jfc_pass == 1)
  { for (m = 0; m < jfc_argc; m++)
    { if (jfc_argv[m].type == 5)  /* : */
        jfc_head->limit_c++;
    }
  }
  else
  { if (hno == -1)
    { hedge->type = JFS_HT_NEGATE;
      hedge->flags = 0;
      hedge->limit_c = 0;
      hedge->f_limit_no = 0;
      hedge->comment_no = -1;
      hedge->hedge_arg = 0.0;
      strcpy(hedge->name, jfc_argv[v].arg);
    }
    v++;
    if (strcmp(jfc_argv[v].arg, jfc_t_type) == 0)
      v++;
    if ((m = jfc_afind(jfs_t_hts, JFS_HT_COUNT,
                       jfc_argv[v].arg)) != -1)
    { if (m == JFS_HT_NEGATE)
        ac = 0;
      else
        ac = 1;
      hedge->type = m;
      v++;
      if (ac == 1)
      { if (jfc_argv[v].type == 2)
        { hedge->flags += JFS_HF_IARG;
          v++;
        }
        hedge->hedge_arg = jfc_atof(v);
        if (hedge->type != JFS_HT_YNOT && hedge->hedge_arg <= 0.0)
          jfc_error(122, jfc_argv[v].arg, JFE_ERROR);
        if (hedge->type == JFS_HT_BELL && hedge->hedge_arg >= 1.0)
          jfc_error(123,jfc_argv[v].arg, JFE_ERROR);
      }
    }
    else
    { hedge->type = JFS_HT_LIMITS;   /* pl-function */
      hedge->f_limit_no = jfc_head->limit_c;
      hedge->limit_c = jfc_plf_insert(&v);
    }
  }
  return 0;
}

static void jfc_first_hedges(void)
{
  struct jfr_hedge_desc *hedge;

  hedge = &(jfc_head->hedges[JFS_HNO_NOT]);
  hedge->type = JFS_HT_NEGATE;
  hedge->flags = 0;
  hedge->limit_c = 0;
  hedge->f_limit_no = 0;
  hedge->comment_no = -1;
  hedge->hedge_arg = 0.0;
  strcpy(hedge->name, jfc_hn_not);
}

static int jfc_relation_ins(void)
{
  int v, m;
  struct jfr_relation_desc *relation;

  jfc_comment_id = -1;
  if (jfc_head->relation_c >= 255)
    return jfc_error(108, jfc_empty, JFE_ERROR);
  if (jfc_argc < 4)
    return jfc_error(127, jfc_empty, JFE_ERROR);
  v = 0;

  jfc_tcut(v, 16);
  if (jfc_relation_find(jfc_argv[v].arg) != -1)
    return jfc_error(128, jfc_argv[v].arg, JFE_ERROR);

  relation = &(jfc_head->relations[jfc_head->relation_c]);
  jfc_comment_id = jfc_head->relation_c;
  jfc_head->relation_c++;

  if (jfc_pass == 1)
  { for (m = 0; m < jfc_argc; m++)
    { if (jfc_argv[m].type == 5)  /* : */
        jfc_head->limit_c++;
    }
  }
  else
  { strcpy(relation->name, jfc_argv[v].arg);
    v++;
    relation->limit_c = 0;
    relation->f_limit_no = 0;
    relation->hedge_no = 0;
    relation->comment_no = -1;
    relation->flags = 0;
    relation->dummy = 0;

    while (v < jfc_argc)
    { if (strcmp(jfc_argv[v].arg, jfc_t_hedge) == 0)
      { v++;
        m = jfc_hedge_find(jfc_argv[v].arg);
        if (m == -1)
          return jfc_error(113, jfc_argv[v].arg, JFE_ERROR);
        relation->hedge_no = m;
        relation->flags += JFS_RF_HEDGE;
        v++;
      }
      else
      { relation->f_limit_no = jfc_head->limit_c;
        relation->limit_c = jfc_plf_insert(&v);
      }
    }
    if (relation->limit_c == 0)
      jfc_error(158, jfc_argv[v].arg, JFE_ERROR);
  }
  return 0;
}

static int jfc_operator_ins(void)
{
  int v, m, ac, opno, kw, hedge_entered;
  struct jfr_operator_desc *op;

  jfc_comment_id = -1;
  if (jfc_head->operator_c >= 255)
    return jfc_error(114, jfc_empty, JFE_ERROR);
  if (jfc_targc < 2)
    return jfc_error(127, jfc_empty, JFE_ERROR);
  v = 0;
  jfc_tcut(0, 16);
  if ((opno = jfc_operator_find(jfc_argv[v].arg)) != -1)
  { if (opno > 4)
      return jfc_error(128, jfc_argv[v].arg, JFE_ERROR);
    op = &(jfc_head->operators[opno]);
    jfc_comment_id = opno;
  }
  else
  { op = &(jfc_head->operators[jfc_head->operator_c]);
    jfc_comment_id = jfc_head->operator_c;
    jfc_head->operator_c++;
  }
  if (jfc_pass == 2)
  { if (opno == -1)
    { op->op_1 = op->op_2 = 0;
      op->hedge_mode = JFS_OHM_NONE;
      op->hedge_no = 0;
      op->flags = 0;
      op->op_arg = 0.0;
      op->precedence = 19;
      op->comment_no = -1;
      strcpy(op->name, jfc_argv[v].arg);
    }
    v++;
    hedge_entered = 0;

    while (v < jfc_argc)
    { kw = jfc_table_find(&ac, jfc_t_opargs, jfc_argv[v].arg);
      if (kw == -1)
        kw = 2; /* type */
      else
        v++;
      switch (kw)
      { case 0:  /* hedge */
          if ((m = jfc_hedge_find(jfc_argv[v].arg)) == -1)
            return jfc_error(113, jfc_argv[v].arg, JFE_ERROR);
          op->hedge_no = m;
          hedge_entered = 1;
          v++;
          /* op->flags += JFS_OF_HEDGE; */
          break;
        case 1: /* precedence */
          m = atoi(jfc_argv[v].arg);   /* Check ? */
          if (m < 1 || m > 255)
            return jfc_error(124, jfc_argv[v].arg, JFE_ERROR);
          op->precedence = m;
          v++;
          break;
        case 2:   /* operator type */
          m = jfc_table_find(&ac, jfc_t_operators, jfc_argv[v].arg);
          if (m == -1)
            return jfc_error(125, jfc_argv[v].arg, JFE_ERROR);
          op->op_1 = m;
          op->op_2 = m;
          v++;
          if (ac == 1)  /* single-operator with argument */
          { if (jfc_argv[v].type == 2)
            { op->flags += JFS_OF_IARG;
              v++;
            }
            op->op_arg = jfc_atof(v);
            if (op->op_arg <= 0.0)
            { jfc_error(122, jfc_argv[v].arg, JFE_ERROR);
              op->op_arg = 1.0;
            }
            v++;
          }
          else
          { m = jfc_table_find(&ac, jfc_t_operators, jfc_argv[v].arg);
            if (m >= 0)
            { if (ac == 1)
                return jfc_error(115, jfc_argv[v].arg, JFE_ERROR);
              op->op_2 = m;
              v++;
              if (jfc_argv[v].type == 2)
              { op->flags += JFS_OF_IARG;
                v++;
              }
              op->op_arg = jfc_atof(v);
              if (op->op_arg < 0.0 || op->op_arg > 1.0)
                jfc_error(121, jfc_argv[v].arg, JFE_ERROR);
              v++;
            }
          }
          break;
        case 3:    /* hedge-mode */
          m = jfst_ohmode_find(jfc_argv[v].arg);
          if (m == -1)
            return jfc_error(111, jfc_argv[v].arg, JFE_ERROR);
          op->hedge_mode = m;
          v++;
          break;
      }
    }
    if (op->op_1 == 0)
      jfc_error(159, jfc_empty, JFE_ERROR);
    if (hedge_entered == 0)
      op->hedge_mode = JFS_OHM_NONE;
    else
    { if (op->hedge_mode == JFS_OHM_NONE)
        op->hedge_mode = JFS_OHM_POST;
    }
  }
  return 0;
}

static void jfc_first_operators(void)
{
  struct jfr_operator_desc *op;

  op = &(jfc_head->operators[JFS_ONO_CASEOP]);
  strcpy(op->name, jfc_on_caseop);
  op->op_1 = op->op_2 = JFS_FOP_PROD;
  op->flags = 0;
  op->precedence = 20;
  op->hedge_mode = JFS_OHM_NONE;
  op->hedge_no = 0;
  op->comment_no = -1;

  op = &(jfc_head->operators[JFS_ONO_WEIGHTOP]);
  strcpy(op->name, jfc_on_weightop);
  op->op_1 = op->op_2 = JFS_FOP_PROD    ;
  op->flags = 0;
  op->precedence = 20;
  op->hedge_mode = JFS_OHM_NONE;
  op->hedge_no = 0;
  op->comment_no = -1;

  op = &(jfc_head->operators[JFS_ONO_AND]);
  strcpy(op->name, jfc_on_and);
  op->op_1 = op->op_2 = JFS_FOP_MIN;
  op->flags = 4;   /* ?? precedence */
  op->precedence = 30;
  op->hedge_mode = JFS_OHM_NONE;
  op->hedge_no = 0;
  op->comment_no = -1;

  op = &(jfc_head->operators[JFS_ONO_OR]);
  strcpy(op->name, jfc_on_or);
  op->op_1 = op->op_2 = JFS_FOP_MAX;
  op->flags = 4;  /* ?? precedence */
  op->precedence = 20;
  op->hedge_mode = JFS_OHM_NONE;
  op->hedge_no = 0;
  op->comment_no = -1;

  op = &(jfc_head->operators[JFS_ONO_WHILEOP]);
  strcpy(op->name, jfc_on_whileop);
  op->op_1 = op->op_2 = JFS_FOP_PROD;
  op->flags = 0;
  op->precedence = 20;
  op->hedge_mode = JFS_OHM_NONE;
  op->hedge_no = 0;
  op->comment_no = -1;
}

static int jfc_array_ins(void)
{
  int v, kw, ac;
  struct jfr_array_desc *array;

  jfc_comment_id = -1;
  if (jfc_head->array_c >= 255)
    return jfc_error(147, jfc_empty, JFE_ERROR);
  v = 0;

  jfc_tcut(v, 16);
  if (jfc_array_find(jfc_argv[v].arg) != -1)
    return jfc_error(128, jfc_argv[v].arg, JFE_ERROR);
  if (jfc_var_find(jfc_argv[v].arg) != -1)
    return jfc_error(128, jfc_argv[v].arg, JFE_ERROR);

  array = &(jfc_head->arrays[jfc_head->array_c]);
  jfc_comment_id = jfc_head->array_c;
  jfc_head->array_c++;

  if (jfc_pass == 2)
  { strcpy(array->name, jfc_argv[v].arg);
    v++;
    array->array_c = 64;
    array->comment_no = -1;
    array->f_array_id = 0;

    while (v < jfc_argc)
    { kw = jfc_table_find(&ac, jfc_t_arrargs, jfc_argv[v].arg);
      if (kw == -1)
      { if (jfc_argv[v].type == 6 && jfc_argv[v + 2].type == 7) /* [,] */
        { v++;
          kw = 0;
        }
      }
      else
        v++;
      switch (kw)
      { case 0:   /* size */
          array->array_c = atoi(jfc_argv[v].arg);
          if (array->array_c <= 2)
          { array->array_c = 2;
            jfc_error(148, jfc_argv[v].arg, JFE_ERROR);
          }
          v++;
          if (jfc_argv[v].type == 7)
            v++;
          break;
        default:
          jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
          v++;
          break;
      }
    }
  }
  return 0;
}


static int jfc_progline_ins(void)
{
  jfc_head->program_code = (unsigned char *) jfc_ff_jfr;
  return 0;
}


static void jfc_var_def_handle(void)
{
   int m, ano;
   struct jfr_var_desc *var;
   struct jfr_domain_desc *dom;

   for (m = 0; m < jfc_head->var_c; m++)
   { var = &(jfc_head->vars[m]);
     if (var->default_type == JFS_VDT_ADJECTIV)
     { ano = jfc_adjectiv_find(m, jfc_def_names[var->default_no].name);
       if (ano >= 0)
       { if (var->fzvar_c == 0)
         { dom = &(jfc_head->domains[var->domain_no]);
           var->default_no = (unsigned char) (ano - dom->f_adjectiv_no);
         }
         else
           var->default_no = (unsigned char) (ano - var->f_adjectiv_no);
         var->default_val = jfc_head->adjectives[ano].center;
       }
       else
         jfc_error(152, var->name, JFE_ERROR);
     }
   }
}

static void jfc_init(void)
{
  int m;

  for (m = 0; m < 256; m++)
  { if (isspace(m))
      jfc_ttypes[m] = 0;
    else
      jfc_ttypes[m] = 1;
  }
  jfc_ttypes['%']  = 2;
  jfc_ttypes[';']  = 3;
  jfc_ttypes['\0'] = 3;
  jfc_ttypes['"']  = 4;
  jfc_ttypes[':']  = 5;
  jfc_ttypes['(']  = jfc_ttypes['['] = 6;
  jfc_ttypes[')']  = jfc_ttypes[']'] = 7;
  jfc_ttypes['\n'] = 8;
  jfc_ttypes['#']  = 9;
  jfc_ttypes['+']  = jfc_ttypes['*'] = jfc_ttypes['/'] = 10;
  jfc_ttypes['>']  = jfc_ttypes['<'] = jfc_ttypes['='] = 10;
  jfc_ttypes['!']  = 10;
  jfc_ttypes[',']  = 12;
  jfc_ttypes['|']  = 0;

  jfc_err_count = 0;
  jfc_line_count = 0;
  jfc_ff_def_names = 0;

  if (jfc_pass == 1)
    jfc_ff_jfr = (unsigned char *) jfc_memory;
  jfc_head->title[0] = '\0';
  jfc_head->comment_no = -1;
  jfc_head->vbytes = 1;          /* NB ?? */
  jfc_head->a_size  = 0;
  jfc_head->adjectiv_c  = 0;
  jfc_head->ivar_c   = 0;
  jfc_head->ovar_c   = 0;
  jfc_head->fzvar_c  = 0;
  jfc_head->array_c  = 0;
  jfc_head->limit_c  = 0;
  jfc_head->relation_c = 0;
  jfc_head->comment_c  = 0;
  jfc_head->function_c = 0;
  jfc_head->funcarg_c  = 0;
  jfc_head->domain_c = 2;
  jfc_head->lvar_c   = 0;
  jfc_head->var_c    = 0;
  jfc_head->hedge_c  = 1;
  jfc_head->operator_c = 5;
  jfc_head->com_block_c = 0;
  jfc_head->funccode_c = 0;
  jfc_head->program_c = 0;

  if (jfc_pass == 1)
  { jfc_head->comments   = (struct jfr_comment_desc *) jfc_ff_jfr;
    jfc_head->domains    = (struct jfr_domain_desc *) jfc_ff_jfr;
    jfc_head->adjectives = (struct jfr_adjectiv_desc *) jfc_ff_jfr;
    jfc_head->vars       = (struct jfr_var_desc *) jfc_ff_jfr;
    jfc_head->arrays     = (struct jfr_array_desc *) jfc_ff_jfr;
    jfc_head->fzvars     = (struct jfr_fzvar_desc *) jfc_ff_jfr;
    jfc_head->limits     = (struct jfr_limit_desc *) jfc_ff_jfr;
    jfc_head->hedges     = (struct jfr_hedge_desc *) jfc_ff_jfr;
    jfc_head->relations  = (struct jfr_relation_desc *) jfc_ff_jfr;
    jfc_head->operators  = (struct jfr_operator_desc *) jfc_ff_jfr;
    jfc_head->program_code  = jfc_ff_jfr;
    jfc_head->comment_block = jfc_ff_jfr;
  }
  jfc_ff_buf = 0;
  jfc_cu_buf = 0;
  jfc_cu_source = 0; /* NEW 00114 */
  jfc_eof    = 0;

  jfc_argc = 0;
}

static int jfc_comp(void)
{
  int m, cstate, comment_state, k, trueline, slut;
  unsigned char *adr;

  cstate = 0;
  comment_state = 0;
  slut = 0;
  while (slut == 0 && jfc_getline() != -1)
  { m = jfc_table_find(&k, jfc_t_blocks, jfc_argv[0].arg);
    if (m != -1)
    { if (m == 10) /* rules */
        m = 8;   /* program */
      comment_state = 0;
      cstate = m;  /* do nothing */

      if (cstate == 8)  /* program */
      { jfc_head->program_code = (unsigned char *) jfc_ff_jfr;
        if (jfc_pass == 2)
        { while (jfc_getchar() == 0)
          { adr = jfc_malloc(1);
            if (jfc_gl_error_mode == JFE_FATAL)
              return -1;
            *adr = jfc_cu_source;
            jfc_head->program_c++;
          }
        }
        slut = 1;
        jfc_targc = 0;
      }
    }
    else                   /* not keyword (m == -1) */
    { trueline = 1;
      if (jfc_argv[0].type == 13) /* comment */
      { trueline = 0;
        if (comment_state == 0)
          jfc_error(144, jfc_argv[1].arg, JFE_WARNING);
        else
        { jfc_comment_handle(cstate);
          comment_state = 0;
        }
      }
      if (jfc_argc == 0)
        trueline = 0;
      if (trueline == 1)
      { switch (cstate)
        { case 0:                                           /* title */
            if (jfc_targc != 1)
              jfc_error(127, jfc_empty, JFE_ERROR);
            else
            { jfc_tcut(0, 60);
              strcpy(jfc_head->title, jfc_argv[0].arg);
            }
            break;
          case 1:                                          /* domains */
            jfc_domain_ins();
            break;
          case 2:                                          /* adjectives */
            jfc_adjectiv_ins();
            break;
          case 3:                                           /* ivars */
            jfc_var_ins(0);
             break;
          case 4:                                           /* ovars */
            jfc_var_ins(1);
            break;
          case 5:                                           /* lvars */
            jfc_var_ins(2);
            break;
          case 6:                                           /* hedges */
            jfc_hedge_ins();
            break;
          case 7:                                           /* relations */
            jfc_relation_ins();
            break;
          case 8:    /* Aldrig ?? */                        /* rules */
            jfc_progline_ins();
            break;
          case 9:                                           /* synonyms */
            jfc_syn_ins();
            break;
          case 11:                                          /* operators */
            jfc_operator_ins();
            break;
          case 12:                                          /* arrays */
            jfc_array_ins();
            break;
        }
      }
      comment_state = 1;
    }
  } /* while */

  if (jfc_targc != 0)
    jfc_error(11, jfc_empty, JFE_FATAL);

  return 0;
}

static int jfc_close(void)
{
  if (jfc_source != NULL)
    fclose(jfc_source);
  if (jfc_memory != NULL)
    free(jfc_memory);
  if (jfc_argline != NULL)
    free(jfc_argline);
  if (jfc_argv != NULL)
    free(jfc_argv);
  return 1;
}

static void jfc_mem_set(void)
{
  jfc_head->domains = (struct jfr_domain_desc *)
         jfc_malloc(jfc_head->domain_c * JFR_DOMAIN_SIZE);
  jfc_head->adjectives = (struct jfr_adjectiv_desc *)
   jfc_malloc(jfc_head->adjectiv_c * JFR_ADJECTIV_SIZE);
  jfc_head->vars = (struct jfr_var_desc *)
     jfc_malloc(jfc_head->var_c * JFR_VAR_SIZE);
  jfc_head->arrays = (struct jfr_array_desc *)
      jfc_malloc(jfc_head->array_c * JFR_ARRAY_SIZE);
  jfc_head->limits = (struct jfr_limit_desc *)
      jfc_malloc(jfc_head->limit_c * JFR_LIMIT_SIZE);
  jfc_head->hedges = (struct jfr_hedge_desc *)
      jfc_malloc(jfc_head->hedge_c * JFR_HEDGE_SIZE);
  jfc_head->relations = (struct jfr_relation_desc *)
         jfc_malloc(jfc_head->relation_c * JFR_RELATION_SIZE);
  jfc_head->operators = (struct jfr_operator_desc *)
         jfc_malloc(jfc_head->operator_c * JFR_OPERATOR_SIZE);
  jfc_head->comments = (struct jfr_comment_desc *)
         jfc_malloc(jfc_head->comment_c * JFR_COMMENT_SIZE);
  jfc_head->comment_block = jfc_malloc(jfc_head->com_block_c);

  if (jfc_gl_error_mode == JFE_FATAL)
    return;

  jfc_first_domains();
  jfc_first_hedges();
  jfc_head->f_lvar_no = 0;
  jfc_head->f_ivar_no = jfc_head->lvar_c;
  jfc_head->f_ovar_no = jfc_head->f_ivar_no + jfc_head->ivar_c;
  jfc_first_operators();
}

int jfs2w_convert(char *de_fname, char *so_fname,
                  char *er_fname, int er_append,
                  int maxargline, int margc,
                  int err_message_mode)
{
  long ms;
  int res, save_errors;

  jfc_gl_error_mode = JFE_NONE;
  if (er_fname == NULL || strlen(er_fname) == 0)
    jfc_erfile = stdout;
  else
  { if (er_append == 0)
      jfc_erfile = fopen(er_fname, "w");
    else
      jfc_erfile = fopen(er_fname, "a");
    if (jfc_erfile == NULL)
    { jfc_erfile = stdout;
      { jfc_error(1, er_fname, JFE_FATAL);
        return jfc_close();
      }
    }
  }
  jfc_err_message_mode = err_message_mode;

  ms = 64000;
  if ((jfc_memory = calloc(ms, 1)) == NULL)
  { ms = 32000;
    if ((jfc_memory = calloc(ms, 1)) == NULL)
    { jfc_error(6, jfc_empty, JFE_FATAL);
      return jfc_close();
    }
  }
  jfc_ff_memory = ((unsigned char *) jfc_memory) + ms;
  jfc_head = &jfc_ihead;

  if (maxargline != 0)
    jfc_maxargline = maxargline;
  if ((jfc_argline = (char *) malloc(jfc_maxargline + 10)) == NULL)
  { jfc_error(6, jfc_empty, JFE_FATAL);
    return jfc_close();
  }

  if ((jfc_source = fopen(so_fname, "r")) == NULL)
  { jfc_error(1, so_fname, JFE_FATAL);
    return jfc_close();
  }

  if (margc != 0)
    jfc_margc = margc;
  if ((jfc_argv = (struct jfc_argv_desc *)
   malloc(sizeof(struct jfc_argv_desc) * jfc_margc))
      == NULL)
  { jfc_error(6, jfc_empty, JFE_FATAL);
    return jfc_close();
  }

  if (jfc_gl_error_mode != JFE_FATAL)
  { jfc_pass = 1;
    jfc_ff_synonyms = 0;
    jfc_synonyms = (struct jfw_synonym_desc *) jfc_memory;
    jfc_init();
    jfc_comp();
  }

  if (jfc_gl_error_mode != JFE_FATAL)
  { jfc_pass = 2;
    jfc_ff_synonyms = 0;
    jfc_synonyms = (struct jfw_synonym_desc *) jfc_memory;
    rewind(jfc_source);
    jfc_mem_set();
    jfc_init();
    jfc_comp();
    if (jfc_gl_error_mode != JFE_FATAL)
      jfc_var_def_handle();
  }

  if (jfc_gl_error_mode == JFE_FATAL)
    return jfc_close();

  fclose(jfc_source);
  jfc_source = NULL;

  save_errors = 0;
  if (jfc_err_count == 0)
  { jfc_save(de_fname);
    if (jfc_err_count != 0)
      save_errors = 1;
  }
  if (jfc_memory != NULL)
    free(jfc_memory);
  if (jfc_argline != NULL)
    free(jfc_argline);
  if (jfc_argv != NULL)
    free(jfc_argv);

  if (jfc_erfile != stdout)
  { fclose(jfc_erfile);
    jfc_erfile = NULL;
  }

  res = 0;
  if (jfc_err_count != 0)
  { if (save_errors == 0)
      res = 1;
    else
      res = 2;
  }
  return res;
}

static int jfc_save(char *fname)
{
  struct jfw_head_desc jfw_head;
  char ctekst[4] = "jfw";

  strcpy(jfw_head.check, ctekst);
  strcpy(jfc_head->check, ctekst);
  jfw_head.version = jfc_head->version = 200;

  jfc_head->a_size =
     JFW_SYNONYM_SIZE * jfc_ff_synonyms
   + JFR_COMMENT_SIZE * jfc_head->comment_c
   + JFR_DOMAIN_SIZE * jfc_head->domain_c
   + JFR_ADJECTIV_SIZE * jfc_head->adjectiv_c
   + JFR_VAR_SIZE * jfc_head->var_c
   + JFR_ARRAY_SIZE * jfc_head->array_c
   + JFR_LIMIT_SIZE * jfc_head->limit_c
   + JFR_HEDGE_SIZE * jfc_head->hedge_c
   + JFR_RELATION_SIZE * jfc_head->relation_c
   + JFR_OPERATOR_SIZE * jfc_head->operator_c
   + jfc_head->com_block_c
   + jfc_head->program_c;
  strcpy(jfw_head.title, jfc_head->title);
  jfw_head.comment_no = jfc_head->comment_no;
  jfw_head.f_prog_line_no = jfc_line_count;
  /* jfw_head.a_size     = jfc_head->a_size; */
  jfw_head.synonym_c  = jfc_ff_synonyms;
  jfw_head.comment_c  = jfc_head->comment_c;
  jfw_head.domain_c   = jfc_head->domain_c;
  jfw_head.adjectiv_c = jfc_head->adjectiv_c;
  jfw_head.ivar_c     = jfc_head->ivar_c;
  jfw_head.ovar_c     = jfc_head->ovar_c;
  jfw_head.lvar_c     = jfc_head->lvar_c;
  jfw_head.var_c      = jfc_head->var_c;
  jfw_head.array_c    = jfc_head->array_c;
  jfw_head.limit_c    = jfc_head->limit_c;
  jfw_head.hedge_c    = jfc_head->hedge_c;
  jfw_head.relation_c = jfc_head->relation_c;
  jfw_head.operator_c = jfc_head->operator_c;
  jfw_head.com_block_c= jfc_head->com_block_c;
  /* jfw_head.program_c  = jfc_head->program_c; */
  jfw_head.arg_1_c    = jfc_head->arg_1_c = 0;
  jfw_head.arg_2_c    = jfc_head->arg_2_c = 0;

  if ((jfc_dest = fopen(fname, "wb")) == NULL)
    return jfc_error(1, fname, JFE_FATAL);

  if (fwrite((char *) &jfw_head, JFW_HEAD_SIZE, 1, jfc_dest) != 1)
    return jfc_error(3, fname, JFE_FATAL);

  if (fwrite((char *) jfc_synonyms,
        jfc_head->a_size, 1, jfc_dest) != 1)
      return jfc_error(3, jfc_empty, JFE_FATAL);

  fclose(jfc_dest);
  jfc_dest = NULL;
  return 0;
}




