/* $Id$ */
#include "softbot.h"
#include "mod_mp/mod_mp.h"
#include "mod_api/http_client.h"

static int url_retrieve(){
	http_client_t *temp;
	char testurl[STRING_SIZE];
	char *host;
	char *path;
	char *ptr, *url;
	int nRet;
	crit("test start");
	
	memset(testurl, 0, STRING_SIZE);
	fprintf(stdout, "insert test url > ");
	fflush(stdin);
	fscanf(stdin, "%s", testurl);

	
	url = strstr(testurl, "http://");
	if ( !url ) {
		url = testurl;
	}else url = testurl + strlen("http://");

	ptr = strchr(url, '/');
	if ( ptr ) {
		*ptr = '\0';
		host = strdup(url);
		*ptr = '/';
		path = ptr;
	}else {
		host = url;
		path = "/";
	}

	crit("host[%s] path[%s]", host, path);
	
	temp = sb_run_http_client_new(host,"80");
	if(temp == NULL){
		error("sb_run_http_client_new");
		return 0;
	}
	sb_run_http_client_reset(temp);
	temp->http->request_http_ver = 1001;
	temp->http->method = "GET";
	temp->http->path = path;
	temp->http->host = host;
/*	temp->http->path = "/";*/
/*	temp->http->host = "colorful";*/
	temp->timeout = 100000;

	nRet = sb_run_http_client_makeRequest(temp, NULL);
	if(nRet != SUCCESS){
		error("sb_run_http_client_makeFullRequest");
		goto end;
	}

	nRet = sb_run_http_client_retrieve(1, &temp);
	if(nRet != SUCCESS){
		error("sb_run_http_client_retrieve failed");
		goto end;
	}
	crit("retrieve success");
/*	http_print(temp->http);*/

	
end:
	memfile_setOffset(temp->current_recv_buffer, 0);
	memfile_writeToFile(temp->current_recv_buffer, "httpclt_test",
			memfile_getSize(temp->current_recv_buffer));
	sb_run_http_client_free(temp);
	
	
	return 0;
}



static int flush_memfile_to_file(memfile *m, FILE *fp){
	int n;
	char temp[65536];
	memfile_setOffset(m, 0);
	while ( memfile_getOffset(m) < memfile_getSize(m) ){
		n = memfile_read(m, temp, 65536 );
		if ( n < 0 ) {
			error("memfile_read failed");
			return 0;
		}
		fwrite(temp, 1, n, fp);
	}
	return 1;
}

char *NTH_FILE(int i){
	static char buf[STRING_SIZE];
	sprintf(buf,"httpclt_testfile.%d", i);
	return buf;
}

