  /*************************************************************************/
  /*                                                                       */
  /* jffam.c - JFS Fam-creation by a cellular automat                      */
  /*                             Copyright (c) 1998-2000 Jan E. Mortensen  */
  /*                                       Copyright (c) 2010 Miriam Ruiz  */
  /*                                                                       */
  /*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cmds_lib.h"
#include "jft_lib.h"
#include "jffamlib.h"

static const char usage[] =
	"jffam [-D dm] [-d df] [-f fs] [-o of] [-Mp ps] [-Md ds]"
	" [-rf df] [-iw wgt] [-c m] [-r ru] [-ms s] [-nf]"
	" [-w] [-a] [-so sf] [-tt tm] <file.jfr>";

struct jfscmd_option_desc jf_options[] = {
	{"-f",  1},        /*  0 */
	{"-s",  0},        /*  1 */
	{"-D",  1},        /*  2 */
	{"-d", -1},        /*  3 */
	{"-o",  1},        /*  4 */
	{"-Mp", 1},        /*  5 */
	{"-Md", 1},        /*  6 */
	{"-rf", 1},        /*  7 */
	{"-iw", -1},       /*  8 */
	{"-c",  1},        /*  9 */
	{"-r",  1},        /* 10 */
	{"-ms", 1},        /* 11 */
	{"-nf", 0},        /* 12 */
	{"-a",  0},        /* 13 */
	{"-so", 1},        /* 14 */
	{"-w",  0},        /* 15 */
	{"-tt", 1},        /* 16 */
	{"-?",  0},        /* 17 */
	{"?",   0},        /* 18 */
	{" ",  -2}
};

int data_mode = JFT_FM_INPUT_EXPECTED;

int fixed     = 1; /* 1: input-rules is fixed. */

long data_size = 5000;
long prog_size = 20000;

char field_sep[256];   /* 0: brug space, tab etc som felt-seperator, */
                     			    /* andet: kun field_sep er feltsepator.       */

int res_confl = 0;   /* Conflict resolution-mode.                    */
                     			  /* 0: score,                                    */
                     			  /* 1: count.                                    */

int ca_rule    = 0;  /* celular automat rule:                        */
                     			  /* 0: avg,                                      */
                     			  /* 1: avg(min,max),                             */
                     			  /* 2: avg(delta).                               */

float weight_val = 0.0;   /* if > 0.0 if-statmts of the form 'ifw %<wgt>' */

int max_steps = 100; /* maximalt antal opdateringer cel-automat     */

int batch = 1;
int append = 0;

int then_type = 0;  /* then-part of generated rules is:                   */
                    /*    0: 'then <var> is <adj>;',                      */
                    /*    1: 'then <var> = %<adj.center>;'                */

char empty[] = " ";

char t_score[] = "s";
char t_count[] = "c";
char t_avg[]   = "a";
char t_minmax[]= "m";
char t_delta[] = "d";

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

static int us_error(void);

static const char *about[] = {
  "usage: jffam [options] <file.jfr>",
  "",
  "JFFAM replaces the statement: 'extern jfrd input {[<op>] <vname>} output [<op>] <vname>' "
    "in the jfs-program <file.jfr> with rules (a FAM) generated from a data file (by a CA).",
  "",
  "Options:",
  "-d <df>    : Data from file <df>.       -so <of>  : Redirect stdout to <of>.",
  "-a         : append output.             -w        : Wait for return.",
  "-Mp <pb>   : Alloc <pb> K to program.   -Md <db>  : Alloc <db> K to rules.",
  "-f <fs>    : Use <fs> as field-separator.",
  "-D <d>     : Data order. <d>={i|e|t}|f. i:input,e:expected,t:text,f:firstline.",
  "-o <of>    : Write the changed jfs-program to the file <of>.",
  "-rf <rf>   : Write the generated rules to the data-file <rf>.",
  "-iw <wgt>  : Generate 'ifw <wgt> ...' statements.",
  "-c <cr>    : Conflict-resolve: <cr>=s:score, <cr>=c:count.",
  "-r <ru>    : Rule used by CA. <ru>='a':avg, 'm':minmax, 'd':delta.",
  "-ms <ms>   : <ms> is maximum number of steps in cellular automat.",
  "-nf        : No fixed rules in cellular automat.",
  "-tt <t>    : then-type: <t>='a':adjectiv, 'c':center.",
  NULL
};

