/* $Id$ */
#include "softbot.h"
#include "data_handle.h"
#include "mod_vrfi.h"

#define OPEN_FLAG   (O_RDWR)
#define CREATE_FLAG (O_RDWR|O_CREAT)
#define MODE        (S_IREAD|S_IWRITE)

#define MAX_VRF_DATAFILE 32

typedef struct {
	int start_data_file_idx;
	int last_data_file_idx;
	uint32_t last_file_used_byte;
	char path[STRING_SIZE];
} data_info_t;

struct data_handle_t {
	int fds[MAX_VRF_DATAFILE];
	int info_fd;
	data_info_t datainfo;
};

char *get_data_info(data_handle_t *this)
{
	static char result[LONG_STRING_SIZE];
	int size_left = LONG_STRING_SIZE;
	char buf[LONG_STRING_SIZE];
	int i=0;

	sz_snprintf(result, LONG_STRING_SIZE, "\nVariable data file information:\n");
	size_left = LONG_STRING_SIZE - strlen(result);

	sz_snprintf(buf, LONG_STRING_SIZE, 
		"  start_data_file_idx:%d, last_data_file_idx:%d\n",
		this->datainfo.start_data_file_idx, this->datainfo.last_data_file_idx);
	
	sz_strncat(result, buf, size_left);
	size_left -= strlen(buf);

	sz_snprintf(buf, LONG_STRING_SIZE,
		"  last_file_used_byte:%u \n", 
		this->datainfo.last_file_used_byte);
	sz_strncat(result, buf, size_left);
	size_left -= strlen(buf);

	sz_snprintf(buf, LONG_STRING_SIZE, "  path:%s\n", this->datainfo.path);
	
	sz_strncat(result, buf, size_left);
	size_left -= strlen(buf);

	sz_strncat(result, "  data file fds:\n\t", size_left);
	size_left = LONG_STRING_SIZE - strlen(result);
	for (i=0; i<MAX_VRF_DATAFILE; i++) {
		sz_snprintf(buf, STRING_SIZE, "[%d]%d ", i, this->fds[i]);

		sz_strncat(result, buf, size_left);
		size_left -= strlen(buf);

		if (i%10 == 9) {
			sz_strncat(result, "\n\t", size_left);
			size_left -= 2;
		}
	}

	sz_strncat(result, "\n", size_left);
	size_left -= 1;

	return result;
}

