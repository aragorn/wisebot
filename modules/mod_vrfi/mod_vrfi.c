/* $Id$ */
#include "softbot.h"
#include "mod_vrfi.h"

#define OPEN_FLAG   (O_RDWR)
#define CREATE_FLAG (O_RDWR|O_CREAT)
#define MODE        (S_IREAD|S_IWRITE)

#ifdef STANDALONE_DEBUG
#	define OUTSTREAM stdout
#else
#	define OUTSTREAM stderr
#endif

#define MAX_HEADER_AT_READ 2048
#define MAX_LOCATION_AT_READ 2048

#if 0
static char *get_dbinfo(dbinfo_t *dbinfo)
{
	// XXX: not thread safe, intentionally. --jiwon
	static char result[LONG_STRING_SIZE];
	int size_left = LONG_STRING_SIZE;
	char buf[LONG_STRING_SIZE];

	sz_snprintf(result, LONG_STRING_SIZE, 
			"  fixedsize:%d, default_variable_size:%d \n",
			dbinfo->fixedsize, dbinfo->default_variable_size);
	size_left = LONG_STRING_SIZE - strlen(result);

	sz_snprintf(buf, LONG_STRING_SIZE, 
			"  last_unused_key:%u\n  path:%s\n  dbstamp:%s \n",
			dbinfo->last_unused_key, dbinfo->path, dbinfo->dbstamp);

	sz_strncat(result, buf, size_left);
	size_left -= strlen(buf);

	sz_snprintf(buf, LONG_STRING_SIZE, 
				"  STRING_INFO_SIZE:%d, BINARY_INFO_SIZE:%d\n",
				STRING_INFO_SIZE, BINARY_INFO_SIZE);
	sz_strncat(result, buf, size_left);
	size_left -= strlen(buf);

	sz_snprintf(buf, LONG_STRING_SIZE,
				"  MAX_VRF_SHM_NUM:%d, NUM_OF_EACH_BUNDLED_KEYS:%d",
				MAX_VRF_SHM_NUM, NUM_OF_EACH_BUNDLED_KEYS);
	sz_strncat(result, buf, size_left);
	size_left -= strlen(buf);

	sz_strncat(result, "\n", size_left);
	size_left -= 1;

	return result;
}

static char *get_vrfi_info(VariableRecordFile *this)
{
	static char result[LONG_LONG_STRING_SIZE];
	int size_left=LONG_LONG_STRING_SIZE;
	char buf[STRING_SIZE];
	int i=0;

	sz_snprintf(result, size_left, 
		"  MASTER_DATA_SIZE:%d(fixed+laster_header_pos), VRFI_EACH_SHM_SIZE:%.2fM\n",
		MASTER_DATA_SIZE(this), (double)VRFI_EACH_SHM_SIZE(this)/1024/1024);
	size_left -= strlen(result);

	sz_strncat(result, "\n  attach_flag_table:\n\t", size_left);
	size_left = LONG_LONG_STRING_SIZE - strlen(result);
	for (i=0; i<MAX_VRF_SHM_NUM; i++) {
		sz_snprintf(buf, STRING_SIZE, "[%d]%d ", i, this->attach_flag_table[i]);
		
		sz_strncat(result, buf, size_left);
		size_left -= strlen(buf);

		if (i%10 == 9) {
			sz_strncat(result, "\n\t", size_left);
			size_left -= 2;
		}
	}

	sz_strncat(result, "\n  attached_memory_table:\n\t", size_left);
	size_left = LONG_LONG_STRING_SIZE - strlen(result);
	for (i=0; i<MAX_VRF_SHM_NUM; i++) {
		sz_snprintf(buf, STRING_SIZE, "[%d]%p ", i, this->attached_memory_table[i]);
		
		sz_strncat(result, buf, size_left);
		size_left -= strlen(buf);

		if (i%5 == 4) {
			sz_strncat(result, "\n\t", size_left);
			size_left -= 2;
		}
	}

	sz_strncat(result, "\n  dirtybit_table:\n\t", size_left);
	size_left = LONG_LONG_STRING_SIZE - strlen(result);
	for (i=0; i<MAX_VRF_SHM_NUM; i++) {
		sz_snprintf(buf, STRING_SIZE, "[%d]%d ", i, this->dirtybit_table[i]);
		
		sz_strncat(result, buf, size_left);
		size_left -= strlen(buf);

		if (i%10 == 9) {
			sz_strncat(result, "\n\t", size_left);
			size_left -= 2;
		}
	}

	sz_strncat(result, "\n", size_left);
	size_left -= 1;

	sz_strncat(result, get_dbinfo(&(this->dbinfo)), size_left);

	size_left = LONG_LONG_STRING_SIZE - strlen(result);

	/* concaternate header info */
	sz_strncat(result, get_header_info(this->header_handle), size_left);
	size_left = LONG_LONG_STRING_SIZE - strlen(result);

	/* concaternate data info */
	sz_strncat(result, get_data_info(this->data_handle), size_left);
	size_left = LONG_LONG_STRING_SIZE - strlen(result);

	return result;
}

