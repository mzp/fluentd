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

static VALUE mEventIO      = Qnil;
static VALUE cEventIO_Loop = Qnil;

static VALUE EventIO_Loop_allocate(VALUE klass);
static void EventIO_Loop_mark(struct EventIO_Loop *loop);
static void EventIO_Loop_free(struct EventIO_Loop *loop);

static VALUE EventIO_Loop_initialize(VALUE self);
static VALUE EventIO_Loop_uv_loop_new(VALUE self);
static VALUE EventIO_Loop_run_once(VALUE self);
static void EventIO_Loop_uv_loop_oneshot(struct EventIO_Loop *loop_data);
static void EventIO_Loop_dispatch_events(struct EventIO_Loop *loop_data);

static const int DEFAULT_EVENTBUF_SIZE = 32;

inline void RUN_LOOP(struct EventIO_Loop* loop_data) {
  loop_data->running = 1;
  uv_run_once(loop_data->uv_loop);
  loop_data->running = 0;
}

/*
 * EventIO::Loop represents an event loop.  Event watchers can be attached and
 * unattached.  When an event loop is run, all currently attached watchers
 * are monitored for events, and their respective callbacks are signaled
 * whenever events occur.
 */
void Init_eventio_loop()
{
  mEventIO      = rb_define_module_under(rb_define_module("Fluent"), "EventIO");
  cEventIO_Loop = rb_define_class_under(mEventIO, "Loop", rb_cObject);

  rb_define_alloc_func(cEventIO_Loop, EventIO_Loop_allocate);

  rb_define_method(cEventIO_Loop        , "initialize" , EventIO_Loop_initialize , 0);
  rb_define_private_method(cEventIO_Loop, "uv_loop_new", EventIO_Loop_uv_loop_new, 0);
  rb_define_method(cEventIO_Loop        , "run_once"   , EventIO_Loop_run_once   , 0);
}

static VALUE EventIO_Loop_allocate(VALUE klass)
{
  struct EventIO_Loop *loop = (struct EventIO_Loop *)xmalloc(sizeof(struct EventIO_Loop));
  memset(loop, 0, sizeof(struct EventIO_Loop));
  loop->eventbuf_size = DEFAULT_EVENTBUF_SIZE;
  loop->eventbuf = (struct EventIO_Event *)xmalloc(sizeof(struct EventIO_Event) * DEFAULT_EVENTBUF_SIZE);

  return Data_Wrap_Struct(klass, EventIO_Loop_mark, EventIO_Loop_free, loop);
}

static void EventIO_Loop_mark(struct EventIO_Loop *loop)
{
}

static void EventIO_Loop_free(struct EventIO_Loop *loop)
{
  if(!loop->uv_loop)
    return;

  uv_loop_delete(loop->uv_loop);

  xfree(loop->eventbuf);
  xfree(loop);
}

static VALUE EventIO_Loop_initialize(VALUE self)
{
  EventIO_Loop_uv_loop_new(self);
  return Qnil;
}

/* Wrapper for populating a EventIO_Loop struct with a new event loop */
static VALUE EventIO_Loop_uv_loop_new(VALUE self)
{
  struct EventIO_Loop *loop_data;
  Data_Get_Struct(self, struct EventIO_Loop, loop_data);

  if(loop_data->uv_loop)
    rb_raise(rb_eRuntimeError, "loop already initialized");

  loop_data->uv_loop = uv_loop_new();

  loop_data->empty_loop = (uv_timer_t*)xmalloc(sizeof(uv_timer_t));
  uv_timer_init(loop_data->uv_loop, loop_data->empty_loop);

  return Qnil;
}

static void EventIO__libuv_timer_cb(uv_timer_t* handle, int status){
}

static void EventIO__prevent_sleeping_loop(struct EventIO_Loop* loop) {
  uv_timer_start(loop->empty_loop, EventIO__libuv_timer_cb, 1, 0);
}

