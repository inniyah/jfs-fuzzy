  /*************************************************************************/
  /*                                                                       */
  /* jfc.c - JFS Compiler                                                  */
  /*                             Copyright (c) 1999-2000 Jan E. Mortensen  */
  /*                                       Copyright (c) 2010 Miriam Ruiz  */
  /*                                                                       */
  /*************************************************************************/

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cmds_lib.h"
#include "jfs2wlib.h"
#include "jfw2rlib.h"

#define CM_SW   1
#define CM_WR   2
#define CM_SR   3
#define CM_SWR  4

static const char usage[] =
	"jfc [-o jfrf] [-e errf] [-em emode] [-so sout] [-s] [-a] [-m ctyp]"
	" [-mt mc] [-mw wc] [-ms ss] [-w m] <file.jfs>";

static const char *about[] = {
  "usage: jfc [options] <file.jfs>",
  "",
  "JFC is the JFS compiler. It compiles the given file, depending on the options, from a jfs-file or a jfw-file, to a jfw-file or a jfr-file.",
  "",
  "Options:",
  "-m <cm>      : compile-mode. <cm> in {'sr', 'sw', 'wr', 'swr'}.",
  "-s           : silent (don't write messages to stdout).",
  "-e <errf>    : write error messages to the file <errf>.",
  "-so <sof>    : redirect messages from stdout to <sof>.",
  "-a           : append to error-file/stdout-file.",
  "-em <m>      : error message format <m> in {'s' (standard), 'c'(compact)}.",
  "-o <jfrf>    : write the compiled program to the file <jfrf>.",
  "-mt <tc>     : <tc> is maximal number of chars in sentence.",
  "-mw <wc>     : <wc> is maximal number of words in sentence.",
  "-ms <ss>     : <ss> is size of expression stack.",
  "-w <m>       : <m>='y':wait for RETURN, 'n';dont wait, 'e':wait if errors.",
  NULL
};

static struct jfscmd_option_desc jf_options[] = {
        {"-e", 1},         /* 0 */
        {"-s", 0},         /* 1 */
        {"-o", 1},         /* 2 */
        {"-mt",1},         /* 3 */
        {"-mw",1},         /* 4 */
        {"-ms",1},         /* 5 */
        {"-a", 0},         /* 6 */
        {"-em",1},         /* 7 */
        {"-so",1},         /* 8 */
        {"-m", 1},         /* 9 */
        {"-w", 1},         /*10 */
        {"-?", 0},
        {"?",  0},
        {" ", -2}
};

static const char *extensions[] = {
	"jfs",     /* 0 */
	"jfw",     /* 1 */
	"jfr"      /* 2 */
};

static int jfc_wait_mode = 0;

static int us_error(int silent_mode)
{
	if (silent_mode == 0) {
		jfscmd_fprint_wrapped(stdout, jfscmd_num_of_columns() - 7, "usage: ", "       ", usage);
	}
	return 1;
}

static void wait_if_needed()
{
	char tmp[80];
	if (jfc_wait_mode !=  0)
	{
		printf("Press RETURN to continue");
		fgets(tmp, 10, stdin);
	}
}

