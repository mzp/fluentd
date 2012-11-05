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

static VALUE mEventIO         = Qnil;
static VALUE cEventIO_Watcher = Qnil;

static VALUE EventIO_Watcher_allocate(VALUE klass);
static void EventIO_Watcher_mark(struct EventIO_Watcher *watcher);
static void EventIO_Watcher_free(struct EventIO_Watcher *watcher);

static VALUE EventIO_Watcher_initialize(VALUE self);
static VALUE EventIO_Watcher_attach(VALUE self, VALUE loop);
static VALUE EventIO_Watcher_detach(VALUE self);
static VALUE EventIO_Watcher_evloop(VALUE self);
static VALUE EventIO_Watcher_attached(VALUE self);

/*
 * Watchers are EventIO's event observers.  They contain a set of callback
 * methods prefixed by on_* which fire whenever events occur.
 *
 * In order for a watcher to fire events it must be attached to a running
 * loop.  Every watcher has an attach and detach method to control which
 * loop it's associated with.
 *
 * Watchers also have an enable and disable method.  This allows a watcher
 * to temporarily ignore certain events while remaining attached to a given
 * loop.  This is good for watchers which need to be toggled on and off.
 */
void Init_eventio_watcher()
{
  mEventIO = rb_define_module_under(rb_define_module("Fluent"), "EventIO");

  cEventIO_Watcher = rb_define_class_under(mEventIO, "Watcher", rb_cObject);
  rb_define_alloc_func(cEventIO_Watcher, EventIO_Watcher_allocate);

  rb_define_method(cEventIO_Watcher, "initialize", EventIO_Watcher_initialize, 0);
  rb_define_method(cEventIO_Watcher, "attach", EventIO_Watcher_attach, 1);
  rb_define_method(cEventIO_Watcher, "detach", EventIO_Watcher_detach, 0);
  rb_define_method(cEventIO_Watcher, "evloop", EventIO_Watcher_evloop, 0);
  rb_define_method(cEventIO_Watcher, "attached?", EventIO_Watcher_attached, 0);
}

static VALUE EventIO_Watcher_allocate(VALUE klass)
{
  struct EventIO_Watcher *watcher_data = (struct EventIO_Watcher *)xmalloc(sizeof(struct EventIO_Watcher));

  watcher_data->loop = Qnil;

  return Data_Wrap_Struct(klass, EventIO_Watcher_mark, EventIO_Watcher_free, watcher_data);
}

static void EventIO_Watcher_mark(struct EventIO_Watcher *watcher_data)
{
  if(watcher_data->loop != Qnil)
    rb_gc_mark(watcher_data->loop);
}

static void EventIO_Watcher_free(struct EventIO_Watcher *watcher_data)
{
  xfree(watcher_data);
}

static VALUE EventIO_Watcher_initialize(VALUE self)
{
  rb_raise(rb_eRuntimeError, "watcher base class should not be initialized directly");
}

/**
 *  call-seq:
 *    EventIO::Watcher.attach(loop) -> EventIO::Watcher
 *
 * Attach the watcher to the given EventIO::Loop.  If the watcher is already attached
 * to a loop, detach it from the old one and attach it to the new one.
 */
static VALUE EventIO_Watcher_attach(VALUE self, VALUE loop)
{
  struct EventIO_Watcher *watcher_data;

  Data_Get_Struct(self, struct EventIO_Watcher, watcher_data);

  VALUE loop_watchers = rb_iv_get(loop, "@watchers");

  if(loop_watchers == Qnil) {
    /* we should never get here */
    loop_watchers = rb_hash_new();
    rb_iv_set(loop, "@watchers", loop_watchers);
  }

  /* Add us to the loop's array of active watchers.  This is mainly done
   * to keep the VM from garbage collecting watchers that are associated
   * with a loop (and also lets you see within Ruby which watchers are
   * associated with a given loop), but isn't really necessary for any
   * other reason */
  rb_hash_aset(loop_watchers, self, Qtrue);

  VALUE active_watchers = rb_iv_get(loop, "@active_watchers");
  if(active_watchers == Qnil)
    active_watchers = INT2NUM(1);
  else
    active_watchers = INT2NUM(NUM2INT(active_watchers) + 1);
  rb_iv_set(loop, "@active_watchers", active_watchers);

  return self;
}

