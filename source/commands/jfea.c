  /*************************************************************************/
  /*                                                                       */
  /* jfea.c - JFS rule creater using evolutionary programing               */
  /*                             Copyright (c) 1999-2000 Jan E. Mortensen  */
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
#include "jfea_lib.h"

#define JFE_WARNING 0
#define JFE_ERROR   1
#define JFE_FATAL   2

#define JFI_VMAX 100

static const char usage[] =
	"jfea [-D dm] [-d df] [-f fs] [-o of] [-Et m] [-Ee e] [-fr] [-nd] [-s] [-I ic]"
	" [-EI i] [-sm m] [-r] [-so s] [-a] [-w] [-R r] [-pm m] <file.jfr> ";

static const char *about[] = {
  "usage: jfea [options] <file.jfr>",
  "",
  "JFEA replaces a statement 'extern jfrd input {[<op>] <var>} output [<op>]<var>' in the jfs-program <file.jfr>"
    " with rules generated from a data file."
  "",
  "Options:",
  "-f <fs>    : Use <fs> field-separator.    -s:      : silent.",
  "-I <c>     : Population size.             -r:      : score rounding to [0,1].",
  "-so <s>    : Redirect stdout to <s>.      -a:      : Append to stdout.",
  "-w         : Wait for RETURN.             -R <r>   : number of if-statements.",
  "-fr        : fixed rules.                 -nd      : No rules default-adjectiv.",
  "-Et <et>   : Stop after <et> minutes.     -Ee <e>  : Stop if error <= <e>.",
  "-Ei <i>    : Stop after <i> individuals.  -pm <p>  : Read penalty from <p>.",
  "-d <df>    : Read data from the file <df>.",
  "-Es <p>    : Use <p> percent of datasets for cross-validation.",
  "-D <d>     : Data-order. <d>={i|e|t}|f. i:input,e:expected,t:text,f:first-line.",
  "-o <of>    : Write changed jfs-program to the file <of>.",
  "-sm <s>    : score-method. a:avg, s:sum, s2:sum(sqr), p:penalty-matrix.",
  "-sc <m>    : score-calc. <m>='f':fast, <m>='e':exact.",
  "-uv <m>    : value unknown vars. <m>=z:0,o:1,a:1/count,d0:conf=0,d:conf=1.",
  NULL
};

struct jfscmd_option_desc jf_options[] =
  {     {"-f",  1},        /*  0 */
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
        {"-R",  1},        /* 16 */
        {"-fr", 0},        /* 17 */
        {"-nd", 0},        /* 18 */
        {"-Es", 1},        /* 19 */
        {"-pm", 1},        /* 20 */

        {"-?",  0},        /* 21 */
        {"?",   0},        /* 22 */
        {" ",  -2}

      };

/************************************************************************/
/* Traenings-data-set                                                   */
/************************************************************************/

float *jfea_data;
/*                   dataene:  inputs, expecteds, weight, cval, wexp.   */

long  jfea_data_c;      /* antal data-set.                               */

struct jfg_sprog_desc spdesc;

float *jfea_f_data;

int jfea_ivar_c;
int jfea_ovar_c;
int jfea_laes_c;  /* = jfea_ivar_c + jfear_ovar_c + jfea_early_stop */

float jfea_ivalues[JFI_VMAX];
float jfea_ovalues[JFI_VMAX];
float jfea_expected[JFI_VMAX];
float jfea_confidences[JFI_VMAX];
float jfea_def_values[JFI_VMAX];

int jfea_cross_validation = 0;

/************************************************************************/
/* Afslutningstest/Statistik information                                */
/************************************************************************/

int jfea_err_mode = 0;  /* 0: error = avg-error      */
                        /* 1: error = sum-error      */
                        /* 2: error = sqr-sum        */
                        /* 3: error = penalty-matrix */