int main(int argc, const char *argv[])
{
  int m, option_no, append_mode;
  int el_mode;  /* error_line_mode0: start with line 1, 1: start with fprog */
  int maxstack, maxargline, margc;
  int comp_mode;
  int jfc_silent_mode;
  int jfc_err_message_mode;
  FILE *sout;
  char so_fname[256];
  char er_fname[256];
  char tmp_fname[256];
  char de_fname[256];
  char sout_fname[256];

  so_fname[0] = er_fname[0] = tmp_fname[0] = de_fname[0] = sout_fname[0] = '\0';
  sout = stdout;
  jfc_wait_mode = 0;
  maxstack = maxargline = margc = 0;
  jfc_silent_mode = jfc_err_message_mode = el_mode = 0;
  append_mode = 0;
  comp_mode = CM_SR;
  if (argc == 1) {
    jfscmd_print_about(about);
    return 0;
  }
  if (argc == 2 && strcmp(argv[1], "-w") == 0)
  {
    jfc_wait_mode = 1;
    jfscmd_print_about(about);
    wait_if_needed();
    return 0;
  }
  strcpy(so_fname, argv[argc - 1]);
  if (jfscmd_isoption(so_fname) == 1)
    return us_error(jfc_silent_mode);
  for (m = 1; m < argc - 1; )
  { option_no = jfscmd_getoption(jf_options, argv, m, argc - 1);
    if (option_no == -1)
      return us_error(jfc_silent_mode);
    m++;
    switch (option_no)
    { case 0:                         /* -e */
        strcpy(er_fname, argv[m]);
        m++;
        break;
      case 1:                         /* -s */
        jfc_silent_mode = 1;
        break;
      case 2:                         /* -o */
        strcpy(de_fname, argv[m]);
        m++;
        break;
      case 3:                         /* -mt */
        maxargline = atoi(argv[m]);
        if (maxargline <= 10)
          return us_error(jfc_silent_mode);
        m++;
        break;
      case 4:                         /* -mw */
        margc = atoi(argv[m]);
        if (margc <= 10)
          return us_error(jfc_silent_mode);
        m++;
        break;
      case 5:                         /* -ms */
        maxstack = atoi(argv[m]);
        if (maxstack <= 10)
          return us_error(jfc_silent_mode);
        m++;
        break;
      case 6:                        /* -a */
        append_mode = 1;
        break;
      case 7:                        /* -me */
        if (strcmp(argv[m], "s") == 0)
          jfc_err_message_mode = 0;
        else
        if (strcmp(argv[m], "c") == 0)
          jfc_err_message_mode = 1;
        else
          return us_error(jfc_silent_mode);
        m++;
        break;
      case 8:                       /* -so */
        strcpy(sout_fname, argv[m]);
        m++;
        break;
      case 9:                       /* -m */
        if (strcmp(argv[m], "sw") == 0)
          comp_mode = CM_SW;
        else
        if (strcmp(argv[m], "wr") == 0)
          comp_mode = CM_WR;
        else
        if (strcmp(argv[m], "sr") == 0)
          comp_mode = CM_SR;
        else
        if (strcmp(argv[m], "swr") == 0)
          comp_mode = CM_SWR;
        else
          return us_error(jfc_silent_mode);
        m++;
        break;
      case 10:                     /* -w */
        if (strcmp(argv[m], "n") == 0)
          jfc_wait_mode = 0;
        else
        if (strcmp(argv[m], "y") == 0)
          jfc_wait_mode = 1;
        else
        if (strcmp(argv[m], "e") == 0)
          jfc_wait_mode = 2;
        else
          return us_error(jfc_silent_mode);
        m++;
        break;
      default:
      return us_error(jfc_silent_mode);
    }
  }  /* for  */

  if (strlen(so_fname) == 0)
    return us_error(jfc_silent_mode);
  if (strlen(de_fname) == 0)
    strcpy(de_fname, so_fname);
  switch (comp_mode)
  {  case CM_SW:
       jfscmd_ext_subst(so_fname, extensions[0], 1);
       jfscmd_ext_subst(de_fname, extensions[1], 1);
       strcpy(tmp_fname, de_fname);
       break;
     case CM_WR:
       jfscmd_ext_subst(so_fname, extensions[1], 1);
       strcpy(tmp_fname, so_fname);
       jfscmd_ext_subst(de_fname, extensions[2], 1);
       break;
     case CM_SR:
       jfscmd_ext_subst(so_fname, extensions[0], 1);
       tmpnam(tmp_fname);
       jfscmd_ext_subst(tmp_fname, extensions[1], 1);
       jfscmd_ext_subst(de_fname, extensions[2], 1);
       el_mode = 1;
       break;
     case CM_SWR:
       jfscmd_ext_subst(so_fname, extensions[0], 1);
       strcpy(tmp_fname, de_fname);
       jfscmd_ext_subst(tmp_fname, extensions[1], 1);
       jfscmd_ext_subst(de_fname, extensions[2], 1);
       el_mode = 1;
       break;
  }

  if (strlen(sout_fname) != 0)
  { if (append_mode == 0)
      sout = fopen(sout_fname, "w");
    else
      sout = fopen(sout_fname, "a");
    if (sout == NULL)
      sout = stdout;
  }
  if (jfc_silent_mode == 0)
  { fprintf(sout, "\n%s\n\n", jfs_copyright);
    fprintf(sout, "compiling: %s\n\n", so_fname);
  }
  if (strlen(sout_fname) != 0)
    fclose(sout);
  if (strlen(er_fname) == 0)
  { if (strlen(sout_fname) != 0)
    { strcpy(er_fname, sout_fname);
      append_mode = 1;
    }
  }
  m = 0;
  if (comp_mode != CM_WR)
  { m = jfs2w_convert(tmp_fname, so_fname,
                      er_fname, append_mode,
                      maxargline, margc,
                      jfc_err_message_mode);
    append_mode = 1;
  }
  if (m == 2 && comp_mode == CM_SR)
    m = 1;
  if (m == 0 && comp_mode != CM_SW)
    m = jfw2r_convert(de_fname, tmp_fname,
                      er_fname, append_mode,
                      maxstack, maxargline, margc,
                      jfc_err_message_mode, el_mode);

  if (strlen(sout_fname) != 0)
    sout = fopen(sout_fname, "a");
  if (comp_mode == CM_SR)
    remove(tmp_fname);

  if (jfc_silent_mode == 0)
  { if (m == 0)
      fprintf(sout, "success. Compiled file written to: %s.\n\n", de_fname);
    else
    if (m == 1)
      fprintf(sout, "Errors. Nothing written to: %s.\n", de_fname);
    else
      fprintf(sout, "Error when writing to %s. File damaged!\n", de_fname);
    if (jfc_wait_mode == 1 || (jfc_wait_mode == 2 && m != 0))
    { printf("Press RETURN to continue");
      fgets(tmp_fname, 10, stdin);
    }
  }
  if (sout != stdout)
    fclose(sout);
  return m;
}
