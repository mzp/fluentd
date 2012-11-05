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

static VALUE mEventIO = Qnil;
static VALUE cEventIO_Watcher = Qnil;
static VALUE cEventIO_Udp = Qnil;
static VALUE cEventIO_Loop = Qnil;

static VALUE EventIO_Udp_attach(VALUE self, VALUE loop);
static VALUE EventIO_Udp_send(VALUE self, VALUE mesg, VALUE flags, VALUE host, VALUE port);
static void EventIO_Udp_dispatch_callback(VALUE self, int revents);
static void EventIO_Udp_libuv_callback(uv_udp_t* handle, ssize_t nread, uv_buf_t buf,
                                       struct sockaddr* addr, unsigned flags);

enum {
  ON_READ,
  ON_SEND
};

/*
 * EventIO::UDPSocket lets you create UDPServer which
 * run within EventIO's event loop.
 */
void Init_eventio_udp() {
  mEventIO = rb_define_module_under(rb_define_module("Fluent"), "EventIO");
  cEventIO_Watcher = rb_define_class_under(mEventIO, "Watcher"  , rb_cObject);
  cEventIO_Udp     = rb_define_class_under(mEventIO, "UDPSocket", cEventIO_Watcher);
  cEventIO_Loop    = rb_define_class_under(mEventIO, "Loop"     , rb_cObject);

  rb_define_method(cEventIO_Udp, "attach", EventIO_Udp_attach, 1);
  rb_define_method(cEventIO_Udp, "send"  , EventIO_Udp_send  , 4);
}

/**
 *  call-seq:
 *    EventIO::UDPSocket.attach(loop) -> EventIO::UDPSocket
 *
 * Attach the udp socket to the given EventIO::Loop.  If the watcher is already
 * attached to a loop, detach it from the old one and attach it to the new one.
 */
static VALUE EventIO_Udp_attach(VALUE self, VALUE loop) {
  Watcher_Setup(Udp);

  watcher_data->handle = (uv_handle_t*)xmalloc(sizeof(uv_udp_t));
  CHECK("uv_udp_init",
        uv_udp_init(loop_data->uv_loop, (uv_udp_t*)watcher_data->handle));
  watcher_data->handle->data = (void *)self;

  if(rb_iv_get(self, "@host") != Qnil) {
    CHECK("uv_udp_bind",
          uv_udp_bind((uv_udp_t*)watcher_data->handle,
                      uv_ip4_addr(RSTRING_PTR(rb_iv_get(self, "@host")),
                                  NUM2INT(rb_iv_get(self, "@port"))),
                      0));
  }
  rb_iv_set(self, "@buf", rb_str_new(0,0));
  CHECK("uv_udp_recv_start",
        uv_udp_recv_start((uv_udp_t*)watcher_data->handle,
                          EventIO_alloc,
                          EventIO_Udp_libuv_callback));
  return rb_call_super(1, &loop);
}

static VALUE rsock_ipaddr(struct sockaddr *sockaddr, int norevlookup)
{
    VALUE port, addr1, addr2;
    VALUE ary;
    int error;
    char hbuf[1024], pbuf[1024];

    addr1 = Qnil;
    error = getnameinfo(sockaddr, sizeof(struct sockaddr), hbuf, sizeof(hbuf),
                        NULL, 0, 0);
    if (! error) {
      addr1 = rb_str_new2(hbuf);
    }
    error = getnameinfo(sockaddr, sizeof(struct sockaddr), hbuf, sizeof(hbuf),
                        pbuf, sizeof(pbuf), NI_NUMERICHOST | NI_NUMERICSERV);

    addr2 = rb_str_new2(hbuf);
    if (addr1 == Qnil) {
        addr1 = addr2;
    }
    port = INT2FIX(atoi(pbuf));
    ary = rb_ary_new3(4, Qnil, port, addr1, addr2);

    return ary;
}

/* libuv callback */
static void EventIO_Udp_libuv_callback(uv_udp_t* handle, ssize_t nread, uv_buf_t buf,
                                       struct sockaddr* addr, unsigned flags) {
  VALUE self = (VALUE)handle->data;
  if(nread == -1) {
    CHECK("uv_udp_stop", uv_udp_recv_stop(handle));
    rb_iv_set(self, "@buf", Qnil);
  } else {
    if( rb_iv_get(self, "@buf") == Qnil ) {
      rb_iv_set(self, "@buf", rb_str_new(0,0));
    }
    VALUE buffer = rb_str_new(buf.base, nread);
    rb_funcall(rb_iv_get(self, "@buf"), rb_intern("<<"), 1, buffer);
    rb_iv_set(self, "@addr", rsock_ipaddr(addr, 1));
    xfree(buf.base);
  }
  EventIO_Loop_process_event(self, 0);
}

/* EventIO::Loop dispatch callback */
static void EventIO_Udp_dispatch_callback(VALUE self, int status) {
  if(status == ON_READ) {
    VALUE buf = rb_iv_get(self, "@buf");
    VALUE addr = rb_iv_get(self, "@addr");
    if(buf == Qnil) {
      rb_funcall(self, rb_intern("on_close"), 0, 0);
    } else {
      rb_funcall(self, rb_intern("on_read"), 2, buf, addr);
    }
    rb_iv_set(self, "@buf", Qnil);
  } else if(status = ON_SEND) {
    rb_funcall(self, rb_intern("on_send_complete"), 0, 0);
  } else {
    rb_raise(rb_eRuntimeError, "unknown revents value for uv_udp: %d", status);
  }
}

static void EventIO_Socket_libuv_write_callback(uv_udp_send_t* req, int status){
  VALUE self = (VALUE)req->handle->data;
  EventIO_Loop_process_event(self, ON_SEND);
}

/**
 *  call-seq:
 *    EventIO::UDPSocket#send -> nil
 *
 * Send data via this UDPSocket.
 */
static VALUE EventIO_Udp_send(VALUE self, VALUE mesg, VALUE flags, VALUE host, VALUE port){
  struct EventIO_Watcher *watcher_data;
  Data_Get_Struct(self, struct EventIO_Watcher, watcher_data);

  uv_udp_send_t* req = xmalloc(sizeof(uv_udp_send_t));
  uv_buf_t bufs[] = {
    uv_buf_init(StringValuePtr(mesg), (int)rb_str_strlen(mesg))
  };

  CHECK("uv_udp_send",
        uv_udp_send(req, (uv_udp_t*)watcher_data->handle, bufs, 1,
          uv_ip4_addr(StringValueCStr(host), NUM2INT(port)),
          EventIO_Socket_libuv_write_callback));
  return Qnil;
}
