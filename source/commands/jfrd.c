  /*************************************************************************/
  /*                                                                       */
  /* jfrd.c - JFS Rule-discover program                                    */
  /*                             Copyright (c) 1998-2000 Jan E. Mortensen  */
  /*                                       Copyright (c) 2010 Miriam Ruiz  */
  /*                                                                       */
  /*************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cmds_lib.h"
#include "jft_lib.h"
#include "jfrd_lib.h"

static const char usage[] =
	"jfrd [-D dm] [-d df] [-f fs] [-o of] [-Mp pb] [-S] [-so s]"
	" [-Md db] [-Mt t] [-r m] [-iw wgt] [-b] [-c m] [-e] [-a]"
	" [-w] <file.jfr>";

static const char *about[] = {
  "usage: jfrd [options] <file.jfr>",
  "",
  "JFRD replaces the statement 'extern jfrd input {[<op>] <vname>} output [<op>] <vname>'"
    " in the jfs-program <file.jfr> with rules generated from a data file.",
  "",
  "Options:",
  "-f <fs>    : <fs> field-separator.     -iw <wgt>  : 'ifw &<w>' statements.",
  "-Mp <pb>   : Alloc <pb> K to program.  -Md <db>   : Alloc <db> K to rules.",
  "-so <s>    : Redirect stdout to <s>.   -a         : Append stdout to <s>.",
  "-w         : Wait for return.          -S         : Reduction in entered order.",
  "-d <df>    : Read data from the file <df>.",
  "-D <d>     : Data order. <d>={i|e|t}|f. i:input,e:expected,t:text,f:firstline.",
  "-o <of>    : Write the changed jfs-program to the file <of>.",
  "-Mt <mt>   : <mt> is Maximum number of minutes used in rewind-reduction.",
  "-r <rm>    : reduce-mode. d:default,n:none,a:all,b:between,i:in,ib:inbetween.",
  "-b         : Case-reduction.",
  "-c <cr>    : Conflict-resolve: <cr>=s:score, <cr>=c:count.",
  "-e         : Remove rules with default output value.",
  NULL
};

struct jfscmd_option_desc jf_options[] =
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

static int us_error(void) /* usage-error */
{
	jfscmd_fprint_wrapped(stdout, jfscmd_num_of_columns() - 7, "usage: ", "       ", usage);
	return 1;
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
  {
    jfscmd_print_about(about);
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
        strcpy(field_sep, argv[m]);
        m++;
        break;
      case 1:              /* -s */
        silent = 1;
        break;
      case 2:              /* -D */
        data_mode = jfscmd_tmap_find(jf_im_texts, argv[m]);
        if (data_mode == -1)
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
        jfscmd_ext_subst(op_fname, extensions[0], 1);
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
    jfscmd_ext_subst(op_fname, extensions[0], 1);
  }
  if (strlen(da_fname) == 0)
  { strcpy(da_fname, ip_fname);
    jfscmd_ext_subst(da_fname, extensions[1], 1);
  }
  m = jfrd_run(op_fname, ip_fname, sout_fname, da_fname, data_mode, field_sep,
               prog_size, data_size, max_time, red_mode, red_weight, weight_value,
               red_case, res_confl_mode, red_order, def_fzvar,
               append_mode, silent, batch);

  return m;
}

