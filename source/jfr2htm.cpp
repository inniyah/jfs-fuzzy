  /***************************************************************************/
  /*                                                                         */
  /* jfr2htm.cpp  Version  2.04  Copyright (c) 1998, 1999 Jan E. Mortensen   */
  /*                                                                         */
  /* JFS Converter. Converts a compiled jfs-program to html                  */
  /* (Javascript).                                                           */
  /*                                                                         */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                      */
  /*    Lollandsvej 35 3.tv.                                                 */
  /*    DK-2000 Frederiksberg                                                */
  /*    Denmark                                                              */
  /*                                                                         */
  /***************************************************************************/

#ifdef __BCPLUSPLUS__
  #pragma hdrstop
  #include <condefs.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfr2hlib.h"

#ifdef __BCPLUSPLUS__
  //---------------------------------------------------------------------------
  USEUNIT("..\..\COMMON\jfs_text.cpp");
USEUNIT("..\..\COMMON\jfr2hlib.cpp");
USEUNIT("..\..\COMMON\jfr_lib.cpp");
USEUNIT("..\..\COMMON\jfg_lib.cpp");
//---------------------------------------------------------------------------
#pragma argsused
#endif


static FILE *sout;
static int jfr2htm_batch = 1;
const char usage_1[] =
"usage: jfr2htm [-g d] [-Ss s] [-St s] [-Sa c] [-j] [-Sh s]";
const char usage_2[] =
"               [-r] [-so sf] [-a] [-w] [-o of] [-ow] [-Nc] [-s]";
const char usage_3[] =
"               {-l m] [-Na] [-c] [-Cm m] [-Ct t] [-p f]         jfrf";
const char coptxt[] =
"JFR2HTM    version 2.04    Copyright (c) 1999-2000 Jan E. Mortensen";

struct jf_option_desc { const char *option;
                        int argc;  /* -1: variabelt,     */
                                   /* -2: last argument. */
                     };

struct jf_option_desc jf_options[] =
{
    {"-r",  0},    /*  0 */
    {"-ow", 0},    /*  1 */
    {"-Sa", 1},    /*  2 */
    {"-St", 1},    /*  3 */
    {"-Ss", 1},    /*  4 */
    {"-so", 1},    /*  5 */
    {"-a",  0},    /*  6 */
    {"-g",  1},    /*  7 */
    {"-s",  0},    /*  8 */
    {"-j",  0},    /*  9 */
    {"-w",  0},    /* 10 */
    {"-o",  1},    /* 11 */
    {"-Nc", 0},    /* 12 */
    {"-Sh", 1},    /* 13 */
    {"-Na", 0},    /* 14 */
    {"-l",  1},    /* 15 */
    {"-c",  0},    /* 16 */
    {"-Cm", 1},    /* 17 */
    {"-Ct", 1},    /* 18 */
    {"-p",  1},    /* 19 */
    {"-?",  0},    /* 20 */
    {"?",   0},    /* 21 */
    {" ",  -2}
};

static int isoption(const char *s);
static int us_error(void);
static int jf_about(void);
int jf_getoption(const char *argv[], int no, int argc);
static void ext_subst(char *d, const char *e, int forced);
int filkopier(char *de_fname, char *so_fname);


/*************************************************************************/
/* Hjaelpe-funktioner                                                    */
/*************************************************************************/


static int isoption(const char *s)
{
  if (s[0] == '-' || s[0] == '?')
    return 1;
  return 0;
}

static int us_error(void)         /* usage-error. Fejl i kald af jfs */
{
  printf("\n%s\n%s\n%s\n", usage_1, usage_2, usage_3);
  return 1;
}

