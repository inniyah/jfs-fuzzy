  /**************************************************************************/
  /*                                                                        */
  /* jffam.cpp   Version  2.03   Copyright (c) 1998-2000 Jan E. Mortensen   */
  /*                                                                        */
  /* JFS Fam-creation by a cellular automat.                                */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jft_lib.h"
#include "jffamlib.h"

const char usage_1[] =
"usage: jffam [-D dm] [-d df] [-f fs] [-o of] [-Mp ps] [-Md ds]";
const char usage_2[] =
"            [-rf df] [-iw wgt] [-c m] [-r ru] [-ms s] [-nf]";
const char usage_3[] =
"            [-w] [-a] [-so sf] [-tt tm]                             jfrf";

struct jf_option_desc {
	const char *option;
	int argc;      /* -1: variabelt */
};               /* -2: sidste argument */

struct jf_option_desc jf_options[] = {
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



static int isoption(const char *s)
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

static int jf_about(void)
{
  printf(
  "\n\nJFFAM   version 2.03    Copyright (c) 1998-2000 Jan E. Mortensen\n\n");

  printf("usage: jffam [options] jfrf\n\n");

  printf(
"JFFAM replaces the statement:\n");
  printf(
"'extern jfrd input {[<op>] <vname>} output [<op>] <vname>' in the jfs-program\n");
  printf(
"<jfrf> with rules (a FAM) generated from a data file (by a CA).\n\n");
  printf("OPTIONS\n");

  printf(
"-d <df>    : Data from file <df>.       -so <of>  : Redirect stdout to <of>.\n");
  printf(
"-a         : append output.             -w        : Wait for return.\n");
  printf(
"-Mp <pb>   : Alloc <pb> K to program.   -Md <db>  : Alloc <db> K to rules.\n");

  printf("-f <fs>    : Use <fs> as field-separator.\n");
  printf(
"-D <d>     : Data order. <d>={i|e|t}|f. i:input,e:expected,t:text,f:firstline.\n");
  printf("-o <of>    : Write the changed jfs-program to the file <of>.\n");
  printf("-rf <rf>   : Write the generated rules to the data-file <rf>.\n");
  printf("-iw <wgt>  : Generate 'ifw <wgt> ...' statements.\n");
  printf("-c <cr>    : Conflict-resolve: <cr>=s:score, <cr>=c:count.\n");
  printf(
    "-r <ru>    : Rule used by CA. <ru>='a':avg, 'm':minmax, 'd':delta.\n");
  printf(
    "-ms <ms>   : <ms> is maximum number of steps in cellular automat.\n");
  printf("-nf        : No fixed rules in cellular automat.\n");
  printf("-tt <t>    : then-type: <t>='a':adjectiv, 'c':center.\n");
  return 0;
}

static int us_error(void)         /* usage-error. Fejl i kald af jfs */
{
  printf("\n%s\n%s\n%s\n", usage_1, usage_2, usage_3);
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
       	break;
      case 2:              /* -D */
        data_mode = jf_tmap_find(jf_im_texts, argv[m]);
        if (data_mode == -1)
          return us_error();
        m++;
        break;
      case 3:            /* -d */
        if (m < argc - 1 && isoption(argv[m]) == 0)
        { strcpy(da_fname, argv[m]);
          ext_subst(da_fname, extensions[1], 0);
          m++;
        }
        else
        { strcpy(da_fname, ip_fname);
          ext_subst(da_fname, extensions[1], 1);
        }
        break;
      case 4:            /* -o */
        strcpy(op_fname, argv[m]);
        ext_subst(op_fname, extensions[0], 0);
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
        ext_subst(ru_fname, extensions[1], 0);
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
        return jf_about();
        /* break;  */
      default:
       	return us_error();
    }
  }  /* for  */

  if (strlen(op_fname) == 0 && strlen(ru_fname) == 0)
  { strcpy(op_fname, ip_fname);
    ext_subst(op_fname, extensions[0], 1);
  }
  if (strlen(da_fname) == 0)
  { strcpy(da_fname, ip_fname);
    ext_subst(da_fname, extensions[1], 1);
  }
  res = jffam_run(op_fname, ip_fname, ru_fname, da_fname,
                  field_sep, data_mode, prog_size, data_size,
                  res_confl, ca_rule, weight_val, then_type,
                  fixed, max_steps,
                  so_fname, append, batch);

  return res;
}

