  /*********************************************************************/
  /*                                                                   */
  /* jfr2hlib.c  Version 2.04  Copyright (c) 1999-2000 Jan E. Mortensen*/
  /*                                                                   */
  /* JFR-to-HTML (JavaScript) converter.                               */
  /* Converts a compiled jfs-program to a HTML-file, with code         */
  /* converted to JavaScript.                                          */
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
#include "jfr2hlib.h"

#define JFR2HTM_MAX_TEXT 256

static struct jfg_sprog_desc jfr2htm_spdesc;
static void *jfr_head;

static int jfr2htm_digits = 5;
static int jfr2htm_js_file = 0;
static int jfr2htm_no_check = 0;
static int jfr2htm_use_args = 1;
static int jfr2htm_label_mode;
static int jfr2htm_main_comment = 0;
static float jfr2htm_max_conf = 1.0;
static char jfr2htm_conf_text[256] = "Conf.:";
static char jfr2htm_prefix_txt[256] = "";
static int jfr2htm_use_ssheet = 0;
static char jfr2htm_ssheet[256] = "";

static char jfr2htm_ds_text[256]; /* global var to div/span-text */

static char *jfr2htm_text = NULL;
static int   jfr2htm_maxtext = 512;

static char jfr2htm_spaces[] = "    ";
static char t_info[256];

static struct jfg_limit_desc jfr2htm_limits[256];


struct pladr_desc { short first;
                    short last;
                    char  type;  /* 0: hedge, 1: relation,             */
                                 /* 2: adjectiv.                       */
                    unsigned char id;
                  };
/* Bruges til at finde et objekts pl-funktion i arrayene x og y.       */

static struct pladr_desc *jfr2htm_pl_adr = NULL;
static int jfr2htm_ff_pl_adr = 0;


/* Data til at afgoere om der behoeves fuz/defuz-funcs for variable */
struct jfr2htm_varuse_desc { char rvar;   /* read domain-var */
                             char wvar;   /* write domain-var */
                             char rfzvar; /* read fuzzy-var */
                             char wfzvar; /* write fuzy-var */
                             char aarray; /* create adjective-array */
                             char vhelp;  /* create help-alert */
                           };
static struct jfr2htm_varuse_desc *jfr2htm_varuse = NULL;


/* Hvilke 1-arg-funktioner, 2-arg-funktioner, var-funcs benyttes */
static int jfr2htm_sfunc_use[20];
static int jfr2htm_dfunc_use[20];
static int jfr2htm_vfunc_use[20];
static int jfr2htm_switch_use;
static int jfr2htm_tot_validate;
static int jfr2htm_iif_use;
static int jfr2htm_radiouse;

#define jfr2htm_LTYPES_MAX 128
static unsigned char jfr2htm_ltypes[jfr2htm_LTYPES_MAX]; /* 0: switch, 1: while */
static int jfr2htm_ff_ltypes;

struct jfr2htm_between_desc { int var_no;
                          int rano_1;
                          int rano_2;
                        };
#define jfr2htm_BETWEEN_MAX 256
static struct jfr2htm_between_desc jfr2htm_betweens[jfr2htm_BETWEEN_MAX];
static int jfr2htm_ff_betweens;

static struct jfg_tree_desc *jfr2htm_tree;
static int    jfr2htm_maxtree = 128;

static int    jfr2htm_stacksize = 64;
static int    jfr2htm_overwrite = 0;

#define JFR2HTM_MAX_WORDS 256
static const char *jfr2htm_words[JFR2HTM_MAX_WORDS];

struct jfr2htm_tarea_desc
{
  char name[32];
  char label[128];
  int cols;
  int rows;
};
#define JFR2HTM_MAX_TEXTAREA 32
static struct jfr2htm_tarea_desc jfr2htm_textareas[JFR2HTM_MAX_TEXTAREA];
static int jfr2htm_ff_textarea = 0;

struct jfr2htm_ptxt_desc
{
  char name[32];
  int dtype;
};
#define JFR2HTM_MAX_PTXT 32
static struct jfr2htm_ptxt_desc jfr2htm_ptxts[JFR2HTM_MAX_PTXT];
static int jfr2htm_ff_ptxt = 0;

/* describtion of a form:      */
struct jfr2htm_fdesc_struct
{ int lab_size; /* label-size form or actual fieldset.                   */
  int label_mode;
  int fs_mode;  /* 0 : no field-sets, write everything in a single pass. */
                /* 1 : field-sets.                                       */
  int head_mode; /* 0: standard-table,                                   */
                 /* 1: multi-coloumn-table and all variables in fieldset */
                 /*    are radio-buttons from same domain. Write labels  */
                 /*    as table-headers.                                 */
  int head_f_adjective_no;
  int head_adjective_c;
  int table_mode;  /* 0: table,                                          */
                   /* 1: multicolumn-table,                              */
                   /* 2: mulitcolumn with header-line.                   */

  int fs_no;     /* field-set no.                                        */
  int no_legend;
  int no_fieldset;
  int tarea_started; /* 1: the last output-variable was a fildset-variable. */
};
static struct jfr2htm_fdesc_struct jfr2htm_fdesc;

struct jfr2htm_varg_struct
{
  int va_type; /* variable type */
  /* va_type == JFR2HTM_VT_STANDARD: */
  int ft_type; /* field type */
  int com_label;
  int help_button;
  int conf;
  int no_nl;
  /* va_type == JFR2HTM_VT_FIELDSET: */
  /* int com_label; */
};

static struct jfr2htm_varg_struct jfr2htm_vargs;

static char *jfr2htm_comment = NULL;
static int jfr2htm_comm_size = 512;

static FILE *jfr2htm_oph = NULL;
static FILE *jfr2htm_opj = NULL;
static FILE *jfr2htm_op  = NULL;
static FILE *jfr2htm_ip  = NULL;
static FILE *jfr2htm_sout = NULL;

static const char jf_empty[] = " ";

static const char *jfr2htm_fixed[] =
{
  "function PREFIXrmm(v, mi, ma)",     /* rounds v to [mi, ma] */
  "{var r = v;",
  " if (r < mi) r = mi;",
  " if (r > ma) r = ma;",
  " return r",
  "}",
  "function PREFIXrmin(v,mi)",
  "{ if (v<mi) return mi\n  else return v}",
  "function PREFIXrmax(v,ma)",
  "{ if (v>ma) return ma\n  else return v}",
  "function PREFIXcut(a,v)",
  "{ if (a<v) return 0.0\n  else return a}",
  "function PREFIXr01(v)",
  "{ return PREFIXrmm(v,0.0,1.0)}",

  "<!-- JFS END -->"
};

/*----------------------------------------------------------------------*/
/* Functions to check input:                                            */

static const char *jfr2htm_t_fval[] =  /* validate a number with isNaN() check */
{
  "function PREFIXf_validate(elem, minval, maxval)",
  "{ var res = 0;",
  "  if (isNaN(elem.value))",
  "  { res = 1",
  "  }",
  "  else",
  "  { if (elem.value != PREFIXrmm(elem.value, minval, maxval))",
  "    { res = 2;",
  "    }",
  "  }",
  "  return res;",
  "}",

  "<!-- JFS END -->",
};

static const char *jfr2htm_t_no_fval[] =  /* validate a number without isNaN() check */
{
  "function PREFIXf_validate(elem, minval, maxval)",
  "{ var res = 0;",
  "  if (elem.value != PREFIXrmm(elem.value, minval, maxval))",
  "    res = 2;",
  "  return res;",
  "}",

  "<!-- JFS END -->",
};

static const char *jfr2htm_t_val[] =
{
  "function PREFIXvalidate(elem, minval, maxval)",
  "{ var res = PREFIXf_validate(elem, minval, maxval);",
  "  if (res == 1)",
  "  { window.alert(elem.name+\"=\"+elem.value+\" is not a legal number\");",
  "    elem.select();",
  "    elem.focus();",
  "  };",
  "  if (res == 2)",
  "  { window.alert(\"Invalid \"+elem.name+\". Value out of range: \"+",
  "       minval+\"<=\"+elem.name+\"<=\"+maxval);",
  "    elem.select()",
  "    elem.focus()",
  "  }",
  "  PREFIXclr_output();",
  "  return res;",
  "}",

  "<!-- JFS END -->",
};

/*------------------------------------------------------------------*/

static const char *jfr2htm_t_iif[] =
{
  "function PREFIXiif(c,e1,e2)",
  "{ var r=PREFIXr01(c);",
  "  return r*e1+(1.0-r)*e2}",

  "<!-- JFS END -->"

};

static const char *jfr2htm_t_whichRadio[] =
{
  "function PREFIXwhichRadio(radioBtn)",
  "{ for (var i=0; i < radioBtn.length; i++)",
  "  {  if (radioBtn[i].checked)",
  "        return i",
  "  }",
  "  return \"none\"",
  "}",
  "<!-- JFS END -->"
};

static const char *jfr2htm_t_plcalc[] =
{
  "function PREFIXpl_calc(xv, first, last)",
  "{var a; var r; var m",
  " if (xv < PREFIXx[first] || (xv == PREFIXx[first] && PREFIXexcl[first]==0))",
  "   r = PREFIXy[first] ",
  " else if (xv > PREFIXx[last]) r = PREFIXy[last] ",
  " else",
  " { for (m = first; m < last; m++)",
  "   { if (xv < PREFIXx[m+1] || (xv == PREFIXx[m+1] && PREFIXexcl[m+1]==0))",
  "     { if (PREFIXx[m] == PREFIXx[m+1]) r = PREFIXy[m+1] ",
  "       else",
  "       { a = (PREFIXy[m+1]-PREFIXy[m]) / (PREFIXx[m+1]-PREFIXx[m]);",
  "         r = a * xv + PREFIXy[m] - a * PREFIXx[m];",
  "       }",
  "       break;",
  " } } }",
  " return r;",
  "}",

  "<!-- JFS END -->",
};

static char jfr2htm_t_ediv[16];
static char jfr2htm_t_espan[16];

static const char jfr2htm_t_end[]      = "<!-- JFS END -->";
static const char jfr2htm_t_begin[]    = "<!-- JFS BEGIN -->";
static const char jfr2htm_t_stack[]    = "stack";
static const char jfr2htm_t_java[]     = "javascript";
static const char jfr2htm_t_textarea[] = "textarea";
static const char jfr2htm_t_printf[]   = "printf";
static const char jfr2htm_t_fprintf[]  = "fprintf";
static const char jfr2htm_t_alert[]    = "alert";
static const char jfr2htm_t_stdout[]   = "stdout";

static const char *jfr2htm_t_labels[] =
{ "label_left",  /* never */
  "label_left",
  "label_above",
  "label_above",
  "label_table",
  "label_table",
};

static const char *jfr2htm_t_tables[] =
{
  "table_2cl",
  "table_mcl",
  "table_mclhead",
};

#define JFE_WARNING 0
#define JFE_ERROR   1
#define JFE_FATAL   2

static int jfr2htm_errcount = 0;

struct jfr_err_desc { int eno;
                      const char *text;
                    };

static struct jfr_err_desc jfr_err_texts[] =
   { {  0, " "},
     {  1, "Cannot open file:"},
     {  2, "Error reading from file:"},
     {  3, "Error writing to file:"},
     {  4, "Not a jfr-file:"},
     {  5, "Wrong version:"},
     {  6, "Cannot allocate memory to:"},
     {  9, "Illegal number:"},
     { 10, "Value out of domain-range:"},
     { 11, "Unexpected EOF."},
     { 13, "Undefined adjective:"},
     {301, "statement to long."},
     {302, "jfg-tree to small to hold statement."},
     {303, "Stack-overflow (jfg-stack)."},
     {304, "program-id (pc) is not the start of a statement."},
     {1000,"HTML file corupted. No <!-- JFR BEGIN -->. Run in overwrite mode."},
     {1007,"Too many block-levels (switch/while-blocks). Max 128."},
     {1008,"Extern-statement ignored."},
     {1009,"Too many different between-expresions. Max 256."},
     {1010,"Illegal argument to 'extern textarea'-statement:"},
     {1011,"Too few arguments to 'extern textarea'-statement."},
     {1012,"Syntax error in 'extern printf' argument:"},
     {1013,"Unknown variable in 'extern printf' or 'extern alert' statement:"},
     {1014,"Unknown 'extern printf' or 'extern alert' specifier:"},
     {1015,"No adjectives bound to:"},
     {1016,"Wrong number of variables in 'extern printf' or 'extern alert' statement."},
     {1017," 'extern printf' statement, but no 'extern textarea' statement."},
     {1018,"Too many destinations in 'extern fprintf' statements. max 32."},
     {1019,"Too many 'extern textarea' statements. max 32."},
     {1020,"Errors in program."},
     {1021,"Field type TEXT_ADJ is only for output variables:"},
     {1022,"Field type CHECKBOX only possible if exactly 2 adjectives:"},
     {1023,"Field type not supported in current version of JFR2HTM:"},
     {1024,"Field type CHECKBOX only supported for input variables:"},
     {1025,"Field type not supported without adjectives:"},
     {1026,"Label mode not supported: "},
     {1027,"Variable type not supported: "},
     {9999, "unknown error."}
   };

static int jf_error(int errno, const char *name, int mode);
static void jfr2htm_dst_write(char *dest, const char *dstype, const char *clstxt);
static char *jfr2htm_divt(const char *clstxt);
static char *jfr2htm_spant(const char *clstxt);
static void jf_ftoa(char *txt, float f);
static void jfr2htm_float(float f);
static char *jfr2htm_ttrunc(char *txt, int len);
static void jfr2htm_pfloat(float f);
static void jfr2htm_read_comment(int comment_no);
static void jfr2htm_write_comment(int comment_no, int use_br);
static void jfr2htm_write_fround(void);
static int jfr2htm_pl_adr_add(int typ, int id, int ant);
static int jfr2htm_pl_find(int typ, int id);
static int jfr2htm_pl_write(void);  /* write constant arrays to pl-functions */
static int  jfr2htm_var_find(const char *name);
static void jfr2htm_var_write(void);
static void jfr2htm_fzvar_write(void);
static void jfr2htm_hedges_write(void);
static void jfr2htm_relations_write(void);
static void jfr2htm_op1_write(int op, float arg);
static void jfr2htm_operators_write(void);
static void jfr2htm_aaray_write(void);
static void jfr2htm_vhelp_write(void);
static void jfr2htm_sfunc_write(void);
static void jfr2htm_dfunc_write(void);
static void jfr2htm_vfunc_write(int vno, int fno);
static int  jfr2htm_expr_check(unsigned short id);
static int  jfr2htm_printf_check(const char *jfr2htm_words[], int a);
static int  jfr2htm_program_check(void);
static void jfr2htm_between_write(void);
static void jfr2htm_vround_write(void);
static void jfr2htm_var2index_write(void);
static void jfr2htm_fuz_write(void);  /* fuzzification-functions */
static void jfr2htm_defuz_write(void);
static void jfr2htm_leaf_write(int id);

static void jfr2htm_vget_write(void);
static void jfr2htm_vcget_write(void);
static void jfr2htm_vput_write(void);
static void jfr2htm_init_write(void);
static void jfr2htm_form2var_write(void);
static void jfr2htm_var2form_write(void);

static void jfr2htm_printf_write(const char *dest, const char *words[], int count, int sword);
static int jfr2htm_run_write(void);
static int jfr2htm_vget(struct jfg_var_desc *vdesc, int vno, int fset);
static void jfr2htm_jfs_write(void);
static int jfr2htm_head_write(void);
static int jfr2htm_subst(char *d, const char *s, const char *ds, const char *ss);
static void jfr2htm_t_write(const char **txts);

static void jfr2htm_iface_functions_write(void);
static void jfr2htm_form_functions_write(void);
static int jfr2htm_program_write(char * jfname);

static int jfr2htm_buf_cmp(const char *t);
static int jfr2htm_head_copy(int mode);
static void jfr2htm_body_write(void);
static void jfr2htm_body_copy(int mode);
static void jfr2htm_formvar_write(int vno, int is_ovar,
                                  int lab_size, int head_mode, int on_nl);
static void jfr2htm_form_test(void);
static int jfr2htm_fset_write(int rel_from_v, int is_ovar);
static void jfr2htm_form_write(void);
static void jfr2htm_end_write(void);
static void jfr2htm_end_copy(void);


/*************************************************************************/
/* Hjaelpe-funktioner                                                    */
/*************************************************************************/

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
  { fprintf(jfr2htm_sout, "WARNING %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    m = 0;
  }
  else
  { if (eno != 0)
    { fprintf(jfr2htm_sout, "*** error %d: %s %s\n", eno, jfr_err_texts[e].text, name);
      jfr2htm_errcount++;
      if (mode == JFE_FATAL)
      { if (jfr2htm_pl_adr != NULL)
          free(jfr2htm_pl_adr);
        if (jfr2htm_varuse != NULL)
          free(jfr2htm_varuse);
        if (jfr2htm_comment != NULL)
          free(jfr2htm_comment);
        if (eno != 0)
          fprintf(jfr2htm_sout, "\n*** PROGRAM ABORTED! ***\n");
      }
    }
    m = -1;
  }
  return m;
}

static void jfr2htm_dst_write(char *dest, const char *dstype, const char *clstxt)
{
   /* writes a DIV or SPAN tag to dest */

   if (jfr2htm_use_ssheet == 1)
     sprintf(dest, "<%s CLASS=\"%s\">", dstype, clstxt);
   else
     dest[0] = '\0';
}

static char *jfr2htm_divt(const char *clstxt)
{
  /* writes a DIV-tag to the global var jfr2htm_ds_text.  */
  jfr2htm_dst_write(jfr2htm_ds_text, "DIV", clstxt);
  return jfr2htm_ds_text;
}

