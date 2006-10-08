/* $Id$ */
#ifndef MOD_HTTPD_H
#define MOD_HTTPD_H

//#include "common_core.h" /* slot_t */
#include "apr_tables.h" /* apr_array_header_t */
#include "apr_buckets.h" /* apr_bucket_brigade */
#include "apr_uri.h" /* apr_uri_t */

/* ----------------------------- config dir ------------------------------ */

/* Define this to be the default server home dir. Most things later in this
 * file with a relative pathname will have this added.
 */
#ifndef HTTPD_ROOT
# ifdef OS2
/* Set default for OS/2 file system */
#  define HTTPD_ROOT "/os2httpd"
# elif defined(WIN32)
/* Set default for Windows file system */
#  define HTTPD_ROOT "/apache"
# elif defined (BEOS)
/* Set the default for BeOS */
#  define HTTPD_ROOT "/boot/home/apache"
# elif defined (NETWARE)
/* Set the default for NetWare */
#  define HTTPD_ROOT "/apache"
# else
#  define HTTPD_ROOT "/usr/local/apache"
# endif
#endif /* HTTPD_ROOT */

/* 
 * --------- You shouldn't have to edit anything below this line ----------
 *
 * Any modifications to any defaults not defined above should be done in the 
 * respective configuration file. 
 *
 */

/* Default location of documents.  Can be overridden by the DocumentRoot
 * directive.
 */
#ifndef DOCUMENT_LOCATION
#ifdef OS2
/* Set default for OS/2 file system */
#define DOCUMENT_LOCATION  HTTPD_ROOT "/docs"
#else
#define DOCUMENT_LOCATION  HTTPD_ROOT "/htdocs"
#endif 
#endif /* DOCUMENT_LOCATION */

/* Default administrator's address */
#define DEFAULT_ADMIN "[no address given]"

/* Define this to be what your per-directory security files are called */
#ifndef DEFAULT_ACCESS_FNAME
#ifdef OS2
/* Set default for OS/2 file system */
#define DEFAULT_ACCESS_FNAME "htaccess"
#else
#define DEFAULT_ACCESS_FNAME ".htaccess"
#endif
#endif /* DEFAULT_ACCESS_FNAME */

/* Whether we should enable rfc1413 identity checking */
#ifndef DEFAULT_RFC1413
#define DEFAULT_RFC1413 0
#endif

/* The timeout for waiting for messages */
#ifndef DEFAULT_TIMEOUT
#define DEFAULT_TIMEOUT 15
#endif

/* The timeout for waiting for keepalive timeout until next request */
#ifndef DEFAULT_KEEPALIVE_TIMEOUT
#define DEFAULT_KEEPALIVE_TIMEOUT 60
#endif

/* The number of requests to entertain per connection */
#ifndef DEFAULT_KEEPALIVE
#define DEFAULT_KEEPALIVE 100
#endif

/* Limits on the size of various request items.  These limits primarily
 * exist to prevent simple denial-of-service attacks on a server based
 * on misuse of the protocol.  The recommended values will depend on the
 * nature of the server resources -- CGI scripts and database backends
 * might require large values, but most servers could get by with much
 * smaller limits than we use below.  The request message body size can
 * be limited by the per-dir config directive LimitRequestBody.
 *
 * Internal buffer sizes are two bytes more than the DEFAULT_LIMIT_REQUEST_LINE
 * and DEFAULT_LIMIT_REQUEST_FIELDSIZE below, which explains the 8190.
 * These two limits can be lowered (but not raised) by the server config
 * directives LimitRequestLine and LimitRequestFieldsize, respectively.
 *
 * DEFAULT_LIMIT_REQUEST_FIELDS can be modified or disabled (set = 0) by
 * the server config directive LimitRequestFields.
 */
