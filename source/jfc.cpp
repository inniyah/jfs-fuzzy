  /*************************************************************************/
  /*                                                                       */
  /* jfc.cpp    Version  2.03    Copyright (c) 1999-2000 Jan E. Mortensen  */
  /*                                                                       */
  /* JFS Compiler.                                                         */
  /*                                                                       */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                    */
  /*    Lollandsvej 35 3.tv.                                               */
  /*    DK-2000 Frederiksberg                                              */
  /*    Denmark                                                            */
  /*                                                                       */
  /*************************************************************************/

#ifdef __BCPLUSPLUS__
  #pragma hdrstop
  #include <condefs.h>
#endif

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jfs2wlib.h"
#include "jfw2rlib.h"

//---------------------------------------------------------------------------
#ifdef __BCPLUSPLUS__
  USEUNIT("..\..\COMMON\jfs_text.cpp");
USEUNIT("..\..\COMMON\jfw2rlib.cpp");
USEUNIT("..\..\COMMON\jfs2wlib.cpp");
//---------------------------------------------------------------------------
#pragma argsused
#endif

#define CM_SW   1
#define CM_WR   2
#define CM_SR   3
#define CM_SWR  4

struct jf_option_desc { const char *option;
                        int argc;      /* -1: variabelt */
                      };               /* -2: sidste argument */

struct jf_option_desc jf_options[] =
  {     {"-e", 1},         /* 0 */
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

const char usage_1[] =
  "usage: jfc [-o jfrf] [-e errf] [-em emode] [-so sout] [-s] [-a] ";
const char usage_2[] =
  "           [-m ctyp] [-mt mc] [-mw wc] [-ms ss] [-w m]           jfs";

const char bslash[] = "\\";

const char jfc_version[] =
      "JFC    version  2.03    Copyright (c) 1999-2000 Jan E. Mortensen";


const char *extensions[] = {
	"jfs",     /* 0 */
	"jfw",     /* 1 */
	"jfr"      /* 2 */
};

static int jfc_wait_mode = 0;

static void ext_subst(char *d, char *e, int forced);
static int isoption(char *s);
static int us_error(int silent_mode);
static int jf_about(void);
int jf_getoption(const char *argv[], int no, int argc);



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

static int isoption(char *s)
{
  if (s[0] == '-' || s[0] == '?')
    return 1;
  return 0;
}

static int us_error(int silent_mode)
{
   if (silent_mode == 0)
     printf("\n%s\n%s\n", usage_1, usage_2);
   return 1;
}

static int jf_about(void)
{
  char tmp[80];

  printf("\n\n%s\n\n", jfc_version);
  printf("by Jan E. Mortensen       email:  jemor@inet.uni2.dk\n\n");

  printf("usage: jfc [options] jfs\n\n");
  printf(
"JFC is the JFS compiler. It compiles the file <jfs>. Depending on the compile-\n");
  printf(
"mode it compiles from a jfs-file or a jfw-file, to a jfw-file or a jfr-file.\n\n");

  printf("OPTIONS\n");
  printf("-m <cm>      : compile-mode. <cm> in {'sr', 'sw', 'wr', 'swr'}.\n");
  printf("-s           : silent (don't write messages to stdout).\n");
  printf("-e <errf>    : write error messages to the file <errf>. \n");
  printf("-so <sof>    : redirect messages from stdout to <sof>.\n");
  printf("-a           : append to error-file/stdout-file.\n");
  printf("-em <m>      : error message format <m> in {'s' (standard), 'c'(compact)}.\n");
  printf("-o <jfrf>    : write the compiled program to the file <jfrf>.\n");
  printf("-mt <tc>     : <tc> is maximal number of chars in sentence.\n");
  printf("-mw <wc>     : <wc> is maximal number of words in sentence.\n");
  printf("-ms <ss>     : <ss> is size of expression stack.\n");
  printf("-w <m>       : <m>='y':wait for RETURN, 'n';dont wait, 'e':wait if errors.\n");
  if (jfc_wait_mode !=  0)
  { printf("Press RETURN to continue");
    fgets(tmp, 10, stdin);
  }
  return 0;
}

int jf_getoption(char *argv[], int no, int argc)
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

int main(int argc, char *argv[])
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
  if (argc == 1)
    return jf_about();
  if (argc == 2 && strcmp(argv[1], "-w") == 0)
  { jfc_wait_mode = 1;
    return jf_about();
  }
  strcpy(so_fname, argv[argc - 1]);
  if (isoption(so_fname) == 1)
    return us_error(jfc_silent_mode);
  for (m = 1; m < argc - 1; )
  { option_no = jf_getoption(argv, m, argc - 1);
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
       ext_subst(so_fname, extensions[0], 1);
       ext_subst(de_fname, extensions[1], 1);
       strcpy(tmp_fname, de_fname);
       break;
     case CM_WR:
       ext_subst(so_fname, extensions[1], 1);
       strcpy(tmp_fname, so_fname);
       ext_subst(de_fname, extensions[2], 1);
       break;
     case CM_SR:
       ext_subst(so_fname, extensions[0], 1);
       tmpnam(tmp_fname);
       ext_subst(tmp_fname, extensions[1], 1);
       ext_subst(de_fname, extensions[2], 1);
       el_mode = 1;
       break;
     case CM_SWR:
       ext_subst(so_fname, extensions[0], 1);
       strcpy(tmp_fname, de_fname);
       ext_subst(tmp_fname, extensions[1], 1);
       ext_subst(de_fname, extensions[2], 1);
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
  { fprintf(sout, "\n%s\n\n", jfc_version);
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