static char *jfr2htm_spant(const char *clstxt)
{
  /* writes a SPAN-tag to the global var jfr2htm_ds_text.  */
  jfr2htm_dst_write(jfr2htm_ds_text, "SPAN", clstxt);
  return jfr2htm_ds_text;
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
  ep = dp + jfr2htm_digits - 1;
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

static char *jfr2htm_ttrunc(char *t, int len)
{
  char *w;

  w = t;
  if (w[0] == '"')
    w++;
  if (w[strlen(w) - 1] == '"')
    w[strlen(w) - 1] = '\0';
  if (strlen(w) >= len)
    w[len - 1] = '\0';
  return w;  
}

static void jfr2htm_float(float f)
{
  char txt[60];

  jf_ftoa(txt, f);
  fprintf(jfr2htm_op, "%s", txt);
}

static void jfr2htm_pfloat(float f)
{
  char txt[60];

  jf_ftoa(txt, f);
  if (txt[0]=='-')
    fprintf(jfr2htm_op, "(%s)", txt);
  else
    fprintf(jfr2htm_op, "%s", txt);
}

static void jfr2htm_read_comment(int comment_no)
{
  int read;
  char *ttxt;

  if (comment_no >= 0)
  { read = 0;
    while (read == 0)
    { if (jfg_comment(jfr2htm_comment, jfr2htm_comm_size, jfr_head, comment_no)
          == 301)
      { /* jfr2htm_comment to short */
        ttxt = jfr2htm_comment;
        jfr2htm_comment = (char *) malloc(2 * jfr2htm_comm_size);
        if (jfr2htm_comment == NULL)
        { jfr2htm_comment = ttxt;
          jfr2htm_comment[jfr2htm_comm_size - 1] = '\0';
          read = 1;
        }
        else
        { free(ttxt);
          jfr2htm_comm_size = 2 * jfr2htm_comm_size;
        }
      }
      else
        read = 1;
    }
  }
  else
    jfr2htm_comment[0] = '\0';
}

static void jfr2htm_write_comment(int comment_no, int cmode)
{
  /* <mode> is one of  0: comment written to output with '\n'-newlines,
                       1:     -"-                        newlines replaced
                           by a space,
                       2: comment written to the string vcom,
   */
  int m, br_written, sb_written;
  char ttxt[4];

  br_written = 0;
  sb_written = 0;
  ttxt[1] = ttxt[2] = '\0';
  if (comment_no >= 0)
  { jfr2htm_read_comment(comment_no);
    for (m = 0; jfr2htm_comment[m] != '\0'; m++)
    { if (cmode == 2 && sb_written == 0)
      { fprintf(jfr2htm_op, "vcom+=\"");
        sb_written = 1;
      }
      if (jfr2htm_comment[m] == '\n' || jfr2htm_comment[m] == '\r')
      { if (br_written == 0 || br_written == jfr2htm_comment[m])
        { if (cmode == 1)
            fprintf(jfr2htm_op, "\n "); /*fprintf(jfr2htm_op, "<BR>\n"); */
          else
          if (cmode == 0)
            fprintf(jfr2htm_op, "\n");
          else
          if (cmode == 2)
          { fprintf(jfr2htm_op, "\\n\";\n"); /* write '\n";' */
            sb_written = 0;
          }
          br_written = jfr2htm_comment[m];
        }
      }
      else
      { br_written = 0;
        ttxt[0] = jfr2htm_comment[m];
        fprintf(jfr2htm_op, "%s", ttxt);
      }
    }
    if (cmode == 2 && sb_written == 1)
      fprintf(jfr2htm_op, "\\n\";\n"); /* write '\n";' */
  }
}

void jfr2htm_write_fround(void)
{
  int a;

  fprintf(jfr2htm_op, "function %sfround(v) {\n", jfr2htm_prefix_txt);
  fprintf(jfr2htm_op, "var r=(Math.round(v*1");
  for (a=1; a < jfr2htm_digits; a++)
    fprintf(jfr2htm_op, "0");
  fprintf(jfr2htm_op, ".0))/1");
  for (a=1; a< jfr2htm_digits; a++)
    fprintf(jfr2htm_op, "0");
  fprintf(jfr2htm_op, ".0;\n");
  fprintf(jfr2htm_op, "return r;\n}\n");
}

/************************************************************************/
/* functions to plf-handling:                                           */

static int jfr2htm_pl_adr_add(int typ, int id, int ant)
{
  jfr2htm_pl_adr[jfr2htm_ff_pl_adr].type = typ;
  jfr2htm_pl_adr[jfr2htm_ff_pl_adr].id = id;
  if (jfr2htm_ff_pl_adr == 0)
    jfr2htm_pl_adr[jfr2htm_ff_pl_adr].first = 0;
  else
    jfr2htm_pl_adr[jfr2htm_ff_pl_adr].first
      = jfr2htm_pl_adr[jfr2htm_ff_pl_adr - 1].last + 1;
  jfr2htm_pl_adr[jfr2htm_ff_pl_adr].last
    = jfr2htm_pl_adr[jfr2htm_ff_pl_adr].first + ant - 1;
  jfr2htm_ff_pl_adr++;
  return 0;
}

static int jfr2htm_pl_find(int typ, int id)
{
  int m;

  for (m = 0; m < jfr2htm_ff_pl_adr; m++)
  { if (jfr2htm_pl_adr[m].type == typ && jfr2htm_pl_adr[m].id == id)
      return m;
  }
  return -1;
}

static int jfr2htm_pl_write(void)  /* write constant arrays to pl-functions */
{
  int m, a, i, t, mode, plf_c, s, fundet;
  struct jfg_hedge_desc hdesc;
  struct jfg_relation_desc rdesc;
  struct jfg_adjectiv_desc adesc;

  plf_c = 1; /* allocate extra to avoid problems if no plf's */
  for (mode = 0; mode < 2; mode++)
  { if (mode == 1)
    { s = sizeof(struct pladr_desc) * plf_c;
      jfr2htm_pl_adr = (struct pladr_desc *) malloc(s);
      if (jfr2htm_pl_adr == NULL)
        return jf_error(6, jfr2htm_spaces, JFE_ERROR);
    }
    for (m = 0; m < jfr2htm_spdesc.hedge_c; m++)
    { jfg_hedge(&hdesc, jfr_head, m);
      if (hdesc.type == JFS_HT_LIMITS)
      { if (mode == 0)
          plf_c++;
        else
          jfr2htm_pl_adr_add(0, m, hdesc.limit_c);
      }
    }
    for (m = 0; m < jfr2htm_spdesc.relation_c; m++)
    { jfg_relation(&rdesc, jfr_head, m);
      if (mode == 0)
        plf_c++;
      else
        jfr2htm_pl_adr_add(1, m, rdesc.limit_c);
    }
    for (m = 0; m < jfr2htm_spdesc.adjectiv_c; m++)
    { jfg_adjectiv(&adesc, jfr_head, m);
      if (adesc.limit_c > 0)
      { if (mode == 0)
          plf_c++;
        else
          jfr2htm_pl_adr_add(2, m, adesc.limit_c);
      }
    }
  }
  if (jfr2htm_ff_pl_adr == 0) /* overfloedige?? */
  { fprintf(jfr2htm_op, "var %sx = new Array(1)\n", jfr2htm_prefix_txt);
    fprintf(jfr2htm_op, "var %sy = new Array(1)\n", jfr2htm_prefix_txt);
    fprintf(jfr2htm_op, "var %sexcl = new Array(1)\n", jfr2htm_prefix_txt);
  }
  else
  { fprintf(jfr2htm_op, "var %sx = new Array(%d)\n", jfr2htm_prefix_txt,
            jfr2htm_pl_adr[jfr2htm_ff_pl_adr-1].last + 1);
    fprintf(jfr2htm_op, "var %sy = new Array(%d)\n", jfr2htm_prefix_txt,
            jfr2htm_pl_adr[jfr2htm_ff_pl_adr-1].last + 1);
    fprintf(jfr2htm_op, "var %sexcl = new Array(%d)\n", jfr2htm_prefix_txt,
            jfr2htm_pl_adr[jfr2htm_ff_pl_adr-1].last + 1);
  }
  /* declare text-destinations: */
  fprintf(jfr2htm_op, "var %st_stdout = \"\";\n", jfr2htm_prefix_txt);
  fprintf(jfr2htm_op, "var %st_alert = \"\";\n", jfr2htm_prefix_txt);
  for (m = 0; m < jfr2htm_ff_ptxt; m++)
  { if (jfr2htm_ptxts[m].dtype == 0)
      fprintf(jfr2htm_op, "var %st_%s = \"\";\n",
              jfr2htm_prefix_txt, jfr2htm_ptxts[m].name);
  }
  for (m = 0; m < jfr2htm_ff_textarea; m++)
  { if (strcmp(jfr2htm_textareas[m].name, jfr2htm_t_stdout) == 0)
      fundet = 1;
    else
      fundet = 0;
    for (a = 0; fundet == 0 && a < jfr2htm_ff_ptxt; a++)
    { if (strcmp(jfr2htm_textareas[m].name, jfr2htm_ptxts[a].name) == 0)
        fundet = 1;
    }
    if (fundet == 0) /* text-area without 'extern printf' to area */
    { fprintf(jfr2htm_op, "var %st_%s = \"\";\n",
              jfr2htm_prefix_txt, jfr2htm_textareas[m].name);
    }
  }

  t = 0;
  for (m = 0; m < jfr2htm_ff_pl_adr; m++)
  { switch (jfr2htm_pl_adr[m].type)
    { case 0:
        jfg_hlimits(jfr2htm_limits, jfr_head, jfr2htm_pl_adr[m].id);
        break;
      case 1:
        jfg_rlimits(jfr2htm_limits, jfr_head, jfr2htm_pl_adr[m].id);
        break;
      case 2:
        jfg_alimits(jfr2htm_limits, jfr_head, jfr2htm_pl_adr[m].id);
        break;
    }
    for (i = 0; i <= jfr2htm_pl_adr[m].last - jfr2htm_pl_adr[m].first; i++)
    { fprintf(jfr2htm_op, "%sx[%d]=", jfr2htm_prefix_txt,
                          jfr2htm_pl_adr[m].first + i);
      jfr2htm_float(jfr2htm_limits[i].limit);
      fprintf(jfr2htm_op, "; %sy[%d]=", jfr2htm_prefix_txt,
             jfr2htm_pl_adr[m].first + i);
      jfr2htm_float(jfr2htm_limits[i].value);
      fprintf(jfr2htm_op, ";");
      fprintf(jfr2htm_op, " %sexcl[%d]=%d;", jfr2htm_prefix_txt,
              jfr2htm_pl_adr[m].first + i, jfr2htm_limits[i].exclusiv);

      t++;
      if ((t % 3) == 0)
        fprintf(jfr2htm_op, "\n");
    }
  }
  fprintf(jfr2htm_op, "\n");
  return 0;
}

static int  jfr2htm_var_find(const char *name)
{
  int m, res;
  struct jfg_var_desc vdesc;

  res = -1;
  for (m = 0; res == -1 && m < jfr2htm_spdesc.var_c; m++)
  { jfg_var(&vdesc, jfr_head, m);
    if (strcmp(vdesc.name, name) == 0)
      res = m;
  }
  return res;
}

static void jfr2htm_var_write(void)
{
  struct jfg_var_desc vdesc;
  int m;

  for (m = 0; m < jfr2htm_spdesc.var_c; m++)
  { jfg_var(&vdesc, jfr_head, m);
    fprintf(jfr2htm_op, "var %sv%d; ", jfr2htm_prefix_txt, m);
    fprintf(jfr2htm_op, "var %sc%d; ", jfr2htm_prefix_txt, m);
    fprintf(jfr2htm_op, "var %scs%d; ", jfr2htm_prefix_txt, m);
    if (m % 3 == 0 && m != 0)
      fprintf(jfr2htm_op, "\n");
  }
  fprintf(jfr2htm_op, "\n");
}

static void jfr2htm_array_write(void)
{
  struct jfg_array_desc adesc;
  int m;

  for (m = 0; m < jfr2htm_spdesc.array_c; m++)
  { jfg_array(&adesc, jfr_head, m);
    fprintf(jfr2htm_op, "var %sa%d = new Array(%d);\n", jfr2htm_prefix_txt,
            m, adesc.array_c);
  }
}

static void jfr2htm_fzvar_write(void)
{
  int m;

  for (m = 0; m < jfr2htm_spdesc.fzvar_c; m++)
  { fprintf(jfr2htm_op, "var %sf%d; ", jfr2htm_prefix_txt, m);
    if (m % 5 == 0 && m != 0)
      fprintf(jfr2htm_op, "\n");
  }
  fprintf(jfr2htm_op, "\n");
}

static void jfr2htm_aaray_write(void)
{
  int m, a;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;

  for (m = 0; m < jfr2htm_spdesc.var_c; m++)
  { if (jfr2htm_varuse[m].aarray == 1)
    { jfg_var(&vdesc, jfr_head, m);
      if (vdesc.fzvar_c > 0)
      { fprintf(jfr2htm_op, "%saar%d = new Array(", jfr2htm_prefix_txt, m);
        for (a = 0; a < vdesc.fzvar_c; a++)
        { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
          if (a != 0)
            fprintf(jfr2htm_op, ",\n");
          fprintf(jfr2htm_op, "\"%s\"", adesc.name);
        }
        fprintf(jfr2htm_op, ")\n");
      }
    }
  }
}

static void jfr2htm_vhelp_write(void)
{
  int m;
  struct jfg_var_desc vdesc;

  for (m = 0; m <jfr2htm_spdesc.var_c; m++)
  { if (jfr2htm_varuse[m].vhelp == 1)
    { jfg_var(&vdesc, jfr_head, m);
      fprintf(jfr2htm_op, "function %svhelp%d() {", jfr2htm_prefix_txt, m);
      fprintf(jfr2htm_op, "var vcom=\"\";\n");
      jfr2htm_write_comment(vdesc.comment_no, 2);
      fprintf(jfr2htm_op, "alert(vcom);\n}\n");
    }
  }
}

static void jfr2htm_hedges_write(void)
{
  int m, id;
  struct jfg_hedge_desc hdesc;

  for (m = 0; m < jfr2htm_spdesc.hedge_c; m++)
  { jfg_hedge(&hdesc, jfr_head, m);
    fprintf(jfr2htm_op, "function %sh%d(v) {", jfr2htm_prefix_txt, m);
    if (hdesc.type != JFS_HT_LIMITS)
      fprintf(jfr2htm_op, "var a=%sr01(v); ", jfr2htm_prefix_txt);
    switch (hdesc.type)
    { case JFS_HT_NEGATE:
        fprintf(jfr2htm_op, "return (1.0-a)};\n");
        break;
      case JFS_HT_POWER:
        fprintf(jfr2htm_op, "return Math.pow(a," );
        jfr2htm_float(hdesc.hedge_arg);
        fprintf(jfr2htm_op, ")};\n");
        break;
      case JFS_HT_SIGMOID:
        fprintf(jfr2htm_op,
              "return (1.0/(1.0+Math.pow(2.71828,-(20.0*a-10)*");
        jfr2htm_float(hdesc.hedge_arg);
        fprintf(jfr2htm_op, ")))};\n");
        break;
      case JFS_HT_ROUND:
        fprintf(jfr2htm_op, "if (a>=");
        jfr2htm_float(hdesc.hedge_arg);
        fprintf(jfr2htm_op, ") return 1.0\n else return 0.0};\n");
        break;
      case JFS_HT_YNOT:
        fprintf(jfr2htm_op, "return Math.pow(1.0-Math.pow(a,");
        jfr2htm_float(hdesc.hedge_arg);
        fprintf(jfr2htm_op, "),1.0/");
        jfr2htm_float(hdesc.hedge_arg);
        fprintf(jfr2htm_op, ")};\n");
        break;
      case JFS_HT_BELL:
        fprintf(jfr2htm_op, "if (a<=");
        jfr2htm_float(hdesc.hedge_arg);
        fprintf(jfr2htm_op, ") return Math.pow(a,2.0-(a/" );
        jfr2htm_float(hdesc.hedge_arg);
        fprintf(jfr2htm_op, "))\n else return Math.pow(a,(a+" );
        jfr2htm_pfloat(hdesc.hedge_arg);
        fprintf(jfr2htm_op, "-2.0)/(2.0*");
        jfr2htm_float(hdesc.hedge_arg);
        fprintf(jfr2htm_op, "-2.0))};\n ");
        break;
      case JFS_HT_TCUT:
        fprintf(jfr2htm_op, "if (a>");
        jfr2htm_float(hdesc.hedge_arg);
        fprintf(jfr2htm_op, ") return 1.0\n else return a};\n ");
        break;
      case JFS_HT_BCUT:
        fprintf(jfr2htm_op, "if (a<");
        jfr2htm_float(hdesc.hedge_arg);
        fprintf(jfr2htm_op, ") return 0.0\n else return a};\n");
        break;
      case JFS_HT_LIMITS:
        fprintf(jfr2htm_op, " return %spl_calc(v,", jfr2htm_prefix_txt);
        id = jfr2htm_pl_find(0, m);
        fprintf(jfr2htm_op, "%d,%d)};\n", jfr2htm_pl_adr[id].first,
                        jfr2htm_pl_adr[id].last);
        break;
    }
  }
}

static void jfr2htm_relations_write(void)
{
  struct jfg_relation_desc rdesc;
  int m, id;

  for (m = 0; m < jfr2htm_spdesc.relation_c; m++)
  { jfg_relation(&rdesc, jfr_head, m);
    fprintf(jfr2htm_op, "function %sr%d(x,y) { return ", jfr2htm_prefix_txt, m);
    if ((rdesc.flags & JFS_RF_HEDGE) != 0)
      fprintf(jfr2htm_op, "%sh%d(", jfr2htm_prefix_txt,rdesc.hedge_no);
    id = jfr2htm_pl_find(1, m);
    fprintf(jfr2htm_op, "%spl_calc(x-y,%d,%d)",
            jfr2htm_prefix_txt, jfr2htm_pl_adr[id].first, jfr2htm_pl_adr[id].last);
    if ((rdesc.flags & JFS_RF_HEDGE) != 0)
      fprintf(jfr2htm_op, ")};\n");
    else
      fprintf(jfr2htm_op,"};\n");
  }
}

static void jfr2htm_op1_write(int op, float arg)
{
  switch (op)
  { case JFS_FOP_MIN:
      fprintf(jfr2htm_op, "if (a<b) r=a\n else r=b;\n");
      break;
    case JFS_FOP_MAX:
      fprintf(jfr2htm_op, "if (a>b) r=a\n else r=b;\n");
      break;
    case JFS_FOP_PROD:
      fprintf(jfr2htm_op, "r=a*b;\n");
      break;
    case JFS_FOP_PSUM:
      fprintf(jfr2htm_op, "r=a+b-a*b;\n");
      break;
    case JFS_FOP_AVG:
      fprintf(jfr2htm_op, "r=(a+b)/2.0;\n");
      break;
    case JFS_FOP_BSUM:
      fprintf(jfr2htm_op, "r=%sr01(a+b);\n", jfr2htm_prefix_txt);
      break;
    case JFS_FOP_NEW:
      fprintf(jfr2htm_op, "r=b;\n");
      break;
    case JFS_FOP_MXOR:
      fprintf(jfr2htm_op, "if (a>=b) r=a-b\n else r=b-a;\n");
      break;
    case JFS_FOP_SPTRUE:
      fprintf(jfr2htm_op,
              "r=(2.0*a-1.0)*(2.0*a-1.0)*(2.0*b-1.0)*(2.0*b-1.0);\n");
      break;
    case JFS_FOP_SPFALSE:
      fprintf(jfr2htm_op,
              "r=1.0-(2.0*a-1.0)*(2.0*a-1.0)*(2.0*b-1.0)*(2.0*b-1.0);\n");
      break;
    case JFS_FOP_SMTRUE:
      fprintf(jfr2htm_op, "r=1.0-16.0*(a-a*a)*(b-b*b);\n");
      break;
    case JFS_FOP_SMFALSE:
      fprintf(jfr2htm_op, "r=16.0*(a-a*a)*(b-b*b);\n");
      break;
    case JFS_FOP_R0:
      fprintf(jfr2htm_op, "r=0.0;\n");
      break;
    case JFS_FOP_R1:
      fprintf(jfr2htm_op, "r=a*b-a-b+1;\n");
      break;
    case JFS_FOP_R2:
      fprintf(jfr2htm_op, "r=b-a*b;\n");
      break;
    case JFS_FOP_R3:
      fprintf(jfr2htm_op, "r=1.0-a;\n");
      break;
    case JFS_FOP_R4:
      fprintf(jfr2htm_op, "r=a-a*b;\n");
      break;
    case JFS_FOP_R5:
      fprintf(jfr2htm_op, "r=1.0-b;\n");
      break;
    case JFS_FOP_R6:
      fprintf(jfr2htm_op, "r=a+b-2.0*a*b;\n");
      break;
    case JFS_FOP_R7:
      fprintf(jfr2htm_op, "r=1.0-a*b;\n");
      break;
    case JFS_FOP_R8:
      fprintf(jfr2htm_op, "r=a*b;\n");
      break;
    case JFS_FOP_R9:
      fprintf(jfr2htm_op, "r=1.0-a-b+2.0*a*b;\n");
      break;
    case JFS_FOP_R10:
      fprintf(jfr2htm_op, "r=b;\n");
      break;
    case JFS_FOP_R11:
      fprintf(jfr2htm_op, "r=1.0-a+a*b;\n");
      break;
    case JFS_FOP_R12:
      fprintf(jfr2htm_op, "r=a;\n");
      break;
    case JFS_FOP_R13:
      fprintf(jfr2htm_op, "r=a*b-b+1.0;\n");
      break;
    case JFS_FOP_R14:
      fprintf(jfr2htm_op, "r=a+b-a*b;\n");
      break;
    case JFS_FOP_R15:
      fprintf(jfr2htm_op, "r=1.0;\n");
      break;
    case JFS_FOP_HAMAND:
      fprintf(jfr2htm_op, "r=(a*b)/(");
      jfr2htm_float(arg);
      fprintf(jfr2htm_op, "+(1.0-");
      jfr2htm_pfloat(arg);
      fprintf(jfr2htm_op, ")*(a+b-a*b));\n");
      break;
    case JFS_FOP_HAMOR:
      fprintf(jfr2htm_op, "r=(a+b-(2.0-");
      jfr2htm_pfloat(arg);
      fprintf(jfr2htm_op, ")*a*b)/(1.0-(1.0-");
      jfr2htm_pfloat(arg);
      fprintf(jfr2htm_op, ")*a*b);\n");
      break;
    case JFS_FOP_YAGERAND:
      fprintf(jfr2htm_op, "r=Math.pow(Math.pow(1.0-a,");
      jfr2htm_float(arg);
      fprintf(jfr2htm_op, ")+Math.pow(1.0-b,");
      jfr2htm_float(arg);
      fprintf(jfr2htm_op, "),1.0/");
      jfr2htm_float(arg);
      fprintf(jfr2htm_op, ");\nif (r>1.0) r=0.0\n else r=1.0-r;\n");
      break;
    case JFS_FOP_YAGEROR:
      fprintf(jfr2htm_op, "r=Math.pow(Math.pow(a,");
      jfr2htm_float(arg);
      fprintf(jfr2htm_op, ")+Math.pow(b,");
      jfr2htm_float(arg);
      fprintf(jfr2htm_op, "),1.0/");
      jfr2htm_float(arg);
      fprintf(jfr2htm_op, ");\nif (r>1.0) r=1.0;\n");
      break;
    case JFS_FOP_BUNION:
      fprintf(jfr2htm_op, "r=a+b-1.0; if (r<0.0) r=0.0;\n");
      break;
    case JFS_FOP_SIMILAR:
      fprintf(jfr2htm_op, "if (a==b) r=1.0\n else if (a<b) r=a/b\n else r=b/a;\n");
      break;
    default:
      break;
  }
}

static void jfr2htm_operators_write(void)
{
  struct jfg_operator_desc odesc;
  int m;

  for (m = 0; m < jfr2htm_spdesc.operator_c; m++)
  { jfg_operator(&odesc, jfr_head, m);
    fprintf(jfr2htm_op, "function %so%d(x,y) { var a=%sr01(x);\n",
                        jfr2htm_prefix_txt, m, jfr2htm_prefix_txt);
    fprintf(jfr2htm_op, "var b=%sr01(y);var r;", jfr2htm_prefix_txt);
    if (odesc.hedge_mode == JFS_OHM_ARG1 || odesc.hedge_mode == JFS_OHM_ARG12)
      fprintf(jfr2htm_op, "a=%sh%d(a);", jfr2htm_prefix_txt, odesc.hedge_no);
    if (odesc.hedge_mode == JFS_OHM_ARG2 || odesc.hedge_mode == JFS_OHM_ARG12)
      fprintf(jfr2htm_op, "b=%sh%d(b);", jfr2htm_prefix_txt, odesc.hedge_no);
    jfr2htm_op1_write(odesc.op_1, odesc.op_arg);
    if (odesc.op_2 != JFS_FOP_NONE && odesc.op_2 != odesc.op_1)
    { fprintf(jfr2htm_op, "var t=r;");
      jfr2htm_op1_write(odesc.op_2, odesc.op_arg);
      fprintf(jfr2htm_op, "r=t*(1.0-");
      jfr2htm_float(odesc.op_arg);
      fprintf(jfr2htm_op, ")+r*");
      jfr2htm_pfloat(odesc.op_arg);
      fprintf(jfr2htm_op, ";\n");
    }
    if (odesc.hedge_mode == JFS_OHM_POST)
      fprintf(jfr2htm_op, "r=%sh%d(r);", jfr2htm_prefix_txt, odesc.hedge_no);
    fprintf(jfr2htm_op, "return r};\n");
  }
}

static void jfr2htm_sfunc_write(void)
{
  int m;

  for (m = 0; m < 20; m++)
  { if (jfr2htm_sfunc_use[m] == 1)
    { fprintf(jfr2htm_op, "function %ss%d(a) {var r=0.0\n ",
              jfr2htm_prefix_txt, m);
      switch (m)
      { case JFS_SFU_COS:
          fprintf(jfr2htm_op, "r=Math.cos(a);");
          break;
        case JFS_SFU_SIN:
          fprintf(jfr2htm_op, "r=Math.sin(a);");
          break;
        case JFS_SFU_TAN:
          fprintf(jfr2htm_op, "r=Math.cos(a);if (r!=0.0) r=Math.sin(a)/r;");
          break;
        case JFS_SFU_ACOS:
          fprintf(jfr2htm_op, "if (a>=-1 && a<=1) r=Math.acos(a);");
          break;
        case JFS_SFU_ASIN:
          fprintf(jfr2htm_op, "if (a>=-1 && a<=1) r=Math.asin(a);");
          break;
        case JFS_SFU_ATAN:
          fprintf(jfr2htm_op, "r=Math.atan(a);");
          break;
        case JFS_SFU_LOG:
          fprintf(jfr2htm_op, "if (a>0) r=Math.log(a);");
          break;
        case JFS_SFU_FABS:
          fprintf(jfr2htm_op, "r=Math.abs(a);");
          break;
        case JFS_SFU_FLOOR:
          fprintf(jfr2htm_op, "r=Math.floor(a);");
          break;
        case JFS_SFU_CEIL:
          fprintf(jfr2htm_op, "r=Math.ceil(a);");
          break;
        case JFS_SFU_NEGATE:
          fprintf(jfr2htm_op, "r=-a;");
          break;
        case JFS_SFU_RANDOM:
          fprintf(jfr2htm_op, "r=Math.random()*a;");
          break;
        case JFS_SFU_SQR:
          fprintf(jfr2htm_op, "r=sqr(a);");
          break;
        case JFS_SFU_SQRT:
          fprintf(jfr2htm_op, "if (a>=0) r=sqrt(a);");
          break;
        case JFS_SFU_WGET:
          fprintf(jfr2htm_op, "if (%sglw>a) r=%sglw;",
                  jfr2htm_prefix_txt, jfr2htm_prefix_txt);
          break;
      }
      fprintf(jfr2htm_op, "return r};\n");
    }
  }
}

static void jfr2htm_dfunc_write(void)
{
  int m;

  for (m = 0; m < 20; m++)
  { if (jfr2htm_dfunc_use[m] == 1)
    { fprintf(jfr2htm_op, "function %sd%d(a,b) {var r=0.0\n ",
              jfr2htm_prefix_txt, m);
      switch (m)
      { case JFS_DFU_PLUS:
          fprintf(jfr2htm_op, "r=a+b;");
          break;
        case JFS_DFU_MINUS:
          fprintf(jfr2htm_op, "r=a-b;");
          break;
        case JFS_DFU_PROD:
          fprintf(jfr2htm_op, "r=a*b;");
          break;
        case JFS_DFU_DIV:
          fprintf(jfr2htm_op, "if (b!=0.0) r=a/b;");
          break;
        case JFS_DFU_POW:
          fprintf(jfr2htm_op,
                  "if ((a==0 && b<0) || (a<0 && b != Math.floor(b)))\n");
          fprintf(jfr2htm_op, "r=0.0\n else r=Math.pow(a,b);");
          break;
        case JFS_DFU_MIN:
          fprintf(jfr2htm_op, "r=Math.min(a,b);");
          break;
        case JFS_DFU_MAX:
          fprintf(jfr2htm_op, "r=Math.max(a,b);");
          break;
        case JFS_DFU_CUT:
          fprintf(jfr2htm_op, "if (a>b) r=a\n else r=0.0;");
          break;
        case JFS_ROP_EQ:
          fprintf(jfr2htm_op, "if (a==b) r=1.0;");
          break;
        case JFS_ROP_NEQ:
          fprintf(jfr2htm_op, "if (a!=b) r=1.0;");
          break;
        case JFS_ROP_GT:
          fprintf(jfr2htm_op, "if (a>b) r=1.0;");
          break;
        case JFS_ROP_GEQ:
          fprintf(jfr2htm_op, "if (a>=b) r=1.0;");
          break;
        case JFS_ROP_LT:
          fprintf(jfr2htm_op, "if (a<b) r=1.0;");
          break;
        case JFS_ROP_LEQ:
          fprintf(jfr2htm_op, "if (a<=b) r=1.0;");
          break;
      }
      fprintf(jfr2htm_op, "return r};\n");
    }
  }
}

static void jfr2htm_vfunc_write(int vno, int fno)
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
          jfr2htm_pfloat(ddesc.dmin);
        else
        { /* ((v%-dmin)/(dmax-dmin)) */
          fprintf(jfr2htm_op, "((%sv%d-", jfr2htm_prefix_txt, vno);
          jfr2htm_pfloat(ddesc.dmin);
          fprintf(jfr2htm_op, ")/");
          jfr2htm_float(ddesc.dmax - ddesc.dmin);
          fprintf(jfr2htm_op, ")");
        }
      }
      else
        fprintf(jfr2htm_op, "%sv%d", jfr2htm_prefix_txt, vno);
      break;
    case JFS_VFU_M_FZVAR:
      for (a = 0; a < vdesc.fzvar_c - 1; a++)
        fprintf(jfr2htm_op, "Math.max(%sf%d,", jfr2htm_prefix_txt,
                vdesc.f_fzvar_no + a);
      fprintf(jfr2htm_op, "%sf%d",
              jfr2htm_prefix_txt, vdesc.f_fzvar_no + vdesc.fzvar_c - 1);
      for (a = 0; a < vdesc.fzvar_c - 1; a++)
        fprintf(jfr2htm_op, ")");
      break;
    case JFS_VFU_S_FZVAR:
      fprintf(jfr2htm_op, "(");
      for (a = 0; a < vdesc.fzvar_c; a++)
      { if (a != 0)
          fprintf(jfr2htm_op, "+");
        fprintf(jfr2htm_op, "%sf%d", jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
      }
      fprintf(jfr2htm_op, ")");
      break;
    case JFS_VFU_DEFAULT:
      jfr2htm_pfloat(vdesc.default_val);
      break;
    case JFS_VFU_CONFIDENCE:
      fprintf(jfr2htm_op, "%sc%d", jfr2htm_prefix_txt, vno);
      break;
  }
}