#ifndef DEFAULT_LIMIT_REQUEST_LINE
#define DEFAULT_LIMIT_REQUEST_LINE 8190
#endif /* default limit on bytes in Request-Line (Method+URI+HTTP-version) */
#ifndef DEFAULT_LIMIT_REQUEST_FIELDSIZE
#define DEFAULT_LIMIT_REQUEST_FIELDSIZE 8190
#endif /* default limit on bytes in any one header field  */
#ifndef DEFAULT_LIMIT_REQUEST_FIELDS
#define DEFAULT_LIMIT_REQUEST_FIELDS 100
#endif /* default limit on number of request header fields */


/**
 * The default default character set name to add if AddDefaultCharset is
 * enabled.  Overridden with AddDefaultCharsetName.
 */
#define DEFAULT_ADD_DEFAULT_CHARSET_NAME "euc-kr"

/** default HTTP Server protocol */
#define SERVER_PROTOCOL "HTTP/1.1"

/** The default string lengths */
#define MAX_STRING_LEN HUGE_STRING_LEN
#define HUGE_STRING_LEN 8192

/** The size of the server's internal read-write buffers */
#define IOBUFSIZE 8192


/* ------------------ stuff that modules are allowed to look at ----------- */

/** Define this to be what your HTML directory content files are called */
#ifndef AP_DEFAULT_INDEX
#define AP_DEFAULT_INDEX "index.html"
#endif


/** 
 * Define this to be what type you'd like returned for files with unknown 
 * suffixes.  
 * @warning MUST be all lower case. 
 */
#ifndef DEFAULT_CONTENT_TYPE
#define DEFAULT_CONTENT_TYPE "text/plain"
#endif

/** The name of the MIME types file */
#ifndef AP_TYPES_CONFIG_FILE
#define AP_TYPES_CONFIG_FILE "conf/mime.types"
#endif

/*
 * Define the HTML doctype strings centrally.
 */
/** HTML 2.0 Doctype */
#define DOCTYPE_HTML_2_0  "<!DOCTYPE HTML PUBLIC \"-//IETF//" \
                          "DTD HTML 2.0//EN\">\n"
/** HTML 3.2 Doctype */
#define DOCTYPE_HTML_3_2  "<!DOCTYPE HTML PUBLIC \"-//W3C//" \
                          "DTD HTML 3.2 Final//EN\">\n"
/** HTML 4.0 Strict Doctype */
#define DOCTYPE_HTML_4_0S "<!DOCTYPE HTML PUBLIC \"-//W3C//" \
                          "DTD HTML 4.0//EN\"\n" \
                          "\"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
/** HTML 4.0 Transitional Doctype */
#define DOCTYPE_HTML_4_0T "<!DOCTYPE HTML PUBLIC \"-//W3C//" \
                          "DTD HTML 4.0 Transitional//EN\"\n" \
                          "\"http://www.w3.org/TR/REC-html40/loose.dtd\">\n"
/** HTML 4.0 Frameset Doctype */
#define DOCTYPE_HTML_4_0F "<!DOCTYPE HTML PUBLIC \"-//W3C//" \
                          "DTD HTML 4.0 Frameset//EN\"\n" \
                          "\"http://www.w3.org/TR/REC-html40/frameset.dtd\">\n"

/** Internal representation for a HTTP protocol number, e.g., HTTP/1.1 */

#define HTTP_VERSION(major,minor) (1000*(major)+(minor))
/** Major part of HTTP protocol */
#define HTTP_VERSION_MAJOR(number) ((number)/1000)
/** Minor part of HTTP protocol */
#define HTTP_VERSION_MINOR(number) ((number)%1000)

/* -------------- Port number for server running standalone --------------- */

/** default HTTP Port */
#define DEFAULT_HTTP_PORT   80
/** default HTTPS Port */
#define DEFAULT_HTTPS_PORT  443
/**
 * Check whether @a port is the default port for the request @a r.
 * @param port The port number
 * @param r The request
 * @see #ap_default_port
 */
#define ap_is_default_port(port,r)  ((port) == sb_run_default_port(r))



#ifndef AP_DECLARE
/**
 * Stuff marked #AP_DECLARE is part of the API, and intended for use
 * by modules. Its purpose is to allow us to add attributes that
 * particular platforms or compilers require to every exported function.
 */
