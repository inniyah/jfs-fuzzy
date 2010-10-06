/***************************************************************************/
/*                                                                         */
/* jhlp.cpp        Version 1.02  Copyright (c) 1999-2000 Jan E. Mortensen  */
/*                                                                         */
/* Converts a jhlp-system to html.                                         */
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
#include "jhlp_lib.h"

#ifdef __BCPLUSPLUS__
//---------------------------------------------------------------------------
USEUNIT("..\..\COMMON\jhlp_lib.cpp");
USEUNIT("..\..\COMMON\jfm_lib.cpp");
//---------------------------------------------------------------------------
  #pragma argsused
#endif

struct jf_option_desc {
	const char *option;
	int argc;      /* -1: variabelt */
};               /* -2: sidste argument */

struct jf_option_desc jf_options[] =
  {     {"-e", 1},         /* 0 */
        {"-h", 1},         /* 1 */
        {"-o", 1},         /* 2 */
        {"-a", 0},         /* 3 */
        {"-so",1},         /* 4 */
        {"-w", 0},         /* 5 */
        {"-hi",1},         /* 6 */
        {"-s", 1},         /* 7 */
        {"-An",1},         /* 8 */
        {"-Am",1},         /* 9 */
        {"-si",0},         /*10 */
        {"-c", 0},         /*11 */
        {"-?", 0},
        {"?",  0},
        {" ", -2}
   };

const char usage_1[] =
 "usage: jhlp [-o dest] [-e ef] [-h head] [-so sout] [-hi hif] [-a] [-w]";
const char usage_2[] =
 "            [-s css] [-An n] [-Am m] [-si] {-c]                        hif";

const char bslash[] = "\\";

const char jfh_version[] =
      "JHLP    version  1.02   Copyright (c) 1999-2000 Jan E. Mortensen";

const char *extensions[]  = { "jhi",     /* 0 */
                        "jhc",     /* 1 */
                        "htm"      /* 2 */
                      };

static void ext_subst(char *d, const char *e, int forced);
static int isoption(const char *s);
static int us_error(void);
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

static int isoption(const char *s)
{
  if (s[0] == '-' || s[0] == '?')
    return 1;
  return 0;
}

static int us_error(void)
{
   printf("\n%s\n%s\n", usage_1, usage_2);
   return 1;
}

