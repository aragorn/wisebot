/* $Id$ */
#include "softbot.h"
#include "header_handle.h"
#include "mod_vrfi.h"

#define OPEN_FLAG   (O_RDWR)
#define CREATE_FLAG (O_RDWR|O_CREAT)
#define MODE        (S_IREAD|S_IWRITE)

#define MAX_VRF_HEADERFILE 32

typedef struct {
	int last_header_file_idx;
	uint32_t last_file_used_byte;
	char path[STRING_SIZE];
} header_info_t;

struct header_handle_t {
	int fds[MAX_VRF_HEADERFILE];
	int info_fd;
	header_info_t headerinfo;
};

char *get_header_info(header_handle_t *this)
{
	static char result[LONG_STRING_SIZE];
	int size_left = LONG_STRING_SIZE;
	char buf[LONG_STRING_SIZE];

	sz_snprintf(result, LONG_STRING_SIZE, "\nVariable header file information:\n");
	size_left = LONG_STRING_SIZE - strlen(result);

	sz_snprintf(buf, LONG_STRING_SIZE,
			"  last_header_file_idx:%d, last_file_used_byte:%d\n",
			this->headerinfo.last_header_file_idx,
			this->headerinfo.last_file_used_byte);
	sz_strncat(result, buf, size_left);
	size_left -= strlen(buf);

	sz_snprintf(buf, LONG_STRING_SIZE, "  path:%s", this->headerinfo.path);
	sz_strncat(result, buf, size_left);
	size_left -= strlen(buf);

	return result;
}

static int write_headerinfo_in_string(int fd, header_info_t *headerinfo)
{
	char result[STRING_INFO_SIZE]="";
	char buf[STRING_INFO_SIZE]="";
	int rv = 0;
	int size_left = 0;

	memset(result, ' ', STRING_INFO_SIZE);
	memset(buf, ' ', STRING_INFO_SIZE);

	sz_snprintf(result, STRING_INFO_SIZE,
			"last_header_file_idx:%d, last_file_used_byte:%u\n",
			headerinfo->last_header_file_idx, headerinfo->last_file_used_byte);
	size_left = STRING_INFO_SIZE - strlen(result);

	sz_snprintf(buf, STRING_INFO_SIZE, "path:%s\n", headerinfo->path);
	sz_strncat(result, buf, size_left);
	size_left -= strlen(buf);

	sz_strncat(result, "-------------------------------------", size_left);
	size_left = STRING_INFO_SIZE - strlen(result);

	/* so that head will show only string data */
	sz_strncat(result, "\n\n\n\n\n\n\n\n\n\n", size_left);
	size_left = STRING_INFO_SIZE - strlen(result);

	rv = lseek(fd, 0, SEEK_SET);
	if (rv == -1) {
		error("lseek failed(fd:%d):%s", fd, strerror(errno));
		return FAIL;
	}

	rv = write(fd, result, STRING_INFO_SIZE);
	if (rv == -1) {
		error("write failed.(fd:%d):%s", fd, strerror(errno));
		return FAIL;
	}

	return SUCCESS;

}
static int write_headerinfo_in_binary(int fd, header_info_t *headerinfo)
{
	int rv=0;
	
	rv = lseek(fd, BINARY_INFO_SIZE, SEEK_SET);
	if (rv == -1) {
		error("lseek failed:%s", strerror(errno));
		return FAIL;
	}

	rv = write(fd, headerinfo, sizeof(header_info_t));
	if (rv == -1) {
		error("write failed:%s", strerror(errno));
		return FAIL;
	}

	return SUCCESS;
}