# define AP_DECLARE(type)    type
#endif

#ifndef AP_DECLARE_NONSTD
/**
 * Stuff marked #AP_DECLARE_NONSTD is part of the API, and intended for
 * use by modules.  The difference between #AP_DECLARE and
 * #AP_DECLARE_NONSTD is that the latter is required for any functions
 * which use varargs or are used via indirect function call.  This
 * is to accomodate the two calling conventions in windows dlls.
 */
# define AP_DECLARE_NONSTD(type)    type
#endif

#ifndef AP_DECLARE_DATA
# define AP_DECLARE_DATA
#endif

#ifndef AP_MODULE_DECLARE
# define AP_MODULE_DECLARE(type)    type
#endif

#ifndef AP_MODULE_DECLARE_NONSTD
# define AP_MODULE_DECLARE_NONSTD(type)  type
#endif

#ifndef AP_MODULE_DECLARE_DATA
# define AP_MODULE_DECLARE_DATA
#endif

/**
 * @internal
 * modules should not used functions marked AP_CORE_DECLARE
 */
#ifndef AP_CORE_DECLARE
# define AP_CORE_DECLARE    AP_DECLARE
#endif
/**
 * @internal
 * modules should not used functions marked AP_CORE_DECLARE_NONSTD
 */
#ifndef AP_CORE_DECLARE_NONSTD
# define AP_CORE_DECLARE_NONSTD AP_DECLARE_NONSTD
#endif

/**
 * @defgroup HTTP_Status HTTP Status Codes
 * @{
 */
/**
 * The size of the static array in http_protocol.c for storing
 * all of the potential response status-lines (a sparse table).
 * A future version should dynamically generate the apr_table_t at startup.
 */
#define RESPONSE_CODES 55

#define HTTP_CONTINUE                      100
#define HTTP_SWITCHING_PROTOCOLS           101
#define HTTP_PROCESSING                    102
#define HTTP_OK                            200
#define HTTP_CREATED                       201
#define HTTP_ACCEPTED                      202
#define HTTP_NON_AUTHORITATIVE             203
#define HTTP_NO_CONTENT                    204
#define HTTP_RESET_CONTENT                 205
#define HTTP_PARTIAL_CONTENT               206
#define HTTP_MULTI_STATUS                  207
#define HTTP_MULTIPLE_CHOICES              300
#define HTTP_MOVED_PERMANENTLY             301
#define HTTP_MOVED_TEMPORARILY             302
#define HTTP_SEE_OTHER                     303
#define HTTP_NOT_MODIFIED                  304
#define HTTP_USE_PROXY                     305
#define HTTP_TEMPORARY_REDIRECT            307
#define HTTP_BAD_REQUEST                   400
#define HTTP_UNAUTHORIZED                  401
#define HTTP_PAYMENT_REQUIRED              402
#define HTTP_FORBIDDEN                     403
#define HTTP_NOT_FOUND                     404
#define HTTP_METHOD_NOT_ALLOWED            405
#define HTTP_NOT_ACCEPTABLE                406
#define HTTP_PROXY_AUTHENTICATION_REQUIRED 407
#define HTTP_REQUEST_TIME_OUT              408
#define HTTP_CONFLICT                      409
#define HTTP_GONE                          410
#define HTTP_LENGTH_REQUIRED               411
#define HTTP_PRECONDITION_FAILED           412
#define HTTP_REQUEST_ENTITY_TOO_LARGE      413
#define HTTP_REQUEST_URI_TOO_LARGE         414
#define HTTP_UNSUPPORTED_MEDIA_TYPE        415
#define HTTP_RANGE_NOT_SATISFIABLE         416
#define HTTP_EXPECTATION_FAILED            417
#define HTTP_UNPROCESSABLE_ENTITY          422
#define HTTP_LOCKED                        423
#define HTTP_FAILED_DEPENDENCY             424
#define HTTP_INTERNAL_SERVER_ERROR         500
#define HTTP_NOT_IMPLEMENTED               501
#define HTTP_BAD_GATEWAY                   502
#define HTTP_SERVICE_UNAVAILABLE           503
#define HTTP_GATEWAY_TIME_OUT              504
#define HTTP_VERSION_NOT_SUPPORTED         505
#define HTTP_VARIANT_ALSO_VARIES           506
#define HTTP_INSUFFICIENT_STORAGE          507
#define HTTP_NOT_EXTENDED                  510