static int jf_about(void)
{
  printf("\n\n%s\n\n", jfh_version);

  printf("usage: jhlp [options] jhi\n\n");
  printf(
"JHLP converts the jhlp-system described by <jhi> to html.\n\n");

  printf("OPTIONS\n");
  printf("-o <dest>    : Write the html-system to the file <dest>.\n");
  printf("-e <errf>    : write error messages to the file <errf>. \n");
  printf("-so <sof>    : redirect messages from stdout to <sof>.\n");
  printf("-a           : append to error-file/stdout-file.\n");
  printf("-h  <head>   : Convert only the subsystem defined by <head> (to a file).\n");
  printf("-hi <jhc>    : Add the new jhc-file to <jhi> before convertion.\n");
  printf("-s <css>     : Link all html-files to the stylesheet-file <css>.\n");
  printf("-An <n>      : Allocate <n> nodes to temporary data.\n");
  printf("-Am <m>      : Allocate <m> K to temporary data.\n");
  printf("-w           : wait for RETURN.\n");
  printf("-si          : silent.\n");
  printf("-c           : copy image-files etc to destination-directory.\n");
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

int main(int argc, const char *argv[])
{
  int m, option_no, append_mode;
  int wait_mode, silent, copy_mode;
  long nodes, mem;
  FILE *sout;
  char jhi_fname[256];
  char er_fname[256];
  char de_fname[256];
  char sout_fname[256];
  char head_name[256];
  char new_so_fname[256];
  char dest_dir[256];
  char stylesheet[256];

  jhi_fname[0] = er_fname[0] = head_name[0]
              = de_fname[0] = sout_fname[0] = new_so_fname[0] = '\0';
  sout = stdout;
  stylesheet[0] = '\0';
  wait_mode = 0;
  silent = 0;
  copy_mode = 0;
  append_mode = 0;
  nodes = 0; mem = 0;
  if (argc == 1)
    return jf_about();
  strcpy(jhi_fname, argv[argc - 1]);
  if (isoption(jhi_fname) == 1)
    return us_error();
  for (m = 1; m < argc - 1; )
  { option_no = jf_getoption(argv, m, argc - 1);
    if (option_no == -1)
      return us_error();
    m++;
    switch (option_no)
    { case 0:                         /* -e */
        strcpy(er_fname, argv[m]);
        m++;
        break;
      case 1:                         /* -h */
        strcpy(head_name, argv[m]);
        m++;
        break;
      case 2:                         /* -o */
        strcpy(de_fname, argv[m]);
        m++;
        break;
      case 3:                         /* -a */
        append_mode = 1;
        break;
      case 4:                         /* -so */
        strcpy(sout_fname, argv[m]);
        m++;
        break;
      case 5:                         /* -w */
        wait_mode = 1;
        break;
      case 6:                        /* -hi */
        strcpy(new_so_fname, argv[m]);
        m++;
        break;
      case 7:                        /* -s */
        strcpy(stylesheet, argv[m]);
        m++;
        break;
      case 8:                        /* -An */
        nodes = atoi(argv[m]);
        m++;
        break;
     case  9:                        /* -Am */
        mem = 1024 * ((long) atoi(argv[m]));
        m++;
        break;
      case 10:                       /* -si */
        silent = 1;
        break;
      case 11:                       /* -c */
        copy_mode = 1;
        break;
      default:
        return us_error();
    }
  }  /* for  */

  if (strlen(jhi_fname) == 0)
    return us_error();
  ext_subst(jhi_fname, extensions[0], 1);
  strcpy(dest_dir, jhi_fname);
  for (m = strlen(dest_dir) - 1;
       m > 0 && dest_dir[m] != '\\' && dest_dir[m] != '/'; m--)
    dest_dir[m] = '\0';
  if (strlen(new_so_fname) != 0)
    ext_subst(new_so_fname, extensions[1], 1);
  if (strlen(head_name) != 0)
  { if (strlen(de_fname) == 0)
      strcpy(de_fname, jhi_fname);
    ext_subst(de_fname, extensions[2], 1);
  }
  if (strlen(stylesheet) != 0)
    ext_subst(stylesheet, ".css", 0);
  if (strlen(sout_fname) != 0)
  { if (append_mode == 0)
      sout = fopen(sout_fname, "w");
    else
      sout = fopen(sout_fname, "a");
    if (sout == NULL)
      sout = stdout;
  }
  fprintf(sout, "\n%s\n\n", jfh_version);
  fprintf(sout, "Creating html-files from: %s\n\n", jhi_fname);

  if (strlen(sout_fname) != 0)
    fclose(sout);
  if (strlen(er_fname) == 0)
  { if (strlen(sout_fname) != 0)
    { strcpy(er_fname, sout_fname);
      append_mode = 1;
    }
  }

  m = jhlp_convert(de_fname, jhi_fname, new_so_fname, stylesheet,
                   head_name, er_fname, append_mode, copy_mode,
                   nodes, mem, silent);

  if (strlen(sout_fname) != 0)
    sout = fopen(sout_fname, "a");

  if (strlen(new_so_fname) != 0)
    strcpy(dest_dir, new_so_fname);
  if (m == 0)
    fprintf(sout, "Success. Html-file(s) written to: %s.\n\n", dest_dir);
  else
  if (m == 1)
    fprintf(sout, "Errors. Nothing written to: %s.\n", dest_dir);
  else
    fprintf(sout, "Error when writting to %s. File(s) damaged!\n", dest_dir);
  if (wait_mode == 1)
  { printf("Press RETURN ...");
    fgets(de_fname, 78, stdin);
  }
  if (sout != stdout)
    fclose(sout);
  return m;
}
