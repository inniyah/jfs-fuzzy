  /*************************************************************************/
  /*                                                                       */
  /* jfgp.c - JFS rule discover-functions using Genetic programing         */
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
#include "jfgp_lib.h"

#define JFE_WARNING 0
#define JFE_ERROR   1
#define JFE_FATAL   2

static const char usage[] =
	"jfgp [-D dm] [-d df] [-o of] [-Et min] [-Ei ind] [-pm pm]"
	" [-so so] [-a] [-w] [-mm m] [-f fs] [-r] [-sc m]"
	" [-I ind] [-A atm] [-s] [-ml lev] [-sm s] [-gs s] <file.jfr>";

static const char *about[] = {
  "usage: jfgp [options] <file.jfr>",
  "",
  "JFGP modifies the jfr-program <file.jfr>. It replaces statements of the type:"
    "'extern jfgp {<arg>} <dest>;' with if/case/return-statements.",
  "<arg>::= fzvars {<v>} | vars {<v>} | functions {<f>} | float <p> | integer <i> <a> | arrays {<a>}",
  "<dest>::= then {<v>} | assign {<v>} | case | return.",
  "",
  "Options:",
  "-f <fs> : <fs> field-separator.   -gs <s> : tournament group-size.",
  "-I <c>  : population size.        -A <a>  : number of atoms.",
  "-s      : Silent.                 -r      : Score rounding to [0,1].",
  "-so <s> : Write stdout to <s>.    -a      : Append stdout to <s>.",
  "-Et <t> : Stop after <t> minutes. -Ei <i> : Stop after <i> individuals.",
  "-w      : wait for RETURN.        -d <df> : Read data from the file <df>.",
  "-sc <m> : calc f:fast, e:exact.   -pm <mf>: Read penalty-matrix from <mf>.",
  "-o <of> : Write changed program to <of>.",
  "-sm <s> : Score-method. 'a':avg,'s':sum,'s2':sum(sqr),'p':penalty-matrix.",
  "-D <d>  : Data order. <d>={i|e|t}|f. i:input,e:expect,t:text,f:first-line.",
  "-mm <m> : size-minimize. <m>='n':no, <m>='h':from halfway, <m>='y':yes.",
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
        {"-sm", 1},        /*  6 */
        {"-I",  1},        /*  7 */
        {"-A",  1},        /*  8 */
        {"-so", 1},        /*  9 */
        {"-r",  0},        /* 10 */
        {"-Ei", 1},        /* 11 */
        {"-mm", 1},        /* 12 */
        {"-m",  0},        /* 13 */
        {"-rc", 1},        /* 14 */
        {"-a",  0},        /* 15 */
        {"-w",  0},        /* 16 */
        {"-gs", 1},        /* 17 */
        {"-sc", 1},        /* 18 */
        {"-pm", 1},        /* 19 */
        {"-uv", 1},        /* 20 */

        {"-?",  0},        /* 21 */
        {"?",   0},        /* 22 */
        {" ",  -2}
      };

struct jfgp_hoved_desc
{
   long         ind_c;
   long         atom_c;

   float        best_score;
   signed short best_count;
   long         repro_c;
   int          init;
};

struct jfgp_hoved_desc jfgp_hoved;

struct jfg_sprog_desc jfgp_spdesc;

/************************************************************************/
/* Traenings-data-set                                                   */
/************************************************************************/

float *jfgp_data;
/*                   dataene:  inputs, expecteds, weight, cval, wexp.   */

long  jfgp_data_c;      /* antal data-set.                              */


float *jfgp_f_data;     /* [data_c * (ivars + ovars + 1 (wexp)) ]  */

unsigned char *gl_pc;

int jfgp_ivar_c;
int jfgp_ovar_c;
int jfgp_laes_c;