static int 
file_upload(){
	http_client_t *temp;
	char testurl[STRING_SIZE];
	char *host, *path, *port;
	char *ptr, *url;
	int nRet;

	FILE *up = fopen("httpclt_upload", "r");
	FILE *down = fopen("httpclt_download", "w");

	struct stat statbuf;
	
	if ( !up || !down ) {
		crit("cannot open test file");
		return 0;
	}

	if ( fstat(fileno(up), &statbuf) < 0 ){
		error("fstat failed [%s]", strerror(errno));
		return 0;
	}
	
	crit("test start");
	
	memset(testurl, 0, STRING_SIZE);
	fprintf(stdout, "insert test url to upload> ");
	fflush(stdin);
	fscanf(stdin, "%s", testurl);

	url = strstr(testurl, "http://");
	if ( !url ) {
		url = testurl;
	}else url = testurl + strlen("http://");

	port = "80";
	ptr = strchr(url, ':');
	if ( ptr ) {
		*ptr = '\0';
		host = url;
		port = ptr+1;
		ptr = strchr(port, '/');
		if ( ptr ) {
			*ptr = '\0';
			port = strdup(port);
			path = ptr;
			*path = '/';
		}else {
			path = "/";
		}
	}else {
		ptr = strchr(url, '/');
		if ( ptr ) {
			*ptr = '\0';
			host = strdup(url);
			*ptr = '/';
			path = ptr;
		}else {
			host = url;
			path = "/";
		}
	}

	crit("host[%s] path[%s] port[%s]", host, path, port);
	
	temp = sb_run_http_client_new(host,port);
	if(temp == NULL){
		error("sb_run_http_client_new");
		return 0;
	}

	

	
	temp->http->request_http_ver = 1001;
	temp->http->method = "POST";
	temp->http->path = path;
	temp->http->host = host;
/*	temp->http->path = "/";*/
/*	temp->http->host = "colorful";*/
	temp->timeout = 10000;

	http_reserveMessageBody(temp->http, "x-softbotd/binary", statbuf.st_size);
	
	if ( sb_run_http_client_connect(temp) == FAIL ) {
		error("http_client_connect to %s:%s failed",
				temp->host, temp->port);
		return 0;
	}

	/* make request */
	if( sb_run_http_client_makeRequest(temp, NULL) != SUCCESS){
		error("sb_run_http_client_makeRequest");
		return 0;
	}

	/* send request */
	if ( sb_run_http_client_sendRequest(temp) != SUCCESS ) {
		error("send request failed at client");
		return 0;
	}
	{
		long tosend = statbuf.st_size, totalsent=0;
		char buffer[65536];
		int read, sent;
		while( totalsent < tosend ) {
			sent = 0;
			fseek(up, totalsent, SEEK_SET);
			read = fread(buffer, 1, 65536, up);
/*	crit("-------[read[%d]offset[%ld]--totalsent[%ld]--------------", */
/*				read, ftell(up), totalsent);*/
/*	sleep(1);*/
			if ( read < 0 ){
				error("fread faild : %s", strerror(errno));
				return 0;
			}
			if ( sb_run_http_client_sendRequestMore(temp, buffer, read, &sent)
					!= SUCCESS ) {
				error("sendRequestMore failed totalsent[%ld]", totalsent);
				return 0;
			}
			totalsent+= sent;
			info("sent [%d] bytes of total[%f]Kbytes", sent, (double)totalsent/(double)1000 );
		}
	}
	fclose(up);
			
			
			/* recv response */
	while(1){
		static int i = 0;
		crit("[%d]th receive", i++);
		crit("recvbuffer size[%ld] offset[%ld] ", 
				memfile_getSize(temp->current_recv_buffer),
				memfile_getOffset(temp->current_recv_buffer));

		nRet = sb_run_http_client_recvResponse(temp);

		
		if ( nRet != SUCCESS ) {
			error("receive response failed at client");
			return 0;
		}
		
		
		nRet = sb_run_http_client_parseResponse(temp);

		
		crit("parseResponse ret[%d] recvbuffer size[%ld] offset[%ld] parsing_state[%d]", 
				nRet, memfile_getSize(temp->current_recv_buffer), 
				memfile_getOffset(temp->current_recv_buffer),
				temp->parsing_status.state);
		crit("content_buf[%ld] parsed_data[%ld] flushed_data[%ld] wanted_data[%ld]",
				temp->parsing_status.content_buf_offset,
				temp->parsing_status.parsed_data_size,
				temp->parsing_status.flushed_data_size,
				temp->parsing_status.wanted_data_size);

		
		
		if ( nRet == SUCCESS ){
			if ( temp->parsing_status.state == PARSING_COMPLETE ) {
				crit("receive success");
				flush_memfile_to_file(temp->http->content_buf, down);
				sb_run_http_client_flushBuffer(temp);
				info("downloading total [%f] Kbytes", (double)ftell(down)/(double)1000 );
				break;
			}else {
				error("parse response error at client[%s|%s]", temp->host, temp->port);
				return 0;
			}
		}else {
			flush_memfile_to_file(temp->http->content_buf, down);
			sb_run_http_client_flushBuffer(temp);
			info("downloading total [%f] Kbytes", (double)ftell(down)/(double)1000 );
		}
	}

	fclose(down);
	crit("retrieve success");

	return 0;

}


static int
file_download(){
	return 0;
}


static int 
test_main(slot_t *slot){
	int test_no;

	crit("possible test list ------");
	crit("1. default url retrieve test");
	crit("2. (large) file upload test");
	crit("3. (large) file download test");
	crit("insert test no > ");
	fflush(stdin);
	fscanf(stdin, "%d", &test_no);

	switch(test_no){
		case 1:
			return url_retrieve();
		case 2:
			return file_upload();
		case 3:
			return file_download();
		default:
			crit("invalid test_no[%d]", test_no);
	}

	slot->state = SLOT_FINISH;
	return 0;
}

static scoreboard_t scoreboard[] = {PROCESS_SCOREBOARD(1)};

/* signal handler *//*{{{*/
static RETSIGTYPE _do_nothing(int sig) { return; }
/*static RETSIGTYPE _start_index(int sig) { start_index_flag = 1; }*/

static RETSIGTYPE _shutdown(int sig)
{
	struct sigaction act;

	memset(&act, 0x00, sizeof(act));

	act.sa_flags = SA_RESTART;
	sigfillset(&act.sa_mask);

	act.sa_handler = _do_nothing;
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

	scoreboard->shutdown++;
}

static RETSIGTYPE _graceful_shutdown(int sig)
{
	struct sigaction act;

	memset(&act, 0x00, sizeof(act));

	act.sa_flags = SA_RESTART;
	sigfillset(&act.sa_mask);

	act.sa_handler = _do_nothing;
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGINT, &act, NULL);

	scoreboard->graceful_shutdown++;
}/*}}}*/

static int module_main(slot_t *slot) {
	sb_set_default_sighandlers(_shutdown, _graceful_shutdown); 
	sb_init_scoreboard(scoreboard); 
	sb_spawn_processes(scoreboard,"test_httpclient_module process",test_main);
	scoreboard->period = 5;
	sb_monitor_processes(scoreboard);

	return 0;
}

module test_httpclient_module = {
	STANDARD_MODULE_STUFF,
    NULL,                   /* config */
    NULL,                   /* registry */
    NULL,                   /* initialize */
    module_main,            /* child_main */
    scoreboard,                   /* scoreboard */
    NULL                    /* register hook api */
};
