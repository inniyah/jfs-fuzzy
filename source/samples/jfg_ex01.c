/**************************************************************************/
/*                                                                        */
/* This example shows how to get some general information about a         */
/* jfs-program using the jfg_lib-function jfg_spgr.                       */
/*                                                                        */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jfr_lib.h"
#include "jfg_lib.h"

void *head;                   /* jfr-program   */
struct jfg_sprog_desc spdesc; /* describes of the jfs-program */

int main(int argc, char *argv[])
{
  char fname[] = "truck.jfr";  /* filename      */
  int res;

  res = jfr_init(0);
  if (res != 0)
  { printf("Error in initialisation of jfr-library. Error code: %d\n", res);
    exit(1);
  }

  res = jfg_init(JFG_PM_NORMAL, 0, 0);
  if (res != 0)
  { jfr_free();
    printf("Error in initialisation of jfg_lib. Error code: %d\n", res);
    exit(1);
  }
  res = jfr_load(&head, fname);
  if (res != 0)
  { printf("Cannot load jfs-program: %s. Error-code: %d\n", fname, res);
    jfg_free();
    jfr_free();
    exit(1);
  }

  jfg_sprg(&spdesc, head);   /* get general information about program */

  printf("name of program ....: %s\n", spdesc.title);
  printf("number of variables : %d\n", spdesc.var_c);
  printf("number of hedges ...: %d\n", spdesc.hedge_c);
  printf("number of operators : %d\n", spdesc.operator_c);

  jfr_close(head);
  jfg_free();
  jfr_free();

  return 0;
}
