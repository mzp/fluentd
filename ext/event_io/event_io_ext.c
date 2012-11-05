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
#include <ruby.h>

void Init_eventio_loop();
void Init_eventio_watcher();
void Init_eventio_timer_watcher();
void Init_eventio_tcp();
void Init_eventio_udp();
void Init_eventio_pipe();
void Init_eventio_socket();
void Init_eventio_stat_watcher();

void Init_event_io_ext(){
  Init_eventio_loop();
  Init_eventio_watcher();
  Init_eventio_timer_watcher();
  Init_eventio_tcp();
  Init_eventio_udp();
  Init_eventio_pipe();
  Init_eventio_socket();
  Init_eventio_stat_watcher();
}
