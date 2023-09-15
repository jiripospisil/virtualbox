/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/

#include "curl_setup.h"

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_IPHLPAPI_H
#include <Iphlpapi.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef __VMS
#include <in.h>
#include <inet.h>
#endif

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifndef HAVE_SOCKET
#error "We can't compile without socket() support!"
#endif

#include <limits.h>

#include "doh.h"
#include "urldata.h"
#include "netrc.h"
#include "formdata.h"
#include "mime.h"
#include "vtls/vtls.h"
#include "hostip.h"
#include "transfer.h"
#include "sendf.h"
#include "progress.h"
#include "cookie.h"
#include "strcase.h"
#include "strerror.h"
#include "escape.h"
#include "strtok.h"
#include "share.h"
#include "content_encoding.h"
#include "http_digest.h"
#include "http_negotiate.h"
#include "select.h"
#include "multiif.h"
#include "easyif.h"
#include "speedcheck.h"
#include "warnless.h"
#include "getinfo.h"
#include "urlapi-int.h"
#include "system_win32.h"
#include "hsts.h"
#include "noproxy.h"
#include "cfilters.h"
#include "idn.h"

/* And now for the protocols */
#include "ftp.h"
#include "dict.h"
#include "telnet.h"
#include "tftp.h"
#include "http.h"
#include "http2.h"
#include "file.h"
#include "curl_ldap.h"
#include "vssh/ssh.h"
#include "imap.h"
#include "url.h"
#include "connect.h"
#include "inet_ntop.h"
#include "http_ntlm.h"
#include "curl_rtmp.h"
#include "gopher.h"
#include "mqtt.h"
#include "http_proxy.h"
#include "conncache.h"
#include "multihandle.h"
#include "strdup.h"
#include "setopt.h"
#include "altsvc.h"
#include "dynbuf.h"
#include "headers.h"

/* The last 3 #include files should be in this order */
#include "curl_printf.h"
#include "curl_memory.h"
#include "memdebug.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(A) (sizeof(A)/sizeof((A)[0]))
#endif

#ifdef USE_NGHTTP2
static void data_priority_cleanup(struct Curl_easy *data);
#else
#define data_priority_cleanup(x)
#endif

/* Some parts of the code (e.g. chunked encoding) assume this buffer has at
 * more than just a few bytes to play with. Don't let it become too small or
 * bad things will happen.
 */
#if READBUFFER_SIZE < READBUFFER_MIN
# error READBUFFER_SIZE is too small
#endif

#ifdef USE_UNIX_SOCKETS
#define UNIX_SOCKET_PREFIX "localhost"
#endif

/* Reject URLs exceeding this length */
#define MAX_URL_LEN 0xffff

/*
* get_protocol_family()
*
* This is used to return the protocol family for a given protocol.
*
* Parameters:
*
* 'h'  [in]  - struct Curl_handler pointer.
*
* Returns the family as a single bit protocol identifier.
*/
static curl_prot_t get_protocol_family(const struct Curl_handler *h)
{
  DEBUGASSERT(h);
  DEBUGASSERT(h->family);
  return h->family;
}


/*
 * Protocol table. Schemes (roughly) in 2019 popularity order:
 *
 * HTTPS, HTTP, FTP, FTPS, SFTP, FILE, SCP, SMTP, LDAP, IMAPS, TELNET, IMAP,
 * LDAPS, SMTPS, TFTP, SMB, POP3, GOPHER POP3S, RTSP, RTMP, SMBS, DICT
 */
static const struct Curl_handler * const protocols[] = {

#if defined(USE_SSL) && !defined(CURL_DISABLE_HTTP)
  &Curl_handler_https,
#endif

#ifndef CURL_DISABLE_HTTP
  &Curl_handler_http,
#endif

#ifdef USE_WEBSOCKETS
#if defined(USE_SSL) && !defined(CURL_DISABLE_HTTP)
  &Curl_handler_wss,
#endif

#ifndef CURL_DISABLE_HTTP
  &Curl_handler_ws,
#endif
#endif

#ifndef CURL_DISABLE_FTP
  &Curl_handler_ftp,
#endif

#if defined(USE_SSL) && !defined(CURL_DISABLE_FTP)
  &Curl_handler_ftps,
#endif

#if defined(USE_SSH)
  &Curl_handler_sftp,
#endif

#ifndef CURL_DISABLE_FILE
  &Curl_handler_file,
#endif

#if defined(USE_SSH) && !defined(USE_WOLFSSH)
  &Curl_handler_scp,
#endif

#ifndef CURL_DISABLE_SMTP
  &Curl_handler_smtp,
#ifdef USE_SSL
  &Curl_handler_smtps,
#endif
#endif

#ifndef CURL_DISABLE_LDAP
  &Curl_handler_ldap,
#if !defined(CURL_DISABLE_LDAPS) && \
    ((defined(USE_OPENLDAP) && defined(USE_SSL)) || \
     (!defined(USE_OPENLDAP) && defined(HAVE_LDAP_SSL)))
  &Curl_handler_ldaps,
#endif
#endif

#ifndef CURL_DISABLE_IMAP
  &Curl_handler_imap,
#ifdef USE_SSL
  &Curl_handler_imaps,
#endif
#endif

#ifndef CURL_DISABLE_TELNET
  &Curl_handler_telnet,
#endif

#ifndef CURL_DISABLE_TFTP
  &Curl_handler_tftp,
#endif

#ifndef CURL_DISABLE_POP3
  &Curl_handler_pop3,
#ifdef USE_SSL
  &Curl_handler_pop3s,
#endif
#endif

#if !defined(CURL_DISABLE_SMB) && defined(USE_CURL_NTLM_CORE) && \
   (SIZEOF_CURL_OFF_T > 4)
  &Curl_handler_smb,
#ifdef USE_SSL
  &Curl_handler_smbs,
#endif
#endif

#ifndef CURL_DISABLE_RTSP
  &Curl_handler_rtsp,
#endif

#ifndef CURL_DISABLE_MQTT
  &Curl_handler_mqtt,
#endif

#ifndef CURL_DISABLE_GOPHER
  &Curl_handler_gopher,
#ifdef USE_SSL
  &Curl_handler_gophers,
#endif
#endif

#ifdef USE_LIBRTMP
  &Curl_handler_rtmp,
  &Curl_handler_rtmpt,
  &Curl_handler_rtmpe,
  &Curl_handler_rtmpte,
  &Curl_handler_rtmps,
  &Curl_handler_rtmpts,
#endif

#ifndef CURL_DISABLE_DICT
  &Curl_handler_dict,
#endif

  (struct Curl_handler *) NULL
};

void Curl_freeset(struct Curl_easy *data)
{
  /* Free all dynamic strings stored in the data->set substructure. */
  enum dupstring i;
  enum dupblob j;

  for(i = (enum dupstring)0; i < STRING_LAST; i++) {
    Curl_safefree(data->set.str[i]);
  }

  for(j = (enum dupblob)0; j < BLOB_LAST; j++) {
    Curl_safefree(data->set.blobs[j]);
  }

  if(data->state.referer_alloc) {
    Curl_safefree(data->state.referer);
    data->state.referer_alloc = FALSE;
  }
  data->state.referer = NULL;
  if(data->state.url_alloc) {
    Curl_safefree(data->state.url);
    data->state.url_alloc = FALSE;
  }
  data->state.url = NULL;

  Curl_mime_cleanpart(&data->set.mimepost);

#ifndef CURL_DISABLE_COOKIES
  curl_slist_free_all(data->set.cookielist);
  data->set.cookielist = NULL;
#endif
}

/* free the URL pieces */
static void up_free(struct Curl_easy *data)
{
  struct urlpieces *up = &data->state.up;
  Curl_safefree(up->scheme);
  Curl_safefree(up->hostname);
  Curl_safefree(up->port);
  Curl_safefree(up->user);
  Curl_safefree(up->password);
  Curl_safefree(up->options);
  Curl_safefree(up->path);
  Curl_safefree(up->query);
  curl_url_cleanup(data->state.uh);
  data->state.uh = NULL;
}

/*
 * This is the internal function curl_easy_cleanup() calls. This should
 * cleanup and free all resources associated with this sessionhandle.
 *
 * We ignore SIGPIPE when this is called from curl_easy_cleanup.
 */

CURLcode Curl_close(struct Curl_easy **datap)
{
  struct Curl_easy *data;

  if(!datap || !*datap)
    return CURLE_OK;

  data = *datap;
  *datap = NULL;

  Curl_expire_clear(data); /* shut off timers */

  /* Detach connection if any is left. This should not be normal, but can be
     the case for example with CONNECT_ONLY + recv/send (test 556) */
  Curl_detach_connection(data);
  if(data->multi)
    /* This handle is still part of a multi handle, take care of this first
       and detach this handle from there. */
    curl_multi_remove_handle(data->multi, data);

  if(data->multi_easy) {
    /* when curl_easy_perform() is used, it creates its own multi handle to
       use and this is the one */
    curl_multi_cleanup(data->multi_easy);
    data->multi_easy = NULL;
  }

  data->magic = 0; /* force a clear AFTER the possibly enforced removal from
                      the multi handle, since that function uses the magic
                      field! */

  if(data->state.rangestringalloc)
    free(data->state.range);

  /* freed here just in case DONE wasn't called */
  Curl_free_request_state(data);

  /* Close down all open SSL info and sessions */
  Curl_ssl_close_all(data);
  Curl_safefree(data->state.first_host);
  Curl_safefree(data->state.scratch);
  Curl_ssl_free_certinfo(data);

  /* Cleanup possible redirect junk */
  free(data->req.newurl);
  data->req.newurl = NULL;

  if(data->state.referer_alloc) {
    Curl_safefree(data->state.referer);
    data->state.referer_alloc = FALSE;
  }
  data->state.referer = NULL;

  up_free(data);
  Curl_safefree(data->state.buffer);
  Curl_dyn_free(&data->state.headerb);
  Curl_safefree(data->state.ulbuf);
  Curl_flush_cookies(data, TRUE);
  Curl_altsvc_save(data, data->asi, data->set.str[STRING_ALTSVC]);
  Curl_altsvc_cleanup(&data->asi);
  Curl_hsts_save(data, data->hsts, data->set.str[STRING_HSTS]);
#ifndef CURL_DISABLE_HSTS
  if(!data->share || !data->share->hsts)
    Curl_hsts_cleanup(&data->hsts);
  curl_slist_free_all(data->set.hstslist); /* clean up list */
#endif
#if !defined(CURL_DISABLE_HTTP) && !defined(CURL_DISABLE_DIGEST_AUTH)
  Curl_http_auth_cleanup_digest(data);
#endif
  Curl_safefree(data->info.contenttype);
  Curl_safefree(data->info.wouldredirect);

  /* this destroys the channel and we cannot use it anymore after this */
  Curl_resolver_cancel(data);
  Curl_resolver_cleanup(data->state.async.resolver);

  data_priority_cleanup(data);

  /* No longer a dirty share, if it exists */
  if(data->share) {
    Curl_share_lock(data, CURL_LOCK_DATA_SHARE, CURL_LOCK_ACCESS_SINGLE);
    data->share->dirty--;
    Curl_share_unlock(data, CURL_LOCK_DATA_SHARE);
  }

  Curl_safefree(data->state.aptr.proxyuserpwd);
  Curl_safefree(data->state.aptr.uagent);
  Curl_safefree(data->state.aptr.userpwd);
  Curl_safefree(data->state.aptr.accept_encoding);
  Curl_safefree(data->state.aptr.te);
  Curl_safefree(data->state.aptr.rangeline);
  Curl_safefree(data->state.aptr.ref);
  Curl_safefree(data->state.aptr.host);
  Curl_safefree(data->state.aptr.cookiehost);
  Curl_safefree(data->state.aptr.rtsp_transport);
  Curl_safefree(data->state.aptr.user);
  Curl_safefree(data->state.aptr.passwd);
  Curl_safefree(data->state.aptr.proxyuser);
  Curl_safefree(data->state.aptr.proxypasswd);

#ifndef CURL_DISABLE_DOH
  if(data->req.doh) {
    Curl_dyn_free(&data->req.doh->probe[0].serverdoh);
    Curl_dyn_free(&data->req.doh->probe[1].serverdoh);
    curl_slist_free_all(data->req.doh->headers);
    Curl_safefree(data->req.doh);
  }
#endif

  Curl_mime_cleanpart(data->state.formp);
#ifndef CURL_DISABLE_HTTP
  Curl_safefree(data->state.formp);
#endif

  /* destruct wildcard structures if it is needed */
  Curl_wildcard_dtor(&data->wildcard);
  Curl_freeset(data);
  Curl_headers_cleanup(data);
  free(data);
  return CURLE_OK;
}

/*
 * Initialize the UserDefined fields within a Curl_easy.
 * This may be safely called on a new or existing Curl_easy.
 */
CURLcode Curl_init_userdefined(struct Curl_easy *data)
{
  struct UserDefined *set = &data->set;
  CURLcode result = CURLE_OK;

  set->out = stdout; /* default output to stdout */
  set->in_set = stdin;  /* default input from stdin */
  set->err  = stderr;  /* default stderr to stderr */

  /* use fwrite as default function to store output */
  set->fwrite_func = (curl_write_callback)fwrite;

  /* use fread as default function to read input */
  set->fread_func_set = (curl_read_callback)fread;
  set->is_fread_set = 0;

  set->seek_func = ZERO_NULL;
  set->seek_client = ZERO_NULL;

  set->filesize = -1;        /* we don't know the size */
  set->postfieldsize = -1;   /* unknown size */
  set->maxredirs = 30;       /* sensible default */

  set->method = HTTPREQ_GET; /* Default HTTP request */
#ifndef CURL_DISABLE_RTSP
  set->rtspreq = RTSPREQ_OPTIONS; /* Default RTSP request */
#endif
#ifndef CURL_DISABLE_FTP
  set->ftp_use_epsv = TRUE;   /* FTP defaults to EPSV operations */
  set->ftp_use_eprt = TRUE;   /* FTP defaults to EPRT operations */
  set->ftp_use_pret = FALSE;  /* mainly useful for drftpd servers */
  set->ftp_filemethod = FTPFILE_MULTICWD;
  set->ftp_skip_ip = TRUE;    /* skip PASV IP by default */
#endif
  set->dns_cache_timeout = 60; /* Timeout every 60 seconds by default */

  /* Set the default size of the SSL session ID cache */
  set->general_ssl.max_ssl_sessions = 5;
  /* Timeout every 24 hours by default */
  set->general_ssl.ca_cache_timeout = 24 * 60 * 60;

  set->httpauth = CURLAUTH_BASIC;  /* defaults to basic */

#ifndef CURL_DISABLE_PROXY
  set->proxyport = 0;
  set->proxytype = CURLPROXY_HTTP; /* defaults to HTTP proxy */
  set->proxyauth = CURLAUTH_BASIC; /* defaults to basic */
  /* SOCKS5 proxy auth defaults to username/password + GSS-API */
  set->socks5auth = CURLAUTH_BASIC | CURLAUTH_GSSAPI;
#endif

  /* make libcurl quiet by default: */
  set->hide_progress = TRUE;  /* CURLOPT_NOPROGRESS changes these */

  Curl_mime_initpart(&set->mimepost);

  /*
   * libcurl 7.10 introduced SSL verification *by default*! This needs to be
   * switched off unless wanted.
   */
#ifndef CURL_DISABLE_DOH
  set->doh_verifyhost = TRUE;
  set->doh_verifypeer = TRUE;
#endif
  set->ssl.primary.verifypeer = TRUE;
  set->ssl.primary.verifyhost = TRUE;
#ifdef USE_SSH
  /* defaults to any auth type */
  set->ssh_auth_types = CURLSSH_AUTH_DEFAULT;
  set->new_directory_perms = 0755; /* Default permissions */
#endif
  set->ssl.primary.sessionid = TRUE; /* session ID caching enabled by
                                        default */
#ifndef CURL_DISABLE_PROXY
  set->proxy_ssl = set->ssl;
#endif

  set->new_file_perms = 0644;    /* Default permissions */
  set->allowed_protocols = (curl_prot_t) CURLPROTO_ALL;
  set->redir_protocols = CURLPROTO_HTTP | CURLPROTO_HTTPS | CURLPROTO_FTP |
                         CURLPROTO_FTPS;

#if defined(HAVE_GSSAPI) || defined(USE_WINDOWS_SSPI)
  /*
   * disallow unprotected protection negotiation NEC reference implementation
   * seem not to follow rfc1961 section 4.3/4.4
   */
  set->socks5_gssapi_nec = FALSE;
#endif

  /* Set the default CA cert bundle/path detected/specified at build time.
   *
   * If Schannel is the selected SSL backend then these locations are
   * ignored. We allow setting CA location for schannel only when explicitly
   * specified by the user via CURLOPT_CAINFO / --cacert.
   */
  if(Curl_ssl_backend() != CURLSSLBACKEND_SCHANNEL) {
#if defined(CURL_CA_BUNDLE)
    result = Curl_setstropt(&set->str[STRING_SSL_CAFILE], CURL_CA_BUNDLE);
    if(result)
      return result;

    result = Curl_setstropt(&set->str[STRING_SSL_CAFILE_PROXY],
                            CURL_CA_BUNDLE);
    if(result)
      return result;
#endif
#if defined(CURL_CA_PATH)
    result = Curl_setstropt(&set->str[STRING_SSL_CAPATH], CURL_CA_PATH);
    if(result)
      return result;

    result = Curl_setstropt(&set->str[STRING_SSL_CAPATH_PROXY], CURL_CA_PATH);
    if(result)
      return result;
#endif
  }

#ifndef CURL_DISABLE_FTP
  set->wildcard_enabled = FALSE;
  set->chunk_bgn      = ZERO_NULL;
  set->chunk_end      = ZERO_NULL;
  set->fnmatch = ZERO_NULL;
#endif
  set->tcp_keepalive = FALSE;
  set->tcp_keepintvl = 60;
  set->tcp_keepidle = 60;
  set->tcp_fastopen = FALSE;
  set->tcp_nodelay = TRUE;
  set->ssl_enable_alpn = TRUE;
  set->expect_100_timeout = 1000L; /* Wait for a second by default. */
  set->sep_headers = TRUE; /* separated header lists by default */
  set->buffer_size = READBUFFER_SIZE;
  set->upload_buffer_size = UPLOADBUFFER_DEFAULT;
  set->happy_eyeballs_timeout = CURL_HET_DEFAULT;
  set->upkeep_interval_ms = CURL_UPKEEP_INTERVAL_DEFAULT;
  set->maxconnects = DEFAULT_CONNCACHE_SIZE; /* for easy handles */
  set->maxage_conn = 118;
  set->maxlifetime_conn = 0;
  set->http09_allowed = FALSE;
#ifdef USE_HTTP2
  set->httpwant = CURL_HTTP_VERSION_2TLS
#else
  set->httpwant = CURL_HTTP_VERSION_1_1
#endif
    ;
#if defined(USE_HTTP2) || defined(USE_HTTP3)
  memset(&set->priority, 0, sizeof(set->priority));
#endif
  set->quick_exit = 0L;
  return result;
}

