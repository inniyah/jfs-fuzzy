/**************************************************************************/
/*                                                                        */
/* This example uses the tree-function 'jfg_if_tree' to convert the       */
/* conditional-expresions of if-statements to LISP-like code. A statement */
/* like:                                                                  */
/*       if x + 2 > 3 and x low then y high                               */
/*                                                                        */
/* is converted to:                                                       */
/*                                                                        */
/*      (and (> (+ x 2) 3) (is x low))                                    */
/*                                                                        */
/* The converted expresions are written to standard-output.               */
/*                                                                        */
/* usage: jfg_ex04 progname                                               */
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
#include "jfs_text.h"

#ifdef __BCPLUSPLUS__
  /* The folowing lines are only needed if the program is compiled with */
  /* Borland C++Builder.                                                */
  #pragma argsused
  USEUNIT("..\..\COMMON\Jfr_lib.cpp");
  USEUNIT("..\..\COMMON\Jfg_lib.cpp");
  USEUNIT("..\..\COMMON\jfs_text.cpp");
//---------------------------------------------------------------------------
#endif

void *head;                   /* jfs-program   */
struct jfg_sprog_desc spdesc; /* describes of the jfs-program */

struct jfg_tree_desc tree[100];

struct jfg_var_desc      vdesc;    /* temporary variable to the decode  */
struct jfg_operator_desc odesc;    /* function.                         */
struct jfg_hedge_desc    hdesc;
struct jfg_relation_desc rdesc;
struct jfg_fzvar_desc    fzvdesc;
struct jfg_adjectiv_desc adesc;
struct jfg_function_desc ufdesc;
struct jfg_array_desc    ardesc;


void decode(int tree_adr)
{
  switch (tree[tree_adr].type)
  {  case JFG_TT_OP:
       jfg_operator(&odesc, head, tree[tree_adr].op);
       printf("(%s ", odesc.name);
       decode(tree[tree_adr].sarg_1);
       decode(tree[tree_adr].sarg_2);
       printf(")" );
       break;
     case JFG_TT_HEDGE:
       jfg_hedge(&hdesc, head, tree[tree_adr].op);
       printf("(%s ", hdesc.name);
       decode(tree[tree_adr].sarg_1);
       printf(")" );
       break;
     case JFG_TT_UREL:
       jfg_relation(&rdesc, head, tree[tree_adr].op);
       printf("(%s ", rdesc.name);
       decode(tree[tree_adr].sarg_1);
       decode(tree[tree_adr].sarg_2);
       printf(")" );
       break;
     case JFG_TT_SFUNC:
       printf("(%s ", jfs_t_sfus[tree[tree_adr].op]);
       decode(tree[tree_adr].sarg_1);
       printf(")" );
       break;
     case JFG_TT_DFUNC:
       printf("(%s ", jfs_t_dfus[tree[tree_adr].op]);
       decode(tree[tree_adr].sarg_1);
       decode(tree[tree_adr].sarg_2);
       printf(")" );
       break;
     case JFG_TT_CONST:
       printf("%6.2f ", tree[tree_adr].farg);
       break;
     case JFG_TT_VAR:
       jfg_var(&vdesc, head, tree[tree_adr].sarg_1);
       printf("%s ", vdesc.name);
       break;
     case JFG_TT_FZVAR:
       jfg_fzvar(&fzvdesc, head, tree[tree_adr].sarg_1);
       jfg_var(&vdesc, head, fzvdesc.var_no);
       jfg_adjectiv(&adesc, head,
 		                 vdesc.f_adjectiv_no + tree[tree_adr].sarg_1
               	    - vdesc.f_fzvar_no);
       printf("(is %s %s) ", vdesc.name, adesc.name);
       break;
     case JFG_TT_TRUE:
       printf("true ");
       break;
     case JFG_TT_FALSE:
       printf("false ");
       break;
     case JFG_TT_BETWEEN:
       jfg_var(&vdesc, head, tree[tree_adr].sarg_1);
       printf("(between %s ", vdesc.name);
       jfg_adjectiv(&adesc, head, vdesc.f_adjectiv_no
				  + tree[tree_adr].sarg_2);
       printf("%s ", adesc.name);
       jfg_adjectiv(&adesc, head, vdesc.f_adjectiv_no
				  + tree[tree_adr].op);
       printf("%s) ", adesc.name);
       break;
     case JFG_TT_VFUNC:
       printf("(%s ", jfs_t_vfus[tree[tree_adr].op]);
       jfg_var(&vdesc, head, tree[tree_adr].sarg_1);
       printf(" %s) ", vdesc.name);
       break;
     case JFG_TT_UFUNC:
       jfg_function(&ufdesc, head, tree[tree_adr].op);
       printf("(%s ", ufdesc.name);
       decode(tree[tree_adr].sarg_1);
       printf(")");
       break;
     case JFG_TT_ARGLIST:
       decode(tree[tree_adr].sarg_1);
       printf(" ");
       decode(tree[tree_adr].sarg_2);
       break;
     case JFG_TT_IIF:
       printf("(iif ");
       decode(tree[tree_adr].sarg_1);
       printf(" ");
       decode(tree[tree_adr].sarg_2);
       printf(") ");
       break;
     case JFG_TT_ARVAL:
       jfg_array(&ardesc, head, tree[tree_adr].op);
       printf("(array %s ", ardesc.name);
       decode(tree[tree_adr].sarg_1);
       printf(") ");
       break;
  }
}

main(int argc, char *argv[])
{
   int res, finished;
   unsigned char *pc;
   struct jfg_statement_desc sdesc;
   char fname[80];
   char ext[5] = ".jfr";
   unsigned short eroot, croot, iroot;

   if (argc != 2)
   { printf("usage: jfg_ex04 progname\n");
     return 1;
   }
   res = jfr_init(0);
   if (res != 0)
   { printf("Error when initialising jfr_lib. Error code %d\n", res);
     exit(1);
   }
   res = jfr_load(&head, argv[1]);
   if (res != 0)
   { strcpy(fname, argv[1]);
     strcat(fname, ext);
     res = jfr_load(&head, fname);
     if (res != 0)
     {	printf("Error loading jfr-program from %s. Errcode: %d\n",
	             argv[1], res);
       jfr_free();       
      	return 1;
     }
   }

   jfr_activate(head, NULL, NULL, NULL);
   jfg_init(JFG_PM_NORMAL, 32, 4);
   jfg_sprg(&spdesc, head);
   pc = spdesc.pc_start;
   finished = 0;
   while (finished == 0)
   { jfg_statement(&sdesc, head, pc);
     switch(sdesc.type)
     { case JFG_ST_IF:
       	 res = jfg_if_tree(tree, 100, &croot, &eroot, &iroot, head, pc);
       	 if (res == 0)
       	 { printf("\n");
       	   decode(croot);
       	   printf("\n");
       	 }
       	 else
        	   printf("Error getting jfg_tree. Errcode: %d\n", res);
       	 break;
       case JFG_ST_EOP:
       	 finished = 1;
       	 break;
     }
     pc = sdesc.n_pc;
   }
   jfr_close(head);
   jfg_free();
   jfr_free();
   return 0;
}
