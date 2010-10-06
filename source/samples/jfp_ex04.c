/**************************************************************************/
/*                                                                        */
/* This example uses jfp_lib to insert a FAM into a jfs-program. The      */
/* statement 'extern fam <v1> <v2> <ov>;' is replaced with a list of if-  */
/* statements:                                                            */
/*                                                                        */
/* if <v1> <v1a1> and <v2> <v2a1> then <ov> <ova1>;                       */
/* if <v1> <v1a1> and <v2> <v2a2> then <ov> <ova1>;                       */
/*     .                                                                  */
/*     .                                                                  */
/*                                                                        */
/* Usage:  jfp_ex04 <jfrf>                                                */
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
#include "jfg_lib.h"
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
char t_fam[] = "fam";

char *cargv[10];
struct jfg_tree_desc tree[4];
unsigned char *pc;

struct jfg_sprog_desc spdesc;

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


int insert_fam()
{
  int res, ins, argc, v1_no, v2_no, vo_no, i1, i2, io;
  float t;
  struct jfg_var_desc v1_desc, v2_desc, vo_desc;
  struct jfg_statement_desc sdesc;
  char *dummy[2];

  ins = 0;
  argc = jfg_a_statement(cargv, 10, head, pc);
  if (argc == 4)
  { if (strcmp(cargv[0], t_fam) == 0)
    { v1_no = find_var(cargv[1]);
      v2_no = find_var(cargv[2]);
      vo_no = find_var(cargv[3]);
      if (v1_no < 0 || v2_no < 0 || vo_no < 0)
      { printf("Undefined variablename in 'call fam'-statement.\n");
       	return 0;
      }
      jfp_d_statement(head, pc); /* Note: after the delete, the values of  */
				                             /* cargv are meaningsless.                */
      ins = 1;
      jfg_var(&v1_desc, head, v1_no);
      jfg_var(&v2_desc, head, v2_no);
      jfg_var(&vo_desc, head, vo_no);
      if (v1_desc.fzvar_c == 0 || v2_desc.fzvar_c == 0
       	  || vo_desc.fzvar_c == 0)
      { printf("No adjectives bound to variable.\n");
       	return ins;
      }

      /* insert FAM */
      for (i1 = 0; i1 < v1_desc.fzvar_c; i1++)
      { for (i2 = 0; i2 < v2_desc.fzvar_c; i2++)
       	{ /* then-adjectiv-no is calculated as "scaled avg" of the 2  */
          /* if-adjectives:                                           */
          t = (  ((float) i1 + 1.0) / ((float) v1_desc.fzvar_c)
	              + ((float) i2 + 1.0) / ((float) v2_desc.fzvar_c)) / 2.0;
       	  io = t * (vo_desc.fzvar_c - 1.0);
       	  if (io >= vo_desc.fzvar_c)
     	    io = vo_desc.fzvar_c - 1;

          /* build tree (expressions of the type '<v1> <a1> and <v2> <a2>' */
          tree[0].type = JFG_TT_OP;
          tree[0].op = JFS_ONO_AND;
          tree[0].sarg_1 = 1;
          tree[0].sarg_2 = 2;

          tree[1].type   = JFG_TT_FZVAR;
          tree[1].sarg_1 = v1_desc.f_fzvar_no + i1;

          tree[2].type   = JFG_TT_FZVAR;
          tree[2].sarg_1 = v2_desc.f_fzvar_no + i2;

          sdesc.type     = JFG_ST_IF;
          sdesc.sec_type = JFG_SST_FZVAR;
          sdesc.flags    = 0;
          sdesc.sarg_1   = vo_desc.f_fzvar_no + io;

          res = jfp_i_tree(head, &pc, &sdesc, tree,
               0,                       /* root tree.      */
               0, 0, dummy, 0);         /* not used.       */
                                        /* note that pc is changed. */
          if (res != 0)
            printf("error in jfp_ins_tree. Errcode %d\n", res);

       	}
      }
    }
  }
  return ins;
}


int main(int argc, char *argv[])
{
  struct jfg_statement_desc sdesc;
  int m, res, ins;
  char fname[80];

  if (argc != 2)
  { printf("Usage: jfp_ex04 jfrf\n");
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
  res = jfr_aload(&head, fname, 1000);         /* Note jfr_aload() is   */
					                                          /* used to reserve extra */
                                   					       /* memory to statements. */
  if (res != 0)
  { strcat(fname, ext);
    res = jfr_aload(&head, fname, 1000);
  }
  if (res == 0)
  {
    jfg_sprg(&spdesc, head);

    pc = spdesc.pc_start;
    jfg_statement(&sdesc, head, pc);
    while (sdesc.type != JFG_ST_EOP) /* step thorugh the main-program, find  */
                            				     /* 'extern'-statements (the demo don't  */
                                     /* handle 'extern'-statements in        */
                                     /* functions/procedures.                */
    { ins = 0;
      if (sdesc.type == JFG_ST_IF && sdesc.sec_type == JFG_SST_EXTERN)
       	ins = insert_fam();
      if (ins == 0)
       	pc = sdesc.n_pc;
      jfg_statement(&sdesc, head, pc);
    }

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