static void _show_vrfi_info(VariableRecordFile *this, FILE *stream, char* function)
{
	char *info_str=NULL;

	info_str = get_vrfi_info(this);

	notice("Vrfi Information[called by %s()]", function);
	fprintf(stream, info_str);
}
#endif

static int write_dbinfo_in_string(int fd, dbinfo_t *dbinfo)
{
	char result[STRING_INFO_SIZE]="";
	char buf[STRING_INFO_SIZE]="";
	int rv=0, size_left=0;

	memset(result, ' ', STRING_INFO_SIZE);
	memset(buf, ' ', STRING_INFO_SIZE);

	sz_strncpy(result, DB_STAMP, STRING_INFO_SIZE);
	size_left = STRING_INFO_SIZE - strlen(result);
	
	sz_snprintf(buf,STRING_INFO_SIZE, 
			"fixedsize:%d default_variable_size:%d \n",
			dbinfo->fixedsize, dbinfo->default_variable_size);
	sz_strncat(result, buf, size_left);
	size_left -= strlen(buf);

	sz_snprintf(buf,STRING_INFO_SIZE,
			"last_unused_key:%u path:%s\n", 
			dbinfo->last_unused_key, dbinfo->path);

	sz_strncat(result, buf, size_left);
	size_left -= strlen(buf);

	sz_strncat(result, "-------------------------------------", size_left);
	size_left = STRING_INFO_SIZE - strlen(result);

	/* so that head will show only string data */
	sz_strncat(result, "\n\n\n\n\n\n\n\n\n\n", size_left);
	size_left = STRING_INFO_SIZE - strlen(result);

	rv = lseek(fd, 0, SEEK_SET);
	if (rv == -1) {
		error("error while lseeking(fd:%d):%s", fd, strerror(errno));
		return FAIL;
	}

	rv = write(fd, result, STRING_INFO_SIZE);
	if (rv == -1) {
		error("error while writing dbinfo(fd:%d):%s", fd, strerror(errno));
		return FAIL;
	}
	return SUCCESS;
}

static int write_dbinfo_in_binary(int fd, dbinfo_t *dbinfo)
{
	int rv=0;
	rv = lseek(fd, BINARY_INFO_OFFSET, SEEK_SET);
	if (rv == -1) {
		error("lseeking error:%s", strerror(errno));
		return FAIL;
	}
	rv = write(fd, dbinfo, sizeof(dbinfo_t));
	if (rv == -1) {
		error("writing error:%s", strerror(errno));
		return FAIL;
	}
	
	return SUCCESS;
}

static int write_dbinfo(int fd, dbinfo_t *dbinfo)
{
	int rv=0;

	rv = write_dbinfo_in_string(fd, dbinfo);
	if (rv == FAIL) {
		error("error writing string dbinfo");
		return FAIL;
	}

	rv = write_dbinfo_in_binary(fd, dbinfo);
	if (rv == FAIL) {
		error("error writing string dbinfo");
		return FAIL;
	}

	return SUCCESS;
}

static int read_dbinfo_in_binary(int fd, dbinfo_t *dbinfo)
{
	int rv=0;

	rv = lseek(fd, BINARY_INFO_OFFSET, SEEK_SET);
	if (rv == -1) {
		error("error while lseeking(fd:%d):%s", fd, strerror(errno));
		return FAIL;
	}

	rv = read(fd, dbinfo, sizeof(dbinfo_t));
	if (rv == -1) {
		error("error reading dbinfo(fd:%d):%s", fd, strerror(errno));
		return FAIL;
	}

	return SUCCESS;
}
static int read_dbinfo(int fd, dbinfo_t *dbinfo)
{
	return read_dbinfo_in_binary(fd, dbinfo);
}

