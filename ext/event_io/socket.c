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
static VALUE cEventIO_Socket = Qnil;
static VALUE cEventIO_Loop = Qnil;

static VALUE EventIO_Socket_attach(VALUE self, VALUE loop);
static VALUE EventIO_Socket_write(VALUE self, VALUE buffer);
static void EventIO_Socket_dispatch_callback(VALUE self, int revents);
static void EventIO_Socket_libuv_read_callback(uv_stream_t* stream, ssize_t nread, uv_buf_t buf);

static void EventIO_Socket_libuv_write_callback(uv_write_t* req, int status);

enum {
  ON_READ,
  ON_WRITE
};

/*
 * EventIO::Socket is handler class. It is useful to use with TCPServer or UNIXServer.
 * */
void Init_eventio_socket() {
  mEventIO         = rb_define_module_under(rb_define_module("Fluent"), "EventIO");
  cEventIO_Watcher = rb_define_class_under(mEventIO, "Watcher", rb_cObject);
  cEventIO_Socket  = rb_define_class_under(mEventIO, "Socket" , cEventIO_Watcher);
  cEventIO_Loop    = rb_define_class_under(mEventIO, "Loop"   , rb_cObject);

  rb_define_method(cEventIO_Socket, "attach", EventIO_Socket_attach, 1);
  rb_define_method(cEventIO_Socket, "write", EventIO_Socket_write  , 1);
}

VALUE EventIO_Socket_create(uv_handle_t* handle, VALUE klass, VALUE args) {
  struct EventIO_Watcher *watcher_data;
  VALUE obj =
    rb_apply(klass, rb_intern("new"), args);

  Data_Get_Struct(obj, struct EventIO_Watcher, watcher_data);
  watcher_data->handle = (uv_handle_t*)handle;
  return obj;
}

// do not use direct. internal usage only.
static VALUE EventIO_Socket_attach(VALUE self, VALUE loop){
  Watcher_Setup(Socket);
  watcher_data->handle->data = (void *)self;
  CHECK("uv_read_start", uv_read_start(
        (uv_stream_t*)watcher_data->handle,
        EventIO_alloc,
        EventIO_Socket_libuv_read_callback));

  return rb_call_super(1, &loop);
}

static void EventIO_Socket_libuv_read_callback(uv_stream_t* stream, ssize_t nread, uv_buf_t buf){
  VALUE self = (VALUE)stream->data;
  if(nread == -1) {
    CHECK("uv_read_stop", uv_read_stop(stream));
    rb_iv_set(self, "@buf", Qnil);
  } else {
    VALUE buffer = rb_str_new(buf.base, nread);
    rb_iv_set(self, "@buf", buffer);
    xfree(buf.base);
  }
  EventIO_Loop_process_event(self, ON_READ);
}

static void EventIO_Socket_dispatch_callback(VALUE self, int revents){
  if(revents == ON_READ) {
    VALUE buf = rb_iv_get(self, "@buf");
    if(buf == Qnil) {
      rb_funcall(self, rb_intern("on_close"), 0, 0);
      rb_funcall(self, rb_intern("detach"),0, 0);
    } else {
      rb_funcall(self, rb_intern("on_read"), 1, buf);
    }
  } else if(revents == ON_WRITE) {
    rb_funcall(self, rb_intern("on_write_complete"), 0, 0);
  }
}

static void EventIO_Socket_libuv_write_callback(uv_write_t* req, int status){
  VALUE self = (VALUE)req->handle->data;
  EventIO_Loop_process_event(self, ON_WRITE);
}

static VALUE EventIO_Socket_write(VALUE self, VALUE buffer){
  struct EventIO_Watcher *watcher_data;
  Data_Get_Struct(self, struct EventIO_Watcher, watcher_data);

  uv_write_t* req = xmalloc(sizeof(uv_write_t));
  uv_buf_t bufs[] = {
    uv_buf_init(StringValueCStr(buffer), (int)rb_str_strlen(buffer))
  };

  CHECK("uv_write", uv_write(req, (uv_stream_t*)watcher_data->handle, bufs, 1,
			     EventIO_Socket_libuv_write_callback));
  return Qnil;
}
