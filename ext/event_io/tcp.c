/*
 *
 * Fluent
 *
 * Copyright (C) 2011 FURUHASHI Sadayuki
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */
#include "event_io.h"
#include "watcher.h"
#include "socket.h"

static const int EVENTIO_BACKLOG = 1024;

static VALUE mEventIO = Qnil;
static VALUE cEventIO_Watcher = Qnil;
static VALUE cEventIO_Tcp = Qnil;
static VALUE cEventIO_Socket = Qnil;
static VALUE cEventIO_Loop = Qnil;

static VALUE EventIO_Tcp_attach(VALUE self, VALUE loop);
static void EventIO_Tcp_dispatch_callback(VALUE self, int revents);
static void EventIO_Tcp_libuv_callback(uv_stream_t* server, int status);

/*
 * EventIO::TCPServer lets you create TCPServer which
 * run within EventIO's event loop.
 */
void Init_eventio_tcp() {
  mEventIO         = rb_define_module_under(rb_define_module("Fluent"), "EventIO");
  cEventIO_Watcher = rb_define_class_under(mEventIO, "Watcher"  , rb_cObject);
  cEventIO_Tcp     = rb_define_class_under(mEventIO, "TCPServer", cEventIO_Watcher);
  cEventIO_Socket  = rb_define_class_under(mEventIO, "Socket"   , cEventIO_Watcher);
  cEventIO_Loop    = rb_define_class_under(mEventIO, "Loop"     , rb_cObject);

  rb_define_method(cEventIO_Tcp, "attach", EventIO_Tcp_attach, 1);
}

/**
 *  call-seq:
 *    EventIO::TCPServer.attach(loop) -> EventIO::TCPServer
 *
 * Attach the TCP server to the given EventIO::Loop.  If the watcher is already
 * attached to a loop, detach it from the old one and attach it to the new one.
 */
static VALUE EventIO_Tcp_attach(VALUE self, VALUE loop) {
  Watcher_Setup(Tcp);

  watcher_data->handle = (uv_handle_t*)xmalloc(sizeof(uv_tcp_t));
  CHECK("uv_tcp_init", uv_tcp_init(loop_data->uv_loop, (uv_tcp_t*)watcher_data->handle));

  watcher_data->handle->data = (void *)self;

  const char* host = RSTRING_PTR(rb_iv_get(self, "@host"));
  const int   port = NUM2INT(rb_iv_get(self, "@port"));
  CHECK("uv_tcp_bind", uv_tcp_bind(
        (uv_tcp_t*)watcher_data->handle, uv_ip4_addr(host, port)));

  CHECK("uv_listen", uv_listen(
        (uv_stream_t*)watcher_data->handle,
        EVENTIO_BACKLOG,
        EventIO_Tcp_libuv_callback));

  return rb_call_super(1, &loop);
}

static void EventIO_Tcp_libuv_callback(uv_stream_t* server, int status){
  EventIO_Loop_process_event((VALUE)server->data, status);
}

/* EventIO::Loop dispatch callback */
static void EventIO_Tcp_dispatch_callback(VALUE self, int status) {
  if(status == 0) {
    uv_tcp_t* handle = (uv_tcp_t*)xmalloc(sizeof(uv_tcp_t));
    uv_tcp_init(get_uv_loop(self), handle);
    uv_accept((uv_stream_t*)get_handle(self), (uv_stream_t*)handle);

    VALUE client = EventIO_Socket_create(
        (uv_handle_t*)handle,
        rb_iv_get(self,"@klass"),
        rb_iv_get(self,"@args"));
    rb_funcall(self, rb_intern("on_connection"), 1, client);
  } else {
    rb_raise(rb_eRuntimeError, "unknown revents value for uv_tcp: %d", status);
  }
}
