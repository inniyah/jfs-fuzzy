  /*************************************************************************/
  /*                                                                       */
  /* jfs2s.c - JFS Reverse compiler (Converts a JFR-file to a JFS-file     */
  /*   or a JFW-file)                                                      */
  /*                             Copyright (c) 1998-1999 Jan E. Mortensen  */
  /*                                       Copyright (c) 2010 Miriam Ruiz  */
  /*                                                                       */
  /*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cmds_common.h"
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfr2wlib.h"
#include "jfw2slib.h"

#define CM_RW   0
#define CM_RS   1
#define CM_WS   2
#define CM_RWS  3

static const char usage[] =
	"jfr2s [-m cm] [-o of] [-so sout] [-g dg] [-pm pm] [-a] [-v]"
	" [-Ss ss] [-St ts] [-Sa cs] [-t] [-k] [-w wm] [-n] <file.jfr>";

static const char *about[] = {
  "usage: jfr2s [options] <file.jfr>",
  "",
  "JFR2S is a JFS converter. It converts the jfr/jfw-file <file.jfr> back to source code (jfs-file) or to a jfw-file.",
  "",
  "Options:",
  "-m <cm>    : Conversion-mode. <cm> in {'rs', 'rw', 'ws', 'rws'}.",
  "-o <of>    : Write the converted program to <of>.",
  "-g <digits>: Max number of digits after decimal-point.",
  "-n         : Write statement numbers.",
  "-v         : Write variable numbers.",
  "-t         : Write '|'-comments in switch-blocks.",
  "-pm <mode> : Parenthes-mode: s: standard, e: extra parentheses.",
  "-k         : Write keywords.",
  "-Sa <size> : Max number of characters in statement (default 1024).",
  "-St <size> : Max number of nodes in conversion-tree (def 128).",
  "-Ss <size> : Max stack-size conversion-stack (default 64).",
  "-so <sof>  : Redirect stdout-messages to <sof>.",
  "-a         : Append messages to <sof>.",
  "-w <m>     : <m>='y':Wait for RETURN, 'n':don't wait, 'e':wait if errors.",
  NULL
};

struct jfscmd_option_desc jf_options[] =
  {
    {"-m",  1},    /*  0 */
    {"-g",  1},    /*  1 */
    {"-pm", 1},    /*  2 */
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

static char t_standard[] = "s";
static char t_extra[]    = "e";

static int us_error(void);

/*************************************************************************/
/* Hjaelpe-funktioner                                                    */
/*************************************************************************/

static int us_error(void)         /* usage-error. Fejl i kald af jfs */
{
  jfscmd_fprint_wrapped(stdout, jfscmd_num_of_columns() - 7, "usage: ", "       ", usage);
  return 1;
}

int main(int argc, const char *argv[])
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

  const char *extensions[]  =
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
  {
    jfscmd_print_about(about);
    return 0;
  }
  strcpy(so_fname, argv[argc - 1]);
  for (m = 1; m < argc - 1; )
  { option_no = jfscmd_getoption(jf_options, argv, m, argc);
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
        jfscmd_print_about(about);
        return 0;
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
      printf("Cannot open %s\n", sout_fname);
    }
  }
  switch (conv_mode)
  {
    case CM_RW:
      jfscmd_ext_subst(so_fname, extensions[0], 1);
      jfscmd_ext_subst(de_fname, extensions[2], 1);
      strcpy(tmp_fname, de_fname);
      break;
    case CM_RS:
      jfscmd_ext_subst(so_fname, extensions[0], 1);
      jfscmd_ext_subst(de_fname, extensions[1], 1);
      tmpnam(tmp_fname);
      jfscmd_ext_subst(tmp_fname, extensions[2], 1);
      break;
    case CM_WS:
      jfscmd_ext_subst(so_fname, extensions[2], 1);
      strcpy(tmp_fname, so_fname);
      jfscmd_ext_subst(de_fname, extensions[1], 1);
      break;
    case CM_RWS:
      jfscmd_ext_subst(so_fname, extensions[0], 1);
      strcpy(tmp_fname, de_fname);
      jfscmd_ext_subst(tmp_fname, extensions[2], 1);
      jfscmd_ext_subst(de_fname, extensions[1], 1);
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


