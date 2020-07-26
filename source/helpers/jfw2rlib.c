  /************************************************************************/
  /*                                                                      */
  /* jfw2rlib.cpp Version 2.03  Copyright (c) 1999-2000 Jan E. Mortensen  */
  /*                                                                      */
  /* JFS Converter. Converts a  JFW-file to a  JFR-file.                  */
  /*                                                                      */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                   */
  /*    Lollandsvej 35 3.tv.                                              */
  /*    DK-2000 Frederiksberg                                             */
  /*    Denmark                                                           */
  /*                                                                      */
  /************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "jfr_gen.h"
#include "jfs_cons.h"
#include "jfs_text.h"

/********************************************************************/
/* Variables for decoding if-statements                             */
/********************************************************************/

#define SYMB_BPAR  0
#define SYMB_EPAR  1
#define SYMB_COMMA 2
#define SYMB_FUNC  3
#define SYMB_PFUNC 4
#define SYMB_END   5
#define SYMB_OP    6


static                  /*ip\stack: (    )   ,  func func( end    op  */
int jfc_tabel[7][7] = { /* (   */  {2,   5,  5,   6,   2,    2,    2},
                        /* )   */  {4,   5,  5,   3,   7,    5,    3},
                        /* ,   */  {5,   5,  5,   3,   8,    5,    3},
                        /* func*/  {2,   5,  5,   2,   2,    2,    2},
                        /*func(*/  {5,   5,  5,   5,   5,    5,    5},
                        /* end */  {5,   5,  5,   3,   5,    9,    3},
                        /* op  */  {2,   5,  5,   3,   2,    2,   10}  };

   /* aktioner i tabellen:                   */
   /* 1: ip   -> op (aldrig),                */
   /* 2: ip   -> stak,                       */
   /* 3: stak -> op,                         */
   /* 4: ip, stack -> NULL,                  */
   /* 5: fejl,                               */
   /* 6: ip->null, stak.s_type = pfunc,      */
   /* 7: stak -> op, ip -> null,             */
   /* 8: stak.arg++, ip -> null,             */
   /* 9: slut,                               */
   /*10: if (ip.prior > stak.prior)          */
   /*      ip -> stak;                       */
   /*    else stak -> op.                    */


struct jfc_stack_desc
 { unsigned char s_type; /* symb_type SYM_BPAR.. */
   unsigned char a_type; /* JFR-commando.        */
   unsigned char arg;
   unsigned char argc;
 };

static int jfc_maxstack = 100;
static struct jfc_stack_desc *jfc_stack = NULL;
static int  jfc_ff_stack;

struct jfc_iet_desc   /* record til stack af switch/default/while    */
       { unsigned char type;  /* 1: switch;,                         */
                              /* 2: switch af ter default,           */
                              /* 3: while,                           */
                              /* 4: switch <var>.                    */
                              /* 5: function.                        */
         unsigned char arg;   /* var-no if switch <var>,             */
                              /* function-no (if function).          */
       };

#define JFC_IET_SIZE 64
static struct jfc_iet_desc jfc_iet_stack[JFC_IET_SIZE];

static int jfc_ff_iet;

static int jfc_func_status = -1;    /* -2: i main-program            */
                                    /* -1: klar til                  */
                                    /*     function/procedure/main.  */
                                    /* >= 0: i function nr           */
                                    /*       jfc_func_status.        */

static int jfc_func_def = 0;        /* = 1: just after function-def  */
                                    /* (function-comment here).      */

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
static int  jfc_cu_source;        /* current char in sourcen.         */

static int  jfc_maxargline = 1024;
static char *jfc_argline = NULL;  /* tokens i current statement.      */
static int  jfc_argl_ff;          /* first free in argl_ff.           */

struct jfc_argv_desc
             {
                 char *arg;    /* pointer to jfc_argline */
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

                   char qoute; /* 1: token i "" */
             };

static int jfc_margc = 256;
static struct jfc_argv_desc *jfc_argv = NULL;/* words in current statement.*/
static int jfc_argc;  /* number of words in current statement. */
static int jfc_targc; /* jfc_argc - %'er.              */

static int jfc_line_count;  /* numer of lines (not number of statemets). */

static struct jfw_synonym_desc *jfc_synonyms = NULL;

/************************************************************************/
/*  Variables til afkodning af keywords                                 */
/************************************************************************/

static const char jfc_t_then[]      = "then";
static const char jfc_t_true[]      = "true";
static const char jfc_t_false[]     = "false";
static const char jfc_t_eq[]        = "=";
static const char jfc_t_between[]   = "between";
static const char jfc_t_in[]        = "in";
static const char jfc_t_and[]       = "and";
static const char jfc_t_is[]        = "is";
static const char jfc_t_increase[]  = "increase";
static const char jfc_t_decrease[]  = "decrease";
static const char jfc_t_with[]      = "with";
static const char jfc_t_function[]  = "function";
static const char jfc_t_procedure[] = "procedure";
static const char jfc_t_clear[]     = "clear";
static const char jfc_t_return[]    = "return";
static const char jfc_t_extern[]    = "extern";
static const char jfc_t_call[]      = "call";

struct jfc_t_table_desc
   { const char *name;
     int  argc;
   };


static struct jfc_t_table_desc jfc_t_statements[] =
    { {"if",           1},   /*  0 */
      {"switch",       1},   /*  1 */
      {"end",          1},   /*  2 */
      {"case",         1},   /*  3 */
      {"default",      1},   /*  4 */
      {"while",        1},   /*  5 */
      {"ifw",          1},   /*  6 */
      {"wset",         1},   /*  7 */
      {" ",           -1}
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
      {" ", -1          }
    };

static struct jfc_t_table_desc jfc_t_vfuncs[] =
    { {"dnormal",      1},  /*  0 */
      {"m_fzvar",      1},  /*  1 */
      {"s_fzvar",      1},  /*  2 */
      {"default",      1},  /*  3 */
      {"confidence",   1},  /*  4 */
      {" ",           -1}
    };


struct jfc_t_func_desc
  { const char *name;
    unsigned char a_type;
    unsigned char arg;
    signed char argc;
  };

struct jfc_t_dop_desc
  { const char *name;
    unsigned char a_type;
    unsigned char arg;
    signed char precedence;
  };

static struct jfc_t_func_desc jfc_t_funcs[] =
  { {"cos",  JFR_OP_SFUNC, 0, 1},
    {"sin",  JFR_OP_SFUNC, 1, 1},
    {"tan",  JFR_OP_SFUNC, 2, 1},
    {"acos", JFR_OP_SFUNC, 3, 1},
    {"asin", JFR_OP_SFUNC, 4, 1},
    {"atan", JFR_OP_SFUNC, 5, 1},
    {"log",  JFR_OP_SFUNC, 6, 1},
    {"fabs", JFR_OP_SFUNC, 7, 1},
    {"floor",JFR_OP_SFUNC, 8, 1},
    {"ceil", JFR_OP_SFUNC, 9, 1},
    {"-",    JFR_OP_SFUNC,10, 1},
    {"random",JFR_OP_SFUNC,11,1},
    {"sqr",  JFR_OP_SFUNC, 12,1},
    {"sqrt", JFR_OP_SFUNC, 13,1},
    {"wget", JFR_OP_SFUNC, 14,1},
    {"pow",  JFR_OP_DFUNC, 4, 2},
    {"min",  JFR_OP_DFUNC, 5, 2},
    {"max",  JFR_OP_DFUNC, 6, 2},
    {"cut",  JFR_OP_DFUNC, 7, 2},
    {"iif",  JFR_OP_IIF,   0, 3},
    {" ",    0,            0,-1}
  };