static void EventIO_Watcher_libuv_on_close(uv_handle_t* handle){
  struct EventIO_Watcher* watcher_data = handle->data;
  xfree(handle);
  watcher_data->handle = NULL;
  watcher_data->loop = Qnil;
}

/**
 *  call-seq:
 *    EventIO::Watcher.detach -> EventIO::Watcher
 *
 * Detach the watcher from its current EventIO::Loop.
 */
static VALUE EventIO_Watcher_detach(VALUE self)
{
  struct EventIO_Watcher *watcher_data;
  struct EventIO_Loop *loop_data;
  VALUE loop_watchers;
  int i;

  Data_Get_Struct(self, struct EventIO_Watcher, watcher_data);

  if(watcher_data->loop == Qnil)
    rb_raise(rb_eRuntimeError, "not attached to a loop");

  loop_watchers = rb_iv_get(watcher_data->loop, "@watchers");

  /* Remove us from the loop's array of active watchers.  This likely
   * has negative performance and scalability characteristics as this
   * isn't an O(1) operation.  Hopefully there's a better way...
   * Trying a hash for now... */
  rb_hash_delete(loop_watchers, self);

  rb_iv_set(
      watcher_data->loop,
      "@active_watchers",
      INT2NUM(NUM2INT(rb_iv_get(watcher_data->loop, "@active_watchers")) - 1)
  );

  Data_Get_Struct(watcher_data->loop, struct EventIO_Loop, loop_data);

  /* Iterate through the events in the loop's event buffer.  If there
   * are any pending events from this watcher, mark them NULL.  The
   * dispatch loop will skip them.  This prevents watchers earlier
   * in the event buffer from detaching others which may have pending
   * events in the buffer but get garbage collected in the meantime */
  for(i = 0; i < loop_data->events_received; i++) {
    if(loop_data->eventbuf[i].watcher == self)
      loop_data->eventbuf[i].watcher = Qnil;
  }

  watcher_data->handle->data = watcher_data;
  uv_close(watcher_data->handle, EventIO_Watcher_libuv_on_close);

  return self;
}

/**
 *  call-seq:
 *    EventIO::Watcher.evloop -> EventIO::Loop
 *
 * Return the loop to which we're currently attached
 */
static VALUE EventIO_Watcher_evloop(VALUE self)
{
  struct EventIO_Watcher *watcher_data;

  Data_Get_Struct(self, struct EventIO_Watcher, watcher_data);
  return watcher_data->loop;
}

/**
 *  call-seq:
 *    EventIO::Watcher.attached? -> Boolean
 *
 * Is the watcher currently attached to an event loop?
 */
static VALUE EventIO_Watcher_attached(VALUE self)
{
  return EventIO_Watcher_evloop(self) != Qnil;
}

uv_buf_t EventIO_alloc(uv_handle_t* handle, size_t suggested_size) {
  return uv_buf_init((char*)xmalloc(suggested_size), (unsigned int)suggested_size);
}

uv_loop_t* get_uv_loop(VALUE self){
  struct EventIO_Watcher *watcher_data;
  struct EventIO_Loop *loop_data;
  Data_Get_Struct(self, struct EventIO_Watcher, watcher_data);
  Data_Get_Struct(watcher_data->loop, struct EventIO_Loop, loop_data);
  return loop_data->uv_loop;
}

uv_handle_t* get_handle(VALUE self){
  struct EventIO_Watcher *watcher_data;
  Data_Get_Struct(self, struct EventIO_Watcher, watcher_data);
  return watcher_data->handle;
}
