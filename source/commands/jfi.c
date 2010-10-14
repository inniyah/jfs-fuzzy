  /*************************************************************************/
  /*                                                                       */
  /* jfi.c - JFS improver using evolutionary programing                    */
  /*                             Copyright (c) 1998-2000 Jan E. Mortensen  */
  /*                                       Copyright (c) 2010 Miriam Ruiz  */
  /*                                                                       */
  /*************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include "cmds_lib.h"
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfp_lib.h"
#include "jft_lib.h"
#include "jfi_lib.h"

#define JFE_WARNING 0
#define JFE_ERROR   1
#define JFE_FATAL   2

#define JFI_VMAX 256

static const char usage[] =
	"jfi [-D dm] [-d df] [-f fs] [-o of] [-Et min] [-Ee er] [-pm p]"
	" [-I ic] [-EI i] [-sm m] [-s] [-r] [-so s] [-a] [-w] <file.jfr>";

static const char *about[] = {
  "usage: jfi [options] <file.jfr>",
  "",
  "JFI improves the jfs-program <file.jfr> by changing the values of constants, starting with a '%'.",
  "",
  "Options:",
  "-f <fs>    : Use <fs> field-separator.    -s:      : silent.",
  "-I <c>     : Population size.             -r:      : score rounding to [0,1].",
  "-so <s>    : Redirect stdout to <s>.      -a:      : Append to stdout.",
  "-w         : Wait for RETURN.",
  "-D <d>     : Data order. <d>={i|e|t}|f. i:input,e:expected,t:key,f:first-line.",
  "-d <df>    : Read data from the file <df>.",
  "-o <of>    : Write changed jfs-program to the file <of>.",
  "-Et <et>   : Stop after <et> minutes.",
  "-Ee <ee>   : Stop if error <= <ee>.",
  "-Ei <i>    : Stop after <i> individuals.",
  "-sm <s>    : score-method: 'a':avg,'s':sum,'s2':sum(sqr),'p':penalty-matrix.",
  "-pm <p>    : Read penalty-matrix from the file <p>.",
  "-sc <m>    : score-calc. <m>='f':fast, <m>='e':exact.",
  "-uv <m>    : value unknown vars. <m>=z:0,o:1,a:1/count,d0:conf=0,d:conf=1.",
  NULL
};

struct jfscmd_option_desc jf_options[] = {
        {"-f",  1},        /*  0 */
        {"-s",  0},        /*  1 */
        {"-D",  1},        /*  2 */
        {"-d",  1},        /*  3 */
        {"-o",  1},        /*  4 */
        {"-Et", 1},        /*  5 */
        {"-Ee", 1},        /*  6 */
        {"-I",  1},        /*  7 */
        {"-Ei", 1},        /*  8 */
        {"-sm", 1},        /*  9 */
        {"-r",  0},        /* 10 */
        {"-so", 1},        /* 11 */
        {"-a",  0},        /* 12 */
        {"-w",  0},        /* 13 */
        {"-sc", 0},        /* 14 */
        {"-uv", 1},        /* 15 */
        {"-pm", 1},        /* 16 */
        {"-?",  0},        /* 17 */
        {"?",   0},        /* 18 */
        {" ",  -2}
};

/************************************************************************/
/* Traenings-data-set                                                   */
/************************************************************************/

float *jfi_data;
/*                   dataene:  inputs, expecteds.                       */

long  jfi_data_c;      /* antal data-set.                               */

struct jfg_sprog_desc spdesc;

int jfi_ivar_c;
int jfi_ovar_c;
int jfi_laes_c;

float jfi_ivalues[JFI_VMAX];
float jfi_ovalues[JFI_VMAX];
float jfi_expected[JFI_VMAX];
float jfi_confidences[JFI_VMAX];
float jfi_def_values[JFI_VMAX];

/************************************************************************/
/* Afslutningstest/Statistik information                                */
/************************************************************************/

int jfi_err_mode = 0;  /* 0: error = avg-error      */
                       /* 1: error = sum-error      */
                       /* 2: error = sqr-sum        */
                       /* 3: error = penalty-matrix */

