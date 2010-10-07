
  /**************************************************************************/
  /*                                                                        */
  /* jfgp_lib.h  Version  2.03    Copyright (c) 1999-2000 Jan E. Mortensen  */
  /*                                                                        */
  /* JFS rule discover-functions using Genetic programing.                  */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/

#ifndef jfgplibH
#define jfgplibH

struct jfgp_stat_rec
       {                  /* statistical information about current individ  */
                          /* and generation.                                */
		       int ind_size;    /* new size of the changed ind.     */
		       float old_score; /* score of ind before change.      */
         float sum_score; /* sum of score for activ individs. */
		       int free_c;      /* Number of free nodes in DAG.     */
		       int alive_c;     /* Number of activ individs.        */
		     };

extern struct jfgp_stat_rec jfgp_stat;

int jfgp_init(void *jfr_head,  /* the jfs-program.                      */
	             long atom_c,     /* number of atoms.                      */
	             int ind_c        /* Number of individs in population.     */
	            );

    /* Reserve memory to individs and atoms. Analyse 'call jfgp'-      */
    /* statement.                                                      */


int jfgp_run(float (*judge)(void),      /* judge-function.             */
             int (*compare)(float score_1, int size_1,
			                         float score_2, int size_2),
             int tournament_size
	   );

     /* Create a population. Create new individuals until jfgp_stop()  */
     /* is called (from the judge-function).                           */
     /* Every time an individual is created it is copied to jfr_head   */
     /* And the judge() is called. The value returned from the         */
     /* judge-functions is the indviduals score.                       */
     /* Every time 2 individuals are compared, it is done by the       */
     /* compare()-function. This function should return 1 if ind_1 is  */
     /* better than ind_2, and -1 if ind_2 is better than ind_1.       */
     /* Tournament_size is the group-size for tournament selection. If */
     /* tournament_size==0 a default group size of 5 is choosen.       */ 



void jfgp_stop(void);

     /* Stops the creation of individs. Copy best individ to jfr_head.  */



void jfgp_free(void);  /* free the memory reserved by jfgp_init(). */

#endif

