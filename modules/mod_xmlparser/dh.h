/* $Id$ */
#ifndef _DH_H_
#define _DH_H_ 1

#define MAX_KEY_LEN						128

typedef void dh_t;

#if defined (__cplusplus)
extern "C" {
#endif

/* create dynamic hash object and return
 * size: size of each value data to insert this hash */
dh_t *dh_create(int size);

/* destroy dynamic hash object; all memory allocated for
 * this object is freed.
 *
 * dh: dynamic hash object that is created by 'dh_create' */
void dh_destroy(dh_t *dh);

/* read dynamic hash data in sequential memory
 * and create dh_t object
 * and return address of dh_t object.
 * whole data is newly duplicate to another memory, so
 * caller can free input data after calling this function.
 *
 * data: data that is saved by dh_save or manualy saved
 * with dh_save2 in sequential memory
 * datalen: length of data; to valdiate formal structure */
dh_t *dh_load(void *data, int *datalen);

/* read dynamic hash data in sequential memory
 * and create dh_t object
 * and return address of dh_t object.
 * but, it doesn't duplicate data, so don't free
 * data that is inputed as parameter after calling
 * this function. 
 *
 * data: data that is saved by dh_save or manualy saved
 * with dh_save2 in sequential memory
 * datalen: length of data; to valdiate formal structure */
dh_t *dh_load2(void *data, int *datalen);

/* save whole data in sequential memory.
 * buffer to save data is must be allocated by caller.
 * return total size of data
 *
 * dh: pointer of dynamic hash object
 * buf: buffer to save data
 * buflen: length of buffer */
int dh_save(dh_t *dh, void *buf, int buflen);

/* return three vectors to save.
 * saving data is up to caller. caller need not to know
 * what these data mean. just save in sequential memory.
 * return value is summation of each data size.
 * you should save these three data in order.
 *
 * dh: pointer of dynamic hash object 
 * chunk*: pointer to *th data
 * len*: length of *th data */
int dh_save2(dh_t *dh, void **chunk1, int *len1, void **chunk2, int *len2,
	      void **chunk3, int *len3);

/* return total size that is needed when saving
 * dynamic hash object 
 *
 * dh: pointer of dynamic hash objec */
int dh_get_savingsize(dh_t *dh);

/* search value that matches to key.
 * return -1 if fault, return value as ptr 
 * 
 * CAUTION: data that get through 'ptr' parameter must not 
 *          be freed by caller. it is up to this library.
 *
 * dh: pointer of dynamic hash object
 * key: key string
 * ptr: to get the result */
int dh_search(dh_t *dh, const char *key, void **ptr);

/* insert data of which size is that you input when create.
 * dynamic hash object(calling 'dh_create'). 
 * return -1 if fault
 *
 * dh: pointer of dynamic hash object
 * key: key string
 * value: data to insert */
int dh_insert(dh_t *dh, const char *key, void *value);

/* not yet implemented */
int dh_delete(dh_t *dh, const char *key);

#if defined (__cplusplus)
}
#endif

#endif