static struct jfc_t_dop_desc jfc_t_dops[] =
 { {"+",     JFR_OP_DFUNC, 0, 50},
   {"-",     JFR_OP_DFUNC, 1, 50},  /* NB: special */
   {"*",     JFR_OP_DFUNC, 2, 60},
   {"/",     JFR_OP_DFUNC, 3, 60},
   {"=",     JFR_OP_DFUNC,10, 40},
   {"==",    JFR_OP_DFUNC,10, 40},
   {"!=",    JFR_OP_DFUNC,11, 40},
   {"<>",    JFR_OP_DFUNC,11, 40},
   {">",     JFR_OP_DFUNC,12, 40},
   {">=",    JFR_OP_DFUNC,13, 40},
   {"<",     JFR_OP_DFUNC,14, 40},
   {"<=",    JFR_OP_DFUNC,15, 40},
   {" ",     0,            0, -1}
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
static struct jfw_head_desc jfw_head;

static int jfc_err_count;      /* antal fejl under oversaettelsen.    */

static char jfc_empty[] = " "; /* tom streng til fejlmeddellelser.    */

static int jfc_gl_error_mode;

static const char *jfc_err_modes[] =
   { "NONE",
     "WARNING",
     "ERROR",
     "FATAL ERROR"
   };

static int jfc_err_line_no = -1;
static int jfc_err_line_mode = 1; /* 0: start with line 1,              */
                                  /* 1: start with line jfw_head.f_line.*/

static int jfc_err_message_mode; /* 0: normal error-messages,           */
                                 /* 1: compact error-messages.          */

struct jfc_err_desc {
	int eno;
	const char *text;
	};

struct jfc_err_desc jfc_err_texts[] =
{  {0, "  "},
  {  1, "Error opening file:"},
  {  2, "Error reading from file:"},
  {  3, "Error writing to file:"},
  {  4, "Not an jfr-file:"},
  {  5, "Wrong version."},
  {  6, "Cannot allocate memory to:"},
  {  7, "Syntax error in:"},
  {  8, "Out of memory"},
  {  9, "Illegal number:"},
  {101, "Too many arguments in statement (max 255)."},
  {102, "Expresion to complex."},
  {103, "Adjectiv value out of domain range:"},
  {105, "Too many words in sentence."},
  {116, "Undefined adjective:"},
  {118, "Undefined variable: "},
  {123, "Semicolon (;) or colon (:) expected."},
  {127, "Wrong number off arguments."},
  {128, "Multiple declaration of:"},
  {130, "Unknown keyword:"},
  {133, "Too many errors."},
  {134, "Text or number expected."},
  {135, "Values in wrong order for:"},
  {136, "Too many nested switch-statements (max 64)"},
  {137, "Default-stmt without switch-stmt."},
  {138, "Case-statement without switch-stmt."},
  {139, "End-statement without switch-statement."},
  {140, "Missing end-statement(s)."},
  {142, "Sentence to long."},
  {144, "Illegal placed comment. Comment ignored."},
  {149, "Too many (max 128) adjectives bound to:"},
  {150, "keyword 'with' expected:"},
  {153, "Illegal placed function/procedure."},
  {154, "Return statement outsize function."},
  {9999, "Unknown error"}
};

/* hjaelpe - funktioner */

  static int jfc_error(int eno, char *name, int mode);
  static int jfw_check(void *head);
  static int jfc_tcut(int argno, int alen);
  static float jfc_atof(int v);
     static int jf_atof(float *d, char *a);
  static int jfc_table_find(int *argc, struct jfc_t_table_desc *tab,
                            char *arg);
  static int jfc_getline(void);
    static void jfc_skipline(void);
    static int jfc_gettoken(void);
      static int jfc_getchar(void);

  static int jfc_adjectiv_find(int varno, char *name);
  static int jfc_array_find(char *arrname);
  static int jfc_var_find(char *name);
  static int jfc_fzvar_find(int var_no, char *adjname);
  static int jfc_hedge_find(char *name);
  static int jfc_operator_find(char *name);
  static int jfc_relation_find(char *name);
  static int jfc_ufunc_find(char *name);
  static int jfc_ufuncarg_find(int func_no, char *name);

  static int jfc_dops_find(int v);
  static int jfc_func_find(char *name);

/* jfc_comp */
  static int jfc_comp(void);
    static void jfc_comment_handle(void);
    static int jfc_init(void);
    static int jfc_rule_ins(void);
      static int jfc_cadd(unsigned char c);
      static int jfc_progadd(int no);
      static int jfc_expr_ins(int adr, int eq_end);
      static int jfc_weight_ins(int op, float val);
      static int jfc_tab_handle(int s_type, int a_type,
                                int arg, int argc);
      static int jfc_function_ins(void);
    static int jfc_progline_ins(void);
    static int jfc_end_ins(void);
    static void jfc_limits_handle(void);
       static void jfc_plf_handle(int start_limit, int lcount, char *txt);
    static void jfc_adjectives_handle(void);
      static int jfc_adj_centtrap_handle(void);
    static int jfc_vars_handle(void);
    static void jfc_arrayval_handle(void);

  static void jfc_var_mem_alloc(void);
  static void jfc_val_init(void);

  static int jfc_close(void);
static int jfc_save(char *fname);

/*************************************************************************/
/* Generelle hjaelpefunktoner                                            */
/*************************************************************************/

static int jfc_error(int eno, char *name, int mode)
{
  int m, v;

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
                eno, jfc_line_count,
      jfc_err_texts[v].text, name);
      if (mode != JFE_WARNING)
      { jfc_err_count++;
        m = -1;
      }
      if (jfc_err_message_mode == 0)
        fprintf(jfc_erfile, "in line %d:\n%s\n",
      jfc_line_count, jfc_sent);
    }
    jfc_err_line_no = jfc_line_count;
    if (mode == JFE_FATAL)
    { fprintf(jfc_erfile, "\n*** COMPILATION ABORTED! ***\n");
      jfc_gl_error_mode = JFE_FATAL;
    }
  }
  return m;
}

