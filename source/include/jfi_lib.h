  /*********************************************************************/
  /*                                                                   */
  /* jfi_lib.h  Version  2.01 Copyright (c) 1999-2000 Jan E. Mortensen */
  /*                                                                   */
  /* JFS  improver-functions using evolutionary programing.            */
  /*                                                                   */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                */
  /*    Lollandsvej 35 3.tv.                                           */
  /*    DK-2000 Frederiksberg                                          */
  /*    Denmark                                                        */
  /*                                                                   */
  /*********************************************************************/

struct jfi_stat_rec
       {
		       float worst_score;
		       float median_score;
		       float old_score; /* score of ind before change.  */
		       float p1_score;  /* score parent-1.              */
		       float p2_score;  /* score parent-2.              */
		       int method;      /* cros-method:                 */
                          /* 0: crosover,                 */
                          /* 1: sumcros,                  */
                          /* 2: mutation.                 */
		     };

extern struct jfi_stat_rec jfi_stat;

int jfi_init(void *jfr_head,  /* the jfs-program.                      */
             int ind_c        /* Number of individs in population.     */
	           );

    /* Reserve memory to individs and atoms. Analyse jfs-program.      */


int jfi_run(float (*judge)(void),       /* judge-function.             */
   	        int maximize
	   );

     /* Create a population. Judge the population (by the judge-        */
     /* function) and create new individuals until jfi_stop() is called.*/



void jfi_stop(void);

     /* Stops the creation of individs. Copy best individ to jfr_head.  */



void jfi_free(void);  /* free the memory reserved by jfi_init(). */