int compare_dbinfo(dbinfo_t *info1, dbinfo_t *info2)
{
	if (info1->fixedsize != info2->fixedsize) {
		warn("info1->fixedsize:%d, info2->fixedsize:%d",
				info1->fixedsize, info2->fixedsize);
		return 1;
	}
	else if (info1->default_variable_size !=
				info2->default_variable_size) {
		warn("info1->default_variable_size:%d, info2->default_variable_size:%d",
				info1->default_variable_size, info2->default_variable_size);
		return 1;
	}
	else if (strncmp(info1->path, info2->path, STRING_SIZE) != 0) {
		warn("info1->path:%s, info2->path:%s",
				info1->path, info2->path);
		return 1;
	}
	else if (strncmp(info1->dbstamp, info2->dbstamp, STRING_SIZE) != 0) {
		warn("info1->dbstamp:%s, info2->dbstamp:%s",
				info1->dbstamp, info2->dbstamp);
		return 1;
	}

	return 0;
}

static int write_data(VariableRecordFile *this)
{
	void *ptr=NULL;
	int i=0, rv=0;

	for (i=0; i<MAX_VRF_SHM_NUM; i++) {
		if (this->attach_flag_table[i] == 0) continue;
		if (this->dirtybit_table[i] == 0) continue;

		ptr = this->attached_memory_table[i];
		sb_assert(ptr);
		rv = sync_mmap(ptr, VRFI_EACH_SHM_SIZE(this));
		if ( rv != SUCCESS ) {
			error("write fail: %s", strerror(errno));
		}
		else debug("wrote attached_memory_table[%d]", i);
	}

	return SUCCESS;
}

static int write_master_file(VariableRecordFile *this)
{
	int rv=0, fail=0;

	rv = write_dbinfo(this->master_fd, &(this->dbinfo));
	if (rv == FAIL) {
		error("error writing dbinfo. (fd:%d):%s",
						this->master_fd, strerror(errno));
		fail++;
	}

	rv = write_data(this);
	if (rv == FAIL) {
		error("error writing master file data. (fd:%d):%s",
						this->master_fd, strerror(errno));
		fail++;
	}

	if (fail) return FAIL;

	return SUCCESS;
}

/* opens masterfile and check db information */
static int masterfile_open(VariableRecordFile *this)
{
	int fd=0, created=0, rv=0;

	fd = sb_open(this->dbinfo.path, OPEN_FLAG, MODE);
	if (fd == -1) {
		notice("creating vrfi master file:%s", this->dbinfo.path);
		created=1;
		fd = sb_open(this->dbinfo.path, CREATE_FLAG, MODE);
		if (fd == -1) {
			crit("unable to create file[%s]:%s",
						this->dbinfo.path, strerror(errno));
			return FAIL;
		}
	}
	this->master_fd = fd;

	if (created) {
		rv = write_dbinfo(this->master_fd, &(this->dbinfo));
		if (rv == FAIL) {
			error("error writing dbinfo to file");
			return FAIL;
		}
	}
	else {
		dbinfo_t compare;
		rv = read_dbinfo(this->master_fd, &compare);
		if (rv == FAIL) {
			error("error reading dbinfo");
			return FAIL;
		}
		if (compare_dbinfo(&(this->dbinfo), &compare) != 0) {
			error("db information is different from the creation");
			return FAIL;
		}

		this->dbinfo = compare;
	}

	return SUCCESS;
}

static int attach_shm_for_cache(VariableRecordFile *this)
{
	ipc_t ipc;
	int rv=0, n=0, i=0;
	
	n = (this->dbinfo.last_unused_key + NUM_OF_EACH_BUNDLED_KEYS -1 ) /
						NUM_OF_EACH_BUNDLED_KEYS;
	if (n <= 0) n = 1;
	CRIT("last_unused_key:%u n: %d", this->dbinfo.last_unused_key, n);

	if (n > MAX_VRF_SHM_NUM) {
		crit("n(%d) is larger than MAX_VRF_SHM_NUM(%d)", n, MAX_VRF_SHM_NUM);
		crit("increase MAX_VRF_SHM_NUM and recompile");
		return FAIL;
	}

	ipc.type     = IPC_TYPE_MMAP;
	ipc.pathname = this->dbinfo.path;
	ipc.size     = n * VRFI_EACH_SHM_SIZE(this);

	rv = alloc_mmap(&ipc, INFO_SIZE);
	if (rv != SUCCESS) {
		error("error while allocating mmap for cache data");
		return FAIL;
	}
	
	for (i=0; i<n; i++) {
		this->attached_memory_table[i] = 
						ipc.addr + VRFI_EACH_SHM_SIZE(this) * i;
		this->attach_flag_table[i] = 1;

		if (ipc.attr == MMAP_CREATED) {
//		if (this->flags != O_RDONLY) {
			memset(this->attached_memory_table[i], 
						0x00, VRFI_EACH_SHM_SIZE(this));
		}
	}

	for (i=n; i<MAX_VRF_SHM_NUM; i++) {
		this->attached_memory_table[i] = NULL;
		this->attach_flag_table[i] = 0;
	}

	return SUCCESS;
}