/**
 * Curl_open()
 *
 * @param curl is a pointer to a sessionhandle pointer that gets set by this
 * function.
 * @return CURLcode
 */

CURLcode Curl_open(struct Curl_easy **curl)
{
  CURLcode result;
  struct Curl_easy *data;

  /* Very simple start-up: alloc the struct, init it with zeroes and return */
  data = calloc(1, sizeof(struct Curl_easy));
  if(!data) {
    /* this is a very serious error */
    DEBUGF(fprintf(stderr, "Error: calloc of Curl_easy failed\n"));
    return CURLE_OUT_OF_MEMORY;
  }

  data->magic = CURLEASY_MAGIC_NUMBER;

  result = Curl_resolver_init(data, &data->state.async.resolver);
  if(result) {
    DEBUGF(fprintf(stderr, "Error: resolver_init failed\n"));
    free(data);
    return result;
  }

  result = Curl_init_userdefined(data);
  if(!result) {
    Curl_dyn_init(&data->state.headerb, CURL_MAX_HTTP_HEADER);
    Curl_initinfo(data);

    /* most recent connection is not yet defined */
    data->state.lastconnect_id = -1;
    data->state.recent_conn_id = -1;
    /* and not assigned an id yet */
    data->id = -1;

    data->progress.flags |= PGRS_HIDE;
    data->state.current_speed = -1; /* init to negative == impossible */
  }

  if(result) {
    Curl_resolver_cleanup(data->state.async.resolver);
    Curl_dyn_free(&data->state.headerb);
    Curl_freeset(data);
    free(data);
    data = NULL;
  }
  else
    *curl = data;

  return result;
}

static void conn_shutdown(struct Curl_easy *data)
{
  DEBUGASSERT(data);
  infof(data, "Closing connection");

  /* possible left-overs from the async name resolvers */
  Curl_resolver_cancel(data);

  Curl_conn_close(data, SECONDARYSOCKET);
  Curl_conn_close(data, FIRSTSOCKET);
}

static void conn_free(struct Curl_easy *data, struct connectdata *conn)
{
  size_t i;

  DEBUGASSERT(conn);

  for(i = 0; i < ARRAYSIZE(conn->cfilter); ++i) {
    Curl_conn_cf_discard_all(data, conn, (int)i);
  }

  Curl_free_idnconverted_hostname(&conn->host);
  Curl_free_idnconverted_hostname(&conn->conn_to_host);
#ifndef CURL_DISABLE_PROXY
  Curl_free_idnconverted_hostname(&conn->http_proxy.host);
  Curl_free_idnconverted_hostname(&conn->socks_proxy.host);
  Curl_safefree(conn->http_proxy.user);
  Curl_safefree(conn->socks_proxy.user);
  Curl_safefree(conn->http_proxy.passwd);
  Curl_safefree(conn->socks_proxy.passwd);
  Curl_safefree(conn->http_proxy.host.rawalloc); /* http proxy name buffer */
  Curl_safefree(conn->socks_proxy.host.rawalloc); /* socks proxy name buffer */
  Curl_free_primary_ssl_config(&conn->proxy_ssl_config);
#endif
  Curl_safefree(conn->user);
  Curl_safefree(conn->passwd);
  Curl_safefree(conn->sasl_authzid);
  Curl_safefree(conn->options);
  Curl_safefree(conn->oauth_bearer);
#ifndef CURL_DISABLE_HTTP
  Curl_dyn_free(&conn->trailer);
#endif
  Curl_safefree(conn->host.rawalloc); /* host name buffer */
  Curl_safefree(conn->conn_to_host.rawalloc); /* host name buffer */
  Curl_safefree(conn->hostname_resolve);
  Curl_safefree(conn->secondaryhostname);
  Curl_safefree(conn->localdev);
  Curl_free_primary_ssl_config(&conn->ssl_config);

#ifdef USE_UNIX_SOCKETS
  Curl_safefree(conn->unix_domain_socket);
#endif

  free(conn); /* free all the connection oriented data */
}

/*
 * Disconnects the given connection. Note the connection may not be the
 * primary connection, like when freeing room in the connection cache or
 * killing of a dead old connection.
 *
 * A connection needs an easy handle when closing down. We support this passed
 * in separately since the connection to get closed here is often already
 * disassociated from an easy handle.
 *
 * This function MUST NOT reset state in the Curl_easy struct if that
 * isn't strictly bound to the life-time of *this* particular connection.
 *
 */

void Curl_disconnect(struct Curl_easy *data,
                     struct connectdata *conn, bool dead_connection)
{
  /* there must be a connection to close */
  DEBUGASSERT(conn);

  /* it must be removed from the connection cache */
  DEBUGASSERT(!conn->bundle);

  /* there must be an associated transfer */
  DEBUGASSERT(data);

  /* the transfer must be detached from the connection */
  DEBUGASSERT(!data->conn);

  DEBUGF(infof(data, "Curl_disconnect(conn #%"
         CURL_FORMAT_CURL_OFF_T ", dead=%d)",
         conn->connection_id, dead_connection));
  /*
   * If this connection isn't marked to force-close, leave it open if there
   * are other users of it
   */
  if(CONN_INUSE(conn) && !dead_connection) {
    DEBUGF(infof(data, "Curl_disconnect when inuse: %zu", CONN_INUSE(conn)));
    return;
  }

  if(conn->dns_entry) {
    Curl_resolv_unlock(data, conn->dns_entry);
    conn->dns_entry = NULL;
  }

  /* Cleanup NTLM connection-related data */
  Curl_http_auth_cleanup_ntlm(conn);

  /* Cleanup NEGOTIATE connection-related data */
  Curl_http_auth_cleanup_negotiate(conn);

  if(conn->connect_only)
    /* treat the connection as dead in CONNECT_ONLY situations */
    dead_connection = TRUE;

  /* temporarily attach the connection to this transfer handle for the
     disconnect and shutdown */
  Curl_attach_connection(data, conn);

  if(conn->handler && conn->handler->disconnect)
    /* This is set if protocol-specific cleanups should be made */
    conn->handler->disconnect(data, conn, dead_connection);

  conn_shutdown(data);

  /* detach it again */
  Curl_detach_connection(data);

  conn_free(data, conn);
}

/*
 * IsMultiplexingPossible()
 *
 * Return a bitmask with the available multiplexing options for the given
 * requested connection.
 */
static int IsMultiplexingPossible(const struct Curl_easy *handle,
                                  const struct connectdata *conn)
{
  int avail = 0;

  /* If an HTTP protocol and multiplexing is enabled */
  if((conn->handler->protocol & PROTO_FAMILY_HTTP) &&
     (!conn->bits.protoconnstart || !conn->bits.close)) {

    if(Curl_multiplex_wanted(handle->multi) &&
       (handle->state.httpwant >= CURL_HTTP_VERSION_2))
      /* allows HTTP/2 */
      avail |= CURLPIPE_MULTIPLEX;
  }
  return avail;
}

#ifndef CURL_DISABLE_PROXY
static bool
proxy_info_matches(const struct proxy_info *data,
                   const struct proxy_info *needle)
{
  if((data->proxytype == needle->proxytype) &&
     (data->port == needle->port) &&
     strcasecompare(data->host.name, needle->host.name))
    return TRUE;

  return FALSE;
}

static bool
socks_proxy_info_matches(const struct proxy_info *data,
                         const struct proxy_info *needle)
{
  if(!proxy_info_matches(data, needle))
    return FALSE;

  /* the user information is case-sensitive
     or at least it is not defined as case-insensitive
     see https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1 */

  /* curl_strequal does a case insensitive comparison,
     so do not use it here! */
  if(Curl_timestrcmp(data->user, needle->user) ||
     Curl_timestrcmp(data->passwd, needle->passwd))
    return FALSE;
  return TRUE;
}
#else
/* disabled, won't get called */
#define proxy_info_matches(x,y) FALSE
#define socks_proxy_info_matches(x,y) FALSE
#endif

/* A connection has to have been idle for a shorter time than 'maxage_conn'
   (the success rate is just too low after this), or created less than
   'maxlifetime_conn' ago, to be subject for reuse. */

static bool conn_maxage(struct Curl_easy *data,
                        struct connectdata *conn,
                        struct curltime now)
{
  timediff_t idletime, lifetime;

  idletime = Curl_timediff(now, conn->lastused);
  idletime /= 1000; /* integer seconds is fine */

  if(idletime > data->set.maxage_conn) {
    infof(data, "Too old connection (%ld seconds idle), disconnect it",
          idletime);
    return TRUE;
  }

  lifetime = Curl_timediff(now, conn->created);
  lifetime /= 1000; /* integer seconds is fine */

  if(data->set.maxlifetime_conn && lifetime > data->set.maxlifetime_conn) {
    infof(data,
          "Too old connection (%ld seconds since creation), disconnect it",
          lifetime);
    return TRUE;
  }


  return FALSE;
}

/*
 * This function checks if the given connection is dead and extracts it from
 * the connection cache if so.
 *
 * When this is called as a Curl_conncache_foreach() callback, the connection
 * cache lock is held!
 *
 * Returns TRUE if the connection was dead and extracted.
 */
static bool extract_if_dead(struct connectdata *conn,
                            struct Curl_easy *data)
{
  if(!CONN_INUSE(conn)) {
    /* The check for a dead socket makes sense only if the connection isn't in
       use */
    bool dead;
    struct curltime now = Curl_now();
    if(conn_maxage(data, conn, now)) {
      /* avoid check if already too old */
      dead = TRUE;
    }
    else if(conn->handler->connection_check) {
      /* The protocol has a special method for checking the state of the
         connection. Use it to check if the connection is dead. */
      unsigned int state;

      /* briefly attach the connection to this transfer for the purpose of
         checking it */
      Curl_attach_connection(data, conn);

      state = conn->handler->connection_check(data, conn, CONNCHECK_ISDEAD);
      dead = (state & CONNRESULT_DEAD);
      /* detach the connection again */
      Curl_detach_connection(data);

    }
    else {
      bool input_pending;

      Curl_attach_connection(data, conn);
      dead = !Curl_conn_is_alive(data, conn, &input_pending);
      if(input_pending) {
        /* For reuse, we want a "clean" connection state. The includes
         * that we expect - in general - no waiting input data. Input
         * waiting might be a TLS Notify Close, for example. We reject
         * that.
         * For protocols where data from other end may arrive at
         * any time (HTTP/2 PING for example), the protocol handler needs
         * to install its own `connection_check` callback.
         */
        dead = TRUE;
      }
      Curl_detach_connection(data);
    }

    if(dead) {
      infof(data, "Connection %" CURL_FORMAT_CURL_OFF_T " seems to be dead",
            conn->connection_id);
      Curl_conncache_remove_conn(data, conn, FALSE);
      return TRUE;
    }
  }
  return FALSE;
}

struct prunedead {
  struct Curl_easy *data;
  struct connectdata *extracted;
};

/*
 * Wrapper to use extract_if_dead() function in Curl_conncache_foreach()
 *
 */
static int call_extract_if_dead(struct Curl_easy *data,
                                struct connectdata *conn, void *param)
{
  struct prunedead *p = (struct prunedead *)param;
  if(extract_if_dead(conn, data)) {
    /* stop the iteration here, pass back the connection that was extracted */
    p->extracted = conn;
    return 1;
  }
  return 0; /* continue iteration */
}

/*
 * This function scans the connection cache for half-open/dead connections,
 * closes and removes them. The cleanup is done at most once per second.
 *
 * When called, this transfer has no connection attached.
 */
static void prune_dead_connections(struct Curl_easy *data)
{
  struct curltime now = Curl_now();
  timediff_t elapsed;

  DEBUGASSERT(!data->conn); /* no connection */
  CONNCACHE_LOCK(data);
  elapsed =
    Curl_timediff(now, data->state.conn_cache->last_cleanup);
  CONNCACHE_UNLOCK(data);

  if(elapsed >= 1000L) {
    struct prunedead prune;
    prune.data = data;
    prune.extracted = NULL;
    while(Curl_conncache_foreach(data, data->state.conn_cache, &prune,
                                 call_extract_if_dead)) {
      /* unlocked */

      /* remove connection from cache */
      Curl_conncache_remove_conn(data, prune.extracted, TRUE);

      /* disconnect it */
      Curl_disconnect(data, prune.extracted, TRUE);
    }
    CONNCACHE_LOCK(data);
    data->state.conn_cache->last_cleanup = now;
    CONNCACHE_UNLOCK(data);
  }
}

#ifdef USE_SSH
static bool ssh_config_matches(struct connectdata *one,
                               struct connectdata *two)
{
  return (Curl_safecmp(one->proto.sshc.rsa, two->proto.sshc.rsa) &&
          Curl_safecmp(one->proto.sshc.rsa_pub, two->proto.sshc.rsa_pub));
}
#else
#define ssh_config_matches(x,y) FALSE
#endif

/*
 * Given one filled in connection struct (named needle), this function should
 * detect if there already is one that has all the significant details
 * exactly the same and thus should be used instead.
 *
 * If there is a match, this function returns TRUE - and has marked the
 * connection as 'in-use'. It must later be called with ConnectionDone() to
 * return back to 'idle' (unused) state.
 *
 * The force_reuse flag is set if the connection must be used.
 */
