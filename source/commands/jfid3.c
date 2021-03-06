  /*************************************************************************/
  /*                                                                       */
  /* jfid3.c - JFS Rule discover using ID3                                 */
  /*                             Copyright (c) 1998-2000 Jan E. Mortensen  */
  /*                                       Copyright (c) 2010 Miriam Ruiz  */
  /*                                                                       */
  /*************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "cmds_lib.h"
#include <math.h>
#include "jft_lib.h"
#include "jfid3lib.h"

static const char usage[] =
	"jfid3 [-D dm] [-d df] [-f fs] [-o of] [-a] [-w] [-ms s]"
	" [-Mp pb] [-Md db] [-c m] [-h hf] [-hm m] [-so sf] <file.jfr>";

static const char *about[] = {
  "usage: jfid3 [options] <file.jfr>",
  "",
  "JFID3 replaces the statement: 'extern jfrd input {[<op>] <vname>} output [<op>] <vname>' "
    "in the jfs-program <file.jfr> with a fuzzy decision-tree generated from a data file.",
  "",
  "Options:",
  "-f <fs>    : Use <fs> as field-separator in data file.",
  "-D <d>     : Data order. <d>={i|e|t}|f. i:input,e:expected,t:text,f:firstline.",
  "-d <df>    : Read data from the file <df>.",
  "-o <of>    : Write the changed jfs-program to the file <of>.",
  "-Mp <pb>   : Allocate extra <pb> K to program.",
  "-Md <db>   : Allocate <db> K to rules.",
  "-c <cr>    : Conflict-resolve: <cr>=s:score, <cr>=c:count.",
  "-so <s>    : Redirect stdout to the file <s>.",
  "-h <hf>    : Write history-info to the file <hf>.",
  "-hm <m>    : History-mode. <m> build from: d:dataset, r:rules, R:ext.rules.",
  "-ms <s>    : Remove rules with rule-score < <s>.",
  "-a         : Append stdout to file specified in -so.",
  "-w         : Wait for RETURN.",
  NULL
};

struct jfscmd_option_desc jf_options[] =
  {   {  "-f",  1},        /*  0 */
      {  "-s",  0},        /*  1 */     /* UD! */
      {  "-D",  1},        /*  2 */
      {  "-d",  1},        /*  3 */
      {  "-o",  1},        /*  4 */
      {  "-Mp", 1},        /*  5 */
      {  "-Md", 1},        /*  6 */
      {  "-c",  1},        /*  7 */
      {  "-r",  0},        /*  8 */    /* UD! */
      {  "-a",  0},        /*  9 */
      {  "-so", 1},        /* 10 */
      {  "-w",  0},        /* 11 */
      {  "-h",  1},        /* 12 */
      {  "-hm", 1},        /* 13 */
      {  "-ms", 1},        /* 14 */
      {  "-?",  0},        /* 15 */
      {  "?",   0},        /* 16 */
      {  " ",  -2}
  };

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

static int us_error(void)         /* usage-error. Fejl i kald af jfs */
{
  jfscmd_fprint_wrapped(stdout, jfscmd_num_of_columns() - 7, "usage: ", "       ", usage);
  return 1;
}

int main(int argc, const char *argv[])
{
  int m, i, res;

  const char *extensions[]  = {
                          "jfr",     /* 0 */
                          "dat",     /* 1 */
                          "txt"      /* 2 */
                        };

  int option_no;
  char da_fname[256] = "";
  char ip_fname[256] = "";
  char op_fname[256] = "";
  char sout_fname[256] = "";
  char h_fname[256] = "";
  int append = 0;
  int batch = 1;
  int f_mode = JFT_FM_INPUT_EXPECTED;
  long jfrd_data_size = 50000;
  long jfrd_prog_size = 20000;
  char jfrd_field_sep[256];   /* 0: brug space, tab etc som felt-seperator, */
                              /* andet: kun field_sep er feltsepator.       */
  int jfrd_res_confl = 0;   /* Conflict resolution-mode.                    */
                            /* 0: score,                                    */
                            /* 1: count.                                    */
  int h_dsets = 0;
  int h_rules = 0;
  float min_score = -1.0;

  jfrd_field_sep[0] = '\0';

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
        strcpy(jfrd_field_sep, argv[m]);
        m++;
        break;
      case 2:              /* -D */
        f_mode = jfscmd_tmap_find(jf_im_texts, argv[m]);
        if (f_mode == -1)
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
      case 5:             /* -Mp */
        jfrd_prog_size = 1024L * atol(argv[m]);
        m++;
        break;
      case 6:            /* -Md */
        jfrd_data_size = 1024L * atol(argv[m]);
        m++;
        break;
      case 7:           /* -c  */
        if (strcmp(argv[m], "s") == 0)
          jfrd_res_confl = 0;
        else
        if (strcmp(argv[m], "c") == 0)
          jfrd_res_confl = 1;
        else
          return us_error();
        m++;
        break;
      case 9:          /* -a */
        append = 1;
        break;
      case 10:         /* -so */
        strcpy(sout_fname, argv[m]);
        m++;
        break;
      case 11:         /* -w  */
        batch = 0;
        break;
      case 12:         /* -h */
        strcpy(h_fname, argv[m]);
        m++;
        break;
      case 13:         /* -hm */
        for (i = 0; argv[m][i] != '\0'; i++)
        { if (argv[m][i] == 'd')
            h_dsets = 1;
          else
          if (argv[m][i] == 'r')
            h_rules = 1;
          else
          if (argv[m][i] == 'R')
            h_rules = 2;
          else
            return us_error();
        }
        m++;
        break;
      case 14:     /* -r */
        min_score = atof(argv[m]);
        m++;
        break;
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
  if (strlen(h_fname) != 0 && h_dsets == 0 && h_rules == 0)
  { h_dsets = 1;
    h_rules = 1;
  }
  if (strlen(h_fname) == 0)
  { strcpy(h_fname, op_fname);
    jfscmd_ext_subst(h_fname, extensions[2], 1);
  }
  res = jfid3_run(op_fname, ip_fname, da_fname, jfrd_field_sep,
                  f_mode, jfrd_prog_size, jfrd_data_size,
                  jfrd_res_confl,
                  h_fname, h_dsets, h_rules, min_score,
                  sout_fname, append, batch);

  return res;
}


