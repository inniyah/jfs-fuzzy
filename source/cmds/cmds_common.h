  /*************************************************************************/
  /*                                                                       */
  /* cmds_common.h - Common subroutines for the command-line applications  */
  /*                             Copyright (c) 1999-2000 Jan E. Mortensen  */
  /*                                       Copyright (c) 2010 Miriam Ruiz  */
  /*                                                                       */
  /*************************************************************************/

#ifndef CMDS_COMMON_H__
#define CMDS_COMMON_H__

#include <stdio.h>

struct jfscmd_option_desc {
	const char *option;
	int argc;
};

struct jfscmd_tmap_desc {
	int value;
	const char *text;
};

extern const char jfs_copyright[];

int jfscmd_isoption(const char *s);
int jfscmd_getoption(const struct jfscmd_option_desc *options, const char *argv[], int no, int argc);
int jfscmd_num_of_columns();
void jfscmd_fprint_wrapped(FILE *stream, int line_size, const char *first_prefix, const char *prefix, const char *text);
void jfscmd_print_about(const char *about[]);
void jfscmd_ext_subst(char *d, const char *e, int forced);
void jfscmd_ext_rm(char *d);
int jfscmd_tmap_find(struct jfscmd_tmap_desc *map, const char *txt);

#endif