static bool
ConnectionExists(struct Curl_easy *data,
                 struct connectdata *needle,
                 struct connectdata **usethis,
                 bool *force_reuse,
                 bool *waitpipe)
{
  struct connectdata *check;
  struct connectdata *chosen = 0;
  bool foundPendingCandidate = FALSE;
  bool canmultiplex = IsMultiplexingPossible(data, needle);
  struct connectbundle *bundle;

#ifdef USE_NTLM
  bool wantNTLMhttp = ((data->state.authhost.want &
                        (CURLAUTH_NTLM | CURLAUTH_NTLM_WB)) &&
                       (needle->handler->protocol & PROTO_FAMILY_HTTP));
#ifndef CURL_DISABLE_PROXY
  bool wantProxyNTLMhttp = (needle->bits.proxy_user_passwd &&
                            ((data->state.authproxy.want &
                              (CURLAUTH_NTLM | CURLAUTH_NTLM_WB)) &&
                             (needle->handler->protocol & PROTO_FAMILY_HTTP)));
#else
  bool wantProxyNTLMhttp = FALSE;
#endif
#endif
  /* plain HTTP with upgrade */
  bool h2upgrade = (data->state.httpwant == CURL_HTTP_VERSION_2_0) &&
    (needle->handler->protocol & CURLPROTO_HTTP);

  *force_reuse = FALSE;
  *waitpipe = FALSE;

  /* Look up the bundle with all the connections to this particular host.
     Locks the connection cache, beware of early returns! */
  bundle = Curl_conncache_find_bundle(data, needle, data->state.conn_cache);
  if(bundle) {
    /* Max pipe length is zero (unlimited) for multiplexed connections */
    struct Curl_llist_element *curr;

    infof(data, "Found bundle for host: %p [%s]",
          (void *)bundle, (bundle->multiuse == BUNDLE_MULTIPLEX ?
                           "can multiplex" : "serially"));

    /* We can't multiplex if we don't know anything about the server */
    if(canmultiplex) {
      if(bundle->multiuse == BUNDLE_UNKNOWN) {
        if(data->set.pipewait) {
          infof(data, "Server doesn't support multiplex yet, wait");
          *waitpipe = TRUE;
          CONNCACHE_UNLOCK(data);
          return FALSE; /* no reuse */
        }

        infof(data, "Server doesn't support multiplex (yet)");
        canmultiplex = FALSE;
      }
      if((bundle->multiuse == BUNDLE_MULTIPLEX) &&
         !Curl_multiplex_wanted(data->multi)) {
        infof(data, "Could multiplex, but not asked to");
        canmultiplex = FALSE;
      }
      if(bundle->multiuse == BUNDLE_NO_MULTIUSE) {
        infof(data, "Can not multiplex, even if we wanted to");
        canmultiplex = FALSE;
      }
    }

    curr = bundle->conn_list.head;
    while(curr) {
      bool match = FALSE;
      size_t multiplexed = 0;

      /*
       * Note that if we use an HTTP proxy in normal mode (no tunneling), we
       * check connections to that proxy and not to the actual remote server.
       */
      check = curr->ptr;
      curr = curr->next;

      if(check->connect_only || check->bits.close)
        /* connect-only or to-be-closed connections will not be reused */
        continue;

      if(extract_if_dead(check, data)) {
        /* disconnect it */
        Curl_disconnect(data, check, TRUE);
        continue;
      }

      if(data->set.ipver != CURL_IPRESOLVE_WHATEVER
         && data->set.ipver != check->ip_version) {
        /* skip because the connection is not via the requested IP version */
        continue;
      }

      if(bundle->multiuse == BUNDLE_MULTIPLEX)
        multiplexed = CONN_INUSE(check);

      if(!canmultiplex) {
        if(multiplexed) {
          /* can only happen within multi handles, and means that another easy
             handle is using this connection */
          continue;
        }

        if(Curl_resolver_asynch() &&
           /* primary_ip[0] is NUL only if the resolving of the name hasn't
              completed yet and until then we don't reuse this connection */
           !check->primary_ip[0])
          continue;
      }

      if(!Curl_conn_is_connected(check, FIRSTSOCKET)) {
        foundPendingCandidate = TRUE;
        /* Don't pick a connection that hasn't connected yet */
        infof(data, "Connection #%" CURL_FORMAT_CURL_OFF_T
              "isn't open enough, can't reuse", check->connection_id);
        continue;
      }

#ifdef USE_UNIX_SOCKETS
      if(needle->unix_domain_socket) {
        if(!check->unix_domain_socket)
          continue;
        if(strcmp(needle->unix_domain_socket, check->unix_domain_socket))
          continue;
        if(needle->bits.abstract_unix_socket !=
           check->bits.abstract_unix_socket)
          continue;
      }
      else if(check->unix_domain_socket)
        continue;
#endif

      if((needle->handler->flags&PROTOPT_SSL) !=
         (check->handler->flags&PROTOPT_SSL))
        /* don't do mixed SSL and non-SSL connections */
        if(get_protocol_family(check->handler) !=
           needle->handler->protocol || !check->bits.tls_upgraded)
          /* except protocols that have been upgraded via TLS */
          continue;

#ifndef CURL_DISABLE_PROXY
      if(needle->bits.httpproxy != check->bits.httpproxy ||
         needle->bits.socksproxy != check->bits.socksproxy)
        continue;

      if(needle->bits.socksproxy &&
        !socks_proxy_info_matches(&needle->socks_proxy,
                                  &check->socks_proxy))
        continue;
#endif
      if(needle->bits.conn_to_host != check->bits.conn_to_host)
        /* don't mix connections that use the "connect to host" feature and
         * connections that don't use this feature */
        continue;

      if(needle->bits.conn_to_port != check->bits.conn_to_port)
        /* don't mix connections that use the "connect to port" feature and
         * connections that don't use this feature */
        continue;

#ifndef CURL_DISABLE_PROXY
      if(needle->bits.httpproxy) {
        if(!proxy_info_matches(&needle->http_proxy, &check->http_proxy))
          continue;

        if(needle->bits.tunnel_proxy != check->bits.tunnel_proxy)
          continue;

        if(IS_HTTPS_PROXY(needle->http_proxy.proxytype)) {
          /* use https proxy */
          if(needle->http_proxy.proxytype !=
             check->http_proxy.proxytype)
            continue;
          else if(needle->handler->flags&PROTOPT_SSL) {
            /* use double layer ssl */
            if(!Curl_ssl_config_matches(&needle->proxy_ssl_config,
                                        &check->proxy_ssl_config))
              continue;
          }
          else if(!Curl_ssl_config_matches(&needle->ssl_config,
                                           &check->ssl_config))
            continue;
        }
      }
#endif

      if(h2upgrade && !check->httpversion && canmultiplex) {
        if(data->set.pipewait) {
          infof(data, "Server upgrade doesn't support multiplex yet, wait");
          *waitpipe = TRUE;
          CONNCACHE_UNLOCK(data);
          return FALSE; /* no reuse */
        }
        infof(data, "Server upgrade cannot be used");
        continue; /* can't be used atm */
      }

      if(!canmultiplex && CONN_INUSE(check))
        /* this request can't be multiplexed but the checked connection is
           already in use so we skip it */
        continue;

      if(CONN_INUSE(check)) {
        /* Subject for multiplex use if 'checks' belongs to the same multi
           handle as 'data' is. */
        struct Curl_llist_element *e = check->easyq.head;
        struct Curl_easy *entry = e->ptr;
        if(entry->multi != data->multi)
          continue;
      }

      if(needle->localdev || needle->localport) {
        /* If we are bound to a specific local end (IP+port), we must not
           reuse a random other one, although if we didn't ask for a
           particular one we can reuse one that was bound.

           This comparison is a bit rough and too strict. Since the input
           parameters can be specified in numerous ways and still end up the
           same it would take a lot of processing to make it really accurate.
           Instead, this matching will assume that reuses of bound connections
           will most likely also reuse the exact same binding parameters and
           missing out a few edge cases shouldn't hurt anyone very much.
        */
        if((check->localport != needle->localport) ||
           (check->localportrange != needle->localportrange) ||
           (needle->localdev &&
            (!check->localdev || strcmp(check->localdev, needle->localdev))))
          continue;
      }

      if(!(needle->handler->flags & PROTOPT_CREDSPERREQUEST)) {
        /* This protocol requires credentials per connection,
           so verify that we're using the same name and password as well */
        if(Curl_timestrcmp(needle->user, check->user) ||
           Curl_timestrcmp(needle->passwd, check->passwd) ||
           Curl_timestrcmp(needle->sasl_authzid, check->sasl_authzid) ||
           Curl_timestrcmp(needle->oauth_bearer, check->oauth_bearer)) {
          /* one of them was different */
          continue;
        }
      }

      /* GSS delegation differences do not actually affect every connection
         and auth method, but this check takes precaution before efficiency */
      if(needle->gssapi_delegation != check->gssapi_delegation)
        continue;

      /* If multiplexing isn't enabled on the h2 connection and h1 is
         explicitly requested, handle it: */
      if((needle->handler->protocol & PROTO_FAMILY_HTTP) &&
         (((check->httpversion >= 20) &&
           (data->state.httpwant < CURL_HTTP_VERSION_2_0))
          || ((check->httpversion >= 30) &&
              (data->state.httpwant < CURL_HTTP_VERSION_3))))
        continue;
#ifdef USE_SSH
      else if(get_protocol_family(needle->handler) & PROTO_FAMILY_SSH) {
        if(!ssh_config_matches(needle, check))
          continue;
      }
#endif
#ifndef CURL_DISABLE_FTP
      else if(get_protocol_family(needle->handler) & PROTO_FAMILY_FTP) {
        /* Also match ACCOUNT, ALTERNATIVE-TO-USER, USE_SSL and CCC options */
        if(Curl_timestrcmp(needle->proto.ftpc.account,
                           check->proto.ftpc.account) ||
           Curl_timestrcmp(needle->proto.ftpc.alternative_to_user,
                           check->proto.ftpc.alternative_to_user) ||
           (needle->proto.ftpc.use_ssl != check->proto.ftpc.use_ssl) ||
           (needle->proto.ftpc.ccc != check->proto.ftpc.ccc))
          continue;
      }
#endif

      if((needle->handler->flags&PROTOPT_SSL)
#ifndef CURL_DISABLE_PROXY
         || !needle->bits.httpproxy || needle->bits.tunnel_proxy
#endif
        ) {
        /* The requested connection does not use an HTTP proxy or it uses SSL
           or it is a non-SSL protocol tunneled or it is a non-SSL protocol
           which is allowed to be upgraded via TLS */

        if((strcasecompare(needle->handler->scheme, check->handler->scheme) ||
            (get_protocol_family(check->handler) ==
             needle->handler->protocol && check->bits.tls_upgraded)) &&
           (!needle->bits.conn_to_host || strcasecompare(
            needle->conn_to_host.name, check->conn_to_host.name)) &&
           (!needle->bits.conn_to_port ||
             needle->conn_to_port == check->conn_to_port) &&
           strcasecompare(needle->host.name, check->host.name) &&
           needle->remote_port == check->remote_port) {
          /* The schemes match or the protocol family is the same and the
             previous connection was TLS upgraded, and the hostname and host
             port match */
          if(needle->handler->flags & PROTOPT_SSL) {
            /* This is a SSL connection so verify that we're using the same
               SSL options as well */
            if(!Curl_ssl_config_matches(&needle->ssl_config,
                                        &check->ssl_config)) {
              DEBUGF(infof(data,
                           "Connection #%" CURL_FORMAT_CURL_OFF_T
                           " has different SSL parameters, can't reuse",
                           check->connection_id));
              continue;
            }
          }
          match = TRUE;
        }
      }
      else {
        /* The requested connection is using the same HTTP proxy in normal
           mode (no tunneling) */
        match = TRUE;
      }

      if(match) {
#if defined(USE_NTLM)
        /* If we are looking for an HTTP+NTLM connection, check if this is
           already authenticating with the right credentials. If not, keep
           looking so that we can reuse NTLM connections if
           possible. (Especially we must not reuse the same connection if
           partway through a handshake!) */
        if(wantNTLMhttp) {
          if(Curl_timestrcmp(needle->user, check->user) ||
             Curl_timestrcmp(needle->passwd, check->passwd)) {

            /* we prefer a credential match, but this is at least a connection
               that can be reused and "upgraded" to NTLM */
            if(check->http_ntlm_state == NTLMSTATE_NONE)
              chosen = check;
            continue;
          }
        }
        else if(check->http_ntlm_state != NTLMSTATE_NONE) {
          /* Connection is using NTLM auth but we don't want NTLM */
          continue;
        }

#ifndef CURL_DISABLE_PROXY
        /* Same for Proxy NTLM authentication */
        if(wantProxyNTLMhttp) {
          /* Both check->http_proxy.user and check->http_proxy.passwd can be
           * NULL */
          if(!check->http_proxy.user || !check->http_proxy.passwd)
            continue;

          if(Curl_timestrcmp(needle->http_proxy.user,
                             check->http_proxy.user) ||
             Curl_timestrcmp(needle->http_proxy.passwd,
                             check->http_proxy.passwd))
            continue;
        }
        else if(check->proxy_ntlm_state != NTLMSTATE_NONE) {
          /* Proxy connection is using NTLM auth but we don't want NTLM */
          continue;
        }
#endif
        if(wantNTLMhttp || wantProxyNTLMhttp) {
          /* Credentials are already checked, we can use this connection */
          chosen = check;

          if((wantNTLMhttp &&
             (check->http_ntlm_state != NTLMSTATE_NONE)) ||
              (wantProxyNTLMhttp &&
               (check->proxy_ntlm_state != NTLMSTATE_NONE))) {
            /* We must use this connection, no other */
            *force_reuse = TRUE;
            break;
          }

          /* Continue look up for a better connection */
          continue;
        }
#endif
        if(canmultiplex) {
          /* We can multiplex if we want to. Let's continue looking for
             the optimal connection to use. */

          if(!multiplexed) {
            /* We have the optimal connection. Let's stop looking. */
            chosen = check;
            break;
          }

#ifdef USE_NGHTTP2
          /* If multiplexed, make sure we don't go over concurrency limit */
          if(check->bits.multiplex) {
            if(multiplexed >= Curl_conn_get_max_concurrent(data, check,
                                                           FIRSTSOCKET)) {
              infof(data, "MAX_CONCURRENT_STREAMS reached, skip (%zu)",
                    multiplexed);
              continue;
            }
            else if(multiplexed >=
                    Curl_multi_max_concurrent_streams(data->multi)) {
              infof(data, "client side MAX_CONCURRENT_STREAMS reached"
                    ", skip (%zu)",
                    multiplexed);
              continue;
            }
          }
#endif
          /* When not multiplexed, we have a match here! */
          chosen = check;
          infof(data, "Multiplexed connection found");
          break;
        }
        else {
          /* We have found a connection. Let's stop searching. */
          chosen = check;
          break;
        }
      }
    }
  }

  if(chosen) {
    /* mark it as used before releasing the lock */
    Curl_attach_connection(data, chosen);
    CONNCACHE_UNLOCK(data);
    *usethis = chosen;
    return TRUE; /* yes, we found one to use! */
  }
  CONNCACHE_UNLOCK(data);

  if(foundPendingCandidate && data->set.pipewait) {
    infof(data,
          "Found pending candidate for reuse and CURLOPT_PIPEWAIT is set");
    *waitpipe = TRUE;
  }

  return FALSE; /* no matching connecting exists */
}

/*
 * verboseconnect() displays verbose information after a connect
 */
#ifndef CURL_DISABLE_VERBOSE_STRINGS
void Curl_verboseconnect(struct Curl_easy *data,
                         struct connectdata *conn)
{
  if(data->set.verbose)
    infof(data, "Connected to %s (%s) port %u",
          CURL_CONN_HOST_DISPNAME(conn), conn->primary_ip, conn->port);
}
#endif

/*
 * Allocate and initialize a new connectdata object.
 */
static struct connectdata *allocate_conn(struct Curl_easy *data)
{
  struct connectdata *conn = calloc(1, sizeof(struct connectdata));
  if(!conn)
    return NULL;

  /* and we setup a few fields in case we end up actually using this struct */

  conn->sock[FIRSTSOCKET] = CURL_SOCKET_BAD;     /* no file descriptor */
  conn->sock[SECONDARYSOCKET] = CURL_SOCKET_BAD; /* no file descriptor */
  conn->connection_id = -1;    /* no ID */
  conn->port = -1; /* unknown at this point */
  conn->remote_port = -1; /* unknown at this point */

  /* Default protocol-independent behavior doesn't support persistent
     connections, so we set this to force-close. Protocols that support
     this need to set this to FALSE in their "curl_do" functions. */
  connclose(conn, "Default to force-close");

  /* Store creation time to help future close decision making */
  conn->created = Curl_now();

  /* Store current time to give a baseline to keepalive connection times. */
  conn->keepalive = conn->created;

#ifndef CURL_DISABLE_PROXY
  conn->http_proxy.proxytype = data->set.proxytype;
  conn->socks_proxy.proxytype = CURLPROXY_SOCKS4;

  /* note that these two proxy bits are now just on what looks to be
     requested, they may be altered down the road */
  conn->bits.proxy = (data->set.str[STRING_PROXY] &&
                      *data->set.str[STRING_PROXY]) ? TRUE : FALSE;
  conn->bits.httpproxy = (conn->bits.proxy &&
                          (conn->http_proxy.proxytype == CURLPROXY_HTTP ||
                           conn->http_proxy.proxytype == CURLPROXY_HTTP_1_0 ||
                           IS_HTTPS_PROXY(conn->http_proxy.proxytype))) ?
    TRUE : FALSE;
  conn->bits.socksproxy = (conn->bits.proxy &&
                           !conn->bits.httpproxy) ? TRUE : FALSE;

  if(data->set.str[STRING_PRE_PROXY] && *data->set.str[STRING_PRE_PROXY]) {
    conn->bits.proxy = TRUE;
    conn->bits.socksproxy = TRUE;
  }