/* hooked functions */

static int vrfi_alloc(VariableRecordFile **this)
{
	*this = (VariableRecordFile*)sb_calloc(1, sizeof(VariableRecordFile));
	
	(*this)->header_handle = new_header_file_handle();
	(*this)->data_handle = new_data_file_handle();
	
	return SUCCESS;
}

/* flags is one of O_RDONLY, O_WRONLY or O_RDWR [as in open(2)]*/
static int vrfi_open(VariableRecordFile *this,
		char filepath[], int fixedsize, int default_variable_size, int flags)
{
	int rv=0;

	INFO("fixedsize:%d, default_variable_size:%d",
				fixedsize, default_variable_size);
	this->dbinfo.fixedsize = fixedsize;
	sz_strncpy(this->dbinfo.path, filepath, STRING_SIZE);
	sz_strncpy(this->dbinfo.dbstamp, DB_STAMP, STRING_SIZE);
	this->dbinfo.default_variable_size = default_variable_size;

	this->flags = flags;

	rv = masterfile_open(this);
	if (rv == FAIL) {
		crit("unable to open masterfile[%s]", filepath);
		return FAIL;
	}

	rv = header_open(this->header_handle, filepath);
	if (rv == FAIL) {
		crit("unable to open header file[%s]", filepath);
		return FAIL;
	}

	rv = data_open(this->data_handle, filepath);
	if (rv == FAIL) {
		crit("unable to open data file[%s]", filepath);
		return FAIL;
	}

	rv = attach_shm_for_cache(this);
	if (rv == FAIL) {
		crit("unable to attach shm");
		return FAIL;
	}

	show_vrfi_info(this, OUTSTREAM);

	return SUCCESS;
}

static int attach_shm_with_index(VariableRecordFile *this, int idx)
{
	ipc_t ipc;
	int rv;
	
    info("^^^^mmap is called^^^^^^");

	ipc.type     = IPC_TYPE_MMAP;
	ipc.pathname = this->dbinfo.path;
	ipc.size     = VRFI_EACH_SHM_SIZE(this);

	info("%s = %d", "INFO_SIZE + idx * VRFI_EACH_SHM_SIZE(this)", INFO_SIZE + (int)(idx * VRFI_EACH_SHM_SIZE(this)) );
	rv = alloc_mmap(&ipc, INFO_SIZE + idx * VRFI_EACH_SHM_SIZE(this));
	if (rv != SUCCESS) {
		error("error while allocating mmap for cache data");
		return FAIL;
	}
	
	this->attached_memory_table[idx] = ipc.addr;
	this->attach_flag_table[idx] = 1;

	if ( ipc.attr == MMAP_CREATED ) {
		memset(this->attached_memory_table[idx], 0x00,
				VRFI_EACH_SHM_SIZE(this));
		show_vrfi_info(this, OUTSTREAM);
	}

	return SUCCESS;
}

static void* get_shmaddr_with_key(VariableRecordFile *this, uint32_t key)
{
	int idx=0, rv=0;
	uint32_t offset=0;
	void *ptr=NULL;

	idx = key / NUM_OF_EACH_BUNDLED_KEYS;
	offset = key % NUM_OF_EACH_BUNDLED_KEYS;

	if (this->attach_flag_table[idx]==0) {
		rv = attach_shm_with_index(this,idx);
		if (rv == FAIL) {
			error("unable to attach shm with index[%d]", idx);
			return NULL;
		}
	}

	ptr = this->attached_memory_table[idx] + offset * MASTER_DATA_SIZE(this);

	return ptr;
}

static int vrfi_setfixed(VariableRecordFile *this, uint32_t key, void* data)
{
	void *ptr=NULL;
	int idx=0;

	if (this->flags == O_RDONLY) {
		error("trying to set data with readonly vrfi object");
		return FAIL;
	}

	ptr = get_shmaddr_with_key(this, key);
	if (ptr == NULL) {
		error("error while getting shmaddr with key[%u]", key);
		return FAIL;
	}

	memcpy(ptr, data, this->dbinfo.fixedsize);
	if (key > this->dbinfo.last_unused_key) this->dbinfo.last_unused_key = key;

	idx = key / NUM_OF_EACH_BUNDLED_KEYS;
	this->dirtybit_table[idx] = 1;

	return SUCCESS;
}

