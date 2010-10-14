  /*************************************************************************/
  /*                                                                       */
  /* jfrplt.c - Run a jfs-program with output to gnuplot-files             */
  /*                             Copyright (c) 1998-2000 Jan E. Mortensen  */
  /*                                       Copyright (c) 2010 Miriam Ruiz  */
  /*                                                                       */
  /*************************************************************************/

#define JFE_WARNING 0
#define JFE_ERROR   1
#define JFE_FATAL   2

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "cmds_lib.h"
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jft_lib.h"
#include "jfs_cons.h"
#include "jopt_lib.h"

static const char usage[] =
	"jfrplt [-f fs] [-p dec] [-d df] [-o of] [-i if] [-Ax a] [-Az a]"
	" [-As] [-w [-wm]] [-a] [-t <t> <e> [<d>]] [-v vn] [-rs ss] <file.jfr>";

static const char *about[] = {
  "usage: jfrplt [options] <file.jfr>",
  "",
  "JFRPLT executes the compiled jfs-program <jfrf> and writes output to Gnuplot files.",
  "",
  "Options:",
  "-p <dec>    : <dec> is precision.       -f <fs>    : <fs> field separator.",
  "-d <df>     : input from file <df>.     -rs <ss>   : <ss> stacksize.",
  "-Ax <a>     : rotation angle x-axis     -Az <a>    : rotation angle z-axis.",
  "-a          : append output.",
  "-v <ov>     : Plot only output variable <ov>.",
  "-As         : Autoscale output axis.",
  "-w <m>      : <m>='y':wait for RETURN, 'n':don't wait, 'e':wait if error.",
  "-o <of>     : write output to the Gnuplot file <of>.plt, and the data files",
  "              <of><vname>.dat (<vname> is the name of output variables).",
  "-i <if>     : read gnuplot initialization from the file <if>.",
  "-t <t> <e> [<d>]: Insert a 'set term <t>'-statement in gnuplot file. The plots",
  "          are written to files in the directory <d> with extension <e>.",
  NULL
};

struct jopt_desc options[] =
{  /* txt, value, min, max */
   {"-f",   0,     1,    1},
   {"-p",   1,     1,    1},
   {"-v",   2,     1,    1},
   {"-t",   3,     2,    3},
   {"-d",   4,     1,    1},
   {"-o",   5,     1,    1},
   {"-a",   6,     0,    0},
   {"-w",   7,     0,    1},
   {"-i",   8,     1,    1},
   {"-rs",  9,     1,    1},
   {"-Ax", 10,     1,    1},
   {"-Az", 11,     1,    1},
   {"-As", 12,     0,    0},
   {"-?",  13,     0,    0},
   {"?",   14,     0,    0},
};
#define OPT_COUNT 15

const char *extensions[]  = {
                        "jfr",     /* 0 */
                        "dat",     /* 1 */
                        "plt",     /* 2 */
                        "gif"      /* 3 */
};

FILE *jfs_ip   = NULL;
FILE *jfs_op_d = NULL;
FILE *jfs_op_p = NULL;
FILE *jfs_ps   = NULL;

void *jf_head = NULL;

struct jfg_sprog_desc spdesc;

/*****************************************************************/
/* Variables to input, output and expected values.               */
/*****************************************************************/

static char so_fname[256] = "";    /* name of compiled jfs-program */
static char ip_fname[256] = "";    /* input-data file              */
static char ps_fname[256] = "";    /* plot-start-file              */
static char op_d_fname[256] = "";
static char op_p_fname[256] = "";

static struct jft_data_record ip_vars[256];

static int ivar_c;
static int ovar_c;

static float ivar_values[256];
static float ovar_values[256];

#define V_IVAR_C_MAX 8
struct v_ivar_desc      /* input variable angivet med '*' */
		   { float start_v;
       float end_v;
		     float addent;
		     int   ivar_no;
		     int   antal;     /* -1: step centers */
		     int   akt;
		   };

struct v_ivar_desc v_ivars[V_IVAR_C_MAX];
int v_ivar_c;
int multivar_no;

char ident_text[256] = "";

/*****************************************************************/
/* Option vars                                                   */
/*****************************************************************/

