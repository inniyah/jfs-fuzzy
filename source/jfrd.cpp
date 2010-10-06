  /************************************************************************/
  /*                                                                      */
  /* jfrd.cpp   Version 2.03    Copyright (c)  1998-2000 Jan E. Mortensen */
  /*                                                                      */
  /* JFS Rule-discover program.                                           */
  /*                                                                      */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                   */
  /*    Lollandsvej 35 3.tv.                                              */
  /*    DK-2000 Frederiksberg                                             */
  /*    Denmark                                                           */
  /*                                                                      */
  /************************************************************************/

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
#include "jfrd_lib.h"


//---------------------------------------------------------------------------
#ifdef __BCPLUSPLUS
USEUNIT("..\..\COMMON\jfs_text.cpp");
USEUNIT("..\..\COMMON\jft_lib.cpp");
USEUNIT("..\..\COMMON\jfp_lib.cpp");
USEUNIT("..\..\COMMON\jfr_lib.cpp");
USEUNIT("..\..\COMMON\jfrd_lib.cpp");
USEUNIT("..\..\COMMON\jfg_lib.cpp");
//---------------------------------------------------------------------------
#pragma argsused
#endif

const char usage_1[] =
"usage: jfrd [-D dm] [-d df] [-f fs] [-o of] [-Mp pb] [-S] [-so s]";
const char usage_2[] =
"            [-Md db] [-Mt t] [-r m] [-iw wgt] [-b] [-c m] [-e] {-a] [-w] jfrf";
struct jf_option_desc { const char *option;
                        int argc;      /* -1: variabelt */
                      };               /* -2: sidste argument */

struct jf_option_desc jf_options[] =
  {    { "-f",  1},        /*  0 */
       { "-s",  0},        /*  1 */
       { "-D",  1},        /*  2 */
       { "-d",  1},        /*  3 */
       { "-o",  1},        /*  4 */
       { "-Mp", 1},        /*  5 */
       { "-Md", 1},        /*  6 */
       { "-r",  1},        /*  7 */
       { "-iw", 1},        /*  8 */
       { "-b",  0},        /*  9 */
       { "-Mt", 1},        /* 10 */
       { "-c",  1},        /* 11 */
       { "-S",  0},        /* 12 */
       { "-e",  0},        /* 13 */
       { "-w",  0},        /* 14 */
       { "-so", 1},        /* 15 */
       { "-a",  0},        /* 16 */
       { "-?",  0},        /* 17 */
       { "?",   0},        /* 18 */
       { " ",  -2}
     };