variable_data_info_t *get_variable_data_info(VariableRecordFile *this, uint32_t key);
static int vrfi_get_num_of_data(VariableRecordFile *this, uint32_t key, uint32_t *nelm)
{
	variable_data_info_t *info=NULL;
	
	info = get_variable_data_info(this, key);
	if (info == NULL) {
		crit("unable to get variable data info with key[%u]", key);
		return FAIL;
	}

	*nelm = info->ndata;
	return SUCCESS;
}

static int vrfi_getfixed_dup(VariableRecordFile *this, uint32_t key, void *data)
{
	void *ptr=NULL;

	ptr = get_shmaddr_with_key(this, key);
	if (ptr == NULL) {
		error("error while getting shmaddr with key[%u]", key);
		return FAIL;
	}

	memcpy(data, ptr, this->dbinfo.fixedsize);

	return SUCCESS;
}

static int vrfi_getfixed(VariableRecordFile *this, uint32_t key, void **data)
{
	void *ptr = NULL;

	ptr = get_shmaddr_with_key(this, key);
	if (ptr == NULL) {
		error("error while getting shmaddr with key[%u]", key);
		return FAIL;
	}

	*data = ptr;

	return SUCCESS;
}

/* XXX:packing could be done */
static int make_headers_from_location(VariableRecordFile *this,
				location_t *location, header_t headers[], int size)
{
	int default_variable_size = this->dbinfo.default_variable_size;
	uint32_t startbyte=0, endbyte=0;
	uint16_t nelm=0, maxnelm =0;
	int i=0;

	if (location->startbyte % default_variable_size != 0) {
		error("location->startbyte[%u] is not default_variable_size[%d] * n",
						location->startbyte, default_variable_size);
	}
	if (location->size % default_variable_size != 0) {
		error("location->size [%u] is not default_variable_size[%d] * n",
						location->size, default_variable_size);
	}

	startbyte = location->startbyte;
	endbyte = location->startbyte + location->size;

	maxnelm = (1 << (sizeof(headers[0].nelm)*8)) -1;

	for (i=0 ; i<size && startbyte < endbyte; i++) {
		headers[i].fileno = location->fileno;
		headers[i].start = startbyte / default_variable_size;
		nelm = (endbyte - startbyte) / default_variable_size;
		headers[i].nelm = (nelm > maxnelm) ? maxnelm : nelm;

		startbyte += maxnelm * default_variable_size;
	}
	
	if (startbyte < endbyte) {
		error("header size[%d] is small. startbyte:%u, endbyte:%u lost",
					size, startbyte, endbyte);
	}

	return i;
}

static int make_locations_from_headers(VariableRecordFile *this,
							header_t headers[], int nheaders,
							location_t locations[], int nlocations)
{
	int i=0, idx=0;
	int default_variable_size = this->dbinfo.default_variable_size;
	location_t _locations[MAX_LOCATION_AT_READ];
	int _nlocations = 0;
	
	for (i=0; i<nheaders; i++) {
		_locations[idx].startbyte = headers[i].start * default_variable_size;
		_locations[idx].size = headers[i].nelm * default_variable_size;
		_locations[idx].fileno = headers[i].fileno;
		idx++;
		if (idx >= nlocations) {
			error("idx[%d] >= nlocations[%d]", nheaders, nlocations);
			break;
		}
	}

	_nlocations = idx;

	idx=0;
	for (i=_nlocations-1; i>=0; i--) {
		locations[idx] = _locations[i];
		idx++;
	}

	return _nlocations;
}

header_pos_t* get_last_header_pos(VariableRecordFile *this, uint32_t key)
{
	void *ptr=NULL;
	header_pos_t *lastpos=NULL;
	int rv=0;

	rv = vrfi_getfixed(this, key, &ptr);
	if (rv == FAIL) {
		crit("vrfi_getfixed failed");
		return NULL;
	}

	lastpos = ptr + this->dbinfo.fixedsize + sizeof(variable_data_info_t);

	return lastpos;
}
variable_data_info_t *get_variable_data_info(VariableRecordFile *this, uint32_t key)
{
	void *ptr=NULL;
	int rv=0;
	variable_data_info_t *info=NULL;

	rv = vrfi_getfixed(this, key, &ptr);
	if (rv == FAIL) {
		crit("vrfi_getfixed failed");
		return NULL;
	}

	info = ptr + this->dbinfo.fixedsize;

	return info;
}

void show_headers(header_t headers[], int nheaders, FILE *stream)
{
	int i=0;

	for (i=0; i<nheaders; i++) {
		fprintf(stream, "[%d] start:%u, nelm:%d, fileno:%d\n",
				i, headers[i].start, headers[i].nelm, headers[i].fileno);
	}
}