#define JFGP_VMAX 256
float jfgp_ivalues[JFGP_VMAX];
float jfgp_ovalues[JFGP_VMAX];
float jfgp_expected[JFGP_VMAX];
float jfgp_confidences[JFGP_VMAX];
float jfgp_def_values[JFGP_VMAX];

float jfgp_ind_score;
float jfgp_undefined = 0.0;

/************************************************************************/
/* Afslutningstest/Statistik information                                */
/************************************************************************/


time_t jfgp_start_time;
time_t jfgp_cur_time;       /* Til HEAD (samlet tid ??)  */

int jfgp_t_mode = 1000; /* tekst-mode */

char jfgp_t_head[] =
"  ind_no  best-score  avg-score  free-atoms  active-inds   used time";

char da_fname[256] = "";
char ip_fname[256] = "";
char op_fname[256] = "";
char sout_fname[256] = "";
char pm_fname[256] = "";    /* penalty matrix */

FILE *sout = NULL;

int jfgp_fmode = JFT_FM_INPUT_EXPECTED;

int jfgp_ss_mode = 0; /* score-method                                 */
                      /* 0:  ss=avg(|op-exp|),                        */
                      /* 1:  ss=sum(|op-exp|).                        */
                      /* 3:  penalty-matrix.                          */
int jfgp_s2_mode = 0; /* 1: score = (op-exp)**2                      */

#define SM_FAST   0
#define SM_EXACT  1
int score_method = SM_FAST;

long  jfgp_end_ind   = 10000;
int   jfgp_silent    = 0;

int   jfgp_maxtime   = 60;

int   jfgp_err_round = 0;     /* 0: no rounding                          */
                              /* 1: round to [0,1].                       */

/* int   jfgp_wgt_mode  = 0;  */  /* 1: insert 'ifw %<val>'-statements.       */
/*float jfgp_wgt_value = 0.5; */

char jfgp_empty[] = " ";
char jfgp_space[80];


int jfgp_min_size = 2;      /* 0: dont minimize,                         */
                            /* 2: minimize from halfways,                */
                            /* 1: minize ind-size from start.            */
int jfgp_r_min_size;

char jfgp_field_sep[256];   /* 0: brug space, tab etc som felt-seperator, */
                            /* andet: kun field_sep er feltsepator.       */


int jfgp_append_mode = 0;

int jfgp_batch_mode = 1;

int jfgp_uv_mode = 4;   /* if unknown variable:   */
                        /* 0: undef, fzvars=zero, */
                        /* 1: undef, fzvars=1.0,  */
                        /* 2: undef, fzvars=1/ant,*/
                        /* 3: default, conf=0.0,  */
                        /* 4: default, conf=1.0.  */

/**************************************************************************/
/* Program-data                                                           */
/**************************************************************************/

void *jfr_head = NULL;

/*************************************************************************/
/* DIverse                                                               */
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

char jfgp_t_data[] = "data";
char jfgp_t_atomd[] = "converting-table";
char jfgp_t_atoms[] = "Atoms";
char jfgp_t_husksaet[] = "level-data";
char jfgp_t_individs[] = "individuals";
char jfgp_t_score[] = "score-tables";
char jfgp_t_stack[]  = "jfg-stack";
char jfgp_t_stat[]   = "jfp-statement";
/*char jfgp_t_jfgp[]   = "jfgp"; */
char jfgp_t_end[]    = "end";

struct jfr_err_desc {
	int eno;
	const char *text;
};

struct jfr_err_desc jfr_err_texts[] =
     { {  0, " "},
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
       {901, "No 'extern jfgp'-statement"},
       {902, "Unknown name in 'extern jfgp'-statement."},
       {903, "Illegal integer argument in 'extern jfgp'-statement."},
       {904, "Syntax error in 'extern jfgp'-statement."},
       {905, "Illegal argument to 'float' in 'extern jfgp'-statement."},
       {906, "No variables or fzvars in 'extern jfgp'-statement."},
       {907, "Cannot start with exception-level when levels = 1."},
       {908, "No adjectives bound to then-variable:"},
       {909, "Not enough atoms to create initial population"},
       {910, "No fuzzy variables bound to variable:"},
      { 9999, "Unknown error!"},
     };

