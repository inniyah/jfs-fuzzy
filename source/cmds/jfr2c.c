
  /************************************************************************/
  /*                                                                      */
  /* jfr2c.cpp   Version  2.05   Copyright (c) 1999-2001 Jan E. Mortensen */
  /*                                                                      */
  /* Program  to convert a compiled jfs-program to                        */
  /* C-sourcecode.                                                        */
  /*                                                                      */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                   */
  /*    Lollandsvej 35 3.tv.                                              */
  /*    DK-2000 Frederiksberg                                             */
  /*    Denmark                                                           */
  /*                                                                      */
  /************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfr2clib.h"

static FILE *sout;

char usage_1[] =
"usage: jfr2c [-g dg] [-Ss ss] [-St ts] [-om m] [-np] [-nr] [-nc]";
char usage_2[] =
"             [-ur] [-um] [-ui] [-n nm] [-so s] [-d] [-a] [-w] [-s] jfrf";

char coptxt[] =
"JFR2C  version 2.05  Copyright (c) 1999-2001 Jan E. Mortensen";

struct jf_option_desc { const char *option;
                        int argc;  /* -1: variabelt,     */
                                   /* -2: last argument. */
                      };

struct jf_option_desc jf_options[] = {
	{"-np", 0},    /*  0 */
	{"-o",  1},    /*  1 */
	{"-om", 1},    /*  2 */
	{"-St", 1},    /*  3 */
	{"-Ss", 1},    /*  4 */
	{"-so", 1},    /*  5 */
	{"-a",  0},    /*  6 */
	{"-g",  1},    /*  7 */
	{"-s",  0},    /*  8 */
	{"-w",  0},    /*  9 */
	{"-n",  1},    /* 10 */
	{"-nr", 0},    /* 11 */
	{"-ur", 0},    /* 12 */
	{"-um", 0},    /* 13 */
	{"-ui", 0},    /* 14 */
	{"-nc", 0},    /* 15 */
	{"-d",  0},    /* 16 */
	{"-?",  0},    /* 17 */
	{"?",   0},    /* 18 */
	{" ",  -2}
};

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
  printf("\n%s\n%s\n", usage_1, usage_2);
  return 1;
}



static int jf_about(void)
{

  printf("\n%s\n\n", coptxt);
  printf("usage: jfr2c [options] rf \n\n");
  printf(
"JFR2C is a JFS converter. It converts the compiled jfs-program <rf> to \n");
  printf("C-sourcecode.\n\n");
  printf("OPTIONS:\n");
  printf(
"-nr     : No argument rounding.    -np     : Use non-protected functions.\n");
  printf(
"-ur     : Use C's relations.       -um     : Sse C's min/max-functions.\n");
  printf(
"-s      : Silent.                  -so <s> : Redirect stdout to <s>.\n");
  printf(
"-a      : Append to stdout.        -w      : Wait for return.\n");
  printf(
"-ui     : Use inline-functions.\n");
  printf(
"-nc     : No confidence values in C-function.\n");
  printf(
"-o <of> : Write the C-sourcecode to <of>.c, <of>.h.\n");
  printf(
"-d      : Use 'double' variables.\n");
  printf(
"-n <nm> : The c-function gets the name <nm>.\n");
  printf(
"-g <p>  : Precision. <p> is the number of decimals.\n");
  printf(
"-om <m> : Optimization for <m>='sp':speed (default), <m>='si':size.\n");
  printf(
"-St <s> : Max number of nodes in conversion tree (def 128).\n");
  printf(
"-Ss <s> : Stack-size conversion stack (default 64).\n");
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

int main(int argc, const char *argv[])
{
  int m, res, option_no, append, batch;
  int maxtree, maxstack;
  int optimize, digits, silent = 0;
  int non_protected, non_rounded;
  int use_minmax, use_relations, use_inline, use_double;
  int conf_func;
  char so_fname[256] = "";
  char de_fname[256] = "";
  char sout_fname[256] = "";
  char func_name[256] = "";
  char txt[80];
  const char *extensions[]  = { 
                          "jfr",     /* 0 */
                          "cpp",     /* 1 */
                          "h"        /* 2 */
                        };

  non_protected = 0;
  non_rounded = 0;
  use_relations = 0;
  use_minmax = 0;
  use_inline = 0;
  conf_func = 1;
  sout = stdout;
  maxtree  = 128;
  maxstack = 64;
  digits = 5;
  append = 0;
  batch = 1;
  optimize = 0; /* speed */
  use_double = 0;

  if (argc == 1)
    return jf_about();
  strcpy(so_fname, argv[argc - 1]);
  ext_subst(so_fname, extensions[0], 0);
  for (m = 1; m < argc - 1; )
  { option_no = jf_getoption(argv, m, argc);
    if (option_no == -1)
   	  return us_error();
    else
    { m++;
      switch (option_no)
      { case 0:              /* -np */
          non_protected = 1;
          break;
        case 1:              /* -o */
          strcpy(de_fname, argv[m]);
          ext_subst(de_fname, extensions[1], 0);
          m++;
          break;
        case 2:          /* -om */
          if (strcmp(argv[m], "sp") == 0)
            optimize = 0;
          else
          if (strcmp(argv[m], "si") == 0)
            optimize = 1;
          else
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
        case 6:
          append = 1;
          break;
        case 7:          /* - g */
          digits = atoi(argv[m]) + 1;
          m++;
          break;
        case 8:          /* -s */
          silent = 1;
          break;
        case 9:          /* -w  */
          batch = 0;
          break;
        case 10:         /* -n */
          strcpy(func_name, argv[m]);
          m++;
          break;
        case 11:         /* -nr */
          non_rounded = 1;
          break;
        case 12:        /* -ur */
          use_relations = 1;
          break;
        case 13:        /* -um */
          use_minmax = 1;
          break;
        case 14:        /* -ui */
          use_inline = 1;
          break;
        case 15:        /* -nc */
          conf_func = 0;
          break;
        case 16:        /* -d */
          use_double = 1;
          break;
        default:          /* -?  */
          return jf_about();
          /* break; */
      }
    }
  }  /* for  */

  if (strlen(sout_fname) != 0)
  { if (append == 0)
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
  if (strlen(func_name) == 0)
    strcpy(func_name, "jfs");

  if (silent == 0)
    fprintf(sout, "\nconverting: %s\n", so_fname);
  res = jfr2c_conv(de_fname, func_name, so_fname, digits,
                   non_protected, non_rounded,
                   use_relations, use_minmax, use_inline,
                   optimize, conf_func,
                   use_double,
	                  maxtree, maxstack, sout);


  if (silent == 0 && res == 0)
    fprintf(sout, "\nsucces. Converted file written to: %s\n\n", de_fname);
  if (res != 0)
    fprintf(sout, "\nERRORS in convertion.\n\n");
  if (batch == 0)
  { printf("Press RETURN...");
    fgets(txt, 78, stdin);
  }
  return 0;
}