static int write_headerinfo(int fd, header_info_t *headerinfo)
{
	int rv=0;
	rv = write_headerinfo_in_string(fd, headerinfo);
	if (rv == FAIL) {
		error("error while writing headerinfo in string");
		return FAIL;
	}

	rv = write_headerinfo_in_binary(fd, headerinfo);
	if (rv == FAIL) {
		error("error while writing headerinfo in binary");
		return FAIL;
	}

	return SUCCESS;
}
static int read_headerinfo_in_binary(int fd, header_info_t *headerinfo)
{
	int rv=0;
	rv = lseek(fd, BINARY_INFO_SIZE, SEEK_SET);
	if (rv == -1) {
		error("lseek failed:%s", strerror(errno));
		return FAIL;
	}

	rv = read(fd, headerinfo, sizeof(header_info_t));
	if (rv == -1) {
		error("read failed:%s", strerror(errno));
		return FAIL;
	}
	else if (rv != sizeof(header_info_t)) {
		error("error while read. got %d bytes expecting %d bytes",
				rv, (int)sizeof(header_info_t));
		return FAIL;
	}

	return SUCCESS;
}
static int read_headerinfo(int fd, header_info_t *headerinfo)
{
	return read_headerinfo_in_binary(fd, headerinfo);
}

static int header_info_open(header_handle_t *this)
{
	int fd=0, created=0, rv=0;
	char infofile[STRING_SIZE];

	sz_snprintf(infofile, STRING_SIZE,
				"%s%s", this->headerinfo.path, ".info");

	fd = sb_open(infofile, OPEN_FLAG, MODE);
	if (fd == -1) {
		notice("creating vrfi header information file:%s", infofile);
		created=1;
		fd = sb_open(infofile, CREATE_FLAG, MODE);
		if (fd == -1) {
			crit("unable to create file[%s]:%s",
						this->headerinfo.path, strerror(errno));
			return FAIL;
		}
	}
	this->info_fd = fd;

	if (created) {
		rv = write_headerinfo(this->info_fd, &(this->headerinfo));
		if (rv == FAIL) {
			error("error writing header info to file");
			return FAIL;
		}
	}
	else {
		rv = read_headerinfo(this->info_fd, &(this->headerinfo));
		if (rv == FAIL) {
			error("error reading header info");
			return FAIL;
		}
	}

	return SUCCESS;
}

static int header_info_close(header_handle_t *this)
{
	int rv = 0;

	rv = write_headerinfo(this->info_fd, &(this->headerinfo));
	if (rv == FAIL) {
		error("error writing header info to file");
		return FAIL;
	}
	return SUCCESS;
}

header_handle_t *new_header_file_handle()
{
	header_handle_t* handle=NULL;
	handle = sb_calloc(1, sizeof(header_handle_t));
	if (handle == NULL) {
		error("error allocating header file handle");
		return NULL;
	}

	return handle;
}

static void put_dummy_header(int fd)
{
	header_data_t headerdata;
	int rv=0;

	info("putting dummy header for fd[%d]", fd);

	memset(&headerdata, 0x00, sizeof(header_data_t));

	rv = lseek(fd, 0, SEEK_SET);
	if (rv == -1) {
		crit("lseek failed:%s", strerror(errno));
	}

	rv = write(fd, &headerdata, sizeof(header_data_t));
	if (rv == -1) {
		crit("write failed:%s", strerror(errno));
	}
}

static int each_header_open(char path[], int idx)
{
	char filename[STRING_SIZE];
	int fd=0;

	sz_snprintf(filename, STRING_SIZE, "%s.%02d", path, idx);

	fd = sb_open(filename, OPEN_FLAG, MODE);
	if (fd == -1) {
		notice("creating vrfi header file:%s idx:%d", filename, idx);
		fd = sb_open(filename, CREATE_FLAG, MODE);
		if (fd == -1) {
			crit("unable to create file[%s] idx:%d :%s",
								filename, idx, strerror(errno));
			return -1;
		}

		if (idx == 0) put_dummy_header(fd);
	}

	return fd;
}