static int jfr2htm_expr_check(unsigned short id)
{
  struct jfg_tree_desc *leaf;
  struct jfg_fzvar_desc fzvdesc;
  int m;

  leaf = &(jfr2htm_tree[id]);
  switch (leaf->type)
  { case JFG_TT_OP:
    case JFG_TT_UREL:
    case JFG_TT_ARGLIST:
      jfr2htm_expr_check(leaf->sarg_1);
      jfr2htm_expr_check(leaf->sarg_2);
      break;
    case JFG_TT_IIF:
      jfr2htm_iif_use = 1;
      jfr2htm_expr_check(leaf->sarg_1);
      jfr2htm_expr_check(leaf->sarg_2);
      break;
    case JFG_TT_HEDGE:
    case JFG_TT_UFUNC:
    case JFG_TT_ARVAL:
      jfr2htm_expr_check(leaf->sarg_1);
      break;
    case JFG_TT_SFUNC:
      jfr2htm_sfunc_use[leaf->op] = 1;
      jfr2htm_expr_check(leaf->sarg_1);
      break;
    case JFG_TT_DFUNC:
      jfr2htm_dfunc_use[leaf->op] = 1;
      jfr2htm_expr_check(leaf->sarg_1);
      jfr2htm_expr_check(leaf->sarg_2);
      break;
    case JFG_TT_VAR:
      jfr2htm_varuse[leaf->sarg_1].rvar = 1;
      break;
    case JFG_TT_FZVAR:
      jfg_fzvar(&fzvdesc, jfr_head, leaf->sarg_1);
      jfr2htm_varuse[fzvdesc.var_no].rfzvar = 1;
      break;
    case JFG_TT_BETWEEN:
      jfr2htm_varuse[leaf->sarg_1].rvar = 1;
      for (m = 0; m < jfr2htm_ff_betweens; m++)
      { if (jfr2htm_betweens[m].var_no == leaf->sarg_1
            && jfr2htm_betweens[m].rano_1 == leaf->sarg_2
            && jfr2htm_betweens[m].rano_2 == leaf->op)
   break;
      }
      if (m == jfr2htm_ff_betweens)
      { if (jfr2htm_ff_betweens >= jfr2htm_BETWEEN_MAX)
          return jf_error(1009, jfr2htm_spaces, JFE_ERROR);
        jfr2htm_betweens[jfr2htm_ff_betweens].var_no = leaf->sarg_1;
        jfr2htm_betweens[jfr2htm_ff_betweens].rano_1 = leaf->sarg_2;
        jfr2htm_betweens[jfr2htm_ff_betweens].rano_2 = leaf->op;
        jfr2htm_ff_betweens++;
      }
      break;
    case JFG_TT_VFUNC:
      jfr2htm_vfunc_use[leaf->op] = 1;
      if (leaf->op == JFS_VFU_DNORMAL)
        jfr2htm_varuse[leaf->sarg_1].rvar = 1;
      if (leaf->op == JFS_VFU_M_FZVAR || leaf->op == JFS_VFU_S_FZVAR)
        jfr2htm_varuse[leaf->sarg_1].rfzvar = 1;
      break;
    default:
      break;
  }
  return 0;
}

static int jfr2htm_printf_check(const char *words[], int a)
{
  int sw, m, p, vno, id;
  char etxt[4];
  const char *w;

  sw = 1;
  if (strcmp(words[0], jfr2htm_t_fprintf) == 0)
  {
    char *wd; /* printf-destination */
    wd = (char *)(malloc (strlen(words[sw]) + 1));
    if (wd != NULL) strcpy(wd, words[sw]);
    if (strlen(wd) > 31)
      wd[31] = '\0';
    id = -1;
    for (m = 0; id == -1 && m < jfr2htm_ff_ptxt; m++)
    { if (strcmp(wd, jfr2htm_ptxts[m].name) == 0)
        id = m;
    }
    if (id == -1)
    { if (jfr2htm_ff_ptxt >= JFR2HTM_MAX_PTXT)
        return jf_error(1018, jfr2htm_spaces, JFE_ERROR);
      else
      { strcpy(jfr2htm_ptxts[jfr2htm_ff_ptxt].name, wd);
        jfr2htm_ptxts[jfr2htm_ff_ptxt].dtype = 0;
        jfr2htm_ff_ptxt++;
      }
    }
    sw++;
    sw++; /* ignore ',' */
    free(wd);
  }
  w = words[sw]; /* format-string */
  p = 0;
  sw++;
  sw++; /* skip ',' */
  for (m = sw; m < a; m++)
  { vno = jfr2htm_var_find(words[m]);
    if (vno == -1)
      return jf_error(1013, words[m], JFE_ERROR);
    while (w[p] != '%' && w[p] != '\0')
      p++;
    if (w[p] == '%')
    { p++;
      if (w[p] == 'f')
        jfr2htm_varuse[vno].rvar = 1;
      else
      if (w[p] == 'a')
      { jfr2htm_varuse[vno].rvar = 1;
        jfr2htm_varuse[vno].aarray = 1;
      }
      else
      if (w[p] == 'c')
        jfr2htm_varuse[vno].rvar = 1;
      else
      { etxt[0] = '%';
        etxt[1] = w[p];
        etxt[2] = '\0';
        return jf_error(1014, etxt, JFE_ERROR);
      }
    }
    else
      return jf_error(1012, w, JFE_ERROR);
    m++; /* step over comma */
  }
  return 0;
}

static int jfr2htm_program_check(void)
{
  int m, fu, res, a, ft, p=0;
  unsigned short c, i, e;
  struct jfg_statement_desc sdesc;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  struct jfg_fzvar_desc fzvdesc;
  struct jfg_function_desc fudesc;
  unsigned char *pc;

  for (m = 0; m < 20; m++)
  { jfr2htm_sfunc_use[m] = 0;
    jfr2htm_dfunc_use[m] = 0;
    jfr2htm_vfunc_use[m] = 0;
  }
  jfr2htm_switch_use = 0;
  jfr2htm_iif_use = 0;
  jfr2htm_ff_betweens = 0;
  jfr2htm_radiouse = 0;

  for (m = 0; m <jfr2htm_spdesc.var_c; m++)
  { jfr2htm_varuse[m].rvar   = 0;
    jfr2htm_varuse[m].wvar   = 0;
    jfr2htm_varuse[m].rfzvar = 0;
    jfr2htm_varuse[m].wfzvar = 0;
    jfr2htm_varuse[m].aarray  = 0;
    jfr2htm_varuse[m].vhelp   = 0;
    jfr2htm_vget(&vdesc, m, 0);
    ft = -1;
    if (m >= jfr2htm_spdesc.f_ivar_no
        && m < jfr2htm_spdesc.f_ivar_no + jfr2htm_spdesc.ivar_c)
    { jfr2htm_varuse[m].wvar   = 1;
      if (jfr2htm_vargs.va_type == JFR2HTM_VT_STANDARD)
        ft = jfr2htm_vargs.ft_type;
    }
    if (m >= jfr2htm_spdesc.f_ovar_no
        && m < jfr2htm_spdesc.f_ovar_no + jfr2htm_spdesc.ovar_c)
    { if (jfr2htm_vargs.va_type == JFR2HTM_VT_STANDARD)
        ft = jfr2htm_vargs.ft_type;
      jfr2htm_varuse[m].rvar = 1;
      jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
      if (ft == JFR2HTM_FT_TEXT_ADJ || ft == JFR2HTM_FT_PULLDOWN
          || ft == JFR2HTM_FT_RADIO || ft == JFR2HTM_FT_RADIO_M)
      { if (vdesc.fzvar_c == 0)
          return jf_error(1015, vdesc.name, JFE_ERROR);
        jfr2htm_varuse[m].aarray = 1;
      }
    }
    if (ft == JFR2HTM_FT_RADIO || ft == JFR2HTM_FT_RADIO_M)
      jfr2htm_radiouse = 1;
    if (jfr2htm_vargs.help_button == 1 && vdesc.comment_no != -1)
      jfr2htm_varuse[m].vhelp = 1;
  }

  for (fu = 0; fu < jfr2htm_spdesc.function_c + 1; fu++)
  { if (fu < jfr2htm_spdesc.function_c)
    { jfg_function(&fudesc, jfr_head, fu);
      pc = fudesc.pc;
    }
    else
      pc = jfr2htm_spdesc.pc_start;

    jfg_statement(&sdesc, jfr_head, pc);
    while (sdesc.type != JFG_ST_EOP
           && (!(sdesc.type == JFG_ST_STEND && sdesc.sec_type == 2)))
    { switch (sdesc.type)
      { case JFG_ST_IF:
          res = jfg_if_tree(jfr2htm_tree, jfr2htm_maxtree, &c, &i, &e,
                            jfr_head, pc);
          if (res != 0)
            return jf_error(res, jfr2htm_spaces, JFE_ERROR);
          switch (sdesc.sec_type)
          { case JFG_SST_FZVAR:
              jfg_fzvar(&fzvdesc, jfr_head, sdesc.sarg_1);
              jfr2htm_varuse[fzvdesc.var_no].rfzvar = 1;  /* i comp-operator */
              jfr2htm_varuse[fzvdesc.var_no].wfzvar = 1;
              jfr2htm_expr_check(c);
               break;
            case JFG_SST_VAR:
            case JFG_SST_INC:
              jfr2htm_varuse[sdesc.sarg_1].rvar = 1;  /* i comp-operator */
              jfr2htm_varuse[sdesc.sarg_1].wvar = 1;
              jfr2htm_expr_check(c);
              jfr2htm_expr_check(e);
              break;
            case JFG_SST_ARR:
              jfr2htm_expr_check(c);
              jfr2htm_expr_check(i);
              jfr2htm_expr_check(e);
              break;
            case JFG_SST_PROCEDURE:
            case JFG_SST_RETURN:
            case JFG_SST_FUARG:
              jfr2htm_expr_check(c);
              jfr2htm_expr_check(e);
              break;
            case JFG_SST_EXTERN:
              jfr2htm_expr_check(c);
              a = jfg_a_statement(jfr2htm_words, JFR2HTM_MAX_WORDS, jfr_head, pc);
              if (a > 0)
              { if (strcmp(jfr2htm_words[0], jfr2htm_t_printf) == 0
                    || strcmp(jfr2htm_words[0], jfr2htm_t_alert) == 0
                    || strcmp(jfr2htm_words[0], jfr2htm_t_fprintf) == 0)
                { if (jfr2htm_printf_check(jfr2htm_words, a) != 0)
                    return -1;
                }
                else
                if (strcmp(jfr2htm_words[0], jfr2htm_t_textarea) == 0)
                {
                  if (a >= 4)
                  {
                    char *w0, *w;
                    /* extern textarea [name] label cols rows; */
                    /*           0        1     2     3    4   */
                    if (jfr2htm_ff_textarea >= JFR2HTM_MAX_TEXTAREA)
                      return jf_error(1019, jfr2htm_spaces, JFE_ERROR);
                    if (a == 4)
                    { p = 0;
                      strcpy(jfr2htm_textareas[jfr2htm_ff_textarea].name,
                             jfr2htm_t_stdout);
                    }
                    else
                    {
                      char *wd, *wd0; /* name */
                      wd0 = (char *)(malloc (strlen(jfr2htm_words[p]) + 1));
                      if (wd0 != NULL) strcpy(wd0, jfr2htm_words[p]);

                      wd = jfr2htm_ttrunc(wd0, 32);
                      p = 1;
                      strcpy(jfr2htm_textareas[jfr2htm_ff_textarea].name, wd);
                      free(wd0);
                    }
                    p++;
                    w0 = (char *)(malloc (strlen(jfr2htm_words[p]) + 1));
                    if (w0 != NULL) strcpy(w0, jfr2htm_words[p]);
                    w = jfr2htm_ttrunc(w0, 128);
                    strcpy(jfr2htm_textareas[jfr2htm_ff_textarea].label, w);
                    p++;
                    jfr2htm_textareas[jfr2htm_ff_textarea].cols
                      = atoi(jfr2htm_words[p]);  /* columns */
                    if (jfr2htm_textareas[jfr2htm_ff_textarea].cols < 2)
                      return jf_error(1010, jfr2htm_words[p], JFE_ERROR);
                    p++;
                    jfr2htm_textareas[jfr2htm_ff_textarea].rows
                      = atoi(jfr2htm_words[p]);
                    if (jfr2htm_textareas[jfr2htm_ff_textarea].cols < 2)
                      return jf_error(1010, jfr2htm_words[p], JFE_ERROR);
                    jfr2htm_ff_textarea++;
                    free(w0);
                  }
                  else
                    return jf_error(1011, jfr2htm_spaces, JFE_ERROR);
                }
              }
              break;
            default:
              jfr2htm_expr_check(c);
          }
          break;
        case JFG_ST_CASE:
          res = jfg_if_tree(jfr2htm_tree, jfr2htm_maxtree, &c, &i, &e, jfr_head, pc);
          if (res != 0)
            return jf_error(res, jfr2htm_spaces, JFE_ERROR);
          jfr2htm_expr_check(c);
          break;
        case JFG_ST_SWITCH:
          jfr2htm_switch_use = 1;
          break;
        case JFG_ST_WHILE:
          jfr2htm_switch_use = 1;
          res = jfg_if_tree(jfr2htm_tree, jfr2htm_maxtree, &c, &i, &e, jfr_head, pc);
          if (res != 0)
            return jf_error(res, jfr2htm_spaces, JFE_ERROR);
          jfr2htm_expr_check(c);
          break;
        case JFG_ST_WSET:
          res = jfg_if_tree(jfr2htm_tree, jfr2htm_maxtree, &c, &i, &e, jfr_head, pc);
          if (res != 0)
            return jf_error(res, jfr2htm_spaces, JFE_ERROR);
          jfr2htm_expr_check(c);
          break;
        default:
          break;
      }
      pc = sdesc.n_pc;
      jfg_statement(&sdesc, jfr_head, pc);
    }
  }
  return 0;
}

static void jfr2htm_between_write(void)
{
  int m, id;
  struct jfr2htm_between_desc *bd;
  struct jfg_adjectiv_desc adesc1;
  struct jfg_adjectiv_desc adesc2;
  struct jfg_adjectiv_desc adesc_tmp;
  struct jfg_var_desc vdesc;

  for (m = 0; m < jfr2htm_ff_betweens; m++)
  { bd = &(jfr2htm_betweens[m]);
    jfg_var(&vdesc, jfr_head, bd->var_no);
    jfg_adjectiv(&adesc1, jfr_head, vdesc.f_adjectiv_no + bd->rano_1);
    jfg_adjectiv(&adesc2, jfr_head, vdesc.f_adjectiv_no + bd->rano_2);
    fprintf(jfr2htm_op, "function %sb%d_%d_%d() { var r \n",
            jfr2htm_prefix_txt, bd->var_no, bd->rano_1, bd->rano_2);
    fprintf(jfr2htm_op, "if (%sv%d<", jfr2htm_prefix_txt, bd->var_no);
    jfr2htm_float(adesc1.trapez_start);
    fprintf(jfr2htm_op, ") {");


    if (adesc1.limit_c > 0)
    { id = jfr2htm_pl_find(2, vdesc.f_adjectiv_no + bd->rano_1);
      fprintf(jfr2htm_op, "r=%spl_calc(%sv%d,%d,%d);\n",
              jfr2htm_prefix_txt, jfr2htm_prefix_txt, bd->var_no,
              jfr2htm_pl_adr[id].first, jfr2htm_pl_adr[id].last);
    }
    else
    { if (bd->rano_1 == 0)
     fprintf(jfr2htm_op, " r=1.0;\n");
      else
      { /* {if (vval <= pre) fzv=0 else fzv=(vval-pre)/(center-pre);} */
        fprintf(jfr2htm_op, "if (%sv%d<=", jfr2htm_prefix_txt, bd->var_no);
        jfg_adjectiv(&adesc_tmp, jfr_head,
                     vdesc.f_adjectiv_no + bd->rano_1 - 1);
        jfr2htm_float(adesc_tmp.trapez_end);
        fprintf(jfr2htm_op, ") r=0.0\n else r=(%sv%d-",
                        jfr2htm_prefix_txt, bd->var_no);
        jfr2htm_pfloat(adesc_tmp.trapez_end);
        fprintf(jfr2htm_op, ")/");
        jfr2htm_float(adesc1.trapez_start-adesc_tmp.trapez_end);
        fprintf(jfr2htm_op, ";");
      }
      if (adesc1.flags & JFS_AF_HEDGE)
      { fprintf(jfr2htm_op, "r=%sh%d(r);\n",
                jfr2htm_prefix_txt, adesc1.h1_no);
      }
    }
    fprintf(jfr2htm_op, "}\n else if (%sv%d>", jfr2htm_prefix_txt, bd->var_no);
    jfr2htm_float(adesc2.trapez_end);
    fprintf(jfr2htm_op, ") {");


    if (adesc2.limit_c > 0)
    { id = jfr2htm_pl_find(2, vdesc.f_adjectiv_no + bd->rano_2);
      fprintf(jfr2htm_op, "r=%spl_calc(%sv%d,%d,%d);\n",
                      jfr2htm_prefix_txt, jfr2htm_prefix_txt, bd->var_no,
                      jfr2htm_pl_adr[id].first, jfr2htm_pl_adr[id].last);
    }
    else
    { if (bd->rano_2 == vdesc.fzvar_c - 1)
        fprintf(jfr2htm_op, "r=1.0;\n");
      else
      { /* {if (vval>=post) fzv=0 else fzv=(post-vval)/(post-center);} */
        jfg_adjectiv(&adesc_tmp, jfr_head,
                     vdesc.f_adjectiv_no + bd->rano_2 +1);
        fprintf(jfr2htm_op, "if (%sv%d>=",jfr2htm_prefix_txt, bd->var_no);
        jfr2htm_float(adesc_tmp.trapez_start);
        fprintf(jfr2htm_op, ") r=0.0\n else r=(");
        jfr2htm_float(adesc_tmp.trapez_start);
        fprintf(jfr2htm_op, "-%sv%d)/", jfr2htm_prefix_txt, bd->var_no);
        jfr2htm_float(adesc_tmp.trapez_start-adesc2.trapez_end);
        fprintf(jfr2htm_op, ";\n");
      }
      if (adesc2.flags & JFS_AF_HEDGE)
      { fprintf(jfr2htm_op, "r=%sh%d(r);\n", jfr2htm_prefix_txt, adesc2.h2_no);
      }
    }
    fprintf(jfr2htm_op, "}\n else r=1.0; return r}\n");
  }
}

