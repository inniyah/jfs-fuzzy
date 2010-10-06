  /*********************************************************************/
  /*                                                                   */
  /* jfr.c   Version  2.03   Copyright (c) 1998-2000 Jan E. Mortensen  */
  /*                                                                   */
  /* JFS- command-line run program.                                    */
  /*                                                                   */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                */
  /*    Lollandsvej 35 3.tv.                                           */
  /*    DK-2000 Frederiksberg                                          */
  /*    Denmark                                                        */
  /*                                                                   */
  /*********************************************************************/

#define JFE_WARNING 0
#define JFE_ERROR   1
#define JFE_FATAL   2

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jft_lib.h"
#include "jfs_cons.h"

const char *usage[] =
{ "usage: jfr [-f fs] [-p dec] [-m md] [-c] [-e] [-wa [wm]] [-lm m]",
  "           [-D [dam]] [-O [opm]] [-d {daf}] [-o [opf]] [-l [lf]]",
  "           [-rs ss] [-u [ua]] [-w] [-a] [-uv m] [-pm [mf]]        jfrf"
};

struct jf_option_desc {
	const char *option;
	int argc;      /* -1: variabelt */
};               /* -2: sidste argument */

struct jf_option_desc jf_options[] =
{
	{"-f",  1},        /*  0 */
	{"-p",  1},        /*  1 */
	{"-m",  1},        /*  2 */
	{"-e",  0},        /*  3 */
	{"-wa",-1},        /*  4 */
	{"-c",  0},        /*  5 */
	{"-D", -1},        /*  6 */
	{"-O", -1},        /*  7 */
	{"-d", -1},        /*  8 */
	{"-o", -1},        /*  9 */
	{"-l", -1},        /* 10 */
	{"-lm", 1},        /* 11 */
	{"-a",  0},        /* 12 */
	{"-w",  0},        /* 13 */
	{"-rs", 1},        /* 14 */
	{"-u", -1},        /* 15 */
	{"-uv", 1},        /* 16 */
	{"-pm", -1},       /* 17 */
	{"-?",  0},        /* 18 */
	{"?",   0},        /* 19 */
	{" ",  -2}
};

const char *extensions[]  = {
	"jfr",     /* 0 */
	"log",     /* 1 */
	"dat",     /* 2 */
	"jfo",     /* 3 */
	"pm",      /* 4 */
	"plt"      /* 5 */
};

FILE *jfs_ip  = NULL;
FILE *jfs_op  = NULL;
FILE *jfs_log = NULL;

void *jf_head = NULL;

struct jfg_sprog_desc spdesc;

char *amemory = NULL;  /* alocated memory */

int file_no; /* aktuel filnr */

/*****************************************************************/
/* Variables to input, output and expected values.               */
/*****************************************************************/

char so_fname[256] = "";
char lo_fname[256] = "";
char ip_fname[256] = "";
char op_fname[256] = "";
char pm_fname[256] = "";

struct jft_data_record *ip_vars;
struct jft_data_record *exp_vars;

int ivar_c;
int ovar_c;

int ip_no;

float *ivar_values;
float *ovar_values;
float *confidences;

#define V_IVAR_C_MAX 8
struct v_ivar_desc      /* input variable angivet med '*' */
		   { float start_v;
		     float addent;
		     int   ivar_no;
		     int   antal;     /* -1: step centers */
		     int   akt;
		   };

struct v_ivar_desc v_ivars[V_IVAR_C_MAX];
int v_ivar_c;
char ident_text[256] = "";

/*****************************************************************/
/* Option vars                                                   */
/*****************************************************************/

int dc_comma   = 0;    /* 1: use decimal-comma.                  */


#define IP_KEYBOARD  0
#define IP_FILE      1
int ip_medie  = IP_KEYBOARD;

/* data-mode (input-data contains):                              */
#define DM_INP      0
#define DM_INP_EXP  1
#define DM_EXP_INP  2
#define DM_EXP      3
#define DM_NONE     4
int data_mode = DM_INP;

/* data identifer-mode */
#define ID_NONE    0
#define ID_START   1
#define ID_END     2
int ident_mode = ID_NONE;

int fmode = JFT_FM_INPUT;  /* jft-file-mode, is converted to data_mode, ident_mode */

/* undefined mode: */
#define UM_NONE        0
#define UM_INPUT       1
#define UM_LOCAL       2
#define UM_INPUT_LOCAL 3
int undef_mode = UM_INPUT;

int dfile_c   = 1;     /* no off data-files.                     */
int dfile_nos[16];    /* argv[d_file_nos[0]= first datafil.      */
		                     /* -1: dfilename = jfrname.dat.           */
int this_eof  = 0;     /* eof actual file.                       */

int d_filt_mode = 0;   /* 0: dont use distance-filter,           */
		                     /* 1: use distance filter.                */

float d_filter = 0.0;  /* distance filter value.                 */

int a_filt_mode = 0;   /* 0: dont use adjectiv-error-filter,     */
		                     /* 1: use adjectiv-error-filter.          */

int op_stat       = -1;/* 1: write output-statistic.             */
int op_ivars      = 0; /* 1: write ivars,                        */
int op_ovars      = 0; /* 1: write ovars,                        */
int op_expected   = 0; /* 1: write expected values,              */
int op_text       = 0; /* 1: write texts,                        */
int op_fuzzy      = 0; /* 1: write fuzzy output variables.       */
int op_key        = 0; /* 1: write key-value in line.            */
int op_header     = 0; /* 1: write header-line.                  */

int op_penalty    = 0; /* 1: use penalty matrix.                 */

int op_vformat    = -1;/* 0: write variable values as floats,    */
		                     /* 1: write variable values as integers,  */
		                     /* 2: write variable values as adjectives.*/
		                     /*-1: benyt type fra doamin.              */

int op_append     = 0;

int digits = 4;        /* no of digits after the decimal point (incl point).*/

#define LM_NONE    0
#define LM_DEBUG   1
#define LM_CHANGED 2
#define LM_ALL     3
int log_mode  = -1;    /* 0: ingen log,                          */
                       /* 1: write changed to log,               */
                       /* 2: log for alle fyrende regler.        */

int warn_input   = 0;  /* warnings if variable out of range.     */
int warn_calc    = 0;  /* warnings if illegal function values.   */
int warn_stack   = 1;  /* warnings if stack over/under-flow.     */

int undef_value  = JFR_FCLR_DEFAULT;

char field_sep[255];   /* 0: brug space, tab etc som felt-seperator, */
              		       /* andet: kun field_sep er feltsepator. */

int stack_size   = 0;

int wait_return  = 0;


/********************************************************************/
/* variables to statistics                                          */
/********************************************************************/

int dset_adj_err_c;  /* antal adj fejl aktuelle data-set.          */
float dset_dist;     /* dist(exp, ovars) aktuelle data-set.        */

struct dset_c_desc { int count[4];  /* 0: no of datasets in datafile. */
                                    /* 1: no of sets after filtering. */
                                    /* 2: no of sets with errrors.    */
                                    /* 3: no of set with missing val. */
	              	   };

struct dset_c_desc *dset_cs;  /* dset_c[datafile_c + 1];    */

struct tot_stat_desc { int   worst_no;    /* dset_no med stoerste err. */
                       float worst_value; /* stoerste fejl.            */
                       float dist_sum;    /* sum(distance(ovars, exps))*/
                       float sqr_sum;     /* sum(sqr(ovars, exps))     */
                       float pen_sum;     /* sum(penalty),             */
                       int   adj_err_c;   /* no adj error,             */
                       int   adj_d_err_c; /* no dsets with adj errors. */
		 };

struct tot_stat_desc *tot_stats; /* tot_stats[datafile_c + 1]       */

int worst_file = 0;  /* fil nr paa filen med worst_value.           */

int df_c; /* if dfile_c == 1 df_f = 1 else df_c = dfile_c + 1.      */

const char *c_t[] = {
	"Datasets                     ",
	"Used datasets                ",
	"Datasets with error          ",
	"Datasets with missing values "
};

char t5[] =     "Avg distance                 ";
char t6[] =     "Worst distance               ";
char t7[] =     "Worst dataset no             ";
char t8[] =     "Adjective errors             ";
char t9[] =     "Datasets with adj. errors    ";
char t10[]=     "Percent correct              ";
char t11[]=     "Sum(distance)                ";
char t12[]=     "Sum(sqr(difference))         ";
char t13[]=     "Sum(penalty)                 ";
char tt[] = "Total";


/*******************************************************************/
/* Variables to output-formating                                   */
/*******************************************************************/

char jf_empty[] = " ";
char jf_space[80];
char jfs_txt[512];

int jfr_t_len;       /* length var-text field.                     */
int jfr_f_len;       /* length float-field.                        */

/* extern char* jfg_t_operators[]; */
char jf_file_txt[256];

  char c_text[]  = "calculated";
  char e_text[]  = "expected";
  char er_text[] = "error";
  char pe_text[] = "pct.err";
  char em_text[] = "OUTPUT";
  char fv_text[] = "FUZZY OUTPUT";
  char is_text[] = " is ";

/*******************************************************************/
/* Variable til call-statement.                                    */
/*******************************************************************/

#define MAX_CALL_ARGS 32
const char *call_args[MAX_CALL_ARGS];

const char call_t_printf[] = "printf";
const char call_t_comma[]  = ",";
const char call_t_read[]   = "read";

/*******************************************************************/
/* Variables to log-printing                                       */
/*******************************************************************/

struct jfg_tree_desc *jfr_tree;
int jfr_maxtree = 128;

char jfr_pb[] = " (";
char jfr_pe[] = ") ";

struct jf_tmap_desc {
	int value;
	const char *text;
};