int jfea_uv_mode = 4;   /* if unknown variable:   */
                        /* 0: undef, fzvars=zero, */
                        /* 1: undef, fzvars=1.0,  */
                        /* 2: undef, fzvars=1/ant,*/
                        /* 3: default, conf=0.0,  */
                        /* 4: default, conf=1.0.  */


int jfea_score_method = 0;  /* 0: fast,           */
                            /* 1: exact.          */

float jfea_undefined;

int jfea_early_stop = 0;    /* 0: don't use early stop,                   */
                            /* 1: use early stop.                         */

time_t jfea_start_time;
time_t jfea_cur_time;

int jfea_t_mode = 1000; /* tekst-mode */

char jfea_t_head[] =
"  ind_no best-score  avg-score  median-score  used time";

/*************************************************************************/
/* Option-data                                                           */
/*************************************************************************/

char da_fname[256] = "";
char ip_fname[256] = "";
char op_fname[256] = "";
char sout_fname[256] = "";
char pm_fname[256]   = "";

FILE *jfea_stdout = NULL;

int jfea_fmode = JFT_FM_INPUT_EXPECTED;

float jfea_maxerr   = 0.0; /* default: stop if no error.           */
int   jfea_maxtime  = 60;
unsigned long  jfea_end_ind   = 10000;
int   jfea_silent    = 0;

int   jfea_err_round = 0;  /* 1: round errors to [0, 1].           */

char jfea_empty[] = " ";
char jfea_space[80];

int  jfea_batch  = 1;
int  jfea_append = 0;
char jfea_field_sep[256];   /* 0: brug space, tab etc som felt-seperator, */
                            /* andet: kun field_sep er feltsepator. */



/**************************************************************************/
/* Program-data                                                           */
/**************************************************************************/

void *jfr_head = NULL;

/*************************************************************************/
/* Diverse                                                               */
/*************************************************************************/

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

struct jfea_head_desc { unsigned long repro_c;
                        float best_score;
                        float sum_score;
                        float best_cv_score;
                        float new_cv_score;
                        long  cv_ind_id;
                        unsigned long ind_c;
                      };

struct jfea_head_desc jfea_head;

const char jfea_t_data[] = "data";
const char jfea_t_atomd[]= "converting-table";
const char jfea_t_individs[]= "individuals";
const char jfea_t_score[] = "score-tables";
const char jfea_t_rules[] = "rules";
const char jfea_t_stack[] = "stacks (jfg-lib)";
const char jfea_t_stat[]  = "statement (jfp-lib)";

const char *jfea_t_methods[] = {
                           "crosover",       /* 0 */
                           "sumcros",        /* 1 */
                           "mutation",       /* 2 */
                           "creation",       /* 3 */
                           "pointcros",      /* 4 */
                           "rulecros",       /* 5 */
                           "stepmutation",   /* 6 */
                           "repeatmutation", /* 7 */
                           "maxcros",        /* 8 */
                           "mincros"         /* 9 */
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
         { 13, "Undefined adjectiv:"},
         { 14, "Missing start/end of interval."},
         { 15, "No value for variable:"},
         { 16, "Too many values in a record (max 255)."},
         { 17, "Illegal jft-file-mode."},
         { 18, "Token to long (max 255 chars)."},
         { 19, "Penalty-matrix and more than one output-variable."},
         { 20, "Too many penalty-values (max 64)."},
         { 21, "No values in first data-line."},
         {401, "jfp cannot insert this type of statement"},
         {402, "Statement to large."},
         {403, "Not enogh free memory to insert statement"},
         {504, "Syntax error in jfrd-statement:"},
         {505, "Too many variables in statement (max 64)."},
         {506, "Undefined variable:"},
         {519, "Too many words in statement (max 255)."},
         {520, "No 'extern jfrd'-statement in program"},
         {522, "'No default'-output, but default not defined."},
         {550, "No min/max domain-values for variable:"},
         {551, "No adjectives bound to:"},
         {552, "Option 'a' or 'v' has to be specified for:"},
         {553, "Too many adjectives (max 24) for 'i'-option to:"},
         {554, "'i'-option without 'a'-option for variable:"},
         {555, "No legal option for variable:"},
         {556, "No Input-variables."},
         {557, "'R'-option, but no fuzzy relations."},
         {560, "Only one adjective bound to:"},
         {561, "No data or 100% data in early-stop set"},
         {9999, "Unknown error!"},
     };

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
  { fprintf(jfea_stdout, "WARNING %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    m = 0;
  }
  else
  { if (eno != 0)
      fprintf(jfea_stdout, "*** error %d: %s %s\n",
                          eno, jfr_err_texts[e].text, name);
    if (mode == JFE_FATAL)
    { if (eno != 0)
        fprintf(jfea_stdout, "\n*** PROGRAM ABORTED! ***\n");
      jfg_free();
      jfp_free();
      if (jfr_head != NULL)
        jfr_close(jfr_head);
      jfea_free();
      if (jfea_data != NULL)
        free(jfea_data);
      jfr_free();
      if (jfea_stdout != stdout)
        fclose(jfea_stdout);
      if (eno != 0)
      { if (jfea_batch == 0)
        { printf("Press RETURN ....");
          fgets(sout_fname, 78, stdin);
        }
        exit(1);
      }
    }
    m = -1;
  }
  return m;
}