/** is the status code informational */
#define is_HTTP_INFO(x)         (((x) >= 100)&&((x) < 200))
/** is the status code OK ?*/
#define is_HTTP_SUCCESS(x)      (((x) >= 200)&&((x) < 300))
/** is the status code a redirect */
#define is_HTTP_REDIRECT(x)     (((x) >= 300)&&((x) < 400))
/** is the status code a error (client or server) */
#define is_HTTP_ERROR(x)        (((x) >= 400)&&((x) < 600))
/** is the status code a client error  */
#define is_HTTP_CLIENT_ERROR(x) (((x) >= 400)&&((x) < 500))
/** is the status code a server error  */
#define is_HTTP_SERVER_ERROR(x) (((x) >= 500)&&((x) < 600))

/** should the status code drop the connection */
#define status_drops_connection(x) \
               (((x) == HTTP_BAD_REQUEST)          || \
               ((x) == HTTP_REQUEST_TIME_OUT)      || \
               ((x) == HTTP_LENGTH_REQUIRED)       || \
               ((x) == HTTP_REQUEST_ENTITY_TOO_LARGE) || \
               ((x) == HTTP_REQUEST_URI_TOO_LARGE) || \
               ((x) == HTTP_INTERNAL_SERVER_ERROR) || \
               ((x) == HTTP_SERVICE_UNAVAILABLE) || \
               ((x) == HTTP_NOT_IMPLEMENTED))
/**
 * @defgroup Methods List of Methods recognized by the server
 */
/**
 * Methods recognized (but not necessarily handled) by the server.
 * These constants are used in bit shifting masks of size int, so it is
 * unsafe to have more methods than bits in an int.  HEAD == M_GET.
 * This list must be tracked by the list in http_protocol.c in routine
 * ap_method_name_of().
 */
#define M_GET                   0       /* RFC 2616: HTTP */
#define M_PUT                   1       /*  :             */
#define M_POST                  2
#define M_DELETE                3
#define M_CONNECT               4
#define M_OPTIONS               5
#define M_TRACE                 6       /* RFC 2616: HTTP */
#define M_PATCH                 7       /* no rfc(!)  ### remove this one? */
#define M_PROPFIND              8       /* RFC 2518: WebDAV */
#define M_PROPPATCH             9       /*  :               */
#define M_MKCOL                 10
#define M_COPY                  11
#define M_MOVE                  12
#define M_LOCK                  13
#define M_UNLOCK                14      /* RFC 2518: WebDAV */
#define M_VERSION_CONTROL       15      /* RFC 3253: WebDAV Versioning */
#define M_CHECKOUT              16      /*  :                          */
#define M_UNCHECKOUT            17
#define M_CHECKIN               18
#define M_UPDATE                19
#define M_LABEL                 20
#define M_REPORT                21
#define M_MKWORKSPACE           22
#define M_MKACTIVITY            23
#define M_BASELINE_CONTROL      24
#define M_MERGE                 25
#define M_INVALID               26      /* RFC 3253: WebDAV Versioning */

/**
 * METHODS needs to be equal to the number of bits
 * we are using for limit masks.
 */
#define METHODS     64

/**
 * The method mask bit to shift for anding with a bitmask.
 */
#define AP_METHOD_BIT ((int64_t)1)
/** @} */


/**
 * Structure for handling HTTP methods.  Methods known to the server are
 * accessed via a bitmask shortcut; extension methods are handled by
 * an array.
 */
typedef struct method_list_t method_list_t;
struct method_list_t {
    /* The bitmask used for known methods */
    int64_t method_mask;
    /* the array used for extension methods */
    apr_array_header_t *method_list;
};