int jfi_uv_mode = 4;   /* if unknown variable:   */
                       /* 0: undef, fzvars=zero, */
                       /* 1: undef, fzvars=1.0,  */
                       /* 2: undef, fzvars=1/ant,*/
                       /* 3: default, conf=0.0,  */
                       /* 4: default, conf=1.0.  */


int jfi_score_method = 0;  /* 0: fast,           */
                           /* 1: exact.          */

float jfi_undefined;   /* float-value for undefined (=max_value + 1.0). */

time_t jfi_start_time;
time_t jfi_cur_time;

int jfi_t_mode = 1000; /* tekst-mode */

char jfi_t_head[] =
"  ind_no best-score  avg-score  median-score  used time";

/*************************************************************************/
/* Option-data                                                           */
/*************************************************************************/

char da_fname[256] = "";
char ip_fname[256] = "";
char op_fname[256] = "";
char sout_fname[256] = "";
char pm_fname[256] = "";

FILE *jfi_stdout = NULL;

int jfi_fmode = JFT_FM_INPUT_EXPECTED;  /* JFT-file mode */

float jfi_maxerr   = 0.0; /* default: stop if no error.           */
int   jfi_maxtime  = 60;
unsigned long  jfi_end_ind   = 10000;
int   jfi_silent    = 0;

int   jfi_err_round = 0;  /* 1: round errors to [0, 1].           */

char jfi_empty[] = " ";
char jfi_space[80];

int  jfi_batch  = 1;
int  jfi_append = 0;

char jfi_field_sep[256];   /* 0: brug space, tab etc som felt-seperator, */
       /* andet: kun field_sep er feltsepator. */

struct jfscmd_tmap_desc jf_im_texts[] =        /* input-modes */
{
  { JFT_FM_INPUT_EXPECTED,     "ie"},
  { JFT_FM_INPUT_EXPECTED_KEY, "iet"},
  { JFT_FM_EXPECTED_INPUT,     "ei"},
  { JFT_FM_EXPECTED_INPUT_KEY, "eit"},
  { JFT_FM_KEY_INPUT_EXPECTED, "tie"},
  { JFT_FM_KEY_EXPECTED_INPUT, "tei"},
  { JFT_FM_FIRST_LINE,         "f"},
  { -1,                        ""}
};

/**************************************************************************/
/* Program-data                                                           */
/**************************************************************************/

void *jfr_head = NULL;

/*************************************************************************/
/* DIverse                                                               */
/*************************************************************************/

struct jfi_head_desc { unsigned long repro_c;
                       float best_score;
                       float sum_score;
                       unsigned long ind_c;
                     };

struct jfi_head_desc jfi_head;

const char jfi_t_data[] = "data";
const char jfi_t_atomd[]= "converting-table";
const char jfi_t_individs[]= "individuals";
const char jfi_t_score[] = "score-tables";
const char jfi_t_rules[] = "rules";
const char jfi_t_stack[] = "stacks (jfg-lib)";
const char jfi_t_stat[]  = "statement (jfp-lib)";

const char *jfi_t_methods[] = {
                          "crosover",   /* 0 */
                          "sumcros",    /* 1 */
                          "mutation"    /* 2 */
                        };

struct jfr_err_desc {
	int eno;
	const char *text;
};

struct jfr_err_desc jfr_err_texts[] =
        {{  0, " "},
         {  1, "Cannot open file:"},
         {  2, "Error reading from file:"},
         {  3, "Error writing to file:"},
         {  4, "Not an jfr-file:"},
         {  5, "Wrong version:"},
         {  6, "Cannot allocate memory to:"},
         {  9, "Illegal number:"},
         { 10, "Value out of domain-range:"},
         { 11, "Unexpected EOF."},
         {  13, "Undefined adjective: "},
         {  14, "Missing start/end of interval."},
         {  15, "No value for variable:"},
         {  16, "Too many values in a record (max 255)."},
         {  17, "Illegal jft-file-mode."},
         {  18, "Token to long (max 255 chars)."},
         {  19, "Penalty-matrix and more than one output-variable."},
         {  20, "Too many penalty-values (max 64)."},
         {  21, "No values in first data-line."},
         {401, "jfp cannot insert this type of statement"},
         {402, "Statement to large."},
         {403, "Not enogh free memory to insert statement"},
         {801, "No stop-condition ! (-Ei, -Et or -Et)"},
         {802, "No '%' in program (nothing to improve!)"},

         {9999, "Unknown error!"},
     };

