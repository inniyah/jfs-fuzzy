/**************************************************************************/
/*                                                                        */
/* This simple example changes the operator-type of the 'and'-operator to */
/* 'yager_and %2.5' in the program 'truck.jfr,' The changed program is    */
/* written to the file 'ntruck.jfr'.                                      */
/*                                                                        */
/**************************************************************************/
#ifdef __BCPLUSPLUS__
  /* The folowing lines are only needed if the program is compiled with */
  /* Borland C++Builder.                                                */
  #pragma hdrstop
  #include <condefs.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jfr_lib.h"
#include "jfg_lib.h";
#include "jfp_lib.h"

#ifdef __BCPLUSPLUS__
  /* The folowing lines are only needed if the program is compiled with */
  /* Borland C++Builder.                                                */
  USEUNIT("..\..\COMMON\Jfr_lib.cpp");
  USEUNIT("..\..\COMMON\Jfp_lib.cpp");
  USEUNIT("..\..\COMMON\Jfg_lib.cpp");
  #pragma argsused
#endif

void *head;                   /* jfr-program   */


int main(int argc, char *argv[])
{
  char fname[] = "truck.jfr";     /* filename                 */
  char nname[] = "ntruck.jfr";    /* filename changed program */
  struct jfg_operator_desc odesc;
  int res;

  res = jfg_init(JFG_PM_NORMAL, 0, 0);
  if (res != 0)
  { printf("Error in jfg_init. errcode %d\n", res);
    return 1;
  }
  res = jfp_init(0);
  if (res != 0)
  { printf("Error in jfp_init(). Errcode %d\n", res);
    jfg_free();
    return 1;
  }
  res = jfr_init(0);
  if (res != 0)
  { printf("Error in jfr_init(). Errcode %d\n", res);
    jfp_free();
    jfg_free();
    return 1;
  }

  res = jfr_load(&head, fname);
  if (res != 0)
  { printf("Error loading program. Error code %d\n", res);
    jfr_free();
    jfp_free();
    jfg_free();
  }

  jfg_operator(&odesc, head, JFS_ONO_AND); /* get operator and */

  odesc.op_1 = JFS_FOP_YAGERAND;
  odesc.op_2 = JFS_FOP_YAGERAND;
  odesc.op_arg = 2.5;
  odesc.flags |= JFS_OF_IARG;   /* '%' in front of argument; */

  jfp_operator(head, JFS_ONO_AND, &odesc);

  res = jfp_save(nname, head);
  if (res != 0)
    printf("Error writing to the file %s. Errcode: %d\n", nname, res);

  jfr_close(head);
  jfr_free();
  jfp_free();
  jfg_free();

  return 0;
}


