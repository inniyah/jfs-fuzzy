/**************************************************************************/
/*                                                                        */
/* This example uses the variable-functions in jfr_lib and jfg_lib        */
/* to produce a simple plot file, to be plotted by the public domain      */
/* program GNU-PLOT. The plot file shows the fuzzification-functions of   */
/* adjectives bound to a specified variable.                              */
/*                                                                        */
/* usage: jfg_ex03 progname varname                                       */
/*                                                                        */
/* jfg_ex03 will create an ascii-file with the name <varname>.dat         */
/* to be plotted from gnu-plot by the command:                            */
/* plot "<varname>.dat"                                                   */
/*                                                                        */
/**************************************************************************/
#ifdef __BCPLUSPLUS__
  /* The folowing lines are only needed if the program is compiled with */
  /* Borland C++Builder.                                                */
  #pragma hdrstop
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jfr_lib.h"
#include "jfg_lib.h"

#ifdef __BCPLUSPLUS__
  /* The folowing lines are only needed if the program is compiled with */
  /* Borland C++Builder.                                                */
  #include <condefs.h>
  #pragma argsused
  USEUNIT("..\..\COMMON\Jfr_lib.cpp");
  USEUNIT("..\..\COMMON\Jfg_lib.cpp");
#endif

void *head;                   /* jfs-program   */
FILE *df;                     /* the plot data file */
struct jfg_sprog_desc spdesc; /* describes of the jfs-program */




int find_var(char *varname)    /* returns the identification-number of the */
			       /* variable <varname> or -1 if no variable  */
{                              /* has this name.                           */
  int v, var_no;
  struct jfg_var_desc vdesc;

  var_no = -1;
  for (v = 0; var_no == -1 && v < spdesc.var_c; v++)
  { jfg_var(&vdesc, head, v);
    if (strcmp(vdesc.name, varname) == 0)
      var_no = v;
  }
  return var_no;
}



void vplot(int vno)
{
  struct jfg_var_desc vdesc;
  struct jfg_domain_desc ddesc;
  struct jfg_adjectiv_desc adesc;
  int a, m;
  float min_val, max_val, x, y, factor;

  jfg_var(&vdesc, head, vno);
  if (vdesc.fzvar_c == 0)
  { printf("No adjectives bound to the variable %s.\n", vdesc.name);
    return ;
  }

  /* find minimum and maximum value */
  jfg_domain(&ddesc, head, vdesc.domain_no);
  if (ddesc.flags & JFS_DF_MINENTER)
    min_val = ddesc.dmin;
  else
  { /* if minimum-value is not specied in domain use the center value */
    /* of the first adjectiv as minimum.                              */
    jfg_adjectiv(&adesc, head, vdesc.f_adjectiv_no);
    min_val = adesc.center;
  }
  if (ddesc.flags & JFS_DF_MAXENTER)
    max_val = ddesc.dmax;
  else
  { /* if no maximum in domain use the center value of last adjectiv as */
    /* maximum.                                                         */
    jfg_adjectiv(&adesc, head, vdesc.f_adjectiv_no + vdesc.fzvar_c - 1);
    max_val = adesc.center;
  }
  factor = (max_val - min_val ) / 100.0; /* 100 points in [min_val,max_val] */

  for (a = 0; a < vdesc.fzvar_c; a++)
  { /* for each adjectiv plot 200 points */
    for (m = 0; m <= 100; m++)
    { x = min_val + ((float) m) * factor;
      jfr_vput(vno, x, 1.0);  /* assign the variable the value <x> */
                      				    /* with confidence 1.0.              */

      y = jfr_fzvget(vdesc.f_fzvar_no + a);
				   /* The value of adjectiv <a> in <x>   */
				   /* is returned from the fuzzy-variable*/
				   /* bound to <a>.                      */
      fprintf(df, "%10.4f %6.4f\n", x, y);
    }
    fprintf(df, "\n");  /* blankline to start new gnu-plot mesh */
  }
}

int about(void)
{
  printf("usage: jfg_ex03 progname varname\n");
  return 1;
}

main(int argc, char *argv[])
{
   int res, var_no;
   char fname[80];
   char ext[5] = ".dat";
   char rext[5] =".jfr";

   if (argc != 3)
     return about();

   jfr_init(0);

   res = jfr_load(&head, argv[1]);
   if (res != 0)
   { strcpy(fname, argv[1]);
     strcat(fname, rext);
     res = jfr_load(&head, fname);
   }
   if (res != 0)
   {  printf("Error loading the program: %s. Error code %d\n",
	     argv[1], res);
      return 1;
   }

   jfr_activate(head, NULL, NULL, NULL); /* the program has to be activated  */
                                         /* because we don't execute it.     */

   jfg_init(JFG_PM_NORMAL, 32, 4);
   jfg_sprg(&spdesc, head);

   var_no = find_var(argv[2]);
   if (var_no == -1)
   { printf("No variable with the name: %s in program.\n", argv[2]);
     jfr_close(head);
     jfg_free();
     jfr_free();
     return 1;
   }
   strcpy(fname, argv[2]);
   if (strlen(fname) > 8)
     fname[8] = '\0';
   strcat(fname, ext);
   if ((df = fopen(fname, "w")) == NULL)
   { printf("Cannot open the file %s\n", fname);
     jfr_close(head);
     jfg_free();
     jfr_free();
     return 1;
   }
   vplot(var_no);

   fclose(df);
   jfr_close(head);
   jfg_free();
   jfr_free();
   return 0;
}