static void jf_close(void);
static int jf_error(int eno, const char *name, int mode);
static int us_error(void);

static void jf_close(void)
{
 jfg_free();
 jfp_free();
 if (jfr_head != NULL)
   jfr_close(jfr_head);
 jfi_free();
 if (jfi_data != NULL)
   free(jfi_data);
 jfr_free();
 if (jfi_stdout != stdout)
   fclose(jfi_stdout);
}

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
  { fprintf(jfi_stdout, "WARNING %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    m = 0;
  }
  else
  { fprintf(jfi_stdout, "*** error %d: %s %s\n",
                          eno, jfr_err_texts[e].text, name);
    if (mode == JFE_FATAL)
    { fprintf(jfi_stdout, "\n*** PROGRAM ABORTED! ***\n");
      jf_close();
      if (jfi_batch == 0)
      { printf("Press RETURN ....");
        fgets(jfi_field_sep, 78, stdin);
      }
      exit(1);
    }
    m = -1;
  }
  return m;
}

/************************************************************************/
/* Funktioner til indlaesning fra fil                                   */
/************************************************************************/

static int jfi_data_get(int mode)
{
  int slut, m;
  float confs[256];
  char txt[256];

  slut = 0;
  jfi_data_c = 0;
  while (slut == 0)
  {
    long adr = 0;
    if (mode == 0)
      slut = jft_getrecord(NULL, NULL, NULL, NULL);
    else
    { adr = jfi_data_c * jfi_laes_c;
      slut = jft_getrecord(&(jfi_data[adr]), confs, &(jfi_data[adr + jfi_ivar_c]),
                           NULL);
    }
    if (slut != 11)
    { if (slut != 0)
      { if (mode == 0)
        { sprintf(txt, " %s in file: %s line %d.",
                  jft_error_desc.carg, da_fname, jft_error_desc.line_no);
          jf_error(jft_error_desc.error_no, txt, JFE_ERROR);
        }
        slut = 0;
      }
      if (mode == 1)
      { for (m = 0; m < jfi_laes_c; m++)
        { if (confs[m] == 0.0)
            jfi_data[adr + m] = jfi_undefined;
        }
      }
      if (slut == 0)
        jfi_data_c++;
    }
  }
  if (mode == 0)
    jfi_undefined = jft_dset_desc.max_value + 1.0;
  return slut;
}

/*************************************************************************/
/* Generelle hjaelpe-funktioner                                          */
/*************************************************************************/

void jfi_d_get(long data_no)
{
  long ad;
  int m;

  ad = data_no * ((long) jfi_laes_c);
  for (m = 0; m < jfi_ivar_c; m++)
  { jfi_ivalues[m] = jfi_data[ad];
    ad++;
    if (jfi_ivalues[m] != jfi_undefined)
      jfi_confidences[m] = 1.0;
    else
    { jfi_ivalues[m] = jfi_def_values[m];
      if (jfi_uv_mode == 0)
        jfi_confidences[m] = -3.0;
      else
      if (jfi_uv_mode == 1)
        jfi_confidences[m] = -1.0;
      else
      if (jfi_uv_mode == 2)
        jfi_confidences[m] = -2.0;
      else
        jfi_confidences[m] = 0.0;
    }
  }
  for (m = 0; m < jfi_ovar_c; m++)
  { jfi_expected[m] = jfi_data[ad];
    ad++;
  }
}

void jfi_p_end(int mode)
{
  fprintf(jfi_stdout, "\nImproving stopped! Stop-condition: ");
  if (mode == 0)
    fprintf(jfi_stdout, " result\n");
  else
  if (mode == 1)
    fprintf(jfi_stdout, " time\n");
  else
    fprintf(jfi_stdout, " individuals\n");
}