/** linefeed */
#define LF 10
/** carrige return */
#define CR 13
/** carrige return /Line Feed Combo */
#define CRLF "\015\012"

/**
 * @defgroup values_request_rec_body Possible values for request_rec.read_body 
 * @{
 * Possible values for request_rec.read_body (set by handling module):
 */

/** Send 413 error if message has any body */
#define REQUEST_NO_BODY          0
/** Send 411 error if body without Content-Length */
#define REQUEST_CHUNKED_ERROR    1
/** If chunked, remove the chunks for me. */
#define REQUEST_CHUNKED_DECHUNK  2
/** @} */


/**
 * @defgroup values_request_rec_used_path_info Possible values 
 * for request_rec.used_path_info 
 * @{
 * Possible values for request_rec.used_path_info:
 */

/** Accept the path_info from the request */
#define AP_REQ_ACCEPT_PATH_INFO    0
/** Return a 404 error if path_info was given */
#define AP_REQ_REJECT_PATH_INFO    1
/** Module may chose to use the given path_info */
#define AP_REQ_DEFAULT_PATH_INFO   2
/** @} */


/* The following four types define a hierarchy of activities, so that
 * given a request_rec r you can write r->connection->server->process
 * to get to the process_rec.  While this reduces substantially the
 * number of arguments that various hooks require beware that in
 * threaded versions of the server you must consider multiplexing
 * issues.  */

/** A structure that represents one process */
typedef struct process_rec process_rec;
/** A structure that represents a virtual server */
typedef struct server_rec server_rec;
/** A structure that represents one connection */
typedef struct conn_rec conn_rec;
/** A structure that represents the current request */
typedef struct request_rec request_rec;


/** A structure that represents one process */
struct process_rec {
	/** Global pool. Cleared upon normal exit */
	apr_pool_t *pool;
	/** Configuration pool. Cleared upon restart */
	apr_pool_t *pconf;
	/** Number of command line arguments passed to the program */
	int argc;
	/** The command line arguments */
	const char * const *argv;
	/** The program name used to execute the program */
	const char *short_name;
};

/** A structure that represents the current request */
struct request_rec {
    /** The pool associated with the request */
    apr_pool_t *pool;
    /** The connection to the client */
    conn_rec *connection;
    /** The virtual host for this request */
	server_rec *server;

    /** Pointer to the redirected request if this is an external redirect */
    request_rec *next;
    /** Pointer to the previous request if this is an internal redirect */
    request_rec *prev;

    /** Pointer to the main request if this is a sub-request (see request.h) */
    request_rec *main;

    /* Info about the request itself... we begin with stuff that only
     * protocol.c should ever touch...
     */
    /** First line of request */
    char *the_request;
    /** HTTP/0.9, "simple" request (e.g. GET /foo\n w/no headers) */
    int assbackwards;
    /** A proxy request (calculated during post_read_request/translate_name)
     *  possible values PROXYREQ_NONE, PROXYREQ_PROXY, PROXYREQ_REVERSE,
     *                  PROXYREQ_RESPONSE
     */
    int proxyreq;
    /** HEAD request, as opposed to GET */
    int header_only;
    /** Protocol string, as given to us, or HTTP/0.9 */
    char *protocol;
    /** Protocol version number of protocol; 1.1 = 1001 */
    int proto_num;
    /** Host, as set by full URI or Host: */
    const char *hostname;

    /** Time when the request started */
    apr_time_t request_time;

    /** Status line, if set by script */
    const char *status_line;
    /** Status line */
    int status;

    /* Request method, two ways; also, protocol, etc..  Outside of protocol.c,
     * look, but don't touch.
     */

    /** Request method (eg. GET, HEAD, POST, etc.) */
    const char *method;
    /** M_GET, M_POST, etc. */
    int method_number;

