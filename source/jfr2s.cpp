  /**********************************************************************/
  /*                                                                    */
  /* jfs2s.cpp Version  2.01   Copyright (c) 1998-1999 Jan E. Mortensen */
  /*                                                                    */
  /* JFS Invers compiler (Converts a JFR-file to a JFS-file or a        */
  /* JFW-file).                                                         */
  /*                                                                    */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                 */
  /*    Lollandsvej 35 3.tv.                                            */
  /*    DK-2000 Frederiksberg                                           */
  /*    Denmark                                                         */
  /*                                                                    */
  /**********************************************************************/

#ifdef __BCPLUSPLUS__
  #pragma hdrstop
  #include <condefs.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfr2wlib.h"
#include "jfw2slib.h"

//---------------------------------------------------------------------------
#ifdef __BCPLUSPLUS__
  USEUNIT("..\..\COMMON\jfs_text.cpp");
USEUNIT("..\..\COMMON\jfw2slib.cpp");
USEUNIT("..\..\COMMON\jfr_lib.cpp");
USEUNIT("..\..\COMMON\jfr2wlib.cpp");
USEUNIT("..\..\COMMON\jfg_lib.cpp");
//---------------------------------------------------------------------------
#pragma argsused
#endif

#define CM_RW   0
#define CM_RS   1
#define CM_WS   2
#define CM_RWS  3

char usage1[] =
"usage: jfr2s [-m cm] [-o of] [-so sout] [-g dg] [-pm pm] [-a] [-v]";
char usage2[] =
"             [-Ss ss] [-St ts] [-Sa cs] [-t] [-k] [-w wm] [-n]       rf";

struct jf_option_desc {
	const char *option;
	int argc;	/* -1: variabelt,     */
			/* -2: last argument. */
};

struct jf_option_desc jf_options[] =
  {
    {"-m",  1},    /*  0 */
    {"-g",  1},    /*  1 */
    {"-pm", 1},   /*  2 */
    {"-Sa", 1},    /*  3 */
    {"-St", 1},    /*  4 */
    {"-Ss", 1},    /*  5 */
    {"-n",  0},    /*  6 */
    {"-t",  0},    /*  7 */
    {"-v",  0},    /*  8 */
    {"-k",  0},    /*  9 */
    {"-a",  0},    /* 10 */
    {"-w",  1},    /* 11 */
    {"-o",  1},    /* 12 */
    {"-so", 1},    /* 13 */
    {"-?",  0},    /* 14 */
    {"?",   0},    /* 15 */
    {" ",  -2}
  };

char t_standard[] = "s";
char t_extra[]    = "e";

static int isoption(const char *s);
static int us_error(void);
static int jf_about(void);
static int jf_getoption(char *argv[], int no, int argc);
static void ext_subst(char *d, char *e, int forced);


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
  printf("\n%s\n", usage1);
  printf("%s\n", usage2);
  return 1;
}