struct jf_tmap_desc jf_im_texts[] =
{ { JFT_FM_INPUT,              "i"},
  { JFT_FM_INPUT_EXPECTED,     "ie"},
  { JFT_FM_INPUT_KEY,          "it"},
  { JFT_FM_INPUT_EXPECTED_KEY, "iet"},
  { JFT_FM_EXPECTED,           "e"},
  { JFT_FM_EXPECTED_INPUT,     "ei"},
  { JFT_FM_EXPECTED_KEY,       "et"},
  { JFT_FM_EXPECTED_INPUT_KEY, "eit"},
  { JFT_FM_KEY_INPUT,          "ti"},
  { JFT_FM_KEY_EXPECTED,       "te"},
  { JFT_FM_KEY_INPUT_EXPECTED, "tie"},
  { JFT_FM_KEY_EXPECTED_INPUT, "tei"},
  { JFT_FM_FIRST_LINE,         "f"},
  { -1,                        ""}
};

struct jfr_err_desc {
	int eno;
	const char *text;
	};

struct jfr_err_desc jfr_err_texts[] = {
	{      0, " "},
	{      1, "Cannot open file:"},
	{      2, "Error reading from file:"},
	{      3, "Error writing to file:"},
	{      4, "FIle does not contain jfs-program:"},
	{      5, "This is JFR version 2. Another version is needed to run:"},
	{      6, "Cannot allocate memory to:"},
	{      9, "Illegal number:"},
	{     10, "Value out of domain-range:"},
	{     11, "Unexpected EOF."},
	{     13, "Undefined adjective:"},
	{     14, "missing start/end of interval."},
	{     15, "No value for variable:"},
	{     16, "Too many values in a record (max 255)."},
	{     17, "Illegal jft-file-mode."},
	{     18, "Token to long (max 255 chars)."},
	{     19, "Penalty-matrix and more than one output-variable."},
	{     20, "Too many penalty-values (max 64)."},
	{     21, "No values in first data-line."},
	{    201, "All values zero, cannot normalise."},
	{    202, "Variable-value out of domain range."},
	{    203, "Illegal operation."},
	{    204, "Cannot defuzificate, all fuzzy variables = 0.0"},
	{    205, "Function argument out of range."},
	{    206, "Stack overflow."},
	{    301, "JFG_LIB: Statement to long (truncated)"},
	{    302, "JFG_LIB: No free nodes in tree"},
	{    303, "JFG_LIB: Stack overflow"},
	{    601, "Too many *-inputs (max 8)."},
	{    602, "Option '-D ie' is only posible if input from file."},
	{    603, "Undefined variable:"},
	{    604, "Wrong number of arguments to call-statement."},
	{    605, "Option '-D' (without 'i') is not posible if input from file."},
	{   9999, "Unknown error!"},
};

static void ext_subst(char *d, const char *e, int forced);
static int jf_error(int errno, const char *name, int mode);
int jf_tmap_find(struct jf_tmap_desc *map, const char *txt);
int jf_getoption(const char *argv[], int no, int argc);
void jf_ftoa(char *txt, float f);
int closest_adjectiv(int var_no, float val);
static int kb_ip_get(struct jft_data_record *v, int var_no);
static int fl_ip_get(struct jft_data_record *dd, int var_no);
int jfr_getvar(struct jft_data_record *dd, int var_no);
void jf_align(char *txt, int tlen, int side);
void jpr_text(FILE *op, int var_no);
void f_print(FILE *op, float val);
void i_print(FILE *op, int i);
void t_print(FILE *op, const char *txt);
void jpr_var(FILE *op, int var_no, float val, float confidence);
void jfs_ip_write(FILE *op, int forced);
static void jfs_op_write(FILE *op, int forced);
void this_call(void);
void err_check(int mode);
void log_write(int mode);
static void jfs_stat_write(void);
static void write_header(void);
static int init(void);
int jfr_ip_first(void);
int jfr_ip_next(void);
int jfr_err_judge(void);
void jfr_opd_stat(void);
static int isoption(const char *s);
static int jf_about(void);
static int us_error(void);


