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

#ifndef WATCHER_H
#define WATCHER_H

#define Watcher_Setup(watcher_type) \
  struct EventIO_Loop *loop_data; \
  struct EventIO_Watcher *watcher_data; \
  Data_Get_Struct(loop, struct EventIO_Loop, loop_data); \
  Data_Get_Struct(self, struct EventIO_Watcher, watcher_data); \
  if(!rb_obj_is_kind_of(loop, cEventIO_Loop)) \
    rb_raise(rb_eArgError, "expected loop to be an instance of EventIO::Loop"); \
  if(watcher_data->loop != Qnil) \
    rb_funcall(self, rb_intern("detach"), 0, 0); \
  watcher_data->loop = loop; \
  watcher_data->dispatch_callback = EventIO_##watcher_type##_dispatch_callback;

uv_buf_t EventIO_alloc(uv_handle_t* handle, size_t suggested_size);
uv_loop_t* get_uv_loop(VALUE self);
uv_handle_t* get_handle(VALUE self);

#endif