    /**
     *  'allowed' is a bitvector of the allowed methods.
     *
     *  A handler must ensure that the request method is one that
     *  it is capable of handling.  Generally modules should DECLINE
     *  any request methods they do not handle.  Prior to aborting the
     *  handler like this the handler should set r->allowed to the list
     *  of methods that it is willing to handle.  This bitvector is used
     *  to construct the "Allow:" header required for OPTIONS requests,
     *  and HTTP_METHOD_NOT_ALLOWED and HTTP_NOT_IMPLEMENTED status codes.
     *
     *  Since the default_handler deals with OPTIONS, all modules can
     *  usually decline to deal with OPTIONS.  TRACE is always allowed,
     *  modules don't need to set it explicitly.
     *
     *  Since the default_handler will always handle a GET, a
     *  module which does *not* implement GET should probably return
     *  HTTP_METHOD_NOT_ALLOWED.  Unfortunately this means that a Script GET
     *  handler can't be installed by mod_actions.
     */
    apr_int64_t allowed;
    /** Array of extension methods */
    apr_array_header_t *allowed_xmethods; 
    /** List of allowed methods */
    method_list_t *allowed_methods; 

    /** byte count in stream is for body */
    apr_off_t sent_bodyct;
    /** body byte count, for easy access */
    apr_off_t bytes_sent;
    /** Last modified time of the requested resource */
    apr_time_t mtime;

    /* HTTP/1.1 connection-level features */

    /** sending chunked transfer-coding */
    int chunked;
    /** The Range: header */
    const char *range;
    /** The "real" content length */
    apr_off_t clength;

    /** Remaining bytes left to read from the request body */
    apr_off_t remaining;
    /** Number of bytes that have been read  from the request body */
    apr_off_t read_length;
    /** Method for reading the request body
     * (eg. REQUEST_CHUNKED_ERROR, REQUEST_NO_BODY,
     *  REQUEST_CHUNKED_DECHUNK, etc...) */
    int read_body;
    /** reading chunked transfer-coding */
    int read_chunked;
    /** is client waiting for a 100 response? */
    unsigned expecting_100;

    /* MIME header environments, in and out.  Also, an array containing
     * environment variables to be passed to subprocesses, so people can
     * write modules to add to that environment.
     *
     * The difference between headers_out and err_headers_out is that the
     * latter are printed even on error, and persist across internal redirects
     * (so the headers printed for ErrorDocument handlers will have them).
     *
     * The 'notes' apr_table_t is for notes from one module to another, with no
     * other set purpose in mind...
     */

    /** MIME header environment from the request */
    apr_table_t *headers_in;
    /** MIME header environment for the response */
    apr_table_t *headers_out;
    /** MIME header environment for the response, printed even on errors and
     * persist across internal redirects */
    apr_table_t *err_headers_out;
    /** Array of environment variables to be used for sub processes */
    apr_table_t *subprocess_env;
    /** Notes from one module to another */
    apr_table_t *notes;

    /* content_type, handler, content_encoding, and all content_languages 
     * MUST be lowercased strings.  They may be pointers to static strings;
     * they should not be modified in place.
     */
    /** The content-type for the current request */
    const char *content_type;	/* Break these out --- we dispatch on 'em */
    /** The handler string that we use to call a handler function */
    const char *handler;	/* What we *really* dispatch on */

    /** How to encode the data */
    const char *content_encoding;
    /** Array of strings representing the content languages */
    apr_array_header_t *content_languages;

    /** variant list validator (if negotiated) */
    char *vlist_validator;
    
    /** If an authentication check was made, this gets set to the user name. */
    char *user;	
    /** If an authentication check was made, this gets set to the auth type. */
    char *auth_type;

    /** This response can not be cached */
    int no_cache;
    /** There is no local copy of this response */
    int no_local_copy;

    /* What object is being requested (either directly, or via include
     * or content-negotiation mapping).
     */

    /** The URI without any parsing performed */
    char *unparsed_uri;	
    /** The path portion of the URI */
    char *uri;
    /** The filename on disk corresponding to this response */
    char *filename;
    /* XXX: What does this mean? Please define "canonicalize" -aaron */
    /** The true filename, we canonicalize r->filename if these don't match */
    char *canonical_filename;
    /** The PATH_INFO extracted from this request */
    char *path_info;
    /** The QUERY_ARGS extracted from this request */
    char *args;	
    /** ST_MODE set to zero if no such file */
    apr_finfo_t finfo;
    /** A struct containing the components of URI */
    apr_uri_t parsed_uri;