static int all_headers_open(header_handle_t *this)
{
	int i=0, fd=0;
	header_info_t *headerinfo=&(this->headerinfo);

	for (i=0; i <= headerinfo->last_header_file_idx; i++) {
		fd = each_header_open(headerinfo->path, i);
		if (fd == -1) {
			error("error opening variable header file(path:%s, idx:%d)",
												headerinfo->path, i);
			return FAIL;
		}
		this->fds[i] = fd;
	}

	return SUCCESS;
}

#define DUMMY_HEADER_SIZE sizeof(header_data_t)
int header_open(header_handle_t *this, char filepath[])
{
	int rv=0;

	sz_strncpy(this->headerinfo.path, filepath, STRING_SIZE);
	sz_strncat(this->headerinfo.path, HEADER_TAG,
						STRING_SIZE - strlen(this->headerinfo.path));
	this->headerinfo.last_header_file_idx = 0;
	this->headerinfo.last_file_used_byte = DUMMY_HEADER_SIZE;

	rv = header_info_open(this);
	if (rv == FAIL) {
		error("error while header info open");
		return FAIL;
	}

	rv = all_headers_open(this);
	if (rv == FAIL) {
		error("error while headers open");
		return FAIL;
	}

	return SUCCESS;
}

int header_close(header_handle_t *this)
{
	int rv = 0;

	rv = header_info_close(this);
	if (rv == FAIL) {
		error("error while header info close");
		return FAIL;
	}

	/* header data is synchronized immediately
	 * so, no need to synch
	 */
	//FIXME: why there's no file closing here?
	return SUCCESS;
}
int header_write(header_handle_t *this)
{
	int rv=0;

	rv = write_headerinfo(this->info_fd, &(this->headerinfo));
	if (rv == FAIL) {
		error("error while writing header info to file");
		return FAIL;
	}
	return SUCCESS;
}

#define MAX_HEADERDATA_SIZE 20
int append_headers(/*in*/header_handle_t *this, 
					/*in*/uint32_t key, /*in*/header_pos_t *lastpos,
					/*in*/header_t headers[], /*in*/int nelm,
					/*out*/header_pos_t *new_last_pos)
{
	int rv=0, idx=0, i=0;
	header_data_t headerdata[MAX_HEADERDATA_SIZE];
	uint32_t last_used_byte=0, size=0;

	if (nelm > MAX_HEADERDATA_SIZE) {
		crit("nelm[%d] > MAX_HEADERDATA_SIZE[%d]", nelm, MAX_HEADERDATA_SIZE);
		return FAIL;
	}

	idx = this->headerinfo.last_header_file_idx;
	size = (uint32_t)nelm * sizeof(header_data_t);
	if (this->headerinfo.last_file_used_byte + size > HEADERFILE_SIZE) {
		char *path = this->headerinfo.path;
		int fd=0;

		idx = this->headerinfo.last_header_file_idx+1;
		fd = each_header_open(path, idx);
		if (fd == -1) {
			error("error opening header data[%s] index:%d", path, idx);
			return FAIL;
		}

		this->headerinfo.last_header_file_idx++;
		this->headerinfo.last_file_used_byte=0;
		this->fds[idx] = fd;
	}

	last_used_byte = this->headerinfo.last_file_used_byte;

	for (i=0; i<nelm; i++) {
		headerdata[i].header = headers[i];
		headerdata[i].metadata.key = key;

		if (i==0) 
			headerdata[i].metadata.prev = *lastpos;
		else {
			headerdata[i].metadata.prev.fileno = idx;
			headerdata[i].metadata.prev.position = 
				last_used_byte + sizeof(header_data_t) * (i-1);
		}
	}

	new_last_pos->fileno = idx;
	new_last_pos->position = last_used_byte + sizeof(header_data_t) * (i-1);

	rv = lseek(this->fds[idx], this->headerinfo.last_file_used_byte, SEEK_SET);
	if (rv == -1) {
		error("lseek failed:%s", strerror(errno));
		return FAIL;
	}

	rv = write(this->fds[idx], headerdata, sizeof(header_data_t) * nelm);
	if (rv == -1) {
		error("write failed:%s", strerror(errno));
		return FAIL;
	}

	this->headerinfo.last_file_used_byte += (sizeof(header_data_t) * nelm);
	
	return SUCCESS;
}