int jfi_test(void)
{
  int res;
  long ut;

  res = 0;
  if (jfi_head.best_score <= jfi_maxerr)
  { jfi_stop();
    jfi_p_end(0);
  }
  jfi_cur_time = time(NULL);
  if (jfi_maxtime > 0 && (jfi_cur_time - jfi_start_time) / 60 >= jfi_maxtime)
  { jfi_stop();
    jfi_p_end(1);
  }
  if (jfi_end_ind > 0 && jfi_head.repro_c >= jfi_end_ind)
  { jfi_stop();
    jfi_p_end(2);
  }
  if (jfi_silent == 0 &&
      (jfi_head.repro_c % 100 == 0 || jfi_head.repro_c == jfi_head.ind_c))
  { if (jfi_t_mode > 20)
    { fprintf(jfi_stdout, "\n%s\n", jfi_t_head);
      jfi_t_mode = 0;
    }
    fprintf(jfi_stdout, "  %4ld ", (long) jfi_head.repro_c);
    fprintf(jfi_stdout, " %10.4f ", jfi_head.best_score);
    fprintf(jfi_stdout, " %10.4f ", jfi_head.sum_score / ((float) jfi_head.ind_c));
    fprintf(jfi_stdout, " %10.4f ", jfi_stat.median_score);
    ut = jfi_cur_time - jfi_start_time;
    if (ut != 0)
    { fprintf(jfi_stdout, "   %3ld:", (long) ut / 60);
      fprintf(jfi_stdout, "%2ld", (long) ut % 60);
    }
    fprintf(jfi_stdout, "\n");
    jfi_t_mode++;
  }
  return res;
}

float jfi_d_judge(long data_no)
{
  float dist;
  int m;

  jfi_d_get(data_no);

  jfr_arun(jfi_ovalues, jfr_head, jfi_ivalues, jfi_confidences,
           NULL, NULL, NULL);

  dist = 0.0;
  if (jfi_err_mode == 3)  /* penalty-matrix */
    dist = jft_penalty_calc(jfi_ovalues[0], jfi_expected[0]);
  else
  {
    float r = 0;
    for (m = 0; m < jfi_ovar_c; m++)
    { r = jfi_expected[m] - jfi_ovalues[m];
      if (jfi_err_mode == 2)
        r = r * r;
      if (r < 0)
        r = -r;
    }
    dist += r;
    if (jfi_err_round == 1)
    { if (dist > 1.0)
        dist = 1.0;
    }
  }  
  return dist;
}

float this_judge(void)
{
  long d;
  float avg_err, dist, worst;
  int slut;

  avg_err = 0.0;
  worst = jfi_stat.worst_score;
  if (jfi_err_mode == 0)
    worst *= ((float) jfi_data_c);
  slut = 0;
  for (d = 0; slut == 0 && d < jfi_data_c; d++)
  { dist = jfi_d_judge(d);
    avg_err += dist;
    if (jfi_score_method == 0 &&
        avg_err > worst && jfi_head.repro_c > jfi_head.ind_c)
      slut = 1;  /* stop judging, because worse than worst individ */
  }

  if (jfi_err_mode == 0)
    avg_err /= ((float) jfi_data_c);

  jfi_head.sum_score -= jfi_stat.old_score;
  jfi_head.sum_score += avg_err;
  if (avg_err < jfi_head.best_score || jfi_head.repro_c == 0)
  { if (jfi_silent == 0)
    { fprintf(jfi_stdout, "\nNew best. Repro_no: %ld",
                          jfi_head.repro_c);
      fprintf(jfi_stdout, " score:%10.4f. Created by %s\n",
      avg_err, jfi_t_methods[jfi_stat.method]);
      fprintf(jfi_stdout, "         parent scores: %12.4f, %12.4f\n",
                          jfi_stat.p1_score, jfi_stat.p2_score);
      jfi_t_mode += 4;
    }
    jfi_head.best_score = avg_err;
  }
  jfi_head.repro_c++;
  jfi_test();
  return avg_err;
}

static int us_error(void) /* usage-error */
{
	jfscmd_fprint_wrapped(stdout, jfscmd_num_of_columns() - 7, "usage: ", "       ", usage);
	if (jfi_batch == 0)
	{
		printf("Press RETURN ....");
		fgets(jfi_field_sep, 78, stdin);
	}
	return 1;
}

static void wait_if_needed()
{
  if (jfi_batch == 0)
  { printf("Press RETURN ....");
    fgets(jfi_field_sep, 78, stdin);
  }
}

