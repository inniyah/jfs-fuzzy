/**************************************************************************/
/*                                                                        */
/* This example uses jfp_lib to change fuzzification functions.           */
/* Usage:  jfp_ex02 <jfrf>                                                */
/* The program finds the variables with 'argument=117'. For adjectives,   */
/* bound to such a variable, the program asks the user for the            */
/* fuzzification-function. The program assumes the fuzzification-function */
/* is a trapeziod pl-function. Let for example the local variable 'price' */
/* be defined as:                                                         */
/*              price "Price" dollar flag 3;                              */
/* with the single adjective:                                             */
/*              price correct  100:0 150:1 400:1 600:0;                   */
/*                                                                        */
/* then the user is asked when the price is to low, correct (min and max) */
/* and to large.                                                          */
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
#include "jfg_lib.h"
#include "jfp_lib.h"

#ifdef __BCPLUSPLUS__
  /* The folowing lines are only needed if the program is compiled with */
  /* Borland C++Builder.                                                */
  USEUNIT("..\..\COMMON\Jfr_lib.cpp");
  USEUNIT("..\..\COMMON\Jfp_lib.cpp");
  USEUNIT("..\..\COMMON\Jfg_lib.cpp");
  #pragma argsused
#endif


void *head;                       /* jfs-program   */
struct jfg_sprog_desc spdesc;
char ext[] = ".jfr";

main(int argc, char *argv[])
{
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;
  struct jfg_limit_desc limits[4];
  int v, a, res;
  char fname[80];
  char iptext[80];

  if (argc != 2)
  { printf("Usage: jfp_ex02 jfrf\n");
    return 1;
  }
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
  strcpy(fname, argv[1]);
  res = jfr_load(&head, fname);
  if (res != 0)
  { strcat(fname, ext);
    res = jfr_load(&head, fname);
  }
  if (res == 0)
  {
    jfg_sprg(&spdesc, head);
    for (v = 0; v < spdesc.var_c; v++)
    { jfg_var(&vdesc, head, v);
      if (vdesc.argument == 117)
      { for (a = 0; a < vdesc.fzvar_c; a++)
        { jfg_adjectiv(&adesc, head, vdesc.f_adjectiv_no + a);
          if (adesc.limit_c == 4)
          { jfg_alimits(limits, head, vdesc.f_adjectiv_no + a);
            printf("\n%s definitiv to low:", vdesc.name);
            gets(iptext);
            limits[0].limit = atof(iptext);
            printf("\n%s lowest correct value:", vdesc.name);
            gets(iptext);
            limits[1].limit = atof(iptext);
            printf("\n%s highest correct value:", vdesc.name);
            gets(iptext);
            limits[2].limit = atof(iptext);
            printf("\n%s definitiv to high:", vdesc.name);
            gets(iptext);
            limits[3].limit = atof(iptext);
            jfp_alimits(head, vdesc.f_adjectiv_no + a, limits);
          }
        }
      }
    }
    res = jfp_save(fname, head); /* in a realistic aplication, instead of  */
	                            			 /* saving the program, the changed program*/
                            				 /* would be used to judge a set of data.  */
    if (res != 0)
      printf("Error writing to the file %s. Errcode: %d\n", fname, res);

    jfr_close(head);
    jfp_free();
    jfg_free();
  }
  else
  { printf("Error loading program. Error code %d\n", res);
    jfg_free();
    jfp_free();
  }
  return 0;
}


