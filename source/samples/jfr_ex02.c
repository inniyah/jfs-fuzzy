
/*********************************************************************/
/*                                                                   */
/* This example shows how to load and run a compiled jfs-program     */
/* from a C-program using the jfr_arun() function. The mcall()       */
/* function is used to print error-messages. The call function is    */
/* only used to write a 'hello-world'-like text on the screen.       */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jfr_lib.h"

void *head;                  /* the jfs-program  */
float ip[10];                /* input values     */
float op[10];                /* output values    */
char fname[] = "truck.jfr";  /* filename         */

void dumcall(void)
{
  struct jfr_stat_desc sdesc;

  jfr_statement_info(&sdesc);
  printf("Call-statement in statement number: %d ignored!\n",
          sdesc.rule_no);
}

void errchk(int place)
{
  int err;
  struct jfr_stat_desc sdesc;

  err = jfr_error(); /* Note: the error-value is cleared by the
                        function-call.                            */
  if (err != 0)
  { switch (place)
    { case 0: /* before first statement */
       	printf("Error in initial fuzzification. Errcode: %d\n", err);
       	break;
      case 1: /* error in statement     */
        jfr_statement_info(&sdesc);
       	printf("Error in statement number %d. Errcode: %d\n",
               sdesc.rule_no, err);
       	break;
      case 2: /* error in final defuzzification */
       	printf("Error in final defuzzication. Errcode: %d\n", err);
       	break;
    }
  }
}

int main(int argc, char *argv[])
{
   int res;

   res = jfr_init(0);
   if (res != 0)
   { printf("Error in initialisation of jfr-library. Error code: %d\n", res);
     exit(1);
   }

   res = jfr_load(&head, fname);
   if (res != 0)
   { printf("Cannot load jfs-program: %s. Error-code: %d\n", fname, res);
     jfr_free();
     exit(1);
   }

   ip[0] = 40.0;  /* x-value   */
   ip[1] = 108.0; /* phi-value */

   printf("Input : x = %-.2f, phi = %-.2f\n", ip[0], ip[1]);

   jfr_arun(op, head, ip, NULL,     /* output, program, input, confidences */
            dumcall, errchk,        /* extern-handler, debug-handler,      */
            NULL);                  /* undef-handler.                      */
            
   printf("Output: theta = %-.2f\n", op[0]);

   jfr_close(head);
   jfr_free();
   return 0;
}
