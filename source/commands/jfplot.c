  /*************************************************************************/
  /*                                                                       */
  /* jfrplot.c - Program to plot the hedges, operators,                    */
  /*   fuzzification-functions etc in a compiled jfs-program               */
  /*   using Gnuplot                                                       */
  /*                                  Copyright (c) 2000 Jan E. Mortensen  */
  /*                                       Copyright (c) 2010 Miriam Ruiz  */
  /*                                                                       */
  /*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cmds_lib.h"
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfpltlib.h"
#include "jopt_lib.h"

static const char *about[] = {
  "usage: jfplot [options] <file.jfr>",
  "",
  "JFPLOT writes plot-information about the compiled jfs-program <file.jfr> to a Gnuplot-file.",
  "",
  "Options:",
  "-a      : append to stdout.",
  "-w      : Wait for return.",
  "-S <s>  : redirect stdout to <s>.",
  "-o <of> : write the Gnuplot-file to <of>.plt.",
  "-p <po> : write plot-info about <po>, where po is build of: 'h':hedges,",
  "          'r':relations, 'o':operators, 'f:fuzification, 'd':defuzz.",
  "-d {<o>}: write only plot-info about the objects {<o>}.",
  "-g <p>  : precision. <p> is the number of decimals.",
  "-m <sa> : number of samples used to generate graph (default 300).",
  "-i <if> : read Gnuplot initialization from the file <if>.",
  "-t <t> <e> [<d>]: Insert a 'set term <t>'-statement in Gnuplot-file. The plots",
  "          are written to files in the directory <d> with extension <e>.",
  NULL
};

static struct jfplt_param_desc params;

static int batch = 1;

struct jopt_desc options[] =
{  /* txt, value, min, max */
  {  "-p",     0,   1, 1},
  {  "-o",     1,   1, 1},
  {  "-d",     2,   1, 128},
  {  "-i",     3,   1, 1},
  {  "-S",     4,   1, 1},
  {  "-a",     5,   0, 0},
  {  "-g",     6,   1, 1},
  {  "-w",     7,   0, 0},
  {  "-t",     8,   2, 3},
  {  "-m",     9,   1, 1},
};
#define OPT_COUNT 10

/****************************************************************************/
/* Error-handling                                                           */
/****************************************************************************/

#define JFE_WARNING 0
#define JFE_ERROR   1

struct jfr_err_desc {
	int eno;
	const char *text;
};

struct jfr_err_desc jfr_err_texts[] = {
	{   0, " "},
	{   1, "Cannot open file:"},
	{   2, "Error reading from file:"},
	{   3, "Error writing to file:"},
	{   4, "Not a jfr-file:"},
	{   5, "Wrong version:"},
	{   6, "Cannot allocate memory to:"},
	{   9, "Illegal number:"},
	{  10, "Value out of domain-range:"},
	{  11, "Unexpected EOF."},
	{  13, "Undefined adjective:"},
	{  51, "Illegal option:"},
	{  52, "Too few arguments to option:"},
	{  53, "Too many arguments to option:"},
	{9999, "unknown error."}
};