static int jfw_check(void *head)
{
  struct jfw_head_desc *jfw_head;

  jfw_head = (struct jfw_head_desc *) head;
  if (jfw_head->check[0] != 'j' ||
      jfw_head->check[1] != 'f' ||
      jfw_head->check[2] != 'w')
    return 4;
  if (jfw_head->version / 100 != 2)
    return 5;
  return 0;
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

static int jfc_array_find(char *arrname)
{
  int m;

  for (m = 0; m < jfc_head->array_c; m++)
  { if (strcmp(arrname, jfc_head->arrays[m].name) == 0)
      return m;
  }
  return -1;
}

static int jfc_adjectiv_find(int var_no, char *adjname)
{
  int m, adjno;
  struct jfr_var_desc *v;

  v = &(jfc_head->vars[var_no]);
  for (m = 0; m < v->fzvar_c; m++)
  { adjno = v->f_adjectiv_no + m;
    if (strcmp(adjname, jfc_head->adjectives[adjno].name) == 0)
      return adjno;
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

static int jfc_fzvar_find(int var_no, char *adjname)
{
  int m;
  struct jfr_var_desc *var;

  var = &(jfc_head->vars[var_no]);
  m = jfc_adjectiv_find(var_no, adjname);
  if (m >= 0)
    m = m - var->f_adjectiv_no + var->f_fzvar_no;
  return m;
}

static int jfc_hedge_find(char *name)
{
  int m;

  for (m = 0; m < jfc_head->hedge_c; m++)
  { if (strcmp(jfc_head->hedges[m].name, name) == 0)
      return m;
  }
  return -1;
}

static int jfc_operator_find(char *name)
{
  int m;

  for (m = 0; m < jfc_head->operator_c; m++)
  { if (strcmp(jfc_head->operators[m].name, name) == 0)
      return m;
  }
  return -1;
}

static int jfc_relation_find(char *name)
{
  int m;

  for (m = 0; m < jfc_head->relation_c; m++)
  { if (strcmp(jfc_head->relations[m].name, name) == 0)
      return m;
  }
  return -1;
}

static int jfc_ufunc_find(char *name)  /* user-function-find */
{
  int m;

  if (jfc_pass == 2)
  for (m = 0; m < jfc_head->function_c; m++)
  { if (strcmp(jfc_head->functions[m].name, name) == 0)
      return m;
  }
  return -1;
}

static int jfc_ufuncarg_find(int func_no, char *name)
{
  int m;
  struct jfr_function_desc *func;

  if (func_no < 0)
    return -1;
  func = &(jfc_head->functions[func_no]);
  if (jfc_pass == 2)
  for (m = 0; m < func->arg_c; m++)
  { if (strcmp(jfc_head->func_args[func->f_arg_no + m].name, name) == 0)
      return m;
  }
  return -1;
}

static int jfc_dops_find(int v)
{
  int m;

  if (jfc_argv[v].type == 11)   /* '-' */
    return 1;
  for (m = 0; jfc_t_dops[m].precedence != -1; m++)
  { if (strcmp(jfc_argv[v].arg, jfc_t_dops[m].name) == 0)
      return m;
  }
  return -1;
}

static int jfc_func_find(char *name)
{
  int m;

  for (m = 0; jfc_t_funcs[m].argc != -1; m++)
  { if (strcmp(name, jfc_t_funcs[m].name) == 0)
      return m;
  }
  return -1;
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
            for (s = 0; sf == -1 && s < jfw_head.synonym_c; s++)
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
            jfc_argline[jfc_argl_ff] = '\0';
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


static void jfc_comment_handle(void)
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

    if (jfc_func_def == 1)
      jfc_head->functions[jfc_func_status].comment_no
        = comment_no;
    else
    { jfc_cadd(JFR_OP_COMMENT);
      jfc_cadd(comment_no / 256);
      if (comment_no != 0)
        jfc_cadd(comment_no % 256);
      else
        jfc_cadd(0);
    }
  }
}

/********************************************************/
/* hjaelpefunktioner til jfc_rule_ins                   */
/********************************************************/

static int jfc_cadd(unsigned char c)
{
  if (jfc_ff_jfr + 1 >= jfc_ff_memory)
    return jfc_error(8, jfc_empty, JFE_FATAL);
  *jfc_ff_jfr = c;
  jfc_ff_jfr++;
  return 0;
}

static int jfc_progadd(int no)
{
  if (jfc_head->vbytes == 2)
  { jfc_cadd((unsigned char) (no / 256));
    jfc_cadd((unsigned char) (no % 256));
  }
  else
    jfc_cadd((unsigned char) no);
  return 0;
}


/* hjaelpefunktion til jfc_rule_ins. Indsaetter en float-saetning */

static int jfc_weight_ins(int op, float val)
{
  if (jfc_ff_jfr + sizeof(float) + 2 >= jfc_ff_memory)
    return jfc_error(8, jfc_empty, JFE_FATAL);

  *(jfc_ff_jfr) = (unsigned char) (op);
  jfc_ff_jfr++;
  memcpy(jfc_ff_jfr, (char *) &val, sizeof(float));
  jfc_ff_jfr += sizeof(float);

  return 0;
}

static int jfc_tab_handle(int s_type, int a_type, int arg, int argc)
{
  int aktion, slut;
  struct jfc_stack_desc *ts;

  slut = 0;
  while (slut == 0)
  { aktion = jfc_tabel[s_type][jfc_stack[jfc_ff_stack - 1].s_type];
    ts = &(jfc_stack[jfc_ff_stack]);
    if (aktion == 10)
    { if (argc > jfc_stack[jfc_ff_stack - 1].argc)  /* argc = precedence */
     aktion = 2;
      else
     aktion = 3;
    }
    switch (aktion)
    { case 1:      /* sker aldrig ! */
        slut = 1;
        break;
      case 2:     /* ip -> stak */
        ts->s_type = s_type;
        ts->a_type = a_type;
        ts->arg = arg;
        ts->argc = argc;
        if (jfc_ff_stack >= jfc_maxstack)
          jfc_error(102, jfc_empty, JFE_ERROR);
        else
          jfc_ff_stack++;
        slut = 1;
        break;
      case 3:
      case 7:
        if (jfc_ff_stack > 0)
        { jfc_ff_stack--;
          ts = &(jfc_stack[jfc_ff_stack]);
          if (ts->a_type != 0)
            jfc_cadd(ts->a_type);
          if (ts->a_type != JFR_OP_IIF)
            jfc_cadd(ts->arg);
          if (ts->s_type == SYMB_FUNC || ts->s_type == SYMB_PFUNC)
          { if (ts->argc != 1)
              jfc_error(7, jfc_empty, JFE_ERROR);
          }
        }
        else
          jfc_error(102, jfc_empty, JFE_ERROR);
        if (aktion == 7)
          slut = 1;
        break;
      case 4:
        slut = 1;
        if (jfc_ff_stack > 0)
          jfc_ff_stack--;
        break;
      case 5:
        return jfc_error(7, jfc_empty, JFE_ERROR);
      case 6:
        jfc_stack[jfc_ff_stack - 1].s_type = SYMB_PFUNC;
        slut = 1;
        break;
      case 8:
        jfc_stack[jfc_ff_stack - 1].argc--;
        slut = 1;
        break;
      case 9:
        slut = 1;
        break;
    }
  }
  return 0;
}

static int jfc_expr_ins(int adr, int eq_end)
{
  float f, sign;
  int m, v, var_no, arg, improve;
  unsigned char cno;
  int icount, konstant, vaf_mode;
  struct jfr_var_desc *var;

  int state;
  /* 0 : varname, '(' or hedge,                */
  /* 1 : efter varname,                        */
  /* 2 : efter varname hedge,                  */
  /* 3 : efter const eller var adjectiv,       */
  /* 4 : slut,                                 */
  /* 5 : efter varname between,                */
  /* 6 : efter varname between adjectiv,       */
  /* 7 : efter varname between adjectiv and,   */
  /* 8 : efter varname in,                     */
  /* 9 : efter varname in (,                   */
  /*10 : efter varname in ( adjectiv           */
  /*11 : efter varfunc                         */
  /*12 : efter varfunc(                        */
  /*13 : efter var-func(varname/arrname        */

  v = adr;

  jfc_stack[0].s_type = SYMB_END;
  jfc_ff_stack = 1;
  state = 0;
  var_no = 0;

  while (state != 4)
  { if (v >= jfc_argc || strcmp(jfc_argv[v].arg, jfc_t_then) == 0
        || (eq_end == 1 && strcmp(jfc_argv[v].arg, jfc_t_eq) == 0)
        || jfc_argv[v].type == 3)  /* ; */
    { if (state == 1)
      { jfc_cadd(JFR_OP_VAR);
        jfc_cadd(var_no);
      }
      jfc_tab_handle(SYMB_END, 0, 0, 0);
      if (state == 0 || state == 2)
        return jfc_error(7, jfc_empty, JFE_ERROR);
      state = 4;
    }

    switch (state)
    { case 0:       /* var */
        if (jfc_argv[v].type == 6)  /* '(' */
          jfc_tab_handle(SYMB_BPAR, 0, 0, 0);
        else
        if ((arg = jfc_hedge_find(jfc_argv[v].arg)) != -1)
          jfc_tab_handle(SYMB_FUNC, JFR_OP_HEDGE, arg, 1);
        else
        if ((arg = jfc_ufunc_find(jfc_argv[v].arg)) != -1)
          jfc_tab_handle(SYMB_FUNC, JFR_OP_USERFUNC, arg,
                         jfc_head->functions[arg].arg_c);
        else
        if ((arg = jfc_func_find(jfc_argv[v].arg)) != -1)
          jfc_tab_handle(SYMB_FUNC, jfc_t_funcs[arg].a_type,
                         jfc_t_funcs[arg].arg, jfc_t_funcs[arg].argc);
        else
        if (jfc_func_status >= 0
            && (arg = jfc_ufuncarg_find(jfc_func_status,
                                        jfc_argv[v].arg)) != -1)
        { jfc_cadd(JFR_OP_SPUSH);
          jfc_cadd((unsigned char) arg);
          state = 3;
        }
        else
        if ((arg = jfc_var_find(jfc_argv[v].arg)) != -1)
        { var_no = arg;
          state = 1;
        }
        else
        if ((arg = jfc_array_find(jfc_argv[v].arg)) != -1)
          jfc_tab_handle(SYMB_FUNC, JFR_OP_ARRAY, arg, 1);
        else
        if ((arg = jfc_table_find(&m, jfc_t_vfuncs, jfc_argv[v].arg)) != -1)
        { jfc_cadd(JFR_OP_VFUNC);
          vaf_mode = 1;   /* variable */
          jfc_cadd((unsigned char) arg);
          state = 11;
        }
        else
        if (strcmp(jfc_argv[v].arg, jfc_t_true) == 0)
        { jfc_cadd(JFR_OP_TRUE);
          state = 3;
        }
        else
        if (strcmp(jfc_argv[v].arg, jfc_t_false) == 0)
        { jfc_cadd(JFR_OP_FALSE);
          state = 3;
        }
        else
        if (jfc_argv[v].type == 11)     /* - (minus) */
        { konstant = 0;
          if (v + 1 < jfc_argc)
          { if (jfc_argv[v + 1].type == 1)
            { if (jf_atof(&f, jfc_argv[v + 1].arg) == 0)
              { konstant = 1;
                v++;                             /* NB v++ */
                jfc_weight_ins(JFR_OP_CONST, -f);
                state = 3;
              }
            }
          }
          if (konstant == 0)
            jfc_tab_handle(SYMB_FUNC, JFR_OP_SFUNC, 10, 1);
        }
        else
        { improve = 0;
          sign = 1.0;
          if (jfc_argv[v].type == 2)
          { improve = 1;
            v++;                             /* NB v++ */
            if (v < jfc_argc && jfc_argv[v].type == 11)  /* - */
            { sign = -1.0;
              v++;                           /* NB v++ */
            }
          }
          f = jfc_atof(v);
          if (improve == 0)
            jfc_weight_ins(JFR_OP_CONST, f);
          else
            jfc_weight_ins(JFR_OP_ICONST, sign * f);
          state = 3;
        }
        break;
      case 1:
        if (jfc_argv[v].type == 7)  /* ) */
        { jfc_cadd(JFR_OP_VAR);
          jfc_cadd(var_no);
          jfc_tab_handle(SYMB_EPAR, 0, 0, 0);
          state = 3;
        }
        else
        if (jfc_argv[v].type == 12)   /* , */
        { jfc_cadd(JFR_OP_VAR);
          jfc_cadd(var_no);
          jfc_tab_handle(SYMB_COMMA, 0, 0, 0);
          state = 0;
        }
        else
        if ((arg = jfc_hedge_find(jfc_argv[v].arg)) != -1)
        { jfc_tab_handle(SYMB_FUNC, JFR_OP_HEDGE, arg, 1);
          state = 2;
        }
        else
        if ((arg = jfc_fzvar_find(var_no, jfc_argv[v].arg)) != -1)
        { jfc_progadd(arg);
          state = 3;
        }
        else
        if (strcmp(jfc_argv[v].arg, jfc_t_between) == 0)
        { jfc_cadd(JFR_OP_BETWEEN);
          jfc_cadd(var_no);
          state = 5;
        }
        else
        if (strcmp(jfc_argv[v].arg, jfc_t_in) == 0)
        { state = 8;
        }
        else
        if ((arg = jfc_dops_find(v)) != -1)
        { jfc_cadd(JFR_OP_VAR);
          jfc_cadd(var_no);
          jfc_tab_handle(SYMB_OP, jfc_t_dops[arg].a_type,
                         jfc_t_dops[arg].arg,
                        jfc_t_dops[arg].precedence);
          state = 0;
        }
        else
        if ((arg = jfc_relation_find(jfc_argv[v].arg)) != -1)
        { jfc_cadd(JFR_OP_VAR);
          jfc_cadd(var_no);
          jfc_tab_handle(SYMB_OP, JFR_OP_UREL, arg, 40);
          state = 0;
        }
        else
        if ((arg = jfc_operator_find(jfc_argv[v].arg)) != -1)
        { jfc_cadd(JFR_OP_VAR);
          jfc_cadd(var_no);
          jfc_tab_handle(SYMB_OP, JFR_OP_OP, arg,
                         jfc_head->operators[arg].precedence);
          state = 0;
        }
        else
          return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
        break;
      case 2:
        if ((arg = jfc_hedge_find(jfc_argv[v].arg)) != -1)
          jfc_tab_handle(SYMB_FUNC, JFR_OP_HEDGE, arg, 1);
        else
        if ((arg = jfc_fzvar_find(var_no, jfc_argv[v].arg)) != -1)
        { jfc_progadd(arg);
          state = 3;
        }
        else
          jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
        break;
      case 3:
        if (jfc_argv[v].type == 7)  /* ) */
          jfc_tab_handle(SYMB_EPAR, 0, 0, 0);
        else
        if (jfc_argv[v].type == 12)   /* , */
        { jfc_tab_handle(SYMB_COMMA, 0, 0, 0);
          state = 0;
        }
        else
        if ((arg = jfc_dops_find(v)) != -1)
        { jfc_tab_handle(SYMB_OP, jfc_t_dops[arg].a_type,
                         jfc_t_dops[arg].arg,
                         jfc_t_dops[arg].precedence);
          state = 0;
        }
        else
        if ((arg = jfc_operator_find(jfc_argv[v].arg)) != -1)
        { jfc_tab_handle(SYMB_OP, JFR_OP_OP, arg,
                         jfc_head->operators[arg].precedence);
          state = 0;
        }
        else
        if ((arg = jfc_relation_find(jfc_argv[v].arg)) != -1)
        { jfc_tab_handle(SYMB_OP, JFR_OP_UREL, arg, 40);
          state = 0;
        }
        else
          return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
        break;
      case 5:
        if ((arg = jfc_fzvar_find(var_no, jfc_argv[v].arg)) != -1)
        { var = &(jfc_head->vars[var_no]);
          cno = (unsigned char) (arg - var->f_fzvar_no);
          jfc_cadd(cno);
          state = 6;
        }
        else
          return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
        break;
      case 6:
        if (strcmp(jfc_argv[v].arg, jfc_t_and) != 0)
          return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
        state = 7;
        break;
      case 7:
        if ((arg = jfc_fzvar_find(var_no, jfc_argv[v].arg)) != -1)
        { var = &(jfc_head->vars[var_no]);
          cno = (unsigned char) (arg - var->f_fzvar_no);
          jfc_cadd(cno);
          state = 3;
        }
        else
          return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
        break;
      case 8:
        if (jfc_argv[v].type != 6)    /* ( */
          return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
        state = 9;
        icount = 0;
        break;
      case 9:
        if ((arg = jfc_fzvar_find(var_no, jfc_argv[v].arg)) == -1)
          return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
        jfc_progadd(arg);
        icount++;
        state = 10;
        break;
      case 10:
        if (jfc_argv[v].type == 12)   /* , */
        { if (icount > 1)
          { jfc_cadd(JFR_OP_OP);
            jfc_cadd(3);   /* JFR_OID_OR */
          }
          state = 9;
        }
        else
        if (jfc_argv[v].type == 7)   /* )  */
        { if (icount > 1)
          { jfc_cadd(JFR_OP_OP);
            jfc_cadd(3);   /* JFR_OID_OR */
          }
          state = 3;
        }
        else
          jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
        break;
      case 11:
        if (jfc_argv[v].type != 6)  /* ( */
          return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
        state = 12;
        break;
      case 12:
        if (vaf_mode == 1)
          arg = jfc_var_find(jfc_argv[v].arg);
        else
          arg = jfc_array_find(jfc_argv[v].arg);
        if (arg == -1)
          return jfc_error(118, jfc_argv[v].arg, JFE_ERROR);
        jfc_cadd(arg);
        state = 13;
        break;
      case 13:
        if (jfc_argv[v].type != 7)
          return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
        state = 3;
        break;
    }
    v++;
  }      /* while */
  return v;
}

static int jfc_function_ins(void)  /* insert function/procedure-head */
{
  int v, func_no, tilst;
  struct jfr_function_desc *func;

  if (jfc_head->function_c >= 255)
    jfc_error(149, jfc_empty, JFE_ERROR);
  if (jfc_argc < 4)
    jfc_error(127, jfc_empty, JFE_ERROR);
  v = 1;
  func = &(jfc_head->functions[jfc_head->function_c]);
  jfc_tcut(v, 16);
  if (jfc_ufunc_find(jfc_argv[v].arg) != -1)
    return jfc_error(128, jfc_argv[v].arg, JFE_ERROR);
  func_no = jfc_head->function_c;
  if (jfc_pass == 1)
  { for (v = 3; v < jfc_argc && jfc_argv[v].type != 7; v++)  /* ') */
    { if (jfc_argv[v].type == 12)  /* , */
     jfc_head->funcarg_c++;
    }
    jfc_head->funcarg_c++;
  }
  else
  {
    if (jfc_ff_iet >= JFC_IET_SIZE)
      return jfc_error(136, jfc_empty, JFE_ERROR);
    else
    { jfc_iet_stack[jfc_ff_iet].type = 5;
      jfc_iet_stack[jfc_ff_iet].arg = func_no;
      jfc_ff_iet++;
    }

    if (strcmp(jfc_argv[0].arg, jfc_t_function) == 0)
      func->type = 1;
    else
      func->type = 0;
    strcpy(func->name, jfc_argv[v].arg);
    func->f_arg_no = jfc_head->funcarg_c;
    func->arg_c = 0;
    func->comment_no = -1;
    func->pc = jfc_ff_jfr - jfc_head->function_code;
    v++;
    if (jfc_argv[v].type != 6)
      return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
    v++;
    tilst = 1;
    while (v < jfc_argc && jfc_argv[v].type != 7)
    { if (tilst == 1)
      { if (jfc_argv[v].type != 1)
          return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
        jfc_tcut(v, 16);
        if (func->arg_c >= 255)
          return jfc_error(160, func->name, JFE_ERROR);
        strcpy(jfc_head->func_args[jfc_head->funcarg_c].name,
               jfc_argv[v].arg);
        jfc_head->func_args[jfc_head->funcarg_c].function_no = func_no;
        func->arg_c++;
        jfc_head->funcarg_c++;
        v++;
        tilst = 2;
      }
      else
      { if (jfc_argv[v].type != 12)  /* , */
          return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
        v++;
        tilst = 1;
      }
    }
    v++;
    if (tilst != 2 || v < jfc_argc)
      jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
  }
  jfc_func_status = jfc_head->function_c;
  jfc_head->function_c++;
  return 0;
}

static int jfc_rule_ins(void)
{
  float rweight, sign;
  int m, len, v, st_no, wlearn, increase;
  int svno, fzvar_no, if_mode;

  if_mode = 0;
  if ((st_no = jfc_table_find(&v, jfc_t_statements, jfc_argv[0].arg))
      == -1)      /* asume asign-statement */
  { v = 0;
    jfc_cadd(JFR_OP_EXPR);
    jfc_cadd(JFR_OP_TRUE);
    if_mode = 1;
    st_no = 0;
  }
  else
    v = 1;
  switch (st_no)
  { case 6:                               /* ifw */
      wlearn = 0;
      if (jfc_argv[v].type == 2)   /* % */
      { wlearn = 1;
        v++;
      }
      if (jfc_argv[v].type == 11)  /* - */
      { sign = -1.0;
        v++;
      }
      else
        sign = 1.0;
      rweight = sign * jfc_atof(v);
      v++;
      if (wlearn == 1)
        jfc_weight_ins(JFR_OP_AWEXPR, rweight);
      else
        jfc_weight_ins(JFR_OP_WEXPR, rweight);
    case 0:                               /* if  */
      if (st_no == 0 && if_mode == 0)
        jfc_cadd(JFR_OP_EXPR);
      if (if_mode == 0)
        v = jfc_expr_ins(v, 0);
      if (v == -1)
        return -1;
      if ((m = jfc_ufuncarg_find(jfc_func_status, jfc_argv[v].arg)) >= 0)
      { v++;
        if (strcmp(jfc_argv[v].arg, jfc_t_eq) == 0)
        { v++;
          jfc_cadd(JFR_OP_THENEXPR);
          v = jfc_expr_ins(v, 0);
          if (v == -1)
            return -1;
          jfc_cadd(JFR_OP_SPOP);
          jfc_cadd(m);
        }
        else
          return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
      }
      else
      if ((m = jfc_var_find(jfc_argv[v].arg)) >= 0)
      { v++;
        if (strcmp(jfc_argv[v].arg, jfc_t_eq) == 0)
        { v++;
          jfc_cadd(JFR_OP_THENEXPR);
          v = jfc_expr_ins(v, 0);
          if (v == -1)
            return -1;
          jfc_cadd(JFR_OP_VPOP);
          jfc_cadd(m);
        }
        else
        { fzvar_no = jfc_fzvar_find(m, jfc_argv[v].arg);
          if (fzvar_no == -1)
            return jfc_error(116, jfc_argv[v].arg, JFE_ERROR);
          jfc_cadd(JFR_OP_THENEXPR);
          jfc_cadd(JFR_OP_FZVPOP);
          jfc_progadd(fzvar_no);
        }
      }
      else
      if (strcmp(jfc_argv[v].arg, jfc_t_return) == 0)
      { v++;
        if (jfc_func_status < 0
            || jfc_head->functions[jfc_func_status].type == 0)  /* proc */
          return jfc_error(154, jfc_empty, JFE_ERROR);
        jfc_cadd(JFR_OP_THENEXPR);
        v = jfc_expr_ins(v, 0);
        if (v == -1)
          return -1;
        jfc_cadd(JFR_OP_FRETURN);
      }
      else
      if ((m = jfc_array_find(jfc_argv[v].arg)) >= 0)
      { v++;
        if (jfc_argv[v].type == 6)  /* [ (array-asignment) */
        { jfc_cadd(JFR_OP_THENEXPR);
          v = jfc_expr_ins(v, 1);   /* indsaetter [<expr>] ! */
          if (v > 0 && strcmp(jfc_argv[v - 1].arg, jfc_t_eq) == 0)
          { v = jfc_expr_ins(v, 0);
            if (v == -1)
              return -1;
            jfc_cadd(JFR_OP_APOP);
            jfc_cadd(m);
          }
          else
            return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
        }
      }
      else
      if (strcmp(jfc_argv[v].arg, jfc_t_increase) == 0 ||
       strcmp(jfc_argv[v].arg, jfc_t_decrease) == 0)
      { if (strcmp(jfc_argv[v].arg, jfc_t_increase) == 0)
          increase = 1;
        else
          increase = 0;
        v++;

        if ((m = jfc_var_find(jfc_argv[v].arg)) == -1)
          return jfc_error(130, jfc_argv[v].arg, JFE_ERROR);
        v++;
        jfc_cadd(JFR_OP_THENEXPR);
        if (strcmp(jfc_argv[v].arg, jfc_t_with) == 0)
        { v++;
          v = jfc_expr_ins(v, 0);
          if (v == -1)
            return -1;
        }
        else
        { if (jfc_argv[v].type != 3)    /* ; */
            return jfc_error(150, jfc_argv[v].arg, JFE_ERROR);
          jfc_weight_ins(JFR_OP_CONST, 1.0);
        }
        if (increase == 1)
          jfc_cadd(JFR_OP_VINCREASE);
        else
          jfc_cadd(JFR_OP_VDECREASE);
        jfc_cadd(m);
      }
      else
      if ((m = jfc_ufunc_find(jfc_argv[v].arg)) != -1)
      { jfc_cadd(JFR_OP_THENEXPR);
        v = jfc_expr_ins(v, 0);
        if (v == -1)
          return -1;
      }
      else
      if (strcmp(jfc_argv[v].arg, jfc_t_clear) == 0)
      { v++;
        m = jfc_var_find(jfc_argv[1].arg);
        if (m == -1)
          return jfc_error(118, jfc_argv[1].arg, JFE_ERROR);
        jfc_cadd(JFR_OP_THENEXPR);
        jfc_cadd(JFR_OP_CLEAR);
        jfc_cadd(m);
        v++;
        if (jfc_argv[v].type != 3)     /* ; */
          return jfc_error(7, jfc_argv[v].arg, JFE_ERROR);
      }
      else
      if (strcmp(jfc_argv[v].arg, jfc_t_extern) == 0 ||
       strcmp(jfc_argv[v].arg, jfc_t_call)   == 0)
      { v++;
        len = 0;
        if (jfc_argc > 255)
          return jfc_error(101, jfc_empty, JFE_ERROR);
        for (m = v; m < jfc_argc; m++)
        { if (jfc_argv[m].type == 1)  /* token */
          { len += strlen(jfc_argv[m].arg) + 1;
            if (jfc_argv[m].qoute == 1)
              len += 2;
          }
          else
            len += 2;
        }
        jfc_cadd(JFR_OP_THENEXPR);
        jfc_cadd(JFR_OP_EXTERN);
        jfc_cadd(len / 256);
        jfc_cadd(len % 256);
        jfc_cadd(jfc_argc - v);
        for (m = v; m < jfc_argc; m++)
        { if (jfc_argv[m].type == 1)
          { if (jfc_argv[m].qoute == 1)
              jfc_cadd('"');
            for (v = 0; jfc_argv[m].arg[v] != '\0'; v++)
              jfc_cadd(jfc_argv[m].arg[v]);
            if (jfc_argv[m].qoute == 1)
              jfc_cadd('"');
            jfc_cadd('\0');
          }
          else
          { switch (jfc_argv[m].type)
            { case 2:
                jfc_cadd('%');
                break;
              case 3:
                jfc_cadd(';');
                break;
              case 4:
                jfc_cadd('"');
                break;
              case 5:
                jfc_cadd(':');
                break;
              case 6:
                jfc_cadd('(');
                break;
              case 7:
                jfc_cadd(')');
                break;
              case 11:
                jfc_cadd('-');
                break;
              case 12:
                jfc_cadd(',');
                break;
            }
            jfc_cadd('\0');
          }
        }  /* for */
      }
      else
        return jfc_error(130, jfc_argv[v].arg, JFE_ERROR);
      break;
    case 1:   /* switch */
      if (jfc_argc > 2)
        return jfc_error(123, jfc_argv[1].arg, JFE_ERROR);
      if (jfc_ff_iet >= JFC_IET_SIZE)
        jfc_error(136, jfc_empty, JFE_ERROR);
      else
      { if (jfc_argc == 1)
        { jfc_iet_stack[jfc_ff_iet].type = 1;
          jfc_cadd(JFR_OP_SWITCH);
        }
        else
        { jfc_iet_stack[jfc_ff_iet].type = 4;
          svno = jfc_var_find(jfc_argv[1].arg);
          if (svno == -1)
            return jfc_error(130, jfc_argv[1].arg, JFE_ERROR);
          jfc_iet_stack[jfc_ff_iet].arg = svno;
          jfc_cadd(JFR_OP_VSWITCH);
          jfc_cadd(svno);
        }
        jfc_ff_iet++;
      }
      break;
    case 2:  /* end */
      if (jfc_argc != 1)
        jfc_error(123, jfc_argv[1].arg, JFE_ERROR);
      if (jfc_ff_iet <= 0)
        jfc_error(139, jfc_empty, JFE_ERROR);
      else
      { jfc_ff_iet--;
        if (jfc_iet_stack[jfc_ff_iet].type == 3) /* while */
          jfc_cadd(JFR_OP_ENDWHILE);
        else
        if (jfc_iet_stack[jfc_ff_iet].type == 5) /* function */
        { jfc_cadd(JFR_OP_ENDFUNC);
          jfc_func_status = -1;
        }
        else
          jfc_cadd(JFR_OP_ENDSWITCH);
      }
      break;
    case 3:  /* case */
      if (jfc_ff_iet <= 0)
        jfc_error(138, jfc_empty, JFE_ERROR);
      if (jfc_iet_stack[jfc_ff_iet - 1].type != 1
          && jfc_iet_stack[jfc_ff_iet - 1].type != 4)
        jfc_error(138, jfc_empty, JFE_ERROR);
      jfc_cadd(JFR_OP_EXPR);
      v = -1;
      if (jfc_iet_stack[jfc_ff_iet - 1].type == 4)
      { if (jfc_argc == 2)
        { v = jfc_fzvar_find(jfc_iet_stack[jfc_ff_iet - 1].arg,
                             jfc_argv[1].arg);
          if (v != -1)
          { jfc_progadd(v);
            jfc_cadd(JFR_OP_VCASE);
          }
        }
      }
      if (v == -1)
      { v = jfc_expr_ins(1, 0);
        if (v == -1)
          return -1;
        jfc_cadd(JFR_OP_CASE);
      }
      break;
    case 7:  /* wset */
      jfc_cadd(JFR_OP_EXPR);
      if (jfc_argc == 1)
         jfc_cadd(JFR_OP_FALSE);
      else
      { v = jfc_expr_ins(1, 0);
        if (v == -1)
          return -1;
      }
      jfc_cadd(JFR_OP_WSET);
      break;
    case 4:  /* default */
      if (jfc_argc != 1)
        jfc_error(123, jfc_argv[1].arg, JFE_ERROR);
      if (jfc_ff_iet <= 0)
        jfc_error(137, jfc_empty, JFE_ERROR);
      else
      { if (jfc_iet_stack[jfc_ff_iet - 1].type != 1
            && jfc_iet_stack[jfc_ff_iet - 1].type != 4)
          jfc_error(137, jfc_empty, JFE_ERROR);
        jfc_cadd(JFR_OP_DEFAULT);
      }
      break;
    case 5: /* while */
      jfc_cadd(JFR_OP_EXPR);
      if (jfc_ff_iet >= JFC_IET_SIZE)
        jfc_error(136, jfc_empty, JFE_ERROR);
      else
      { jfc_iet_stack[jfc_ff_iet].type = 3;
        jfc_ff_iet++;
        v = jfc_expr_ins(1, 0);
        if (v == -1)
          return -1;
        jfc_cadd(JFR_OP_WHILE);
      }
      break;
  }
  return 0;
}

static int jfc_progline_ins(void)
{
  if (strcmp(jfc_argv[0].arg, jfc_t_function) == 0 ||
      strcmp(jfc_argv[0].arg, jfc_t_procedure) == 0)
  { if (jfc_pass == 2 && jfc_func_status != -1)
      jfc_error(153, jfc_empty, JFE_ERROR);
    jfc_function_ins();
    jfc_func_def = 1;
  }
  else
  { jfc_func_def = 0;
    if (jfc_func_status == -1)  /* start paa main-program */
    { jfc_func_status = -2;
      jfc_head->program_code = (unsigned char *) jfc_ff_jfr;
      jfc_head->funccode_c
        = jfc_head->program_code - jfc_head->function_code;
    }
    if (jfc_pass == 2)
      jfc_rule_ins();
  }
  return 0;
}

static int jfc_end_ins(void)
{
  jfc_cadd((unsigned char) JFR_OP_EOP);
  jfc_head->program_c = jfc_ff_jfr - jfc_head->program_code;
  if (jfc_ff_iet != 0)
    return jfc_error(140, jfc_empty, JFE_ERROR);
  jfc_ttypes[':']  = 5;

  return 0;
}

static void jfc_plf_handle(int start_limit, int lcount, char *txt)
{
  int cu_limit, m;
  float x, y, hx, hy;

  x = 0.0; y = 0.0;
  for (m = 0; m < lcount; m++)
  { hx = x;
    hy = y;
    cu_limit = start_limit + m;
    x = jfc_head->limits[cu_limit].limit;
    y = jfc_head->limits[cu_limit].b;
    if (m == 0)
    { jfc_head->limits[cu_limit].a = 0.0;
      jfc_head->limits[cu_limit].b = y;
    }
    else
    { if (x < hx)
      { jfc_error(135, txt, JFE_ERROR);
        jfc_head->limits[cu_limit].a = 0.0;
        jfc_head->limits[cu_limit].b = y;
      }
      else
      if (x == hx)
      { if (jfc_head->limits[cu_limit].exclusiv == 0
            && jfc_head->limits[cu_limit - 1].exclusiv == 0)
          jfc_error(135, txt, JFE_ERROR);
        jfc_head->limits[cu_limit].a = 0;
        jfc_head->limits[cu_limit].b = y;
      }
      else
      { jfc_head->limits[cu_limit].a = (y - hy) / (x - hx);
        jfc_head->limits[cu_limit].b
           = y - jfc_head->limits[cu_limit].a * x;
      }
    }
  }
}

static void jfc_limits_handle(void)
{
  struct jfr_hedge_desc *hedge;
  struct jfr_relation_desc *rel;
  struct jfr_adjectiv_desc *adj;
  struct jfr_var_desc *var;
  struct jfr_domain_desc *dom;

  char txt[256];
  int m, a;

  for (m = 0; m < jfc_head->hedge_c; m++)
  { hedge = &(jfc_head->hedges[m]);
    if (hedge->type == JFS_HT_LIMITS)
    { strcpy(txt, "in hedge: ");
      strcat(txt, hedge->name);
      jfc_plf_handle(hedge->f_limit_no, hedge->limit_c, txt);
    }
  }
  for (m = 0; m < jfc_head->relation_c; m++)
  { rel = &(jfc_head->relations[m]);
    strcpy(txt, "in relation: ");
    strcat(txt, rel->name);
    jfc_plf_handle(rel->f_limit_no, rel->limit_c, txt);
  }
  for (m = 0; m < jfc_head->domain_c; m++)
  { dom = &(jfc_head->domains[m]);
    for (a = 0; a < dom->adjectiv_c; a++)
    { adj = &(jfc_head->adjectives[dom->f_adjectiv_no + a]);
      if (adj->limit_c > 0)
      { strcpy(txt, "in adjectiv: ");
        strcat(txt, dom->name);
        strcat(txt, " is ");
        strcat(txt, adj->name);
        jfc_plf_handle(adj->f_limit_no, adj->limit_c, txt);
      }
    }
  }
  for (m = 0; m < jfc_head->var_c; m++)
  { var = &(jfc_head->vars[m]);
    dom = &(jfc_head->domains[var->domain_no]);
    if (var->fzvar_c > 0 && (var->f_adjectiv_no != dom->f_adjectiv_no ||
                             dom->adjectiv_c == 0))
    { for (a = 0; a < var->fzvar_c; a++)
      { adj = &(jfc_head->adjectives[var->f_adjectiv_no + a]);
        if (adj->limit_c > 0)
        { strcpy(txt, "in adjectiv: ");
          strcat(txt, var->name);
          strcat(txt, " is ");
          strcat(txt, adj->name);
          jfc_plf_handle(adj->f_limit_no, adj->limit_c, txt);
        }
      }
    }
  }
}

static int jfc_adj_centtrap_handle(void)
{
  float x, hx, cg_tael, cg_naevn;
  int adj_no, m, v;
  int cu_limit;
  struct jfr_adjectiv_desc *adjectiv;
  struct jfr_domain_desc *dom;
  struct jfr_var_desc *var;
  char err_text[256];

  for (v = 0; v < jfc_head->var_c; v++)
  { var = &(jfc_head->vars[v]);
    dom = &(jfc_head->domains[var->domain_no]);
    for (adj_no = 0; adj_no < var->fzvar_c; adj_no++)
    { adjectiv = &(jfc_head->adjectives[var->f_adjectiv_no + adj_no]);
      strcpy(err_text, " in adjectiv ");
      strcat(err_text, var->name);
      strcat(err_text, " is ");
      strcat(err_text, adjectiv->name);
      if (adjectiv->limit_c > 0)
      { x = 0;
        cg_tael = cg_naevn = 0.0;
        for (m = 0; m < adjectiv->limit_c; m++)
        { cu_limit = adjectiv->f_limit_no + m;
          hx = x;
          x = jfc_head->limits[cu_limit].limit;
          if (m != 0)
          { /* optael center of gravity                            */
            /* = sum(intgral(x * (ax + b)) / sum(intgral(ax + b))  */

            cg_tael +=  1.0 / 3.0 * jfc_head->limits[cu_limit].a
                        * (x * x * x - hx * hx * hx)
                      + 0.5 * jfc_head->limits[cu_limit].b *
                        (x * x - hx * hx);
            cg_naevn +=  0.5 * jfc_head->limits[cu_limit].a
                       * (x * x - hx * hx)
                     + jfc_head->limits[cu_limit].b * (x - hx);
          }
        }
        if (cg_naevn != 0.0 && (adjectiv->flags & JFS_AF_CENTER) == 0)
          adjectiv->center = cg_tael / cg_naevn;
      }
      else
      { if ((adjectiv->flags & JFS_AF_CENTER) == 0)  /* no center and no plf */
        { if (adjectiv->flags & JFS_AF_TRAPEZ)
          { adjectiv->center
              = (adjectiv->trapez_start + adjectiv->trapez_end) / 2.0;
          }
          else
          { if (dom->flags & JFS_DF_MINENTER)
              adjectiv->center = dom->dmin + adj_no;
            else
              adjectiv->center = adj_no;
            if ((dom->flags & JFS_DF_MAXENTER) != 0
                && adjectiv->center > dom->dmax)
              jfc_error(103, err_text, JFE_ERROR);
          }
        }
      }
      if ((adjectiv->flags & JFS_AF_TRAPEZ) == 0)  /* no trapez entered */
      { adjectiv->trapez_start = adjectiv->trapez_end = adjectiv->center;
      }
    }
  }
  return 0;
}

static void jfc_adjectives_handle(void)
{
  int v, m;
  float pre, post;
  struct jfr_adjectiv_desc *adjectiv;
  struct jfr_domain_desc *dom;
  struct jfr_var_desc *var;

  jfc_adj_centtrap_handle();

  /* beregn base */
  for (v = 0; v < jfc_head->var_c; v++)
  { var = &(jfc_head->vars[v]);
    dom = &(jfc_head->domains[var->domain_no]);
    for (m = 0; m < var->fzvar_c; m++)
    { adjectiv = &(jfc_head->adjectives[var->f_adjectiv_no + m]);
      if (!(adjectiv->flags & JFS_AF_BASE))
      { if (adjectiv->limit_c == 0)
        { if (m == 0)
          { if (dom->flags & JFS_DF_MINENTER)
              pre = dom->dmin;
            else
              pre = adjectiv->trapez_start;
          }
          else
            pre = jfc_head->adjectives[var->f_adjectiv_no + m - 1].trapez_end;
          if (m == var->fzvar_c - 1)
          { if (dom->flags & JFS_DF_MAXENTER)
              post = dom->dmax;
            else
              post = adjectiv->trapez_end;
          }
          else
            post
              = jfc_head->adjectives[var->f_adjectiv_no + m + 1].trapez_start;
        }
        else
        { pre = jfc_head->limits[adjectiv->f_limit_no].limit;
          post = jfc_head->limits[adjectiv->f_limit_no
                  + adjectiv->limit_c - 1].limit;
        }
        if (pre >= post)
          jfc_error(135, var->name, JFE_WARNING);
        adjectiv->base = post - pre;
      }
    }  /* for adj */
  }  /* for var */
}


static int jfc_vars_handle(void)
{
  int v, fzvno;
  struct jfr_var_desc *var;

  fzvno = 0;
  for (v = 0; v < jfc_head->var_c; v++)
  { var = &(jfc_head->vars[v]);
    if (var->fzvar_c == 0)
    { var->f_adjectiv_no = jfc_head->domains[var->domain_no].f_adjectiv_no;
      var->fzvar_c = jfc_head->domains[var->domain_no].adjectiv_c;
    }
    var->f_fzvar_no = fzvno;
    var->value = var->conf = var->conf_sum = 0.0;
    fzvno += var->fzvar_c;
  }
  jfc_head->fzvar_c = fzvno;
  return 0;
}

static void jfc_arrayval_handle(void)
{
  int m, aid;
  struct jfr_array_desc *array;

  aid = 0;
  for (m = 0; m < jfc_head->array_c; m++)
  { array = &(jfc_head->arrays[m]);
    array->f_array_id = aid;
    aid += array->array_c;
  }
  jfc_head->arrayval_c = aid;
}

static void jfc_var_mem_alloc(void)
{
  jfc_vars_handle();
  if (jfc_head->fzvar_c > JFR_OP_FIRST)
    jfc_head->vbytes = 2;
  jfc_limits_handle();
  jfc_adjectives_handle();
  jfc_arrayval_handle();
}

static void jfc_val_init(void)
{
  int v, m;
  struct jfr_var_desc *var;

  for (v = 0; v < jfc_head->var_c; v++)
  { var = &(jfc_head->vars[v]);
    for (m = 0; m < var->fzvar_c; m++)
    { jfc_head->fzvars[var->f_fzvar_no + m].value = 0.0;
      jfc_head->fzvars[var->f_fzvar_no + m].var_no = v;
      jfc_head->fzvars[var->f_fzvar_no + m].adjectiv_no
            = var->f_adjectiv_no + m;
    }
    var->value = var->conf = var->conf_sum = 0.0;
  }
  for (m = 0; m < jfc_head->arrayval_c; m++)
    jfc_head->array_vals[m] = 0.0;
}

static int jfc_init(void)
{
  int m, ar, res;
  int size;

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

  jfc_ff_jfr = (unsigned char *) jfc_memory;

  ar = fread(&jfw_head, 1, JFW_HEAD_SIZE, jfc_source);
  if (ar < 0)
    return jfc_error(2, jfc_empty, JFE_FATAL);
  if (ar == JFW_HEAD_SIZE)
  { res = jfw_check(&jfw_head);
    if (res != 0)
      return jfc_error(res, jfc_empty, JFE_FATAL);
  }
  else
    return jfc_error(4, jfc_empty, JFE_FATAL);

  if (jfc_pass == 1)
  { strcpy(jfc_head->title, jfw_head.title);
    jfc_head->comment_no  = jfw_head.comment_no;
    jfc_head->domain_c    = jfw_head.domain_c;
    jfc_head->adjectiv_c  = jfw_head.adjectiv_c;
    jfc_head->ivar_c      = jfw_head.ivar_c;
    jfc_head->ovar_c      = jfw_head.ovar_c;
    jfc_head->lvar_c      = jfw_head.lvar_c;
    jfc_head->var_c       = jfw_head.var_c;
    jfc_head->array_c     = jfw_head.array_c;
    jfc_head->limit_c     = jfw_head.limit_c;
    jfc_head->relation_c  = jfw_head.relation_c;
    jfc_head->hedge_c     = jfw_head.hedge_c;
    jfc_head->operator_c  = jfw_head.operator_c;
    jfc_head->comment_c   = jfw_head.comment_c;
    jfc_head->f_prog_comment = jfw_head.comment_c;
    jfc_head->com_block_c = jfw_head.com_block_c;
    jfc_head->arg_1_c     = jfw_head.arg_1_c;
    jfc_head->arg_2_c     = jfw_head.arg_2_c;
    jfc_head->function_c  = 0;
    jfc_head->funcarg_c   = 0;
    jfc_head->funccode_c  = 0;
    jfc_head->program_c   = 0;
    /* jfc_head->a_size      = jfw_head.a_size; */
    jfc_head->f_lvar_no   = 0;
    jfc_head->f_ivar_no   = jfw_head.lvar_c;
    jfc_head->f_ovar_no   = jfc_head->f_ivar_no + jfw_head.ivar_c;


  }
  size =   jfw_head.synonym_c    * JFW_SYNONYM_SIZE
  + jfw_head.domain_c     * JFR_DOMAIN_SIZE
  + jfw_head.adjectiv_c   * JFR_ADJECTIV_SIZE
  + jfw_head.var_c        * JFR_VAR_SIZE
  + jfw_head.array_c      * JFR_ARRAY_SIZE
  + jfw_head.limit_c      * JFR_LIMIT_SIZE
  + jfw_head.hedge_c      * JFR_HEDGE_SIZE
  + jfw_head.relation_c   * JFR_RELATION_SIZE
  + jfw_head.operator_c   * JFR_OPERATOR_SIZE
  + jfw_head.comment_c    * JFR_COMMENT_SIZE;

  ar = fread(jfc_ff_jfr, 1, size, jfc_source);
  if (ar != size)
    return jfc_error(2, jfc_empty, JFE_FATAL);


  jfc_ff_jfr += jfw_head.synonym_c * JFW_SYNONYM_SIZE;

  jfc_head->domains = (struct jfr_domain_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->domain_c * JFR_DOMAIN_SIZE;
  jfc_head->adjectives = (struct jfr_adjectiv_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->adjectiv_c * JFR_ADJECTIV_SIZE;
  jfc_head->vars = (struct jfr_var_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->var_c * JFR_VAR_SIZE;
  jfc_head->arrays = (struct jfr_array_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->array_c * JFR_ARRAY_SIZE;
  jfc_head->limits = (struct jfr_limit_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->limit_c * JFR_LIMIT_SIZE;
  jfc_head->hedges = (struct jfr_hedge_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->hedge_c * JFR_HEDGE_SIZE;
  jfc_head->relations = (struct jfr_relation_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->relation_c * JFR_RELATION_SIZE;
  jfc_head->operators = (struct jfr_operator_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->operator_c * JFR_OPERATOR_SIZE;

  jfc_var_mem_alloc();   /* convert vars, adjectives, plfs from jfw    */
                         /* to jfr-format (and fzvar-size, array-size  */
                         /* are calculated).                           */

  jfc_head->comments = (struct jfr_comment_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->comment_c * JFR_COMMENT_SIZE;
  jfc_head->functions = (struct jfr_function_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->function_c * JFR_FUNCTION_SIZE;
  jfc_head->func_args = (struct jfr_funcarg_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->funcarg_c * JFR_FUNCARG_SIZE;
  jfc_head->comment_block = (unsigned char *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->com_block_c;
  jfc_head->fzvars = (struct jfr_fzvar_desc *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->fzvar_c * JFR_FZVAR_SIZE;
  jfc_head->array_vals = (float *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->arrayval_c * sizeof(float);
  jfc_head->function_code = (unsigned char *) jfc_ff_jfr;
  jfc_ff_jfr += jfc_head->funccode_c;
  jfc_head->program_code = jfc_ff_jfr;

  if (jfc_ff_jfr >= jfc_ff_memory)
    return jfc_error(8, jfc_empty, JFE_FATAL);

  jfc_val_init();

  if (jfw_head.com_block_c > 0)
  { ar = fread(jfc_head->comment_block, 1, jfw_head.com_block_c, jfc_source);
    if (ar != jfw_head.com_block_c)
      return jfc_error(2, jfc_empty, JFE_FATAL);
  }

  if (jfc_err_line_mode == 1)
    jfc_line_count = jfw_head.f_prog_line_no;
  else
    jfc_line_count = 0;
  if (jfc_pass == 2)
  { jfc_head->comment_c = jfw_head.comment_c;
    jfc_head->com_block_c = jfw_head.com_block_c;
    jfc_head->function_c = 0;
    jfc_head->funcarg_c = 0;
  }
  jfc_ff_buf = 0;
  jfc_cu_buf = 0;
  jfc_eof    = 0;

  jfc_func_status = -1;

  jfc_argc = 0;
  return 0;
}

static int jfc_comp(void)
{
  int comment_state, trueline, slut;


  jfc_head->function_code = (unsigned char *) jfc_ff_jfr;
  jfc_head->program_code = (unsigned char *) jfc_ff_jfr;
  jfc_ttypes[':']  = 3;
  jfc_ttypes['-']  = 11; /* Nu er '-' et ord (rettes midlertidigt */
          /* tilbage ved afkodning af weight).     */
  jfc_func_def = 0;

  comment_state = 0;
  slut = 0;
  while (slut == 0 && jfc_getline() != -1)
  { /* comment_state = 0; */

    trueline = 1;
    if (jfc_argv[0].type == 13) /* comment */
    { trueline = 0;
      if (comment_state == 0)
        jfc_error(144, jfc_argv[1].arg, JFE_WARNING);
      else
      { jfc_comment_handle();
        comment_state = 0;
      }
    }
    if (jfc_argc == 0)
      trueline = 0;
    if (trueline == 1)
    {
      jfc_progline_ins();
      comment_state = 1;
    }
  } /* while */

  if (jfc_pass == 2)
    jfc_end_ins();

  if (jfc_targc != 0)
    return jfc_error(11, jfc_empty, JFE_FATAL);

  return 0;
}

static int jfc_close(void)
{
    if (jfc_memory != NULL)
      free(jfc_memory);
    if (jfc_stack != NULL)
      free(jfc_stack);
    if (jfc_argline != NULL)
      free(jfc_argline);
    if (jfc_argv != NULL)
      free(jfc_argv);
    if (jfc_source != NULL)
      fclose(jfc_source);
    if (jfc_erfile != stdout)
      fclose(jfc_erfile);
    if (jfc_dest != NULL)
      fclose(jfc_dest);
  return -1;
}

int jfw2r_convert(char *de_fname, char *so_fname,
                  char *er_fname, int er_append,
                  int maxstack, int maxargline, int margc,
                  int err_message_mode, int err_line_mode)
{
  long ms;

  jfc_err_message_mode = err_message_mode;
  jfc_err_line_mode = err_line_mode;
  if (er_fname == NULL || strlen(er_fname) == 0)
    jfc_erfile = stdout;
  else
  { if (er_append == 0)
      jfc_erfile = fopen(er_fname, "w");
    else
      jfc_erfile = fopen(er_fname, "a");
    if (jfc_erfile == NULL)
    { jfc_erfile = stdout;
      jfc_error(1, er_fname, JFE_FATAL);
      return jfc_close();
    }
  }

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

  if (maxstack != 0)
    jfc_maxstack = maxstack;
  if ((jfc_stack = (struct jfc_stack_desc *)
                   malloc(sizeof(struct jfc_stack_desc) * jfc_maxstack))
      == NULL)
  { jfc_error(6, jfc_empty, JFE_FATAL);
    return jfc_close();
  }

  if (maxargline != 0)
    jfc_maxargline = maxargline;
  if ((jfc_argline = (char *) malloc(jfc_maxargline + 10)) == NULL)
  { jfc_error(6, jfc_empty, JFE_FATAL);
    return jfc_close();
  }

  if ((jfc_source = fopen(so_fname, "rb")) == NULL)
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

  jfc_pass = 1;
  jfc_synonyms = (struct jfw_synonym_desc *) jfc_memory;
  jfc_init();
  if (jfc_gl_error_mode != JFE_FATAL)
    jfc_comp();

  if (jfc_gl_error_mode != JFE_FATAL)
  { jfc_pass = 2;
    jfc_synonyms = (struct jfw_synonym_desc *) jfc_memory;
    rewind(jfc_source);
    jfc_init();
    jfc_comp();
  }
  if (jfc_source != NULL)
    fclose(jfc_source);
  jfc_source = NULL;

  if (jfc_err_count == 0)
    jfc_save(de_fname);
  if (jfc_memory != NULL)
    free(jfc_memory);
  if (jfc_stack != NULL)
    free(jfc_stack);
  if (jfc_argline != NULL)
    free(jfc_argline);
  if (jfc_argv != NULL)
    free(jfc_argv);

  if (jfc_erfile != stdout)
  { fclose(jfc_erfile);
    jfc_erfile = NULL;
  }

  if (jfc_err_count != 0)
    return 1;

  return 0;
}

static int jfc_save(char *fname)
{
  struct jfr_ehead_desc jf_ehead;
  char ctekst[4] = "jfr";

  strcpy(jf_ehead.check, ctekst);
  strcpy(jfc_head->check, ctekst);
  jf_ehead.version = jfc_head->version = 200;

  jfc_head->a_size =
     JFR_COMMENT_SIZE * jfc_head->comment_c
   + JFR_DOMAIN_SIZE * jfc_head->domain_c
   + JFR_ADJECTIV_SIZE * jfc_head->adjectiv_c
   + JFR_VAR_SIZE * jfc_head->var_c
   + JFR_ARRAY_SIZE * jfc_head->array_c
   + JFR_FZVAR_SIZE * jfc_head->fzvar_c
   + JFR_LIMIT_SIZE * jfc_head->limit_c
   + JFR_HEDGE_SIZE * jfc_head->hedge_c
   + JFR_RELATION_SIZE * jfc_head->relation_c
   + JFR_OPERATOR_SIZE * jfc_head->operator_c
   + JFR_FUNCTION_SIZE * jfc_head->function_c
   + JFR_FUNCARG_SIZE  * jfc_head->funcarg_c
   + sizeof(float) * jfc_head->arrayval_c
   + jfc_head->com_block_c
   + jfc_head->funccode_c
   + jfc_head->program_c;
  strcpy(jf_ehead.title, jfc_head->title);
  jf_ehead.comment_no = jfc_head->comment_no;
  jf_ehead.vbytes     = jfc_head->vbytes;
  jf_ehead.a_size     = jfc_head->a_size;
  jf_ehead.comment_c  = jfc_head->comment_c;
  jf_ehead.f_prog_comment = jfc_head->f_prog_comment;
  jf_ehead.domain_c   = jfc_head->domain_c;
  jf_ehead.adjectiv_c = jfc_head->adjectiv_c;
  jf_ehead.f_ivar_no  = jfc_head->f_ivar_no;
  jf_ehead.ivar_c     = jfc_head->ivar_c;
  jf_ehead.f_ovar_no  = jfc_head->f_ovar_no;
  jf_ehead.ovar_c     = jfc_head->ovar_c;
  jf_ehead.f_lvar_no  = jfc_head->f_lvar_no;
  jf_ehead.lvar_c     = jfc_head->lvar_c;
  jf_ehead.var_c      = jfc_head->var_c;
  jf_ehead.array_c    = jfc_head->array_c;
  jf_ehead.arrayval_c = jfc_head->arrayval_c;
  jf_ehead.fzvar_c    = jfc_head->fzvar_c;
  jf_ehead.limit_c    = jfc_head->limit_c;
  jf_ehead.hedge_c    = jfc_head->hedge_c;
  jf_ehead.relation_c = jfc_head->relation_c;
  jf_ehead.operator_c = jfc_head->operator_c;
  jf_ehead.function_c = jfc_head->function_c;
  jf_ehead.funcarg_c  = jfc_head->funcarg_c;
  jf_ehead.com_block_c= jfc_head->com_block_c;
  jf_ehead.funccode_c = jfc_head->funccode_c;
  jf_ehead.program_c  = jfc_head->program_c;
  jf_ehead.arg_1_c    = jfc_head->arg_1_c = 0;
  jf_ehead.arg_2_c    = jfc_head->arg_2_c = 0;

  if ((jfc_dest = fopen(fname, "wb")) == NULL)
    return jfc_error(1, fname, JFE_FATAL);

  if (fwrite((char *) &jf_ehead, JFR_EHEAD_SIZE, 1, jfc_dest) != 1)
    return jfc_error(3, fname, JFE_FATAL);

  if (fwrite((char *) jfc_head->domains,
          jfc_head->a_size, 1, jfc_dest) != 1)
      return jfc_error(3, jfc_empty, JFE_FATAL);

  fclose(jfc_dest);
  jfc_dest = NULL;
  return 0;
}