/************************************************************************/
/* Functions for reading in from a file                                 */
/* Funktioner til indlaesning fra fil                                   */
/************************************************************************/

static int jfea_data_get(int mode)
{
  int slut, m;
  long adr = 0;
  float confs[256];
  char txt[540];

  slut = 0;
  jfea_data_c = 0;
  while (slut == 0)
  { if (mode == 0)
      slut = jft_getrecord(NULL, NULL, NULL, NULL);
    else
    { adr = jfea_data_c * jfea_laes_c;
      slut = jft_getrecord(&(jfea_data[adr]), confs,
                           &(jfea_data[adr + jfea_ivar_c]), NULL);
    }
    if (slut != 11)
    { if (slut != 0)
      { if (mode == 0)
        { sprintf(txt, " %s in file: %s line %d.",
                  jft_error_desc.carg, da_fname, jft_error_desc.line_no);
          jf_error(jft_error_desc.error_no, txt , JFE_ERROR);
        }
        slut = 0;
      }
      if (mode == 1)
      { for (m = 0; m < jfea_laes_c; m++)
        { if (confs[m] == 0.0)
            jfea_data[adr + m] = jfea_undefined;
        }
        if (jfea_early_stop == 1)
          jfea_data[adr + m] = 0.0;
      }
      if (slut == 0)
        jfea_data_c++;
    }
  }
  if (mode == 0)
    jfea_undefined = jft_dset_desc.max_value + 1.0;
  return slut;
}


/*************************************************************************/
/* Generelle hjaelpe-funktioner                                          */
/*************************************************************************/


void jfea_d_get(long data_no)
{
  long ad;
  int m;

  ad = data_no * ((long) jfea_laes_c);
  for (m = 0; m < jfea_ivar_c; m++)
  { jfea_ivalues[m] = jfea_data[ad];
    ad++;
    if (jfea_ivalues[m] != jfea_undefined)
      jfea_confidences[m] = 1.0;
    else
    { jfea_ivalues[m] = jfea_def_values[m];
      if (jfea_uv_mode == 0)
        jfea_confidences[m] = -3.0;
      else
      if (jfea_uv_mode == 1)
        jfea_confidences[m] = -1.0;
      else
      if (jfea_uv_mode == 2)
        jfea_confidences[m] = -2.0;
      else
        jfea_confidences[m] = 0.0;
    }
  }
  for (m = 0; m < jfea_ovar_c; m++)
  {
    jfea_expected[m] = jfea_data[ad];
    ad++;
  }
  jfea_cross_validation = 0;
  if (jfea_early_stop == 1)
  { if (jfea_data[ad] == 1.0)
      jfea_cross_validation = 1;
  }
}