  conn->bits.proxy_user_passwd =
    (data->state.aptr.proxyuser) ? TRUE : FALSE;
  conn->bits.tunnel_proxy = data->set.tunnel_thru_httpproxy;
#endif /* CURL_DISABLE_PROXY */

#ifndef CURL_DISABLE_FTP
  conn->bits.ftp_use_epsv = data->set.ftp_use_epsv;
  conn->bits.ftp_use_eprt = data->set.ftp_use_eprt;
#endif
  conn->ssl_config.verifystatus = data->set.ssl.primary.verifystatus;
  conn->ssl_config.verifypeer = data->set.ssl.primary.verifypeer;
  conn->ssl_config.verifyhost = data->set.ssl.primary.verifyhost;
  conn->ssl_config.ssl_options = data->set.ssl.primary.ssl_options;
#ifndef CURL_DISABLE_PROXY
  conn->proxy_ssl_config.verifystatus =
    data->set.proxy_ssl.primary.verifystatus;
  conn->proxy_ssl_config.verifypeer = data->set.proxy_ssl.primary.verifypeer;
  conn->proxy_ssl_config.verifyhost = data->set.proxy_ssl.primary.verifyhost;
  conn->proxy_ssl_config.ssl_options = data->set.proxy_ssl.primary.ssl_options;
#endif
  conn->ip_version = data->set.ipver;
  conn->connect_only = data->set.connect_only;
  conn->transport = TRNSPRT_TCP; /* most of them are TCP streams */

#if !defined(CURL_DISABLE_HTTP) && defined(USE_NTLM) && \
    defined(NTLM_WB_ENABLED)
  conn->ntlm.ntlm_auth_hlpr_socket = CURL_SOCKET_BAD;
  conn->proxyntlm.ntlm_auth_hlpr_socket = CURL_SOCKET_BAD;
#endif

  /* Initialize the easy handle list */
  Curl_llist_init(&conn->easyq, NULL);

#ifdef HAVE_GSSAPI
  conn->data_prot = PROT_CLEAR;
#endif

  /* Store the local bind parameters that will be used for this connection */
  if(data->set.str[STRING_DEVICE]) {
    conn->localdev = strdup(data->set.str[STRING_DEVICE]);
    if(!conn->localdev)
      goto error;
  }
#ifndef CURL_DISABLE_BINDLOCAL
  conn->localportrange = data->set.localportrange;
  conn->localport = data->set.localport;
#endif

  /* the close socket stuff needs to be copied to the connection struct as
     it may live on without (this specific) Curl_easy */
  conn->fclosesocket = data->set.fclosesocket;
  conn->closesocket_client = data->set.closesocket_client;
  conn->lastused = conn->created;
  conn->gssapi_delegation = data->set.gssapi_delegation;

  return conn;
error:

  free(conn->localdev);
  free(conn);
  return NULL;
}

/* returns the handler if the given scheme is built-in */
const struct Curl_handler *Curl_builtin_scheme(const char *scheme,
                                               size_t schemelen)
{
  const struct Curl_handler * const *pp;
  const struct Curl_handler *p;
  /* Scan protocol handler table and match against 'scheme'. The handler may
     be changed later when the protocol specific setup function is called. */
  if(schemelen == CURL_ZERO_TERMINATED)
    schemelen = strlen(scheme);
  for(pp = protocols; (p = *pp) != NULL; pp++)
    if(strncasecompare(p->scheme, scheme, schemelen) && !p->scheme[schemelen])
      /* Protocol found in table. */
      return p;
  return NULL; /* not found */
}


static CURLcode findprotocol(struct Curl_easy *data,
                             struct connectdata *conn,
                             const char *protostr)
{
  const struct Curl_handler *p = Curl_builtin_scheme(protostr,
                                                     CURL_ZERO_TERMINATED);

  if(p && /* Protocol found in table. Check if allowed */
     (data->set.allowed_protocols & p->protocol)) {

    /* it is allowed for "normal" request, now do an extra check if this is
       the result of a redirect */
    if(data->state.this_is_a_follow &&
       !(data->set.redir_protocols & p->protocol))
      /* nope, get out */
      ;
    else {
      /* Perform setup complement if some. */
      conn->handler = conn->given = p;

      /* 'port' and 'remote_port' are set in setup_connection_internals() */
      return CURLE_OK;
    }
  }

  /* The protocol was not found in the table, but we don't have to assign it
     to anything since it is already assigned to a dummy-struct in the
     create_conn() function when the connectdata struct is allocated. */
  failf(data, "Protocol \"%s\" not supported or disabled in " LIBCURL_NAME,
        protostr);

  return CURLE_UNSUPPORTED_PROTOCOL;
}


CURLcode Curl_uc_to_curlcode(CURLUcode uc)
{
  switch(uc) {
  default:
    return CURLE_URL_MALFORMAT;
  case CURLUE_UNSUPPORTED_SCHEME:
    return CURLE_UNSUPPORTED_PROTOCOL;
  case CURLUE_OUT_OF_MEMORY:
    return CURLE_OUT_OF_MEMORY;
  case CURLUE_USER_NOT_ALLOWED:
    return CURLE_LOGIN_DENIED;
  }
}

#ifdef ENABLE_IPV6
/*
 * If the URL was set with an IPv6 numerical address with a zone id part, set
 * the scope_id based on that!
 */

static void zonefrom_url(CURLU *uh, struct Curl_easy *data,
                         struct connectdata *conn)
{
  char *zoneid;
  CURLUcode uc = curl_url_get(uh, CURLUPART_ZONEID, &zoneid, 0);
#ifdef CURL_DISABLE_VERBOSE_STRINGS
  (void)data;
#endif

  if(!uc && zoneid) {
    char *endp;
    unsigned long scope = strtoul(zoneid, &endp, 10);
    if(!*endp && (scope < UINT_MAX))
      /* A plain number, use it directly as a scope id. */
      conn->scope_id = (unsigned int)scope;
#if defined(HAVE_IF_NAMETOINDEX)
    else {
#elif defined(WIN32)
    else if(Curl_if_nametoindex) {
#endif

#if defined(HAVE_IF_NAMETOINDEX) || defined(WIN32)
      /* Zone identifier is not numeric */
      unsigned int scopeidx = 0;
#if defined(WIN32)
      scopeidx = Curl_if_nametoindex(zoneid);
#else
      scopeidx = if_nametoindex(zoneid);
#endif
      if(!scopeidx) {
#ifndef CURL_DISABLE_VERBOSE_STRINGS
        char buffer[STRERROR_LEN];
        infof(data, "Invalid zoneid: %s; %s", zoneid,
              Curl_strerror(errno, buffer, sizeof(buffer)));
#endif
      }
      else
        conn->scope_id = scopeidx;
    }
#endif /* HAVE_IF_NAMETOINDEX || WIN32 */

    free(zoneid);
  }
}
#else
#define zonefrom_url(a,b,c) Curl_nop_stmt
#endif

/*
 * Parse URL and fill in the relevant members of the connection struct.
 */
static CURLcode parseurlandfillconn(struct Curl_easy *data,
                                    struct connectdata *conn)
{
  CURLcode result;
  CURLU *uh;
  CURLUcode uc;
  char *hostname;
  bool use_set_uh = (data->set.uh && !data->state.this_is_a_follow);

  up_free(data); /* cleanup previous leftovers first */

  /* parse the URL */
  if(use_set_uh) {
    uh = data->state.uh = curl_url_dup(data->set.uh);
  }
  else {
    uh = data->state.uh = curl_url();
  }

  if(!uh)
    return CURLE_OUT_OF_MEMORY;

  if(data->set.str[STRING_DEFAULT_PROTOCOL] &&
     !Curl_is_absolute_url(data->state.url, NULL, 0, TRUE)) {
    char *url = aprintf("%s://%s", data->set.str[STRING_DEFAULT_PROTOCOL],
                        data->state.url);
    if(!url)
      return CURLE_OUT_OF_MEMORY;
    if(data->state.url_alloc)
      free(data->state.url);
    data->state.url = url;
    data->state.url_alloc = TRUE;
  }

  if(!use_set_uh) {
    char *newurl;
    uc = curl_url_set(uh, CURLUPART_URL, data->state.url,
                      CURLU_GUESS_SCHEME |
                      CURLU_NON_SUPPORT_SCHEME |
                      (data->set.disallow_username_in_url ?
                       CURLU_DISALLOW_USER : 0) |
                      (data->set.path_as_is ? CURLU_PATH_AS_IS : 0));
    if(uc) {
      failf(data, "URL rejected: %s", curl_url_strerror(uc));
      return Curl_uc_to_curlcode(uc);
    }

    /* after it was parsed, get the generated normalized version */
    uc = curl_url_get(uh, CURLUPART_URL, &newurl, 0);
    if(uc)
      return Curl_uc_to_curlcode(uc);
    if(data->state.url_alloc)
      free(data->state.url);
    data->state.url = newurl;
    data->state.url_alloc = TRUE;
  }

  uc = curl_url_get(uh, CURLUPART_SCHEME, &data->state.up.scheme, 0);
  if(uc)
    return Curl_uc_to_curlcode(uc);

  uc = curl_url_get(uh, CURLUPART_HOST, &data->state.up.hostname, 0);
  if(uc) {
    if(!strcasecompare("file", data->state.up.scheme))
      return CURLE_OUT_OF_MEMORY;
  }
  else if(strlen(data->state.up.hostname) > MAX_URL_LEN) {
    failf(data, "Too long host name (maximum is %d)", MAX_URL_LEN);
    return CURLE_URL_MALFORMAT;
  }
  hostname = data->state.up.hostname;

  if(hostname && hostname[0] == '[') {
    /* This looks like an IPv6 address literal. See if there is an address
       scope. */
    size_t hlen;
    conn->bits.ipv6_ip = TRUE;
    /* cut off the brackets! */
    hostname++;
    hlen = strlen(hostname);
    hostname[hlen - 1] = 0;

    zonefrom_url(uh, data, conn);
  }

  /* make sure the connect struct gets its own copy of the host name */
  conn->host.rawalloc = strdup(hostname ? hostname : "");
  if(!conn->host.rawalloc)
    return CURLE_OUT_OF_MEMORY;
  conn->host.name = conn->host.rawalloc;

  /*************************************************************
   * IDN-convert the hostnames
   *************************************************************/
  result = Curl_idnconvert_hostname(&conn->host);
  if(result)
    return result;

#ifndef CURL_DISABLE_HSTS
  /* HSTS upgrade */
  if(data->hsts && strcasecompare("http", data->state.up.scheme)) {
    /* This MUST use the IDN decoded name */
    if(Curl_hsts(data->hsts, conn->host.name, TRUE)) {
      char *url;
      Curl_safefree(data->state.up.scheme);
      uc = curl_url_set(uh, CURLUPART_SCHEME, "https", 0);
      if(uc)
        return Curl_uc_to_curlcode(uc);
      if(data->state.url_alloc)
        Curl_safefree(data->state.url);
      /* after update, get the updated version */
      uc = curl_url_get(uh, CURLUPART_URL, &url, 0);
      if(uc)
        return Curl_uc_to_curlcode(uc);
      uc = curl_url_get(uh, CURLUPART_SCHEME, &data->state.up.scheme, 0);
      if(uc) {
        free(url);
        return Curl_uc_to_curlcode(uc);
      }
      data->state.url = url;
      data->state.url_alloc = TRUE;
      infof(data, "Switched from HTTP to HTTPS due to HSTS => %s",
            data->state.url);
    }
  }
#endif

  result = findprotocol(data, conn, data->state.up.scheme);
  if(result)
    return result;

  /*
   * User name and password set with their own options override the
   * credentials possibly set in the URL.
   */
  if(!data->set.str[STRING_PASSWORD]) {
    uc = curl_url_get(uh, CURLUPART_PASSWORD, &data->state.up.password, 0);
    if(!uc) {
      char *decoded;
      result = Curl_urldecode(data->state.up.password, 0, &decoded, NULL,
                              conn->handler->flags&PROTOPT_USERPWDCTRL ?
                              REJECT_ZERO : REJECT_CTRL);
      if(result)
        return result;
      conn->passwd = decoded;
      result = Curl_setstropt(&data->state.aptr.passwd, decoded);
      if(result)
        return result;
    }
    else if(uc != CURLUE_NO_PASSWORD)
      return Curl_uc_to_curlcode(uc);
  }

  if(!data->set.str[STRING_USERNAME]) {
    /* we don't use the URL API's URL decoder option here since it rejects
       control codes and we want to allow them for some schemes in the user
       and password fields */
    uc = curl_url_get(uh, CURLUPART_USER, &data->state.up.user, 0);
    if(!uc) {
      char *decoded;
      result = Curl_urldecode(data->state.up.user, 0, &decoded, NULL,
                              conn->handler->flags&PROTOPT_USERPWDCTRL ?
                              REJECT_ZERO : REJECT_CTRL);
      if(result)
        return result;
      conn->user = decoded;
      result = Curl_setstropt(&data->state.aptr.user, decoded);
    }
    else if(uc != CURLUE_NO_USER)
      return Curl_uc_to_curlcode(uc);
    else if(data->state.aptr.passwd) {
      /* no user was set but a password, set a blank user */
      result = Curl_setstropt(&data->state.aptr.user, "");
    }
    if(result)
      return result;
  }

  uc = curl_url_get(uh, CURLUPART_OPTIONS, &data->state.up.options,
                    CURLU_URLDECODE);
  if(!uc) {
    conn->options = strdup(data->state.up.options);
    if(!conn->options)
      return CURLE_OUT_OF_MEMORY;
  }
  else if(uc != CURLUE_NO_OPTIONS)
    return Curl_uc_to_curlcode(uc);

  uc = curl_url_get(uh, CURLUPART_PATH, &data->state.up.path,
                    CURLU_URLENCODE);
  if(uc)
    return Curl_uc_to_curlcode(uc);

  uc = curl_url_get(uh, CURLUPART_PORT, &data->state.up.port,
                    CURLU_DEFAULT_PORT);
  if(uc) {
    if(!strcasecompare("file", data->state.up.scheme))
      return CURLE_OUT_OF_MEMORY;
  }
  else {
    unsigned long port = strtoul(data->state.up.port, NULL, 10);
    conn->port = conn->remote_port =
      (data->set.use_port && data->state.allow_port) ?
      data->set.use_port : curlx_ultous(port);
  }

  (void)curl_url_get(uh, CURLUPART_QUERY, &data->state.up.query, 0);

#ifdef ENABLE_IPV6
  if(data->set.scope_id)
    /* Override any scope that was set above.  */
    conn->scope_id = data->set.scope_id;
#endif

  return CURLE_OK;
}


/*
 * If we're doing a resumed transfer, we need to setup our stuff
 * properly.
 */
static CURLcode setup_range(struct Curl_easy *data)
{
  struct UrlState *s = &data->state;
  s->resume_from = data->set.set_resume_from;
  if(s->resume_from || data->set.str[STRING_SET_RANGE]) {
    if(s->rangestringalloc)
      free(s->range);

    if(s->resume_from)
      s->range = aprintf("%" CURL_FORMAT_CURL_OFF_T "-", s->resume_from);
    else
      s->range = strdup(data->set.str[STRING_SET_RANGE]);

    s->rangestringalloc = (s->range) ? TRUE : FALSE;

    if(!s->range)
      return CURLE_OUT_OF_MEMORY;

    /* tell ourselves to fetch this range */
    s->use_range = TRUE;        /* enable range download */
  }
  else
    s->use_range = FALSE; /* disable range download */

  return CURLE_OK;
}


/*
 * setup_connection_internals() -
 *
 * Setup connection internals specific to the requested protocol in the
 * Curl_easy. This is inited and setup before the connection is made but
 * is about the particular protocol that is to be used.
 *
 * This MUST get called after proxy magic has been figured out.
 */
static CURLcode setup_connection_internals(struct Curl_easy *data,
                                           struct connectdata *conn)
{
  const struct Curl_handler *p;
  CURLcode result;

  /* Perform setup complement if some. */
  p = conn->handler;

  if(p->setup_connection) {
    result = (*p->setup_connection)(data, conn);

    if(result)
      return result;

    p = conn->handler;              /* May have changed. */
  }

  if(conn->port < 0)
    /* we check for -1 here since if proxy was detected already, this
       was very likely already set to the proxy port */
    conn->port = p->defport;

  return CURLE_OK;
}

/*
 * Curl_free_request_state() should free temp data that was allocated in the
 * Curl_easy for this single request.
 */

void Curl_free_request_state(struct Curl_easy *data)
{
  Curl_safefree(data->req.p.http);
  Curl_safefree(data->req.newurl);

#ifndef CURL_DISABLE_DOH
  if(data->req.doh) {
    Curl_close(&data->req.doh->probe[0].easy);
    Curl_close(&data->req.doh->probe[1].easy);
  }
#endif
}


#ifndef CURL_DISABLE_PROXY

#ifndef CURL_DISABLE_HTTP
/****************************************************************
* Detect what (if any) proxy to use. Remember that this selects a host
* name and is not limited to HTTP proxies only.
* The returned pointer must be freed by the caller (unless NULL)
****************************************************************/
static char *detect_proxy(struct Curl_easy *data,
                          struct connectdata *conn)
{
  char *proxy = NULL;

