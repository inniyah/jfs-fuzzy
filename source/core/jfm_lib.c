  /***********************************************************/
  /* jfm_lib.cpp               Version  2.00                 */
  /*                                                         */
  /* Memory-management system.                               */
  /*                                                         */
  /***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jfm_lib.h"

struct jfm_node_desc  { signed short next;
                        signed short prev;
                        signed short no;
                        unsigned long data;
                      };


struct jfm_dh_desc    { signed short id;       /* ref til nodes              */
                                               /* -1: free memory.           */
                        unsigned short data_c; /* data_size excl head        */
                      };


struct jfm_head_desc  { struct jfm_node_desc *nodes;
                        signed short         node_c;
                        signed short         free_list;
                        signed short         free_c;
                        unsigned char        *data;
                        unsigned long        data_c;
                        unsigned long        ff_data;
                      };


static struct jfm_head_desc jfm_head;

static signed short jfm_rm_node(signed short id);
static signed short jfm_data_add(signed short id, void *data,
                            				 unsigned short data_c);
static signed short jfm_insert(signed short id,
                      		       void *data, unsigned short data_c, int imode);
static void jfm_data_free(unsigned long data);


void jfm_init(void)
{
  signed short m;

  jfm_head.ff_data = 0;
  for (m = 0; m < jfm_head.node_c; m++)
  { if (m == 0)
      jfm_head.nodes[m].prev = jfm_head.node_c - 1;
    else
      jfm_head.nodes[m].prev = m - 1;
    if (m == jfm_head.node_c - 1)
      jfm_head.nodes[m].next = 0;
    else
      jfm_head.nodes[m].next = m + 1;
    jfm_head.nodes[m].data = 0;
  }
  jfm_head.free_list = 0;
  jfm_head.free_c = jfm_head.node_c;
}

int jfm_create(signed short id_size, unsigned long data_size)
{
  unsigned char *a;

  jfm_head.data = NULL;
  jfm_head.nodes = NULL;
  if ((a = (unsigned char *) malloc(id_size * sizeof(struct jfm_node_desc))) == NULL)
    return -6;
  jfm_head.nodes = (struct jfm_node_desc *) a;
  jfm_head.node_c = id_size;
  if ((a = (unsigned char *) malloc(data_size)) == NULL)
    return 6;
  jfm_head.data = a;
  jfm_head.data_c = data_size;
  jfm_init();
  return 0;
}


void jfm_free(void)
{

  if (jfm_head.nodes != NULL)
    free(jfm_head.nodes);
  jfm_head.nodes = NULL;
  if (jfm_head.data != NULL)
    free(jfm_head.data);
  jfm_head.data = NULL;

}


void jfm_garbage_collect(void)
{
  unsigned long ff, cur, dhs, m, a;
  struct jfm_dh_desc *datahead;

  cur = ff = 0;
  dhs = sizeof(struct jfm_dh_desc);

  while (cur < jfm_head.ff_data)
  { datahead = (struct jfm_dh_desc *) &(jfm_head.data[cur]);
    if (datahead->id == -1)
      cur += dhs + datahead->data_c;
    else
    { jfm_head.nodes[datahead->id].data = ff + dhs;
      a = datahead->data_c + dhs;
      for (m = 0; m < a; m++)
      { jfm_head.data[ff] = jfm_head.data[cur];
       	ff++; cur++;
      }
    }
  }
  jfm_head.ff_data = ff;
}

static signed short jfm_rm_node(signed short id)
{
   /* removes node id from list and return next node (-1 if id   */
   /* is the only node).                                         */

  signed short nextid, previd, nn, pn;
  struct jfm_node_desc *node;

  node = &(jfm_head.nodes[id]);
  if (node->next == id)
    nextid = -1;
  else
  { previd = node->prev;
    nextid = node->next;
    jfm_head.nodes[previd].next = nextid;
    jfm_head.nodes[nextid].prev = previd;
  }
  if (jfm_head.free_list == -1)
  { node->prev = node->next = id;
  }
  else
  { nn = jfm_head.free_list;
    pn  = jfm_head.nodes[nn].prev;
    node->next = nn;
    node->prev = pn;
    jfm_head.nodes[nn].prev = id;
    jfm_head.nodes[pn].next = id;
  }
  jfm_head.free_list = id;
  jfm_head.free_c++;
  return nextid;
}