void jfea_p_end(int mode)
{
  if (jfea_head.repro_c >= jfea_head.ind_c)
  { fprintf(jfea_stdout, "\nImproving stopped! Stop-condition: ");
    if (mode == 0)
      fprintf(jfea_stdout, " result\n");
    else
    if (mode == 1)
      fprintf(jfea_stdout, " time\n");
    else
      fprintf(jfea_stdout, " individuals\n");
  }
}

int jfea_test(void)
{
  int res;
  long ut;

  res = 0;
  if (jfea_early_stop == 0 && jfea_head.best_score <= jfea_maxerr)
  { jfea_stop();
    jfea_p_end(0);
  }
  if (jfea_early_stop == 1 && jfea_head.best_cv_score <= jfea_maxerr)
  { jfea_stop();
    jfea_p_end(0);
  }
  jfea_cur_time = time(NULL);
  if (jfea_maxtime > 0 && (jfea_cur_time - jfea_start_time) / 60 >= jfea_maxtime)
  { jfea_stop();
    jfea_p_end(1);
  }
  if (jfea_end_ind > 0 && jfea_head.repro_c >= jfea_end_ind)
  { jfea_stop();
    jfea_p_end(2);
  }
  if (jfea_silent == 0 &&
      (jfea_head.repro_c % 100 == 0 || jfea_head.repro_c == jfea_head.ind_c))
  { if (jfea_t_mode > 20)
    { fprintf(jfea_stdout, "\n%s\n", jfea_t_head);
      jfea_t_mode = 0;
    }
    fprintf(jfea_stdout, "  %4ld ", (long) jfea_head.repro_c);
    fprintf(jfea_stdout, " %10.4f ", jfea_head.best_score);
    if (jfea_head.ind_c < jfea_head.repro_c)
      fprintf(jfea_stdout, " %10.4f ", jfea_head.sum_score / ((float) jfea_head.ind_c));
    else
    if (jfea_head.repro_c > 0)
      fprintf(jfea_stdout, " %10.4f ", jfea_head.sum_score / ((float) jfea_head.repro_c));

    fprintf(jfea_stdout, " %10.4f ", jfea_stat.median_score);
    ut = jfea_cur_time - jfea_start_time;
    if (ut != 0)
    { fprintf(jfea_stdout, "   %3ld:", (long) ut / 60);
      fprintf(jfea_stdout, "%2ld", (long) ut % 60);
    }
    fprintf(jfea_stdout, "\n");
    jfea_t_mode++;
  }
  return res;
}

int jfea_cv(long data_no)
{
  int res;
  long adr;

  res = 0;
  if (jfea_early_stop == 1)
  { adr = data_no * jfea_laes_c + jfea_laes_c - 1;
    if (jfea_data[adr] == 1.0)
      res = 1;
  }
  return res;
}

float jfea_d_judge(long data_no)
{
  float r, dist;
  int m;

  jfea_d_get(data_no);

  jfr_arun(jfea_ovalues, jfr_head, jfea_ivalues, jfea_confidences,
           NULL, NULL, NULL);

  dist = 0.0;
  if (jfea_err_mode == 3)
    dist = jft_penalty_calc(jfea_ovalues[0], jfea_expected[0]);
  else
  { for (m = 0; m < jfea_ovar_c; m++)
    { r = jfea_expected[m] - jfea_ovalues[m];
      if (jfea_err_mode == 2)
        r = r * r;
      if (r < 0)
        r = -r;
      dist += r;
    }
    if (jfea_err_round == 1)
    { if (dist > 1.0)
        dist = 1.0;
    }
  }
  return dist;
}