static void jfr2htm_var2index_write(void)
{ /* write v2i-functions for variables. A v2i-function returns an index */
  /* for the adjective closest to the variables value                   */

  int vno, a;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  struct jfg_adjectiv_desc adesc;

  for (vno = 0; vno < jfr2htm_spdesc.var_c; vno++)
  { if (jfr2htm_varuse[vno].aarray == 1)
    { jfg_var(&vdesc, jfr_head, vno);
      jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
      if (vdesc.fzvar_c > 0)
      { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no);
        fprintf(jfr2htm_op,
                "function %sv2id%d() {var t; var r;\n", jfr2htm_prefix_txt, vno);
        fprintf(jfr2htm_op, "  t=0;r=Math.abs(%sv%d-", jfr2htm_prefix_txt, vno);
        jfr2htm_pfloat(adesc.center);
        fprintf(jfr2htm_op, ");\n");
        for (a = 1; a < vdesc.fzvar_c; a++)
        { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
          fprintf(jfr2htm_op, "  if (Math.abs(%sv%d-", jfr2htm_prefix_txt, vno);
          jfr2htm_pfloat(adesc.center);
          fprintf(jfr2htm_op, ")<r) {t=%d;r=Math.abs(%sv%d-",
                  a, jfr2htm_prefix_txt, vno);
          jfr2htm_pfloat(adesc.center);
          fprintf(jfr2htm_op, ");}\n");
        }
        fprintf(jfr2htm_op, "  return t;\n");
        fprintf(jfr2htm_op, "};\n\n");
      }
    }
  }
}