#define IP_KEYBOARD  0
#define IP_FILE      1
int ip_medie  = IP_KEYBOARD;

int digits = 4;        /* no of digits after the decimal point (incl point).*/

char field_sep[255];   /* 0: brug space, tab etc som felt-seperator, */
                       /* andet: kun field_sep er feltsepator. */

int stack_size   = 0;

#define WM_NO     0
#define WM_ERROR  1
#define WM_YES    2

static int wait_return  = WM_NO;

static int op_append = 0;

static float rot_x = 60;
static float rot_z = 300;

static int plt_2_gif = 0;
static int auto_scale = 0;

static char ov_name[64] = "";
static char op_dir[256] = "";
static char op_extension[64] = "";
static char term_name[256] = "";

/*******************************************************************/
/* Variables to output-formating                                   */
/*******************************************************************/

static char jf_empty[] = " ";

struct jfr_err_desc {
	int eno;
	const char *text;
	};

struct jfr_err_desc jfr_err_texts[] =
{ {      0, " "},
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
  {     51, "Illegal option:"},
  {     52, "Too few arguments to option:"},
  {     53, "Too many arguments to option:"},
  {    301, "JFG_LIB: Statement to long (truncated)"},
  {    302, "JFG_LIB: No free nodes in tree"},
  {    303, "JFG_LIB: Stack overflow"},
  {    601, "Too many *-inputs (max 8)."},
  {   1300, "No *-inputs or too many *-inputs (max 2)."},
  {   1301, "Unknown output-variable:"},
  {   1302, "Too many '*m'-inputs (max 1)."},
  {   9999, "Unknown error!"},
};

static int jf_error(int errno, const char *name, int mode);
int closest_adjectiv(int var_no, float val);
static int kb_ip_get(struct jft_data_record *v, int var_no);
static int fl_ip_get(struct jft_data_record *dd, int var_no);
int getvar(struct jft_data_record *dd, int var_no);
int ip_get(void);
int var_find(char *name);
static void data_write(FILE *op, int ovar_no);
static int write_plt_start(void);
static int write_plt_data(int ov_no);
static int init(void);
int ip_first(void);
int ip_next(void);