  /* If proxy was not specified, we check for default proxy environment
   * variables, to enable i.e Lynx compliance:
   *
   * http_proxy=http://some.server.dom:port/
   * https_proxy=http://some.server.dom:port/
   * ftp_proxy=http://some.server.dom:port/
   * no_proxy=domain1.dom,host.domain2.dom
   *   (a comma-separated list of hosts which should
   *   not be proxied, or an asterisk to override
   *   all proxy variables)
   * all_proxy=http://some.server.dom:port/
   *   (seems to exist for the CERN www lib. Probably
   *   the first to check for.)
   *
   * For compatibility, the all-uppercase versions of these variables are
   * checked if the lowercase versions don't exist.
   */
  char proxy_env[128];
  const char *protop = conn->handler->scheme;
  char *envp = proxy_env;
  char *prox;
#ifdef CURL_DISABLE_VERBOSE_STRINGS
  (void)data;
#endif

  /* Now, build <protocol>_proxy and check for such a one to use */
  while(*protop)
    *envp++ = Curl_raw_tolower(*protop++);

  /* append _proxy */
  strcpy(envp, "_proxy");

  /* read the protocol proxy: */
  prox = curl_getenv(proxy_env);

  /*
   * We don't try the uppercase version of HTTP_PROXY because of
   * security reasons:
   *
   * When curl is used in a webserver application
   * environment (cgi or php), this environment variable can
   * be controlled by the web server user by setting the
   * http header 'Proxy:' to some value.
   *
   * This can cause 'internal' http/ftp requests to be
   * arbitrarily redirected by any external attacker.
   */
  if(!prox && !strcasecompare("http_proxy", proxy_env)) {
    /* There was no lowercase variable, try the uppercase version: */
    Curl_strntoupper(proxy_env, proxy_env, sizeof(proxy_env));
    prox = curl_getenv(proxy_env);
  }

  envp = proxy_env;
  if(prox) {
    proxy = prox; /* use this */
  }
  else {
    envp = (char *)"all_proxy";
    proxy = curl_getenv(envp); /* default proxy to use */
    if(!proxy) {
      envp = (char *)"ALL_PROXY";
      proxy = curl_getenv(envp);
    }
  }
  if(proxy)
    infof(data, "Uses proxy env variable %s == '%s'", envp, proxy);

  return proxy;
}
#endif /* CURL_DISABLE_HTTP */

/*
 * If this is supposed to use a proxy, we need to figure out the proxy
 * host name, so that we can reuse an existing connection
 * that may exist registered to the same proxy host.
 */
static CURLcode parse_proxy(struct Curl_easy *data,
                            struct connectdata *conn, char *proxy,
                            curl_proxytype proxytype)
{
  char *portptr = NULL;
  int port = -1;
  char *proxyuser = NULL;
  char *proxypasswd = NULL;
  char *host = NULL;
  bool sockstype;
  CURLUcode uc;
  struct proxy_info *proxyinfo;
  CURLU *uhp = curl_url();
  CURLcode result = CURLE_OK;
  char *scheme = NULL;
#ifdef USE_UNIX_SOCKETS
  char *path = NULL;
  bool is_unix_proxy = FALSE;
#endif


  if(!uhp) {
    result = CURLE_OUT_OF_MEMORY;
    goto error;
  }

  /* When parsing the proxy, allowing non-supported schemes since we have
     these made up ones for proxies. Guess scheme for URLs without it. */
  uc = curl_url_set(uhp, CURLUPART_URL, proxy,
                    CURLU_NON_SUPPORT_SCHEME|CURLU_GUESS_SCHEME);
  if(!uc) {
    /* parsed okay as a URL */
    uc = curl_url_get(uhp, CURLUPART_SCHEME, &scheme, 0);
    if(uc) {
      result = CURLE_OUT_OF_MEMORY;
      goto error;
    }

    if(strcasecompare("https", scheme)) {
      if(proxytype != CURLPROXY_HTTPS2)
        proxytype = CURLPROXY_HTTPS;
      else
        proxytype = CURLPROXY_HTTPS2;
    }
    else if(strcasecompare("socks5h", scheme))
      proxytype = CURLPROXY_SOCKS5_HOSTNAME;
    else if(strcasecompare("socks5", scheme))
      proxytype = CURLPROXY_SOCKS5;
    else if(strcasecompare("socks4a", scheme))
      proxytype = CURLPROXY_SOCKS4A;
    else if(strcasecompare("socks4", scheme) ||
            strcasecompare("socks", scheme))
      proxytype = CURLPROXY_SOCKS4;
    else if(strcasecompare("http", scheme))
      ; /* leave it as HTTP or HTTP/1.0 */
    else {
      /* Any other xxx:// reject! */
      failf(data, "Unsupported proxy scheme for \'%s\'", proxy);
      result = CURLE_COULDNT_CONNECT;
      goto error;
    }
  }
  else {
    failf(data, "Unsupported proxy syntax in \'%s\': %s", proxy,
          curl_url_strerror(uc));
    result = CURLE_COULDNT_RESOLVE_PROXY;
    goto error;
  }

#ifdef USE_SSL
  if(!Curl_ssl_supports(data, SSLSUPP_HTTPS_PROXY))
#endif
    if(IS_HTTPS_PROXY(proxytype)) {
      failf(data, "Unsupported proxy \'%s\', libcurl is built without the "
            "HTTPS-proxy support.", proxy);
      result = CURLE_NOT_BUILT_IN;
      goto error;
    }

  sockstype =
    proxytype == CURLPROXY_SOCKS5_HOSTNAME ||
    proxytype == CURLPROXY_SOCKS5 ||
    proxytype == CURLPROXY_SOCKS4A ||
    proxytype == CURLPROXY_SOCKS4;

  proxyinfo = sockstype ? &conn->socks_proxy : &conn->http_proxy;
  proxyinfo->proxytype = (unsigned char)proxytype;

  /* Is there a username and password given in this proxy url? */
  uc = curl_url_get(uhp, CURLUPART_USER, &proxyuser, CURLU_URLDECODE);
  if(uc && (uc != CURLUE_NO_USER))
    goto error;
  uc = curl_url_get(uhp, CURLUPART_PASSWORD, &proxypasswd, CURLU_URLDECODE);
  if(uc && (uc != CURLUE_NO_PASSWORD))
    goto error;

  if(proxyuser || proxypasswd) {
    Curl_safefree(proxyinfo->user);
    proxyinfo->user = proxyuser;
    result = Curl_setstropt(&data->state.aptr.proxyuser, proxyuser);
    proxyuser = NULL;
    if(result)
      goto error;
    Curl_safefree(proxyinfo->passwd);
    if(!proxypasswd) {
      proxypasswd = strdup("");
      if(!proxypasswd) {
        result = CURLE_OUT_OF_MEMORY;
        goto error;
      }
    }
    proxyinfo->passwd = proxypasswd;
    result = Curl_setstropt(&data->state.aptr.proxypasswd, proxypasswd);
    proxypasswd = NULL;
    if(result)
      goto error;
    conn->bits.proxy_user_passwd = TRUE; /* enable it */
  }

  (void)curl_url_get(uhp, CURLUPART_PORT, &portptr, 0);

  if(portptr) {
    port = (int)strtol(portptr, NULL, 10);
    free(portptr);
  }
  else {
    if(data->set.proxyport)
      /* None given in the proxy string, then get the default one if it is
         given */
      port = (int)data->set.proxyport;
    else {
      if(IS_HTTPS_PROXY(proxytype))
        port = CURL_DEFAULT_HTTPS_PROXY_PORT;
      else
        port = CURL_DEFAULT_PROXY_PORT;
    }
  }
  if(port >= 0) {
    proxyinfo->port = port;
    if(conn->port < 0 || sockstype || !conn->socks_proxy.host.rawalloc)
      conn->port = port;
  }

  /* now, clone the proxy host name */
  uc = curl_url_get(uhp, CURLUPART_HOST, &host, CURLU_URLDECODE);
  if(uc) {
    result = CURLE_OUT_OF_MEMORY;
    goto error;
  }
#ifdef USE_UNIX_SOCKETS
  if(sockstype && strcasecompare(UNIX_SOCKET_PREFIX, host)) {
    uc = curl_url_get(uhp, CURLUPART_PATH, &path, CURLU_URLDECODE);
    if(uc) {
      result = CURLE_OUT_OF_MEMORY;
      goto error;
    }
    /* path will be "/", if no path was found */
    if(strcmp("/", path)) {
      is_unix_proxy = TRUE;
      free(host);
      host = aprintf(UNIX_SOCKET_PREFIX"%s", path);
      if(!host) {
        result = CURLE_OUT_OF_MEMORY;
        goto error;
      }
      Curl_safefree(proxyinfo->host.rawalloc);
      proxyinfo->host.rawalloc = host;
      proxyinfo->host.name = host;
      host = NULL;
    }
  }

  if(!is_unix_proxy) {
#endif
    Curl_safefree(proxyinfo->host.rawalloc);
    proxyinfo->host.rawalloc = host;
    if(host[0] == '[') {
      /* this is a numerical IPv6, strip off the brackets */
      size_t len = strlen(host);
      host[len-1] = 0; /* clear the trailing bracket */
      host++;
      zonefrom_url(uhp, data, conn);
    }
    proxyinfo->host.name = host;
    host = NULL;
#ifdef USE_UNIX_SOCKETS
  }
#endif

error:
  free(proxyuser);
  free(proxypasswd);
  free(host);
  free(scheme);
#ifdef USE_UNIX_SOCKETS
  free(path);
#endif
  curl_url_cleanup(uhp);
  return result;
}

/*
 * Extract the user and password from the authentication string
 */
static CURLcode parse_proxy_auth(struct Curl_easy *data,
                                 struct connectdata *conn)
{
  const char *proxyuser = data->state.aptr.proxyuser ?
    data->state.aptr.proxyuser : "";
  const char *proxypasswd = data->state.aptr.proxypasswd ?
    data->state.aptr.proxypasswd : "";
  CURLcode result = Curl_urldecode(proxyuser, 0, &conn->http_proxy.user, NULL,
                                   REJECT_ZERO);
  if(!result)
    result = Curl_setstropt(&data->state.aptr.proxyuser,
                            conn->http_proxy.user);
  if(!result)
    result = Curl_urldecode(proxypasswd, 0, &conn->http_proxy.passwd,
                            NULL, REJECT_ZERO);
  if(!result)
    result = Curl_setstropt(&data->state.aptr.proxypasswd,
                            conn->http_proxy.passwd);
  return result;
}

/* create_conn helper to parse and init proxy values. to be called after unix
   socket init but before any proxy vars are evaluated. */
static CURLcode create_conn_helper_init_proxy(struct Curl_easy *data,
                                              struct connectdata *conn)
{
  char *proxy = NULL;
  char *socksproxy = NULL;
  char *no_proxy = NULL;
  CURLcode result = CURLE_OK;
  bool spacesep = FALSE;

  /*************************************************************
   * Extract the user and password from the authentication string
   *************************************************************/
  if(conn->bits.proxy_user_passwd) {
    result = parse_proxy_auth(data, conn);
    if(result)
      goto out;
  }

  /*************************************************************
   * Detect what (if any) proxy to use
   *************************************************************/
  if(data->set.str[STRING_PROXY]) {
    proxy = strdup(data->set.str[STRING_PROXY]);
    /* if global proxy is set, this is it */
    if(!proxy) {
      failf(data, "memory shortage");
      result = CURLE_OUT_OF_MEMORY;
      goto out;
    }
  }

  if(data->set.str[STRING_PRE_PROXY]) {
    socksproxy = strdup(data->set.str[STRING_PRE_PROXY]);
    /* if global socks proxy is set, this is it */
    if(!socksproxy) {
      failf(data, "memory shortage");
      result = CURLE_OUT_OF_MEMORY;
      goto out;
    }
  }

  if(!data->set.str[STRING_NOPROXY]) {
    const char *p = "no_proxy";
    no_proxy = curl_getenv(p);
    if(!no_proxy) {
      p = "NO_PROXY";
      no_proxy = curl_getenv(p);
    }
    if(no_proxy) {
      infof(data, "Uses proxy env variable %s == '%s'", p, no_proxy);
    }
  }

  if(Curl_check_noproxy(conn->host.name, data->set.str[STRING_NOPROXY] ?
                        data->set.str[STRING_NOPROXY] : no_proxy,
                        &spacesep)) {
    Curl_safefree(proxy);
    Curl_safefree(socksproxy);
  }
#ifndef CURL_DISABLE_HTTP
  else if(!proxy && !socksproxy)
    /* if the host is not in the noproxy list, detect proxy. */
    proxy = detect_proxy(data, conn);
#endif /* CURL_DISABLE_HTTP */
  if(spacesep)
    infof(data, "space-separated NOPROXY patterns are deprecated");

  Curl_safefree(no_proxy);

#ifdef USE_UNIX_SOCKETS
  /* For the time being do not mix proxy and unix domain sockets. See #1274 */
  if(proxy && conn->unix_domain_socket) {
    free(proxy);
    proxy = NULL;
  }
#endif

  if(proxy && (!*proxy || (conn->handler->flags & PROTOPT_NONETWORK))) {
    free(proxy);  /* Don't bother with an empty proxy string or if the
                     protocol doesn't work with network */
    proxy = NULL;
  }
  if(socksproxy && (!*socksproxy ||
                    (conn->handler->flags & PROTOPT_NONETWORK))) {
    free(socksproxy);  /* Don't bother with an empty socks proxy string or if
                          the protocol doesn't work with network */
    socksproxy = NULL;
  }

  /***********************************************************************
   * If this is supposed to use a proxy, we need to figure out the proxy host
   * name, proxy type and port number, so that we can reuse an existing
   * connection that may exist registered to the same proxy host.
   ***********************************************************************/
  if(proxy || socksproxy) {
    curl_proxytype ptype = (curl_proxytype)conn->http_proxy.proxytype;
    if(proxy) {
      result = parse_proxy(data, conn, proxy, ptype);
      Curl_safefree(proxy); /* parse_proxy copies the proxy string */
      if(result)
        goto out;
    }

    if(socksproxy) {
      result = parse_proxy(data, conn, socksproxy, ptype);
      /* parse_proxy copies the socks proxy string */
      Curl_safefree(socksproxy);
      if(result)
        goto out;
    }

    if(conn->http_proxy.host.rawalloc) {
#ifdef CURL_DISABLE_HTTP
      /* asking for an HTTP proxy is a bit funny when HTTP is disabled... */
      result = CURLE_UNSUPPORTED_PROTOCOL;
      goto out;
#else
      /* force this connection's protocol to become HTTP if compatible */
      if(!(conn->handler->protocol & PROTO_FAMILY_HTTP)) {
        if((conn->handler->flags & PROTOPT_PROXY_AS_HTTP) &&
           !conn->bits.tunnel_proxy)
          conn->handler = &Curl_handler_http;
        else
          /* if not converting to HTTP over the proxy, enforce tunneling */
          conn->bits.tunnel_proxy = TRUE;
      }
      conn->bits.httpproxy = TRUE;
#endif
    }
    else {
      conn->bits.httpproxy = FALSE; /* not an HTTP proxy */
      conn->bits.tunnel_proxy = FALSE; /* no tunneling if not HTTP */
    }

    if(conn->socks_proxy.host.rawalloc) {
      if(!conn->http_proxy.host.rawalloc) {
        /* once a socks proxy */
        if(!conn->socks_proxy.user) {
          conn->socks_proxy.user = conn->http_proxy.user;
          conn->http_proxy.user = NULL;
          Curl_safefree(conn->socks_proxy.passwd);
          conn->socks_proxy.passwd = conn->http_proxy.passwd;
          conn->http_proxy.passwd = NULL;
        }
      }
      conn->bits.socksproxy = TRUE;
    }
    else
      conn->bits.socksproxy = FALSE; /* not a socks proxy */
  }
  else {
    conn->bits.socksproxy = FALSE;
    conn->bits.httpproxy = FALSE;
  }
  conn->bits.proxy = conn->bits.httpproxy || conn->bits.socksproxy;

  if(!conn->bits.proxy) {
    /* we aren't using the proxy after all... */
    conn->bits.proxy = FALSE;
    conn->bits.httpproxy = FALSE;
    conn->bits.socksproxy = FALSE;
    conn->bits.proxy_user_passwd = FALSE;
    conn->bits.tunnel_proxy = FALSE;
    /* CURLPROXY_HTTPS does not have its own flag in conn->bits, yet we need
       to signal that CURLPROXY_HTTPS is not used for this connection */
    conn->http_proxy.proxytype = CURLPROXY_HTTP;
  }

out:

