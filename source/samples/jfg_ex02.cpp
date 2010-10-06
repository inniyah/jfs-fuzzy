/**************************************************************************/
/*                                                                        */
/* This example shows how to write an alternative to the standard jfs-    */
/* interpretter JFR.                                                      */
/*      usage: jfg_ex02 <fname>                                           */
/* The program asks for input from the keyboard and then executes the     */
/* jfs-program <fname> accepting the call-statement:                      */
/*      call dump <vname>                                                 */
/* This statements writes the value of the variable <vname> and the       */
/* values of fuzzy-variables bound to <vname> to the screen.              */
/* The program also includes a simplified version of jfr's log-writer.    */
/* After the jfr-program is executed the values of the output-variables   */
/* are written to the screen.                                             */
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

#ifdef __BCPLUSPLUS__
  /* The folowing lines are only needed if the program is compiled with */
  /* Borland C++Builder.                                                */
  USEUNIT("..\..\COMMON\Jfr_lib.cpp");
  USEUNIT("..\..\COMMON\Jfg_lib.cpp");
  USEUNIT("..\..\COMMON\jfs_text.cpp");
  #pragma argsused
#endif


void *jfr_head;               /* the jfs-program   */
struct jfg_sprog_desc spdesc; /* describes of the jfs-program */

float ipvalues[256];     /* Input values to the jfs-program   */
float opvalues[256];     /* Output values from the jfs-program */

#define MAX_CALL_ARGS 20
char *call_args[MAX_CALL_ARGS];  /* Used to decode a call-statement */

char call_t_dump[] = "dump";
char ext[] = ".jfr";

char jfs_txt[512];               /* text used to hold a jfs-statement */
                            				 /* in log-writing.                   */

struct jfg_tree_desc tree[100];  /* Used as temporary variable when   */
                            				 /* converting a program-statement to */
                            				 /* text.                             */


int var_find(char *name) /* returns the identification-number of the  */
{                        /* variable <name> (return -1 if not found). */
  int m;
  struct jfg_var_desc vdesc;

  for (m = 0; m < spdesc.var_c; m++)
  { jfg_var(&vdesc, jfr_head, m);
    if (strcmp(name, vdesc.name) == 0)
      return m;
  }
  return -1;
}

void dump_call(void)             /* Handles call-statements           */
{
  struct jfr_stat_desc ddesc;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;
  int a, vno, argc;
  float value, conf;

  jfr_statement_info(&ddesc);  /* get pc for the call statement */
  argc = jfg_a_statement(call_args, MAX_CALL_ARGS,
                     			 jfr_head, ddesc.pc);
  if (argc > 0)
  { if (strcmp(call_args[0], call_t_dump) == 0 && argc == 2)
    { vno = var_find(call_args[1]);
      if (vno == -1)
      { printf("Undefined variable %s\n", call_args[1]);
       	return ;
      }
      jfg_var(&vdesc, jfr_head, vno);
      value = jfr_vget(vno);
      conf = jfr_cget(vno);
      printf("\nDUMP %s. value: %6.2f, confidence: %6.2f\n",
       	      vdesc.name, value, conf);
      for (a = 0; a < vdesc.fzvar_c; a++)
      { jfg_adjectiv(&adesc, jfr_head, vdesc.f_adjectiv_no + a);
       	value = jfr_fzvget(vdesc.f_fzvar_no + a);
       	printf("%s is %s = %f\n", vdesc.name, adesc.name, value);
      }
    }
    /* else if (strcmp(call_args[0], ....) then .... Handle other */
    /* call-statements.                                           */
    else
      printf("Unknown call-commando: %s\n", call_args[0]);
  }

}

void log_write(int mode)      /* Simplified version of the log-writing */
{                             /* from JFR.                             */
  struct jfr_stat_desc ddesc;
  struct jfg_statement_desc sdesc;
  struct jfg_var_desc vdesc;
  struct jfg_fzvar_desc fzvdesc;
  struct jfg_adjectiv_desc adesc;
  float f;

  if (mode == 1)     /* Called from program-execution */
  {
    jfr_statement_info(&ddesc);
    jfg_statement(&sdesc, jfr_head, ddesc.pc);
    if (ddesc.changed == 1) /* Write only log-info if last statement  */
    {                       /* changed something.                     */
      printf("  rule %d\n", ddesc.rule_no);
      jfg_t_statement(jfs_txt, 512, 4,
              		      tree, 100,
                      jfr_head, ddesc.function_no, ddesc.pc);
      printf("%s\n", jfs_txt);

      if (sdesc.type == JFG_ST_IF && sdesc.sec_type == JFG_SST_FZVAR)
      { printf("    if %6.4f.", ddesc.cond_value);
       	f = jfr_fzvget(sdesc.sarg_1);
      	jfg_fzvar(&fzvdesc, jfr_head, sdesc.sarg_1);
      	jfg_var(&vdesc, jfr_head, fzvdesc.var_no);
      	jfg_adjectiv(&adesc, jfr_head, fzvdesc.adjectiv_no);
      	printf(" %s is %s = %6.4f.\n", vdesc.name, adesc.name, f);
      }
      /* else if (sdesc.type == ... Handle the other types of statements */
    }
  }
}

int main(int argc, char *argv[])
{
  char fname[80];
  char iptext[80];
  int res, cont, ip_no, v;
  struct jfg_var_desc vdesc;

  if (argc != 2)
  { printf("usage jfg_ex02 progname\n");
    return 1;
  }

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

  res = jfr_load(&jfr_head, argv[1]);
  if (res != 0)  /* If file not found try same filename with the */
  {              /* extension '.jfr"                             */
    strcpy(fname, argv[1]);
    strcat(fname, ext);
    res = jfr_load(&jfr_head, fname);
    if (res != 0)
    { printf("Cannot load jfs-program: %s. Error-code: %d\n", fname, res);
      jfg_free();
      jfr_free();
      exit(1);
    }
  }

  jfg_sprg(&spdesc, jfr_head);
  cont = 1;
  ip_no = 0;
  while (cont == 1)
  { printf("\nINPUT %d (Type ! to quit):\n", ip_no + 1);
   	for (v = 0; cont == 1 && v < spdesc.ivar_c; v++)
	   { jfg_var(&vdesc, jfr_head, spdesc.f_ivar_no + v);
	     printf("%s :", vdesc.text);
	     gets(iptext);
	     if (strcmp(iptext, "!") == 0)
	       cont = 0;
   	  else
   	    ipvalues[v] = atof(iptext);  /* No domain-check in demo */
   	}
   	printf("\n");

    if (cont == 1)
    { jfr_arun(opvalues, jfr_head, ipvalues, NULL,
               dump_call, log_write, NULL);

      printf("\nOUTPUT:\n");
      for (v = 0; v < spdesc.ovar_c; v++)
      { jfg_var(&vdesc, jfr_head, spdesc.f_ovar_no + v);
        printf("spdesc.text: %6.4f\n", opvalues[v]);
      }
    }
  }

  jfr_close(jfr_head);
  jfg_free();
  jfr_free();

  return 0;
}