static signed short jfm_data_add(signed short id, void *data,
                            				 unsigned short data_c)
{
  /* insert data at ff_data. nodes[id].data := insert_point    */
  unsigned long ds;
  struct jfm_dh_desc datahead;
  unsigned char *adr;

  ds = data_c + sizeof(struct jfm_dh_desc);
  if (ds + jfm_head.ff_data >= jfm_head.data_c)
  { jfm_garbage_collect();
    if (ds + jfm_head.ff_data >= jfm_head.data_c)
      return -6;
  }
  datahead.id = id;
  datahead.data_c = data_c;
  adr = &(jfm_head.data[jfm_head.ff_data]);
  memcpy(adr, &(datahead), sizeof(struct jfm_dh_desc));
  jfm_head.ff_data += sizeof(struct jfm_dh_desc);
  adr += sizeof(struct jfm_dh_desc);
  jfm_head.nodes[id].data = jfm_head.ff_data;
  memcpy(adr, data, data_c);
  jfm_head.ff_data += data_c;
  return 0;
}

static signed short jfm_insert(signed short id,
                      		       void *data, unsigned short data_c, int imode)
{
  signed short newid, previd, nextid;
  struct jfm_node_desc *node;

  if (jfm_head.free_list == -1)
    return -14;

  newid = jfm_head.free_list;
  if (jfm_data_add(newid, data, data_c) != 0)
    return -6;

  jfm_head.free_list = jfm_rm_node(newid);
  jfm_head.free_c--;

  node = &(jfm_head.nodes[newid]);
  if (id == -1)
  { node->next = newid;
    node->prev = newid;
  }
  else
  { if (imode == 0)  /* pre */
    { nextid = id;
      previd  = jfm_head.nodes[nextid].prev;
    }
    else
    { previd = id;
      nextid = jfm_head.nodes[previd].next;
    }
    node->prev = previd;
    node->next = nextid;
    jfm_head.nodes[previd].next = newid;
    jfm_head.nodes[nextid].prev = newid;
  }
  return newid;
}


signed short jfm_pre_insert(signed short id, void *data,
		                    	     unsigned short data_c)
{
  return jfm_insert(id, data, data_c, 0);
}

signed short jfm_post_insert(signed short id, void *data,
                             unsigned short data_c)
{
  return jfm_insert(id, data, data_c, 1);
}


static void jfm_data_free(unsigned long data)
{
  unsigned char *adr;
  struct jfm_dh_desc *datahead;

  adr = &(jfm_head.data[data]);
  adr -= sizeof(struct jfm_dh_desc);
  datahead = (struct jfm_dh_desc *) adr;
  datahead->id = -1;
}


signed short jfm_delete(signed short id)
{
  jfm_data_free(jfm_head.nodes[id].data);
  return jfm_rm_node(id);
}


signed short jfm_move_forward(signed short id)
{
  signed short prev, next, nextnext;

  next = jfm_head.nodes[id].next;
  if (id != next)
  { /* first rm id from list */
    prev = jfm_head.nodes[id].prev;
    if (prev != next)
    { jfm_head.nodes[prev].next = next;
      jfm_head.nodes[next].prev = prev;
      /* then place id in front of next */
      nextnext = jfm_head.nodes[next].next;
      jfm_head.nodes[id].prev = next;
      jfm_head.nodes[id].next = nextnext;
      jfm_head.nodes[next].next = id;
      jfm_head.nodes[nextnext].prev = id;
    }
  }
  return next;
}