  free(socksproxy);
  free(proxy);
  return result;
}
#endif /* CURL_DISABLE_PROXY */

/*
 * Curl_parse_login_details()
 *
 * This is used to parse a login string for user name, password and options in
 * the following formats:
 *
 *   user
 *   user:password
 *   user:password;options
 *   user;options
 *   user;options:password
 *   :password
 *   :password;options
 *   ;options
 *   ;options:password
 *
 * Parameters:
 *
 * login    [in]     - The login string.
 * len      [in]     - The length of the login string.
 * userp    [in/out] - The address where a pointer to newly allocated memory
 *                     holding the user will be stored upon completion.
 * passwdp  [in/out] - The address where a pointer to newly allocated memory
 *                     holding the password will be stored upon completion.
 * optionsp [in/out] - The address where a pointer to newly allocated memory
 *                     holding the options will be stored upon completion.
 *
 * Returns CURLE_OK on success.
 */
CURLcode Curl_parse_login_details(const char *login, const size_t len,
                                  char **userp, char **passwdp,
                                  char **optionsp)
{
  CURLcode result = CURLE_OK;
  char *ubuf = NULL;
  char *pbuf = NULL;
  char *obuf = NULL;
  const char *psep = NULL;
  const char *osep = NULL;
  size_t ulen;
  size_t plen;
  size_t olen;

  /* Attempt to find the password separator */
  if(passwdp)
    psep = memchr(login, ':', len);

  /* Attempt to find the options separator */
  if(optionsp)
    osep = memchr(login, ';', len);

  /* Calculate the portion lengths */
  ulen = (psep ?
          (size_t)(osep && psep > osep ? osep - login : psep - login) :
          (osep ? (size_t)(osep - login) : len));
  plen = (psep ?
          (osep && osep > psep ? (size_t)(osep - psep) :
                                 (size_t)(login + len - psep)) - 1 : 0);
  olen = (osep ?
          (psep && psep > osep ? (size_t)(psep - osep) :
                                 (size_t)(login + len - osep)) - 1 : 0);

  /* Allocate the user portion buffer, which can be zero length */
  if(userp) {
    ubuf = malloc(ulen + 1);
    if(!ubuf)
      result = CURLE_OUT_OF_MEMORY;
  }

  /* Allocate the password portion buffer */
  if(!result && passwdp && psep) {
    pbuf = malloc(plen + 1);
    if(!pbuf) {
      free(ubuf);
      result = CURLE_OUT_OF_MEMORY;
    }
  }

  /* Allocate the options portion buffer */
  if(!result && optionsp && olen) {
    obuf = malloc(olen + 1);
    if(!obuf) {
      free(pbuf);
      free(ubuf);
      result = CURLE_OUT_OF_MEMORY;
    }
  }

  if(!result) {
    /* Store the user portion if necessary */
    if(ubuf) {
      memcpy(ubuf, login, ulen);
      ubuf[ulen] = '\0';
      Curl_safefree(*userp);
      *userp = ubuf;
    }

    /* Store the password portion if necessary */
    if(pbuf) {
      memcpy(pbuf, psep + 1, plen);
      pbuf[plen] = '\0';
      Curl_safefree(*passwdp);
      *passwdp = pbuf;
    }

    /* Store the options portion if necessary */
    if(obuf) {
      memcpy(obuf, osep + 1, olen);
      obuf[olen] = '\0';
      Curl_safefree(*optionsp);
      *optionsp = obuf;
    }
  }

  return result;
}

/*************************************************************
 * Figure out the remote port number and fix it in the URL
 *
 * No matter if we use a proxy or not, we have to figure out the remote
 * port number of various reasons.
 *
 * The port number embedded in the URL is replaced, if necessary.
 *************************************************************/
static CURLcode parse_remote_port(struct Curl_easy *data,
                                  struct connectdata *conn)
{

  if(data->set.use_port && data->state.allow_port) {
    /* if set, we use this instead of the port possibly given in the URL */
    char portbuf[16];
    CURLUcode uc;
    conn->remote_port = data->set.use_port;
    msnprintf(portbuf, sizeof(portbuf), "%d", conn->remote_port);
    uc = curl_url_set(data->state.uh, CURLUPART_PORT, portbuf, 0);
    if(uc)
      return CURLE_OUT_OF_MEMORY;
  }

  return CURLE_OK;
}

/*
 * Override the login details from the URL with that in the CURLOPT_USERPWD
 * option or a .netrc file, if applicable.
 */
static CURLcode override_login(struct Curl_easy *data,
                               struct connectdata *conn)
{
  CURLUcode uc;
  char **userp = &conn->user;
  char **passwdp = &conn->passwd;
  char **optionsp = &conn->options;

  if(data->set.str[STRING_OPTIONS]) {
    free(*optionsp);
    *optionsp = strdup(data->set.str[STRING_OPTIONS]);
    if(!*optionsp)
      return CURLE_OUT_OF_MEMORY;
  }

#ifndef CURL_DISABLE_NETRC
  if(data->set.use_netrc == CURL_NETRC_REQUIRED) {
    Curl_safefree(*userp);
    Curl_safefree(*passwdp);
  }
  conn->bits.netrc = FALSE;
  if(data->set.use_netrc && !data->set.str[STRING_USERNAME]) {
    int ret;
    bool url_provided = FALSE;

    if(data->state.aptr.user) {
      /* there was a user name in the URL. Use the URL decoded version */
      userp = &data->state.aptr.user;
      url_provided = TRUE;
    }

    ret = Curl_parsenetrc(conn->host.name,
                          userp, passwdp,
                          data->set.str[STRING_NETRC_FILE]);
    if(ret > 0) {
      infof(data, "Couldn't find host %s in the %s file; using defaults",
            conn->host.name, data->set.str[STRING_NETRC_FILE]);
    }
    else if(ret < 0) {
      failf(data, ".netrc parser error");
      return CURLE_READ_ERROR;
    }
    else {
      /* set bits.netrc TRUE to remember that we got the name from a .netrc
         file, so that it is safe to use even if we followed a Location: to a
         different host or similar. */
      conn->bits.netrc = TRUE;
    }
    if(url_provided) {
      Curl_safefree(conn->user);
      conn->user = strdup(*userp);
      if(!conn->user)
        return CURLE_OUT_OF_MEMORY;
    }
    /* no user was set but a password, set a blank user */
    if(!*userp && *passwdp) {
      *userp = strdup("");
      if(!*userp)
        return CURLE_OUT_OF_MEMORY;
    }
  }
#endif

  /* for updated strings, we update them in the URL */
  if(*userp) {
    CURLcode result;
    if(data->state.aptr.user != *userp) {
      /* nothing to do then */
      result = Curl_setstropt(&data->state.aptr.user, *userp);
      if(result)
        return result;
    }
  }
  if(data->state.aptr.user) {
    uc = curl_url_set(data->state.uh, CURLUPART_USER, data->state.aptr.user,
                      CURLU_URLENCODE);
    if(uc)
      return Curl_uc_to_curlcode(uc);
    if(!*userp) {
      *userp = strdup(data->state.aptr.user);
      if(!*userp)
        return CURLE_OUT_OF_MEMORY;
    }
  }
  if(*passwdp) {
    CURLcode result = Curl_setstropt(&data->state.aptr.passwd, *passwdp);
    if(result)
      return result;
  }
  if(data->state.aptr.passwd) {
    uc = curl_url_set(data->state.uh, CURLUPART_PASSWORD,
                      data->state.aptr.passwd, CURLU_URLENCODE);
    if(uc)
      return Curl_uc_to_curlcode(uc);
    if(!*passwdp) {
      *passwdp = strdup(data->state.aptr.passwd);
      if(!*passwdp)
        return CURLE_OUT_OF_MEMORY;
    }
  }

  return CURLE_OK;
}

/*
 * Set the login details so they're available in the connection
 */
static CURLcode set_login(struct Curl_easy *data,
                          struct connectdata *conn)
{
  CURLcode result = CURLE_OK;
  const char *setuser = CURL_DEFAULT_USER;
  const char *setpasswd = CURL_DEFAULT_PASSWORD;

  /* If our protocol needs a password and we have none, use the defaults */
  if((conn->handler->flags & PROTOPT_NEEDSPWD) && !data->state.aptr.user)
    ;
  else {
    setuser = "";
    setpasswd = "";
  }
  /* Store the default user */
  if(!conn->user) {
    conn->user = strdup(setuser);
    if(!conn->user)
      return CURLE_OUT_OF_MEMORY;
  }

  /* Store the default password */
  if(!conn->passwd) {
    conn->passwd = strdup(setpasswd);
    if(!conn->passwd)
      result = CURLE_OUT_OF_MEMORY;
  }

  return result;
}

/*
 * Parses a "host:port" string to connect to.
 * The hostname and the port may be empty; in this case, NULL is returned for
 * the hostname and -1 for the port.
 */
static CURLcode parse_connect_to_host_port(struct Curl_easy *data,
                                           const char *host,
                                           char **hostname_result,
                                           int *port_result)
{
  char *host_dup;
  char *hostptr;
  char *host_portno;
  char *portptr;
  int port = -1;
  CURLcode result = CURLE_OK;

#if defined(CURL_DISABLE_VERBOSE_STRINGS)
  (void) data;
#endif

  *hostname_result = NULL;
  *port_result = -1;

  if(!host || !*host)
    return CURLE_OK;

  host_dup = strdup(host);
  if(!host_dup)
    return CURLE_OUT_OF_MEMORY;

  hostptr = host_dup;

  /* start scanning for port number at this point */
  portptr = hostptr;

  /* detect and extract RFC6874-style IPv6-addresses */
  if(*hostptr == '[') {
#ifdef ENABLE_IPV6
    char *ptr = ++hostptr; /* advance beyond the initial bracket */
    while(*ptr && (ISXDIGIT(*ptr) || (*ptr == ':') || (*ptr == '.')))
      ptr++;
    if(*ptr == '%') {
      /* There might be a zone identifier */
      if(strncmp("%25", ptr, 3))
        infof(data, "Please URL encode %% as %%25, see RFC 6874.");
      ptr++;
      /* Allow unreserved characters as defined in RFC 3986 */
      while(*ptr && (ISALPHA(*ptr) || ISXDIGIT(*ptr) || (*ptr == '-') ||
                     (*ptr == '.') || (*ptr == '_') || (*ptr == '~')))
        ptr++;
    }
    if(*ptr == ']')
      /* yeps, it ended nicely with a bracket as well */
      *ptr++ = '\0';
    else
      infof(data, "Invalid IPv6 address format");
    portptr = ptr;
    /* Note that if this didn't end with a bracket, we still advanced the
     * hostptr first, but I can't see anything wrong with that as no host
     * name nor a numeric can legally start with a bracket.
     */
#else
    failf(data, "Use of IPv6 in *_CONNECT_TO without IPv6 support built-in");
    result = CURLE_NOT_BUILT_IN;
    goto error;
#endif
  }

  /* Get port number off server.com:1080 */
  host_portno = strchr(portptr, ':');
  if(host_portno) {
    char *endp = NULL;
    *host_portno = '\0'; /* cut off number from host name */
    host_portno++;
    if(*host_portno) {
      long portparse = strtol(host_portno, &endp, 10);
      if((endp && *endp) || (portparse < 0) || (portparse > 65535)) {
        failf(data, "No valid port number in connect to host string (%s)",
              host_portno);
        result = CURLE_SETOPT_OPTION_SYNTAX;
        goto error;
      }
      else
        port = (int)portparse; /* we know it will fit */
    }
  }

  /* now, clone the cleaned host name */
  DEBUGASSERT(hostptr);
  *hostname_result = strdup(hostptr);
  if(!*hostname_result) {
    result = CURLE_OUT_OF_MEMORY;
    goto error;
  }

  *port_result = port;

error:
  free(host_dup);
  return result;
}

/*
 * Parses one "connect to" string in the form:
 * "HOST:PORT:CONNECT-TO-HOST:CONNECT-TO-PORT".
 */
static CURLcode parse_connect_to_string(struct Curl_easy *data,
                                        struct connectdata *conn,
                                        const char *conn_to_host,
                                        char **host_result,
                                        int *port_result)
{
  CURLcode result = CURLE_OK;
  const char *ptr = conn_to_host;
  int host_match = FALSE;
  int port_match = FALSE;

  *host_result = NULL;
  *port_result = -1;

  if(*ptr == ':') {
    /* an empty hostname always matches */
    host_match = TRUE;
    ptr++;
  }
  else {
    /* check whether the URL's hostname matches */
    size_t hostname_to_match_len;
    char *hostname_to_match = aprintf("%s%s%s",
                                      conn->bits.ipv6_ip ? "[" : "",
                                      conn->host.name,
                                      conn->bits.ipv6_ip ? "]" : "");
    if(!hostname_to_match)
      return CURLE_OUT_OF_MEMORY;
    hostname_to_match_len = strlen(hostname_to_match);
    host_match = strncasecompare(ptr, hostname_to_match,
                                 hostname_to_match_len);
    free(hostname_to_match);
    ptr += hostname_to_match_len;

    host_match = host_match && *ptr == ':';
    ptr++;
  }

  if(host_match) {
    if(*ptr == ':') {
      /* an empty port always matches */
      port_match = TRUE;
      ptr++;
    }
    else {
      /* check whether the URL's port matches */
      char *ptr_next = strchr(ptr, ':');
      if(ptr_next) {
        char *endp = NULL;
        long port_to_match = strtol(ptr, &endp, 10);
        if((endp == ptr_next) && (port_to_match == conn->remote_port)) {
          port_match = TRUE;
          ptr = ptr_next + 1;
        }
      }
    }
  }

  if(host_match && port_match) {
    /* parse the hostname and port to connect to */
    result = parse_connect_to_host_port(data, ptr, host_result, port_result);
  }

  return result;
}

/*
 * Processes all strings in the "connect to" slist, and uses the "connect
 * to host" and "connect to port" of the first string that matches.
 */
static CURLcode parse_connect_to_slist(struct Curl_easy *data,
                                       struct connectdata *conn,
                                       struct curl_slist *conn_to_host)
{
  CURLcode result = CURLE_OK;
  char *host = NULL;
  int port = -1;

  while(conn_to_host && !host && port == -1) {
    result = parse_connect_to_string(data, conn, conn_to_host->data,
                                     &host, &port);
    if(result)
      return result;

    if(host && *host) {
      conn->conn_to_host.rawalloc = host;
      conn->conn_to_host.name = host;
      conn->bits.conn_to_host = TRUE;

      infof(data, "Connecting to hostname: %s", host);
    }
    else {
      /* no "connect to host" */
      conn->bits.conn_to_host = FALSE;
      Curl_safefree(host);
    }

    if(port >= 0) {
      conn->conn_to_port = port;
      conn->bits.conn_to_port = TRUE;
      infof(data, "Connecting to port: %d", port);
    }
    else {
      /* no "connect to port" */
      conn->bits.conn_to_port = FALSE;
      port = -1;
    }

    conn_to_host = conn_to_host->next;
  }

#ifndef CURL_DISABLE_ALTSVC
  if(data->asi && !host && (port == -1) &&
     ((conn->handler->protocol == CURLPROTO_HTTPS) ||
#ifdef CURLDEBUG
      /* allow debug builds to circumvent the HTTPS restriction */
      getenv("CURL_ALTSVC_HTTP")
#else
      0
#endif
       )) {
    /* no connect_to match, try alt-svc! */
    enum alpnid srcalpnid;
    bool hit;
    struct altsvc *as;
    const int allowed_versions = ( ALPN_h1
#ifdef USE_HTTP2
                                   | ALPN_h2
#endif
#ifdef ENABLE_QUIC
                                   | ALPN_h3
#endif
      ) & data->asi->flags;

    host = conn->host.rawalloc;
#ifdef USE_HTTP2
    /* with h2 support, check that first */
    srcalpnid = ALPN_h2;
    hit = Curl_altsvc_lookup(data->asi,
                             srcalpnid, host, conn->remote_port, /* from */
                             &as /* to */,
                             allowed_versions);
    if(!hit)
#endif
    {
      srcalpnid = ALPN_h1;
      hit = Curl_altsvc_lookup(data->asi,
                               srcalpnid, host, conn->remote_port, /* from */
                               &as /* to */,
                               allowed_versions);
    }
    if(hit) {
      char *hostd = strdup((char *)as->dst.host);
      if(!hostd)
        return CURLE_OUT_OF_MEMORY;
      conn->conn_to_host.rawalloc = hostd;
      conn->conn_to_host.name = hostd;
      conn->bits.conn_to_host = TRUE;
      conn->conn_to_port = as->dst.port;
      conn->bits.conn_to_port = TRUE;
      conn->bits.altused = TRUE;
      infof(data, "Alt-svc connecting from [%s]%s:%d to [%s]%s:%d",
            Curl_alpnid2str(srcalpnid), host, conn->remote_port,
            Curl_alpnid2str(as->dst.alpnid), hostd, as->dst.port);
      if(srcalpnid != as->dst.alpnid) {
        /* protocol version switch */
        switch(as->dst.alpnid) {
        case ALPN_h1:
          conn->httpversion = 11;
          break;
        case ALPN_h2:
          conn->httpversion = 20;
          break;
        case ALPN_h3:
          conn->transport = TRNSPRT_QUIC;
          conn->httpversion = 30;
          break;
        default: /* shouldn't be possible */
          break;
        }
      }
    }
  }
#endif