static int jf_about(void)
{

  printf("\nJFR2S   version  2.01   Copyright (c) 1998-1999 Jan E. Mortensen\n\n");
  // printf("by Jan E. Mortensen         email: jemor@inet.uni2.dk\n\n");
  printf("usage: jfr2s [options] rf\n\n");

  printf("JFR2S is a JFS converter. It converts the jfr/jfw-file <rf> back to\n");
  printf("source code (jfs-file) or to a jfw-file.\n\n");
  printf("OPTIONS:\n");

  printf("-m <cm>    : Conversion-mode. <cm> in {'rs', 'rw', 'ws', 'rws'}.\n");
  printf("-o <of>    : Write the converted program to <of>.\n");
  printf("-g <digits>: Max number of digits after decimal-point.\n");
  printf("-n         : Write statement numbers.\n");
  printf("-v         : Write variable numbers.\n");
  printf("-t         : Write '|'-comments in switch-blocks.\n");
  printf("-pm <mode> : Parenthes-mode: s: standard, e: extra parentheses.\n");
  printf("-k         : Write keywords.\n");
  printf(
  "-Sa <size> : Max number of characters in statement (default 1024).\n");
  printf("-St <size> : Max number of nodes in conversion-tree (def 128).\n");
  printf("-Ss <size> : Max stack-size conversion-stack (default 64).\n");
  printf("-so <sof>  : Redirect stdout-messages to <sof>.\n");
  printf("-a         : Append messages to <sof>.\n");
  printf(
  "-w <m>     : <m>='y':Wait for RETURN, 'n':don't wait, 'e':wait if errors.\n");
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


int main(int argc, char *argv[])
{
  int m, res, rm_res, option_no;
  int ndigits, maxtext, maxtree, maxstack, parent_mode, rule_nos;
  int sw_comments, variable_nos, keywords;
  int wait_mode, conv_mode, append_mode;
  FILE *so;
  char so_fname[256]   = "";
  char de_fname[256]   = "";
  char tmp_fname[256]  = "";
  char sout_fname[256] = "";

  char *extensions[]  =
  { "jfr",     /* 0 */
    "jfs",     /* 1 */
    "jfw"      /* 2 */
  };

  so = stdout;
  conv_mode    = CM_RS;
  parent_mode  = 0;
  maxtext      = 1024;
  maxtree      = 128;
  maxstack     = 64;
  rule_nos     = 0;
  ndigits      = 5;
  sw_comments  = 0;
  variable_nos = 0;
  keywords     = 0;
  wait_mode    = 0;  /* don't wait */
  append_mode  = 0;

  if (argc == 1)
    return jf_about();
  strcpy(so_fname, argv[argc - 1]);
  for (m = 1; m < argc - 1; )
  { option_no = jf_getoption(argv, m, argc);
    m++;
    switch (option_no)
    { case 0:              /* -m  */
        if (strcmp(argv[m], "rw") == 0)
          conv_mode = CM_RW;
        else
        if (strcmp(argv[m], "rs") == 0)
          conv_mode = CM_RS;
        else
        if (strcmp(argv[m], "ws") == 0)
          conv_mode = CM_WS;
        else
        if (strcmp(argv[m], "rws") == 0)
          conv_mode = CM_RWS;
        else
          return us_error();
        m++;
        break;
      case 1:              /* -g */
        ndigits = atoi(argv[m]);
        if (ndigits <= 0 || ndigits >= 10)
        { printf("\n Illegal number of digits\n");
          return us_error();
        }
        ndigits++;
        m++;
        break;
      case 2:                /* -pm */
        if (strcmp(argv[m], t_standard) == 0)
          parent_mode = 0;
        else
        if (strcmp(argv[m], t_extra) == 0)
          parent_mode = 1;
        else
          return us_error();
        m++;
        break;
      case 3:          /* -Sa */
        maxtext = atoi(argv[m]);
        if (maxtext <= 0)
          return us_error();
        m++;
        break;
      case 4:          /* -St */
        maxtree = atoi(argv[m]);
        if (maxtree <= 0)
          return us_error();
        m++;
        break;
      case 5:          /* -Ss */
        maxstack = atoi(argv[m]);
        if (maxstack <= 0)
          return us_error();
        m++;
        break;
      case 6:        /* n  */
        rule_nos = 1;
        break;
      case 7:          /* -t */
        sw_comments = 1;
        break;
      case 8:         /* -v */
        variable_nos = 1;
        break;
      case 9:         /* -k  */
        keywords = 1;
        break;
      case 10:         /* -a  */
        append_mode = 1;
        break;
      case 11:         /* -w */
        if (strcmp(argv[m], "n") == 0)
          wait_mode = 0;
        else
        if (strcmp(argv[m], "y") == 0)
          wait_mode = 1;
        else
        if (strcmp(argv[m], "e") == 0)
          wait_mode = 2;
        else
          return us_error();
        m++;
        break;
      case 12:         /* -o  */
        strcpy(de_fname, argv[m]);
        m++;
        break;
      case 13:        /* -so */
        strcpy(sout_fname, argv[m]);
        m++;
        break;
      case 14:        /* -? */
      case 15:        /* ?  */
        return jf_about();
      default:
        return us_error();
    }
  }  /* for  */

  if (strlen(de_fname) == 0)
    strcpy(de_fname, so_fname);
  if (strlen(sout_fname) != 0)
  { if (append_mode == 0)
      so = fopen(sout_fname, "w");
    else
      so = fopen(sout_fname, "a");
    if (so == NULL)
    { so = stdout;
      printf("Cannot open %d\n", sout_fname);
    }
  }
  switch (conv_mode)
  {
    case CM_RW:
      ext_subst(so_fname, extensions[0], 1);
      ext_subst(de_fname, extensions[2], 1);
      strcpy(tmp_fname, de_fname);
      break;
    case CM_RS:
      ext_subst(so_fname, extensions[0], 1);
      ext_subst(de_fname, extensions[1], 1);
      tmpnam(tmp_fname);
      ext_subst(tmp_fname, extensions[2], 1);
      break;
    case CM_WS:
      ext_subst(so_fname, extensions[2], 1);
      strcpy(tmp_fname, so_fname);
      ext_subst(de_fname, extensions[1], 1);
      break;
    case CM_RWS:
      ext_subst(so_fname, extensions[0], 1);
      strcpy(tmp_fname, de_fname);
      ext_subst(tmp_fname, extensions[2], 1);
      ext_subst(de_fname, extensions[1], 1);
      break;
  }
  fprintf(so,
  "\nJFR2S    version   2.00    Copyright (c) 1998-1999 Jan E. Mortensen\n\n");
  fprintf(so, "invers compiling: %s\n\n", so_fname);

  if (so != stdout)
    fclose(so);
  res = 0;
  if (conv_mode != CM_WS)
  res = jfr2w_conv(tmp_fname, so_fname,  sout_fname,
                   1, ndigits,
                   maxtext, maxtree, maxstack,
                   rule_nos, parent_mode, sw_comments);
  if (res < 2)
  { rm_res = res;
    if (conv_mode != CM_RW)
    res = jfw2s_conv(de_fname, tmp_fname, sout_fname,
                     ndigits, maxtext, 1,
                     variable_nos, keywords);
    if (conv_mode == CM_RS)
      remove(tmp_fname);
    if (res < rm_res)
      res = rm_res;
  }
  if (strlen(sout_fname) != 0)
    so = fopen(sout_fname, "a");
  if (res == 0)
    fprintf(so, "success. Source-code written to: %s\n\n", de_fname);
  else
  if (res == 1)
    fprintf(so, "Source-code written to: %s with warnings\n\n", de_fname);
  else
    fprintf(so, "Errors in conversion. Nothing written to %s\n\n", de_fname);
  if (wait_mode == 1 || (wait_mode == 2 && res != 0))
  { printf("Press RETURN");
    fgets(tmp_fname, 78, stdin);
  }
  if (so != stdout)
    fclose(so);

  return res;
}