static void jfr2htm_fuz_write(void)  /* fuzzification-functions */
{
  int vno, a, id;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;
  struct jfg_adjectiv_desc adesc2;

  for (vno = 0; vno < jfr2htm_spdesc.var_c; vno++)
  { if (jfr2htm_varuse[vno].wvar == 1 && jfr2htm_varuse[vno].rfzvar == 1)
    { jfg_var(&vdesc, jfr_head, vno);
      fprintf(jfr2htm_op, "function %sfu%d() {if (%sv%d!=%sh) {\n",
              jfr2htm_prefix_txt, vno,
              jfr2htm_prefix_txt, vno, jfr2htm_prefix_txt);
      for (a = 0; a < vdesc.fzvar_c; a++)
      { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
        if (adesc.limit_c > 0)
        { id = jfr2htm_pl_find(2, vdesc.f_adjectiv_no + a);
          fprintf(jfr2htm_op, "%sf%d=%spl_calc(%sv%d,%d,%d);\n",
                  jfr2htm_prefix_txt, vdesc.f_fzvar_no + a,
                  jfr2htm_prefix_txt, jfr2htm_prefix_txt,
                  vno, jfr2htm_pl_adr[id].first, jfr2htm_pl_adr[id].last);
        }
        else
        { fprintf(jfr2htm_op, "if (%sv%d<=", jfr2htm_prefix_txt, vno);
          jfr2htm_float(adesc.trapez_start);
          if (a == 0)
            fprintf(jfr2htm_op, ") %sf%d=1.0 \n",
                    jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
          else
          { /* {if (vval <= pre) fzv=0\n else fzv=(vval-pre)/(center-pre);} */
            fprintf(jfr2htm_op, ") {if (%sv%d<=", jfr2htm_prefix_txt, vno);
            jfg_adjectiv(&adesc2, jfr_head, vdesc.f_adjectiv_no + a - 1);
            jfr2htm_float(adesc2.trapez_end);
            fprintf(jfr2htm_op, ") %sf%d=0.0\n else %sf%d=(%sv%d-",
                            jfr2htm_prefix_txt, vdesc.f_fzvar_no + a,
                            jfr2htm_prefix_txt, vdesc.f_fzvar_no + a,
                            jfr2htm_prefix_txt, vno);
            jfr2htm_pfloat(adesc2.trapez_end);
            fprintf(jfr2htm_op, ")/");
            jfr2htm_float(adesc.trapez_start-adesc2.trapez_end);
            fprintf(jfr2htm_op,"} ");
          }
          /* else if (vval <= trapez_end) fzv = 1.0;  */
          fprintf(jfr2htm_op, "else if (%sv%d<=", jfr2htm_prefix_txt, vno);
          jfr2htm_float(adesc.trapez_end);
          fprintf(jfr2htm_op, ") %sf%d=1.0\n ",
                  jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);

          fprintf(jfr2htm_op, "else ");
          if (a == vdesc.fzvar_c - 1)
            fprintf(jfr2htm_op, "%sf%d=1.0;\n",
                    jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
          else
          { /* {if (vval>=post) fzv=0\n else fzv=(post-vval)/(post-center);} */
            jfg_adjectiv(&adesc2, jfr_head, vdesc.f_adjectiv_no + a +1);
            fprintf(jfr2htm_op, "{if (%sv%d>=", jfr2htm_prefix_txt, vno);
            jfr2htm_float(adesc2.trapez_start);
            fprintf(jfr2htm_op, ") %sf%d=0.0\n else %sf%d=(",
                    jfr2htm_prefix_txt, vdesc.f_fzvar_no + a,
                    jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
            jfr2htm_float(adesc2.trapez_start);
            fprintf(jfr2htm_op, "-%sv%d)/", jfr2htm_prefix_txt, vno);
            jfr2htm_float(adesc2.trapez_start-adesc.trapez_end);
            fprintf(jfr2htm_op, "};\n");
          }
        }
        if (adesc.flags & JFS_AF_HEDGE)
        { if (adesc.h2_no == adesc.h1_no)
            fprintf(jfr2htm_op, "%sf%d=%sh%d(%sf%d);\n",
                    jfr2htm_prefix_txt, vdesc.f_fzvar_no + a,
                    jfr2htm_prefix_txt, adesc.h1_no,
                    jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
          else
          { fprintf(jfr2htm_op, "if (%sv%d<=", jfr2htm_prefix_txt, vno);
            jfr2htm_float(adesc.center);
            fprintf(jfr2htm_op, ") %sf%d=%sh%d(%sf%d)\n else %sf%d=%sh%d(%sf%d);\n",
                     jfr2htm_prefix_txt, vdesc.f_fzvar_no + a,
                     jfr2htm_prefix_txt, adesc.h1_no,
                     jfr2htm_prefix_txt, vdesc.f_fzvar_no + a,
                     jfr2htm_prefix_txt, vdesc.f_fzvar_no + a,
                     jfr2htm_prefix_txt, adesc.h2_no,
                     jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
          }
        }
      }
      if ((vdesc.flags & JFS_VF_NORMAL) != 0 && vdesc.fzvar_c > 0)
      { fprintf(jfr2htm_op, "var ns=");
        for (a = 0; a < vdesc.fzvar_c; a++)
        { if (a != 0)
            fprintf(jfr2htm_op, "+");
          fprintf(jfr2htm_op, "%sf%d", jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
        }
        fprintf(jfr2htm_op, ";\n");
        fprintf(jfr2htm_op, "if (ns!=0.0) {");
        if (vdesc.no_arg > 0.0)
        { fprintf(jfr2htm_op, "ns=");
          jfr2htm_float(vdesc.no_arg);
          fprintf(jfr2htm_op, "/ns;\n");
        }
        else
          fprintf(jfr2htm_op, "ns=%sc%d/ns;\n", jfr2htm_prefix_txt, vno);
        for (a = 0; a < vdesc.fzvar_c; a++)
          fprintf(jfr2htm_op, "%sf%d*=ns;\n",
                  jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
        fprintf(jfr2htm_op, "};");
      }
      fprintf(jfr2htm_op, "}};\n");
    }
  } /* for var */
}


static void jfr2htm_vround_write(void)
{
  int a, vno;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;
  struct jfg_domain_desc ddesc;

  for (vno = 0; vno < jfr2htm_spdesc.var_c; vno++)
  { if (jfr2htm_varuse[vno].wvar == 1 || jfr2htm_varuse[vno].rvar == 1)
    { jfg_var(&vdesc, jfr_head, vno);
      fprintf(jfr2htm_op, "function %svr%d() {", jfr2htm_prefix_txt, vno);
      jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
      switch (ddesc.type)
      { case JFS_DT_INTEGER:
          fprintf(jfr2htm_op, "%sv%d=Math.round(%sv%d);\n",
                  jfr2htm_prefix_txt, vno, jfr2htm_prefix_txt, vno);
          break;
        case JFS_DT_CATEGORICAL:
          jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no);
          fprintf(jfr2htm_op, "var res=");
          jfr2htm_float(adesc.center);
          fprintf(jfr2htm_op, "; var bdist=Math.abs(%sv%d-",
                  jfr2htm_prefix_txt, vno);
          jfr2htm_pfloat(adesc.center);
          fprintf(jfr2htm_op, ");\n");
          for (a = 1; a <vdesc.fzvar_c; a++)
          { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
            fprintf(jfr2htm_op, "if (Math.abs(%sv%d-", jfr2htm_prefix_txt, vno);
            jfr2htm_pfloat(adesc.center);
            fprintf(jfr2htm_op, ")<bdist) {res=");
            jfr2htm_float(adesc.center);
            fprintf(jfr2htm_op, "; bdist=Math.abs(%sv%d-",
                    jfr2htm_prefix_txt, vno);
            jfr2htm_pfloat(adesc.center);
            fprintf(jfr2htm_op, ")}\n");
          }
          fprintf(jfr2htm_op, "%sv%d=res;", jfr2htm_prefix_txt, vno);
          break;
        default:
          break;
      }
      if (ddesc.flags & JFS_DF_MINENTER)
      { if (ddesc.flags & JFS_DF_MAXENTER)
        { fprintf(jfr2htm_op, "%sv%d=%srmm(%sv%d,",
                  jfr2htm_prefix_txt, vno, jfr2htm_prefix_txt,
                  jfr2htm_prefix_txt, vno);
          jfr2htm_float(ddesc.dmin);
          fprintf(jfr2htm_op, ",");
          jfr2htm_float(ddesc.dmax);
        }
        else
        { fprintf(jfr2htm_op, "%sv%d=%srmin(%sv%d,",
                  jfr2htm_prefix_txt, vno, jfr2htm_prefix_txt,
                  jfr2htm_prefix_txt, vno);
          jfr2htm_float(ddesc.dmin);
        }
        fprintf(jfr2htm_op, ")");
      }
      else
      { if (ddesc.flags & JFS_DF_MAXENTER)
        { fprintf(jfr2htm_op, "%sv%d=%srmax(%sv%d,",
                  jfr2htm_prefix_txt, vno, jfr2htm_prefix_txt,
                  jfr2htm_prefix_txt, vno);
          jfr2htm_float(ddesc.dmax);
          fprintf(jfr2htm_op, ")");
        }
      }
      fprintf(jfr2htm_op, "}\n");
    }
  }
}

static void jfr2htm_defuz_write(void)
{
  int a, d, defuz_func, vno;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;
  struct jfg_domain_desc ddesc;

  for (vno = 0; vno < jfr2htm_spdesc.var_c; vno++)
  { if (jfr2htm_varuse[vno].wfzvar == 1 && jfr2htm_varuse[vno].rvar == 1)
    { jfg_var(&vdesc, jfr_head, vno);
      /* s=f0+f1+..+fn;             */
      fprintf(jfr2htm_op, "function %sdf%d(n) {if (n!=%sh) {var s=",
              jfr2htm_prefix_txt, vno, jfr2htm_prefix_txt);
      for (a = 0; a < vdesc.fzvar_c; a++)
      { if (a != 0)
          fprintf(jfr2htm_op, "+");
        if (vdesc.acut == 0.0)
          fprintf(jfr2htm_op, "%sf%d", jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
        else
        { fprintf(jfr2htm_op, "%scut(%sf%d,",
                  jfr2htm_prefix_txt, jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
          jfr2htm_float(vdesc.acut);
          fprintf(jfr2htm_op, ")");
        }
      }
      fprintf(jfr2htm_op, ";\nif (s==0.0) {%sc%d=0.0;%sv%d=",
               jfr2htm_prefix_txt, vno, jfr2htm_prefix_txt, vno);
      jfr2htm_float(vdesc.default_val);
      fprintf(jfr2htm_op, "}\n else {\n");

      /* c=max(f0,max(f1,..));   */
      fprintf(jfr2htm_op, "%sc%d=", jfr2htm_prefix_txt, vno);
      for (a = 0; a < vdesc.fzvar_c - 1; a++)
        fprintf(jfr2htm_op, "Math.max(%sf%d,",
                jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
      fprintf(jfr2htm_op, "%sf%d",
              jfr2htm_prefix_txt, vdesc.f_fzvar_no + vdesc.fzvar_c - 1);
      for (a = 0; a < vdesc.fzvar_c - 1; a++)
        fprintf(jfr2htm_op, ")");
      fprintf(jfr2htm_op, ";\n");

      defuz_func = vdesc.defuz_1;
      for (d = 0; d < 2; d++)
      { switch (defuz_func)
        { case JFS_VD_CENTROID:
            fprintf(jfr2htm_op, "var tael=");
            for (a = 0; a < vdesc.fzvar_c; a++)
            { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
              if (a != 0)
                fprintf(jfr2htm_op, "+");
              jfr2htm_pfloat(0.5*adesc.base*adesc.center);
              if (vdesc.acut == 0)
                fprintf(jfr2htm_op, "*%sf%d/s",
                        jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
              else
              { fprintf(jfr2htm_op, "*%scut(%sf%d,",
                        jfr2htm_prefix_txt,
                        jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
                jfr2htm_float(vdesc.acut);
                fprintf(jfr2htm_op, ")/s");
              }
            }
            fprintf(jfr2htm_op, ";\nvar naevn=");
            for (a = 0; a < vdesc.fzvar_c; a++)
            { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
              if (a != 0)
                fprintf(jfr2htm_op, "+");
              jfr2htm_pfloat(0.5*adesc.base);
              if (vdesc.acut == 0)
                fprintf(jfr2htm_op, "*%sf%d/s",
                        jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
              else
              { fprintf(jfr2htm_op, "*%scut(%sf%d,",
                        jfr2htm_prefix_txt,
                        jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
                jfr2htm_float(vdesc.acut);
                fprintf(jfr2htm_op, ")/s");
              }
            }
            fprintf(jfr2htm_op, ";\nif (naevn==0.0) {%sc%d=0.0; %sv%d=",
                    jfr2htm_prefix_txt, vno, jfr2htm_prefix_txt, vno);
            jfr2htm_float(vdesc.default_val);
            fprintf(jfr2htm_op, ";}\n else %sv%d=tael/naevn;\n",
                    jfr2htm_prefix_txt, vno);
            break;
          case JFS_VD_AVG:
            fprintf(jfr2htm_op, "%sv%d=", jfr2htm_prefix_txt, vno);
            for (a = 0; a < vdesc.fzvar_c; a++)
            { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
              if (a != 0)
                fprintf(jfr2htm_op, "+");
              jfr2htm_pfloat(adesc.center);
              fprintf(jfr2htm_op, "*");
              if (vdesc.acut == 0.0)
                fprintf(jfr2htm_op, "%sf%d/s",
                        jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
              else
              { fprintf(jfr2htm_op, "%scut(%sf%d,",
                        jfr2htm_prefix_txt,
                        jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
                jfr2htm_float(vdesc.acut);
                fprintf(jfr2htm_op, ")/s");
              }
            }
            fprintf(jfr2htm_op, ";");
            break;
          case JFS_VD_CMAX:
          case JFS_VD_FMAX:
          case JFS_VD_LMAX:
            jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no);
            fprintf(jfr2htm_op, "var f=%sf%d;", jfr2htm_prefix_txt, vdesc.f_fzvar_no);
            if (defuz_func != JFS_VD_LMAX)
            { fprintf(jfr2htm_op, "var b=");
              jfr2htm_float(adesc.center);
              fprintf(jfr2htm_op, ";");
            }
            if (defuz_func != JFS_VD_FMAX)
            { fprintf(jfr2htm_op, "var e=");
              jfr2htm_float(adesc.center);
              fprintf(jfr2htm_op, ";");
            }
            for (a = 1; a < vdesc.fzvar_c; a++)
            { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
              fprintf(jfr2htm_op, "if (%sf%d>f) {f=%sf%d;",
                      jfr2htm_prefix_txt, vdesc.f_fzvar_no + a,
                      jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
              if (defuz_func != JFS_VD_LMAX)
              { fprintf(jfr2htm_op, "b=");
                jfr2htm_float(adesc.center);
                fprintf(jfr2htm_op, ";");
              }
              if (defuz_func != JFS_VD_FMAX)
              { fprintf(jfr2htm_op, "e=");
                jfr2htm_float(adesc.center);
                fprintf(jfr2htm_op, ";");
              }
              fprintf(jfr2htm_op, "};\n");
              if (defuz_func != JFS_VD_FMAX)
              { fprintf(jfr2htm_op, "if (%sf%d==f) e=",
                        jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
                jfr2htm_float(adesc.center);
                fprintf(jfr2htm_op, ";");
              }
            }
            if (defuz_func == JFS_VD_CMAX)
              fprintf(jfr2htm_op, "%sv%d=(b+e)/2.0;\n", jfr2htm_prefix_txt, vno);
            if (defuz_func == JFS_VD_FMAX)
              fprintf(jfr2htm_op, "%sv%d=b;\n", jfr2htm_prefix_txt, vno);
            if (defuz_func == JFS_VD_LMAX)
              fprintf(jfr2htm_op, "%sv%d=e;\n", jfr2htm_prefix_txt, vno);
            break;
          case JFS_VD_FIRST:
            jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
            if ((ddesc.flags & JFS_DF_MINENTER) != 0
                && (ddesc.flags & JFS_DF_MAXENTER) != 0)
            { /* v=f*(dmax-dmin)+dmin; */
              fprintf(jfr2htm_op, "%sv%d=%sf%d*(",
                      jfr2htm_prefix_txt, vno,
                      jfr2htm_prefix_txt, vdesc.f_fzvar_no);
              jfr2htm_float(ddesc.dmax);
              fprintf(jfr2htm_op, "-");
              jfr2htm_pfloat(ddesc.dmin);
              fprintf(jfr2htm_op, ")+");
              jfr2htm_pfloat(ddesc.dmin);
              fprintf(jfr2htm_op, ";\n");
            }
            else
              fprintf(jfr2htm_op, "%sv%d=%s%d;\n",
                      jfr2htm_prefix_txt, vno,
                      jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
            break;
        }
        if (d == 0)
        { if (vdesc.defuz_2 == vdesc.defuz_1)
            d = 7;
          else
          { fprintf(jfr2htm_op, "var tv=%sv%d;\n", jfr2htm_prefix_txt, vno);
            defuz_func = vdesc.defuz_2;
          }
        }
        else
        { fprintf(jfr2htm_op, "%sv%d=tv*(1.0-", jfr2htm_prefix_txt, vno);
          jfr2htm_pfloat(vdesc.defuz_arg);
          fprintf(jfr2htm_op, ")+%sv%d*", jfr2htm_prefix_txt, vno);
          jfr2htm_pfloat(vdesc.defuz_arg);
          fprintf(jfr2htm_op, ";\n");
        }
      }
      fprintf(jfr2htm_op, "%svr%d()}",
              jfr2htm_prefix_txt, vno); /* slut paa else-blok */
      fprintf(jfr2htm_op, "}};\n");
    }
  } /* for (m in vars */
}

static void jfr2htm_leaf_write(int id)
{
  struct jfg_tree_desc *leaf;
  struct jfg_array_desc adesc;

  leaf = &(jfr2htm_tree[id]);
  switch (leaf->type)
  { case JFG_TT_OP:
      fprintf(jfr2htm_op, "%so%d(", jfr2htm_prefix_txt, leaf->op);
      jfr2htm_leaf_write(leaf->sarg_1);
      fprintf(jfr2htm_op, ",");
      jfr2htm_leaf_write(leaf->sarg_2);
      fprintf(jfr2htm_op, ")");
      break;
    case JFG_TT_HEDGE:
      fprintf(jfr2htm_op, "%sh%d(", jfr2htm_prefix_txt, leaf->op);
      jfr2htm_leaf_write(leaf->sarg_1);
      fprintf(jfr2htm_op, ")");
      break;
    case JFG_TT_UREL:
      fprintf(jfr2htm_op, "%sr%d(", jfr2htm_prefix_txt, leaf->op);
      jfr2htm_leaf_write(leaf->sarg_1);
      fprintf(jfr2htm_op, ",");
      jfr2htm_leaf_write(leaf->sarg_2);
      fprintf(jfr2htm_op, ")");
      break;
    case JFG_TT_SFUNC:
      fprintf(jfr2htm_op, "%ss%d(", jfr2htm_prefix_txt, leaf->op);
      jfr2htm_leaf_write(leaf->sarg_1);
      fprintf(jfr2htm_op, ")");
      break;
    case JFG_TT_DFUNC:
      fprintf(jfr2htm_op, "%sd%d(", jfr2htm_prefix_txt, leaf->op);
      jfr2htm_leaf_write(leaf->sarg_1);
      fprintf(jfr2htm_op, ",");
      jfr2htm_leaf_write(leaf->sarg_2);
      fprintf(jfr2htm_op, ")");
      break;
    case JFG_TT_CONST:
      jfr2htm_pfloat(leaf->farg);
      break;
    case JFG_TT_VAR:
      fprintf(jfr2htm_op, "%sv%d", jfr2htm_prefix_txt, leaf->sarg_1);
      break;
    case JFG_TT_FZVAR:
      fprintf(jfr2htm_op, "%sf%d", jfr2htm_prefix_txt, leaf->sarg_1);
      break;
    case JFG_TT_TRUE:
      fprintf(jfr2htm_op, "1.0");
      break;
    case JFG_TT_FALSE:
      fprintf(jfr2htm_op, "0.0");
      break;
    case JFG_TT_BETWEEN:
      fprintf(jfr2htm_op, "%sb%d_%d_%d()",
              jfr2htm_prefix_txt, leaf->sarg_1, leaf->sarg_2, leaf->op);
      break;
    case JFG_TT_VFUNC:
      jfr2htm_vfunc_write(leaf->sarg_1, leaf->op);
      break;
    case JFG_TT_UFUNC:
      fprintf(jfr2htm_op, "%suf%d(", jfr2htm_prefix_txt, leaf->op);
      jfr2htm_leaf_write(leaf->sarg_1);
      fprintf(jfr2htm_op, ")");
      break;
    case JFG_TT_ARGLIST:
      jfr2htm_leaf_write(leaf->sarg_1);
      fprintf(jfr2htm_op,", ");
      jfr2htm_leaf_write(leaf->sarg_2);
      break;
    case JFG_TT_UF_VAR:
      fprintf(jfr2htm_op,"lv%d", leaf->sarg_1);
      break;
    case JFG_TT_IIF:
      fprintf(jfr2htm_op,"%siif(", jfr2htm_prefix_txt);
      jfr2htm_leaf_write(leaf->sarg_1);
      fprintf(jfr2htm_op, ",");
      jfr2htm_leaf_write(leaf->sarg_2);
      fprintf(jfr2htm_op, ")");
      break;
    case JFG_TT_ARVAL:
      fprintf(jfr2htm_op, "%sa%d[%srmm(",
              jfr2htm_prefix_txt, leaf->op, jfr2htm_prefix_txt);
      jfg_array(&adesc, jfr_head, leaf->op);
      jfr2htm_leaf_write(leaf->sarg_1);
      fprintf(jfr2htm_op, ",0,%d)]", adesc.array_c - 1);
      break;
  }
}

static void jfr2htm_vget_write(void)
{
  int m;

  fprintf(jfr2htm_op, "function %svget(vno)\n{", jfr2htm_prefix_txt);
  for (m = 0; m < jfr2htm_spdesc.ovar_c; m++)
  { fprintf(jfr2htm_op, "if (vno==%d) return %sv%d;\n",
                        m, jfr2htm_prefix_txt, jfr2htm_spdesc.f_ovar_no + m);
  }
  fprintf(jfr2htm_op, "}\n");
}

static void jfr2htm_vcget_write(void)
{
  int m;

  fprintf(jfr2htm_op, "function %svvget(vno)\n{", jfr2htm_prefix_txt);
  for (m = 0; m < jfr2htm_spdesc.ovar_c; m++)
  { fprintf(jfr2htm_op, "if (vno==%d) return %sc%d;\n",
                        m, jfr2htm_prefix_txt, jfr2htm_spdesc.f_ovar_no + m);
  }
  fprintf(jfr2htm_op, "}\n");
}


static void jfr2htm_vput_write(void)
{
  int m;

  fprintf(jfr2htm_op, "function %svput(vno, v, c)\n{", jfr2htm_prefix_txt);
  for (m = 0; m < jfr2htm_spdesc.ivar_c; m++)
  { fprintf(jfr2htm_op, "if (vno==%d) { %sv%d=v;%svr%d();\n",
                        m, jfr2htm_prefix_txt, jfr2htm_spdesc.f_ivar_no + m,
                        jfr2htm_prefix_txt, jfr2htm_spdesc.f_ivar_no + m);
    fprintf(jfr2htm_op, "%sc%d=%sr01(c);}\n",
                        jfr2htm_prefix_txt, jfr2htm_spdesc.f_ivar_no + m,
                        jfr2htm_prefix_txt);
  }
  fprintf(jfr2htm_op, "}\n");
}

static void jfr2htm_init_write(void)
{
  int m, a;
  struct jfg_var_desc vdesc;
  struct jfg_array_desc ardesc;

  fprintf(jfr2htm_op, "function %sinit()\n{", jfr2htm_prefix_txt);
  for (m = 0; m < jfr2htm_spdesc.var_c; m++)
  { jfg_var(&vdesc, jfr_head, m);
    fprintf(jfr2htm_op, "%sv%d=", jfr2htm_prefix_txt, m);
    jfr2htm_float(vdesc.default_val);
    fprintf(jfr2htm_op, ";");
    if (m >= jfr2htm_spdesc.f_ivar_no
        && m < jfr2htm_spdesc.f_ivar_no + jfr2htm_spdesc.ivar_c)
      fprintf(jfr2htm_op, "%sc%d=1.0;", jfr2htm_prefix_txt, m);
    else
      fprintf(jfr2htm_op, "%sc%d=0.0;", jfr2htm_prefix_txt, m);
    fprintf(jfr2htm_op, "%scs%d=0.0;\n", jfr2htm_prefix_txt, m);
    for (a = 0; a < vdesc.fzvar_c; a++)
    { fprintf(jfr2htm_op, "%sf%d=0.0;", jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
      if (a > 0 && a % 5 == 0)
        fprintf(jfr2htm_op, "\n");
    }
    fprintf(jfr2htm_op, "\n");
  }
  for (m = 0; m < jfr2htm_spdesc.array_c; m++)
  { jfg_array(&ardesc, jfr_head, m);
    for (a = 0; a < ardesc.array_c; a++)
    { fprintf(jfr2htm_op, "%sa%d[%d]=0.0;", jfr2htm_prefix_txt, m, a);
      if (a % 5 == 0)
     fprintf(jfr2htm_op, "\n");
    }
    fprintf(jfr2htm_op, "\n");
  }
  for (m = 0; m < jfr2htm_ff_ptxt; m++)
  { fprintf(jfr2htm_op, "%st_%s = \"\";\n",
            jfr2htm_prefix_txt, jfr2htm_ptxts[m].name);
  }
  fprintf(jfr2htm_op, "%st_stdout = \"\";\n", jfr2htm_prefix_txt);
  fprintf(jfr2htm_op, "%st_aalert = \"\";\n", jfr2htm_prefix_txt);

  fprintf(jfr2htm_op, "%sglw=1.0;%srmw=1.0;\n",
          jfr2htm_prefix_txt, jfr2htm_prefix_txt);
  fprintf(jfr2htm_op, "}\n");
}

static void jfr2htm_form2var_write(void)
{
  int m, ft;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;

  fprintf(jfr2htm_op, "function %sform2var() {\n", jfr2htm_prefix_txt);
  for (m = 0; m < jfr2htm_spdesc.var_c; m++)
  { jfr2htm_vget(&vdesc, m, 0);
    if (m >= jfr2htm_spdesc.f_ivar_no
        && m < jfr2htm_spdesc.f_ivar_no + jfr2htm_spdesc.ivar_c
        && jfr2htm_vargs.va_type == JFR2HTM_VT_STANDARD)
    { jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
      ft = jfr2htm_vargs.ft_type;
      switch (ft)
      { case JFR2HTM_FT_PULLDOWN:
          fprintf(jfr2htm_op, "%sv%d=parseFloat(", jfr2htm_prefix_txt, m);
          fprintf(jfr2htm_op,
                  "document.jfs.%s.options[document.jfs.%s.selectedIndex].value);",
                  vdesc.name, vdesc.name);
          break;
        case JFR2HTM_FT_RADIO:
        case JFR2HTM_FT_RADIO_M:
          fprintf(jfr2htm_op,
          "%sv%d=parseFloat(document.jfs.%s[%swhichRadio(document.jfs.%s)].value);",
                  jfr2htm_prefix_txt, m, vdesc.name, jfr2htm_prefix_txt,
                  vdesc.name);
          break;
        case JFR2HTM_FT_TEXT_NUM:
        case JFR2HTM_FT_CHECKBOX:
          fprintf(jfr2htm_op,
            "%sv%d=parseFloat(document.jfs.%s.value);",
                  jfr2htm_prefix_txt, m, vdesc.name);
          break;
      }
      if (jfr2htm_vargs.conf == 1)
      { fprintf(jfr2htm_op,
                "%sc%d=parseFloat(document.jfs.conf_%s.value) / ",
                jfr2htm_prefix_txt, m, vdesc.name);
        jfr2htm_float(jfr2htm_max_conf);
        fprintf(jfr2htm_op, ";");
      }
      else
        fprintf(jfr2htm_op, "%sc%d=1.0;", jfr2htm_prefix_txt, m);
      fprintf(jfr2htm_op, "%scs%d=0.0;\n", jfr2htm_prefix_txt, m);
    }
  }
  fprintf(jfr2htm_op, "}\n");
}

static void jfr2htm_var2form_write(void)
{
  int m, vno, ft;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;

  fprintf(jfr2htm_op, "function %svar2form()\n{var t;\n", jfr2htm_prefix_txt);
  /* rounding, formatering af output: */
  for (m = 0; m < jfr2htm_spdesc.ovar_c; m++)
  { vno = jfr2htm_spdesc.f_ovar_no + m;
    jfr2htm_vget(&vdesc, vno, 0);
    jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
    if (jfr2htm_vargs.va_type == JFR2HTM_VT_STANDARD)
    { ft = jfr2htm_vargs.ft_type;
      switch (ft)
      { case JFR2HTM_FT_PULLDOWN:
          fprintf(jfr2htm_op, "t=%sv2id%d();\n", jfr2htm_prefix_txt, vno);
          fprintf(jfr2htm_op, "document.jfs.%s.options[t].selected=true;\n",
                  vdesc.name);
          break;
        case JFR2HTM_FT_RADIO:
        case JFR2HTM_FT_RADIO_M:
          fprintf(jfr2htm_op, "t=%sv2id%d();\n", jfr2htm_prefix_txt, vno);
          fprintf(jfr2htm_op, "document.jfs.%s[t].checked=true;\n",
                  vdesc.name);
          break;
        case JFR2HTM_FT_TEXT_ADJ:
          fprintf(jfr2htm_op, "document.jfs.%s.value=%saar%d[%sv2id%d()];\n",
                  vdesc.name, jfr2htm_prefix_txt, vno, jfr2htm_prefix_txt, vno);
          break;
        case JFR2HTM_FT_TEXT_NUM:
          if (ddesc.type == JFS_DT_INTEGER)
            fprintf(jfr2htm_op, "document.jfs.%s.value=Math.round(%sv%d);",
                    vdesc.name, jfr2htm_prefix_txt, vno);
          else
          { fprintf(jfr2htm_op, "document.jfs.%s.value=%sfround(%sv%d);\n",
                    vdesc.name, jfr2htm_prefix_txt, jfr2htm_prefix_txt, vno);
          }
          break;
      }
      if (jfr2htm_vargs.conf == 1)
      { fprintf(jfr2htm_op, "document.jfs.conf_%s.value=%sfround(%sc%d*",
                vdesc.name, jfr2htm_prefix_txt, jfr2htm_prefix_txt, vno);
        jfr2htm_float(jfr2htm_max_conf);
        fprintf(jfr2htm_op, ");\n");
      }
    }
  }
  for (m = 0; m < jfr2htm_ff_textarea; m++)
  { fprintf(jfr2htm_op, "document.jfs.%s.value = %st_%s;\n",
            jfr2htm_textareas[m].name,
            jfr2htm_prefix_txt, jfr2htm_textareas[m].name);
  }
  fprintf(jfr2htm_op, "};\n");
}

static void jfr2htm_printf_write(const char *dest, const char *words[], int count, int sword)
{
  /* adds text to t_variable from 'extern printf/alert/fprintf' statement */
  int m, i, did, vno, first;
  const char *w;
  char dtxt[256];

  fprintf(jfr2htm_op, "%st_%s += ", jfr2htm_prefix_txt, dest);
  w = words[sword];
  i = sword + 2;  /* first variable-name */
  m = 0;
  if (w[0] == '"')
    m++;
  did = 0;
  first = 1;
  while (w[m] != '\0' && w[m] != '"')
  { if (w[m] == '%')
    { if (did != 0)
      { dtxt[did] = '\0';
        if (i >= count)
          jf_error(1016, jfr2htm_spaces, JFE_ERROR);
        if (first == 0)
          fprintf(jfr2htm_op, " + ");
        first = 0;  
        fprintf(jfr2htm_op, "\"%s\" ", dtxt);
        did = 0;
      }
      m++;
      if (w[m] == 'f')
      { vno = jfr2htm_var_find(words[i]);
        fprintf(jfr2htm_op, " + %sfround(%sv%d)",
                jfr2htm_prefix_txt, jfr2htm_prefix_txt, vno);
        i += 2;
      }
      else
      if (w[m] == 'a')
      { vno = jfr2htm_var_find(words[i]);
        fprintf(jfr2htm_op, " + %saar%d[%sv2id%d()]",
                jfr2htm_prefix_txt, vno, jfr2htm_prefix_txt, vno);
        i += 2;
      }
      else
      if (w[m] == 'c')
      { vno = jfr2htm_var_find(words[i]);
        fprintf(jfr2htm_op, " + %sfround(%sc%d*",
                jfr2htm_prefix_txt, jfr2htm_prefix_txt, vno);
        jfr2htm_float(jfr2htm_max_conf);
        fprintf(jfr2htm_op, ")");
        i += 2;
      }
      m++;
    }
    else
    { dtxt[did] = w[m];
      did++;
      m++;
    }
  }
  if (did != 0)
  { dtxt[did] = '\0';
    if (i != sword + 2)
      fprintf(jfr2htm_op, " + ");
    fprintf(jfr2htm_op, " \"%s\" ", dtxt);
  }
  fprintf(jfr2htm_op, ";\n");
}

static int jfr2htm_run_write(void)
{
  unsigned short c, i, e;
  unsigned char *pc;
  int a, fu, first;
  struct jfg_var_desc vdesc;
  struct jfg_statement_desc sdesc;
  struct jfg_fzvar_desc fzvdesc;
  struct jfg_function_desc fudesc;
  struct jfg_array_desc adesc;

  for (fu = 0; fu < jfr2htm_spdesc.function_c + 1; fu++)
  { if (fu < jfr2htm_spdesc.function_c)
    { jfg_function(&fudesc, jfr_head, fu);
      pc = fudesc.pc;
      first = 1;
      fprintf(jfr2htm_op, "function %suf%d(", jfr2htm_prefix_txt, fu);
      for (a = 0; a < fudesc.arg_c; a++)
      { if (first == 0)
          fprintf(jfr2htm_op, ",");
        first = 0;
        fprintf(jfr2htm_op, "lv%d", a);
      }
      fprintf(jfr2htm_op, ")\n{");
      fprintf(jfr2htm_op, "var rfw; var r\n");  /* var t ?? */
      fprintf(jfr2htm_op, "rfw=%sglw;\n", jfr2htm_prefix_txt);
    }
    else
    { fprintf(jfr2htm_op, "function %srun()\n{", jfr2htm_prefix_txt);
      pc = jfr2htm_spdesc.pc_start;
      jfr2htm_ff_ltypes = 0;
      fprintf(jfr2htm_op, "var t; var r\n");
      for (a = 0; a < jfr2htm_spdesc.ivar_c; a++)
      { jfg_var(&vdesc, jfr_head, jfr2htm_spdesc.f_ivar_no + a);
        if (vdesc.fzvar_c > 0 && jfr2htm_varuse[jfr2htm_spdesc.f_ivar_no + a].wvar == 1
            && jfr2htm_varuse[jfr2htm_spdesc.f_ivar_no + a].rfzvar == 1)
          fprintf(jfr2htm_op, "%sh=%sv%d+1.0;%sfu%d();",
                  jfr2htm_prefix_txt,
                  jfr2htm_prefix_txt, jfr2htm_spdesc.f_ivar_no + a,
                  jfr2htm_prefix_txt,
                  jfr2htm_spdesc.f_ivar_no + a);
      }
      fprintf(jfr2htm_op, "\n");
    }
    jfg_statement(&sdesc, jfr_head, pc);
    while (sdesc.type != JFG_ST_EOP &&
    (!(sdesc.type == JFG_ST_STEND && sdesc.sec_type == 2))) /* end-of-func */
    {
      switch (sdesc.type)
      { case JFG_ST_IF:
          jfg_if_tree(jfr2htm_tree, jfr2htm_maxtree, &c, &i, &e, jfr_head, pc);
          /* rmw=glw;glw=r01(weight_op(glw, lweight)*expr,0.0,1.0); */
          fprintf(jfr2htm_op, "%srmw=%sglw;%sglw=%sr01(",
                  jfr2htm_prefix_txt, jfr2htm_prefix_txt,
                  jfr2htm_prefix_txt, jfr2htm_prefix_txt);
          if ((sdesc.flags & 1) != 0)
          { fprintf(jfr2htm_op, "%so%d(%sglw,",
                    jfr2htm_prefix_txt, JFS_ONO_WEIGHTOP, jfr2htm_prefix_txt);
            jfr2htm_float(sdesc.farg);
            fprintf(jfr2htm_op, ")");
          }
          else
            fprintf(jfr2htm_op, "%sglw", jfr2htm_prefix_txt);
          fprintf(jfr2htm_op, "*(");
          jfr2htm_leaf_write(c);
          fprintf(jfr2htm_op, "));\n");
          /* if (glw!=0.0) (...); */
          fprintf(jfr2htm_op, "if (%sglw!=0.0) { ", jfr2htm_prefix_txt);
          switch (sdesc.sec_type)
          { case JFG_SST_FZVAR:
              jfg_fzvar(&fzvdesc, jfr_head, sdesc.sarg_1);
              jfg_var(&vdesc, jfr_head, fzvdesc.var_no);
              fprintf(jfr2htm_op, "%sh=%sf%d;%sf%d=%so%d(%sf%d,%sglw);",
                      jfr2htm_prefix_txt, jfr2htm_prefix_txt, sdesc.sarg_1,
                      jfr2htm_prefix_txt, sdesc.sarg_1,
                      jfr2htm_prefix_txt, vdesc.f_comp,
                      jfr2htm_prefix_txt, sdesc.sarg_1,
                      jfr2htm_prefix_txt);
              if (jfr2htm_varuse[fzvdesc.var_no].wfzvar == 1
                  && jfr2htm_varuse[fzvdesc.var_no].rvar == 1)
                fprintf(jfr2htm_op, "%sdf%d(%sf%d)",
                        jfr2htm_prefix_txt, fzvdesc.var_no,
                        jfr2htm_prefix_txt, sdesc.sarg_1);
              fprintf(jfr2htm_op, "}\n%sglw=%srmw;\n",
                      jfr2htm_prefix_txt, jfr2htm_prefix_txt);
              break;
            case JFG_SST_VAR:
              jfg_var(&vdesc, jfr_head, sdesc.sarg_1);
              fprintf(jfr2htm_op, "%sh=%sv%d; r=",
                      jfr2htm_prefix_txt, jfr2htm_prefix_txt, sdesc.sarg_1);
              jfr2htm_leaf_write(e);
              fprintf(jfr2htm_op, ";\n");
              switch(vdesc.d_comp)
              { case JFS_VCF_NEW:
                  fprintf(jfr2htm_op, "%sv%d=r;%sc%d=%sglw;",
                          jfr2htm_prefix_txt, sdesc.sarg_1,
                          jfr2htm_prefix_txt, sdesc.sarg_1,
                          jfr2htm_prefix_txt);
                  break;
                case JFS_VCF_AVG:
                  fprintf(jfr2htm_op, "r=%sv%d*%scs%d+r*%sglw;%scs%d+=%sglw;",
                          jfr2htm_prefix_txt, sdesc.sarg_1,
                          jfr2htm_prefix_txt, sdesc.sarg_1,
                          jfr2htm_prefix_txt,
                          jfr2htm_prefix_txt, sdesc.sarg_1, jfr2htm_prefix_txt);
                  fprintf(jfr2htm_op, "%sv%d=r/%scs%d;\n",
                          jfr2htm_prefix_txt, sdesc.sarg_1,
                          jfr2htm_prefix_txt, sdesc.sarg_1);
                  break;
                case JFS_VCF_MAX:
                  fprintf(jfr2htm_op, "if (%sglw>%sc%d) {%sv%d=r;%sc%d=%sglw}\n",
                          jfr2htm_prefix_txt, jfr2htm_prefix_txt, sdesc.sarg_1,
                          jfr2htm_prefix_txt, sdesc.sarg_1,
                          jfr2htm_prefix_txt, sdesc.sarg_1,
                          jfr2htm_prefix_txt);
                  break;
              }
              fprintf(jfr2htm_op, "%svr%d();", jfr2htm_prefix_txt, sdesc.sarg_1);
              if (jfr2htm_varuse[sdesc.sarg_1].wvar == 1
                  && jfr2htm_varuse[sdesc.sarg_1].rfzvar == 1)
                fprintf(jfr2htm_op, "%sfu%d()", jfr2htm_prefix_txt, sdesc.sarg_1);
              fprintf(jfr2htm_op, "}\n%sglw=%srmw;\n",
                      jfr2htm_prefix_txt, jfr2htm_prefix_txt);
              break;
            case JFG_SST_ARR:
              fprintf(jfr2htm_op, "%sa%d[%srmm(",
                      jfr2htm_prefix_txt, sdesc.sarg_1, jfr2htm_prefix_txt);
              jfr2htm_leaf_write(i);
              jfg_array(&adesc, jfr_head, sdesc.sarg_1);
              fprintf(jfr2htm_op, ",0.0,");
              jfr2htm_float(adesc.array_c - 1.0);
              fprintf(jfr2htm_op, ")]=");
              jfr2htm_leaf_write(e);
              fprintf(jfr2htm_op, "}\n%sglw=%srmw;\n",
                      jfr2htm_prefix_txt, jfr2htm_prefix_txt);
              break;
            case JFG_SST_INC:
              jfg_var(&vdesc, jfr_head, sdesc.sarg_1);
              fprintf(jfr2htm_op, "%sh=%sv%d; r=",
                      jfr2htm_prefix_txt, jfr2htm_prefix_txt, sdesc.sarg_1);
              jfr2htm_leaf_write(e);
              fprintf(jfr2htm_op, ";\n");
              if (sdesc.flags & 4)
                fprintf(jfr2htm_op, "%sv%d-=r*%sglw;\n",
                        jfr2htm_prefix_txt, sdesc.sarg_1, jfr2htm_prefix_txt);
              else
                fprintf(jfr2htm_op, "%sv%d+=r*%sglw;\n",
                        jfr2htm_prefix_txt, sdesc.sarg_1, jfr2htm_prefix_txt);
              fprintf(jfr2htm_op, "if (%sglw>%sc%d) %sc%d=%sglw;\n",
                      jfr2htm_prefix_txt, jfr2htm_prefix_txt, sdesc.sarg_1,
                      jfr2htm_prefix_txt, sdesc.sarg_1, jfr2htm_prefix_txt);
              fprintf(jfr2htm_op, "%svr%d();", jfr2htm_prefix_txt, sdesc.sarg_1);
              if (jfr2htm_varuse[sdesc.sarg_1].wvar == 1
                  && jfr2htm_varuse[sdesc.sarg_1].rfzvar == 1)
                fprintf(jfr2htm_op, "%sfu%d()", jfr2htm_prefix_txt, sdesc.sarg_1);
              fprintf(jfr2htm_op, "}\n%sglw=%srmw;\n",
                      jfr2htm_prefix_txt, jfr2htm_prefix_txt);
              break;
            case JFG_SST_EXTERN:
              a = jfg_a_statement(jfr2htm_words, JFR2HTM_MAX_WORDS, jfr_head, pc);
              if (a > 0)
              { if (strcmp(jfr2htm_words[0], jfr2htm_t_java) == 0)
                { if (a > 1)
                  {
                    char *w;
                    w = (char *)(malloc (strlen(jfr2htm_words[1]) + 1));
                    if (w != NULL) strcpy(w, jfr2htm_words[1]);
                    if (w[0] == '"')
                      w++;
                    if (w[strlen(w) - 1] == '"')
                      w[strlen(w) - 1] = '\0';
                    fprintf(jfr2htm_op, "%s;\n", w);
                    free(w);
                  }
                }
                else
                if (strcmp(jfr2htm_words[0], jfr2htm_t_textarea) == 0)
                  ;
                else
                if (strcmp(jfr2htm_words[0], jfr2htm_t_printf) == 0)
                  jfr2htm_printf_write("stdout", jfr2htm_words, a, 1);
                else
                if (strcmp(jfr2htm_words[0], jfr2htm_t_fprintf) == 0)
                  jfr2htm_printf_write(jfr2htm_words[1], jfr2htm_words, a, 3);
                else
                if (strcmp(jfr2htm_words[0], jfr2htm_t_alert) == 0)
                { fprintf(jfr2htm_op, "%st_alert=\"\";\n", jfr2htm_prefix_txt);
                  jfr2htm_printf_write("alert", jfr2htm_words, a, 1);
                  fprintf(jfr2htm_op, "alert(%st_alert);\n", jfr2htm_prefix_txt);
                }
                else
                 jf_error(1008, jfr2htm_words[0], JFE_WARNING);
              }
              fprintf(jfr2htm_op,"}\n %sglw=%srmw;\n",
                      jfr2htm_prefix_txt, jfr2htm_prefix_txt);
              break;
            case JFG_SST_CLEAR:
              jfg_var(&vdesc, jfr_head, sdesc.sarg_1);
              fprintf(jfr2htm_op, "%sv%d=", jfr2htm_prefix_txt, sdesc.sarg_1);
              jfr2htm_float(vdesc.default_val);
              fprintf(jfr2htm_op, ";%sc%d=0.0;%scs%d=0.0;\n",
                      jfr2htm_prefix_txt, sdesc.sarg_1,
                      jfr2htm_prefix_txt, sdesc.sarg_1);
              for (a = 0; a < vdesc.fzvar_c; a++)
                fprintf(jfr2htm_op, "%sf%d=0.0;",
                        jfr2htm_prefix_txt, vdesc.f_fzvar_no + a);
              fprintf(jfr2htm_op, "}\n%sglw=%srmw;\n",
                      jfr2htm_prefix_txt, jfr2htm_prefix_txt);
              break;
            case JFG_SST_PROCEDURE:
              jfr2htm_leaf_write(e);
              fprintf(jfr2htm_op, "}\n");
              break;
            case JFG_SST_RETURN:
              fprintf(jfr2htm_op, "%sglw=rfw;return ", jfr2htm_prefix_txt);
              jfr2htm_leaf_write(e);
              fprintf(jfr2htm_op, "}\n");
              break;
            case JFG_SST_FUARG:
              fprintf(jfr2htm_op, "lv%d", sdesc.sarg_1);
              fprintf(jfr2htm_op, "=");
              jfr2htm_leaf_write(e);
              fprintf(jfr2htm_op, "}\n%sglw=%srmw;\n",
                      jfr2htm_prefix_txt, jfr2htm_prefix_txt);
              break;
          }
       break;
     case JFG_ST_CASE:
          jfg_if_tree(jfr2htm_tree, jfr2htm_maxtree, &c, &i, &e, jfr_head, pc);
          fprintf(jfr2htm_op, "%sglw=(", jfr2htm_prefix_txt);
          jfr2htm_leaf_write(c);
          fprintf(jfr2htm_op, ");");
          fprintf(jfr2htm_op, "%ssw[%d]+=%sglw;",
                  jfr2htm_prefix_txt, jfr2htm_ff_ltypes - 1, jfr2htm_prefix_txt);
          fprintf(jfr2htm_op, "%sglw=%so%d(%srw[%d],%sglw);\n",
                  jfr2htm_prefix_txt, jfr2htm_prefix_txt, JFS_ONO_CASEOP,
                  jfr2htm_prefix_txt, jfr2htm_ff_ltypes - 1,
                  jfr2htm_prefix_txt);
          break;
        case JFG_ST_WSET:
          jfg_if_tree(jfr2htm_tree, jfr2htm_maxtree, &c, &i, &e, jfr_head, pc);
          fprintf(jfr2htm_op, "if (%sglw != 0.0) %sglw=(",
                  jfr2htm_prefix_txt, jfr2htm_prefix_txt);
          jfr2htm_leaf_write(c);
          fprintf(jfr2htm_op, ");\n");
          break;
        case JFG_ST_DEFAULT:
           fprintf(jfr2htm_op, "%sglw=%so%d(%srw[%d],Math.max(0.0,1.0-%ssw[%d]));\n",
                   jfr2htm_prefix_txt, jfr2htm_prefix_txt, JFS_ONO_CASEOP,
                   jfr2htm_prefix_txt, jfr2htm_ff_ltypes - 1,
                   jfr2htm_prefix_txt, jfr2htm_ff_ltypes - 1);
           break;
        case JFG_ST_STEND:
          jfr2htm_ff_ltypes--;
          if (jfr2htm_ltypes[jfr2htm_ff_ltypes] == 0)  /* switch-statement */
            fprintf(jfr2htm_op, "%sglw=%srw[%d];\n",
                    jfr2htm_prefix_txt, jfr2htm_prefix_txt, jfr2htm_ff_ltypes);
          else    /* while-statement */
            fprintf(jfr2htm_op, "%sglw=%srw[%d];\n}\n%sglw=%srw[%d];\n",
                    jfr2htm_prefix_txt, jfr2htm_prefix_txt, jfr2htm_ff_ltypes,
                    jfr2htm_prefix_txt, jfr2htm_prefix_txt, jfr2htm_ff_ltypes);
          break;
         case JFG_ST_SWITCH:
           jfr2htm_ltypes[jfr2htm_ff_ltypes] = 0;
           fprintf(jfr2htm_op, "%srw[%d]=%sglw;\n",
                   jfr2htm_prefix_txt, jfr2htm_ff_ltypes, jfr2htm_prefix_txt);
           fprintf(jfr2htm_op, "%ssw[%d]=0.0;\n",
                   jfr2htm_prefix_txt, jfr2htm_ff_ltypes);
           jfr2htm_ff_ltypes++;
           if (jfr2htm_ff_ltypes >= jfr2htm_LTYPES_MAX)
             return jf_error(1007, jfr2htm_spaces, JFE_ERROR);
           break;
         case JFG_ST_WHILE:
           jfr2htm_ltypes[jfr2htm_ff_ltypes] = 1;
           fprintf(jfr2htm_op, "%srw[%d]=%sglw;%srmw=%sglw;\n",
                   jfr2htm_prefix_txt, jfr2htm_ff_ltypes,jfr2htm_prefix_txt,
                   jfr2htm_prefix_txt, jfr2htm_prefix_txt);
           fprintf(jfr2htm_op, "while (true){\n");
           fprintf(jfr2htm_op, "%sglw=%so%d(%sglw,(",
                   jfr2htm_prefix_txt, jfr2htm_prefix_txt, JFS_ONO_WHILEOP,
                   jfr2htm_prefix_txt);
           jfg_if_tree(jfr2htm_tree, jfr2htm_maxtree, &c, &i, &e, jfr_head, pc);
           jfr2htm_leaf_write(c);
           fprintf(jfr2htm_op, "));\nif (%sglw==0.0) break;\n", jfr2htm_prefix_txt);
           jfr2htm_ff_ltypes++;
           if (jfr2htm_ff_ltypes >= jfr2htm_LTYPES_MAX)
             return jf_error(1007, jfr2htm_spaces, JFE_ERROR);
           break;
         default:
           break;
      }
      pc = sdesc.n_pc;
      jfg_statement(&sdesc, jfr_head, pc);
    }
    if (fu < jfr2htm_spdesc.function_c)
      fprintf(jfr2htm_op, "%sglw=rfw;return 0.0;\n", jfr2htm_prefix_txt);
    fprintf(jfr2htm_op, "}\n");
  }
  return 0;
}

static int jfr2htm_vget(struct jfg_var_desc *vdesc, int vno, int fsetset)
{
  /* read a variable-description, set argument-values in jfr2htm_vargs */

  struct jfg_domain_desc ddesc;
  int ft=JFR2HTM_FT_DEFAULT, is_ovar, varg;

  if (vno >= jfr2htm_spdesc.f_ovar_no &&
      vno < jfr2htm_spdesc.f_ovar_no + jfr2htm_spdesc.ovar_c)
    is_ovar = 1;
  else
    is_ovar = 0;
  jfg_var(vdesc, jfr_head, vno);
  jfg_domain(&ddesc, jfr_head, vdesc->domain_no);
  if (jfr2htm_use_args == 0)
    varg = 0;
  else
    varg = vdesc->argument;

  jfr2htm_vargs.ft_type = jfr2htm_vargs.com_label = 0;
  jfr2htm_vargs.help_button = jfr2htm_vargs.conf = jfr2htm_vargs.no_nl = 0;

  jfr2htm_vargs.va_type = (varg & 3);
  if (jfr2htm_vargs.va_type > 1)
  { jf_error(1027, vdesc->name, JFE_WARNING);
    jfr2htm_vargs.va_type = 0;
  }
  switch (jfr2htm_vargs.va_type)
  { case JFR2HTM_VT_STANDARD:
      ft = ((varg / 4) & 15);
      if (ft == JFR2HTM_FT_TEXT || ft == JFR2HTM_FT_RULER
          || ft == JFR2HTM_FT_ADJECTIVES ||
          (ft >= 10))
      { ft = 0;
        jf_error(1023, vdesc->name, JFE_WARNING);
      }
      if (ft != JFR2HTM_FT_DEFAULT)
      { if (is_ovar == 0)
        { if (ft == JFR2HTM_FT_TEXT_ADJ)
          { ft = JFR2HTM_FT_TEXT_NUM;
            jf_error(1021, vdesc->name, JFE_WARNING);
          }
          if (ft == JFR2HTM_FT_CHECKBOX && vdesc->fzvar_c != 2)
          { ft = JFR2HTM_FT_PULLDOWN;
            jf_error(1022, vdesc->name, JFE_WARNING);
          }
        }
        else
        { if (ft == JFR2HTM_FT_CHECKBOX)
          { ft = JFR2HTM_FT_PULLDOWN;
            jf_error(1024, vdesc->name, JFE_WARNING);
          }
        }
        if (ft != JFR2HTM_FT_TEXT_NUM && vdesc->fzvar_c == 0)
        { ft = JFR2HTM_FT_TEXT_NUM;
          jf_error(1025, vdesc->name, JFE_WARNING);
        }
      }
      else
      { if (ddesc.type == JFS_DT_CATEGORICAL)
          ft = JFR2HTM_FT_PULLDOWN;
        else
          ft = JFR2HTM_FT_TEXT_NUM;
      }
      jfr2htm_vargs.ft_type = ft;
      if ((varg & JFR2HTM_VA_COM_LABEL) != 0)
        jfr2htm_vargs.com_label = 1;
      if ((varg & JFR2HTM_VA_HELP_BUTTON) != 0)
        jfr2htm_vargs.help_button = 1;
      if ((varg & JFR2HTM_VA_CONFIDENCE) != 0)
        jfr2htm_vargs.conf = 1;
      if ((varg & JFR2HTM_VA_NO_NL) != 0)
        jfr2htm_vargs.no_nl = 1;
      break;
    case JFR2HTM_VT_FIELDSET:
      if (fsetset == 1)
      { jfr2htm_fdesc.label_mode = ((varg / 4) & 15);
        if (jfr2htm_fdesc.label_mode > 5)
        { jf_error(1026, vdesc->name, JFE_WARNING);
          jfr2htm_fdesc.label_mode = 0;
        }
        if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_DEFAULT)
          jfr2htm_fdesc.label_mode = jfr2htm_label_mode;
        if ((varg & JFR2HTM_VA_NO_LEGEND) != 0)
          jfr2htm_fdesc.no_legend = 1;
        if ((varg & JFR2HTM_VA_NO_FIELDSET) != 0)
          jfr2htm_fdesc.no_fieldset = 1;
        if (jfr2htm_fdesc.no_fieldset == 1)
          jfr2htm_fdesc.no_legend = 1;
      }
      if ((varg & JFR2HTM_VA_COM_LABEL) != 0)
        jfr2htm_vargs.com_label = 1;
    break;
  }
  return ft;
}

static void jfr2htm_jfs_write(void)
{
  fprintf(jfr2htm_op, "function %sjfr() {\n", jfr2htm_prefix_txt);
  if (jfr2htm_tot_validate == 1)
  { fprintf(jfr2htm_op, "  if (%stot_validate()!=0)\n", jfr2htm_prefix_txt);
    fprintf(jfr2htm_op, "    return 1;\n");
  }
  fprintf(jfr2htm_op, "  %sinit();\n", jfr2htm_prefix_txt);
  fprintf(jfr2htm_op, "  %sform2var();\n", jfr2htm_prefix_txt);
  fprintf(jfr2htm_op, "  %srun();\n", jfr2htm_prefix_txt);
  fprintf(jfr2htm_op, "  %svar2form();\n", jfr2htm_prefix_txt);

  fprintf(jfr2htm_op, "};\n");
}

static int jfr2htm_head_write(void)
{
  fprintf(jfr2htm_op, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\">\n");
  fprintf(jfr2htm_op, "<HTML>\n\n");
  fprintf(jfr2htm_op,    "<HEAD><TITLE>%s</TITLE>\n\n", jfr2htm_spdesc.title);
  if (jfr2htm_use_ssheet == 1)
    fprintf(jfr2htm_op,
            "       <LINK REL=STYLESHEET TYPE=TEXT/CSS HREF=\"%s\">\n",
            jfr2htm_ssheet);
  return 0;          
}

static int jfr2htm_subst(char *d, const char *s, const char *ds, const char *ss)
{
  /* copies <s> to <d> with all occurences of <ss> replaced by <ds> */

  int st, dt, tt, ss_len, ds_len;
  char ttxt[JFR2HTM_MAX_TEXT];

  dt = tt = 0;
  ss_len = strlen(ss);
  ds_len = strlen(ds);
  for (st = 0; s[st] != '\0'; st++)
  { if (s[st] == ss[tt])
    { ttxt[tt] = s[st];
      tt++;
      if (tt == ss_len)
      { dt += ds_len;
        if (dt >= JFR2HTM_MAX_TEXT)
          return jf_error(1010, d, JFE_ERROR);
        strcat(d, ds);
        tt = 0;
      }
    }
    else
    { if (tt > 0)
      { dt += tt;
        if (dt >= JFR2HTM_MAX_TEXT)
          return jf_error(1010, d, JFE_ERROR);
        ttxt[tt] = '\0';
        strcat(d, ttxt);
        tt = 0;
      }
      if (dt >= JFR2HTM_MAX_TEXT)
        return jf_error(1010, d, JFE_ERROR);
      d[dt] = s[st];
      dt++;
      d[dt] = '\0';
    }
  }
  if (tt > 0)
  { dt += tt;
    if (dt >= JFR2HTM_MAX_TEXT)
      return jf_error(1010, d, JFE_ERROR);
    ttxt[tt] = '\0';
    strcat(d, ttxt);
  }
  d[dt] = '\0';
  return 0;
}

static void jfr2htm_t_write(const char **txts)
{
  char ttxt[JFR2HTM_MAX_TEXT];
  int m, slut;

  slut = 0;
  for (m = 0; slut == 0; m++)
  { if (strcmp(txts[m], jfr2htm_t_end) == 0)
      slut = 1;
    else
    { jfr2htm_subst(ttxt, txts[m], jfr2htm_prefix_txt, "PREFIX");
      fprintf(jfr2htm_op, "%s\n", ttxt);
    }
  }
}

static void jfr2htm_iface_functions_write(void)
{
  jfr2htm_vget_write();
  jfr2htm_vcget_write();
  jfr2htm_vput_write();
  jfr2htm_init_write();
  jfr2htm_run_write();
}

static void jfr2htm_form_functions_write(void)
{
  int m, fundet, ft, a;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;

  if (jfr2htm_radiouse == 1)
    jfr2htm_t_write(jfr2htm_t_whichRadio);
  jfr2htm_vhelp_write();

  /* if an input-variable has domain-min-max then write validate-function: */
  fundet = 0;
  jfr2htm_tot_validate = 0;
  for (m = 0; fundet == 0 && m < jfr2htm_spdesc.ivar_c; m++)
  { jfg_var(&vdesc, jfr_head, jfr2htm_spdesc.f_ivar_no + m);
    jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
    if (   (ddesc.flags & JFS_DF_MINENTER) != 0
        || (ddesc.flags & JFS_DF_MAXENTER) != 0)
      fundet = 1;
  }
  if (fundet == 1 || jfr2htm_no_check == 0)
  { if (jfr2htm_no_check == 0)
      jfr2htm_t_write(jfr2htm_t_fval);
    else
      jfr2htm_t_write(jfr2htm_t_no_fval);
    jfr2htm_t_write(jfr2htm_t_val);

    /* write tot_validate() function */
    fprintf(jfr2htm_op, "function %stot_validate()\n", jfr2htm_prefix_txt);
    fprintf(jfr2htm_op, "{ var r=0;\n");
    for (m = 0; m < jfr2htm_spdesc.ivar_c; m++)
    { jfg_var(&vdesc, jfr_head, jfr2htm_spdesc.f_ivar_no + m);
      jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
      if (jfr2htm_vargs.va_type == JFR2HTM_VT_STANDARD)
      { ft = jfr2htm_vargs.ft_type;
        if (ft == JFR2HTM_FT_TEXT_NUM)
        { fprintf(jfr2htm_op, " if (r==0)\n");
          fprintf(jfr2htm_op, "r=%svalidate(document.jfs.%s, ",
                  jfr2htm_prefix_txt, vdesc.name);
          if (ddesc.flags & JFS_DF_MINENTER)
            jfr2htm_float(ddesc.dmin);
          else
            fprintf(jfr2htm_op, "document.jfs.%s.value", vdesc.name);
          fprintf(jfr2htm_op, ", ");
          if (ddesc.flags & JFS_DF_MAXENTER)
            jfr2htm_float(ddesc.dmax);
          else
            fprintf(jfr2htm_op, "document.jfs.%s.value", vdesc.name);
          fprintf(jfr2htm_op, ");\n");
        }
      }
    }
    fprintf(jfr2htm_op, " return r;\n");
    fprintf(jfr2htm_op, "}\n");
    jfr2htm_tot_validate = 1;
  }

  /* write clr_output()-function: */
  fprintf(jfr2htm_op, "function %sclr_output()\n", jfr2htm_prefix_txt);
  fprintf(jfr2htm_op, "{\n");
  for (m = 0; m < jfr2htm_spdesc.ovar_c; m++)
  { jfr2htm_vget(&vdesc, jfr2htm_spdesc.f_ovar_no + m, 0);
    if (jfr2htm_vargs.va_type == JFR2HTM_VT_STANDARD)
    { jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
      ft = jfr2htm_vargs.ft_type;
      switch (ft)
      { case JFR2HTM_FT_PULLDOWN:
          fprintf(jfr2htm_op, "document.jfs.%s.options[%d].selected=true;\n",
                  vdesc.name, vdesc.fzvar_c);
          break;
        case JFR2HTM_FT_RADIO:
        case JFR2HTM_FT_RADIO_M:
          for (a = 0; a < vdesc.fzvar_c; a++)
            fprintf(jfr2htm_op, "document.jfs.%s[%d].checked=false;\n",
                    vdesc.name, a);
          break;
        case JFR2HTM_FT_TEXT_NUM:
        case JFR2HTM_FT_TEXT_ADJ:
          fprintf(jfr2htm_op, "document.jfs.%s.value=\"\";\n", vdesc.name);
        break;
      }
      if (jfr2htm_vargs.conf == 1)
        fprintf(jfr2htm_op, "document.jfs.conf_%s.value=\"\";\n", vdesc.name);
    }
  }
  for (m = 0; m < jfr2htm_ff_textarea; m++)
    fprintf(jfr2htm_op, "document.jfs.%s.value=\"\";\n",
                        jfr2htm_textareas[m].name);
  fprintf(jfr2htm_op, "}\n");

  /* write form/var transfer functions: */
  jfr2htm_form2var_write();
  jfr2htm_var2form_write();
}

static int jfr2htm_program_write(char *jfname)
{
  if (jfr2htm_program_check() != 0)
    return -1;

  fprintf(jfr2htm_op, "%s\n", jfr2htm_t_begin);
  fprintf(jfr2htm_op, "<!-- %s -->\n", t_info);
  if (jfr2htm_js_file == 0)
  { fprintf(jfr2htm_op, "<SCRIPT LANGUAGE=\"JavaScript\">\n");
    fprintf(jfr2htm_op, "<!--\n");
  }
  else
  { fprintf(jfr2htm_op, "<SCRIPT LANGUAGE=\"JavaScript\" SRC = \"%s\">\n",
            jfname);
    jfr2htm_op = jfr2htm_opj;
  }

  /* Write constants and variable-declarations */
  fprintf(jfr2htm_op, "var %sglw; var %srmw; var %sh \n",
          jfr2htm_prefix_txt, jfr2htm_prefix_txt, jfr2htm_prefix_txt);
  jfr2htm_pl_write();  /* constants in pl-functions */
  jfr2htm_var_write(); /* domain variables */
  jfr2htm_array_write();
  jfr2htm_fzvar_write(); /* fuzy variables */
  jfr2htm_aaray_write();
  if (jfr2htm_switch_use == 1)
    fprintf(jfr2htm_op, "var %ssw = new Array()\n var %srw = new Array()\n",
            jfr2htm_prefix_txt, jfr2htm_prefix_txt);

  /* write jfr-calc-functions: */
  jfr2htm_t_write(jfr2htm_fixed);
  jfr2htm_write_fround();

  if (jfr2htm_ff_pl_adr > 0)
    jfr2htm_t_write(jfr2htm_t_plcalc);

  jfr2htm_hedges_write();
  jfr2htm_relations_write();
  jfr2htm_operators_write();

  jfr2htm_sfunc_write();
  jfr2htm_dfunc_write();
  if (jfr2htm_iif_use == 1)
    jfr2htm_t_write(jfr2htm_t_iif);
  jfr2htm_between_write();
  jfr2htm_vround_write();
  jfr2htm_var2index_write();
  jfr2htm_fuz_write();
  jfr2htm_defuz_write();

  /* write the run-function and the other user-interface functions */
  jfr2htm_iface_functions_write();

  jfr2htm_form_functions_write();
  jfr2htm_jfs_write();

  if (jfr2htm_js_file == 0)
  { fprintf(jfr2htm_op, "\n// -->\n");
  }
  else
  { jfr2htm_op = jfr2htm_oph;
  }
  fprintf(jfr2htm_op, "</SCRIPT>\n");
  fprintf(jfr2htm_op, "%s\n", jfr2htm_t_end);
  return 0;
}

static int jfr2htm_buf_cmp(const char *t)
{
  int tid, bid, res;

  tid = bid = 0;
  res = -1;
  while (jfr2htm_text[bid] == ' ')
    bid++;
  while (res == -1)
  { if (t[tid] == '\0')
      res = 0; /* ens */
    else
    { if (t[tid] != jfr2htm_text[bid])
        res = 1;
    }
    tid++; bid++;
  }
  return res;
}

static int jfr2htm_head_copy(int mode)
{
/* copy from file until jfr2htm_t_begin,
   if mode=0: then ignore until jft2htm_t_end,
   if mode=1: continue to copy until jfr2htm_t_en (used if form has to be
   copied). */

  int state;
  char *c;

  state = 0;
  while (state < 2)
  { c = fgets(jfr2htm_text, jfr2htm_maxtext, jfr2htm_ip);
    if (c == NULL)
      return jf_error(1000, jfr2htm_spaces, JFE_ERROR);
    if (state == 0)
    { if (jfr2htm_buf_cmp(jfr2htm_t_begin) == 0)
        state = 1;
    }
    if (state == 1)
    { if (jfr2htm_buf_cmp(jfr2htm_t_end) == 0)
        state = 2;
    }
    if (state == 0 || mode != 0)
      fprintf(jfr2htm_op, "%s", jfr2htm_text);
  }
  return 0;
}

static void jfr2htm_body_write(void)
{
  fprintf(jfr2htm_op, "</HEAD>\n\n");
  fprintf(jfr2htm_op, "<BODY>\n");
  fprintf(jfr2htm_op, "<H1>%s</H1>\n\n", jfr2htm_spdesc.title);
  if (jfr2htm_main_comment == 1)
  { fprintf(jfr2htm_op, "%s", jfr2htm_divt("jfr2htm_comment"));
    jfr2htm_write_comment(jfr2htm_spdesc.comment_no, 1);
    fprintf(jfr2htm_op, "%s\n", jfr2htm_t_ediv);
  }
}

static void jfr2htm_body_copy(int mode)
{
  jfr2htm_head_copy(mode);
}

/**********************************************************************/
/* write form:                                                        */

static void jfr2htm_formvar_write(int vno, int is_ovar,
                                  int lab_size, int head_mode, int on_nl)
{
  /* write form-field with label for a single variable */

  int s, le, use_label_mode, ft, loc_lm;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  struct jfg_adjectiv_desc adesc;

  jfr2htm_vget(&vdesc, vno, 0);
  jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
  if (jfr2htm_vargs.va_type == JFR2HTM_VT_FIELDSET)
    return;
  ft = jfr2htm_vargs.ft_type;
  use_label_mode = jfr2htm_fdesc.label_mode;
  if (jfr2htm_vargs.com_label == 1 && jfr2htm_fdesc.label_mode == JFR2HTM_LM_LEFT)
    use_label_mode = JFR2HTM_LM_ABOVE;

  /* first write label: */
  if (use_label_mode == JFR2HTM_LM_TABLE
      || jfr2htm_fdesc.label_mode == JFR2HTM_LM_MULTABLE)
  { if (on_nl == 1)
      fprintf(jfr2htm_op, "<TR>");
    fprintf(jfr2htm_op, "<TD>\n");
  }
  fprintf(jfr2htm_op, "%s", jfr2htm_spant(jfr2htm_t_labels[jfr2htm_fdesc.label_mode]));
  if (jfr2htm_vargs.com_label == 0 || vdesc.comment_no == -1)
  { /* write label from variable.text */
    le = strlen(vdesc.text);
    if (le == 0)
    { le = strlen(vdesc.name);
      fprintf(jfr2htm_op, "%s:", vdesc.name);
    }
    else
      fprintf(jfr2htm_op, "%s:", vdesc.text);
    for (s = le; s < lab_size; s++)
      fprintf(jfr2htm_op, " ");
  }
  else
  { /* write label from comment */
    if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_LEFT ||    /* correct! */
        use_label_mode == JFR2HTM_LM_TABLE
        || jfr2htm_fdesc.label_mode == JFR2HTM_LM_MULTABLE)
      jfr2htm_write_comment(vdesc.comment_no, 0); /* no <CR> in comment */
    else
      jfr2htm_write_comment(vdesc.comment_no, 1);
  }
  fprintf(jfr2htm_op, "%s", jfr2htm_t_espan);
  /* maybe write newline between label and field: */
  if (use_label_mode == JFR2HTM_LM_ABOVE
      || use_label_mode == JFR2HTM_LM_BLABOVE)
  { if (ft != JFR2HTM_FT_RADIO_M)
      fprintf(jfr2htm_op, "<BR>");
  }

  if (use_label_mode == JFR2HTM_LM_TABLE
      || use_label_mode == JFR2HTM_LM_MULTABLE)
    fprintf(jfr2htm_op, "</TD><TD>\n");
  switch (ft)
  { case JFR2HTM_FT_PULLDOWN:
      fprintf(jfr2htm_op,
              "<SELECT name=\"%s\" SIZE=1", vdesc.name);
      if (is_ovar == 0)
        fprintf(jfr2htm_op, " onChange=\"%sclr_output()\"", jfr2htm_prefix_txt);
      fprintf(jfr2htm_op, ">\n");
      for (s = 0; s < vdesc.fzvar_c; s++)
      { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + s);
        fprintf(jfr2htm_op, "<OPTION ");
        if (is_ovar == 0)
        { if ((s == 0 && vdesc.default_type != 2)
              || (vdesc.default_type == 2 && s == vdesc.default_no))
            fprintf(jfr2htm_op, "selected ");
        }
        fprintf(jfr2htm_op, "value=\"");
        jfr2htm_float(adesc.center);
        fprintf(jfr2htm_op, "\">%s\n", adesc.name);
      }
      if (is_ovar == 1)
      { /* write an extra empty entry */
        fprintf(jfr2htm_op, "<OPTION selected value = \"0\">\n");
      }
      fprintf(jfr2htm_op, "</SELECT>");
      break;
    case JFR2HTM_FT_CHECKBOX:
      fprintf(jfr2htm_op,
         "<INPUT TYPE= \"checkbox\" NAME=\"%s\" onChange=\"%sclr_output();\" ",
              vdesc.name, jfr2htm_prefix_txt);
      if (vdesc.default_type == 2 && vdesc.default_no == 1)
        fprintf(jfr2htm_op, "checked ");
      jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + 1);
      fprintf(jfr2htm_op, " onClick=\"(this.checked) ? this.value='");
      jfr2htm_float(adesc.center);
      jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no);
      fprintf(jfr2htm_op, "': this.value = '");
      jfr2htm_float(adesc.center);
      fprintf(jfr2htm_op, "%f'\">", adesc.center);
      break;
    case JFR2HTM_FT_RADIO:
    case JFR2HTM_FT_RADIO_M:
      for (s = 0; s < vdesc.fzvar_c; s++)
      { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + s);
        if (ft == JFR2HTM_FT_RADIO_M)
        { if (use_label_mode == JFR2HTM_LM_LEFT)
            fprintf(jfr2htm_op, "\n");
          else
          { if (s > 0
                || (use_label_mode != JFR2HTM_LM_TABLE
                    && use_label_mode != JFR2HTM_LM_MULTABLE))
              fprintf(jfr2htm_op, "<BR>\n");
          }
        }
        fprintf(jfr2htm_op, " <INPUT TYPE=\"radio\" NAME=\"%s\" ",
                vdesc.name);

        if (is_ovar == 0)
        { if ((s == 0 && vdesc.default_type != 2)
              || (vdesc.default_type == 2 && s == vdesc.default_no))
            fprintf(jfr2htm_op, "checked ");
          fprintf(jfr2htm_op, " onChange=\"%sclr_output();\" ",
                  jfr2htm_prefix_txt);
        }
        fprintf(jfr2htm_op, "value=\"");
        jfr2htm_float(adesc.center);
        if (head_mode == 0)
          fprintf(jfr2htm_op, "\"> %s%s%s", jfr2htm_spant("label_radio"),
                    adesc.name, jfr2htm_t_espan);
        else
          fprintf(jfr2htm_op, "\">");
        if (ft == JFR2HTM_FT_RADIO && use_label_mode == JFR2HTM_LM_MULTABLE
            && s != vdesc.fzvar_c - 1)
          fprintf(jfr2htm_op, "</TD><TD>");
      }
      break;
    case JFR2HTM_FT_TEXT_NUM:
    case JFR2HTM_FT_TEXT_ADJ:
      fprintf(jfr2htm_op,
       "<INPUT TYPE=\"text\" NAME=\"%s\" SIZE = 16 ", vdesc.name);
      if (is_ovar == 0)
      { fprintf(jfr2htm_op, " VALUE=");
        jfr2htm_float(vdesc.default_val);
        if ((ddesc.flags & JFS_DF_MINENTER)
            || (ddesc.flags & JFS_DF_MAXENTER)
            || jfr2htm_no_check == 0)
        { fprintf(jfr2htm_op,
                  "\nonChange=\"%svalidate(document.jfs.%s, ",
                  jfr2htm_prefix_txt, vdesc.name);
          if (ddesc.flags & JFS_DF_MINENTER)
            jfr2htm_float(ddesc.dmin);
          else
            fprintf(jfr2htm_op, "document.jfs.%s.value", vdesc.name);
          fprintf(jfr2htm_op, ", ");
          if (ddesc.flags & JFS_DF_MAXENTER)
            jfr2htm_float(ddesc.dmax);
          else
            fprintf(jfr2htm_op, "document.jfs.%s.value", vdesc.name);
          fprintf(jfr2htm_op, ");\"");
        }
      }
      else
        fprintf(jfr2htm_op, " READONLY");
      fprintf(jfr2htm_op, ">");
      break;
  }

  /* write confidence input : */
  if (jfr2htm_vargs.conf == 1)
  { if (use_label_mode == JFR2HTM_LM_MULTABLE)
      fprintf(jfr2htm_op, "</TD><TD>");
    loc_lm = jfr2htm_fdesc.label_mode;
    if (loc_lm == JFR2HTM_LM_ABOVE || loc_lm == JFR2HTM_LM_BLABOVE)
      loc_lm = JFR2HTM_LM_LEFT;
    fprintf(jfr2htm_op, "%s %s %s",
                        jfr2htm_spant(jfr2htm_t_labels[loc_lm]),
                        jfr2htm_conf_text, jfr2htm_t_espan);
    if (use_label_mode == JFR2HTM_LM_MULTABLE)
      fprintf(jfr2htm_op, "</TD><TD>");
    fprintf(jfr2htm_op,
            "<INPUT TYPE=\"text\" NAME=\"conf_%s\" SIZE = 16 ", vdesc.name);
    if (is_ovar == 0)
    { fprintf(jfr2htm_op, " VALUE=");
      jfr2htm_float(jfr2htm_max_conf);
      fprintf(jfr2htm_op,
              "\nonChange=\"%svalidate(document.jfs.conf_%s, 0, ",
              jfr2htm_prefix_txt, vdesc.name);
      jfr2htm_float(jfr2htm_max_conf);
      fprintf(jfr2htm_op, ");\"");
    }
    else
      fprintf(jfr2htm_op, " READONLY");
    fprintf(jfr2htm_op, ">");
  }

  /* write help-button: */
  if (jfr2htm_varuse[vno].vhelp == 1)
  { if (use_label_mode == JFR2HTM_LM_MULTABLE)
      fprintf(jfr2htm_op, "</TD><TD>");
    fprintf(jfr2htm_op, " <INPUT TYPE=\"button\" value=\"Help\" TABINDEX=-1 ");
    fprintf(jfr2htm_op, "onClick=\"%svhelp%d();\">", jfr2htm_prefix_txt, vno);
  }

  if (jfr2htm_vargs.no_nl == 0)
    fprintf(jfr2htm_op, "\n");
  else
    fprintf(jfr2htm_op, " ");

  /* newlines after field (no newline after JFR2HTM_LM_LEFT because then
     the block is in <PRE> */
  if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_BLABOVE
      || jfr2htm_fdesc.label_mode == JFR2HTM_LM_ABOVE)
    fprintf(jfr2htm_op, "<BR>\n");
  if (use_label_mode == JFR2HTM_LM_BLABOVE)
    fprintf(jfr2htm_op, "<BR>\n");
  if (use_label_mode == JFR2HTM_LM_TABLE
       || use_label_mode == JFR2HTM_LM_MULTABLE)
  { fprintf(jfr2htm_op, "</TD>\n");
    if (jfr2htm_vargs.no_nl == 0)
      fprintf(jfr2htm_op, "</TR>");
    fprintf(jfr2htm_op, "\n");
  }
}

static void jfr2htm_form_test(void)
{
  int m;
  struct jfg_var_desc vdesc;
  int le, on_new_line;

  jfr2htm_fdesc.fs_mode = 0;
  jfr2htm_fdesc.lab_size = 0;
  jfr2htm_fdesc.fs_no = 0;
  on_new_line = 1;
  for (m = 0; m < jfr2htm_spdesc.var_c; m++)
  { if (  (m >= jfr2htm_spdesc.f_ivar_no
            && m < jfr2htm_spdesc.f_ivar_no + jfr2htm_spdesc.ivar_c)
        || (m >= jfr2htm_spdesc.f_ovar_no
            && m < jfr2htm_spdesc.f_ovar_no + jfr2htm_spdesc.ovar_c))
    {
      jfr2htm_vget(&vdesc, m, 0);
      if (jfr2htm_vargs.va_type != JFR2HTM_VT_FIELDSET &&
          on_new_line == 1 &&
          (jfr2htm_use_args == 0 || jfr2htm_vargs.com_label == 0))
      { le = strlen(vdesc.text);
        if (le == 0)
          le = strlen(vdesc.name);
        if (le > jfr2htm_fdesc.lab_size && jfr2htm_fdesc.label_mode == JFR2HTM_LM_LEFT)
          jfr2htm_fdesc.lab_size = le;
      }
      if (jfr2htm_vargs.va_type == JFR2HTM_VT_FIELDSET)
        jfr2htm_fdesc.fs_mode = 1;
      if (jfr2htm_vargs.no_nl == 1)
        on_new_line = 0;
      else
        on_new_line = 1;
    }
  }
  jfr2htm_fdesc.lab_size++;
}

static int jfr2htm_fset_write(int rel_from_v, int is_ovar)
{
  /* find the number of variables in the field-set, label-size and
     head-mode. Writes field-set-command and label:                     */

  int from_v, ff_var_no, ft, ff_v, fundet, le;
  int on_new_line;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;

  if (is_ovar == 0)
  { from_v = jfr2htm_spdesc.f_ivar_no + rel_from_v;
    ff_var_no = jfr2htm_spdesc.f_ivar_no + jfr2htm_spdesc.ivar_c;
  }
  else
  { from_v = jfr2htm_spdesc.f_ovar_no + rel_from_v;
    ff_var_no = jfr2htm_spdesc.f_ovar_no + jfr2htm_spdesc.ovar_c;
  }

  jfr2htm_fdesc.label_mode = jfr2htm_label_mode;
  jfr2htm_fdesc.no_legend = 0;
  jfr2htm_fdesc.no_fieldset = 0;
  jfr2htm_fdesc.tarea_started = 0;
  jfr2htm_vget(&vdesc, from_v, 1);
  if (is_ovar == 1 && from_v + 1 == ff_var_no
      && jfr2htm_vargs.va_type == JFR2HTM_VT_FIELDSET)
    jfr2htm_fdesc.tarea_started = 1;

  fprintf(jfr2htm_op, "%s", jfr2htm_divt("o_fieldset"));
  if (jfr2htm_fdesc.no_fieldset == 0)
    fprintf(jfr2htm_op, "<FIELDSET>");
  if (jfr2htm_vargs.va_type == JFR2HTM_VT_FIELDSET)
  { /* use text and comment to make header of fieldset: */
    ft = -1;
    if (strlen(vdesc.text) > 0 && jfr2htm_fdesc.no_legend == 0)
      fprintf(jfr2htm_op, "%s<LEGEND>%s</LEGEND>%s",
              jfr2htm_spant("label_legend"), vdesc.text, jfr2htm_t_espan);
    fprintf(jfr2htm_op, "%s", jfr2htm_divt("i_fieldset"));
    if (jfr2htm_use_ssheet == 1)
      fprintf(jfr2htm_op, "<DIV CLASS=\"fs_%d\">",  jfr2htm_fdesc.fs_no);
    if (jfr2htm_vargs.com_label == 1)
    { fprintf(jfr2htm_op, "%s", jfr2htm_divt("label_fieldset"));
      if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_LEFT
          || jfr2htm_fdesc.label_mode == JFR2HTM_LM_TABLE
          || jfr2htm_fdesc.label_mode == JFR2HTM_LM_MULTABLE)
        jfr2htm_write_comment(vdesc.comment_no, 0); /* no <CR> in comment */
      else
        jfr2htm_write_comment(vdesc.comment_no, 1);
      fprintf(jfr2htm_op, "%s", jfr2htm_t_ediv);
      /* if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_ABOVE
             || jfr2htm_fdesc.label_mode == JFR2HTM_LM_BLABOVE)
           fprintf(jfr2htm_op, "<BR>");
       */
    }
  }
  else
  { ft = jfr2htm_vargs.ft_type;
    fprintf(jfr2htm_op, "%s", jfr2htm_divt("i_fieldset"));
    if (jfr2htm_use_ssheet == 1)
      fprintf(jfr2htm_op, "<DIV CLASS=\"fs_%d\">", jfr2htm_fdesc.fs_no);
  }
  fprintf(jfr2htm_op, "\n");

  if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_LEFT)
    fprintf(jfr2htm_op, "<PRE>\n");
  ff_v = from_v + 1;
  /* check if first variable is a radio-button: */
  jfr2htm_fdesc.head_mode = 0;
  if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_MULTABLE)
  { if (ft == JFR2HTM_FT_RADIO)
    { jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
      if (ddesc.f_adjectiv_no == vdesc.f_adjectiv_no
          && ddesc.adjectiv_c == vdesc.fzvar_c)
      { jfr2htm_fdesc.head_mode = 1;
        jfr2htm_fdesc.head_f_adjective_no = ddesc.f_adjectiv_no;
        jfr2htm_fdesc.head_adjective_c = ddesc.adjectiv_c;
      }
    }
    else
    if (jfr2htm_vargs.va_type == JFR2HTM_VT_FIELDSET)
    { jfr2htm_fdesc.head_mode = 1;
      jfr2htm_fdesc.head_adjective_c = -1;
    }
  }
  else
    jfr2htm_fdesc.head_mode = 0;

  jfr2htm_fdesc.lab_size = 1;
  if (jfr2htm_vargs.va_type != JFR2HTM_VT_FIELDSET
      && jfr2htm_vargs.com_label == 0)
  { le = strlen(vdesc.text);
    if (le == 0)
      le = strlen(vdesc.name);
    if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_LEFT)
      jfr2htm_fdesc.lab_size = le;
  }
  if (jfr2htm_vargs.no_nl == 1)
    on_new_line = 0;
  else
    on_new_line = 1;

  /* find last variable in fieldset, head-mode and label-size: */
  fundet = 0;
  while (fundet == 0)
  { if (ff_v >= ff_var_no)
      fundet = 1;
    else
    { jfr2htm_vget(&vdesc, ff_v, 0);
      ft = jfr2htm_vargs.ft_type;
      if (jfr2htm_vargs.va_type == JFR2HTM_VT_FIELDSET)
        fundet = 1;
      else
      { /* check for head_mode: */
        if (jfr2htm_fdesc.head_mode == 1 && ft == JFR2HTM_FT_RADIO)
        { if (jfr2htm_fdesc.head_adjective_c == -1)
          { jfg_domain(&ddesc, jfr_head, vdesc.domain_no);
            if (ddesc.f_adjectiv_no == vdesc.f_adjectiv_no
                && ddesc.adjectiv_c == vdesc.fzvar_c)
            { jfr2htm_fdesc.head_f_adjective_no = ddesc.f_adjectiv_no;
              jfr2htm_fdesc.head_adjective_c = ddesc.adjectiv_c;
            }
            else
              jfr2htm_fdesc.head_mode = 0;
          }
          else
          { if (vdesc.f_adjectiv_no != jfr2htm_fdesc.head_f_adjective_no ||
                vdesc.fzvar_c != jfr2htm_fdesc.head_adjective_c)
               jfr2htm_fdesc.head_mode = 0;
          }
        }
        else
          jfr2htm_fdesc.head_mode = 0;
        /* calculate label-size: */
        if (on_new_line == 1 &&
            (jfr2htm_use_args == 0 || jfr2htm_vargs.com_label == 0))
        { le = strlen(vdesc.text);
          if (le == 0)
            le = strlen(vdesc.name);
          if (le > jfr2htm_fdesc.lab_size
              && jfr2htm_fdesc.label_mode == JFR2HTM_LM_LEFT)
            jfr2htm_fdesc.lab_size = le;
        }
        if (jfr2htm_vargs.no_nl == 1)
           on_new_line = 0;
        else
          on_new_line = 1;
        ff_v++;
      }
    }
  }
  if (jfr2htm_fdesc.tarea_started == 1)
    jfr2htm_fdesc.head_mode = 0;

  jfr2htm_fdesc.table_mode = 0;
  if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_MULTABLE)
  { if (jfr2htm_fdesc.head_mode == 0)
      jfr2htm_fdesc.table_mode = 1;
    else
      jfr2htm_fdesc.table_mode = 2;
  }
  if (is_ovar == 0)
    return ff_v - jfr2htm_spdesc.f_ivar_no;
  else
    return ff_v - jfr2htm_spdesc.f_ovar_no;
}

static void jfr2htm_form_write(void)
{
  int m, s, ano;
  int fs_mode;  /* 0 : no field-sets, write everything in a single pass. */
                /* 1 : input-variables.                                  */
                /* 2 : buttons.                                          */
                /* 3 : output-variables.                                 */
                /* 4 : text-areas.                                       */
                /* 5 : finished.                                         */
  int from_v, ff_v; /* first and last variable in current field-set.     */
  int on_new_line;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;

  jfr2htm_fdesc.label_mode = jfr2htm_label_mode;
  jfr2htm_fdesc.no_legend = 0;
  jfr2htm_fdesc.no_fieldset = 0;
  jfr2htm_form_test();
  fs_mode = jfr2htm_fdesc.fs_mode;
  jfr2htm_fdesc.fs_no = 0;

  fprintf(jfr2htm_op, "%s\n", jfr2htm_t_begin);
  fprintf(jfr2htm_op, "%s\n", jfr2htm_divt("jfr2htm_form"));
  /* fprintf(jfr2htm_op, "<HR>\n"); */
  fprintf(jfr2htm_op, "<FORM NAME=\"jfs\">\n");
  if (fs_mode == 0)
  { jfr2htm_fdesc.label_mode = jfr2htm_label_mode;
    if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_LEFT)
      fprintf(jfr2htm_op, "<PRE>\n");
  }

  /* main loop */
  from_v = 0;
  fprintf(jfr2htm_op, "%s", jfr2htm_divt("ip_block"));
  while (fs_mode != 5)
  { if (fs_mode == 0 || fs_mode == 1)
    { /* write input-variables: */
      if (fs_mode == 0)
        ff_v = jfr2htm_spdesc.ivar_c; /* write all input-variables. */
      else
        ff_v = jfr2htm_fset_write(from_v, 0);

      if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_TABLE
          || jfr2htm_fdesc.label_mode == JFR2HTM_LM_MULTABLE)
      { fprintf(jfr2htm_op, "%s",
                 jfr2htm_divt(jfr2htm_t_tables[jfr2htm_fdesc.table_mode]));
        fprintf(jfr2htm_op, "<TABLE>\n");
      }
      if (jfr2htm_fdesc.head_mode == 1 && jfr2htm_fdesc.head_adjective_c > 0)
      { fprintf(jfr2htm_op, "<THEAD><TR><TH></TH>");
        for (ano = jfr2htm_fdesc.head_f_adjective_no;
             ano < jfr2htm_fdesc.head_f_adjective_no
                   + jfr2htm_fdesc.head_adjective_c; ano++)
        { jfg_adjectiv(&adesc, jfr_head, ano);
          fprintf(jfr2htm_op, "<TH>%s</TH>", adesc.name);
        }
        fprintf(jfr2htm_op, "</TR></THEAD>\n");
      }
      if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_TABLE
          || jfr2htm_fdesc.label_mode == JFR2HTM_LM_MULTABLE)
          fprintf(jfr2htm_op, "<TBODY>\n");
      on_new_line = 1;
      for (m = from_v; m < ff_v; m++)
      { jfr2htm_vget(&vdesc, jfr2htm_spdesc.f_ivar_no + m, 0);
        jfr2htm_formvar_write(jfr2htm_spdesc.f_ivar_no + m, 0,
                              on_new_line * jfr2htm_fdesc.lab_size,
                              jfr2htm_fdesc.head_mode, on_new_line);
        if (jfr2htm_vargs.no_nl == 1)
          on_new_line = 0;
        else
          on_new_line = 1;
      }
      from_v = ff_v;
      jfr2htm_fdesc.fs_no++;
    }

    if (fs_mode == 0 || fs_mode == 2)
    { /* write buttons */
      fprintf(jfr2htm_op, "%s%s", jfr2htm_t_ediv, jfr2htm_divt("button_block"));
      if (fs_mode == 2 && jfr2htm_label_mode == JFR2HTM_LM_LEFT)
        fprintf(jfr2htm_op, "<PRE>");
      if (jfr2htm_label_mode == JFR2HTM_LM_ABOVE)
        fprintf(jfr2htm_op, "<BR>\n");
      if (fs_mode == 0 && (jfr2htm_label_mode == JFR2HTM_LM_TABLE
                           || jfr2htm_label_mode == JFR2HTM_LM_MULTABLE))
        fprintf(jfr2htm_op, "<TR><TD></TD><TD>\n");
      fprintf(jfr2htm_op, "<INPUT TYPE=\"button\"\nvalue=\"");
      for (s = 0; s < jfr2htm_fdesc.lab_size + 8; s++)
        fprintf(jfr2htm_op, " ");
      fprintf(jfr2htm_op, "Calculate");
      for (s = 0; s < jfr2htm_fdesc.lab_size + 8; s++)
        fprintf(jfr2htm_op, " ");
      fprintf(jfr2htm_op,
              "\"\nonClick=\"%sjfr();\">", jfr2htm_prefix_txt);
      if (fs_mode == 0 && jfr2htm_label_mode == JFR2HTM_LM_MULTABLE)
        fprintf(jfr2htm_op, "</TD><TD>");
      fprintf(jfr2htm_op, "<INPUT TYPE=\"RESET\" VALUE=\"Reset\">\n");
      if (jfr2htm_label_mode == JFR2HTM_LM_BLABOVE
          || jfr2htm_label_mode == JFR2HTM_LM_ABOVE)
        fprintf(jfr2htm_op, "<BR><BR>\n");
      if (fs_mode == 0 && (jfr2htm_label_mode == JFR2HTM_LM_TABLE
                           || jfr2htm_label_mode == JFR2HTM_LM_MULTABLE))
        fprintf(jfr2htm_op, "</TD></TR>\n");
      if (fs_mode == 2 && jfr2htm_label_mode == JFR2HTM_LM_LEFT)
        fprintf(jfr2htm_op, "</PRE>");
      fprintf(jfr2htm_op, "%s", jfr2htm_t_ediv);
      fprintf(jfr2htm_op, "%s", jfr2htm_divt("op_block"));
    }

    if (fs_mode == 0 || fs_mode == 3)
    { /* write output variables */
      if (fs_mode == 0)
      { from_v = 0;
        ff_v = jfr2htm_spdesc.ovar_c; /* write all output-variables. */
      }
      else
        ff_v = jfr2htm_fset_write(from_v, 1);

      if (fs_mode != 0
          && (jfr2htm_fdesc.label_mode == JFR2HTM_LM_TABLE
              || jfr2htm_fdesc.label_mode == JFR2HTM_LM_MULTABLE))
      { fprintf(jfr2htm_op, "%s",
                  jfr2htm_divt(jfr2htm_t_tables[jfr2htm_fdesc.table_mode]));
         fprintf(jfr2htm_op, "<TABLE><TBODY>\n");
      }
      on_new_line = 1;
      for (m = from_v; m < ff_v; m++)
      { jfr2htm_vget(&vdesc, jfr2htm_spdesc.f_ovar_no + m, 0);
        jfr2htm_formvar_write(jfr2htm_spdesc.f_ovar_no + m, 1,
                              on_new_line * jfr2htm_fdesc.lab_size, 0, on_new_line);
        if (jfr2htm_vargs.no_nl == 1)
          on_new_line = 0;
        else
          on_new_line = 1;
      }
      from_v = ff_v;
      jfr2htm_fdesc.fs_no++;
    }

    if (fs_mode == 0 || fs_mode == 4)
    { /* write text-araes: */
      /* fprintf(jfr2htm_op, "%s\n", jfr2htm_t_ediv);  end of op_block */
      if (jfr2htm_ff_textarea > 0)
      { /* fprintf(jfr2htm_op, "%s", jfr2htm_divt("ta_block")); */
        if (fs_mode == 4)
        { if (jfr2htm_fdesc.tarea_started == 0)
          { fprintf(jfr2htm_op, "%s<FIELDSET>", jfr2htm_divt("o_fieldset"));
            fprintf(jfr2htm_op, "%s", jfr2htm_divt("i_fieldset"));
            if (jfr2htm_use_ssheet == 1)
              fprintf(jfr2htm_op, "<DIV CLASS=\"fs_%d\">", jfr2htm_fdesc.fs_no);
          }
          jfr2htm_fdesc.fs_no++;
          if (jfr2htm_label_mode == JFR2HTM_LM_LEFT)
            fprintf(jfr2htm_op, "<PRE>\n");
        }
        if (fs_mode != 0 && jfr2htm_fdesc.tarea_started == 0
              && (jfr2htm_label_mode == JFR2HTM_LM_TABLE
                  || jfr2htm_label_mode == JFR2HTM_LM_MULTABLE))
        { fprintf(jfr2htm_op, "%s", jfr2htm_divt(jfr2htm_t_tables[0]));
          fprintf(jfr2htm_op, "<TABLE><TBODY>\n");
        }
        for (m = 0; m < jfr2htm_ff_textarea; m++)
        { if (jfr2htm_label_mode == JFR2HTM_LM_TABLE
             || jfr2htm_label_mode == JFR2HTM_LM_MULTABLE)
            fprintf(jfr2htm_op, "<TR><TD>\n");
          fprintf(jfr2htm_op, "%s%s%s\n",
                          jfr2htm_spant(jfr2htm_t_labels[jfr2htm_label_mode]),
                          jfr2htm_textareas[m].label, jfr2htm_t_espan);
          if (jfr2htm_label_mode == JFR2HTM_LM_BLABOVE
              || jfr2htm_label_mode == JFR2HTM_LM_ABOVE)
            fprintf(jfr2htm_op, "<BR>\n");
          if (jfr2htm_label_mode == JFR2HTM_LM_TABLE
              || jfr2htm_label_mode == JFR2HTM_LM_MULTABLE)
            fprintf(jfr2htm_op, "</TD><TD>\n");
          fprintf(jfr2htm_op, "<TEXTAREA NAME=\"%s\" COLS=%d ROWS=%d READONLY>\n",
                  jfr2htm_textareas[m].name, jfr2htm_textareas[m].cols,
                  jfr2htm_textareas[m].rows);
          fprintf(jfr2htm_op, "</TEXTAREA>\n");
          if (jfr2htm_label_mode == JFR2HTM_LM_BLABOVE
              || jfr2htm_label_mode == JFR2HTM_LM_ABOVE)
            fprintf(jfr2htm_op, "<BR>\n");
          if (jfr2htm_label_mode == JFR2HTM_LM_BLABOVE)
            fprintf(jfr2htm_op, "<BR>\n");
          if (jfr2htm_label_mode == JFR2HTM_LM_TABLE
              || jfr2htm_label_mode == JFR2HTM_LM_MULTABLE)
            fprintf(jfr2htm_op, "</TD></TR>\n");
        }
      }
    }
    switch (fs_mode)
    { case 0: /* no fieldsets */
        if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_TABLE
            || jfr2htm_fdesc.label_mode == JFR2HTM_LM_MULTABLE)
          fprintf(jfr2htm_op, "</TBODY></TABLE>%s\n", jfr2htm_t_ediv);
        if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_LEFT)
          fprintf(jfr2htm_op, "</PRE>\n");
        fs_mode = 5;
        break;
      case 1: /* input-block */
        if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_TABLE
            || jfr2htm_fdesc.label_mode == JFR2HTM_LM_MULTABLE)
          fprintf(jfr2htm_op, "</TBODY></TABLE>%s\n", jfr2htm_t_ediv);
        if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_LEFT)
          fprintf(jfr2htm_op, "</PRE>");
        fprintf(jfr2htm_op, "%s%s", jfr2htm_t_ediv, jfr2htm_t_ediv);
        if (jfr2htm_fdesc.no_fieldset == 0)
          fprintf(jfr2htm_op, "</FIELDSET>");
        fprintf(jfr2htm_op, "%s\n", jfr2htm_t_ediv);
        if (from_v >= jfr2htm_spdesc.ivar_c)
          fs_mode = 2;
        break;
      case 2: /* buttons */
        from_v = 0;
        fs_mode = 3;
        break;
      case 3: /* output-block */
        if (jfr2htm_fdesc.tarea_started == 0
            && (jfr2htm_fdesc.label_mode == JFR2HTM_LM_TABLE
                || jfr2htm_fdesc.label_mode == JFR2HTM_LM_MULTABLE))
          fprintf(jfr2htm_op, "</TBODY></TABLE>%s\n", jfr2htm_t_ediv);
        if (jfr2htm_fdesc.label_mode == JFR2HTM_LM_LEFT)
          fprintf(jfr2htm_op, "</PRE>");
        if (jfr2htm_fdesc.tarea_started == 0)
        { fprintf(jfr2htm_op, "%s%s", jfr2htm_t_ediv, jfr2htm_t_ediv);
          if (jfr2htm_fdesc.no_fieldset == 0)
            fprintf(jfr2htm_op, "</FIELDSET>");
          fprintf(jfr2htm_op, "%s\n", jfr2htm_t_ediv);
        }
        if (from_v >= jfr2htm_spdesc.ovar_c)
          fs_mode = 4;
        break;
      case 4: /* text-areas */
        if (jfr2htm_ff_textarea > 0 || jfr2htm_fdesc.tarea_started == 1)
        { if (jfr2htm_label_mode == JFR2HTM_LM_TABLE
              || jfr2htm_label_mode == JFR2HTM_LM_MULTABLE)
            fprintf(jfr2htm_op, "</TBODY></TABLE>%s\n", jfr2htm_t_ediv);
          if (jfr2htm_label_mode == JFR2HTM_LM_LEFT)
            fprintf(jfr2htm_op, "</PRE>\n");
          fprintf(jfr2htm_op, "%s%s</FIELDSET>%s\n",
                   jfr2htm_t_ediv, jfr2htm_t_ediv, jfr2htm_t_ediv);
        }
        fprintf(jfr2htm_op, "%s\n", jfr2htm_t_ediv); /* end of output-block */
        fs_mode = 5;
        break;
    }
  }

  /* write end of form */
  fprintf(jfr2htm_op, "</FORM>\n");
  fprintf(jfr2htm_op, "%s\n", jfr2htm_t_ediv);
  fprintf(jfr2htm_op, "%s\n", jfr2htm_t_end);
}