/* libev callback for receiving events */
void EventIO_Loop_process_event(VALUE watcher, int revents)
{
  struct EventIO_Loop *loop_data;
  struct EventIO_Watcher *watcher_data;

  /* The Global VM lock isn't held right now, but hopefully
   * we can still do this safely */
  Data_Get_Struct(watcher, struct EventIO_Watcher, watcher_data);
  Data_Get_Struct(watcher_data->loop, struct EventIO_Loop, loop_data);

  /*  Well, what better place to explain how this all works than
   *  where the most wonky and convoluted stuff is going on!
   *
   *  Our call path up to here looks a little something like:
   *
   *  -> release GVL -> event syscall -> libev callback
   *  (GVL = Global VM Lock)             ^^^ You are here
   *
   *  We released the GVL in the EventIO_Loop_run_once() function
   *  so other Ruby threads can run while we make a blocking
   *  system call (one of epoll, kqueue, port, poll, or select,
   *  depending on the platform).
   *
   *  More specifically, this is a libev callback abstraction
   *  called from a real libev callback in every watcher,
   *  hence this function not being static.  The real libev
   *  callbacks are event-specific and handled in a watcher.
   *
   *  For syscalls like epoll and kqueue, the kernel tells libev
   *  a pointer (to a structure with a pointer) to the watcher
   *  object.  No data structure lookups are required at all
   *  (beyond structs), it's smooth O(1) sailing the entire way.
   *  Then libev calls out to the watcher's callback, which
   *  calls this function.
   *
   *  Now, you may be curious: if the watcher already knew what
   *  event fired, why the hell is it telling the loop?  Why
   *  doesn't it just rb_funcall() the appropriate callback?
   *
   *  Well, the problem is the Global VM Lock isn't held right
   *  now, so we can't rb_funcall() anything.  In order to get
   *  it back we have to:
   *
   *  stash event and return -> acquire GVL -> dispatch to Ruby
   *
   *  Which is kinda ugly and confusing, but still gives us
   *  an O(1) event loop whose heart is in the kernel itself. w00t!
   *
   *  So, stash the event in the loop's data struct.  When we return
   *  the uv_loop() call being made in the EventIO_Loop_run_once_blocking()
   *  function below will also return, at which point the GVL is
   *  reacquired and we can call out to Ruby */

  /* Grow the event buffer if it's too small */
  if(loop_data->events_received >= loop_data->eventbuf_size) {
    loop_data->eventbuf_size *= 2;
    loop_data->eventbuf = (struct EventIO_Event *)xrealloc(
        loop_data->eventbuf,
        sizeof(struct EventIO_Event) * loop_data->eventbuf_size
        );
  }

  loop_data->eventbuf[loop_data->events_received].watcher = watcher;
  loop_data->eventbuf[loop_data->events_received].revents = revents;

  loop_data->events_received++;

  // At some platform(e.g. Windows), running loop sleep until some IO event happen.
  // But not all callback generate IO event.
  // By attaching one-shot timer event, we make loop running next time.
  EventIO__prevent_sleeping_loop(loop_data);
}

/**
 *  call-seq:
 *    EventIO::Loop.run_once -> nil
 *
 * Run the EventIO::Loop once, blocking until events are received.
 */
static VALUE EventIO_Loop_run_once(VALUE self)
{
  VALUE nevents;

  struct EventIO_Loop *loop_data;
  Data_Get_Struct(self, struct EventIO_Loop, loop_data);

  assert(loop_data->uv_loop && !loop_data->events_received);

  EventIO_Loop_uv_loop_oneshot(loop_data);

  EventIO_Loop_dispatch_events(loop_data);

  nevents = INT2NUM(loop_data->events_received);
  loop_data->events_received = 0;
  return nevents;
}

/* Ruby 1.9 supports blocking system calls through rb_thread_blocking_region() */
static VALUE EventIO_Loop_uv_loop_oneshot_blocking(void *ptr)
{
  /* The libev loop has now escaped through the Global VM Lock unscathed! */
  struct EventIO_Loop *loop_data = (struct EventIO_Loop *)ptr;

  RUN_LOOP(loop_data);

  return Qnil;
}

static void EventIO_Loop_uv_loop_oneshot(struct EventIO_Loop *loop_data)
{
  /* Use Ruby 1.9's rb_thread_blocking_region call to make a blocking system call */
  rb_thread_blocking_region(EventIO_Loop_uv_loop_oneshot_blocking, loop_data, RUBY_UBF_IO, 0);
}

static void EventIO_Loop_dispatch_events(struct EventIO_Loop *loop_data)
{
  struct EventIO_Watcher *watcher_data;

  for(int i = 0; i < loop_data->events_received; i++) {
    /* A watcher with pending events may have been detached from the loop
     * during the dispatch process.  If so, the watcher clears the pending
     * events, so skip over them */
    if(loop_data->eventbuf[i].watcher == Qnil)
      continue;

    Data_Get_Struct(loop_data->eventbuf[i].watcher, struct EventIO_Watcher, watcher_data);
    watcher_data->dispatch_callback(loop_data->eventbuf[i].watcher, loop_data->eventbuf[i].revents);
  }
}
