/*
 * libev-based HTTP server implementation
 * Copyright (C) 2014  Seong-Kook Shin <cinsky@gmail.com>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */
#ifndef EVHTTP_H__
#define EVHTTP_H__

#include <netinet/in.h>
#include <ev.h>

#include <pthread.h>

#ifdef EVHTTP_HANDLE_FORM
#include <pcre.h>
#endif

#if 0
#include "xobstack.h"
#include "hdrstore.h"
#include "buffer.h"
#endif  /* 0 */

#include "evhttpconn.h"
#include "patable.h"

/* This indirect using of extern "C" { ... } makes Emacs happy */
#ifndef BEGIN_C_DECLS
# ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
# else
#  define BEGIN_C_DECLS
#  define END_C_DECLS
# endif
#endif /* BEGIN_C_DECLS */

BEGIN_C_DECLS

/*
 * When REVENTS contains EV_TIMER, it means that timeout occurred.  if
 * the user returns zero, it means that user wants to let HTTP module
 * release the connection.  If not (nonzero), HTTP module will ignore
 * the timeout event just for this time.  Note that EOB is undefined
 * on EV_TIMER.
 *
 * When REVENTS contains EV_READ, it means that a complete HTTP
 * request is ready to serve.  The user need to fill w->rsp_code with
 * the status code, w->rsp_hdrs with response headers, and w->obuf
 * with the response body.  If the user cannot provide the complete
 * response body, then the user need to fill it as much as he/she can,
 * then return from the callback with nonzero value.  If the callback
 * returns with zero, it means that the response body is completed.
 *
 *   If the user provide the complete response body when the callback
 *   was called at the first time, evhttpserv module will set
 *   CONTENT-LENGTH header for you if it was not set.  Otherwise, the
 *   user should provide CONTENT-LENGTH in the callback.
 *
 * How to receive the response body:
 *
 *   If EOB is nonzero, then the whole request body is stored in
 *   w->ibuf.  If EOB is zero, then the partial request body is stored
 *   in w->ibuf.  In either case, the callback must read(consume) the
 *   contents in w->ibuf.  The callback need to just read the
 *   contents, since the buffer contents in w->ibuf will be
 *   automatically released.  If w->body_chnk is zero, it means that
 *   the response body is being transfered in identity encoding (as
 *   is, no transformation).
 *
 *   You may read w->body_chnk to determine whether the chunked
 *   transfer encoding is in effect.  Although I don't think that the
 *   callback need it.  Currently, as the callback's point of view,
 *   w->body_size and w->body_rest may be useless, since they will be
 *   updated after the callback is called.
 */
typedef int (*http_callback)(struct ev_loop *loop, struct ev_httpconn *w,
                             int eob,
                             int revents,
                             int argc,
                             const char *argv[]);

/*
  struct ev_httpconn {
    ev_io read;      // watcher for reading
    ev_io write;     // watcher for writing
    ...
  };

  struct ev_httpconn {
    ev_io io;        // watcher for reading/writing at the same time
    ...
  };

  // this will be my choice.
  struct ev_httpconn {
    ev_io io;        // watcher for reading/writing
                     // start/stop for reading, then start/stop for writing
    ...
  };
 */

struct ev_http;


struct httpworker {
  int fd;
  pthread_t tid;
};

struct ev_http {
  ev_io io;
  ev_io ctrl;
  ev_idle idle;

  int quit;                     /* When 'QUIT' control request
                                 * received, QUIT will be nonzero, and
                                 * IO will be stoped, and its fd will
                                 * be closed.  In other words, IO is
                                 * in invalid state when QUIT is
                                 * nonzero. */
  size_t nclients;
  http_callback cb;

  char address[INET_ADDRSTRLEN];
  int port;
  int ctrlport;

  int obufsize;
  int ibufsize;

#ifdef EVHTTP_HANDLE_FORM
  pcre *boundary;
  pcre_extra *boundary_ext;
#endif

  struct patable dispatcher;

  struct httpworker *workers;
  size_t nworkers;
};
typedef struct ev_http ev_http;

int ev_http_init(ev_http *http, size_t nworkers, http_callback cb, char *address,
                 int port, int ctrl_port);

int ev_http_set_site(ev_http *http, const char *site_root);
int ev_http_dispatcher_add(ev_http *http, const char *pattern, http_callback cb);

void ev_http_start(struct ev_loop *loop, ev_http *http);
void ev_http_break(struct ev_loop *loop, ev_http *http);
void ev_http_stop(struct ev_loop *loop, ev_http *http);

END_C_DECLS

#endif /* EVHTTP_H__ */