struct jf_tmap_desc {
	int value;
	const char *text;
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

static int isoption(const char *s);
static int jf_getoption(const char *argv[], int no, int argc);
static int jf_tmap_find(struct jf_tmap_desc *map, const char *txt);
static void ext_subst(char *d, const char *e, int forced);
static int jf_about(void);
static int us_error(void);

int isoption(const char *s)
{
  if (s[0] == '-' || s[0] == '?')
    return 1;
  return 0;
}

static int jf_getoption(const char *argv[], int no, int argc)
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

static int jf_tmap_find(struct jf_tmap_desc *map, const char *txt)
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

static int us_error(void)         /* usage-error. Fejl i kald af jfs */
{
  printf("\n%s\n%s\n", usage_1, usage_2);
  return 1;
}

static int jf_about(void)
{
  printf("\n\nJFRD   version 2.03  Copyright (c) 1998-2000 Jan E. Mortensen\n\n");

  printf("usage: jfrd [options] jfrf\n\n");

  printf(
"JFRD replaces the statement:\n");
  printf(
"'extern jfrd input {[<op>] <vname>} output [<op>] <vname>' in the\n");
  printf(
"jfs-program <jfrf> with rules generated from a data file.\n\n");
  printf("OPTIONS\n");

  printf(
"-f <fs>    : <fs> field-separator.     -iw <wgt>  : 'ifw &<w>' statements.\n");
  printf(
"-Mp <pb>   : Alloc <pb> K to program.  -Md <db>   : Alloc <db> K to rules.\n");
  printf(
"-so <s>    : Redirect stdout to <s>.   -a         : Append stdout to <s>.\n");
  printf(
"-w         : Wait for return.          -S         : Reduction in entered order.\n");
  printf("-d <df>    : Read data from the file <df>.\n");
  printf(
"-D <d>     : Data order. <d>={i|e|t}|f. i:input,e:expected,t:text,f:firstline.\n");
  printf("-o <of>    : Write the changed jfs-program to the file <of>.\n");
  printf("-Mt <mt>   : <mt> is Maximum number of minutes used in rewind-reduction.\n");
  printf(
"-r <rm>    : reduce-mode. d:default,n:none,a:all,b:between,i:in,ib:inbetween.\n");
  printf("-b         : Case-reduction.\n");
  printf("-c <cr>    : Conflict-resolve: <cr>=s:score, <cr>=c:count.\n");
  printf("-e         : Remove rules with default output value.\n");
  return 0;
}

int main(int argc, const char *argv[])
{
  int m;

  const char *extensions[]  = {
                          "jfr",     /* 0 */
                          "dat"      /* 1 */
                        };

  int option_no;

  char field_sep[256];
  char da_fname[256] = "";
  char ip_fname[256] = "";
  char op_fname[256] = "";
  char sout_fname[256] = "";
  int silent = 0;
  int data_mode = JFT_FM_INPUT_EXPECTED;
  long prog_size = 30000;
  long data_size = 30000;
  int red_mode = -1;
  int red_weight = 0;
  float weight_value = 0.5;
  int red_case = 0;
  int max_time = 60;
  int res_confl_mode = 0;
  int red_order = 0;
  int def_fzvar = -1;
  int append_mode = 0;
  int batch = 1;

  field_sep[0] = '\0';


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
        strcpy(field_sep, argv[m]);
        m++;
        break;
      case 1:              /* -s */
        silent = 1;
        break;
      case 2:              /* -D */
        data_mode = jf_tmap_find(jf_im_texts, argv[m]);
        if (data_mode == -1)
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
        ext_subst(op_fname, extensions[0], 1);
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
      case 7:            /* -r */
        if (strcmp(argv[m], "n") == 0)
          red_mode = 0;
        else
        if (strcmp(argv[m], "a") == 0)
          red_mode = 1;
        else
        if (strcmp(argv[m], "b") == 0)
          red_mode = 2;
        else
        if (strcmp(argv[m], "i") == 0)
          red_mode = 3;
        else
        if (strcmp(argv[m], "ib") == 0)
          red_mode = 4;
        else
        if (strcmp(argv[m], "d") == 0)
          red_mode = -1;
        else
          return us_error();
        m++;
        break;
      case 8:            /* -iw */
        red_weight = 1;
        weight_value = atof(argv[m]);
        if (weight_value < 0.0 || weight_value > 1.0)
          return us_error();
        m++;
        break;
      case 9:            /* -b */
        red_case   = 1;
        break;
      case 10:           /* -Mt */
        max_time = atoi(argv[m]);
        m++;
        break;
      case 11:           /* -c  */
        if (strcmp(argv[m], "s") == 0)
          res_confl_mode = 0;
        else
        if (strcmp(argv[m], "c") == 0)
          res_confl_mode = 1;
        else
          return us_error();
        m++;
        break;
      case 12:           /* -S */
        red_order = 1;
        break;
      case 13:           /* -e */
        def_fzvar = 0;
        break;
      case 14:            /* -w */
        batch = 0;
        break;
      case 15:            /* -so */
        strcpy(sout_fname, argv[m]);
        m++;
        break;
      case 16:            /* -a */
        append_mode = 1;
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
  m = jfrd_run(op_fname, ip_fname, sout_fname, da_fname, data_mode, field_sep,
               prog_size, data_size, max_time, red_mode, red_weight, weight_value,
               red_case, res_confl_mode, red_order, def_fzvar,
               append_mode, silent, batch);

  return m;
}