float this_judge(void)
{
  long d, cvc;
  float avg_err, cv_avg_err, dist, worst;
  int slut;

  avg_err = 0.0;
  cv_avg_err = 0.0; cvc = 0;
  worst = jfea_stat.worst_score;
  if (jfea_err_mode == 0)
    worst *= ((float) jfea_data_c);
  slut = 0;
  for (d = 0; slut == 0 && d < jfea_data_c; d++)
  { dist = jfea_d_judge(d);
    if (jfea_cv(d) == 1)
    { cv_avg_err += dist;
      cvc++;
    }
    else
    { avg_err += dist;
      if (jfea_score_method == 0 &&
          avg_err > worst && jfea_head.repro_c > jfea_head.ind_c)
        slut = 1;  /* stop judging, becaus worse than worst individ */
    }
  }

  if (jfea_err_mode == 0)
  { avg_err /= ((float) (jfea_data_c - cvc));
    if (jfea_early_stop == 1)
      cv_avg_err /= ((float) cvc);
  }
  jfea_head.sum_score -= jfea_stat.old_score;
  jfea_head.sum_score += avg_err;
  if (avg_err < jfea_head.best_score || jfea_head.repro_c == 0)
  { if (jfea_silent == 0)
    { fprintf(jfea_stdout, "\nNew best. Repro_no: %ld",
                          jfea_head.repro_c);
      fprintf(jfea_stdout, " score:%10.4f. Created by %s\n",
      avg_err, jfea_t_methods[jfea_stat.method]);
      fprintf(jfea_stdout, "         parent scores: %12.4f, %12.4f\n",
                          jfea_stat.p1_score, jfea_stat.p2_score);
      jfea_t_mode += 4;
    }
    jfea_head.best_score = avg_err;
  }
  if (jfea_head.repro_c == 0 && jfea_early_stop == 1)
  { jfea_head.best_cv_score = cv_avg_err;
    jfea_head.cv_ind_id = jfea_protect();
  }
  jfea_head.new_cv_score = cv_avg_err;
  if (jfea_early_stop == 1)
  { if (jfea_head.new_cv_score < jfea_head.best_cv_score)
    { jfea_head.best_cv_score = jfea_head.new_cv_score;
      jfea_un_protect(jfea_head.cv_ind_id);
      jfea_head.cv_ind_id = jfea_protect();
      if (jfea_silent == 0)
      { fprintf(jfea_stdout, "\nNew best CROS_VAL. Repro_no: %ld",
                            jfea_head.repro_c);
        fprintf(jfea_stdout, " score:%10.4f. Created by %s\n\n",
                             cv_avg_err, jfea_t_methods[jfea_stat.method]);
        jfea_t_mode += 2;
      }
    }
  }

  jfea_head.repro_c++;
  jfea_test();
  return avg_err;
}


static int jfea_set_early_stop(int val_pct)
{
  long count, c, adr, i;

  count = (jfea_data_c * val_pct) / 100;
  if (count == 0 || count == jfea_data_c)
    return -1;
  c = 0;
  fprintf(jfea_stdout,
   "  Use %ld training-sets for training, %ld sets for cross-validation.\n",
          jfea_data_c - count, count);
  while (c < count)
  { i = jfea_random(jfea_data_c);
    adr = jfea_laes_c * i + jfea_laes_c - 1;
    if (jfea_data[adr] == 0.0)
    { jfea_data[adr] = 1.0;
      c++;
    }
  }
  return 0;
}

static int us_error(const char *argument)         /* usage-error. Fejl i kald af jfs */
{
  jfscmd_fprint_wrapped(stdout, jfscmd_num_of_columns() - 7, "usage: ", "       ", usage);
  printf("Error in: %s\n", argument);
  if (jfea_batch == 0)
  { printf("Press RETURN ....");
    fgets(sout_fname, 78, stdin);
  }
  return 1;
}

static void wait_if_needed()
{
  if (jfea_batch == 0)
  { printf("Press RETURN...");
    fgets(jfea_field_sep, 78, stdin);
  }
}

