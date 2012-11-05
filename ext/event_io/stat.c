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
#ifdef _WIN32
#include <windows.h>
#include <ddk/ntifs.h>
#endif

#include "event_io.h"
#include "watcher.h"

static VALUE mEventIO = Qnil;
static VALUE cEventIO_Watcher = Qnil;
static VALUE cEventIO_StatWatcher= Qnil;
static VALUE cEventIO_Loop = Qnil;

static VALUE EventIO_StatWatcher_initialize(int argc, VALUE *argv, VALUE self);
static VALUE EventIO_StatWatcher_attach(VALUE self, VALUE loop);
static void EventIO_StatWatcher_libuv_callback(uv_fs_event_t* handle, const char* filename, int events, int status);
static void EventIO_StatWatcher_dispatch_callback(VALUE self, int revents);

static VALUE EventIO_StatWatcher_file_id(VALUE self, VALUE path);

/*
 * EventIO::StatWatcher lets you create either one-shot or periodic stats which
 * run within EventIO's event loop.  It's useful for creating timeouts or
 * events which fire periodically.
 */
void Init_eventio_stat_watcher()
{
  mEventIO             = rb_define_module_under(rb_define_module("Fluent"), "EventIO");
  cEventIO_Watcher     = rb_define_class_under(mEventIO, "Watcher"    , rb_cObject);
  cEventIO_StatWatcher = rb_define_class_under(mEventIO, "StatWatcher", cEventIO_Watcher);
  cEventIO_Loop        = rb_define_class_under(mEventIO, "Loop"       , rb_cObject);

  rb_define_method(cEventIO_StatWatcher, "initialize", EventIO_StatWatcher_initialize, -1);
  rb_define_method(cEventIO_StatWatcher, "attach", EventIO_StatWatcher_attach, 1);
  rb_define_singleton_method(cEventIO_StatWatcher, "file_id", EventIO_StatWatcher_file_id, 1);
}

/**
 *  call-seq:
 *    EventIO::StatWatcher.initialize(interval, repeating = false) -> EventIO::StatWatcher
 *
 * Create a new EventIO::StatWatcher for the given IO object and add it to the
 * given EventIO::Loop.  Interval defines a duration in seconds to wait for events,
 * and can be specified as an Integer or Float.  Repeating is a boolean
 * indicating whether the stat is one shot or should fire on the given
 * interval.
 */
static VALUE EventIO_StatWatcher_initialize(int argc, VALUE *argv, VALUE self)
{
  VALUE path;

  rb_scan_args(argc, argv, "1", &path);
  rb_iv_set(self, "@path", path);
  return Qnil;
}

/**
 *  call-seq:
 *    EventIO::StatWatcher.attach(loop) -> EventIO::StatWatcher
 *
 * Attach the stat watcher to the given EventIO::Loop.  If the watcher is already
 * attached to a loop, detach it from the old one and attach it to the new one.
 */
static VALUE EventIO_StatWatcher_attach(VALUE self, VALUE loop)
{
  Watcher_Setup(StatWatcher);
  watcher_data->handle = (uv_handle_t*)xmalloc(sizeof(uv_fs_event_t));

  watcher_data->handle->data = (void *)self;
  CHECK("uv_fs_event_init",
        uv_fs_event_init(loop_data->uv_loop,
                         (uv_fs_event_t*)watcher_data->handle,
                         RSTRING_PTR(rb_iv_get(self, "@path")),
                         EventIO_StatWatcher_libuv_callback,
                         0));

  rb_call_super(1, &loop);
  return self;
}

/* libuv callback */
static void EventIO_StatWatcher_libuv_callback(uv_fs_event_t* handle, const char* filename, int events, int status)
{
  VALUE self = (VALUE)handle->data;
  // filename might be null at Darwin
  rb_iv_set(self, "@_filename",
            filename ? rb_str_new2(filename) : rb_iv_get(self, "@path"));
  EventIO_Loop_process_event(self, events);
}

/* EventIO::Loop dispatch callback */
static void EventIO_StatWatcher_dispatch_callback(VALUE self, int events)
{
  rb_funcall(self, rb_intern("on_stat"),
             2, rb_iv_get(self,"@_filename"), INT2FIX(events));
}

#ifdef _WIN32
/**
 *  call-seq:
 *    EventIO::StatWatcher.file_id(path) -> Number
 *
 * Return unique file id like inode, but works on Windows, too.
 */
static VALUE EventIO_StatWatcher_file_id(VALUE self, VALUE file) {
  VALUE io = rb_check_convert_type(file, T_FILE, "IO", "to_io");
  rb_io_t *fptr;
  GetOpenFile(io, fptr);

  HANDLE handle = rb_w32_get_osfhandle(fptr->fd);
  FILE_FS_OBJECT_ID_INFORMATION buffer;
  BOOL ret = DeviceIoControl((HANDLE) handle,
                             FSCTL_CREATE_OR_GET_OBJECT_ID,
                             NULL,
                             0,
                             (LPVOID)&buffer,
                             sizeof(FILE_FS_OBJECT_ID_INFORMATION),
                             NULL, NULL);
  if(ret) {
    VALUE id = INT2NUM(0);
    for(int i = 0; i < 16; i++){
      id = rb_funcall(id, rb_intern("<<"), 1, INT2FIX(8));
      id = rb_funcall(id, rb_intern("+"), 1, INT2FIX(buffer.ObjectId[15 - i]));
    }
    return id;
  }else{
    rb_raise(rb_eRuntimeError, "cannot get object id");
    return Qnil;
  }
}
#else
static VALUE EventIO_StatWatcher_file_id(VALUE self, VALUE file) {
  VALUE fstat  = rb_funcall(file, rb_intern("stat"), 0, 0);
  return rb_funcall(fstat, rb_intern("ino"), 0, 0);
}
#endif
