  /*************************************************************************/
  /*                                                                       */
  /* jfr2htm.c - JFS Converter. Converts a compiled jfs-program            */
  /*   to HTML (Javascript)                                                */
  /*                             Copyright (c) 1998-1999 Jan E. Mortensen  */
  /*                                       Copyright (c) 2010 Miriam Ruiz  */
  /*                                                                       */
  /*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "cmds_common.h"
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfr2hlib.h"

static FILE *sout;
static int jfr2htm_batch = 1;

static const char usage[] =
	"jfr2htm [-g d] [-Ss s] [-St s] [-Sa c] [-j] [-Sh s]"
	" [-r] [-so sf] [-a] [-w] [-o of] [-ow] [-Nc] [-s]"
	" [-l m] [-Na] [-c] [-Cm m] [-Ct t] [-p f] <file.jfr>";

struct jfscmd_option_desc jf_options[] =
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

/*************************************************************************/
/* Hjaelpe-funktioner                                                    */
/*************************************************************************/

static int us_error(void)         /* usage-error. Fejl i kald af jfs */
{
  jfscmd_fprint_wrapped(stdout, jfscmd_num_of_columns() - 7, "usage: ", "       ", usage);
  return 1;
}

static const char *about[] = {
  "usage: jfr2htm [options] <file.jfr>",
  "",
  "JFR2HTM is a JFS converter. It converts the compiled jfs-program <file.jfr> "
    "to a HTML file, with the program converted to Javascript.",
  "",
  "Options:",
  "-o <o>    : Write HTML file to <o>.    -j      : Write JavaScript to <o>.js.",
  "-c         : write program comment.    -ow     : Overwrite HTML file.",
  "-g <dec>   : <dec> is precision.       -Nc     : Don't check numbers.",
  "-a         : Append output.            -w      : Wait for return.",
  "-s         : Silent (to stdout).       -so <s> : Redirect stdout to <s>.",
  "-Cm <m>    : <m> is max-confidence.    -Ct <t> : <t> is confidence text.",
  "-Na        : Ignore var-arguments.     -p <p>  : <p> is object prefix.",
  "-l <m>     : Label-placement: <m>='l':left, 'a':above, 't':table,",
  "             'ab':above, extra blank line, 'mt': multicolumn table.",
  "-Sh <ss>   : Include reference to the stylesheet <ss> in HTML-file.",
  "-r         : Convert only program-part (not input/output-form).",
  "-Sa <size> : Max number of characters in statement (default 512).",
  "-St <size> : Max number of nodes in conversion tree (def 128).",
  "-Ss <size> : Max stacksize conversion-stack (default 64).",
  NULL
};

static int jf_about(void)
{
  char txt[80];

  txt[0] = '\0';

  jfscmd_print_about(about);

  if (jfr2htm_batch == 0)
  { printf("Press RETURN ....");
    fgets(txt, 78, stdin);
  }

  return 0;
}

static int jf_copy_file(char *de_fname, char *so_fname)
{
  int c;
  FILE *jfi_op = NULL;
  FILE *jfi_ip = NULL;

  if ((jfi_ip = fopen(so_fname, "rb")) == NULL)
  {
    return 1;  /* overwrite */
  }

  if ((jfi_op = fopen(de_fname, "wb")) == NULL)
  {
    fprintf(sout, "cannot open the file: %s for writing\n", de_fname);
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
  { if (jfscmd_getoption(jf_options, argv, 1, argc) == 10)  /* -w */
    { jfr2htm_batch = 0;
      return jf_about();
    }
  }

  strcpy(so_fname, argv[argc - 1]);
  jfscmd_ext_subst(so_fname, extensions[0], 1);

  for (m = 1; m < argc - 1; )
  { option_no = jfscmd_getoption(jf_options, argv, m, argc);
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
          jfscmd_ext_subst(de_fname, extensions[1], 0);
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

  if (strlen(so_fname) == 0)
    return us_error();
  if (strlen(de_fname) == 0)
  { strcpy(de_fname, so_fname);
    jfscmd_ext_subst(de_fname, extensions[1], 1);
  }
  if (js_file == 1)
  { strcpy(js_fname, de_fname);
    jfscmd_ext_subst(js_fname, extensions[2], 1);
  }

  strcpy(rm_fname, de_fname);
  jfscmd_ext_subst(rm_fname, extensions[3], 1);


  if (silent == 0)
    fprintf(sout, "\nconverting: %s\n", so_fname);

  if (overwrite == 0)
    overwrite = jf_copy_file(rm_fname, de_fname);

  res = jfr2h_conv(de_fname, js_fname,
                   so_fname, rm_fname,
                   mode, digits, overwrite, js_file,
                   maxtext, maxtree, maxstack, no_check,
                   ssheet, use_args, label_mode, main_comment,
                   conf_txt, conf_max, prefix_txt,
                   sout);

  if (overwrite == 0 && res != 0)
  { jf_copy_file(de_fname, rm_fname);
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