static int jf_error(int eno, const char *name, int mode)
{
  int res, v, e;
  char ttxt[82];

  e = 0;
  if (eno == 0)
    res = 0;
  else
    res = 1;
  for (v = 0; e == 0; v++)
  { if (jfr_err_texts[v].eno == eno
       	|| jfr_err_texts[v].eno == 9999)
      e = v;
  }
  if (eno != 0)
    printf("*** error %d: %s %s\n", eno, jfr_err_texts[e].text, name);
  if (mode == JFE_FATAL)
  { if (eno != 0)
    { printf("\n*** PROGRAM ABORTED! ***\n");
      if (wait_return != WM_NO)
      { printf("\nPress RETURN...");
        fgets(ttxt, 80, stdin);
      }
    }
    if (jfs_ip != NULL)
      fclose(jfs_ip);
    if (jfs_op_d != NULL)
      fclose(jfs_op_d);
    if (jfs_op_p != NULL)
      fclose(jfs_op_p);
    if (jf_head != NULL)
      jfr_close(jf_head);
    jft_close();
    jfr_free();
    jfg_free();
    exit(res);
  }
  return -1;
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
  char text[256];
  /* char ttxt[80]; */
  int m, a, res, slut;
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  struct jfg_adjectiv_desc adesc;

  slut = 0;
  jfg_var(&vdesc, jf_head, var_no);
  jfg_domain(&ddesc, jf_head, vdesc.domain_no);

  v->farg = vdesc.default_val;
  v->conf = 1.0;

  res = -1;
  while (res != 0)
  { if (ddesc.type == JFS_DT_FLOAT)
    { if ((ddesc.flags & JFS_DF_MINENTER) != 0
          && (ddesc.flags & JFS_DF_MAXENTER) != 0)
        sprintf(text, "*21:%1.4f:%1.4f", ddesc.dmin, ddesc.dmax);
      else
        sprintf(text, "*21");
    }
    else
    if (ddesc.type == JFS_DT_INTEGER)
    { if ((ddesc.flags & JFS_DF_MINENTER) != 0
          && (ddesc.flags & JFS_DF_MAXENTER) != 0)
        sprintf(text, "*%d:%d:%d", (int) (ddesc.dmax - ddesc.dmin + 1.0),
                                   (int) ddesc.dmin, (int) ddesc.dmax);
      else
        sprintf(text, "*11");
    }
    else
    { a = closest_adjectiv(var_no, v->farg);
      jfg_adjectiv(&adesc, jf_head, a);
      strcpy(text, "*");
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
      if (res == 0)
      { strcpy(iptext, text);
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
	         jf_error(res, iptext, JFE_ERROR);
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
        { jfscmd_ftoa(text, ddesc.dmin, digits);
          printf(" >= %s", text);
          if ((ddesc.flags & JFS_DF_MAXENTER) != 0)
            printf(" and");
        }
        if ((ddesc.flags & JFS_DF_MAXENTER) != 0)
        {
          jfscmd_ftoa(text, ddesc.dmax, digits);
          printf(" <= %s", text);
        }
        printf(",\n   ");
      }
      printf(" the symbol '!' (end),\n");
      printf(
      "    the symbol '*' (optionally followed by: steps[:begin:end]), or one of:\n    ");
      for (m = 0; m < vdesc.fzvar_c; m++)
      {
        jfg_adjectiv(&adesc, jf_head, vdesc.f_adjectiv_no + m);
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

int getvar(struct jft_data_record *dd, int var_no)
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

int ip_get(void)
{
  int m, slut;

  slut = 0;
  ident_text[0] = '\0';
  if (ip_medie == IP_KEYBOARD)
    printf("INPUT variable-values/intervals (Type ! to quit, ? for help):\n");

  for (m = 0; slut == 0 && m < ivar_c; m++)
  { slut = getvar(&(ip_vars[m]), spdesc.f_ivar_no + m);
    if (slut == 11 && m != 0)
      jf_error(11, jf_empty, JFE_ERROR);
  }
  v_ivar_c = 0;
  multivar_no = -1;
  for (m = 0; slut == 0 && m < ivar_c; m++)
  { if (ip_vars[m].mode == JFT_DM_INTERVAL
       || ip_vars[m].mode == JFT_DM_MMINTERVAL)
    { if (v_ivar_c >= V_IVAR_C_MAX)
	       return jf_error(601, jf_empty, JFE_ERROR);
      if (ip_vars[m].mode == JFT_DM_INTERVAL
          && ip_vars[m].sarg == -2)
      { if (multivar_no != -1)
          return jf_error(1302, jf_empty, JFE_ERROR);
        multivar_no = m;
      }
      else
      { v_ivars[v_ivar_c].ivar_no = m;
        v_ivars[v_ivar_c].antal = ip_vars[m].sarg;
        v_ivars[v_ivar_c].start_v = ip_vars[m].imin;
        v_ivars[v_ivar_c].end_v = ip_vars[m].imax;
        v_ivars[v_ivar_c].addent =  (ip_vars[m].imax - ip_vars[m].imin)
                                / ((float) v_ivars[v_ivar_c].antal - 1.0);
        v_ivar_c++;
      }
    }
    else
      ivar_values[m] = ip_vars[m].farg;
  }
  if (v_ivar_c == 0 || v_ivar_c > 2)
    return jf_error(1300, jf_empty, JFE_FATAL);
  return slut;
}

int var_find(char *name)
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

static void data_write(FILE *opd, int ovar_no)
{
  char txt[80];
  int m;
  float val;

  for (m = 0; m < v_ivar_c; m++)
  { val = ivar_values[v_ivars[m].ivar_no];
    jfscmd_ftoa(txt, val, digits);
    fprintf(opd, "%s ", txt);
  }
  jfscmd_ftoa(txt, ovar_values[ovar_no], digits);
  fprintf(opd, "%s\n", txt);
}

static int write_plt_start(void)
{
  char txt[514];

  fprintf(jfs_op_p, "\n#Plots from the jfs-program: %s\n\n",
           spdesc.title);
  if (plt_2_gif == 1)
    fprintf(jfs_op_p, "set terminal %s\n", term_name);
  fprintf(jfs_op_p, "\n");

  if (strlen(ps_fname) != 0)
  { jfs_ps = fopen(ps_fname, "r");
    if (jfs_ps == NULL)
      return jf_error(1, ps_fname, JFE_FATAL);
    while (fgets(txt, 512, jfs_ps) != NULL)
      fprintf(jfs_op_p, "%s", txt);
    fclose(jfs_ps);
    jfs_ps = NULL;
  }
  else
  { fprintf(jfs_op_p, "set title \"%s\"\n", spdesc.title);
    fprintf(jfs_op_p, "set grid\n");
    fprintf(jfs_op_p, "set contour surface\n");
    fprintf(jfs_op_p, "set data style lines\n");
    fprintf(jfs_op_p, "set view %1.2f,%1.2f,1,1\n", rot_x, rot_z);
    if (rot_x != 0.0)
      fprintf(jfs_op_p, "set surface\n");
    else
      fprintf(jfs_op_p, "set nosurface\n");
  }
  fprintf(jfs_op_p, "\n");
  return 0;
}

int write_plt_data(int ov_no)
{
  struct jfg_var_desc vdesc;
  struct jfg_var_desc multi_vdesc;
  struct jfg_adjectiv_desc adesc;
  struct jfg_domain_desc ddesc;
  int m;

  jfg_var(&vdesc, jf_head, ov_no + spdesc.f_ovar_no);
  jfg_domain(&ddesc, jf_head, vdesc.domain_no);
  fprintf(jfs_op_p, "\n");
  if (plt_2_gif == 1)
  { fprintf(jfs_op_p, "set output '");
    if (strlen(op_dir) > 0)
#ifndef _WIN32
      fprintf(jfs_op_p, "%s/",  op_dir);
#else
      fprintf(jfs_op_p, "%s\\", op_dir);
#endif
    fprintf(jfs_op_p, "%s.%s'\n", vdesc.name, op_extension);
  }
  jfg_var(&vdesc, jf_head, v_ivars[0].ivar_no + spdesc.f_ivar_no);
  fprintf(jfs_op_p, "set xlabel \"%s\" 0\n", vdesc.name);
  if (v_ivar_c == 1)
  { jfg_var(&vdesc, jf_head, ov_no + spdesc.f_ovar_no);
    fprintf(jfs_op_p, "set ylabel \"%s\" 0\n", vdesc.name);
    fprintf(jfs_op_p, "plot [%1.4f:%1.4f] ",
            v_ivars[0].start_v, v_ivars[0].end_v);
    if ((ddesc.flags & JFS_DF_MINENTER) != 0 &&
        (ddesc.flags & JFS_DF_MAXENTER) != 0 && auto_scale == 0)
      fprintf(jfs_op_p, "[%1.4f:%1.4f] ", ddesc.dmin, ddesc.dmax);

  }
  else
  { jfg_var(&vdesc, jf_head, v_ivars[1].ivar_no + spdesc.f_ivar_no);
    fprintf(jfs_op_p, "set ylabel \"%s\" 0\n", vdesc.name);
    jfg_var(&vdesc, jf_head, ov_no + spdesc.f_ovar_no);
    fprintf(jfs_op_p, "set zlabel \"%s\" 0\n", vdesc.name);
    fprintf(jfs_op_p,
       "splot [%1.4f:%1.4f] [%1.4f:%1.4f] ",
            v_ivars[0].start_v, v_ivars[0].end_v,
            v_ivars[1].start_v, v_ivars[1].end_v);
    if ((ddesc.flags & JFS_DF_MINENTER) != 0 &&
        (ddesc.flags & JFS_DF_MAXENTER) != 0 && auto_scale == 0)
      fprintf(jfs_op_p, "[%1.4f:%1.4f] ", ddesc.dmin, ddesc.dmax);
    /* fprintf(jfs_op_p, "'%s' notitle\n", op_d_fname); */
  }
  if (multivar_no == -1)
    fprintf(jfs_op_p, "'%s' notitle\n", op_d_fname);
  else
  { jfg_var(&multi_vdesc, jf_head, spdesc.f_ivar_no + multivar_no);
    for (m = 0; m < multi_vdesc.fzvar_c; m++)
    { jfg_adjectiv(&adesc, jf_head, multi_vdesc.f_adjectiv_no + m);
      fprintf(jfs_op_p, "'%s' index %d title \"%s\"",
              op_d_fname, m, adesc.name);
      if (m != multi_vdesc.fzvar_c - 1)
        fprintf(jfs_op_p, ",\\");
      fprintf(jfs_op_p, "\n");
    }
  }
  if (plt_2_gif == 0)
    fprintf(jfs_op_p, "pause -1 \"Hit RETURN to continue\"\n");
  fprintf(jfs_op_p, "\n");
  return 0;
}

static int init(void)
{
  int m;
  time_t t;

  m = jfr_init(stack_size);
  if (m != 0)
    return jf_error(m, jf_empty, JFE_FATAL);
  m = jfr_load(&jf_head, so_fname);
  if (m != 0)
    return jf_error(m, so_fname, JFE_FATAL);
  jfg_sprg(&spdesc, jf_head);
  ivar_c = spdesc.ivar_c;
  ovar_c = spdesc.ovar_c;

  if (strlen(ov_name) != 0)
  { m = var_find(ov_name);
    if (m == -1 || m < spdesc.f_ovar_no
        || m >= spdesc.f_ovar_no + spdesc.ovar_c)
      return jf_error(1301, ov_name, JFE_FATAL);
  }
  jft_init(jf_head);
  for (m = 0; m < ((int) strlen(field_sep)); m++)
    jft_char_type(field_sep[m], JFT_T_SPACE);

  if (op_append == 0)
  { jfs_op_p = fopen(op_p_fname, "w");
  }
  else
  { jfs_op_p = fopen(op_p_fname, "a");
  }
  if (jfs_op_p == NULL)
    jf_error(1, op_p_fname, JFE_FATAL);

  srand((unsigned) time(&t));   /* randomize(); */

  if (strlen(term_name) > 0)
    plt_2_gif = 1;
  else
    plt_2_gif = 0;

  return 0;
}

int ip_first(void)
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

int ip_next(void)
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
	         fprintf(jfs_op_d, "\n");
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
          fprintf(jfs_op_d, "\n");
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

static int us_error(void) /* usage-error */
{
  char ttxt[82];

  jfscmd_fprint_wrapped(stdout, jfscmd_num_of_columns() - 7, "usage: ", "       ", usage);
  if (wait_return != WM_NO)
  { printf("\nPress RETURN...");
    fgets(ttxt, 80, stdin);
  }
  return 1;
}

int main(int argc, const char *argv[])
{
  int m, slut, largc, res, mv_index = 0;
  unsigned short option_no;
  char ttxt[256];
  const char *largv[128];
  struct jfg_var_desc vdesc;
  struct jfg_var_desc multi_vdesc;
  struct jfg_adjectiv_desc adesc;
  
  field_sep[0]  = '\0';
  if (argc == 1)
  {
    jfscmd_print_about(about);
    return 0;
  }
  strcpy(so_fname, argv[argc - 1]);
  jfscmd_ext_subst(so_fname, extensions[0], 0);

  jopt_init(options, OPT_COUNT, argv, argc - 1);
  while ((res = jopt_get(&option_no, largv, &largc)) == 0)
  { switch (option_no)
    { case 0:              /* -f  */
        strcpy(field_sep, largv[0]);
        break;
      case 1:              /* -p */
        digits = atoi(largv[0]) + 1;
        break;
      case 2:            /* -ov */
        strcpy(ov_name, largv[0]);
        break;
      case 3:          /* -t */
        strcpy(term_name, largv[0]);
        strcpy(op_extension, largv[1]);
        if (largc == 3)
          strcpy(op_dir, largv[2]);
        break;
      case 4:            /* -d */
        ip_medie = IP_FILE;
        strcpy(ip_fname, largv[0]);
        jfscmd_ext_subst(ip_fname, extensions[1], 0);
        break;
      case 5:            /* -o */
        strcpy(op_p_fname, largv[0]);
        jfscmd_ext_subst(op_p_fname, extensions[2], 0);
        break;
      case 6:           /* -a */
        op_append = 1;
        break;
      case 7:          /* -w */
        if (largc == 1)
        { if (strcmp(largv[0], "y") == 0)
            wait_return = WM_YES;
          else
          if (strcmp(largv[0], "n") == 0)
            wait_return = WM_NO;
          else
            wait_return = WM_ERROR;
        }
        else
          wait_return = WM_YES;
        break;
      case 8:          /* -i */
        strcpy(ps_fname, largv[0]);
        jfscmd_ext_subst(ps_fname, extensions[2], 0);
        break;
      case 9:          /* -rs */
        stack_size = atoi(largv[0]);
        if (stack_size <= 0)
          return us_error();
        break;
      case 10:                     /* -Ax */
        rot_x = atof(largv[0]);
        break;
      case 11:                     /* -Az */
        rot_z = atof(largv[0]);
        break;
      case 12:                     /* -As */
        auto_scale = 1;
        break;
      case 13:          /* -?  */
      case 14:          /* ? */
        jfscmd_print_about(about);
        return 0;
    }
  }  /* while  */
  if (jopt_error_desc.error_mode != JOPT_EM_NONE)
  { res = jf_error(jopt_error_desc.error_no, jopt_error_desc.argument, JFE_ERROR);
    return res;
  }

  if (argc == 2)
  { if (strcmp(argv[1], "-w") == 0)
    {
      wait_return = WM_YES;
      jfscmd_print_about(about);
      if (wait_return == WM_YES)
      { printf("\nPress RETURN...");
        fgets(ttxt, 80, stdin);
      }
      return 0;
    }
  }

  if (jfg_init(JFG_PM_NORMAL, 64, 4) != 0)
    return jf_error(6, "jfg-stack", JFE_FATAL);

  if (strlen(op_p_fname) == 0)
  { strcpy(op_p_fname, so_fname);
    jfscmd_ext_subst(op_p_fname, extensions[2], 1);
  }

  init();

  slut = 0;
  if (ip_medie == IP_FILE)
  { slut = jft_fopen(ip_fname, JFT_FM_INPUT, 0);
    if (slut != 0)
      return jf_error(jft_error_desc.error_no, ip_fname, JFE_FATAL);
  }
  slut = ip_get();
  if (slut == 0)
  { write_plt_start();
    for (m =  0; m < ovar_c; m++)
    { jfg_var(&vdesc, jf_head, spdesc.f_ovar_no + m);
      if (strlen(ov_name) == 0 || strcmp(ov_name, vdesc.name) == 0)
      { strcpy(op_d_fname, op_p_fname);
        jfscmd_ext_rm(op_d_fname);
        strcat(op_d_fname, vdesc.name);
        jfscmd_ext_subst(op_d_fname, extensions[1], 0);
        jfs_op_d = fopen(op_d_fname, "w");
        if (jfs_op_d == NULL)
          return jf_error(1, op_d_fname, JFE_FATAL);
        if (multivar_no != -1)
        { jfg_var(&multi_vdesc, jf_head, spdesc.f_ivar_no + multivar_no);
          mv_index = 0;
        }
        while (slut == 0)
        { slut = ip_first();
          if (multivar_no != -1)
          { jfg_adjectiv(&adesc, jf_head, multi_vdesc.f_adjectiv_no + mv_index);
            ivar_values[multivar_no] = adesc.center;
          }
          while (slut == 0)
          { jfr_run(ovar_values, jf_head, ivar_values);
            data_write(jfs_op_d, m);
            slut = ip_next();
          }
          if (multivar_no != -1)
          { mv_index++;
            if (mv_index >= multi_vdesc.fzvar_c)
              slut = 1;
            else
            { slut = 0;
              fprintf(jfs_op_d, "\n");
            }
          }
        }
        fclose(jfs_op_d);
        jfs_op_d = NULL;
        write_plt_data(m);
      }
    }
  }
  if (wait_return == WM_YES)
  { printf("\nPress RETURN...");
    fgets(ttxt, 80, stdin);
  }

  return jf_error(0, jf_empty, JFE_FATAL); /* no error */
}
