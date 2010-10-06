  /***********************************************************************/
  /*                                                                     */
  /* jfopt_lib.cpp  Version  1.00   Copyright (c) 2000 Jan E. Mortensen  */
  /*                                                                     */
  /* Decodes an option-string.                                           */
  /*                                                                     */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                  */
  /*    Lollandsvej 35 3.tv.                                             */
  /*    DK-2000 Frederiksberg                                            */
  /*    Denmark                                                          */
  /*                                                                     */
  /***********************************************************************/

#include <stdio.h>
#include <string.h>
#include "jopt_lib.h"

static int jopt_cur_word;
static int jopt_cur_char;
static char jopt_sign;
static struct jopt_desc *jopt_options;
static int jopt_opt_c;
static const char **jopt_argv;
static int jopt_argc;
struct jopt_error_rec jopt_error_desc;

int jopt_error(int emode, int error_no, const char *argument)
{
  jopt_error_desc.error_no = error_no;
  strcpy(jopt_error_desc.argument, argument);
  jopt_error_desc.error_mode = emode;
  return -1;
}

int jopt_init(struct jopt_desc *options, int opt_c,
              const char *argv[], int argc)
{
  jopt_error(JOPT_EM_NONE, 0, "no errors");
  jopt_options = options;
  jopt_opt_c = opt_c;
  jopt_argv = argv;
  jopt_argc = argc;

  jopt_cur_word = 1;  
  jopt_cur_char = 0;
  jopt_sign = 0;
  return 0;
}

static int jopt_match(char *txt)
{
  int m;

  for (m = 0; m < jopt_opt_c; m++)
  { if (strcmp(txt, jopt_options[m].txt) == 0)
      return m;
  }
  return -1;
}

static int jopt_get_args(const char *aargv[])
{
  int ac, slut;
  char s;

  ac = 0;
  slut = 0;
  while (slut == 0)
  { if (jopt_cur_word >= jopt_argc)
      slut = 1;
    else
    { s = jopt_argv[jopt_cur_word][0];
      if (s == '-' || s == '+')
        slut = 1;
      else
      { aargv[ac] = jopt_argv[jopt_cur_word];
        jopt_cur_word++;
        ac++;
      }
    }
  }
  return ac;
}

int jopt_get(unsigned short *value, const char *aargv[], int *aargc)
{
  int res, ac, id, id1, id2;
  const char *word;
  char otxt[4];

  res = 2;
  while (res == 2)
  { if (jopt_cur_word >= jopt_argc)
      return 1; /* end of words */
    word = jopt_argv[jopt_cur_word];
    if (word[jopt_cur_char] == '\0')
    { jopt_cur_word++;
      jopt_cur_char = 0;
      jopt_sign = 0;
    }
    else
    if (jopt_sign == 0)
    { jopt_sign = word[jopt_cur_char];
      jopt_cur_char++;
      if (jopt_sign != '+' && jopt_sign != '-')
      { jopt_sign = 0;
        return jopt_error(JOPT_EM_ERROR, 51, word);
      }
    }
    else
    { otxt[0] = jopt_sign;
      otxt[1] = word[jopt_cur_char];
      otxt[2] = '\0';
      id1 = jopt_match(otxt);
      id2 = -1;
      jopt_cur_char++;
      if (word[jopt_cur_char] != '\0')
      { otxt[2] = word[jopt_cur_char];
        otxt[3] = '\0';
        id2 = jopt_match(otxt);
      }
      if (id1 == -1 && id2 == -1)
        return jopt_error(JOPT_EM_ERROR, 51, word);
      if (id1 == 1 && id2 == 1)
      { id1 = -1; /* conflict_resolve ?? */
      }
      if (id1 != -1)
        id = id1;
      else
      { id = id2;
        jopt_cur_char++;
      }
      ac = 0;
      if (word[jopt_cur_char] == '\0')
      { jopt_cur_char = 0;
        jopt_cur_word++;
        jopt_sign = 0;
        ac = jopt_get_args(aargv);
      }
      if (ac < jopt_options[id].min_argc)
        return jopt_error(JOPT_EM_ERROR, 52, jopt_options[id].txt);
      if (ac > jopt_options[id].max_argc)
        return jopt_error(JOPT_EM_ERROR, 53, jopt_options[id].txt);
      res = 0;
      *aargc = ac;
      *value = jopt_options[id].value;
    }
  }
  return res;
}



