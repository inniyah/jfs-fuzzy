  /**************************************************************************/
  /*                                                                        */
  /* jfid3.cpp    Version  2.03  Copyright (c) 1998-2000 Jan E. Mortensen   */
  /*                                                                        */
  /* JFS Rule discover using ID3.                                           */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/

#ifdef __BCPLUSPLUS__
  #pragma hdrstop
  #include <condefs.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jft_lib.h"
#include "jfid3lib.h"

#ifdef __BCPLUSPLUS__
//---------------------------------------------------------------------------
USEUNIT("..\..\COMMON\jfs_text.cpp");
USEUNIT("..\..\COMMON\jft_lib.cpp");
USEUNIT("..\..\COMMON\jfid3lib.cpp");
USEUNIT("..\..\COMMON\jfp_lib.cpp");
USEUNIT("..\..\COMMON\jfr_lib.cpp");
USEUNIT("..\..\COMMON\jfg_lib.cpp");
//---------------------------------------------------------------------------
#pragma argsused
#endif

char usage_1[] =
"usage: jfid3 [-D dm] [-d df] [-f fs] [-o of] [-a] [-w] [-ms s] ";
char usage_2[] =
"             [-Mp pb] [-Md db] [-c m] [-h hf] [-hm m] [-so sf]    jfrf";

struct jf_option_desc { char *option;
                        int argc;      /* -1: variabelt */
                      };               /* -2: sidste argument */

struct jf_option_desc jf_options[] =
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

struct jf_tmap_desc { int value;
                       char *text;
                     };
struct jf_tmap_desc jf_im_texts[] =        /* input-modes */
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

static int isoption(char *s);
static int jf_getoption(char *argv[], int no, int argc);
static int jf_tmap_find(struct jf_tmap_desc *map, char *txt);
static void ext_subst(char *d, char *e, int forced);
static int jf_about(void);
static int us_error(void);



static int isoption(char *s)
{
  if (s[0] == '-' || s[0] == '?')
    return 1;
  return 0;
}

static int jf_getoption(char *argv[], int no, int argc)
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

static int jf_tmap_find(struct jf_tmap_desc *map, char *txt)
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

static void ext_subst(char *d, char *e, int forced)
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

static int jf_about(void)
{
  printf(
 "\n\nJFID3   version 2.03    Copyright (c) 1998-2000 Jan E. Mortensen\n\n");

  printf("usage: jfid3 [options] jfrf\n\n");

  printf(
"JFID3 replaces the statement:\n");
  printf(
"'extern jfrd input {[<op>] <vname>} output [<op>] <vname>' in\n");
  printf(
"the jfs-program <jfrf> with a fuzzy decision-tree generated from a data file.\n\n");
  printf("OPTIONS\n");

  printf("-f <fs>    : Use <fs> as field-separator in data file.\n");
  printf(
"-D <d>     : Data order. <d>={i|e|t}|f. i:input,e:expected,t:text,f:firstline.\n");
  printf("-d <df>    : Read data from the file <df>.\n");
  printf("-o <of>    : Write the changed jfs-program to the file <of>.\n");
  printf("-Mp <pb>   : Allocate extra <pb> K to program.\n");
  printf("-Md <db>   : Allocate <db> K to rules.\n");
  printf("-c <cr>    : Conflict-resolve: <cr>=s:score, <cr>=c:count.\n");
  printf("-so <s>    : Redirect stdout to the file <s>.\n");
  printf("-h <hf>    : Write history-info to the file <hf>.\n");
  printf(
"-hm <m>    : History-mode. <m> build from: d:dataset, r:rules, R:ext.rules.\n");
  printf("-ms <s>    : Remove rules with rule-score < <s>.\n");
  printf("-a         : Append stdout to file specified in -so.\n");
  printf("-w         : Wait for RETURN.\n");
  return 0;
}

static int us_error(void)         /* usage-error. Fejl i kald af jfs */
{
  printf("\n%s\n\%s\n", usage_1, usage_2);
  return 1;
}

int main(int argc, char *argv[])
{
  int m, i, res;

  char *extensions[]  = { "jfr",     /* 0 */
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
    return jf_about();
  strcpy(ip_fname, argv[argc - 1]);
  ext_subst(ip_fname, extensions[0], 0);
  for (m = 1; m < argc - 1; )
  { option_no = jf_getoption(argv, m, argc - 1);
    if (option_no == -1)
      return us_error();
    m++;
    switch (option_no)
    { case 0:              /* -f */
        strcpy(jfrd_field_sep, argv[m]);
        m++;
        break;
      case 2:              /* -D */
        f_mode = jf_tmap_find(jf_im_texts, argv[m]);
        if (f_mode == -1)
          return us_error();
        m++;
        break;
      case 3:            /* -d */
        strcpy(da_fname, argv[m]);
        ext_subst(da_fname, extensions[1], 0);
        m++;
        break;
      case 4:            /* -o */
        strcpy(op_fname, argv[m]);
        ext_subst(op_fname, extensions[0], 0);
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
    ext_subst(op_fname, extensions[0], 1);
  }
  if (strlen(da_fname) == 0)
  { strcpy(da_fname, ip_fname);
    ext_subst(da_fname, extensions[1], 1);
  }
  if (strlen(h_fname) != 0 && h_dsets == 0 && h_rules == 0)
  { h_dsets = 1;
    h_rules = 1;
  }
  if (strlen(h_fname) == 0)
  { strcpy(h_fname, op_fname);
    ext_subst(h_fname, extensions[2], 1);
  }
  res = jfid3_run(op_fname, ip_fname, da_fname, jfrd_field_sep,
                  f_mode, jfrd_prog_size, jfrd_data_size,
                  jfrd_res_confl,
                  h_fname, h_dsets, h_rules, min_score,
                  sout_fname, append, batch);

  return res;
}