static int us_error(void)         /* usage-error. Fejl i kald af jfs */
{
  jfscmd_fprint_wrapped(stdout, jfscmd_num_of_columns() - 7, "usage: ", "       ", usage);
  return 1;
}

int main(int argc, const char *argv[])
{
  int m, res;

  char da_fname[256] = "";
  char ip_fname[256] = "";
  char op_fname[256] = "";
  char ru_fname[256] = "";
  char so_fname[256] = "";

  const char *extensions[]  = {
                                "jfr",     /* 0 */
                                "dat"      /* 1 */
  };
  int option_no;

  field_sep[0] = '\0';

  if (argc == 1)
  {
    jfscmd_print_about(about);
    return 0;
  }
  strcpy(ip_fname, argv[argc - 1]);
  jfscmd_ext_subst(ip_fname, extensions[0], 0);
  for (m = 1; m < argc - 1; )
  {
    option_no = jfscmd_getoption(jf_options, argv, m, argc - 1);
    if (option_no == -1)
      return us_error();
    m++;
    switch (option_no)
    { case 0:              /* -f */
        strcpy(field_sep, argv[m]);
        m++;
        break;
      case 1:              /* -s */
       	break;
      case 2:              /* -D */
        data_mode = jfscmd_tmap_find(jf_im_texts, argv[m]);
        if (data_mode == -1)
          return us_error();
        m++;
        break;
      case 3:            /* -d */
        if (m < argc - 1 && jfscmd_isoption(argv[m]) == 0)
        { strcpy(da_fname, argv[m]);
          jfscmd_ext_subst(da_fname, extensions[1], 0);
          m++;
        }
        else
        { strcpy(da_fname, ip_fname);
          jfscmd_ext_subst(da_fname, extensions[1], 1);
        }
        break;
      case 4:            /* -o */
        strcpy(op_fname, argv[m]);
        jfscmd_ext_subst(op_fname, extensions[0], 0);
        m++;
        break;
      case 5:             /* -Mp */
        prog_size = 1024L * atol(argv[m]);
        m++;
        break;
      case 6:            /* -Md */
        data_size = 1024L * atol(argv[m]);
        m++;
        break;
      case 7:            /* -rf */
        strcpy(ru_fname, argv[m]);
        jfscmd_ext_subst(ru_fname, extensions[1], 0);
        m++;
        break;
      case 8:            /* -iw */
        weight_val = atof(argv[m]);
        if (weight_val <= 0.0 || weight_val > 1.0)
          return us_error();
        m++;
        break;
      case 9:           /* -c  */
        if (strcmp(argv[m], t_score) == 0)
          res_confl = 0;
        else
        if (strcmp(argv[m], t_count) == 0)
          res_confl = 1;
        else
          return us_error();
        m++;
        break;
      case 10:          /* -r */
        if (strcmp(argv[m], t_avg) == 0)
          ca_rule = 0;
        else
        if (strcmp(argv[m], t_minmax) == 0)
          ca_rule = 1;
        else
        if (strcmp(argv[m], t_delta) == 0)
          ca_rule = 2;
        else
          return us_error();
        m++;
        break;
      case 11:        /* -ms */
        max_steps = atoi(argv[m]);
        m++;
        if (m <= 0)
          return us_error();
        break;
      case 12:       /* -nf */
        fixed = 0;
        break;
      case 13:       /* -a */
        append = 1;
        break;
      case 14:       /* -so */
        strcpy(so_fname, argv[m]);
        m++;
        break;
      case 15:       /* -w */
        batch = 0;
        break;
      case 16:
        if (strcmp(argv[m], "a") == 0)
          then_type = 0;
        else
        if (strcmp(argv[m], "c") == 0)
          then_type = 1;
        else
          return us_error();
        m++;
        break;
      case 17:
      case 18:
        jfscmd_print_about(about);
        return 0;
      default:
       	return us_error();
    }
  }  /* for  */

  if (strlen(op_fname) == 0 && strlen(ru_fname) == 0)
  { strcpy(op_fname, ip_fname);
    jfscmd_ext_subst(op_fname, extensions[0], 1);
  }
  if (strlen(da_fname) == 0)
  { strcpy(da_fname, ip_fname);
    jfscmd_ext_subst(da_fname, extensions[1], 1);
  }
  res = jffam_run(op_fname, ip_fname, ru_fname, da_fname,
                  field_sep, data_mode, prog_size, data_size,
                  res_confl, ca_rule, weight_val, then_type,
                  fixed, max_steps,
                  so_fname, append, batch);

  return res;
}

