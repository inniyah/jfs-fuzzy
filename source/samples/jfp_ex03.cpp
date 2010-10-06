/**************************************************************************/
/*                                                                        */
/* This example uses jfp_lib to change the weight-value of statements of  */
/* the type 'ifw %<w> ....;'                                              */
/*                                                                        */
/* Usage:  jfp_ex03 <jfrf>                                                */
/*                                                                        */
/* the changed program is written to <jfrf> (overwriting the original     */
/* code).                                                                 */
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
  #pragma argsused
  USEUNIT("..\..\COMMON\Jfp_lib.cpp");
  USEUNIT("..\..\COMMON\Jfg_lib.cpp");
  USEUNIT("..\..\COMMON\Jfr_lib.cpp");
#endif

void *head;                       /* jfs-program   */
char ext[] = ".jfr";

unsigned char *pcs[100];  /* Remember program-addresses of 'ifw %'-      */
                     			  /*  statements.                                */
int ff_pcs;               /* First free post in pcs[].                   */

void w_change(void)       /* randomly change a weight.                   */
{
  struct jfg_statement_desc sdesc;
  int sno;
  float w, a;

  sno = random(ff_pcs);
  jfg_statement(&sdesc, head, pcs[sno]);
  w = sdesc.farg;
  a = ((float) random(1000) - 500.0) / 10000.0;  /* a random number in  */
						 /* [-0.05, 0.05]       */
  w += a;
  if (w < 0.0)
    w = 0.0;
  if (w > 1.0)
    w = 1.0;

  sdesc.farg = w;
  jfp_statement(pcs[sno], &sdesc);
}


main(int argc, char *argv[])
{
  struct jfg_statement_desc sdesc;
  struct jfg_function_desc fudesc;
  struct jfg_sprog_desc pdesc;
  int m, res, funcno;
  unsigned char *pc;
  char fname[256];

  if (argc != 2)
  { printf("Usage: jfp_ex03 jfrf\n");
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
  { jfg_sprg(&pdesc, head);
    ff_pcs = 0;
    /* step through all functions and the main-part of the jfs-program: */
    for (funcno = 0; funcno <= pdesc.function_c; funcno++)
    { if (funcno < pdesc.function_c)
      { jfg_function(&fudesc, head, funcno);
        pc = fudesc.pc;  /* first statement function */
      }
      else
        pc = pdesc.pc_start;  /* first statement main-program */
      jfg_statement(&sdesc, head, pc);
      /* step through the function/main-program: */
      while (sdesc.type != JFG_ST_EOP &&
             !(sdesc.type == JFG_ST_STEND && sdesc.sec_type == 2))
      { if (sdesc.type == JFG_ST_IF)
        { if ((sdesc.flags & 1)     /* ifw-statement.                  */
	             && (sdesc.flags & 2)  /* % before weight.                */
	             && ff_pcs < 100)
         	{
         	  pcs[ff_pcs] = pc;
            ff_pcs++;
         	}
        }
        pc = sdesc.n_pc;
        jfg_statement(&sdesc, head, pc);
      }
    }

    if (ff_pcs != 0)
    for (m = 0; m < 100; m++)    /* make 100 random changes to weights.   */
      w_change();

    res = jfp_save(fname, head);
    if (res != 0)
      printf("Error writing to the file %s. Errcode: %d\n", fname, res);

    jfr_close(head);
    jfr_free();
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