    /**
     * Flag for the handler to accept or reject path_info on 
     * the current request.  All modules should respect the
     * AP_REQ_ACCEPT_PATH_INFO and AP_REQ_REJECT_PATH_INFO 
     * values, while AP_REQ_DEFAULT_PATH_INFO indicates they
     * may follow existing conventions.  This is set to the
     * user's preference upon HOOK_VERY_FIRST of the fixups.
     */
    int used_path_info;

    /* Various other config info which may change with .htaccess files
     * These are config vectors, with one void* pointer for each module
     * (the thing pointed to being the module's business).
     */

    /** Options set in config files, etc. */
    struct ap_conf_vector_t *per_dir_config;
    /** Notes on *this* request */
    struct ap_conf_vector_t *request_config;

    /**
     * A linked list of the .htaccess configuration directives
     * accessed by this request.
     * N.B. always add to the head of the list, _never_ to the end.
     * that way, a sub request's list can (temporarily) point to a parent's list
     */
    const struct htaccess_result *htaccess;

    /** A list of output filters to be used for this request */
    struct ap_filter_t *output_filters;
    /** A list of input filters to be used for this request */
    struct ap_filter_t *input_filters;

    /** A list of protocol level output filters to be used for this
     *  request */
    struct ap_filter_t *proto_output_filters;
    /** A list of protocol level input filters to be used for this
     *  request */
    struct ap_filter_t *proto_input_filters;

    /** A flag to determine if the eos bucket has been sent yet */
    int eos_sent;

/* Things placed at the end of the record to avoid breaking binary
 * compatibility.  It would be nice to remember to reorder the entire
 * record to improve 64bit alignment the next time we need to break
 * binary compatibility for some other reason.
 */
};

/**
 * @defgroup ProxyReq Proxy request types
 *
 * Possible values of request_rec->proxyreq. A request could be normal,
 *  proxied or reverse proxied. Normally proxied and reverse proxied are
 *  grouped together as just "proxied", but sometimes it's necessary to
 *  tell the difference between the two, such as for authentication.
 * @{
 */

#define PROXYREQ_NONE 0		/**< No proxy */
#define PROXYREQ_PROXY 1	/**< Standard proxy */
#define PROXYREQ_REVERSE 2	/**< Reverse proxy */
#define PROXYREQ_RESPONSE 3 /**< Origin response */

/* @} */


/** Structure to store things which are per connection */
struct conn_rec {
	/** Pool associated with this connection */
	apr_pool_t *pool;
	/** Physical vhost this conn came in on */
	server_rec *base_server;
	/** used by http_vhost.c */
//	void *vhost_lookup_data;

    /* Information about the connection itself */
    /** local address */
    apr_sockaddr_t *local_addr;
    /** remote address */
    apr_sockaddr_t *remote_addr;

    /** Client's IP address */
    char *remote_ip;
    /** Client's DNS name, if known.  NULL if DNS hasn't been checked,
     *  "" if it has and no address was found.  N.B. Only access this though
     * get_remote_host() */
    char *remote_host;
    /** Only ever set if doing rfc1413 lookups.  N.B. Only access this through
     *  get_remote_logname() */
    char *remote_logname;

    /** Are we still talking? */
    unsigned aborted:1;

    /** Are we going to keep the connection alive for another request?
     *  -1 fatal error, 0 undecided, 1 yes   */
    signed int keepalive:2;

    /** have we done double-reverse DNS? -1 yes/failure, 0 not yet, 
     *  1 yes/success */
    signed int double_reverse:2;

    /** How many times have we used it? */
    int keepalives;
    /** server IP address */
    char *local_ip;
    /** used for ap_get_server_name when UseCanonicalName is set to DNS
     *  (ignores setting of HostnameLookups) */
    char *local_host;