static void jf_luk(void);
static int jf_error(int eno, char *name, int mode);
static int jfgp_data_get(int mode);
int this_compare(float score_1, int count_1,
                        float score_2, int count_2);
static void jfgp_d_get(long data_no);
static float jfgp_d_judge(long data_no);
float this_judge(void);
static void jfgp_p_end(int mode);
static int jfgp_test(void);

static void jf_luk(void)
{
  if (jfr_head != NULL)
    jfr_close(jfr_head);
  if (jfgp_data != NULL)
    free(jfgp_data);
  jfp_free();
  jfg_free();
  jfr_free();
  if (jfgp_hoved.init == 1)
    jfgp_free();
  if (sout != stdout)
    fclose(sout);
}

static int jf_error(int eno, char *name, int mode)
{
  int m, v, e;
  char dummy[80];

  e = -1;
  for (v = 0; e == -1; v++)
  { if (jfr_err_texts[v].eno == eno
        || jfr_err_texts[v].eno == 9999)
      e = v;
  }
  if (mode == JFE_WARNING)
  { fprintf(sout, "WARNING %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    m = 0;
  }
  else
  { if (eno != 0)
      fprintf(sout, "*** error %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    if (mode == JFE_FATAL)
    { fprintf(sout, "\n*** PROGRAM ABORTED! ***\n");
      jf_luk();
      if (jfgp_batch_mode == 0)
      { printf("Press RETURN to continue..");
        fgets(dummy, 78, stdin);
      }
      exit(eno);
    }
    m = -1;
  }
  return m;
}

/************************************************************************/
/* Funktioner til indlaesning fra fil                                   */
/************************************************************************/

static int jfgp_data_get(int mode)
{
  int slut, m;
  long adr = 0;
  float confs[256];
  char txt[256];

  slut = 0;
  jfgp_data_c = 0;
  while (slut == 0)
  { if (mode == 0)
      slut = jft_getrecord(NULL, NULL, NULL, NULL);
    else
    { adr = jfgp_data_c * jfgp_laes_c;
      slut = jft_getrecord(&(jfgp_data[adr]), confs,
                           &(jfgp_data[adr + jfgp_ivar_c]), NULL);
    }
    if (slut != 11)
    { if (slut != 0)
      { sprintf(txt, " In file: %s line %d.", da_fname, jft_error_desc.line_no);
        jf_error(jft_error_desc.error_no, txt, JFE_ERROR);
        slut = 0;
      }
      if (mode == 1)
      { for (m = 0; m < jfgp_laes_c; m++)
        { if (confs[m] == 0.0)
            jfgp_data[adr + m] = jfgp_undefined;
        }
      }
      if (slut == 0)
        jfgp_data_c++;
    }
  }
  if (mode == 0)
    jfgp_undefined = jft_dset_desc.max_value + 1.0;
  return slut;
}


/************************************************************************/
/* Judge-functions                                                      */
/************************************************************************/

int this_compare(float score_1, int count_1,
                 float score_2, int count_2)
{
  int res;

  res = 0;
  if (score_1 < score_2)
    res = 1;
  else
  if (score_1 == score_2)
  { if (jfgp_r_min_size == 0)
      res = rand() % 2;
    else
    { if (count_1 < count_2)
        res = 1;
      else
      if (count_1 > count_2)
        res = -1;
    }
  }
  else
    res = -1;
  if (res == 0)
    res = 1;
  return res;
}

static void jfgp_d_get(long data_no)
{
  long ad;
  int m;

  ad = data_no * jfgp_laes_c;
  for (m = 0; m < jfgp_ivar_c; m++)
  { jfgp_ivalues[m] = jfgp_data[ad];
    if (jfgp_ivalues[m] != jfgp_undefined)
      jfgp_confidences[m] = 1.0;
    else
    { jfgp_ivalues[m] = jfgp_def_values[m];
      if (jfgp_uv_mode == 0)
        jfgp_confidences[m] = -3.0;
      else
      if (jfgp_uv_mode == 1)
        jfgp_confidences[m] = -1.0;
      else
      if (jfgp_uv_mode == 2)
        jfgp_confidences[m] = -2.0;
      else
        jfgp_confidences[m] = 0.0;
    }
    ad++;
  }
  for (m = 0; m < jfgp_ovar_c; m++)
  { jfgp_expected[m] = jfgp_data[ad];
    ad++;
  }
}

static float jfgp_d_judge(long data_no)
{
  float dist, d;
  int m;

  jfgp_d_get(data_no);
  jfr_arun(jfgp_ovalues, jfr_head, jfgp_ivalues, jfgp_confidences,
           NULL, NULL, NULL);
  dist = 0.0;
  if (jfgp_ss_mode == 3)  /* penalty-matrix */
    dist = jft_penalty_calc(jfgp_ovalues[0], jfgp_expected[0]);
  else
  { for (m = 0; m < jfgp_ovar_c; m++)
    { d = fabs(jfgp_expected[m] - jfgp_ovalues[m]);
      if (jfgp_s2_mode == 1)
        d *= d;
      dist += d;
    }
    if (jfgp_err_round == 1)
    { if (dist > 1.0)
        dist = 1.0;
    }
  }
  return dist;
}


float this_judge(void)
{
  long d;
  float res = 0, sdist = 0, dist = 0, nbscore = 0;
  int slut;

  if (jfgp_ss_mode == 0)  /* avg */
    nbscore = jfgp_stat.old_score * ((float) jfgp_data_c);
  else if (jfgp_ss_mode == 1 || jfgp_ss_mode == 3)  /* sum */
    nbscore = jfgp_stat.old_score;
  slut = 0;
  for (d = 0; slut == 0 && d < jfgp_data_c; d++)
  { dist = jfgp_d_judge(d);
    if (d == 0)
      sdist = dist;
    else
      sdist += dist;
    if (score_method == SM_FAST && nbscore > 0.0 &&
        jfgp_hoved.repro_c > jfgp_hoved.ind_c && sdist > nbscore)
      slut = 1;
  }
  if (jfgp_ss_mode == 0)
    res = sdist / ((float) jfgp_data_c);
  else
    res = sdist;
  jfgp_ind_score = res;
  slut = jfgp_test();
  if (slut == 1)
    jfgp_stop();
  return res;
}

/****************************************************************************/
/* Hjaelpe-funktioner til jfl_load                                          */
/****************************************************************************/



static void jfgp_p_end(int mode)
{
  fprintf(sout, "\nImproving stopped! Stop-condition: ");
  if (mode == 0)
    fprintf(sout, " result\n");
  else
  if (mode == 1)
    fprintf(sout, " time\n");
  else
    fprintf(sout, " individuals\n");
  fprintf(sout, "  score best individual: %10.3f\n", jfgp_hoved.best_score);
  fprintf(sout, "  Number off created individuals: %ld\n\n", jfgp_hoved.repro_c);
  if (sout == stdout)
    printf("\a");      /* bell */
}

static int jfgp_test(void)
{
  int res;
  long ut;

  res = 0;
  jfgp_hoved.repro_c++;
  if (jfgp_hoved.repro_c == 1
      ||
      jfgp_hoved.best_score > jfgp_ind_score
      || (jfgp_hoved.best_score == jfgp_ind_score
          && jfgp_r_min_size == 1
          && jfgp_stat.ind_size < jfgp_hoved.best_count)
     )
  { jfgp_hoved.best_score = jfgp_ind_score;
    jfgp_hoved.best_count = jfgp_stat.ind_size;
    if (jfgp_silent == 0)
    { fprintf(sout, "\nNew best. Repro_no: %ld",
      jfgp_hoved.repro_c);
      fprintf(sout, " score:%10.4f ", jfgp_hoved.best_score);
      fprintf(sout, "size: %4d\n", jfgp_hoved.best_count);
      jfgp_t_mode += 2;
    }
  }
  jfgp_cur_time = time(NULL);
  if ((jfgp_cur_time - jfgp_start_time) / 60 >= jfgp_maxtime)
  { res = 1;
    jfgp_p_end(1);
  }
  if (jfgp_hoved.repro_c >= jfgp_end_ind)
  { res = 1;
    jfgp_p_end(2);
  }
  if (jfgp_r_min_size == 0 && jfgp_min_size == 2
      && (jfgp_hoved.repro_c * 2 > jfgp_end_ind
          || ((jfgp_cur_time - jfgp_start_time) / 60) * 2 >= jfgp_maxtime))
  {  jfgp_r_min_size = 1;
     if (jfgp_silent == 0)
     { fprintf(sout, "\n Now minimizing individ size\n");
       jfgp_t_mode += 3;
     }
  }

  if (jfgp_silent == 0 &&
      (jfgp_hoved.repro_c % 1000 == 0
       || jfgp_hoved.repro_c == jfgp_hoved.ind_c))
  { if (jfgp_t_mode > 20)
    { fprintf(sout, "\n%s\n", jfgp_t_head);
      jfgp_t_mode = 0;
    }
    fprintf(sout, "  %4ld ", (long) jfgp_hoved.repro_c);
    fprintf(sout, " %10.4f ", jfgp_hoved.best_score);
    fprintf(sout, " %10.4f ",
                         jfgp_stat.sum_score / ((float) jfgp_stat.alive_c));
    fprintf(sout, "  %4ld ", (long) jfgp_stat.free_c);
    fprintf(sout, "        %4ld ",  (long) jfgp_stat.alive_c);
    ut = jfgp_cur_time - jfgp_start_time;
    if (ut != 0)
    { fprintf(sout, "     %3ld:", (long) ut / 60);
      fprintf(sout, "%2ld\n", (long) ut % 60);
    }
    else
      fprintf(sout, "\n");
    jfgp_t_mode++;
  }
  return res;
}

static void wait_if_needed()
{
  char dummy[80];
  if (jfgp_batch_mode == 0)
  { printf("Press RETURN to continue..");
    fgets(dummy, 78, stdin);
  }
}

static int us_error(void)         /* usage-error. Fejl i kald af jfs */
{
  jfscmd_fprint_wrapped(stdout, jfscmd_num_of_columns() - 7, "usage: ", "       ", usage);
  wait_if_needed();
  return 1;
}

int main(int argc, const char *argv[])
{
  int m;
  long size;
  time_t t;
  const char *extensions[]  = {
                          "jfr",     /* 0 */
                          "dat",     /* 1 */
                          "pm"       /* 2 */
   };

  char sm_t_a[] = "a";
  char sm_t_s[] = "s";
  char sm_t_2[] = "s2";
  char sm_t_p[] = "p";
  struct jfg_var_desc vdesc;
  int option_no;
  int tournament_size = 5;

  sout = stdout;

  srand((unsigned) time(&t));
  jfgp_field_sep[0] = '\0';


  jfgp_hoved.ind_c = 100;
  jfgp_hoved.init = 0;
  jfgp_hoved.atom_c = 1000;

  jfgp_data = NULL;

  jfgp_silent = 0;

  if (argc == 1)
  {
    jfscmd_print_about(about);
    return 0;
  }
  if (argc == 2 && strcmp(argv[argc - 1], "-w") == 0)
  {
    jfgp_batch_mode = 0;
    jfscmd_print_about(about);
    wait_if_needed();
    return 0;
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
        strcpy(jfgp_field_sep, argv[m]);
        m++;
        break;
      case 1:           /* -s */
        jfgp_silent = 1;
        break;
      case 2:              /* -D */
        jfgp_fmode = jfscmd_tmap_find(jf_im_texts, argv[m]);
        if (jfgp_fmode == -1)
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
        jfgp_maxtime = atoi(argv[m]);
        m++;
        break;
      case 6:           /* -sm */
        if (strcmp(argv[m], sm_t_a) == 0)
          jfgp_ss_mode = 0;
        else
        if (strcmp(argv[m], sm_t_s) == 0)
          jfgp_ss_mode = 1;
        else
        if (strcmp(argv[m], sm_t_2) == 0)
        { jfgp_ss_mode = 1;
          jfgp_s2_mode = 1;
        }
        else
        if (strcmp(argv[m], sm_t_p) == 0)
          jfgp_ss_mode = 3;
        else
          return us_error();
        m++;
        break;
      case 7:            /* -I */
        jfgp_hoved.ind_c = atol(argv[m]);
        m++;
        break;
      case 8:           /* -A  */
        jfgp_hoved.atom_c = atol(argv[m]);
        m++;
        break;
      case 9:            /* -so */
        strcpy(sout_fname, argv[m]);
        m++;
        break;
      case 10:           /* -r */
        jfgp_err_round = 1;
        break;
      case 11:           /* -Ei */
        jfgp_end_ind = atol(argv[m]);
        m++;
        break;
      case 12:           /* -mm  */
        if (strcmp(argv[m], "n") == 0)
           jfgp_min_size = 0;
        else
        if (strcmp(argv[m], "h") == 0)
           jfgp_min_size = 2;
        else
        if (strcmp(argv[m], "y") == 0)
           jfgp_min_size = 1;
        else
          return us_error();
        m++;
        break;
      case 13:           /* -m  */
        jfgp_min_size = 1;
        break;
      case 14:           /* -rc */
        break;
      case 15:          /* -a */
        jfgp_append_mode = 1;
        break;
      case 16:           /* -w */
        jfgp_batch_mode = 0;
        break;
      case 17:            /* -gs */
        tournament_size = atoi(argv[m]);
        m++;
        break;
      case 18:            /* -sc */
        if (strcmp(argv[m], "e") == 0)
          score_method = SM_EXACT;
        else
        if (strcmp(argv[m], "f") == 0)
          score_method = SM_FAST;
        else
          return us_error();
        m++;
        break;
      case 19:           /* -pm */
        strcpy(pm_fname, argv[m]);
        jfgp_ss_mode = 3;
        m++;
        break;
      case 20:        /* -uv */
        if (strcmp(argv[m], "z") == 0)
          jfgp_uv_mode = 0;
        else
        if (strcmp(argv[m], "o") == 0)
          jfgp_uv_mode = 1;
        else
        if (strcmp(argv[m], "a") == 0)
          jfgp_uv_mode = 2;
        else
        if (strcmp(argv[m], "d0") == 0)
          jfgp_uv_mode = 3;
        else
        if (strcmp(argv[m], "d") == 0)
          jfgp_uv_mode = 4;
        else
          return us_error();
        m++;
        break;
      case 21:           /* ? */
      case 22:
        jfscmd_print_about(about);
        wait_if_needed();
        return 0;
      default:
        return us_error();
    }
  }  /* for  */

  if (strlen(op_fname) == 0)
  { strcpy(op_fname, ip_fname);
    jfscmd_ext_subst(op_fname, extensions[0], 1);
  }

  if (strlen(da_fname) == 0)
  { strcpy(da_fname, ip_fname);
    jfscmd_ext_subst(da_fname, extensions[1], 1);
  }
  if (strlen(sout_fname) != 0)
  { if (jfgp_append_mode == 0)
      sout = fopen(sout_fname, "w");
    else
      sout = fopen(sout_fname, "a");
    if (sout == NULL)
    { sout = stdout;
      printf("Cannot open %s for writing\n", sout_fname);
    }
  }

  m = jfg_init(JFG_PM_NORMAL, 200, 4);   /* Stacksize som parameter ? */
  if (m != 0)
    jf_error(m, jfgp_t_stack, JFE_FATAL);
  m = jfp_init(0);
  if (m != 0)
    jf_error(m, jfgp_t_stack, JFE_FATAL);
  m = jfr_init(2048);
  if (m != 0)
    jf_error(m, jfgp_t_stat, JFE_FATAL);
  m = jfr_aload(&jfr_head, ip_fname, 5000);
  if (m != 0)
    jf_error(m, ip_fname, JFE_FATAL);
  fprintf(sout, "jfs-program: %s loaded.\n\n", ip_fname);

  jft_init(jfr_head);
  for (m = 0; m < ((int) strlen(jfgp_field_sep)); m++)
    jft_char_type(jfgp_field_sep[m], JFT_T_SPACE);


  if (jfgp_ss_mode == 3)
  { if (strlen(pm_fname) == 0)
    { strcpy(pm_fname, ip_fname);
      jfscmd_ext_subst(pm_fname, extensions[2], 1);
    }
    if (jft_penalty_read(pm_fname) == -1)
      return jf_error(jft_error_desc.error_no, jft_error_desc.carg, JFE_FATAL);
  }

  jfg_sprg(&jfgp_spdesc, jfr_head);
  jfgp_ivar_c = jfgp_spdesc.ivar_c;
  jfgp_ovar_c = jfgp_spdesc.ovar_c;
  jfgp_laes_c = jfgp_ivar_c + jfgp_ovar_c + 1;

  /* Find antal data-s‘t, og indlaes data */
  m = jft_fopen(da_fname, jfgp_fmode, 0);
  if (m != 0)
    return jf_error(m, da_fname, JFE_FATAL);
  fprintf(sout, "Data loader started..\n");
  jfgp_data_get(0);
  jft_rewind();
  size = jfgp_data_c * (long) jfgp_laes_c * (long) sizeof(float);
  if ((jfgp_data = (float *) malloc(size)) == NULL)
    return jf_error(6, jfgp_t_data, JFE_FATAL);
  jfgp_f_data = (float *) jfgp_data;
  jfgp_data_get(1);
  fprintf(sout,
          "data loader finished. %ld training-sets loaded from: %s.\n\n",
          jfgp_data_c, da_fname);
  jft_close();

  /* read default-values for input-variables (used in undefined input-values)*/
  for (m = 0; m < jfgp_ivar_c; m++)
  { jfg_var(&vdesc, jfr_head, jfgp_spdesc.f_ivar_no + m);
    jfgp_def_values[m] = vdesc.default_val;
  }

  if ((m = jfgp_init(jfr_head, jfgp_hoved.atom_c, jfgp_hoved.ind_c)) != 0)
    return jf_error(m, jfgp_empty, JFE_FATAL);
  jfgp_hoved.init = 1;

  jfgp_hoved.repro_c = 0;
  jfgp_hoved.best_score = 0.0;
  if (jfgp_min_size == 2)
    jfgp_r_min_size = 0;
  else
    jfgp_r_min_size = jfgp_min_size;
  jfgp_start_time = time(NULL);
  fprintf(sout, "creating initial genereration\n");
  fprintf(sout, "number of data sets: %d\n", (int) jfgp_data_c);

  fprintf(sout, "Training started.\n");
  if ((m = jfgp_run(this_judge, this_compare, tournament_size)) != 0)
    return jf_error(m, jfgp_empty, JFE_FATAL);

  jfp_save(op_fname, jfr_head);
  fprintf(sout, "Changed program written to: %s\n\n", op_fname);

  if (jfgp_batch_mode == 0)
  { printf("Press RETURN to continue..");
    fgets(sout_fname, 78, stdin);
  }
  jf_luk();

  return 0;
}




