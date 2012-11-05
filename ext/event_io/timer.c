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
static VALUE cEventIO_TimerWatcher= Qnil;
static VALUE cEventIO_Loop = Qnil;

static VALUE EventIO_TimerWatcher_initialize(int argc, VALUE *argv, VALUE self);
static VALUE EventIO_TimerWatcher_attach(VALUE self, VALUE loop);
static VALUE EventIO_TimerWatcher_on_timer(VALUE self);

static void EventIO_TimerWatcher_libuv_callback(uv_timer_t* handle, int status);
static void EventIO_TimerWatcher_dispatch_callback(VALUE self, int revents);

/*
 * EventIO::TimerWatcher lets you create either one-shot or periodic timers which
 * run within EventIO's event loop.  It's useful for creating timeouts or
 * events which fire periodically.
 */
void Init_eventio_timer_watcher()
{
  mEventIO = rb_define_module_under(rb_define_module("Fluent"), "EventIO");
  cEventIO_Watcher = rb_define_class_under(mEventIO, "Watcher", rb_cObject);
  cEventIO_TimerWatcher = rb_define_class_under(mEventIO, "TimerWatcher", cEventIO_Watcher);
  cEventIO_Loop = rb_define_class_under(mEventIO, "Loop", rb_cObject);

  rb_define_method(cEventIO_TimerWatcher, "initialize", EventIO_TimerWatcher_initialize, -1);
  rb_define_method(cEventIO_TimerWatcher, "attach", EventIO_TimerWatcher_attach, 1);
  rb_define_method(cEventIO_TimerWatcher, "on_timer", EventIO_TimerWatcher_on_timer, 0);
}

/**
 *  call-seq:
 *    EventIO::TimerWatcher.initialize(interval, repeating = false) -> EventIO::TimerWatcher
 *
 * Create a new EventIO::TimerWatcher for the given IO object and add it to the
 * given EventIO::Loop.  Interval defines a duration in seconds to wait for events,
 * and can be specified as an Integer or Float.  Repeating is a boolean
 * indicating whether the timer is one shot or should fire on the given
 * interval.
 */
static VALUE EventIO_TimerWatcher_initialize(int argc, VALUE *argv, VALUE self)
{
  VALUE interval, repeating;;

  rb_scan_args(argc, argv, "11", &interval,&repeating);
  interval = rb_convert_type(interval, T_FLOAT, "Float", "to_f");

  rb_iv_set(self, "@interval", interval);
  rb_iv_set(self, "@repeating", repeating);

  return Qnil;
}

/**
 *  call-seq:
 *    EventIO::TimerWatcher.attach(loop) -> EventIO::TimerWatcher
 *
 * Attach the timer watcher to the given EventIO::Loop.  If the watcher is already
 * attached to a loop, detach it from the old one and attach it to the new one.
 */
static VALUE EventIO_TimerWatcher_attach(VALUE self, VALUE loop)
{
  Watcher_Setup(TimerWatcher);

  watcher_data->handle = (uv_handle_t*)xmalloc(sizeof(uv_timer_t));
  CHECK("uv_timer_init",
        uv_timer_init(loop_data->uv_loop, (uv_timer_t*)watcher_data->handle));
  watcher_data->handle->data = (void *)self;

  int64_t interval = NUM2LONG(rb_iv_get(self, "@interval")) * 1000;
  VALUE repeating = rb_iv_get(self, "@repeating");
  CHECK("uv_timer_start",
        uv_timer_start((uv_timer_t*)watcher_data->handle,
                       EventIO_TimerWatcher_libuv_callback,
                       interval,
                       repeating == Qtrue ? 1 : 0));
  rb_call_super(1, &loop);
  return self;
}


/**
 *  call-seq:
 *    EventIO::TimerWatcher#on_timer -> nil
 *
 * Called whenever the TimerWatcher fires
 */
static VALUE EventIO_TimerWatcher_on_timer(VALUE self)
{
  return Qnil;
}

/* libuv callback */
static void EventIO_TimerWatcher_libuv_callback(uv_timer_t* timer, int status)
{
  EventIO_Loop_process_event((VALUE)timer->data, status);
}

/* EventIO::Loop dispatch callback */
static void EventIO_TimerWatcher_dispatch_callback(VALUE self, int status)
{
  if(status == 0)
    rb_funcall(self, rb_intern("on_timer"), 0, 0);
  else
    rb_raise(rb_eRuntimeError, "unknown revents value for ev_timer: %d", status);
}
