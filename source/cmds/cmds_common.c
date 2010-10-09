  /*************************************************************************/
  /*                                                                       */
  /* cmds_common.c - Common subroutines for the command-line applications  */
  /*                             Copyright (c) 1999-2000 Jan E. Mortensen  */
  /*                                       Copyright (c) 2000 Miriam Ruiz  */
  /*                                                                       */
  /*************************************************************************/

#include "cmds_common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const char jfs_copyright[] =
	"JFS - Jan's Fuzzy System - Copyright (c) 1999-2000 Jan E. Mortensen";

int jfscmd_isoption(const char *s)
{
	if (s[0] == '-' || s[0] == '?')
		return 1;
	return 0;
}

int jfscmd_getoption(const struct jfscmd_option_desc *options, const char *argv[], int no, int argc)
{
  int m, v, res;

  res = -2;
  for (m = 0; res == -2; m++)
  { if (options[m].argc == -2)
      res = -1;
    else
    if (strcmp(options[m].option, argv[no]) == 0)
    { res = m;
      if (options[m].argc > 0)
      { if (no + options[m].argc >= argc)
          res = -1; /* missing arguments */
        else
        { for (v = 0; v < options[m].argc; v++)
          { if (jfscmd_isoption(argv[no + 1 + v]) == 1)
              res = -1;
          }
        }
      }
    }
  }
  return res;
}

void jfscmd_fprint_wrapped(FILE *stream, int line_size, const char *first_prefix, const char *prefix, const char *text)
{
	const char *head = text;
	int pos = 0, last_space = 0;
	while (head[pos] !=0 ) {
		int is_eol = (head[pos] == '\n');
		if (is_eol || pos == line_size) {
			if (is_eol || last_space == 0)
				last_space = pos;  /* just cut it */
			if (prefix!=NULL) {
				if ( head == text)
					fprintf(stream, "%s", first_prefix);
				else
					fprintf(stream, "%s", prefix);
			}
			while (*head != 0 && last_space-- > 0)
				fprintf(stream, "%c", *head++);
			fprintf(stream, "\n");
			if (is_eol)
				head++; /* jump the line feed */
			while (*head!=0 && *head==' ')
				head++;  /* clear the leading space */
			last_space = pos = 0;
		} else {
			if (head[pos]==' ')
				last_space = pos;
			pos++;
		}
	}
			if (prefix!=NULL)
				fprintf(stream, "%s", prefix);
	fprintf(stream, "%s\n", head);
}

void jfscmd_print_about(const char *about[])
{
	int line = 0;

	jfscmd_fprint_wrapped(stdout, 76, NULL, NULL, jfs_copyright);
	printf("\n");

	while (about[line]) {
		jfscmd_fprint_wrapped(stdout, 76, NULL, NULL, about[line]);
		line++;
	}
}

void jfscmd_ext_subst(char *d, const char *e, int forced)
{
	int m, fundet;
	char punkt[] = ".";

	fundet = 0;
	for (m = strlen(d) - 1; m >= 0 && fundet == 0 ; m--)
	{
		if (d[m] == '.')
		{
			fundet = 1;
			if (forced == 1)
				d[m] = '\0';
		}
	}
	if (fundet == 0 || forced == 1)
	{
		if (strlen(e) != 0)
		strcat(d, punkt);
		strcat(d, e);
	}
}
