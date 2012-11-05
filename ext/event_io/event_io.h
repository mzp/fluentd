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
#ifndef EVENT_IO_H
#define EVENT_IO_H

#include <uv.h>
#include <ruby.h>
#include <ruby/io.h>
#include <assert.h>

#ifdef GetReadFile
#define FPTR_TO_FD(fptr) (fileno(GetReadFile(fptr)))
#else

#if !HAVE_RB_IO_T || (RUBY_VERSION_MAJOR == 1 && RUBY_VERSION_MINOR == 8)
#define FPTR_TO_FD(fptr) fileno(fptr->f)
#else
#define FPTR_TO_FD(fptr) fptr->fd
#endif

#endif

struct EventIO_Event
{
  /* These values are used to extract events from libev callbacks */
  VALUE watcher;
  int revents;
};

struct EventIO_Loop
{
  uv_loop_t *uv_loop;
  uv_timer_t *empty_loop;

  int running;
  int events_received;
  int eventbuf_size;
  struct EventIO_Event *eventbuf;
};

struct EventIO_Watcher
{
  uv_handle_t* handle;
  VALUE loop;

  void (*dispatch_callback)(VALUE self, int revents);
};

void EventIO_Loop_process_event(VALUE watcher, int revents);

#define CHECK(MSG, X)                            \
  { int retval = (X); \
    if(retval != 0) { \
    rb_raise(rb_eException, "libuv failed(%s) %d",MSG, retval);      \
  }}

#endif