int read_one_header(header_handle_t *this,
		header_metadata_t *header_meta, header_data_t *header_data)
{
	uint8_t fileno=0;
	uint32_t position=0;
	int rv=0;

	fileno = header_meta->prev.fileno;
	position = header_meta->prev.position;

	/*
	debug("reading with metadata(fileno:%d, position:%u, key:%u)",
						fileno, position, header_meta->key);*/

	if (this->fds[fileno] == 0) { // if file is not yet opened
		int fd = 0;
		fd = each_header_open(this->headerinfo.path, fileno);
		if (fd == -1) {
			error("error opening header data[%s] index:%d", 
					this->headerinfo.path, fileno);
			return FAIL;
		}

		this->fds[fileno] = fd;
	}

	rv = lseek(this->fds[fileno], position, SEEK_SET);
	if (rv == -1) {
		error("lseek failed:%s", strerror(errno));
		return FAIL;
	}

	rv = read(this->fds[fileno], header_data, sizeof(header_data_t));
	if (rv == -1) {
		error("read failed:%s", strerror(errno));
		return FAIL;
	}
	else if(rv != sizeof(header_data_t)) {
		error("error while read. got %d bytes, expecting %d bytes",
						rv, (int)sizeof(header_data_t));
		return FAIL;
	}

	return SUCCESS;
}

static int is_first_header(header_data_t *headerdata)
{
	header_metadata_t *headermeta=&(headerdata->metadata);
	if (headermeta->prev.fileno == 0 && 
			headermeta->prev.position == 0)
		return TRUE;

	return FALSE;
}

// 2004.11.03 실제 nelm값을 리턴하도록 r_nelm추가
int get_headers(/*in*/header_handle_t *this,
				/*in*/header_metadata_t *header_meta,
				/*in*/uint32_t skip, /*in*/uint32_t nelm, /*out*/uint32_t *r_nelm,
				/*out*/header_t headers[], /*in*/ int size)
{
	uint32_t key=0;
	uint32_t _skip=0, _nelm = 0; /* actually skipped, read size */
	int rv=0, idx=0;
	header_data_t header_data;
	header_metadata_t _headermeta = *header_meta;

	key = header_meta->key;
	while(_nelm < nelm) {
		header_t *header=NULL;

		rv = read_one_header(this, &_headermeta, &header_data);
		if (rv == FAIL) {
			error("read_one_header failed");
			return FAIL;
		}
		else if (header_data.metadata.key != key) {
			error("integrity error. given key[%u], read key[%u] differ",
											key, header_data.metadata.key);
			return FAIL;
		}
		
		/*
		debug("header_data.key:%u, header.start:%u, header.nelm:%d",
				header_data.metadata.key,
				header_data.header.start, header_data.header.nelm);*/

		header = &(header_data.header);

		if (_skip + header->nelm <= skip) {
			_skip += header->nelm;
			_headermeta = header_data.metadata;
			continue;
		}
		else if (_skip < skip && _skip + header->nelm > skip) {
			header->nelm -= (skip - _skip);

			_skip += (skip - _skip);
		}

		if (_nelm + header->nelm > nelm) {
			uint32_t nreads = nelm - _nelm;
			header->start += header->nelm - nreads;
			header->nelm = nreads;
		}
		_nelm += header->nelm;
	
		headers[idx] = header_data.header;
		idx++;
		if (idx >= size && _nelm < nelm) {
			error("header size[%d] is too small. _nelm[%u] data is not read",
							size, _nelm);
			break;
		}

		if (is_first_header(&header_data) == TRUE &&
				_nelm < nelm) {
			warn("no more header to read, but _nelm[%u] data to read", _nelm);
			break;
		}

		_headermeta = header_data.metadata;
	}
	*r_nelm = _nelm;

	return idx;
}