void update_last_header_pos(VariableRecordFile *this,
				uint32_t key, header_pos_t *new_last_pos)
{
	header_pos_t *last_pos=NULL;
	int idx=0;
	
	last_pos = get_last_header_pos(this, key);
	if (last_pos == NULL) {
		crit("unable to get last header position with key[%u]", key);
		return;
	}

	memcpy(last_pos, new_last_pos, sizeof(header_pos_t));

	idx = key / NUM_OF_EACH_BUNDLED_KEYS;
	this->dirtybit_table[idx] = 1;
}

void update_variable_data_info(VariableRecordFile *this,
				uint32_t key, uint32_t nelm)
{
	variable_data_info_t *info=NULL;
	int idx=0;

	info = get_variable_data_info(this, key);
	if (info == NULL) {
		crit("unable to get variable data info with key[%u]", key);
		return;
	}

	info->ndata += nelm;

	idx = key / NUM_OF_EACH_BUNDLED_KEYS;
	this->dirtybit_table[idx] = 1;
}

#define MAX_HEADERS_AT_APPEND 10
static int vrfi_appendvariable(VariableRecordFile *this,
		uint32_t key, uint32_t nelm, void *data)
{
	int nheaders=0;
	int default_variable_size = this->dbinfo.default_variable_size;
	int rv=0;
	uint32_t size=0;

	location_t location;
	header_t headers[MAX_HEADERS_AT_APPEND];
	header_pos_t *last_pos=NULL, new_last_pos;

	if (this->flags == O_RDONLY) {
		error("trying to append variable data with readonly vrfi object");
		return FAIL;
	}

	if (nelm == 0) {
		warn("nelm:%u", nelm);
		return SUCCESS;
	}
	
	/* XXX: append header and append data should be atomic */

	size = nelm * default_variable_size;
	rv = append_data(this->data_handle, size, data, &location);
	if (rv == FAIL) {
		error("error appending data(nelm:%u)", nelm);
		return FAIL;
	}

	nheaders = 
	  make_headers_from_location(this, &location, headers, MAX_HEADERS_AT_APPEND);

	last_pos = get_last_header_pos(this, key);
	if (last_pos == NULL) {
		crit("unable to get last header position with key[%u]", key);
		return FAIL;
	}

	rv = append_headers(this->header_handle, key, 
				last_pos, headers, nheaders, &new_last_pos);
	if (rv == FAIL) {
		error("error appending header(key:%u, nelm:%u)", key, nelm);
		return FAIL;
	}

	update_last_header_pos(this, key, &new_last_pos);
	update_variable_data_info(this, key, nelm);
	
	return SUCCESS;
}

static int vrfi_getvariable(VariableRecordFile *this,
		uint32_t key, uint32_t skip, uint32_t nelm, void *data)
{
	header_t headers[MAX_HEADER_AT_READ];
	location_t locations[MAX_LOCATION_AT_READ];
	header_pos_t *last_pos=NULL;
	header_metadata_t header_meta;
	int nheaders=0, nlocations=0, rv=0;
	variable_data_info_t *info=NULL;
	uint32_t _nelm;

	info = get_variable_data_info(this, key);
	if (info == NULL) {
		crit("unable to get variable data info");
		return -1;
	}
	
	if (skip+nelm > info->ndata) {
		if (skip!=0 && skip >= info->ndata) { /* XXX: skip > info->ndata ??? */
			error("num of variable data[%u], but skipping [%u]",
										info->ndata, skip);
			return -1;
		}

		nelm = info->ndata - skip;
		info("setting nelm to %u", nelm);
	}

	last_pos = get_last_header_pos(this, key);
	if (last_pos == NULL) {
		crit("unable to get last header position with key[%u]", key);
		return -1;
	}
	header_meta.key = key;
	header_meta.prev = *last_pos;

	rv = get_headers(this->header_handle,
			&header_meta, skip, nelm, &_nelm, headers, MAX_HEADER_AT_READ);
	if (rv == FAIL) {
		error("get_headers failed. key:%u, skip:%u, nelm:%u", key, skip, nelm);
		return -1;
	}
	nheaders = rv;

	rv = make_locations_from_headers(this, 
						headers, nheaders, locations, MAX_LOCATION_AT_READ);
	nlocations = rv;

	rv = get_data(this->data_handle, locations, nlocations, data);
	if (rv == FAIL) {
		error("get_data failed. key:%u, skip:%u, nelm:%u", key, skip, nelm);
		return -1;
	}

	return _nelm;
}

