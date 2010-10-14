  /*************************************************************************/
  /*                                                                       */
  /* cmds_common.c - Common subroutines for the command-line applications  */
  /*                             Copyright (c) 1999-2000 Jan E. Mortensen  */
  /*                                       Copyright (c) 2010 Miriam Ruiz  */
  /*                                                                       */
  /*************************************************************************/

#include "cmds_common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>

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
	int bracket = 0;
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
			bracket = 0;
		} else {
			switch (head[pos]) {
				case '[': case '<': bracket++; break;
				case ']': case '>': if (bracket>0) bracket--; break;
			}
			if (head[pos]==' ' && !bracket)
				last_space = pos;
			pos++;
		}
	}
			if (prefix!=NULL)
				fprintf(stream, "%s", prefix);
	fprintf(stream, "%s\n", head);
}

int jfscmd_num_of_columns()
{
	struct winsize ws;

	int c_num = 0;
	const char *c_str = getenv("COLUMNS");
	if (c_str != NULL)
		c_num = atoi(c_str);
	if (c_num > 1)
		return c_num;

	ioctl(1, TIOCGWINSZ, &ws); /* Will receive the number of columns (ws.ws_col) */
	                           /* and rows (ws.ws_row) in the tty */
	if (ws.ws_col > 1)
		return ws.ws_col;
	return 79;

	return c_num;
}

void jfscmd_print_about(const char *about[])
{
	int line = 0;
	int cols = jfscmd_num_of_columns();

	jfscmd_fprint_wrapped(stdout, cols, NULL, NULL, jfs_copyright);
	printf("\n");

	while (about[line]) {
		jfscmd_fprint_wrapped(stdout, cols, NULL, NULL, about[line]);
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

void jfscmd_ext_rm(char *d)
{
	int m;

	for (m = strlen(d) - 1; m > 0; m--)
	{
		if (d[m] == '.')
		{
			d[m] = '\0';
			break;
		}
	if (d[m] == '/' || d[m] == '\\')
		break;
	}
}

int jfscmd_tmap_find(struct jfscmd_tmap_desc *map, const char *txt)
{
	int m, res;
	res = -2;
	for (m = 0; res == -2; m++)
	{
		if (map[m].value == -1 || strcmp(map[m].text, txt) == 0)
		res = map[m].value;
	}
	return res;
}

void jfscmd_ftoa(char *txt, float f, int digits)
{
  char it[30] = "   ";
  char *t;
  int m, cif, mente, dp, ep, dl, sign, at;

  if (f < 0.0)
  { f = -f;
    sign = -1;
  }
  else
    sign = 1;

  t = &(it[1]);
  sprintf(t, "%20.10f", f);
  dl = strlen(it);
  dp = dl - 1;
  while (it[dp] != '.')
    dp--;
  mente = 0;
  ep = dp + digits - 1;
  for (m = dl - 1; m >= 0; m--)
  { if (it[m] != '.' && it[m] != ' ')
    { cif = it[m] - '0' + mente;
      if (cif == 10)
      { cif = 0;
        mente = 1;
      }
      else
        mente = 0;
      if (m > ep)
      { if (cif >= 5)
          mente = 1;
        it[m] = '\0';
      }
      else
       it[m] = cif + '0';
    }
    else
    if (it[m] == ' ' && mente == 1)
    { it[m] = '1';
      mente = 0;
    }
  }
  at = 0;
  if (sign == -1)
  { txt[0] = '-';
    at++;
  }
  for (m = 0; it[m] != '\0'; m++)
  { if (it[m] != ' ' && it[m] != '-')
    { txt[at] = it[m];
      at++;
    }
  }
  if (at == 0 || (at == 1 && txt[0] == '-'))
  { txt[0] = '0';
    at = 1;
  }
  txt[at] = '\0';
}

void jfscmd_ftoit(char *txt, float f)
{
  jfscmd_ftoa(txt, f, 0);
  if (txt[strlen(txt) - 1] == '.')
    txt[strlen(txt) - 1] = '\0';
}
