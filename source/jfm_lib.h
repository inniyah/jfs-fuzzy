//---------------------------------------------------------------------------
#ifndef jfm_libH
#define jfm_libH
//---------------------------------------------------------------------------

  /***********************************************************/
  /* jfm_lib.h                 Version  2.00                 */
  /*                                                         */
  /* Memory-management system.                               */
  /* System of circular list with variable-sized data bound  */
  /* to each node.                                           */
  /***********************************************************/

int jfm_create(signed short id_size, unsigned long data_size);

    /* creates and initialsis the data-structure.                     */
    /* <id_size> is the total number of nodes in the lists.           */
    /* <data_size> is the total number of bytes reserved to data.     */
    /* Return:  0: ok,                                                */
    /*         -6: cannot allocate memory.                            */



void jfm_init(void);

     /* initialises the data-structure created by jfm_create();       */



void jfm_free(void);

     /* frees the memory allocated by jfm_init.                       */


void jfm_garbage_collect(void);

    /* defragment the jfm-data-area (is called automatic executed when */
    /* a jfm-functions fails, due to lack of memory).                  */


signed short jfm_pre_insert(signed short id,
                            void *data, unsigned short data_c);
signed short jfm_post_insert(signed short id, void *data,
                             unsigned short data_c);

    /* inserts <data> in the memory. Inserts it in the list           */
    /* containing <id>. If pre_insert, it is inserted in before <id>. */
    /* If post_insert it is inserted after <id>. If <id> < 0 then     */
    /* data is inserted in a new list.                                */
    /* Returns <id> of the inserted data, or                          */
    /*     -6: not enough data-memory,                                */
    /*    -14: no free id.                                            */



signed short jfm_delete(signed short id);

    /* deletes the data referenced by <id> and removes (frees) <id>   */
    /* from its list.                                                 */
    /* Return:  >= 0 : id for next node in the list,                  */
    /*          -1   : <id> was the only node in list.                */



signed short jfm_move_forward(signed short id1);
signed short jfm_move_backward(signed short id1);

    /* moves the node <id1> forward/backward in the list.                      */
    /* Returns the node <id1> changes place with.                     */



unsigned short jfm_data_size(signed short id);

    /* returns the size of the data-element of node <id>.             */



void *jfm_data_adr(signed short id);

     /* returns a pointer to data for the node <id>. N.B. the address */
     /* is only valid until next insert/update.                       */



signed short jfm_update(signed short id,
                     			void *data, unsigned short data_c);

    /* Changes data for <id> to <data>.                               */
    /* Return:  0: ok,                                                */
    /*         -6: cannot alocat memory to <data>.                    */



signed short jfm_get_no(signed short id);
void jfm_set_no(signed short id, signed short no);

    /* get/set the value of no for the node <id>.                     */




signed short jfm_next(signed short id);

    /* returns id for node after <id> in list.                        */


signed short jfm_prev(signed short id);

    /* returns id for node before <id> in list.                       */




void jfm_list_split(signed short id_begin, signed short id_end);

    /* Splits the list containing <id_begin> and <id_end> in two      */
    /* lists. the first contains the nodes from <id_begin> to         */
    /* <id_end>. The other contains the rest of the list.             */


signed short jfm_ar_find(signed short fid, signed short ano);

   /* returns id for the element number <ano> in the list where       */
   /* element number 0 is <fid>. Returns -1 if the list is shorter    */
   /* than <ano>.                                                     */


signed short jfm_no_find(signed short fid, signed short no);

   /* returns id for the element with no==<no> in the list starting   */
   /* with <fid>.                                                     */

void jfm_free_list(signed short fid);

   /*  Deletes all elements in the list starting in (or containing)   */
   /*  the node <fid>.                                                */
   
//-----------------------------------------------------------------------
#endif