static int vrfi_close(VariableRecordFile *this)
{
	int rv=0, fail=0;

	show_vrfi_info(this, OUTSTREAM);

	rv = write_master_file(this);
	if (rv == FAIL) {
		error("error while writing master file");
		fail++;
	}

	rv = header_close(this->header_handle);
	if (rv == FAIL) {
		error("error while closing header file");
		fail++;
	}

	rv = data_close(this->data_handle);
	if (rv == FAIL) {
		error("error while closing data file");
		fail++;
	}

	if (fail) return FAIL;

	//FIXME: why there no file closing here?

	return SUCCESS;
}

static int vrfi_sync(VariableRecordFile *this)
{
	int rv=0, fail=0;

	rv = write_master_file(this);
	if (rv == FAIL) {
		error("error while writing master file");
		fail++;
	}

	rv = header_write(this->header_handle);
	if (rv == FAIL) {
		error("error while closing header file");
		fail++;
	}

	rv = data_write(this->data_handle);
	if (rv == FAIL) {
		error("error while closing data file");
		fail++;
	}

	if (fail) return FAIL;

	return SUCCESS;
}

static void register_hooks(void)
{
#ifndef STANDALONE_DEBUG
	sb_hook_vrfi_alloc(vrfi_alloc, NULL, NULL, HOOK_MIDDLE);
	sb_hook_vrfi_open(vrfi_open, NULL, NULL, HOOK_MIDDLE);
	sb_hook_vrfi_close(vrfi_close, NULL, NULL, HOOK_MIDDLE);
	sb_hook_vrfi_sync(vrfi_sync, NULL, NULL, HOOK_MIDDLE);
	sb_hook_vrfi_set_fixed(vrfi_setfixed,NULL,NULL,HOOK_MIDDLE);
	sb_hook_vrfi_get_fixed_dup(vrfi_getfixed_dup,NULL,NULL,HOOK_MIDDLE);
	sb_hook_vrfi_get_fixed(vrfi_getfixed,NULL,NULL,HOOK_MIDDLE);
	sb_hook_vrfi_append_variable(vrfi_appendvariable,NULL,NULL,HOOK_MIDDLE);
	sb_hook_vrfi_get_variable(vrfi_getvariable,NULL,NULL,HOOK_MIDDLE);
	sb_hook_vrfi_get_num_of_data(vrfi_get_num_of_data, NULL, NULL, HOOK_MIDDLE);
#endif
}

module vrfi_module = {
    STANDARD_MODULE_STUFF,
    NULL,					/* config */
    NULL,                   /* registry */
	NULL,					/* module initialize */
    NULL,					/* child_main */
    NULL,                   /* scoreboard */
    register_hooks          /* register hook api */
};

#ifdef STANDALONE_DEBUG
char gSoftBotRoot[MAX_PATH_LEN] = SERVER_ROOT;
#define FIXEDSIZE 10
#define VARSIZE   16

int select_a_number()
{
	int rv=0, n=0;
	char buf[STRING_SIZE];

	fgets(buf, STRING_SIZE, stdin);
	buf[STRING_SIZE-1] = '\0';
	n = atoi(buf);
		
	return n;
}