static int jf_about(void)
{
  char txt[80];

  txt[0] = '\0';
  printf("\n%s\n\n", coptxt);
  printf("usage: jfr2htm [options] jfrf\n\n");

  printf(
"JFR2HTM is a JFS converter. It converts the compiled jfs-program <jfrf>.jfr\n");
  printf("to a html-file, with the program converted to Javascript.\n\n");
  printf("OPTIONS:\n");

  printf(
"-o <o>    : Write HTML file to <o>.    -j      : Write JavaScript to <o>.js.\n");
  printf(
"-c         : write program comment.    -ow     : Overwrite HTML file.\n");
  printf(
"-g <dec>   : <dec> is precision.       -Nc     : Don't check numbers.\n");
  printf(
"-a         : Append output.            -w      : Wait for return.\n");
  printf(
"-s         : Silent (to stdout).       -so <s> : Redirect stdout to <s>.\n");
  printf(
"-Cm <m>    : <m> is max-confidence.    -Ct <t> : <t> is confidence text.\n");
  printf(
"-Na        : Ignore var-arguments.     -p <p>  : <p> is object prefix.\n");
  printf(
"-l <m>     : Label-placement: <m>='l':left, 'a':above, 't':table,\n");
  printf(
"             'ab':above, extra blank line, 'mt': multicolumn table.\n");
  printf(
"-Sh <ss>   : Include reference to the stylesheet <ss> in HTML-file.\n");
  printf(
"-r         : Convert only program-part (not input/output-form).\n");
  printf(
"-Sa <size> : Max number of characters in statement (default 512).\n");
  printf(
"-St <size> : Max number of nodes in conversion tree (def 128).\n");
  printf(
"-Ss <size> : Max stacksize conversion-stack (default 64).\n");

  if (jfr2htm_batch == 0)
  { printf("Press RETURN ....");
    fgets(txt, 78, stdin);
  }

  return 0;
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

int filkopier(char *de_fname, char *so_fname)
{
  int c;
  FILE *jfi_op;
  FILE *jfi_ip;

  if ((jfi_ip = fopen(so_fname, "rb")) == NULL)
  { return 1;  /* overwrite */
  }
  if ((jfi_op = fopen(de_fname, "wb")) == NULL)
  { fprintf(sout, "cannot open the file: %s for writing\n", de_fname);
    fclose(jfi_ip);
    exit(0);
  }
  while ((c = getc(jfi_ip)) != EOF)
    putc(c, jfi_op);
  fclose(jfi_op);
  fclose(jfi_ip);
  return 0;
}

int main(int argc, const char *argv[])
{
  int m, res, mode, option_no;
  int maxtext, maxtree, maxstack, overwrite, js_file;
  int digits, no_check, silent, use_args;
  int append_mode, label_mode, main_comment;
  float conf_max;
  char so_fname[256] = "";
  char de_fname[256] = "";
  char js_fname[256] = "";
  char rm_fname[256] = "";
  char sout_fname[256] = "";
  char ssheet[256] = "";
  char conf_txt[256] = "Conf.";
  char prefix_txt[256] = "";
  char txt[80];
  const char *extensions[]  = {
                          "jfr",     /* 0 */
                          "htm",     /* 1 */
                          "js",      /* 2 */
                          "old",     /* 3 */
                        };

  silent = 0;
  sout = stdout;
  overwrite = 0;
  mode     = 0;
  maxtext  = 512;
  maxtree  = 128;
  maxstack = 64;
  digits = 5;
  js_file  = 0;
  append_mode = 0;
  jfr2htm_batch = 1;
  no_check = 0;
  use_args = 1;
  label_mode = JFR2HTM_LM_LEFT;
  main_comment = 0;
  conf_max = 1.0;

  if (argc == 1)
    return jf_about();
  if (argc == 2)
  { if (jf_getoption(argv, 1, argc) == 10)  /* -w */
    { jfr2htm_batch = 0;
      return jf_about();
    }
  }

  strcpy(so_fname, argv[argc - 1]);
  ext_subst(so_fname, extensions[0], 1);

  for (m = 1; m < argc - 1; )
  { option_no = jf_getoption(argv, m, argc);
    if (option_no == -1)
      return us_error();
    else
    { m++;
      switch (option_no)
      { case 0:              /* -r  */
          mode = 1;
          break;
        case 1:              /* -ow */
          overwrite = 1;
          break;
        case 2:          /* -Sa */
          maxtext = atoi(argv[m]);
          if (maxtext <= 0)
            return us_error();
          m++;
          break;
        case 3:          /* -St */
          maxtree = atoi(argv[m]);
          if (maxtree <= 0)
            return us_error();
          m++;
          break;
        case 4:          /* -Ss */
          maxstack = atoi(argv[m]);
          if (maxstack <= 0)
            return us_error();
          m++;
          break;
        case 5:          /* -so */
          strcpy(sout_fname, argv[m]);
          m++;
          break;
        case 6:          /* -a */
          append_mode = 1;
          break;
        case 7:          /* - g */
          digits = atoi(argv[m]) + 1;
          m++;
          break;
        case 8:          /* -s */
          silent = 1;
          break;
        case 9:          /* -j */
          js_file = 1;
          break;
        case 10:         /* -w */
          jfr2htm_batch = 0;
          break;
        case 11:          /* -o */
          strcpy(de_fname, argv[m]);
          ext_subst(de_fname, extensions[1], 0);
          m++;
          break;
        case 12:         /* -Nc */
          no_check = 1;
          break;
        case 13:         /* -Sh */
          strcpy(ssheet, argv[m]);
          m++;
          break;
        case 14:         /* -Na */
          use_args = 0;
          break;
        case 15:         /* -l */
          if (strcmp(argv[m], "l") == 0)
            label_mode = JFR2HTM_LM_LEFT;
          else
          if (strcmp(argv[m], "a") == 0)
            label_mode = JFR2HTM_LM_ABOVE;
          else
          if (strcmp(argv[m], "ab") == 0)
            label_mode = JFR2HTM_LM_BLABOVE;
          else
          if (strcmp(argv[m], "t") == 0)
            label_mode = JFR2HTM_LM_TABLE;
          else
          if (strcmp(argv[m], "mt") == 0)
            label_mode = JFR2HTM_LM_MULTABLE;
          else
            return us_error();
          m++;
          break;
        case 16:       /* -c */
          main_comment = 1;
          break;
        case 17:       /* -Cm */
          conf_max = atof(argv[m]);
          if (conf_max <= 0.0)
            return us_error();
          m++;
          break;
        case 18:      /* -Ct */
          strcpy(conf_txt, argv[m]);
          m++;
          break;
        case 19:     /* -p */
          strcpy(prefix_txt, argv[m]);
          m++;
          break;
        default:          /* -?  */
          return jf_about();
          /* break; */
      }
    }
  }  /* for  */

  if (strlen(sout_fname) != 0)
  { if (append_mode == 0)
      sout = fopen(sout_fname, "w");
    else
      sout = fopen(sout_fname, "a");
    if (sout == NULL)
    { sout = stdout;
      printf("Cannot open %s for writing.\n", sout_fname);
    }
  }
  if (silent == 0)
    fprintf(sout, "\n%s\n", coptxt);

  if (strlen(so_fname) == 0)
    return us_error();
  if (strlen(de_fname) == 0)
  { strcpy(de_fname, so_fname);
    ext_subst(de_fname, extensions[1], 1);
  }
  if (js_file == 1)
  { strcpy(js_fname, de_fname);
    ext_subst(js_fname, extensions[2], 1);
  }

  strcpy(rm_fname, de_fname);
  ext_subst(rm_fname, extensions[3], 1);


  if (silent == 0)
    fprintf(sout, "\nconverting: %s\n", so_fname);

  if (overwrite == 0)
    overwrite = filkopier(rm_fname, de_fname);

  res = jfr2h_conv(de_fname, js_fname,
                   so_fname, rm_fname,
                   mode, digits, overwrite, js_file,
                   maxtext, maxtree, maxstack, no_check,
                   ssheet, use_args, label_mode, main_comment,
                   conf_txt, conf_max, prefix_txt,
                   sout);

  if (overwrite == 0 && res != 0)
  { filkopier(de_fname, rm_fname);
    fprintf(sout, "Errors in conversion. Original HTML file recovered.\n");
  }

  if (silent == 0 && res == 0)
    fprintf(sout, "\nsucces. Converted file written to: %s\n\n", de_fname);

  if (jfr2htm_batch == 0)
  { printf("Press RETURN ...");
    fgets(txt, 78, stdin);
  }
  if (sout != stdout)
    fclose(sout);
  return res;
}