    /** ID of this connection; unique at any point in time */
    long id;
    /** Notes on *this* connection */
    struct ap_conf_vector_t *conn_config;
    /** send note from one module to another, must remain valid for all
     *  requests on this conn */
    apr_table_t *notes;

    /** A list of input filters to be used for this connection */
    struct ap_filter_t *input_filters;
    /** A list of output filters to be used for this connection */
    struct ap_filter_t *output_filters;

    /** handle to scoreboard information for this connection */
    slot_t *slot;
    /** The bucket allocator to use for all bucket/brigade creations */
    struct apr_bucket_alloc_t *bucket_alloc;
};



/* Per-vhost config... */

/**
 * The address 255.255.255.255, when used as a virtualhost address,
 * will become the "default" server when the ip doesn't match other vhosts.
 */
#define DEFAULT_VHOST_ADDR 0xfffffffful


/** A structure to be used for Per-vhost config */
typedef struct server_addr_rec server_addr_rec;
struct server_addr_rec {
    /** The next server in the list */
    server_addr_rec *next;
    /** The bound address, for this server */
    apr_sockaddr_t *host_addr;
    /** The bound port, for this server */
    apr_port_t host_port;
    /** The name given in <VirtualHost> */
    char *virthost;
};

/** A structure to store information for each virtual server */
struct server_rec {
	/** The process this server is running in */
	process_rec *process;
	/** The next server in the list */
	server_rec *next;

	/** The name of the server */
	const char *defn_name;
	/** The line of the config file that the server was defined on */
	unsigned defn_line_number;

	/* Contact information */

	/** The admin's contact information */
	char *server_admin;
	/** The server hostname */
	char *server_hostname;
	/** for redirects, etc. */
	apr_port_t port;

	/* Log files --- note that transfer log is now in the modules... */

	/** The name of the error log */
	char *error_fname;
	/** A file descriptor that references the error log */
	apr_file_t *error_log;
	/** The log level for this server */
	int loglevel;

	/* Module-specific configuration for server, and defaults... */

	/** true if this is the virtual server */
	int is_virtual;
	/** Config vector containing pointers to modules' per-server config 
	 *  structures. */
	struct ap_conf_vector_t *module_config; 
	/** MIME type info, etc., before we start checking per-directory info */
	struct ap_conf_vector_t *lookup_defaults;

	/* Transaction handling */

	/** I haven't got a clue */
	server_addr_rec *addrs;
	/** Timeout, as an apr interval, before we give up */
	apr_interval_time_t timeout;
	/** The apr interval we will wait for another request */
	apr_interval_time_t keep_alive_timeout;
	/** Maximum requests per connection */
	int keep_alive_max;
	/** Use persistent connections? */
	int keep_alive;

	/** Pathname for ServerPath */
	const char *path;
	/** Length of path */
	int pathlen;

	/** Normal names for ServerAlias servers */
	apr_array_header_t *names;
	/** Wildcarded names for ServerAlias servers */
	apr_array_header_t *wild_names;

	/** limit on size of the HTTP request line    */
	int limit_req_line;
	/** limit on size of any request header field */
	int limit_req_fieldsize;
	/** limit on number of request header fields  */
	int limit_req_fields;
};


typedef struct core_output_filter_ctx {
    apr_bucket_brigade *b;
    apr_pool_t *subpool; /* subpool of c->pool used for data saved after a
                          * request is finished
                          */
    int subpool_has_stuff; /* anything in the subpool? */
} core_output_filter_ctx_t;
 
typedef struct core_filter_ctx {
    apr_bucket_brigade *b;
} core_ctx_t;
 
typedef struct core_net_rec {
    /** Connection to the client */
    apr_socket_t *client_socket;

    /** connection record */
    conn_rec *c;
 
    core_output_filter_ctx_t *out_ctx;
    core_ctx_t *in_ctx;
} core_net_rec;


int graceful_stop_signalled(void);
int update_slot_state(slot_t *slot, int state, request_rec *r);
char *ap_response_code_string(request_rec *r, int error_index);
const char *ap_psignature(const char *prefix, request_rec *r);

#endif /* !MOD_HTTPD_H */