char *get_variable_data(int *count)
{
#define BUFSIZE 100000
	static char variable_data[BUFSIZE][VARSIZE], *ptr=NULL;
	int idx=0;

	memset(variable_data, 0x00, sizeof(char) * 100 * VARSIZE);
	printf("enter variable data(quit will exit)\n");
	while (1) {
		fgets(variable_data[idx], VARSIZE, stdin);
		if (strncmp(variable_data[idx], "quit", 4) == 0) {
			break;
		}

		ptr = strchr(variable_data[idx], '\n');
		if (ptr != NULL) {
			*ptr = '\0';
		}

		variable_data[idx][VARSIZE-1] = '\0';
		idx++;
		if (idx == BUFSIZE) {
			printf("upto %d data supported\n", BUFSIZE);
			break;
		}
	}
	printf("variable_data[0]:%s, idx:%d\n", variable_data[0], idx);

	*count = idx;
	return (char *)variable_data;
}
void get_variable_data_from_file(VariableRecordFile *this)
{
	char file[STRING_SIZE], keystr[10], buf[BUFSIZE][VARSIZE], *ptr=NULL;
	int i, rv;
	uint32_t key;
	FILE *fp;
	printf("enter a file:");
	scanf("%s", file);

	debug("opening file[%s]", file);
	fp = fopen(file, "rt");
	if (fp == NULL) {
		error("cannot open file");
		return;
	}

	while (!feof(fp)) {
		fgets(keystr, 10, fp);
		key = atoi(keystr);
		debug("key:%u", key);

		i = 0;
		while (!feof(fp)) {
			if (fgets(buf[i], VARSIZE, fp) == NULL) {
				error("fget return NULL, i:%d", i);
				return;
			}
			if (buf[i][0] == '\n') {
				debug("end for key[%u]", key);
				break;
			}

			ptr = strchr(buf[i], '\n');
			*ptr = '\0';

			i++;
			if (i == BUFSIZE) {
				error("maximum bufsize is %d", BUFSIZE);
				return;
			}
		}

		rv = vrfi_appendvariable(this, key, i, buf);
		if (rv == FAIL) {
			error("error appending variable data");
			break;
		}
	}
	fclose(fp);
}
int main(int argc, char **argv)
{
	VariableRecordFile *this=NULL;
	char fixeddata[FIXEDSIZE] = "123456789";
	char fixedbuf[FIXEDSIZE];
	char buf[STRING_SIZE], data[1024][VARSIZE];
	int rv=0, i=0, n=0;
	uint32_t key=0;
	char *ptr=NULL;
	int exitflag=0;
	int open=0, close=0;
	uint32_t skip=0, nelm=0;

	rv = vrfi_alloc(&this);
	if (rv == FAIL) {
		error("error allocating vrfi");
	}

	open++;
	rv = vrfi_open(this, 
			SERVER_ROOT"/modules/mod_vrfi/test/vrfi.test", FIXEDSIZE, VARSIZE);
	if (rv == FAIL) error("error opening vrfi");
	else printf("\nvrfi opened\n");

	while (1) {
		printf("\nMenu\n");
		printf(" 1. set fixed data \n");
		printf(" 2. get fixed data \n");
		printf(" 3. append variable data \n");
		printf(" 4. get variable data \n");
		printf(" 5. show vrfi information \n");
		printf(" 6. get variable from file \n");
		printf(" 9. exit \n");
		printf("< open:%d, close:%d >\n", open, close);
		printf(" choose a number:");

		n = select_a_number();
		printf("selected number:%d\n", n);
		printf("\n");
		switch(n) {
			case 1: /* set fixed */
				printf("enter a key fixeddata[10] (default to %u, %s):",
															key, fixeddata);
				rv = fscanf(stdin, "%u %s", &key, fixeddata);
				fflush(stdin);
				rv = vrfi_setfixed(this, key, fixeddata);
				if (rv == FAIL) error("error setting fixeddata");
				break;
			case 2: /* get fixed */
				printf("enter a key (default to %u):", key);
				fscanf(stdin, "%u", &key);
				fflush(stdin);
				memset(fixedbuf, 0x00, FIXEDSIZE);
				rv = vrfi_getfixed_dup(this, key, fixedbuf);
				if (rv == FAIL) error("error getting fixeddata");
				else printf("fixeddata:|%s|\n", fixedbuf);
				break;
			case 3: /* append variable */
				printf("enter a key (default to %u):", key);
				fgets(buf, STRING_SIZE, stdin); buf[STRING_SIZE-1]='\0';
				key = atol(buf);
				ptr = get_variable_data(&n);
				printf("number of variable data:%d\n", n);
				rv = vrfi_appendvariable(this, key, n, ptr);
				if (rv == FAIL) error("error appending variable data");
				break;
			case 4:
				printf("enter a key: ");
				fgets(buf, STRING_SIZE, stdin); buf[STRING_SIZE-1]='\0';
				key = atol(buf);
				
				printf("enter skip: ");
				fgets(buf, STRING_SIZE, stdin); buf[STRING_SIZE-1]='\0';
				skip = atol(buf);

				printf("enter nelm: ");
				fgets(buf, STRING_SIZE, stdin); buf[STRING_SIZE-1]='\0';
				nelm = atol(buf);

				printf("key:%u, skip:%u, nelm:%u\n", key, skip, nelm);

				memset(data, 0x00, 1024 * VARSIZE);
				rv = vrfi_getvariable(this, key, skip, nelm, data);
				if (rv == FAIL) error("error getting variable data");

				for (i=0; i<rv; i++) {
					data[i][VARSIZE-1] = '\0';
					printf("[%d]: |%s|\n", i, data[i]);
				}
				break;

			case 5: /* show vrfi information */
				show_vrfi_info(this, OUTSTREAM);
				break;

			case 6:
				get_variable_data_from_file(this);
				break;

			case 9:
				rv = vrfi_close(this);
				if (rv == FAIL) error("error closing vrfi");

				exitflag++;
				break;
			default:
				printf("%d not implemented yet\n", n);
				break;
		}
		if (exitflag)
			break;
	}

	return 0;
}
#endif