static int jf_error(int eno, char *name, int mode)
{
  int m, v, e;

  e= 0;
  for (v = 0; e == 0; v++)
  { if (jfr_err_texts[v].eno == eno
	    || jfr_err_texts[v].eno == 9999)
      e = v;
  }
  if (mode == JFE_WARNING)
  { fprintf(params.sout, "WARNING %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    m = 0;
  }
  else
  { if (eno != 0)
      fprintf(params.sout, "*** error %d: %s %s\n", eno, jfr_err_texts[e].text, name);
    m = 1;
  }
  return m;
}

/*************************************************************************/
/* Hjaelpe-funktioner                                                    */
/*************************************************************************/

static int jf_about(void)
{
  char txt[80];
  jfscmd_print_about(about);
  if (batch == 0)
  { printf("Press RETURN...");
    fgets(txt, 16, stdin);
  }
  return 0;
}

int main(int argc, const char *argv[])
{
  int res, append, largc, m;
  unsigned short option_no;
  char sout_fname[256] = "";
  char ip_fname[256] = "";
  char op_fname[256] = "";
  char init_fname[256] = "";
  char op_dir[256] = "";
  char op_extension[64] = "";
  char term_name[256] = "";
  char txt[80];
  const char *extensions[]  = {
                          "jfr",     /* 0 */
                          "plt",     /* 1 */
                          "gif"      /* 2 */
                        };
  const char *largv[128];

  const char *dobjects[128];

  res = 0;
  params.sout = stdout;
  params.data_c = 0;

  ip_fname[0] = '\0';
  op_fname[0] = '\0';
  params.digits = 5;
  append = 0;
  batch = 1;
  params.samples = 300;
  params.plt_hedges = params.plt_relations = params.plt_operators
   = params.plt_fuz = params.plt_defuz = 1;

  if (argc == 1)
    return jf_about();
  if (argc == 2)
  { if (strcmp(argv[1], "-w") == 0)
    { batch = 0;
      return jf_about();
    }
  }
  strcpy(ip_fname, argv[argc - 1]);
  jfscmd_ext_subst(ip_fname, extensions[0], 0);
  jopt_init(options, OPT_COUNT, argv, argc - 1);
  while (jopt_get(&option_no, largv, &largc) == 0)
  { switch (option_no)
    { case 0:              /* -p */
        params.plt_hedges = params.plt_relations = params.plt_operators
                          = params.plt_fuz = params.plt_defuz = 0;
        for (m = 0; m < ((int) strlen(largv[0])); m++)
        { switch (largv[0][m])
          { case 'h':
              params.plt_hedges = 1;
              break;
            case 'r':
              params.plt_relations = 1;
              break;
            case 'o':
              params.plt_operators = 1;
              break;
            case 'f':
              params.plt_fuz = 1;
              break;
            case 'd':
              params.plt_defuz = 1;
              break;
          }
        }
        break;
      case 1:              /* -o */
        strcpy(op_fname, largv[0]);
        jfscmd_ext_subst(op_fname, extensions[1], 0);
        break;
      case 2:          /* -d */
        for (m = 0; m < largc; m++)
          dobjects[m] = largv[m];
        params.data = dobjects;
        params.data_c = largc;
        break;
      case 3:          /* -i */
        strcpy(init_fname, largv[0]);
        jfscmd_ext_subst(init_fname, extensions[2], 0);
        break;
      case 4:          /* -S */
        strcpy(sout_fname, largv[0]);
        break;
      case 5:
        append = 1;
        break;
      case 6:          /* - g */
        params.digits = atoi(largv[0]) + 1;
        break;
      case 7:          /* -w  */
        batch = 0;
        break;
      case 8:          /* -t */
        strcpy(term_name, largv[0]);
        strcpy(op_extension, largv[1]);
        if (largc == 3)
          strcpy(op_dir, largv[2]);
        break;
      case 9:          /* -m */
        params.samples = atoi(largv[0]);
        if (params.samples < 10)
        { printf("***Illegal samples value\n");
          params.samples = 300;
        }  
        break;
    }
  }

  if (strlen(sout_fname) != 0)
  { if (append == 0)
      params.sout = fopen(sout_fname, "w");
    else
      params.sout = fopen(sout_fname, "a");
    if (params.sout == NULL)
    { params.sout = stdout;
      printf("Cannot open %s for writing.\n", sout_fname);
    }
  }
  if (jopt_error_desc.error_mode != JOPT_EM_NONE)
    res = jf_error(jopt_error_desc.error_no, jopt_error_desc.argument, JFE_ERROR);

  if (res == 0)
  { if (strlen(op_fname) == 0)
    { strcpy(op_fname, ip_fname);
      jfscmd_ext_subst(op_fname, extensions[1], 1);
    }
    params.ipfname = ip_fname;
    params.opfname = op_fname;
    params.initfname = init_fname;
    params.term_name = term_name;
    params.op_extension = op_extension;
    params.op_dir = op_dir;

    res = jfplt_plot(&params);
    if (jfplt_error_desc.error_mode != JPLT_EM_NONE)
      res = jf_error(jfplt_error_desc.error_no,
                     jfplt_error_desc.argument, JFE_ERROR);

    if (res == 0)
      fprintf(params.sout, "\nsucces. Plot-file written to: %s\n\n", params.opfname);
    else
      fprintf(params.sout, "\nERRORS in creation of plot-file.\n\n");
  }

  if (batch == 0)
  { printf("Press RETURN...");
    fgets(txt, 16, stdin);
  }
  return res;
}