static void jfr2htm_end_write(void)
{
  fprintf(jfr2htm_op, "</BODY>\n</HTML>\n");
}

static void jfr2htm_end_copy(void)
{
  int state;

  state = 0;
  while (state == 0)
  { if (fgets(jfr2htm_text, jfr2htm_maxtext, jfr2htm_ip) == NULL)
      state = 1;
    if (state == 0)
      fprintf(jfr2htm_op, "%s", jfr2htm_text);
  }
}

int jfr2h_conv(char *op_hfname, char *op_jfname,
               char *jfr_fname, char *so_fname,
               int prog_only, int precision, int overwrite, int js_file,
               int maxtext, int maxtree, int maxstack, int no_check,
               char *ssheet, int use_args, int label_mode, int main_comment,
               char *conf_txt, float max_conf, char *prefix_txt,
               FILE *sout)
{
  int m;
  int res;

  res = 0;
  jfr2htm_sout = sout;
  jfr2htm_overwrite = overwrite;
  jfr2htm_js_file   = js_file;
  jfr2htm_no_check = no_check;
  jfr2htm_use_args = use_args;
  jfr2htm_label_mode = label_mode;
  jfr2htm_main_comment = main_comment;
  jfr2htm_max_conf = max_conf;
  strcpy(jfr2htm_conf_text, conf_txt);
  strcpy(jfr2htm_prefix_txt, prefix_txt);
  if (ssheet != NULL && strlen(ssheet) > 0)
  { jfr2htm_use_ssheet = 1;
    strcpy(jfr2htm_ssheet, ssheet);
    strcpy(jfr2htm_t_ediv, "</DIV>");
    strcpy(jfr2htm_t_espan, "</SPAN>");
  }
  else
  { jfr2htm_use_ssheet = 0;
    jfr2htm_ssheet[0] = '\0';
    jfr2htm_t_ediv[0] = '\0';
    jfr2htm_t_espan[0] = '\0';
  }
  if ((m = jfr_init(0)) != 0)
  { jf_error(m, jf_empty, JFE_FATAL);
    return m;
  }
  if ((m = jfr_load(&jfr_head, jfr_fname)) != 0)
  { jf_error(m, jfr_fname, JFE_FATAL);
    return m;
  }
  if (jfr2htm_overwrite == 0)
  { if ((jfr2htm_ip = fopen(op_hfname, "r")) == NULL)
      jfr2htm_overwrite = 1;
    else
    { fclose(jfr2htm_ip);
      if ((jfr2htm_ip = fopen(so_fname, "r")) == NULL)
        jf_error(m, so_fname, JFE_FATAL);
    }
  }
  if ((jfr2htm_oph = fopen(op_hfname, "w")) == NULL)
  { jf_error(m, op_hfname, JFE_FATAL);
    return 1;
  }
  jfr2htm_op = jfr2htm_oph;
  if (jfr2htm_js_file == 1)
  { if ((jfr2htm_opj = fopen(op_jfname, "w")) == NULL)
      return jf_error(m, op_jfname, JFE_FATAL);
  }

  if (jfr2htm_overwrite == 1)
    prog_only = 0;
  jfr2htm_digits = precision;
  jfr2htm_maxtext = maxtext;

  jfr2htm_text = (char *) malloc(jfr2htm_maxtext);
  if (jfr2htm_text == NULL)
    return jf_error(6, " text", JFE_FATAL);
  jfr2htm_text[0] = '\0';

  jfr2htm_maxtree = maxtree;
  m = sizeof(struct jfg_tree_desc) * jfr2htm_maxtree;
  jfr2htm_tree = (struct jfg_tree_desc *) malloc(m);
  if (jfr2htm_tree == NULL)
    return jf_error(6, "conversion tree", JFE_FATAL);

  jfr2htm_comm_size = 512;
  jfr2htm_comment = (char *) malloc(jfr2htm_comm_size);
  if (jfr2htm_comment == NULL)
    return jf_error(6, " comment", JFE_FATAL);

  jfr2htm_stacksize = maxstack;

  m = jfg_init(0, jfr2htm_stacksize, jfr2htm_digits);
  if (m != 0)
    return jf_error(m, jfr2htm_t_stack, JFE_FATAL);

  jfg_sprg(&jfr2htm_spdesc, jfr_head);

  m = sizeof(struct jfr2htm_varuse_desc) * jfr2htm_spdesc.var_c;
  jfr2htm_varuse = (struct jfr2htm_varuse_desc *) malloc(m);
  if (jfr2htm_varuse == NULL)
    return jf_error(6, jfr2htm_spaces, JFE_FATAL);

  if (jfr2htm_overwrite == 1)
    res = jfr2htm_head_write();
  else
    res = jfr2htm_head_copy(0);

  if (res == 0)
    res = jfr2htm_program_write(op_jfname);

  if (res == 0)
  { if (jfr2htm_overwrite == 1)
      jfr2htm_body_write();
    else
      jfr2htm_body_copy(prog_only);

    if (prog_only == 0)
      jfr2htm_form_write();

    if (jfr2htm_overwrite == 1)
      jfr2htm_end_write();
    else
      jfr2htm_end_copy();
  }

  if (jfr2htm_errcount > 0)
    return jf_error(1020, jfr2htm_spaces, JFE_FATAL);

  jfg_free();
  free(jfr2htm_tree);
  free(jfr2htm_text);

  if (jfr2htm_ip != NULL)
    fclose(jfr2htm_ip);
  fclose(jfr2htm_oph);
  if (jfr2htm_js_file == 1)
    fclose(jfr2htm_opj);
  jfr_close(&jfr_head);
  jfr_free();

  return 0;
}


