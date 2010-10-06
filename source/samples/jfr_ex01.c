/*********************************************************************/
/*                                                                   */
/* This example shows how to load and run a compiled jfs-program     */
/* from a C-program.                                                 */
/*                                                                   */
/*********************************************************************/

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

#ifdef __BCPLUSPLUS__
  /* The folowing lines are only needed if the program is compiled with */
  /* Borland C++Builder.                                                */
  USEUNIT("..\..\COMMON\Jfr_lib.cpp");
  #pragma argsused
#endif

float ip[10];                /* input values  */
float op[10];                /* output values */
char fname[] = "truck.jfr";  /* filename      */
void *head;                  /* jfs-program   */

int main(int argc, char *argv[])
{
   int res;

   res = jfr_init(0);
   if (res != 0)
   { printf("Errors in initilising the jfr-library. Error-code: %d\n", res);
     exit(1);
   }
   res = jfr_load(&head, fname);
   if (res != 0)
   { printf("Cannot load the jfs-program: %s. Error-code: %d\n", fname, res);
     jfr_free();
     exit(1);
   }

   ip[0] = 40.0;  /* x-value   */
   ip[1] = 108.0; /* phi-value */

   printf("Input : x = %-.2f, phi = %-.2f\n", ip[0], ip[1]);
   jfr_run(op, head, ip);
   printf("Output: theta = %-.2f\n", op[0]);

   jfr_close(head);
   jfr_free();
   return 0;
}