  return result;
}

#ifdef USE_UNIX_SOCKETS
static CURLcode resolve_unix(struct Curl_easy *data,
                             struct connectdata *conn,
                             char *unix_path)
{
  struct Curl_dns_entry *hostaddr = NULL;
  bool longpath = FALSE;

  DEBUGASSERT(unix_path);
  DEBUGASSERT(conn->dns_entry == NULL);

  /* Unix domain sockets are local. The host gets ignored, just use the
   * specified domain socket address. Do not cache "DNS entries". There is
   * no DNS involved and we already have the filesystem path available. */
  hostaddr = calloc(1, sizeof(struct Curl_dns_entry));
  if(!hostaddr)
    return CURLE_OUT_OF_MEMORY;

  hostaddr->addr = Curl_unix2addr(unix_path, &longpath,
                                  conn->bits.abstract_unix_socket);
  if(!hostaddr->addr) {
    if(longpath)
      /* Long paths are not supported for now */
      failf(data, "Unix socket path too long: '%s'", unix_path);
    free(hostaddr);
    return longpath ? CURLE_COULDNT_RESOLVE_HOST : CURLE_OUT_OF_MEMORY;
  }

  hostaddr->inuse++;
  conn->dns_entry = hostaddr;
  return CURLE_OK;
}
#endif

#ifndef CURL_DISABLE_PROXY
static CURLcode resolve_proxy(struct Curl_easy *data,
                              struct connectdata *conn,
                              bool *async)
{
  struct Curl_dns_entry *hostaddr = NULL;
  struct hostname *host;
  timediff_t timeout_ms = Curl_timeleft(data, NULL, TRUE);
  int rc;

  DEBUGASSERT(conn->dns_entry == NULL);

  host = conn->bits.socksproxy ? &conn->socks_proxy.host :
    &conn->http_proxy.host;

  conn->hostname_resolve = strdup(host->name);
  if(!conn->hostname_resolve)
    return CURLE_OUT_OF_MEMORY;

  rc = Curl_resolv_timeout(data, conn->hostname_resolve, (int)conn->port,
                           &hostaddr, timeout_ms);
  conn->dns_entry = hostaddr;
  if(rc == CURLRESOLV_PENDING)
    *async = TRUE;
  else if(rc == CURLRESOLV_TIMEDOUT)
    return CURLE_OPERATION_TIMEDOUT;
  else if(!hostaddr) {
    failf(data, "Couldn't resolve proxy '%s'", host->dispname);
    return CURLE_COULDNT_RESOLVE_PROXY;
  }

  return CURLE_OK;
}
#endif

static CURLcode resolve_host(struct Curl_easy *data,
                             struct connectdata *conn,
                             bool *async)
{
  struct Curl_dns_entry *hostaddr = NULL;
  struct hostname *connhost;
  timediff_t timeout_ms = Curl_timeleft(data, NULL, TRUE);
  int rc;

  DEBUGASSERT(conn->dns_entry == NULL);

  connhost = conn->bits.conn_to_host ? &conn->conn_to_host : &conn->host;

  /* If not connecting via a proxy, extract the port from the URL, if it is
   * there, thus overriding any defaults that might have been set above. */
  conn->port = conn->bits.conn_to_port ? conn->conn_to_port :
    conn->remote_port;

  /* Resolve target host right on */
  conn->hostname_resolve = strdup(connhost->name);
  if(!conn->hostname_resolve)
    return CURLE_OUT_OF_MEMORY;

  rc = Curl_resolv_timeout(data, conn->hostname_resolve, (int)conn->port,
                           &hostaddr, timeout_ms);
  conn->dns_entry = hostaddr;
  if(rc == CURLRESOLV_PENDING)
    *async = TRUE;
  else if(rc == CURLRESOLV_TIMEDOUT) {
    failf(data, "Failed to resolve host '%s' with timeout after %ld ms",
          connhost->dispname,
          Curl_timediff(Curl_now(), data->progress.t_startsingle));
    return CURLE_OPERATION_TIMEDOUT;
  }
  else if(!hostaddr) {
    failf(data, "Could not resolve host: %s", connhost->dispname);
    return CURLE_COULDNT_RESOLVE_HOST;
  }

  return CURLE_OK;
}

/* Perform a fresh resolve */
static CURLcode resolve_fresh(struct Curl_easy *data,
                              struct connectdata *conn,
                              bool *async)
{
#ifdef USE_UNIX_SOCKETS
  char *unix_path = conn->unix_domain_socket;

#ifndef CURL_DISABLE_PROXY
  if(!unix_path && conn->socks_proxy.host.name &&
     !strncmp(UNIX_SOCKET_PREFIX"/",
              conn->socks_proxy.host.name, sizeof(UNIX_SOCKET_PREFIX)))
    unix_path = conn->socks_proxy.host.name + sizeof(UNIX_SOCKET_PREFIX) - 1;
#endif

  if(unix_path) {
    conn->transport = TRNSPRT_UNIX;
    return resolve_unix(data, conn, unix_path);
  }
#endif

#ifndef CURL_DISABLE_PROXY
  if(CONN_IS_PROXIED(conn))
    return resolve_proxy(data, conn, async);
#endif

  return resolve_host(data, conn, async);
}

/*************************************************************
 * Resolve the address of the server or proxy
 *************************************************************/
static CURLcode resolve_server(struct Curl_easy *data,
                               struct connectdata *conn,
                               bool *async)
{
  DEBUGASSERT(conn);
  DEBUGASSERT(data);

  /* Resolve the name of the server or proxy */
  if(conn->bits.reuse) {
    /* We're reusing the connection - no need to resolve anything, and
       idnconvert_hostname() was called already in create_conn() for the reuse
       case. */
    *async = FALSE;
    return CURLE_OK;
  }

  return resolve_fresh(data, conn, async);
}

/*
 * Cleanup the connection `temp`, just allocated for `data`, before using the
 * previously `existing` one for `data`.  All relevant info is copied over
 * and `temp` is freed.
 */
static void reuse_conn(struct Curl_easy *data,
                       struct connectdata *temp,
                       struct connectdata *existing)
{
  /* get the user+password information from the temp struct since it may
   * be new for this request even when we reuse an existing connection */
  if(temp->user) {
    /* use the new user name and password though */
    Curl_safefree(existing->user);
    Curl_safefree(existing->passwd);
    existing->user = temp->user;
    existing->passwd = temp->passwd;
    temp->user = NULL;
    temp->passwd = NULL;
  }

#ifndef CURL_DISABLE_PROXY
  existing->bits.proxy_user_passwd = temp->bits.proxy_user_passwd;
  if(existing->bits.proxy_user_passwd) {
    /* use the new proxy user name and proxy password though */
    Curl_safefree(existing->http_proxy.user);
    Curl_safefree(existing->socks_proxy.user);
    Curl_safefree(existing->http_proxy.passwd);
    Curl_safefree(existing->socks_proxy.passwd);
    existing->http_proxy.user = temp->http_proxy.user;
    existing->socks_proxy.user = temp->socks_proxy.user;
    existing->http_proxy.passwd = temp->http_proxy.passwd;
    existing->socks_proxy.passwd = temp->socks_proxy.passwd;
    temp->http_proxy.user = NULL;
    temp->socks_proxy.user = NULL;
    temp->http_proxy.passwd = NULL;
    temp->socks_proxy.passwd = NULL;
  }
#endif

  /* Finding a connection for reuse in the cache matches, among other
   * things on the "remote-relevant" hostname. This is not necessarily
   * the authority of the URL, e.g. conn->host. For example:
   * - we use a proxy (not tunneling). we want to send all requests
   *   that use the same proxy on this connection.
   * - we have a "connect-to" setting that may redirect the hostname of
   *   a new request to the same remote endpoint of an existing conn.
   *   We want to reuse an existing conn to the remote endpoint.
   * Since connection reuse does not match on conn->host necessarily, we
   * switch `existing` conn to `temp` conn's host settings.
   * TODO: is this correct in the case of TLS connections that have
   *       used the original hostname in SNI to negotiate? Do we send
   *       requests for another host through the different SNI?
   */
  Curl_free_idnconverted_hostname(&existing->host);
  Curl_free_idnconverted_hostname(&existing->conn_to_host);
  Curl_safefree(existing->host.rawalloc);
  Curl_safefree(existing->conn_to_host.rawalloc);
  existing->host = temp->host;
  temp->host.rawalloc = NULL;
  temp->host.encalloc = NULL;
  existing->conn_to_host = temp->conn_to_host;
  temp->conn_to_host.rawalloc = NULL;
  existing->conn_to_port = temp->conn_to_port;
  existing->remote_port = temp->remote_port;
  Curl_safefree(existing->hostname_resolve);

  existing->hostname_resolve = temp->hostname_resolve;
  temp->hostname_resolve = NULL;

  /* reuse init */
  existing->bits.reuse = TRUE; /* yes, we're reusing here */