static int write_datainfo_in_string(int fd, data_info_t *datainfo)
{
	char result[STRING_INFO_SIZE]="";
	char buf[STRING_INFO_SIZE]="";
	int rv=0;
	int size_left=0;

	memset(result, ' ', STRING_INFO_SIZE);
	memset(buf, ' ', STRING_INFO_SIZE);

	sz_snprintf(result, STRING_INFO_SIZE, 
			"start_data_file_idx:%d, last_data_file_idx:%d\n",
			datainfo->start_data_file_idx, datainfo->last_data_file_idx);
	size_left = STRING_INFO_SIZE - strlen(result);

	sz_snprintf(buf, STRING_INFO_SIZE,
			"last_file_used_byte:%u\n", datainfo->last_file_used_byte);
	sz_strncat(result, buf, size_left);
	size_left -= strlen(buf);
	
	sz_snprintf(buf, STRING_INFO_SIZE, "path:%s\n", datainfo->path);
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

static int write_datainfo_in_binary(int fd, data_info_t *datainfo)
{
	int rv=0;
	
	rv = lseek(fd, BINARY_INFO_OFFSET, SEEK_SET);
	if (rv == -1) {
		error("lseeking error(fd:%d, offset:%d):%s",
						fd, BINARY_INFO_OFFSET, strerror(errno));
		return FAIL;
	}

	rv = write(fd, datainfo, sizeof(data_info_t));
	if (rv == -1) {
		error("writing error(fd:%d, size:%d, buffer ptr:%p):%s[%d]", 
						fd, (int)sizeof(data_info_t), datainfo, 
						strerror(errno), errno);
		info("EBADF: %d, EFAULT: %d, EINVAL: %d", EBADF, EFAULT, EINVAL);
		error("previously, lseeked with offset:%d", BINARY_INFO_OFFSET);
		return FAIL;
	}

	return SUCCESS;
}

static int write_datainfo(int fd, data_info_t *datainfo)
{
	int rv=0;
	rv = write_datainfo_in_string(fd, datainfo);
	if (rv == FAIL) {
		error("error while writing dbinfo in string");
		return FAIL;
	}

	rv = write_datainfo_in_binary(fd, datainfo);
	if (rv == FAIL) {
		error("error while writing dbinfo in binary");
		return FAIL;
	}

	return SUCCESS;
}
static int read_datainfo_in_binary(int fd, data_info_t *datainfo)
{
	int rv=0;
	rv = lseek(fd, BINARY_INFO_OFFSET, SEEK_SET);
	if (rv == -1) {
		error("lseek failed:%s", strerror(errno));
		return FAIL;
	}

	rv = read(fd, datainfo, sizeof(data_info_t));
	if (rv == -1) {
		error("read[fd:%d] failed:%s", fd, strerror(errno));
		return FAIL;
	}
	else if (rv != sizeof(data_info_t)) {
		error("error while read. got %d bytes, expecting %d bytes",
				rv, (int)sizeof(data_info_t));
		return FAIL;
	}

	return SUCCESS;
}
static int read_datainfo(int fd, data_info_t *datainfo)
{
	return read_datainfo_in_binary(fd, datainfo);
}

static int data_info_open(data_handle_t *this)
{
	int fd=0, created=0, rv=0;
	char infofile[STRING_SIZE];

	sz_snprintf(infofile, STRING_SIZE, 
			"%s%s", this->datainfo.path, ".info");

	fd = sb_open(infofile, OPEN_FLAG, MODE);
	if (fd == -1) {
		notice("creating vrfi data information file:%s", infofile);
		created=1;
		fd = sb_open(infofile, CREATE_FLAG, MODE);
		if (fd == -1) {
			crit("unable to create file[%s]:%s",
						this->datainfo.path, strerror(errno));
			return FAIL;
		}
	}
	this->info_fd = fd;

	if (created) {
		rv = write_datainfo(this->info_fd, &(this->datainfo));
		if (rv == FAIL) {
			error("error writing data info to file");
			return FAIL;
		}
	}
	else {
		rv = read_datainfo(this->info_fd, &(this->datainfo));
		if (rv == FAIL) {
			error("error reading info");
			return FAIL;
		}
	}

	return SUCCESS;
}
static int data_info_close(data_handle_t *this)
{
	int rv=0;

	rv = write_datainfo(this->info_fd, &(this->datainfo));
	if (rv == FAIL) {
		error("error writing data info to file");
		return FAIL;
	}

	return SUCCESS;
}

static int each_data_open(char path[], int idx)
{
	char filename[STRING_SIZE];
	int fd=0;

	sz_snprintf(filename, STRING_SIZE,
			"%s.%02d", path, idx);

	fd = sb_open(filename, OPEN_FLAG, MODE);
	if (fd == -1) {
		notice("creating vrfi data file:%s idx:%d", filename, idx);
		fd = sb_open(filename, CREATE_FLAG, MODE);
		if (fd == -1) {
			crit("unable to create data file[%s] idx:%d :%s",
								filename, idx, strerror(errno));
		}
	}

	return fd;
}

static int all_data_open(data_handle_t *this)
{
	int i=0, fd=0;
	data_info_t *datainfo=&(this->datainfo);

	for (i=datainfo->start_data_file_idx;
				i <= datainfo->last_data_file_idx; i++) {
		fd = each_data_open(datainfo->path, i);
		if (fd == -1) {
			error("error opening variable data file(path:%s, idx:%d)",
												datainfo->path, i);
			return FAIL;
		}
		this->fds[i] = fd;
	}

	return SUCCESS;
}

data_handle_t *new_data_file_handle()
{
	data_handle_t* handle=NULL;
	handle = sb_calloc(1, sizeof(data_handle_t));
	if (handle == NULL) {
		error("error allocating data file handle");
		return NULL;
	}

	return handle;
}

int data_open(data_handle_t *this, char filepath[])
{
	int rv=0;
	
	sz_strncpy(this->datainfo.path, filepath, STRING_SIZE);
	sz_strncat(this->datainfo.path, DATA_TAG,
					STRING_SIZE - strlen(this->datainfo.path));
	this->datainfo.start_data_file_idx = 0;
	this->datainfo.last_data_file_idx = 0;
	this->datainfo.last_file_used_byte = 0;

	rv = data_info_open(this);
	if (rv == FAIL) {
		error("error while data info open");
		return FAIL;
	}

	rv = all_data_open(this);
	if (rv == FAIL) {
		error("error while datafile open");
		return FAIL;
	}

	return SUCCESS;
}

int data_close(data_handle_t *this)
{
	int rv=0;

	rv = data_info_close(this);
	if (rv == FAIL) {
		error("error while data info close");
		return FAIL;
	}

	/* data is synchronized immediately, so
	 * no need to synchronized it here
	 */
	//FIXME: why there's no file closing here? 01/17, --jiwon
	//       like data.00 closing...
	return SUCCESS;
}

int data_write(data_handle_t *this)
{
	int rv=0;

	rv = write_datainfo(this->info_fd, &(this->datainfo));
	if (rv == FAIL) {
		error("error writing data info to file");
		return FAIL;
	}

	return SUCCESS;
}

int append_data(data_handle_t *this, 
		uint32_t size, void *data, location_t *location)
{
	int idx=0, rv=0;

	idx = this->datainfo.last_data_file_idx;
	if (this->datainfo.last_file_used_byte + size > DATAFILE_SIZE) {
		char *path = this->datainfo.path;
		int fd=0;

		idx = this->datainfo.last_data_file_idx+1;
		fd = each_data_open(path, idx);
		if (fd == -1) {
			error("error opening variable data[%s] index:%d", path, idx);
			return FAIL;
		}

		this->datainfo.last_data_file_idx++;
		this->datainfo.last_file_used_byte=0;
		this->fds[idx] = fd;
	}

	rv = lseek(this->fds[idx], this->datainfo.last_file_used_byte, SEEK_SET);
	if (rv == -1) {
		error("lseek failed:%s", strerror(errno));
		return FAIL;
	}

	rv = write(this->fds[idx], data, size);
	if (rv == -1) {
		error("writ failed:%s", strerror(errno));
		return FAIL;
	}

	location->fileno = idx;
	location->startbyte = this->datainfo.last_file_used_byte;
	location->size = size;

	this->datainfo.last_file_used_byte += size;

	return SUCCESS;
}

int get_data(data_handle_t *this,
			location_t locations[], int nlocations, void *data)
{
	void *ptr=NULL;
	int i=0, rv=0, idx=0;

	ptr = data;

	for (i=0; i<nlocations; i++) {
		idx = locations[i].fileno;

		if (this->fds[idx] == 0) { // if not yet opened
			int fd;

			fd = each_data_open(this->datainfo.path, idx);
			if (fd == -1) {
				error("error opening variable data[%s] index:%d", 
						this->datainfo.path, idx);
				return FAIL;
			}

			this->fds[idx] = fd;
		}

		rv = lseek(this->fds[idx], locations[i].startbyte, SEEK_SET);
		if (rv == -1) {
			error("lseek failed(idx:%d, fd:%d,offset:%d) :%s",
						idx, this->fds[idx], locations[i].startbyte, strerror(errno));
			return FAIL;
		}

		rv = read(this->fds[idx], ptr, locations[i].size);
		if (rv == -1) {
			error("read[fd:%d] failed:%s", this->fds[idx], strerror(errno));
			return FAIL;
		}
		else if (rv != locations[i].size) {
			error("error while read. got %d byte, expecting %d byte",
										rv, locations[i].size);
			return FAIL;
		}

		ptr += locations[i].size;
	}
	
	return SUCCESS;
}