signed short jfm_move_backward(signed short id)
{
  signed short prev, next, prevprev;

  prev = jfm_head.nodes[id].prev;
  if (id != prev)
  { next = jfm_head.nodes[id].next;
    if (prev != next)
    { jfm_head.nodes[prev].next = next;
      jfm_head.nodes[next].prev = prev;
      prevprev = jfm_head.nodes[prev].prev;
      jfm_head.nodes[id].next = prev;
      jfm_head.nodes[id].prev = prevprev;
      jfm_head.nodes[prev].prev = id;
      jfm_head.nodes[prevprev].next = id;
    }
  }
  return prev;
}


unsigned short jfm_data_size(signed short id)
{
  unsigned short ds;
  unsigned char *adr;
  struct jfm_dh_desc *datahead;

  if (id == -1)
    ds = 0;
  else
  { adr = &(jfm_head.data[jfm_head.nodes[id].data]);
    adr -= sizeof(struct jfm_dh_desc);
    datahead = (struct jfm_dh_desc *) adr;
    ds = datahead->data_c;
  }
  return ds;
}

void *jfm_data_adr(signed short id)
{
  void *adr;

  adr = (void *) &(jfm_head.data[jfm_head.nodes[id].data]);
  return adr;
}

signed short jfm_update(signed short id,
                       	void *data, unsigned short data_c)
{
  unsigned char *adr;
  signed short res;
  struct jfm_dh_desc *datahead;

  if (jfm_data_size(id) == data_c)
  { adr = &(jfm_head.data[jfm_head.nodes[id].data]);
    memcpy(adr, data, data_c);
    res = 0;
  }
  else
  { jfm_data_free(jfm_head.nodes[id].data);
    res = jfm_data_add(id, data, data_c);
    if (res != 0)
    { /* retter fejlagtig data_free */
      adr = &(jfm_head.data[jfm_head.nodes[id].data]);
      adr -= sizeof(struct jfm_dh_desc);
      datahead = (struct jfm_dh_desc *) adr;
      datahead->id = id;
    }
  }
  return res;
}


signed short jfm_get_no(signed short id)
{
  return jfm_head.nodes[id].no;
}


void jfm_set_no(signed short id, signed short no)
{
  jfm_head.nodes[id].no = no;
}


signed short jfm_next(signed short id)
{
  signed short nextid;

  if (id == -1)
    nextid = -1;
  else
    nextid = jfm_head.nodes[id].next;
  return nextid;
}


signed short jfm_prev(signed short id)
{
  signed short previd;

  if (id == -1)
    previd = -1;
  else
    previd = jfm_head.nodes[id].prev;
  return previd;
}

signed short jfm_ar_find(signed short fid, signed short ano)
{
  signed short id, res, c;

  c = 0; res = -1;
  id = fid;
  while (id != -1 && res == -1)
  { if (c == ano)
      res = id;
    else
    { c++;
      id = jfm_next(id);
      if (id == fid)
        id = -1;
    }
  }
  return res;
}

signed short jfm_no_find(signed short fid, signed short no)
{
  signed short id, res;

  res = -1;
  id = fid;
  while (id != -1 && res == -1)
  { if (jfm_head.nodes[id].no == no)
      res = id;
    else
    { id = jfm_next(id);
      if (id == fid)
        id = -1;
    }
  }
  return res;
}

void jfm_list_split(signed short id_begin, signed short id_end)
{
  signed short next, prev;

  if (jfm_head.nodes[id_end].next != id_begin)
  { prev = jfm_head.nodes[id_begin].prev;
    next = jfm_head.nodes[id_end].next;
    jfm_head.nodes[id_begin].prev = id_end;
    jfm_head.nodes[id_end].next = id_begin;
    jfm_head.nodes[prev].next = next;
    jfm_head.nodes[next].prev = prev;
  }
}

void jfm_free_list(signed short startid)
{
  int id;

  id = startid;
  while (id != -1)
    id = jfm_delete(id);
}