static int jf_error(int eno, const char *name, int mode)
{
  int m, v, e, w;
  char ttxt[82];

  e = 0;
  for (v = 0; e == 0; v++)
  { if (jfr_err_texts[v].eno == eno
       	|| jfr_err_texts[v].eno == 9999)
      e = v;
  }
  if (mode == JFE_WARNING)
  { w = 1;
    if (eno == 202 && warn_input == 0)
      w = 0;
    if ((eno == 201 || eno == 204 || eno == 205) && warn_calc == 0)
      w = 0;
    if (eno >= 206 && eno <= 209 && warn_stack == 0)
      w = 0;
    if (w == 1)
      printf("warning %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    m = 0;
  }
  else
  { if (eno != 0)
      printf("*** error %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    if (mode == JFE_FATAL)
    { if (eno != 0)
	     { printf("\n*** PROGRAM ABORTED! ***\n");
        if (wait_return == 1)
        { printf("\nPress RETURN...");
          fgets(ttxt, 80, stdin);
        }
      }
      if (jfs_ip != NULL)
	       fclose(jfs_ip);
      if (jfs_op != NULL && jfs_op != stdout)
	       fclose(jfs_op);
      if (jfs_log != NULL)
	      fclose(jfs_log);
      if (jf_head != NULL)
	      jfr_close(jf_head);
      if (amemory != NULL)
   	    free(amemory);
      jfr_free();
      jfg_free();
      exit(1);
    }
    m = -1;
  }
  return m;
}

int jf_tmap_find(struct jf_tmap_desc *map, const char *txt)
{
  int m, res;
  res = -2;
  for (m = 0; res == -2; m++)
  { if (map[m].value == -1
       	|| strcmp(map[m].text, txt) == 0)
      res = map[m].value;
  }
  return res;
}

int jf_getoption(const char *argv[], int no, int argc)
{
  int m, v, res;

  res = -2;
  for (m = 0; res == -2; m++)
  { if (jf_options[m].argc == -2)
      res = -1;
    else
    if (strcmp(jf_options[m].option, argv[no]) == 0)
    { res = m;
      if (jf_options[m].argc > 0)
      { if (no + jf_options[m].argc >= argc)
	      res = -1; /* missing arguments */
 	    else
	    { for (v = 0; v < jf_options[m].argc; v++)
	      { if (isoption(argv[no + 1 + v]) == 1)
	          res = -1;
	      }
	    }
      }
    }
  }
  return res;
}


static void ext_subst(char *d, const char *e, int forced)
{
  int m, fundet;
  char punkt[] = ".";

  fundet = 0;
  for (m = strlen(d) - 1; m >= 0 && fundet == 0 ; m--)
  { if (d[m] == '.')
    { fundet = 1;
      if (forced == 1)
	    d[m] = '\0';
    }
  }
  if (fundet == 0 || forced == 1)
  { if (strlen(e) != 0)
      strcat(d, punkt);
    strcat(d, e);
  }
}

void jf_ftoa(char *txt, float f)
{
  char it[30] = "   ";
  char *t;
  int m, cif, mente, dp, ep, dl, sign, at;

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
  ep = dp + digits - 1;
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
    else
    if (it[m] == '.' && dc_comma == 1)
      it[m] = ',';
  }
  at = 0;
  if (sign == -1)
  { txt[0] = '-';
    at++;
  }
  for (m = 0; it[m] != '\0'; m++)
  { if (it[m] != ' ' && it[m] != '-')
    { txt[at] = it[m];
      at++;
    }
  }
  if (at == 0 || (at == 1 && txt[0] == '-'))
  { txt[0] = '0';
    at = 1;
  }
  txt[at] = '\0';
}

void jf_ftoit(char *txt, float f)
{
  int rm_digits;

  rm_digits = digits;
  digits = 0;
  jf_ftoa(txt, f);
  if (txt[strlen(txt) - 1] == '.')
    txt[strlen(txt) - 1] = '\0';
  digits = rm_digits;
}

int closest_adjectiv(int var_no, float val)
{
  int m, best_adjectiv;
  float best_dist, dist;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;

  jfg_var(&vdesc, jf_head, var_no);
  best_adjectiv = 0; best_dist = 0.0;
  for (m = 0; m < vdesc.fzvar_c; m++)
  { jfg_adjectiv(&adesc, jf_head, vdesc.f_adjectiv_no + m);
    dist = fabs(val - adesc.center);
    if (m == 0 || dist < best_dist)
    { best_dist = dist;
      best_adjectiv = m;
    }
  }
  return vdesc.f_adjectiv_no + best_adjectiv;
}


static int kb_ip_get(struct jft_data_record *v, int var_no)
{
  /* gets input for a variable from keyboard */
  char input_text[78];
  char *iptext;
  char text[80];
  char t2[80];
  int m, a, res, slut;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  struct jfg_adjectiv_desc adesc;

  slut = 0;
  jfg_var(&vdesc, jf_head, var_no);
  jfg_domain(&ddesc, jf_head, vdesc.domain_no);

  if (ip_no == 0)
    v->farg = vdesc.default_val;
  v->conf = 1.0;

  res = -1;
  while (res != 0)
  { if (ddesc.type == JFS_DT_FLOAT)
      jf_ftoa(text, v->farg);
    else
    if (ddesc.type == JFS_DT_INTEGER)
      jf_ftoit(text, v->farg);
    else
    { a = closest_adjectiv(var_no, v->farg);
      jfg_adjectiv(&adesc, jf_head, a);
      strcpy(text, adesc.name);
    }

    if (v->mode == JFT_DM_INTERVAL)
    { if (v->sarg > 0)
        sprintf(text, "*%d", v->sarg);
      else
        strcpy(text, "*");
    }
    else
    if (v->mode == JFT_DM_MMINTERVAL)
    { sprintf(text, "*%d:", v->sarg);
      jf_ftoa(t2, v->imin);
      strcat(text, t2);
      strcat(text, ":");
      jf_ftoa(t2, v->imax);
      strcat(text, t2);
    }

    printf("  %s [%s] : ", vdesc.text, text);
    fgets(input_text, 76, stdin);
    m = strlen(input_text) - 1;
    while (m >= 0 && isspace(input_text[m]))
    { input_text[m] = '\0';
      m--;
    }
    m = 0;
    while (input_text[m] != '\0' && isspace(input_text[m]))
      m++;
    iptext = &(input_text[m]);

    if (strcmp(iptext, "!") == 0)
    { slut = 1;
      res = 0;
    }
    if (res != 0)  /* hvis input tom brug default */
    { res = 0;
      for (m = 0; res == 0 && m < ((int) strlen(iptext)); m++)
      { if (!isspace(iptext[m]))
	         res = -1;
      }
    }
    if (res != 0)
    { if (strcmp(iptext, "?") != 0)
      { res = jft_atov(v, var_no, iptext);
	       if (res == 0)
	       { if (v->mode == JFT_DM_END)
	           slut = 1;
	       }
	       else
	         jf_error(jft_error_desc.error_no, iptext, JFE_ERROR);
      }
    }
    if (res != 0)
    { printf("    Legal values:");
      if (ddesc.type == JFS_DT_FLOAT)
        printf(" a floating-point number");
      if (ddesc.type == JFS_DT_INTEGER)
        printf(" an integer");
      if (ddesc.type != JFS_DT_CATEGORICAL)
      { if (strlen(ddesc.unit) > 0)
          printf(" (%s)", ddesc.unit);
        if ((ddesc.flags & JFS_DF_MINENTER) != 0)
        { jf_ftoa(text, ddesc.dmin);
	         printf(" >= %s", text);
	         if ((ddesc.flags & JFS_DF_MAXENTER) != 0)
	           printf(" and");
        }
        if ((ddesc.flags & JFS_DF_MAXENTER) != 0)
        {	jf_ftoa(text, ddesc.dmax);
  	       printf(" <= %s", text);
        }
        printf(",\n   ");
      }
      printf(" the symbol '!' (end),\n");
      printf(
      "    the symbol '*' (optionally followed by steps[:begin:end:]), or one of:\n    ");
      for (m = 0; m < vdesc.fzvar_c; m++)
      { jfg_adjectiv(&adesc, jf_head, vdesc.f_adjectiv_no + m);
	       if (m != 0)
	         printf(", ");
	       printf("%s", adesc.name);
      }
      printf(".\n");
    }
  }
  return slut;
}

static int fl_ip_get(struct jft_data_record *dd, int var_no)
{
  /* reading the value of variable number <var_no> from a file */
  int m;
  char txt[80];

  m = jft_getvar(dd, var_no);
  if (m != 0)
  { if (m != 11)
    { sprintf(txt, " %s, in file: %s dataset %s line %d.",
                   jft_error_desc.carg, ip_fname, ident_text,
                   jft_error_desc.line_no);
      jf_error(jft_error_desc.error_no, txt, JFE_ERROR);
      m = 0;
    }
  }
  return m;
}

int jfr_getvar(struct jft_data_record *dd, int var_no)
{
  int slut;

  if (ip_medie == IP_KEYBOARD)
    slut = kb_ip_get(dd, var_no);
  else
    slut = fl_ip_get(dd, var_no);
  if (slut != 0)
    slut = 1;
  return slut;
}

int jfr_ip_get()
{
  int m, slut, res;
  struct jfg_var_desc vdesc;
  struct jft_data_record dd;
  char txt[80];
  char err_txt[128];

  slut = 0;
  ident_text[0] = '\0';
  if (this_eof == 1)
    return 1;
  if (ip_medie == IP_KEYBOARD)
    printf("INPUT %d (Type ! to quit, ? for help):\n", ip_no + 1);

  if (ident_mode == ID_NONE)
    sprintf(ident_text, "%d", ip_no + 1); /* itoa(ip_no + 1, ident_text, 10); */
  else
  { if (ip_medie == IP_KEYBOARD)
    { printf("  Input identifier : ");
      fgets(ident_text, 254, stdin);
      if (strcmp(ident_text, "!") == 0)
        slut = 1;
    }
  }
  if (ident_mode == ID_NONE && undef_mode == UM_NONE && ip_medie == IP_KEYBOARD
      && (data_mode == DM_NONE || data_mode == DM_EXP))
  { txt[0] = '\0';
    printf("   More inputs [Y] ?");
    fgets(txt, 78, stdin);
    if (txt[0] == 'n' || txt[0] == 'N' || txt[0] == '!')
      slut = 1;
  }

  if (fmode == JFT_FM_FIRST_LINE)
  { for (m = 0; slut == 0 && m < jft_dset_desc.record_size; m++)
    { res = jft_getdata(&dd);
      if (res == 11)
      { slut = 1;
        if (m != 0)
          jf_error(11, jf_empty, JFE_ERROR);
      }
      if (res == 0)
      { switch (dd.vtype)
        { case JFT_VT_IGNORE:
            break;
           case JFT_VT_KEY:
             strcpy(ident_text, dd.token);
             break;
          case JFT_VT_INPUT:
            jft_dd_copy(&(ip_vars[dd.vno]), &dd);
            break;
          case JFT_VT_EXPECTED:
            jft_dd_copy(&(exp_vars[dd.vno]), &dd);
            break;
        }
      }
      if (res != 0 && res != 11)
   	  { sprintf(err_txt, "%s in line %d", jft_error_desc.carg,
                                          jft_error_desc.line_no);
        jf_error(jft_error_desc.error_no, err_txt, JFE_ERROR);
      }
    }
  }
  else /* first_line != 1 */
  { if (ident_mode == ID_START && ip_medie == IP_FILE)
    { m = jft_gettoken(ident_text);
      if (m == 11)
        slut = 1;
    }
    if (data_mode == DM_EXP || data_mode == DM_EXP_INP) /* start med expected */
    { for (m = 0; slut == 0 && m < ovar_c; m++)
      { slut = jfr_getvar(&(exp_vars[m]), spdesc.f_ovar_no + m);
        if (slut == 1 && m != 0)
          jf_error(11, jf_empty, JFE_ERROR);
      }
    }
    if (data_mode == DM_INP || data_mode == DM_INP_EXP || data_mode == DM_EXP_INP)
    { for (m = 0; slut == 0 && m < ivar_c; m++)
      { slut = jfr_getvar(&(ip_vars[m]), spdesc.f_ivar_no + m);
        if (slut == 11 && m != 0)
          jf_error(11, jf_empty, JFE_ERROR);
      }
    }
    if (data_mode == DM_INP_EXP)
    { for (m = 0; slut == 0 && m < ovar_c; m++)
      { slut = jfr_getvar(&(exp_vars[m]), spdesc.f_ovar_no + m);
        if (slut == 11 && m != 0)
          jf_error(11, jf_empty, JFE_ERROR);
      }
    }
    if (data_mode == DM_EXP || data_mode == DM_NONE)  /* No input */
    { for (m = 0; slut == 0 && m < ivar_c; m++)
      { jfg_var(&vdesc, jf_head, spdesc.f_ivar_no + m);
        ip_vars[m].farg = vdesc.default_val;
        ip_vars[m].mode = JFT_DM_NUMBER;
        ip_vars[m].conf = -1.0;
      }
    }
    if (slut == 0 && ident_mode == ID_END && ip_medie == IP_FILE)
    { m = jft_gettoken(ident_text);
      if (m == 11)
        slut = 1;
    }
  }

  v_ivar_c = 0;
  for (m = 0; slut == 0 && m < ivar_c; m++)
  { if (ip_vars[m].mode == JFT_DM_INTERVAL
       || ip_vars[m].mode == JFT_DM_MMINTERVAL)
    { if (v_ivar_c >= V_IVAR_C_MAX)
	       return jf_error(601, jf_empty, JFE_ERROR);
      v_ivars[v_ivar_c].ivar_no = m;
      v_ivars[v_ivar_c].antal = ip_vars[m].sarg;
      v_ivars[v_ivar_c].start_v = ip_vars[m].imin;
      v_ivars[v_ivar_c].addent =  (ip_vars[m].imax - ip_vars[m].imin)
                                  / ((float) v_ivars[v_ivar_c].antal - 1.0);
      v_ivar_c++;
    }
    if (ip_vars[m].mode == JFT_DM_MISSING)
    { if (undef_value == JFR_FCLR_AVG)
        confidences[m] = -2.0;
      else
      if (undef_value == JFR_FCLR_ONE)
        confidences[m] = -1.0;
      else
      if (undef_value == JFR_FCLR_ZERO)
         confidences[m] = -3.0;
      else
        confidences[m] = 0.0;
    }
    else
      confidences[m] = ip_vars[m].conf;
  }
  if (slut == 0)
    ip_no++;
  else
    this_eof = 1;
  return slut;
}

void jf_align(char *txt, int tlen, int side) /* side == 0: hoejre-stillet */
{
  int m, fr, to;

  if (side == 1)           /* append spaces */
  { m = strlen(txt);
    while (m < tlen)
    { txt[m] = ' ';
      m++;
    }
    txt[m] = '\0';
  }
  else                     /* spaces before txt */
  { fr = strlen(txt) - 1;
    to = tlen - 1;
    if (fr >= to)
      fr = to - 1;
    while (to >= 0)
    { if (fr >= 0)
	       txt[to] = txt[fr];
      else
	       txt[to] = ' ';
      to--; fr--;
    }
    txt[tlen] = '\0';
  }
}

void jpr_text(FILE *op, int var_no)
{
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  char txt[80];

  jfg_var(&vdesc, jf_head, var_no);
  jfg_domain(&ddesc, jf_head, vdesc.domain_no);
  strcpy(txt, vdesc.text);
  if (strlen(ddesc.unit) > 0)
  { strcat(txt, jfr_pb);
    strcat(txt, ddesc.unit);
    strcat(txt, jfr_pe);
  }
  jf_align(txt, jfr_t_len, 1);
  fprintf(op, "\n  %s", txt);
}

void f_print(FILE *op, float val)
{
  char txt[80];

  jf_ftoa(txt, val);
  jf_align(txt, jfr_f_len, 0);
  fprintf(op, "%s", txt);
}

void i_print(FILE *op, int i)
{
   char txt[80];
   char *it;

   sprintf(txt, "%d", i);
   it = txt;
   while (*it == ' ')
     it++;
   jf_align(it, jfr_f_len - digits, 0);
   jf_align(it, jfr_f_len, 1);
   fprintf(op, "%s", it);
}

void t_print(FILE *op, const char *txt)
{
  /* prints the txt <txt> in a float_field */
  char a[80];

  strcpy(a, txt);
  jf_align(a, jfr_f_len, 1);
  fprintf(op, "%s", a);
}

void jpr_var(FILE *op, int var_no, float val, float confidence)
{
  struct jfg_adjectiv_desc adesc;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  char txt[80];
  int ano, this_type;

  strcpy(txt, " ?");
  if (op_vformat == -1)  /* follow domains */
  { jfg_var(&vdesc, jf_head, var_no);
    jfg_domain(&ddesc, jf_head, vdesc.domain_no);
    this_type = ddesc.type;
  }
  else
    this_type = op_vformat;
  if (this_type == JFS_DT_CATEGORICAL)
  { if (confidence > 0.0)
    { ano = closest_adjectiv(var_no, val);
      jfg_adjectiv(&adesc, jf_head, ano);
      txt[0] = ' ';
      txt[1] = '\0';
      strcat(txt, adesc.name);
    }
    jf_align(txt, jfr_f_len, 1);
    fprintf(op, "%s", txt);
  }
  else
  { if (confidence > 0.0)
      f_print(op, val);
    else
    { jf_align(txt, jfr_f_len, 1);
      fprintf(op, "%s", txt);
    }
    if (confidence > 0.0 && confidence != 1.0)
    { jf_ftoa(txt, confidence);
      fprintf(op, ":%s", txt);
    }
  }
}

void jfs_ip_write(FILE *op, int forced)
{
  int m;

  if (op_text == 1 || forced == 1)
    fprintf(op, "INPUT %s, file: %s ", ident_text, ip_fname);
  for (m = 0; m < ivar_c; m++)
  { if (op_text == 1 || forced == 1)
      jpr_text(op, spdesc.f_ivar_no + m);
    jpr_var(op, spdesc.f_ivar_no + m, ivar_values[m], confidences[m]);
  }
}

static void jfs_fz_write(FILE *op)
{
  int m, v;
  float val;
  char txt[80];
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;

  if (op_text == 1)
    fprintf(op, "\n%s", fv_text);
  for (m = 0; m <ovar_c; m++)
  { jfg_var(&vdesc, jf_head, spdesc.f_ovar_no + m);
    for (v = 0; v < vdesc.fzvar_c; v++)
    { val = jfr_fzvget(vdesc.f_fzvar_no + v);
      if (op_text == 1)
      { jfg_adjectiv(&adesc, jf_head, vdesc.f_adjectiv_no + v);
        strcpy(txt, vdesc.name);
        strcat(txt, is_text);
        strcat(txt, adesc.name);
        jf_align(txt, jfr_t_len, 1);
        fprintf(op, "\n  %s", txt);
      }
      f_print(op, val);
    }
  }
  if (op_ovars == 0 && op_expected == 0)
  { fprintf(op,"\n");
    if (op_text == 1)
      fprintf(op, "\n");
  }
}

static void jfs_op_write(FILE *op, int forced)
{
  int m;
  char txt[255];

  float err, pcterr;

  if (op_text == 1 || forced == 1)
  { fprintf(op, "\n");
    strcpy(txt, em_text);
    jf_align(txt, jfr_t_len, 1);
    fprintf(op, "%s  ", txt);
    if (  (op_ovars == 1 && (op_expected || op_stat))
	       || forced == 1)
    { strcpy(txt, c_text);
      jf_align(txt, jfr_f_len, 0);
      fprintf(op, "%s", txt);
    }
    if (op_expected == 1)
    { strcpy(txt, e_text);
      jf_align(txt, jfr_f_len, 0);
      fprintf(op, "%s", txt);
    }
    if (op_stat == 1)
    { strcpy(txt, er_text);
      jf_align(txt, jfr_f_len, 0);
      fprintf(op, "%s", txt);
      strcpy(txt, pe_text);
      jf_align(txt, jfr_f_len, 0);
      fprintf(op, "%s", txt);
    }
    for (m = 0; m < ovar_c; m++)
    { jpr_text(op, spdesc.f_ovar_no + m);
      if (op_ovars == 1 || forced == 1)
	       jpr_var(op, spdesc.f_ovar_no + m, ovar_values[m], 1.0);
      if (op_expected == 1)
	       jpr_var(op, spdesc.f_ovar_no + m, exp_vars[m].farg, 1.0);
      if (op_stat == 1)
      { err = ovar_values[m] - exp_vars[m].farg;
	       f_print(op, err);
	       if (exp_vars[m].farg == 0.0)
	         pcterr = 0.0;
	       else
	         pcterr =  (err / exp_vars[m].farg) * 100.0;
	       f_print(op, pcterr);
      }
    }
    if (op_stat == 1 && ovar_c > 1)
    { fprintf(op, "  \n\nDistance : ");
      f_print(op, dset_dist);
      fprintf(op, ", adjectiv_errors: %d\n", dset_adj_err_c);
    }
    else
      fprintf(op, "\n");
  }
  else
  { if (op_ovars == 1 || forced == 1)
    { for (m = 0; m < ovar_c; m++)
	    jpr_var(op, spdesc.f_ovar_no + m, ovar_values[m], 1.0);
    }
    if (op_expected == 1)
    { for (m = 0; m < ovar_c; m++)
       jpr_var(op, spdesc.f_ovar_no + m, exp_vars[m].farg, 1.0);
    }
    if (op_key == 1)
      fprintf(op, " %s", ident_text);
  }
  fprintf(op, "\n");
}

int var_find(const char *name)
{
  int m;
  struct jfg_var_desc vdesc;

  for (m = 0; m < spdesc.var_c; m++)
  { jfg_var(&vdesc, jf_head, m);
    if (strcmp(name, vdesc.name) == 0)
      return m;
  }
  return -1;
}

void this_call(void)  /* call-statement. Only handles printf/read. */
{
  struct jfr_stat_desc dprog_info;
  struct jft_data_record dd;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;
  float f;
  int argc, cc, ac, vno, eofvno, slut, ano;
  int state;      /* 0:normal, 1:after %, 2:after \, 10:end */
  char tmp[2];
  const char *fs;

  tmp[1] = '\0';
  jfr_statement_info(&dprog_info);
  argc = jfg_a_statement(call_args, MAX_CALL_ARGS,
			             jf_head, dprog_info.pc);
  ac = 0;
  if (argc >= 1)
  { if (strcmp(call_args[ac], call_t_printf) == 0)
    { if (argc == 1)
      { printf("Missing argument to call-statement: printf\n");
	       return;
      }
      ac++;
      state = 0;
      fs = call_args[ac];
      cc = 1;
      ac++;
      while (state != 10)
      { switch (state)
        { case 0:           /* normal */
            if (fs[cc] == '%')
              state = 1;
            else
            if (fs[cc] == '"')
              state = 10;
            else
            if (fs[cc] == '\\')
              state = 2;
            else
            if (fs[cc] == '\0')
            { printf("Missing end-qoute in printf-format-string.\n");
              state = 10;
            }
            else
            { tmp[0] = fs[cc];
              fprintf(jfs_op, "%s", tmp);
            }
            break;
          case 1:         /* efter % */
            if (fs[cc] == 'f' || fs[cc] == 'a')
            { if (ac < argc)
              { if (strcmp(call_args[ac], call_t_comma) == 0)
                  ac++;
                if (ac < argc)
                { if ((vno = var_find(call_args[ac])) >= 0)
                  { f = jfr_vget(vno);
                    if (fs[cc] == 'f')
                      f_print(jfs_op, f);
                    else
                    { ano = closest_adjectiv(vno, f);
                      jfg_var(&vdesc, jf_head, vno);
                      jfg_adjectiv(&adesc, jf_head, vdesc.f_adjectiv_no + ano);
                      fprintf(jfs_op, "%s", tmp);
                    }
                    state = 0;
                  }
                  else
                  { printf("Unknown variable in printf-statement: %s\n",
                            call_args[ac]);
                    state = 10;
                  }
                }
                else
                  state = 10;
                ac++;
              }
              else
                state = 10;
            }
            else
              state = 10;
            break;
          case 2:         /* efter \   */
            if (fs[cc] == 'n')
            { fprintf(jfs_op, "\n");
              state = 0;
            }
            else
            if (fs[cc] == '\0')
              state = 10;
            else
            { tmp[0] = fs[cc];
              fprintf(jfs_op, "%s", tmp);
              state = 0;
            }
            break;
        }
	       cc++;
      } /* while */
    }
    else
    if (strcmp(call_args[0], call_t_read) == 0)
    { if (argc > 3 || argc < 2)
	       jf_error(604, jf_empty, JFE_FATAL);
      vno = var_find(call_args[1]);
      if (vno == -1)
	       jf_error(603, call_args[1], JFE_FATAL);
      jfg_var(&vdesc, jf_head, vno);
      if (this_eof == 0)
      { dd.farg = vdesc.default_val;
	       slut = jfr_getvar(&(dd), vno);
	       if (slut == 0)
	         jfr_vput(vno, dd.farg, 1.0);
	       else
	         this_eof = 1;
        if (argc == 3)
        { eofvno = var_find(call_args[2]);
          if (eofvno == -1)
            jf_error(603, call_args[2], JFE_FATAL);
          jfr_vput(eofvno, (float) slut, 1.0);
        }
      }
    }
  }
    /* other call-statements are ignored. */
  return ;
}

void err_check(int mode)
{
  int ecode;
  char sno_txt[100];
  struct jfr_stat_desc dprog_info;

  if (mode == 1)
  { ecode = jfr_error();
    if (ecode != 0)
    { jfr_statement_info(&dprog_info);
      sprintf(sno_txt, "In input: %s statement: %d", ident_text,
						     dprog_info.rule_no);
      jf_error(ecode, sno_txt, JFE_WARNING);
    }
  }
}

void undef_handle(int vno)
{
  struct jfg_var_desc vdesc;
  int ask, slut;
  struct jft_data_record dd;

  ask = 0;
  if (vno >= spdesc.f_ivar_no && vno < spdesc.f_ivar_no + spdesc.ivar_c)
  { if (undef_mode == UM_INPUT || undef_mode  == UM_INPUT_LOCAL)
      ask = 1;
  }
  else
  if (vno >= spdesc.f_lvar_no && vno < spdesc.f_lvar_no + spdesc.lvar_c)
  { if (undef_mode == UM_LOCAL || undef_mode == UM_INPUT_LOCAL)
      ask = 1;
  }
  if (ask == 1 && ip_medie == IP_KEYBOARD)
  { jfg_var(&vdesc, jf_head, vno);
    if (this_eof == 0)
    { dd.farg = vdesc.default_val;
	     slut = jfr_getvar(&(dd), vno);
	     if (slut == 0)
	       jfr_vput(vno, dd.farg, 1.0);
	     else
	      this_eof = 1;
    }
  }
}
void log_write(int mode)
{
  struct jfr_stat_desc dprog_info;
  struct jfg_statement_desc sdesc;
  int m, v;
  struct jfg_var_desc var_info;
  struct jfg_adjectiv_desc adj_info;
  struct jfg_fzvar_desc fzvar_info;
  struct jfg_array_desc adesc;
  struct jfg_function_desc fudesc;
  float f;

  if (mode == 0)
  { jfs_ip_write(jfs_log, 1);
    fprintf(jfs_log,"\nFUZZY IP-VARS\n");
    for (m = 0; m < spdesc.ivar_c; m++)
    { jfg_var(&var_info, jf_head, spdesc.f_ivar_no + m);
      for (v = 0; v < var_info.fzvar_c; v++)
      { f = jfr_fzvget(var_info.f_fzvar_no + v);
	       if (f >= 0.0001)
        { jfg_adjectiv(&adj_info, jf_head, var_info.f_adjectiv_no + v);
          fprintf(jfs_log, "  %s is %s = %6.4f\n", var_info.name,
                           adj_info.name, f);
        }
      }
    }
    fprintf(jfs_log, "RULES\n");
  }
  else
  if (mode == 1)
  { jfr_statement_info(&dprog_info);

    err_check(mode);

    jfg_statement(&sdesc, jf_head, dprog_info.pc);
    if (dprog_info.changed == 1
	       || (log_mode == LM_ALL && dprog_info.weight != 0.0))
    { if (dprog_info.function_no >= 0)
      {	jfg_function(&fudesc, jf_head, dprog_info.function_no);
	       fprintf(jfs_log, "  rule %s: %d\n", fudesc.name, dprog_info.rule_no);
      }
      else
	       fprintf(jfs_log, "  rule main: %d\n", dprog_info.rule_no);

      jfg_t_statement(jfs_txt, 512, 4,
		                    jfr_tree, jfr_maxtree, jf_head,
		                    dprog_info.function_no, dprog_info.pc);
      fprintf(jfs_log, "%s\n", jfs_txt);

      switch(sdesc.type)
      { case JFG_ST_IF:
          switch(sdesc.sec_type)
          { case JFG_SST_VAR:
              jfg_var(&var_info, jf_head, sdesc.sarg_1);
              fprintf(jfs_log, "    if %6.4f then %s = %10.4f;",
                      dprog_info.weight,
                      var_info.name, dprog_info.expr_value);
              f = jfr_vget(sdesc.sarg_1);
              fprintf(jfs_log, " %s ", var_info.name);
              fprintf(jfs_log, " = %10.4f.\n", f);
              break;
            case JFG_SST_RETURN:
              fprintf(jfs_log, "    return %10.4f;\n",
                               dprog_info.expr_value);
              break;
            case JFG_SST_INC:
              jfg_var(&var_info, jf_head, sdesc.sarg_1);
              fprintf(jfs_log, "    if %6.4f then", dprog_info.weight);
              if (sdesc.flags & 4)
                fprintf(jfs_log, " decrease ");
              else
                fprintf(jfs_log, " increase ");
              fprintf(jfs_log, " %s with %10.4f;",
                      var_info.name, dprog_info.expr_value);
              f = jfr_vget(sdesc.sarg_1);
              fprintf(jfs_log, " %s ", var_info.name);
              fprintf(jfs_log, " = %10.4f.\n", f);
              break;
            case JFG_SST_ARR:
              jfg_array(&adesc, jf_head, sdesc.sarg_1);
              fprintf(jfs_log, "    %s[%4.0f] = %6.4f.\n",
                       adesc.name, dprog_info.index_value,
                       dprog_info.expr_value);
              break;
            case JFG_SST_FZVAR:
              fprintf(jfs_log, "    if %6.4f.", dprog_info.weight);
              f = jfr_fzvget(sdesc.sarg_1);
              jfg_fzvar(&fzvar_info, jf_head, sdesc.sarg_1);
              jfg_var(&var_info, jf_head, fzvar_info.var_no);
              jfg_adjectiv(&adj_info, jf_head, fzvar_info.adjectiv_no);
              fprintf(jfs_log," %s is %s = %6.4f.\n",
                  var_info.name, adj_info.name, f);
              break;
          }
	         break;
        case JFG_ST_WHILE:
          fprintf(jfs_log, "    weight = %6.4f.\n", dprog_info.weight);
          break;
        case JFG_ST_CASE:
          fprintf(jfs_log, "    case %6.4f. weight = %6.4f.\n",
                             dprog_info.cond_value, dprog_info.weight);
          break;
        case JFG_ST_DEFAULT:
          fprintf(jfs_log, "    weight = %6.4f.\n", dprog_info.weight);
          break;
        case JFG_ST_WSET:
          fprintf(jfs_log, "    weight = %6.4f,\n", dprog_info.weight);
          break;
      }
    }
  }
  else
  if (mode == 2)
  { err_check(1);
    fprintf(jfs_log, "FUZZY OP-VARS");
    for (m = 0; m < spdesc.ovar_c; m++)
    { jfg_var(&var_info, jf_head, spdesc.f_ovar_no + m);
      for (v = 0; v < var_info.fzvar_c; v++)
      { jfg_fzvar(&fzvar_info, jf_head, var_info.f_fzvar_no + v);
   	    f = jfr_fzvget(var_info.f_fzvar_no + v);
	       if (f >= 0.0001)
	       { jfg_adjectiv(&adj_info, jf_head, var_info.f_adjectiv_no + v);
	         fprintf(jfs_log, "\n  %s is %s = %6.4f", var_info.name,
			               adj_info.name, f);
	       }
      }
    }
  }
  return ;
}

static void jfs_stat_write(void)
{
  int m, s, v;
  char txt[256];

  txt[0] = ' ';
  txt[1] = '\0';
  if (df_c > 1)
  { strcpy(txt, tt);
    jf_align(txt, jfr_f_len, 0);
    strcat(jf_file_txt, txt);
  }
  fprintf(jfs_op, "\nSTATISTICS:\n");
  v = strlen(c_t[3]) - digits;
  txt[0] = '\0';
  jf_align(txt, v, 1);
  fprintf(jfs_op, "  %s%s", txt, jf_file_txt);
  for (s = 0; s < 4; s++)
  { fprintf(jfs_op, "\n  %s", c_t[s]);
    for (m = 0; m < df_c; m++)
      i_print(jfs_op, dset_cs[m].count[s]);
  }
  txt[1] = '\0';
  jf_align(txt, strlen(t9), 1);
  fprintf(jfs_op, "\n\n  %s%s", txt, jf_file_txt);

  fprintf(jfs_op, "\n  %s", t11);
  for (m = 0; m < df_c; m++)
    f_print(jfs_op, tot_stats[m].dist_sum);

  fprintf(jfs_op, "\n  %s", t5);
  for (m = 0; m < df_c; m++)
  { if (dset_cs[m].count[1] == 0.0)
      t_print(jfs_op, "*****");
    else
      f_print(jfs_op, tot_stats[m].dist_sum / dset_cs[m].count[1]);
  }

  fprintf(jfs_op, "\n  %s", t12);
  for (m = 0; m < df_c; m++)
    f_print(jfs_op, tot_stats[m].sqr_sum);

  if (op_penalty == 1)
  { fprintf(jfs_op, "\n  %s", t13);
    for (m = 0; m < df_c; m++)
      f_print(jfs_op, tot_stats[m].pen_sum);
  }    

  fprintf(jfs_op, "\n  %s", t6);
  for (m = 0; m < df_c; m++)
    f_print(jfs_op, tot_stats[m].worst_value);

  fprintf(jfs_op, "\n  %s", t7);
  for (m = 0; m < df_c; m++)
  { i_print(jfs_op, tot_stats[m].worst_no);
  }

  fprintf(jfs_op, "\n  %s", t8);
  for (m = 0; m < df_c; m++)
  { i_print(jfs_op, tot_stats[m].adj_err_c);
  }

  fprintf(jfs_op, "\n  %s", t9);
  for (m = 0; m < df_c; m++)
  { i_print(jfs_op, tot_stats[m].adj_d_err_c);
  }

  fprintf(jfs_op, "\n  %s", t10);
  for (m = 0; m < df_c; m++)
  { if (dset_cs[m].count[1] == 0)
      t_print(jfs_op, "*****");
    else
      f_print(jfs_op, 100.0 * (1.0 - ((float) tot_stats[m].adj_d_err_c) /
				     ((float) dset_cs[m].count[1])));
  }
  fprintf(jfs_op, "\n");
}

static void write_header(void)
{
  int m;
  char txt[80];
  struct jfg_var_desc vdesc;

  fprintf(jfs_op, " ");
  if (op_ivars == 1)
  { for (m = 0; m < spdesc.ivar_c; m++)
    { jfg_var(&vdesc, jf_head, spdesc.f_ivar_no + m);
      sprintf(txt, vdesc.name);
      jf_align(txt, jfr_f_len, 1);
      fprintf(jfs_op, txt);
    }
  }
  if (op_ovars == 1)
  { for (m = 0; m < spdesc.ovar_c; m++)
    { jfg_var(&vdesc, jf_head, spdesc.f_ovar_no + m);
      sprintf(txt, vdesc.name);
      jf_align(txt, jfr_f_len, 1);
      fprintf(jfs_op, txt);
    }
  }
  if (op_expected == 1)
  { for (m = 0; m < spdesc.ovar_c; m++)
    { jfg_var(&vdesc, jf_head, spdesc.f_ovar_no + m);
      sprintf(txt, vdesc.name);
      jf_align(txt, jfr_f_len, 1);
      fprintf(jfs_op, txt);
    }
  }
  if (op_key == 1)
    fprintf(jfs_op, " indent");
  fprintf(jfs_op, "\n");
}

static int init(void)
{
  int m, v, s, le;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  struct jfg_adjectiv_desc adesc;
  unsigned short ms;
  char *a;
  char txt[80];
  time_t t;

  m = jfr_load(&jf_head, so_fname);
  if (m != 0)
    return jf_error(m, so_fname, JFE_FATAL);
  jfr_init(stack_size);
  jfg_sprg(&spdesc, jf_head);
  ivar_c = spdesc.ivar_c;
  ovar_c = spdesc.ovar_c;

  jft_init(jf_head);
  for (m = 0; m < ((int) strlen(field_sep)); m++)
    jft_char_type(field_sep[m], JFT_T_SPACE);

  if (fmode == JFT_FM_FIRST_LINE)
  { m = jft_fopen(ip_fname, fmode, 0);
    if (m != 0)
	     jf_error(jft_error_desc.error_no, ip_fname, JFE_ERROR);
    if (jft_dset_desc.key == 1)
      ident_mode = ID_START;
    else
      ident_mode = ID_NONE;
    if (jft_dset_desc.input == 1)
    { if (jft_dset_desc.expected == 1)
        data_mode = DM_INP_EXP;
      else
        data_mode = DM_INP;
    }
    else
    if (jft_dset_desc.expected == 1)
      data_mode = DM_EXP;
    jft_close();
  }

  if (ip_medie == IP_KEYBOARD && data_mode != DM_INP && data_mode != DM_NONE)
    return jf_error(605, jf_empty, JFE_FATAL);

  if (op_stat == -1)
  { op_stat = 0;
    op_ovars = 1;
    op_text = 1;
    if (ip_medie != IP_KEYBOARD)
    { op_ivars = 1;
      if (data_mode == DM_INP_EXP || data_mode == DM_EXP_INP)
      { op_stat = 1;
	       op_expected = 1;
      }
    }
  }
  if (op_text == 1 || op_fuzzy == 1)
    op_header = 0;

  if (data_mode == DM_INP || data_mode == DM_EXP || data_mode == DM_NONE)
  { d_filt_mode = a_filt_mode = 0;
    op_stat = op_expected = 0;
  }

  if (ip_medie == IP_KEYBOARD)
    wait_return = 0;


  if (strlen(op_fname) != 0)
  { if (op_append == 0)
      jfs_op = fopen(op_fname, "w");
    else
      jfs_op = fopen(op_fname, "a");
    if (jfs_op == NULL)
      jf_error(1, op_fname, JFE_FATAL);
  }
  else
  { jfs_op = stdout;
  }

  { if (strlen(lo_fname) != 0)
    { if ((jfs_log = fopen(lo_fname, "w")) == NULL)
	       jf_error(1, lo_fname, JFE_FATAL);
      if (log_mode == -1)
        log_mode = LM_CHANGED;
    }
    else
      jfs_log = stdout;
  }
  if (log_mode == -1)
    log_mode = LM_NONE;


  if (ivar_c == 0)
    data_mode = DM_NONE;

  if (op_penalty == 1)
  { m = jft_penalty_read(pm_fname);
    if (m == -1)
      return jf_error(jft_error_desc.error_no, jft_error_desc.carg, JFE_FATAL);
  }

  file_no = 0;
  if (dfile_c == 1)
    df_c = 1;
  else
    df_c = dfile_c + 1;

  ms = sizeof(float) * (ivar_c + ovar_c + ivar_c);
  ms += sizeof(struct jft_data_record) * (ivar_c + ovar_c);
  ms += sizeof(struct dset_c_desc) * (dfile_c + 1);
  ms += sizeof(struct tot_stat_desc) * (dfile_c + 1);

  amemory = (char *) malloc(ms);
  if (amemory == NULL)
    return jf_error(6, jf_empty, JFE_FATAL);
  a = amemory;
  ivar_values = (float *) a;
  a += sizeof(float) * ivar_c;
  ovar_values = (float *) a;
  a += sizeof(float) * ovar_c;
  confidences = (float *) a;
  a += sizeof(float) * ivar_c;
  ip_vars = (struct jft_data_record *) a;
  a += sizeof(struct jft_data_record) * ivar_c;
  exp_vars = (struct jft_data_record *) a;
  a += sizeof(struct jft_data_record) * ovar_c;
  dset_cs = (struct dset_c_desc *) a;
  a += sizeof(struct dset_c_desc) * (dfile_c + 1);
  tot_stats = (struct tot_stat_desc *) a;

  for (m = 0; m < ivar_c; m++)
    confidences[m] = 1.0;
  for (m = 0; m < dfile_c + 1; m++)
  { for (s = 0; s < 4; s++)
      dset_cs[m].count[s] = 0;
    tot_stats[m].dist_sum = tot_stats[m].worst_value = 0.0;
    tot_stats[m].sqr_sum = tot_stats[m].pen_sum = 0.0;
    tot_stats[m].adj_err_c = tot_stats[m].adj_d_err_c = 0;
    tot_stats[m].worst_no = 0;
  }

  if (log_mode > LM_NONE)
  { fprintf(jfs_log, "LOGFILE from JFR version 2.00\n\n");
    fprintf(jfs_log, "PROGRAM %s %s\n\n", so_fname, spdesc.title);
  }

  jfr_t_len = 10;

  if (op_text == 1 || op_stat == 1 || log_mode > LM_NONE)
    jfr_f_len = 10;
  else
    jfr_f_len = 1;
  for (m = 0; m < spdesc.var_c; m++)
  { if (   (m >= spdesc.f_ivar_no
	           && m < spdesc.f_ivar_no + spdesc.ivar_c)
	       || (m >= spdesc.f_ovar_no
	           && m < spdesc.f_ovar_no + spdesc.ovar_c))
    { jfg_var(&vdesc, jf_head, m);
      jfg_domain(&ddesc, jf_head, vdesc.domain_no);
      le = strlen(vdesc.text) + strlen(ddesc.unit) + 4;
      if (le > jfr_t_len)
	       jfr_t_len = le;
      if (op_header == 1)
      { if (((int) strlen(vdesc.name)) > jfr_f_len)
          jfr_f_len = strlen(vdesc.name);
      }
      if (op_vformat == JFS_DT_CATEGORICAL
	         || (op_vformat == -1 && ddesc.type == JFS_DT_CATEGORICAL))
      { for (v = 0; v < vdesc.fzvar_c; v++)
	       { jfg_adjectiv(&adesc, jf_head, vdesc.f_adjectiv_no + v);
	         if (((int) strlen(adesc.name)) > jfr_f_len)
	           jfr_f_len = strlen(adesc.name);
	       }
      }
      else
      { if ((ddesc.flags & JFS_DF_MINENTER) != 0)
	       { jf_ftoa(txt, ddesc.dmin);
	         if (((int) strlen(txt)) > jfr_f_len)
	           jfr_f_len = strlen(txt);
	       }
	       if ((ddesc.flags & JFS_DF_MAXENTER) != 0)
	       { jf_ftoa(txt, ddesc.dmax);
	         if (((int) strlen(txt)) > jfr_f_len)
	           jfr_f_len = strlen(txt);
	       }
      }
      if (op_fuzzy == 1 && m >= spdesc.f_ovar_no
	         && m <= spdesc.f_ovar_no + spdesc.ovar_c)
      { for (v = 0; v < vdesc.fzvar_c; v++)
	       { jfg_adjectiv(&adesc, jf_head, vdesc.f_adjectiv_no + v);
	         le = strlen(vdesc.name) + strlen(adesc.name) + strlen(is_text)
	              + 4;
	         if (le > jfr_t_len)
	           jfr_t_len = le;
	       }
      }
    }
  }
  jfr_f_len++;
  jf_file_txt[0] = '\0';

  m = jfr_maxtree * sizeof(struct jfg_tree_desc);
  jfr_tree = (struct jfg_tree_desc *) malloc(m);
  if (jfr_tree == NULL)
    return jf_error(6, jf_empty, JFE_FATAL);

  srand((unsigned) time(&t));   /* randomize(); */

  if (op_header == 1)
    write_header();

  return 0;
}

int jfr_ip_first(void)
{
  int m;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;

  for (m = 0; m < ivar_c; m++)
    ivar_values[m] = ip_vars[m].farg;
  for (m = 0; m < v_ivar_c; m++)
  { v_ivars[m].akt = 0;
    if (v_ivars[m].antal >= 0)
      ivar_values[v_ivars[m].ivar_no] = v_ivars[m].start_v;
    else
    { jfg_var(&vdesc, jf_head, v_ivars[m].ivar_no + spdesc.f_ivar_no);
      if (vdesc.fzvar_c > 0)
      { jfg_adjectiv(&adesc, jf_head, vdesc.f_adjectiv_no);
        ivar_values[v_ivars[m].ivar_no] = adesc.center;
      }
    }
  }
  return 0;
}

int jfr_ip_next(void)
{
  int a, blank_linie;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;

  blank_linie = 0;
  for (a = v_ivar_c - 1; a >= 0; a--)
  { v_ivars[a].akt++;
    if (v_ivars[a].antal >= 0)
    { if (v_ivars[a].akt >= v_ivars[a].antal)
      { v_ivars[a].akt = 0;
        ivar_values[v_ivars[a].ivar_no] = v_ivars[a].start_v;
        if (blank_linie == 0)
        { blank_linie = 1;
	         fprintf(jfs_op, "\n");
        }
      }
      else
      { ivar_values[v_ivars[a].ivar_no]
	         = v_ivars[a].start_v + v_ivars[a].addent * v_ivars[a].akt;
        return 0;
      }
    }
    else
    { jfg_var(&vdesc, jf_head, v_ivars[a].ivar_no + spdesc.f_ivar_no);
      if (v_ivars[a].akt >= vdesc.fzvar_c)
      { v_ivars[a].akt = 0;
        jfg_adjectiv(&adesc, jf_head, vdesc.f_adjectiv_no);
        ivar_values[v_ivars[a].ivar_no] = adesc.center;
        if (blank_linie == 0)
        { blank_linie = 1;
	         fprintf(jfs_op, "\n");
        }
      }
      else
      { jfg_adjectiv(&adesc, jf_head, vdesc.f_adjectiv_no + v_ivars[a].akt);
        ivar_values[v_ivars[a].ivar_no] = adesc.center;
        return 0;
      }
    }
  }
  return 1;
}

int jfr_err_judge(void)   /* return 0: ingen fejl, 1: fejl */
{
  int m, res;
  float f, dist;

  res = 0;
  if (d_filt_mode == 1)
  { dist = 0.0;
    for (m = 0; m < ovar_c; m++)
    { f = ovar_values[m] - exp_vars[m].farg;
      dist += f * f;
    }
    dist = pow(dist, 0.5);
    if (dist > d_filter)
      res = 1;
  }
  if (res == 0 && a_filt_mode == 1)
  { for (m = 0; res == 0 && m < ovar_c; m++)
    { if (closest_adjectiv(m + spdesc.f_ovar_no, ovar_values[m]) !=
	       closest_adjectiv(m + spdesc.f_ovar_no, exp_vars[m].farg))
     	res = 1;
    }
  }
  return res;
}


void jfr_opd_stat(void)
{
  int missing, err, m;
  float f, sqr_dist, penalty;

  dset_dist = 0.0;
  dset_adj_err_c = 0;
  missing = err = 0;
  for (m = 0; m < ivar_c; m++)
  { if (ip_vars[m].mode == JFT_DM_MISSING)
      missing = 1;
    if (ip_vars[m].mode == JFT_DM_ERROR)
      err = 1;
  }
  for (m = 0; m < ovar_c; m++)
  { if (exp_vars[m].mode == JFT_DM_MISSING)
      missing = 1;
    if (exp_vars[m].mode == JFT_DM_ERROR)
      err = 1;
    f = ovar_values[m] - exp_vars[m].farg;
    if (f < 0.0)
      f = -f;
    dset_dist += f * f;
    if (closest_adjectiv(spdesc.f_ovar_no + m, ovar_values[m]) !=
	     closest_adjectiv(spdesc.f_ovar_no + m, exp_vars[m].farg))
                       dset_adj_err_c++;
    if (op_penalty == 1)
      penalty
        = jft_penalty_calc(ovar_values[0], exp_vars[0].farg);
  }
  sqr_dist = dset_dist;
  dset_dist = pow(dset_dist, 0.5);

  dset_cs[file_no].count[1]++;
  dset_cs[dfile_c].count[1]++;
  if (err != 0)
  { dset_cs[file_no].count[2]++;
    dset_cs[dfile_c].count[2]++;
  }
  if (missing != 0)
  { dset_cs[file_no].count[3]++;
    dset_cs[dfile_c].count[3]++;
  }

  tot_stats[file_no].dist_sum += dset_dist;
  tot_stats[dfile_c].dist_sum += dset_dist;
  tot_stats[file_no].sqr_sum += sqr_dist;
  tot_stats[dfile_c].sqr_sum += sqr_dist;
  if (op_penalty == 1)
  { tot_stats[file_no].pen_sum += penalty;
    tot_stats[dfile_c].pen_sum += penalty;
  }
  tot_stats[file_no].adj_err_c += dset_adj_err_c;
  tot_stats[dfile_c].adj_err_c += dset_adj_err_c;
  if (dset_adj_err_c > 0)
  { tot_stats[file_no].adj_d_err_c++;
    tot_stats[dfile_c].adj_d_err_c++;
  }
  if (dset_dist > tot_stats[file_no].worst_value)
  { tot_stats[file_no].worst_value = dset_dist;
    tot_stats[file_no].worst_no = ip_no;
  }
  if (dset_dist > tot_stats[dfile_c].worst_value)
  { tot_stats[dfile_c].worst_value = dset_dist;
    tot_stats[dfile_c].worst_no = ip_no;
    worst_file = file_no;
  }
}

static int isoption(const char *s)
{
  if (s[0] == '-' || s[0] == '?')
    return 1;
  return 0;
}

static int jf_about(void)
{
  printf(
"\nJFR    version 2.03    Copyright (c) 1998-2000 Jan E. Mortensen\n\n");
  printf("usage: jfr [options] jfrf\n\n");

  printf("JFR executes the compiled jfs-program <jfrf>. Options:\n\n");
  /* printf("OPTIONS\n"); */
  printf(
"-p <dec>    : <dec> is precision.       -f <fs>    : <fs> field seperator.\n");
  printf(
"-d {<df>}   : Input from file(s) <df>.  -o [<of>]  : Ouput to file <of>.\n");
  printf(
"-a          : append output.            -w         : Wait for return.\n");
  printf(
"-c          : Use comma as dec-sep.     -rs ss     : <ss> Stacksize.\n");
  printf(
"-l [<lf>]   : write log-info to <lf>.   -pm [<-p>] : read penalty from <p>.\n");
  printf("-m <md>     : Output only if distance(calc, expect) >= <md>.\n");
  printf("-e          : Output only if adjective-error(calc, expect).\n");
  printf(
"-wa <wm>    : Warnings about <wm>={[v][c][s]} (v: var. out of range,\n");
  printf(
"              c: illegal function-arg, s: stack error).\n");
  printf(
"-D <dm>     : Datamode={[t][i][e]}|f. i:input, e:expect, t:text, f:firstline.\n");
  printf(
"-u <um>     : Ask for undef. vars. <um>=[i][l]. i:input, l:local.\n");
  printf(
"-uv <m>     : Fzvars for undefined. <m>=z:0.0, <m>=o:1.0, <m>=a:1/count.\n");

  printf(
"-lm <lm>    : Log-mode. <lm>=d:debug, <lm>=s:standard, <lm>=f:full.\n");
  printf(
"-O [<opm>]  : <opm>= {[i][o][e][u][t][s][f][a][k][h]}. Write i:input values,\n");
  printf(
"              o:output values, e:expected values, u:fuzzy output values,\n");
  printf(
"              t:texts, k:ident, h:header, s:statistic, f:as float, a:as adj.\n");
  return 0;
}

static int us_error(void)         /* usage-error. Fejl i kald af jfs */
{
  char ttxt[82];

  printf("\n%s\n%s\n%s\n", usage[0], usage[1], usage[2]);
  if (wait_return == 1)
  { printf("\nPress RETURN...");
    fgets(ttxt, 80, stdin);
  }
  return 1;
}

int main(int argc, const char *argv[])
{
  int m, v, s, err, slut, option_no;
  char ttxt[256];


  field_sep[0]  = '\0';
  dfile_c = 0;
  if (argc == 1)
    return jf_about();
  strcpy(so_fname, argv[argc - 1]);
  ext_subst(so_fname, extensions[0], 0);
  for (m = 1; m < argc - 1; )
  { option_no = jf_getoption(argv, m, argc - 1);
    if (option_no == -1)
      return us_error();
    m++;
    switch (option_no)
    { case 0:              /* -f  */
        strcpy(field_sep, argv[m]);
        m++;
        break;
      case 1:              /* -p */
        digits = atoi(argv[m]) + 1;
        m++;
        break;
      case 2:              /* -m */
        d_filt_mode = 1;
        if (jft_atof(&d_filter, argv[m]) != 0)
        { printf("\n Illegal number\n");
          return us_error();
        }
        m++;
        break;
      case 3:              /* -e */
        a_filt_mode = 1;
        break;
      case 4:              /* -wa */
        warn_input = warn_calc = warn_stack = 0;
        if (m < argc - 1 && isoption(argv[m]) == 0)
        { for (v = 0; v < ((int) strlen(argv[m])); v++)
          { switch (argv[m][v])
            { case 'v':
                warn_input = 1;
                break;
              case 'c':
                warn_calc = 1;
                break;
              case 's':
                warn_stack = 1;
                break;
              default:
                printf("\n Illegal warning-mode\n");
                return us_error();
                /* break; */
            }
          }
          m++;
        }
        break;
      case 5:              /* -c */
   	    dc_comma = 1;
	       break;
      case 6:              /* -D */
        data_mode = JFT_FM_NONE;
        if (m < argc - 1 && isoption(argv[m]) == 0)
        { fmode = jf_tmap_find(jf_im_texts, argv[m]);
          if (fmode == -1)
            return us_error();
          ident_mode = ID_NONE;
          data_mode = DM_NONE;
          if (fmode == JFT_FM_KEY_INPUT || fmode == JFT_FM_KEY_INPUT_EXPECTED
              || fmode == JFT_FM_KEY_EXPECTED_INPUT)
             ident_mode = ID_START;
          if (fmode ==  JFT_FM_INPUT_KEY || fmode == JFT_FM_INPUT_EXPECTED_KEY
              || fmode == JFT_FM_EXPECTED_KEY
              || fmode == JFT_FM_EXPECTED_INPUT_KEY)
             ident_mode = ID_END;
          if (fmode == JFT_FM_INPUT || fmode == JFT_FM_KEY_INPUT
              || fmode == JFT_FM_INPUT_KEY)
            data_mode = DM_INP;
          if (fmode == JFT_FM_EXPECTED || fmode == JFT_FM_KEY_EXPECTED
              || fmode == JFT_FM_EXPECTED_KEY)
            data_mode = DM_EXP;
          if (fmode == JFT_FM_INPUT_EXPECTED
              || fmode == JFT_FM_KEY_INPUT_EXPECTED
              || fmode == JFT_FM_INPUT_EXPECTED_KEY)
            data_mode = DM_INP_EXP;
          if (fmode == JFT_FM_EXPECTED_INPUT
              || fmode == JFT_FM_KEY_EXPECTED_INPUT
              || fmode == JFT_FM_EXPECTED_INPUT_KEY)
            data_mode = DM_EXP_INP;
          if (fmode == JFT_FM_FIRST_LINE)
            data_mode = DM_INP_EXP; /* rettes senere */
          m++;
        }
	       break;
      case 7:              /* -O */
        op_stat = 0;
        if (m < argc - 1 && isoption(argv[m]) == 0)
        { for (v = 0; v < ((int) strlen(argv[m])); v++)
          { switch (argv[m][v])
            { case 's':
                op_stat = 1;
                break;
              case 'i':
                op_ivars = 1;
                break;
              case 'o':
                op_ovars = 1;
                break;
              case 'e':
                op_expected = 1;
                break;
              case 't':
                op_text = 1;
                break;
              case 'a':
                op_vformat = JFS_DT_CATEGORICAL;
                break;
              case 'f':
                op_vformat = JFS_DT_FLOAT;
                break;
              case 'u':
                op_fuzzy = 1;
                break;
              case 'k':
                op_key = 1;
                break;
              case 'h':
                op_header = 1;
                break;
              default:
                printf("\n Illegal output-mode\n");
                return us_error();
                /* break; */
            }
          }
          if (op_text == 1)
            op_key = 0;
          m++;
        }
        break;
      case 8:            /* -d */
	       ip_medie = IP_FILE;
        if (m  < argc - 1 && isoption(argv[m]) == 0)
        { while (m < argc - 1 && isoption(argv[m]) == 0)
          { dfile_nos[dfile_c] = m;
            dfile_c++;
            m++;
          }
        }
        else
        { dfile_nos[0] = -1;  /* tag filename fra source-filename */
          dfile_c = 1;
        }
        break;
      case 9:            /* -o */
        if (m < argc - 1 && isoption(argv[m]) == 0)
        { strcpy(op_fname, argv[m]);
          ext_subst(op_fname, extensions[3], 0);
          m++;
        }
        else
        { strcpy(op_fname, so_fname);
          ext_subst(op_fname, extensions[3], 1);
        }
        break;
      case 10:           /* -l */
        if (m < argc - 1 && isoption(argv[m]) == 0)
        { strcpy(lo_fname, argv[m]);
          ext_subst(lo_fname, extensions[1], 0);
          m++;
        }
        else
        { strcpy(lo_fname, so_fname);
          ext_subst(lo_fname, extensions[1], 1);
        }
        break;
      case 11:           /* -lm */
        if (strcmp(argv[m], "n") == 0)
          log_mode = LM_NONE;
        else
        if (strcmp(argv[m], "d") == 0)
          log_mode = LM_DEBUG;
        else
        if (strcmp(argv[m], "s") == 0)
            log_mode = LM_CHANGED;
        else
        if (strcmp(argv[m], "f") == 0)
            log_mode = LM_ALL;
        else
          return us_error();
        m++;
        break;
      case 12:           /* -a */
        op_append = 1;
        break;
      case 13:          /* -w */
        wait_return = 1;
        break;
      case 14:          /* -rs */
        stack_size = atoi(argv[m]);
        m++;
        if (stack_size <= 0)
          return us_error();
        break;
      case 15:          /* -u */
        if (m < argc - 1 && isoption(argv[m]) == 0)
        { if (strcmp(argv[m], "i") == 0)
            undef_mode = UM_INPUT;
          else
          if (strcmp(argv[m], "l") == 0)
            undef_mode = UM_LOCAL;
          else
          if (strcmp(argv[m], "il") == 0
              || strcmp(argv[m], "li") == 0)
            undef_mode = UM_INPUT_LOCAL;
          else
            return us_error();
          m++;
        }
        else
          undef_mode = UM_NONE;
        break;
      case 16:          /* -uv */
        if (strcmp(argv[m], "z") == 0)
          undef_value = JFR_FCLR_ZERO;
        else
        if (strcmp(argv[m], "o") == 0)
          undef_value = JFR_FCLR_ONE;
        else
        if (strcmp(argv[m], "a") == 0)
          undef_value = JFR_FCLR_AVG;
        else
        if (strcmp(argv[m], "d") == 0)
          undef_value = JFR_FCLR_DEFAULT;
        else
          return us_error();
        m++;
        break;
      case 17:          /* -pm */
        if (m < argc - 1 && isoption(argv[m]) == 0)
        { strcpy(pm_fname, argv[m]);
          ext_subst(pm_fname, extensions[4], 0);
          m++;
        }
        else
        { strcpy(pm_fname, so_fname);
          ext_subst(pm_fname, extensions[4], 1);
        }
        op_penalty = 1;
        break;
      case 18:          /* -?  */
      case 19:          /* ? */
     	  return jf_about();
      default:
	         return us_error();
    }    /* not option */
  }  /* for  */

  if (jfg_init(JFG_PM_NORMAL, 64, 4) != 0)
  { printf("Could not allocate memory to jfg-stack.\n");
    return 1;
  }
  if (ip_medie == IP_KEYBOARD)
  { if (fmode == JFT_FM_FIRST_LINE)
      fmode = JFT_FM_INPUT;
    dfile_c = 1;
  }
  else
  { /* saet ip_fname til foerste fil, bruges i init hvis first_line */
    if (dfile_nos[0] >= 0)
    { strcpy(ip_fname, argv[dfile_nos[0]]);
      ext_subst(ip_fname, extensions[2], 0);
    }
    else
    { strcpy(ip_fname, so_fname);
      ext_subst(ip_fname, extensions[2], 1);
    }
  }

  init();

  for (file_no = 0; file_no < dfile_c; file_no++)
  { slut = 0;
    this_eof = 0;
    ip_no = 0;
    if (ip_medie == IP_FILE)
    { if (dfile_nos[file_no] >= 0)
      { strcpy(ip_fname, argv[dfile_nos[file_no]]);
        ext_subst(ip_fname, extensions[2], 0);
      }
      else
      { strcpy(ip_fname, so_fname);
        ext_subst(ip_fname, extensions[2], 1);
      }
      slut = jft_fopen(ip_fname, fmode, 0);
      if (slut != 0)
	       jf_error(jft_error_desc.error_no, ip_fname, JFE_ERROR);
      if (fmode == JFT_FM_FIRST_LINE)
      { if (jft_dset_desc.key == 1)
          ident_mode = ID_START;
        else
          ident_mode = ID_NONE;
        if (jft_dset_desc.input == 1)
        { if (jft_dset_desc.expected == 1)
            data_mode = DM_INP_EXP;
          else
            data_mode = DM_INP;
        }
        else
        if (jft_dset_desc.expected == 1)
          data_mode = DM_EXP;
      }
      m = strlen(ip_fname) - 1;
      s = 0;
      while (m >= 0 && s == 0)
      { if (ip_fname[m] == '\\' || ip_fname[m] == '/')
	         s = m;
	       m--;
      }
      for (m = 0; ip_fname[s] != '\0' && ip_fname[s] != '.'; m++, s++)
	       ttxt[m] = ip_fname[s];
      ttxt[m] = '\0';
      jf_align(ttxt, jfr_f_len, 0);
      strcat(jf_file_txt, ttxt);
    }
    if (slut == 0)
    { while (jfr_ip_get() == 0)
      { slut = jfr_ip_first();
        while (slut == 0)
        { err = 1;
          if (log_mode <= LM_NONE)
          { jfr_arun(ovar_values, jf_head, ivar_values, confidences,
                     this_call, err_check, undef_handle);
            err_check(1);
          }
          else
          { if (d_filt_mode == 1 || a_filt_mode == 1)
              jfr_arun(ovar_values, jf_head, ivar_values, confidences,
                       this_call, NULL, undef_handle);
            else
              jfr_arun(ovar_values, jf_head, ivar_values, confidences,
                       this_call, log_write, undef_handle);
          }
          if (d_filt_mode == 1 || a_filt_mode == 1)
            err = jfr_err_judge();
          if (err == 1 && log_mode >= LM_DEBUG && (d_filt_mode || a_filt_mode))
            jfr_arun(ovar_values, jf_head, ivar_values, confidences,
                     this_call, log_write, undef_handle);
          if (err == 1)
          { if (op_stat == 1)
              jfr_opd_stat();
            if (op_ivars == 1)
              jfs_ip_write(jfs_op, 0);
            if (op_fuzzy == 1)
              jfs_fz_write(jfs_op);
            if (op_ovars == 1 || op_expected == 1)
              jfs_op_write(jfs_op, 0);
            if (log_mode > LM_NONE)
              jfs_op_write(jfs_log, 1);
          }
          dset_cs[file_no].count[0]++;
          dset_cs[dfile_c].count[0]++;
          slut = jfr_ip_next();
        }
      }
    }
  }

  if (op_stat == 1)
    jfs_stat_write();

  if (wait_return == 1)
  { printf("\nPress RETURN...");
    fgets(ttxt, 80, stdin);
  }

  return jf_error(0, jf_empty, JFE_FATAL); /* no error */
}