int main(int argc, const char *argv[])
{
  int m, val_pct;
  time_t t;
  long size;
  int rule_c;
  int fixed_rules;
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
  int no_default;
  struct jfg_var_desc vdesc;

  srand((unsigned) time(&t));
  jfea_data = NULL;
  jfea_head.repro_c = 0;
  jfea_head.best_score = 100000.0;
  jfea_head.sum_score = 0.0;
  jfea_head.ind_c = 40;
  rule_c = 0;
  fixed_rules = 0;
  no_default = 0;
  val_pct = 0;

  if (argc == 1)
  {
    jfscmd_print_about(about);
    return 0;
  }
  if (argc == 2)
  { if (jfscmd_getoption(jf_options, argv, 1, argc) == 13)
    {
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
      return us_error(argv[m]);
    m++;
    switch (option_no)
    { case 0:              /* -f */
        strcpy(jfea_field_sep, argv[m]);
        m++;
        break;
      case 1:           /* -s */
       jfea_silent = 1;
       break;
      case 2:              /* -D */
        jfea_fmode = jfscmd_tmap_find(jf_im_texts, argv[m]);
        if (jfea_fmode == -1)
          return us_error(argv[m]);
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
        jfea_maxtime = atoi(argv[m]);
        m++;
        break;
      case 6:             /* -Ee */
        jfea_maxerr = atof(argv[m]);
        m++;
        break;
      case 7:           /* -I  */
        jfea_head.ind_c = atoi(argv[m]) ;
        m++;
        break;
      case 8:           /* -Ei */
        jfea_end_ind = atol(argv[m]);
        m++;
        break;
      case 9:           /* -sm */
        if (strcmp(argv[m], sm_t_a) == 0)
          jfea_err_mode = 0;
        else
        if (strcmp(argv[m], sm_t_s) == 0)
          jfea_err_mode = 1;
        else
        if (strcmp(argv[m], sm_t_s2) == 0)
          jfea_err_mode = 2;
        else
        if (strcmp(argv[m], sm_t_p) == 0)
          jfea_err_mode = 3;
        else
          return us_error(argv[m]);
        m++;
        break;
      case 10:         /* -r */
        jfea_err_round = 1;
        break;
      case 11:         /* -so */
        strcpy(sout_fname, argv[m]);
        m++;
        break;
      case 12:         /* -a */
        jfea_append = 1;
        break;
      case 13:         /* -w */
        jfea_batch = 0;
        break;
      case 14:         /* -sc */
        if (strcmp(argv[m], "f") == 0)
          jfea_score_method = 0;
        else
        if (strcmp(argv[m], "e") == 0)
          jfea_score_method = 1;
        else
          return us_error(argv[m]);
        m++;
        break;
      case 15:        /* -uv */
        if (strcmp(argv[m], "z") == 0)
          jfea_uv_mode = 0;
        else
        if (strcmp(argv[m], "o") == 0)
          jfea_uv_mode = 1;
        else
        if (strcmp(argv[m], "a") == 0)
          jfea_uv_mode = 2;
        else
        if (strcmp(argv[m], "d0") == 0)
          jfea_uv_mode = 3;
        else
        if (strcmp(argv[m], "d") == 0)
          jfea_uv_mode = 4;
        else
          return us_error(argv[m]);
        m++;
        break;
      case 16:  /* -R */
        rule_c = atoi(argv[m]);
        m++;
        break;
      case 17: /* -fr */
        fixed_rules = 1;
        break;
      case 18: /* -nd */
        no_default = 1;
        break;
      case 19: /* -Es */
        jfea_early_stop = 1;
        val_pct = atoi(argv[m]);
        m++;
        if (val_pct <= 0 || val_pct >= 100)
          return us_error(argv[m]);
        break;
      case 20: /* -pm */
        strcpy(pm_fname, argv[m]);
        jfea_err_mode = 3;
        m++;
        break;
      default:
        return us_error(argv[m]);
    }
  }  /* for  */

  jfea_stdout = stdout;
  if (strlen(sout_fname) != 0)
  { if (jfea_append == 0)
      jfea_stdout = fopen(sout_fname, "w");
    else
      jfea_stdout = fopen(sout_fname, "a");
    if (jfea_stdout == NULL)
    { jfea_stdout = stdout;
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
    jf_error(m, jfea_t_stack, JFE_FATAL);
  m = jfg_init(0, 256, 4);
  if (m != 0)
    jf_error(m, jfea_t_stack, JFE_FATAL);
  m = jfp_init(2048);
  if (m != 0)
    jf_error(m, jfea_t_stat, JFE_FATAL);
  m = jfr_aload(&jfr_head, ip_fname, 2000);
  if (m != 0)
    jf_error(m, ip_fname, JFE_FATAL);

  fprintf(jfea_stdout, "  Program: %s loaded.\n\n", ip_fname);
  jft_init(jfr_head);
  for (m = 0; m < ((int) strlen(jfea_field_sep)); m++)
    jft_char_type(jfea_field_sep[m], JFT_T_SPACE);

  if (jfea_err_mode == 3)
  { if (strlen(pm_fname) == 0)
    { strcpy(pm_fname, ip_fname);
      jfscmd_ext_subst(pm_fname, extensions[2], 1);
    }
    m = jft_penalty_read(pm_fname);
    if (m == -1)
      jf_error(jft_error_desc.error_no, jft_error_desc.carg, JFE_FATAL);
  }

  m = jft_fopen(da_fname, jfea_fmode, 0);
  if (m != 0)
  { jft_close();
    return jf_error(jft_error_desc.error_no, da_fname, JFE_FATAL);
  }

  /* Find no of data-sets, read data-sets */
  jfg_sprg(&spdesc, jfr_head);
  jfea_ivar_c = spdesc.ivar_c;
  jfea_ovar_c = spdesc.ovar_c;
  jfea_laes_c = jfea_ivar_c + jfea_ovar_c + jfea_early_stop;

  fprintf(jfea_stdout, "  Data loader started..\n");
  jfea_data_get(0);
  jft_rewind();
  size = (jfea_data_c + 1) * (long) jfea_laes_c * (long) sizeof(float);
  if ((jfea_data = (float *) malloc(size)) == NULL)
    return jf_error(6, jfea_t_data, JFE_FATAL);
  jfea_f_data = (float *) jfea_data;
  jfea_data_get(1);
  fprintf(jfea_stdout,
          "  Data loader finished. %ld training-sets loaded from: %s.\n",
          jfea_data_c, da_fname);
  jft_close();

  if (jfea_early_stop == 1)
  { m = jfea_set_early_stop(val_pct);
    if (m != 0)
      return jf_error(561, " ", JFE_FATAL);
  }

  /* read default-values for input-variables (used in undefined input-values)*/
  for (m = 0; m < jfea_ivar_c; m++)
  { jfg_var(&vdesc, jfr_head, spdesc.f_ivar_no + m);
    jfea_def_values[m] = vdesc.default_val;
  }

  if ((m = jfea_init(jfr_head, jfea_head.ind_c,
                     rule_c, fixed_rules, no_default)) != 0)
    return jf_error(jfea_error_desc.error_no,
                    jfea_error_desc.argument, JFE_FATAL);

  jfea_start_time = time(NULL);
  fprintf(jfea_stdout, "  Training started.\n");

  jfea_run(this_judge, 0);

  fprintf(jfea_stdout, "  Score best individual: %10.3f\n", jfea_head.best_score);
  if (jfea_early_stop == 1)
    fprintf(jfea_stdout, "  Score best cros-indidvidual: %10.2f\n",
            jfea_head.best_cv_score);
  fprintf(jfea_stdout, "  Number off created individuals: %ld\n\n",
                      jfea_head.repro_c);

  if (jfea_stdout == stdout)
    printf("\a");     /* bell */

  if (jfea_early_stop == 1)
    jfea_ind2jfr(jfea_head.cv_ind_id);

  jfp_save(op_fname, jfr_head);

  fprintf(jfea_stdout, "  Changed program written to: %s\n\n", op_fname);
  if (jfea_batch == 0)
  { printf("Press RETURN ....");
    fgets(sout_fname, 78,stdin);
  }
  jf_error(0, jfea_empty, JFE_FATAL);

  return 0;
}