int main(int argc, const char *argv[])
{
  int m;
  long size;

  const char *extensions[]  = {
                          "jfr",     /* 0 */
                          "dat",     /* 1 */
                          "pm"       /* 2 */
                        };

  char sm_t_s[]  = "s";
  char sm_t_a[]  = "a";
  char sm_t_s2[] = "s2";
  char sm_t_p[]  = "p";
  int option_no;
  struct jfg_var_desc vdesc;
  time_t t;

  srand((unsigned) time(&t));
  jfi_data = NULL;
  jfi_head.repro_c = 0;
  jfi_head.best_score = 100000.0;
  jfi_head.sum_score = 0.0;
  jfi_head.ind_c = 40;

  if (argc == 1)
  {
    jfscmd_print_about(about);
    return 0;
  }
  if (argc == 2)
  { if (jfscmd_getoption(jf_options, argv, 1, argc) == 13)
    { jfi_batch = 0;
      jfscmd_print_about(about);
      wait_if_needed();
      return 0;
    }
  }
  strcpy(ip_fname, argv[argc - 1]);
  jfscmd_ext_subst(ip_fname, extensions[0], 0);
  for (m = 1; m < argc - 1; )
  { option_no = jfscmd_getoption(jf_options, argv, m, argc - 1);
    if (option_no == -1)
      return us_error();
    m++;
    switch (option_no)
    { case 0:              /* -f */
        strcpy(jfi_field_sep, argv[m]);
        m++;
        break;
      case 1:           /* -s */
       jfi_silent = 1;
       break;
      case 2:              /* -D */
        jfi_fmode = jfscmd_tmap_find(jf_im_texts, argv[m]);
        if (jfi_fmode == -1)
          return us_error();
        m++;
        break;
      case 3:            /* -d */
        strcpy(da_fname, argv[m]);
        jfscmd_ext_subst(da_fname, extensions[1], 0);
        m++;
        break;
      case 4:            /* -o */
        strcpy(op_fname, argv[m]);
        jfscmd_ext_subst(op_fname, extensions[0], 0);
        m++;
        break;
      case 5:             /* -Et */
        jfi_maxtime = atoi(argv[m]);
        m++;
        break;
      case 6:             /* -Ee */
        jfi_maxerr = atof(argv[m]);
        m++;
        break;
      case 7:           /* -I  */
        jfi_head.ind_c = atoi(argv[m]) ;
        m++;
        break;
      case 8:           /* -Ei */
        jfi_end_ind = atol(argv[m]);
        m++;
        break;
      case 9:           /* -sm */
        if (strcmp(argv[m], sm_t_a) == 0)
          jfi_err_mode = 0;
        else
        if (strcmp(argv[m], sm_t_s) == 0)
          jfi_err_mode = 1;
        else
        if (strcmp(argv[m], sm_t_s2) == 0)
          jfi_err_mode = 2;
        else
        if (strcmp(argv[m], sm_t_p) == 0)
          jfi_err_mode = 3;
        else
          return us_error();
        m++;
        break;
      case 10:         /* -r */
        jfi_err_round = 1;
        break;
      case 11:         /* -so */
        strcpy(sout_fname, argv[m]);
        m++;
        break;
      case 12:         /* -a */
        jfi_append = 1;
        break;
      case 13:         /* -w */
        jfi_batch = 0;
        break;
      case 14:         /* -sc */
        if (strcmp(argv[m], "f") == 0)
          jfi_score_method = 0;
        else
        if (strcmp(argv[m], "e") == 0)
          jfi_score_method = 1;
        else
          return us_error();
        m++;
        break;
      case 15:        /* -uv */
        if (strcmp(argv[m], "z") == 0)
          jfi_uv_mode = 0;
        else
        if (strcmp(argv[m], "o") == 0)
          jfi_uv_mode = 1;
        else
        if (strcmp(argv[m], "a") == 0)
          jfi_uv_mode = 2;
        else
        if (strcmp(argv[m], "d0") == 0)
          jfi_uv_mode = 3;
        else
        if (strcmp(argv[m], "d") == 0)
          jfi_uv_mode = 4;
        else
          return us_error();
        m++;
        break;
      case 16:     /* -pm */
        strcpy(pm_fname, argv[m]);
        jfscmd_ext_subst(pm_fname, extensions[2], 0);
        jfi_err_mode = 3;
        m++;
        break;
      default:
        return us_error();
    }
  }  /* for  */

  jfi_stdout = stdout;
  if (strlen(sout_fname) != 0)
  { if (jfi_append == 0)
      jfi_stdout = fopen(sout_fname, "w");
    else
      jfi_stdout = fopen(sout_fname, "a");
    if (jfi_stdout == NULL)
    { jfi_stdout = stdout;
      printf("Cannot open the file %s.\n", sout_fname);
    }
  }

  if (strlen(op_fname) == 0)
  { strcpy(op_fname, ip_fname);
    jfscmd_ext_subst(op_fname, extensions[0], 1);
  }

  if (strlen(da_fname) == 0)
  { strcpy(da_fname, ip_fname);
    jfscmd_ext_subst(da_fname, extensions[1], 1);
  }

  m = jfr_init(0);
  if (m != 0)
    jf_error(m, jfi_t_stack, JFE_FATAL);
  m = jfg_init(0, 64, 4);
  if (m != 0)
    jf_error(m, jfi_t_stack, JFE_FATAL);
  m = jfp_init(0);
  if (m != 0)
    jf_error(m, jfi_t_stat, JFE_FATAL);
  m = jfr_load(&jfr_head, ip_fname);
  if (m != 0)
    jf_error(m, ip_fname, JFE_FATAL);
  fprintf(jfi_stdout, "jfs-program: %s loaded.\n\n", ip_fname);

  jft_init(jfr_head);
  for (m = 0; m < ((int) strlen(jfi_field_sep)); m++)
    jft_char_type(jfi_field_sep[m], JFT_T_SPACE);

  if (jfi_err_mode == 3)  /* penalty-matrix */
  { if (strlen(pm_fname) == 0)
    { strcpy(pm_fname, ip_fname);
      jfscmd_ext_subst(pm_fname, extensions[2], 1);
    }
    m = jft_penalty_read(pm_fname);
    if (m == -1)
      jf_error(jft_error_desc.error_no, jft_error_desc.carg, JFE_FATAL);
  }

  m = jft_fopen(da_fname, jfi_fmode, 0);
  if (m != 0)
  { jft_close();
    return jf_error(jft_error_desc.error_no, jft_error_desc.carg, JFE_FATAL);
  }

  /* Find antal data-s‘t, og indlaes data */
  jfg_sprg(&spdesc, jfr_head);
  jfi_ivar_c = spdesc.ivar_c;
  jfi_ovar_c = spdesc.ovar_c;
  jfi_laes_c = jfi_ivar_c + jfi_ovar_c;

  fprintf(jfi_stdout, "Data loader started..\n");
  jfi_data_get(0);
  jft_rewind();
  size = (jfi_data_c + 1) * (long) jfi_laes_c * (long) sizeof(float);
  if ((jfi_data = (float *) malloc(size)) == NULL)
  { jft_close();
    return jf_error(6, jfi_t_data, JFE_FATAL);
  }
  jfi_data_get(1);
  fprintf(jfi_stdout,
          "data loader finished. %ld training-sets loaded from: %s.\n",
          jfi_data_c, da_fname);
  jft_close();

  /* read default-values for input-variables (used in undefined input-values)*/
  for (m = 0; m < jfi_ivar_c; m++)
  { jfg_var(&vdesc, jfr_head, spdesc.f_ivar_no + m);
    jfi_def_values[m] = vdesc.default_val;
  }

  if ((m = jfi_init(jfr_head, jfi_head.ind_c)) != 0)
    return jf_error(m, jfi_empty, JFE_FATAL);

  jfi_start_time = time(NULL);
  fprintf(jfi_stdout, "Training started.\n");

  jfi_run(this_judge, 0);

  fprintf(jfi_stdout, "  score best individual: %10.3f\n", jfi_head.best_score);
  fprintf(jfi_stdout, "  Number off created individuals: %ld\n\n",
                      jfi_head.repro_c);
  fprintf(jfi_stdout, "Changed program written to: %s\n\n", op_fname);

  if (jfi_stdout == stdout)
    printf("\a");     /* bell */

  jfp_save(op_fname, jfr_head);

  if (jfi_batch == 0)
  { printf("Press RETURN ....");
    fgets(sout_fname, 78, stdin);
  }
  jf_close();

  return 0;
}