  conn_free(data, temp);
}

/**
 * create_conn() sets up a new connectdata struct, or reuses an already
 * existing one, and resolves host name.
 *
 * if this function returns CURLE_OK and *async is set to TRUE, the resolve
 * response will be coming asynchronously. If *async is FALSE, the name is
 * already resolved.
 *
 * @param data The sessionhandle pointer
 * @param in_connect is set to the next connection data pointer
 * @param async is set TRUE when an async DNS resolution is pending
 * @see Curl_setup_conn()
 *
 */

static CURLcode create_conn(struct Curl_easy *data,
                            struct connectdata **in_connect,
                            bool *async)
{
  CURLcode result = CURLE_OK;
  struct connectdata *conn;
  struct connectdata *existing = NULL;
  bool reuse;
  bool connections_available = TRUE;
  bool force_reuse = FALSE;
  bool waitpipe = FALSE;
  size_t max_host_connections = Curl_multi_max_host_connections(data->multi);
  size_t max_total_connections = Curl_multi_max_total_connections(data->multi);

  *async = FALSE;
  *in_connect = NULL;

  /*************************************************************
   * Check input data
   *************************************************************/
  if(!data->state.url) {
    result = CURLE_URL_MALFORMAT;
    goto out;
  }

  /* First, split up the current URL in parts so that we can use the
     parts for checking against the already present connections. In order
     to not have to modify everything at once, we allocate a temporary
     connection data struct and fill in for comparison purposes. */
  conn = allocate_conn(data);

  if(!conn) {
    result = CURLE_OUT_OF_MEMORY;
    goto out;
  }

  /* We must set the return variable as soon as possible, so that our
     parent can cleanup any possible allocs we may have done before
     any failure */
  *in_connect = conn;

  result = parseurlandfillconn(data, conn);
  if(result)
    goto out;

  if(data->set.str[STRING_SASL_AUTHZID]) {
    conn->sasl_authzid = strdup(data->set.str[STRING_SASL_AUTHZID]);
    if(!conn->sasl_authzid) {
      result = CURLE_OUT_OF_MEMORY;
      goto out;
    }
  }

  if(data->set.str[STRING_BEARER]) {
    conn->oauth_bearer = strdup(data->set.str[STRING_BEARER]);
    if(!conn->oauth_bearer) {
      result = CURLE_OUT_OF_MEMORY;
      goto out;
    }
  }

#ifdef USE_UNIX_SOCKETS
  if(data->set.str[STRING_UNIX_SOCKET_PATH]) {
    conn->unix_domain_socket = strdup(data->set.str[STRING_UNIX_SOCKET_PATH]);
    if(!conn->unix_domain_socket) {
      result = CURLE_OUT_OF_MEMORY;
      goto out;
    }
    conn->bits.abstract_unix_socket = data->set.abstract_unix_socket;
  }
#endif

  /* After the unix socket init but before the proxy vars are used, parse and
     initialize the proxy vars */
#ifndef CURL_DISABLE_PROXY
  result = create_conn_helper_init_proxy(data, conn);
  if(result)
    goto out;

  /*************************************************************
   * If the protocol is using SSL and HTTP proxy is used, we set
   * the tunnel_proxy bit.
   *************************************************************/
  if((conn->given->flags&PROTOPT_SSL) && conn->bits.httpproxy)
    conn->bits.tunnel_proxy = TRUE;
#endif

  /*************************************************************
   * Figure out the remote port number and fix it in the URL
   *************************************************************/
  result = parse_remote_port(data, conn);
  if(result)
    goto out;

  /* Check for overridden login details and set them accordingly so that
     they are known when protocol->setup_connection is called! */
  result = override_login(data, conn);
  if(result)
    goto out;

  result = set_login(data, conn); /* default credentials */
  if(result)
    goto out;

  /*************************************************************
   * Process the "connect to" linked list of hostname/port mappings.
   * Do this after the remote port number has been fixed in the URL.
   *************************************************************/
  result = parse_connect_to_slist(data, conn, data->set.connect_to);
  if(result)
    goto out;

  /*************************************************************
   * IDN-convert the proxy hostnames
   *************************************************************/
#ifndef CURL_DISABLE_PROXY
  if(conn->bits.httpproxy) {
    result = Curl_idnconvert_hostname(&conn->http_proxy.host);
    if(result)
      return result;
  }
  if(conn->bits.socksproxy) {
    result = Curl_idnconvert_hostname(&conn->socks_proxy.host);
    if(result)
      return result;
  }
#endif
  if(conn->bits.conn_to_host) {
    result = Curl_idnconvert_hostname(&conn->conn_to_host);
    if(result)
      return result;
  }

  /*************************************************************
   * Check whether the host and the "connect to host" are equal.
   * Do this after the hostnames have been IDN-converted.
   *************************************************************/
  if(conn->bits.conn_to_host &&
     strcasecompare(conn->conn_to_host.name, conn->host.name)) {
    conn->bits.conn_to_host = FALSE;
  }

  /*************************************************************
   * Check whether the port and the "connect to port" are equal.
   * Do this after the remote port number has been fixed in the URL.
   *************************************************************/
  if(conn->bits.conn_to_port && conn->conn_to_port == conn->remote_port) {
    conn->bits.conn_to_port = FALSE;
  }

#ifndef CURL_DISABLE_PROXY
  /*************************************************************
   * If the "connect to" feature is used with an HTTP proxy,
   * we set the tunnel_proxy bit.
   *************************************************************/
  if((conn->bits.conn_to_host || conn->bits.conn_to_port) &&
      conn->bits.httpproxy)
    conn->bits.tunnel_proxy = TRUE;
#endif

  /*************************************************************
   * Setup internals depending on protocol. Needs to be done after
   * we figured out what/if proxy to use.
   *************************************************************/
  result = setup_connection_internals(data, conn);
  if(result)
    goto out;

  /***********************************************************************
   * file: is a special case in that it doesn't need a network connection
   ***********************************************************************/
#ifndef CURL_DISABLE_FILE
  if(conn->handler->flags & PROTOPT_NONETWORK) {
    bool done;
    /* this is supposed to be the connect function so we better at least check
       that the file is present here! */
    DEBUGASSERT(conn->handler->connect_it);
    Curl_persistconninfo(data, conn, NULL, -1);
    result = conn->handler->connect_it(data, &done);

    /* Setup a "faked" transfer that'll do nothing */
    if(!result) {
      Curl_attach_connection(data, conn);
      result = Curl_conncache_add_conn(data);
      if(result)
        goto out;

      /*
       * Setup whatever necessary for a resumed transfer
       */
      result = setup_range(data);
      if(result) {
        DEBUGASSERT(conn->handler->done);
        /* we ignore the return code for the protocol-specific DONE */
        (void)conn->handler->done(data, result, FALSE);
        goto out;
      }
      Curl_setup_transfer(data, -1, -1, FALSE, -1);
    }

    /* since we skip do_init() */
    Curl_init_do(data, conn);

    goto out;
  }
#endif

  /* Setup filter for network connections */
  conn->recv[FIRSTSOCKET] = Curl_conn_recv;
  conn->send[FIRSTSOCKET] = Curl_conn_send;
  conn->recv[SECONDARYSOCKET] = Curl_conn_recv;
  conn->send[SECONDARYSOCKET] = Curl_conn_send;
  conn->bits.tcp_fastopen = data->set.tcp_fastopen;

  /* Get a cloned copy of the SSL config situation stored in the
     connection struct. But to get this going nicely, we must first make
     sure that the strings in the master copy are pointing to the correct
     strings in the session handle strings array!

     Keep in mind that the pointers in the master copy are pointing to strings
     that will be freed as part of the Curl_easy struct, but all cloned
     copies will be separately allocated.
  */
  data->set.ssl.primary.CApath = data->set.str[STRING_SSL_CAPATH];
  data->set.ssl.primary.CAfile = data->set.str[STRING_SSL_CAFILE];
  data->set.ssl.primary.issuercert = data->set.str[STRING_SSL_ISSUERCERT];
  data->set.ssl.primary.issuercert_blob = data->set.blobs[BLOB_SSL_ISSUERCERT];
  data->set.ssl.primary.cipher_list =
    data->set.str[STRING_SSL_CIPHER_LIST];
  data->set.ssl.primary.cipher_list13 =
    data->set.str[STRING_SSL_CIPHER13_LIST];
  data->set.ssl.primary.pinned_key =
    data->set.str[STRING_SSL_PINNEDPUBLICKEY];
  data->set.ssl.primary.cert_blob = data->set.blobs[BLOB_CERT];
  data->set.ssl.primary.ca_info_blob = data->set.blobs[BLOB_CAINFO];
  data->set.ssl.primary.curves = data->set.str[STRING_SSL_EC_CURVES];

#ifndef CURL_DISABLE_PROXY
  data->set.proxy_ssl.primary.CApath = data->set.str[STRING_SSL_CAPATH_PROXY];
  data->set.proxy_ssl.primary.CAfile = data->set.str[STRING_SSL_CAFILE_PROXY];
  data->set.proxy_ssl.primary.cipher_list =
    data->set.str[STRING_SSL_CIPHER_LIST_PROXY];
  data->set.proxy_ssl.primary.cipher_list13 =
    data->set.str[STRING_SSL_CIPHER13_LIST_PROXY];
  data->set.proxy_ssl.primary.pinned_key =
    data->set.str[STRING_SSL_PINNEDPUBLICKEY_PROXY];
  data->set.proxy_ssl.primary.cert_blob = data->set.blobs[BLOB_CERT_PROXY];
  data->set.proxy_ssl.primary.ca_info_blob =
    data->set.blobs[BLOB_CAINFO_PROXY];
  data->set.proxy_ssl.primary.issuercert =
    data->set.str[STRING_SSL_ISSUERCERT_PROXY];
  data->set.proxy_ssl.primary.issuercert_blob =
    data->set.blobs[BLOB_SSL_ISSUERCERT_PROXY];
  data->set.proxy_ssl.primary.CRLfile =
    data->set.str[STRING_SSL_CRLFILE_PROXY];
  data->set.proxy_ssl.cert_type = data->set.str[STRING_CERT_TYPE_PROXY];
  data->set.proxy_ssl.key = data->set.str[STRING_KEY_PROXY];
  data->set.proxy_ssl.key_type = data->set.str[STRING_KEY_TYPE_PROXY];
  data->set.proxy_ssl.key_passwd = data->set.str[STRING_KEY_PASSWD_PROXY];
  data->set.proxy_ssl.primary.clientcert = data->set.str[STRING_CERT_PROXY];
  data->set.proxy_ssl.key_blob = data->set.blobs[BLOB_KEY_PROXY];
#endif
  data->set.ssl.primary.CRLfile = data->set.str[STRING_SSL_CRLFILE];
  data->set.ssl.cert_type = data->set.str[STRING_CERT_TYPE];
  data->set.ssl.key = data->set.str[STRING_KEY];
  data->set.ssl.key_type = data->set.str[STRING_KEY_TYPE];
  data->set.ssl.key_passwd = data->set.str[STRING_KEY_PASSWD];
  data->set.ssl.primary.clientcert = data->set.str[STRING_CERT];
#ifdef USE_TLS_SRP
  data->set.ssl.primary.username = data->set.str[STRING_TLSAUTH_USERNAME];
  data->set.ssl.primary.password = data->set.str[STRING_TLSAUTH_PASSWORD];
#ifndef CURL_DISABLE_PROXY
  data->set.proxy_ssl.primary.username =
    data->set.str[STRING_TLSAUTH_USERNAME_PROXY];
  data->set.proxy_ssl.primary.password =
    data->set.str[STRING_TLSAUTH_PASSWORD_PROXY];
#endif
#endif
  data->set.ssl.key_blob = data->set.blobs[BLOB_KEY];

  if(!Curl_clone_primary_ssl_config(&data->set.ssl.primary,
                                    &conn->ssl_config)) {
    result = CURLE_OUT_OF_MEMORY;
    goto out;
  }

#ifndef CURL_DISABLE_PROXY
  if(!Curl_clone_primary_ssl_config(&data->set.proxy_ssl.primary,
                                    &conn->proxy_ssl_config)) {
    result = CURLE_OUT_OF_MEMORY;
    goto out;
  }
#endif

  prune_dead_connections(data);

  /*************************************************************
   * Check the current list of connections to see if we can
   * reuse an already existing one or if we have to create a
   * new one.
   *************************************************************/

  DEBUGASSERT(conn->user);
  DEBUGASSERT(conn->passwd);

  /* reuse_fresh is TRUE if we are told to use a new connection by force, but
     we only acknowledge this option if this is not a reused connection
     already (which happens due to follow-location or during an HTTP
     authentication phase). CONNECT_ONLY transfers also refuse reuse. */
  if((data->set.reuse_fresh && !data->state.followlocation) ||
     data->set.connect_only)
    reuse = FALSE;
  else
    reuse = ConnectionExists(data, conn, &existing, &force_reuse, &waitpipe);

  if(reuse) {
    /*
     * We already have a connection for this, we got the former connection in
     * `existing` and thus we need to cleanup the one we just
     * allocated before we can move along and use `existing`.
     */
    reuse_conn(data, conn, existing);
    conn = existing;
    *in_connect = conn;

#ifndef CURL_DISABLE_PROXY
    infof(data, "Re-using existing connection with %s %s",
          conn->bits.proxy?"proxy":"host",
          conn->socks_proxy.host.name ? conn->socks_proxy.host.dispname :
          conn->http_proxy.host.name ? conn->http_proxy.host.dispname :
          conn->host.dispname);
#else
    infof(data, "Re-using existing connection with host %s",
          conn->host.dispname);
#endif
  }
  else {
    /* We have decided that we want a new connection. However, we may not
       be able to do that if we have reached the limit of how many
       connections we are allowed to open. */

    if(conn->handler->flags & PROTOPT_ALPN) {
      /* The protocol wants it, so set the bits if enabled in the easy handle
         (default) */
      if(data->set.ssl_enable_alpn)
        conn->bits.tls_enable_alpn = TRUE;
    }

    if(waitpipe)
      /* There is a connection that *might* become usable for multiplexing
         "soon", and we wait for that */
      connections_available = FALSE;
    else {
      /* this gets a lock on the conncache */
      struct connectbundle *bundle =
        Curl_conncache_find_bundle(data, conn, data->state.conn_cache);

      if(max_host_connections > 0 && bundle &&
         (bundle->num_connections >= max_host_connections)) {
        struct connectdata *conn_candidate;

        /* The bundle is full. Extract the oldest connection. */
        conn_candidate = Curl_conncache_extract_bundle(data, bundle);
        CONNCACHE_UNLOCK(data);

        if(conn_candidate)
          Curl_disconnect(data, conn_candidate, FALSE);
        else {
          infof(data, "No more connections allowed to host: %zu",
                max_host_connections);
          connections_available = FALSE;
        }
      }
      else
        CONNCACHE_UNLOCK(data);

    }

    if(connections_available &&
       (max_total_connections > 0) &&
       (Curl_conncache_size(data) >= max_total_connections)) {
      struct connectdata *conn_candidate;

      /* The cache is full. Let's see if we can kill a connection. */
      conn_candidate = Curl_conncache_extract_oldest(data);
      if(conn_candidate)
        Curl_disconnect(data, conn_candidate, FALSE);
      else {
        infof(data, "No connections available in cache");
        connections_available = FALSE;
      }
    }

    if(!connections_available) {
      infof(data, "No connections available.");

      conn_free(data, conn);
      *in_connect = NULL;

      result = CURLE_NO_CONNECTION_AVAILABLE;
      goto out;
    }
    else {
      /*
       * This is a brand new connection, so let's store it in the connection
       * cache of ours!
       */
      Curl_attach_connection(data, conn);
      result = Curl_conncache_add_conn(data);
      if(result)
        goto out;
    }

#if defined(USE_NTLM)
    /* If NTLM is requested in a part of this connection, make sure we don't
       assume the state is fine as this is a fresh connection and NTLM is
       connection based. */
    if((data->state.authhost.picked & (CURLAUTH_NTLM | CURLAUTH_NTLM_WB)) &&
       data->state.authhost.done) {
      infof(data, "NTLM picked AND auth done set, clear picked");
      data->state.authhost.picked = CURLAUTH_NONE;
      data->state.authhost.done = FALSE;
    }

    if((data->state.authproxy.picked & (CURLAUTH_NTLM | CURLAUTH_NTLM_WB)) &&
       data->state.authproxy.done) {
      infof(data, "NTLM-proxy picked AND auth done set, clear picked");
      data->state.authproxy.picked = CURLAUTH_NONE;
      data->state.authproxy.done = FALSE;
    }
#endif
  }

  /* Setup and init stuff before DO starts, in preparing for the transfer. */
  Curl_init_do(data, conn);

  /*
   * Setup whatever necessary for a resumed transfer
   */
  result = setup_range(data);
  if(result)
    goto out;

  /* Continue connectdata initialization here. */

  /*
   * Inherit the proper values from the urldata struct AFTER we have arranged
   * the persistent connection stuff
   */
  conn->seek_func = data->set.seek_func;
  conn->seek_client = data->set.seek_client;

  /*************************************************************
   * Resolve the address of the server or proxy
   *************************************************************/
  result = resolve_server(data, conn, async);
  if(result)
    goto out;

  /* Everything general done, inform filters that they need
   * to prepare for a data transfer.
   */
  result = Curl_conn_ev_data_setup(data);

out:
  return result;
}

/* Curl_setup_conn() is called after the name resolve initiated in
 * create_conn() is all done.
 *
 * Curl_setup_conn() also handles reused connections
 */
CURLcode Curl_setup_conn(struct Curl_easy *data,
                         bool *protocol_done)
{
  CURLcode result = CURLE_OK;
  struct connectdata *conn = data->conn;

  Curl_pgrsTime(data, TIMER_NAMELOOKUP);

  if(conn->handler->flags & PROTOPT_NONETWORK) {
    /* nothing to setup when not using a network */
    *protocol_done = TRUE;
    return result;
  }

#ifndef CURL_DISABLE_PROXY
  /* set proxy_connect_closed to false unconditionally already here since it
     is used strictly to provide extra information to a parent function in the
     case of proxy CONNECT failures and we must make sure we don't have it
     lingering set from a previous invoke */
  conn->bits.proxy_connect_closed = FALSE;
#endif

#ifdef CURL_DO_LINEEND_CONV
  data->state.crlf_conversions = 0; /* reset CRLF conversion counter */
#endif /* CURL_DO_LINEEND_CONV */

  /* set start time here for timeout purposes in the connect procedure, it
     is later set again for the progress meter purpose */
  conn->now = Curl_now();
  if(!conn->bits.reuse)
    result = Curl_conn_setup(data, conn, FIRSTSOCKET, conn->dns_entry,
                             CURL_CF_SSL_DEFAULT);
  /* not sure we need this flag to be passed around any more */
  *protocol_done = FALSE;
  return result;
}

CURLcode Curl_connect(struct Curl_easy *data,
                      bool *asyncp,
                      bool *protocol_done)
{
  CURLcode result;
  struct connectdata *conn;

  *asyncp = FALSE; /* assume synchronous resolves by default */

  /* init the single-transfer specific data */
  Curl_free_request_state(data);
  memset(&data->req, 0, sizeof(struct SingleRequest));
  data->req.size = data->req.maxdownload = -1;
  data->req.no_body = data->set.opt_no_body;

  /* call the stuff that needs to be called */
  result = create_conn(data, &conn, asyncp);

  if(!result) {
    if(CONN_INUSE(conn) > 1)
      /* multiplexed */
      *protocol_done = TRUE;
    else if(!*asyncp) {
      /* DNS resolution is done: that's either because this is a reused
         connection, in which case DNS was unnecessary, or because DNS
         really did finish already (synch resolver/fast async resolve) */
      result = Curl_setup_conn(data, protocol_done);
    }
  }

  if(result == CURLE_NO_CONNECTION_AVAILABLE) {
    return result;
  }
  else if(result && conn) {
    /* We're not allowed to return failure with memory left allocated in the
       connectdata struct, free those here */
    Curl_detach_connection(data);
    Curl_conncache_remove_conn(data, conn, TRUE);
    Curl_disconnect(data, conn, TRUE);
  }

  return result;
}

/*
 * Curl_init_do() inits the readwrite session. This is inited each time (in
 * the DO function before the protocol-specific DO functions are invoked) for
 * a transfer, sometimes multiple times on the same Curl_easy. Make sure
 * nothing in here depends on stuff that are setup dynamically for the
 * transfer.
 *
 * Allow this function to get called with 'conn' set to NULL.
 */

CURLcode Curl_init_do(struct Curl_easy *data, struct connectdata *conn)
{
  struct SingleRequest *k = &data->req;

  /* if this is a pushed stream, we need this: */
  CURLcode result = Curl_preconnect(data);
  if(result)
    return result;

  if(conn) {
    conn->bits.do_more = FALSE; /* by default there's no curl_do_more() to
                                   use */
    /* if the protocol used doesn't support wildcards, switch it off */
    if(data->state.wildcardmatch &&
       !(conn->handler->flags & PROTOPT_WILDCARD))
      data->state.wildcardmatch = FALSE;
  }

  data->state.done = FALSE; /* *_done() is not called yet */
  data->state.expect100header = FALSE;

  if(data->req.no_body)
    /* in HTTP lingo, no body means using the HEAD request... */
    data->state.httpreq = HTTPREQ_HEAD;

  k->start = Curl_now(); /* start time */
  k->header = TRUE; /* assume header */
  k->bytecount = 0;
  k->ignorebody = FALSE;

  Curl_speedinit(data);
  Curl_pgrsSetUploadCounter(data, 0);
  Curl_pgrsSetDownloadCounter(data, 0);

  return CURLE_OK;
}

#if defined(USE_HTTP2) || defined(USE_HTTP3)

#ifdef USE_NGHTTP2

static void priority_remove_child(struct Curl_easy *parent,
                                  struct Curl_easy *child)
{
  struct Curl_data_prio_node **pnext = &parent->set.priority.children;
  struct Curl_data_prio_node *pnode = parent->set.priority.children;

  DEBUGASSERT(child->set.priority.parent == parent);
  while(pnode && pnode->data != child) {
    pnext = &pnode->next;
    pnode = pnode->next;
  }

  DEBUGASSERT(pnode);
  if(pnode) {
    *pnext = pnode->next;
    free(pnode);
  }

  child->set.priority.parent = 0;
  child->set.priority.exclusive = FALSE;
}

CURLcode Curl_data_priority_add_child(struct Curl_easy *parent,
                                      struct Curl_easy *child,
                                      bool exclusive)
{
  if(child->set.priority.parent) {
    priority_remove_child(child->set.priority.parent, child);
  }

  if(parent) {
    struct Curl_data_prio_node **tail;
    struct Curl_data_prio_node *pnode;

    pnode = calloc(1, sizeof(*pnode));
    if(!pnode)
      return CURLE_OUT_OF_MEMORY;
    pnode->data = child;

    if(parent->set.priority.children && exclusive) {
      /* exclusive: move all existing children underneath the new child */
      struct Curl_data_prio_node *node = parent->set.priority.children;
      while(node) {
        node->data->set.priority.parent = child;
        node = node->next;
      }

      tail = &child->set.priority.children;
      while(*tail)
        tail = &(*tail)->next;

      DEBUGASSERT(!*tail);
      *tail = parent->set.priority.children;
      parent->set.priority.children = 0;
    }

    tail = &parent->set.priority.children;
    while(*tail) {
      (*tail)->data->set.priority.exclusive = FALSE;
      tail = &(*tail)->next;
    }

    DEBUGASSERT(!*tail);
    *tail = pnode;
  }

  child->set.priority.parent = parent;
  child->set.priority.exclusive = exclusive;
  return CURLE_OK;
}

#endif /* USE_NGHTTP2 */

#ifdef USE_NGHTTP2
static void data_priority_cleanup(struct Curl_easy *data)
{
  while(data->set.priority.children) {
    struct Curl_easy *tmp = data->set.priority.children->data;
    priority_remove_child(data, tmp);
    if(data->set.priority.parent)
      Curl_data_priority_add_child(data->set.priority.parent, tmp, FALSE);
  }

  if(data->set.priority.parent)
    priority_remove_child(data->set.priority.parent, data);
}
#endif

void Curl_data_priority_clear_state(struct Curl_easy *data)
{
  memset(&data->state.priority, 0, sizeof(data->state.priority));
}

#endif /* defined(USE_HTTP2) || defined(USE_HTTP3) */
