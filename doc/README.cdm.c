/* $Id$ */
/*
 * Canned Document Management Improved
 *
 * TODO
 *  1) locking (asynchronous I/O)
 *  2) data structure design
 *  3) interface design
 *  4) b+ tree implementation
 *  5) customizing xml parser
 *  6) spooling document
 *  7) filtering handling interface
 *  8) cache implementation
 *  9) linear hash implementation
 * 
 * features
 * 1. persistant dom preservation database
 * 2. similar to native XML database
 *
 * special condition
 * 1. never insert key which is smaller than maximum key
 *    of registered data -> linear hash
 * 2. total number of data to register can be estimated
 *    before creating database -> linear hash + b+ tree
 * 3. cdm should have some special information (such as
 *    header of parser object)
 */

/*
 * database structure
 *
 * * page size is should be 4096 byte: see getpagesize(2)
 * access id
 *  : total (2^32 page, 4096 byte per page) is
 *    accessable
 *
 * 0000 0000 0000 0000
 * ^^^^ db id
 *      ^^^^ ^^^^ ^^^^ page id
 *
 * db id range is 0 - 2^13
 * each file involve 2^19 pages
 * each page is 2^12 bytes (4kb)
 *
 * db accessing unit is page
 * caching unit is page
 *
 * FILE STRUCTURE
 * database
 *  : several files that has some rule of file name (such as
 *    cdm00001.db)
 * 
 * file
 *  : each file has (0x8000 0000 / 4096) = 2^19 pages
 *
 *
 * MEMORY STRUCTURE
 * built in shared memory in case of multi-process
 * built in normal memory in case of multi-thread
 *
 * +========================================+
 * |     |      |      |      |      |      |  <Linear Hash:
 * +========================================+   should be in memory>
 *    |
 *  Bucket
 * +======+
 * |      |  <Bucket: implemented with B+ tree
 * +======+           buckets can be in disk>
 * |      |
 * +======+
 * | Slot | 
 * +======+
 *  <Slot: accessid>
 *
 */

/*
 * Linear hash API
 */
lh_t *lh_create(int size, int nmemb, enum keytype_t type)
{
}

void lh_destroy(lh_t *lh)
{
}

int lh_search(lh_t *lh, key_t key, void *buf)
{
}

int lh_insert(lh_t *lh, key_t key, void *data)
{
}

int lh_update(lh_t *lh, key_t key, void *data)
{
}

int lh_delete(lh_t *lh, key_t key)
{
}

/*
 * B+ Trees API
 *
 * feature
 * 1) b-link
 * 2) multi-access considered (locking)
 */
bp_t *bp_create(int size)
{
}

void bp_destroy(bp_t *bp)
{
}

int bp_search(bp_t *bp, int key, void *buf)
{
}

int bp_insert(bp_t *bp, int key, void *data)
{
}

int bp_update(bp_t *bp, int key, void *data)
{
}

int bp_delete(bp_t *bp, int key)
{
}

/*
 * General Database for CDM
 *
 * features
 * 1. access file by unit of page size(4096B? larger size of page?)
 * 2. update, delete function supported
 * 3. defragment function supported
 * 4. collection of relevance data function supported
 * 5. cache ability
 *
 * HIERARCHY
 *                CDM Improved
 *                   |
 *      +-----------------------------
 *      |            |           |
 *  database
 *      |
 *      +------ Document(stored in format of persistant dom) -+-- Field
 *      |                                                     |
 *      |                                                     +-- Field
 *      +------                                               |
 *      |
 *
 * 
 * SHARE DATA
 * dbi table: implemented by array
 * +========================================
 * | dbi | opened database infomations (such as 
 * |     | linear hash header and its bucket.
 * |     | cache is not include here, cache is
 * |     | global shared data over databases)
 * +========================================
 * |     |
 *
 */

/*
 * CDM API
 */
int cdm_db_create(const char *dbname)
{
}

int cdm_db_select(const char *dbname)
{
}

pdom_t *cdm_retrieve(int dbi, uint32_t docid)
{
}

int cdm_register_xml(const char *xmltext, int len)
{
}

int cdm_render_xml(pdom_t *p, char *buf, int *buflen)
{
}

int cdm_release(pdom_t *p)
{
}

/*
 * Cache API
 *
 * Reducing Miss Rate
 * 1. Larger Block Size
 * 2. Higher Set Associativity
 * 3. Victim Cache
 * 4. Pseudo-Associative Cache
 *
 * CACHE STRUCTURE
 * table: implemented by dynamic hash
 * +====================================================+
 * | accessid | memory address which the page is loaded |
 * +====================================================+
 * |          |                                         |
 * +====================================================+
 * |          |                                         |
 *
 */
cache_t *ch_init(int size, int nmemb)
{
}

int ch_is_hit(cache_t *ch, key_t key, page_t *page)
{
}

int ch_push(key_t key, page_t *page)
{
}

int ch_get_status(cache_t *ch, struct cachestatus *st)
{
}

int ch_set_policy(cache_t *ch, int type)
{
}

int ch_set_priority_handler(cache_t *ch, priority_handler func, void *userdata)
{
}
