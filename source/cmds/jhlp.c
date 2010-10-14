  /*************************************************************************/
  /*                                                                       */
  /* jhlp.c - Converts a jhlp-system to HTML                               */
  /*                             Copyright (c) 1999-2000 Jan E. Mortensen  */
  /*                                       Copyright (c) 2010 Miriam Ruiz  */
  /*                                                                       */
  /*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cmds_common.h"
#include "jhlp_lib.h"

static const char usage[] =
	"jhlp [-o dest] [-e ef] [-h head] [-so sout] [-hi hif] [-a] [-w]"
	" [-s css] [-An n] [-Am m] [-si] [-c] <file.jhi>";

static const char *about[] = {
  "usage: jhlp [options] <file.jhi>",
  "",
  "JHLP converts the jhlp-system described by <file.jhi> to HTML.",
  "",
  "Options:",
  "-o <dest>    : Write the html-system to the file <dest>.",
  "-e <errf>    : write error messages to the file <errf>. ",
  "-so <sof>    : redirect messages from stdout to <sof>.",
  "-a           : append to error-file/stdout-file.",
  "-h  <head>   : Convert only the subsystem defined by <head> (to a file).",
  "-hi <jhc>    : Add the new jhc-file to <jhi> before convertion.",
  "-s <css>     : Link all html-files to the stylesheet-file <css>.",
  "-An <n>      : Allocate <n> nodes to temporary data.",
  "-Am <m>      : Allocate <m> K to temporary data.",
  "-w           : wait for RETURN.",
  "-si          : silent.",
  "-c           : copy image-files etc to destination-directory.",
  NULL
};

struct jfscmd_option_desc jf_options[] =
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

static const char bslash[] = "\\";

static const char *extensions[]  = {
                        "jhi",     /* 0 */
                        "jhc",     /* 1 */
                        "htm"      /* 2 */
                      };

static int us_error(void)
{
  jfscmd_fprint_wrapped(stdout, jfscmd_num_of_columns() - 7, "usage: ", "       ", usage);
  return 1;
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
  {
    jfscmd_print_about(about);
    return 0;
  }
  strcpy(jhi_fname, argv[argc - 1]);
  if (jfscmd_isoption(jhi_fname) == 1)
    return us_error();
  for (m = 1; m < argc - 1; )
  { option_no = jfscmd_getoption(jf_options, argv, m, argc - 1);
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
  jfscmd_ext_subst(jhi_fname, extensions[0], 1);
  strcpy(dest_dir, jhi_fname);
  for (m = strlen(dest_dir) - 1;
       m > 0 && dest_dir[m] != '\\' && dest_dir[m] != '/'; m--)
    dest_dir[m] = '\0';
  if (strlen(new_so_fname) != 0)
    jfscmd_ext_subst(new_so_fname, extensions[1], 1);
  if (strlen(head_name) != 0)
  { if (strlen(de_fname) == 0)
      strcpy(de_fname, jhi_fname);
    jfscmd_ext_subst(de_fname, extensions[2], 1);
  }
  if (strlen(stylesheet) != 0)
    jfscmd_ext_subst(stylesheet, ".css", 0);
  if (strlen(sout_fname) != 0)
  { if (append_mode == 0)
      sout = fopen(sout_fname, "w");
    else
      sout = fopen(sout_fname, "a");
    if (sout == NULL)
      sout = stdout;
  }

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
