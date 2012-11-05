#
# Fluent
#
# Copyright (C) 2011 FURUHASHI Sadayuki
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#
require 'thread'

# Monkeypatch Thread to include a method for obtaining the default Coolio::Loop
class Thread
  def _coolio_loop
    @_coolio_loop ||= Fluent::EventIO::Loop.new
  end
end

module Fluent
module Fluent::EventIO


class Loop
  def self.default
    Thread.current._coolio_loop
  end

  def initialize
    @watchers = {}
    @active_watchers = 0
    @loop = uv_loop_new
  end

  # Attach a watcher to the loop
  def attach(watcher)
    watcher.attach self
  end

  # Run the event loop and dispatch events back to Ruby.  If there
  # are no watchers associated with the event loop it will return
  # immediately.  Otherwise, run will continue blocking and making
  # event callbacks to watchers until all watchers associated with
  # the loop have been disabled or detached.  The loop may be
  # explicitly stopped by calling the stop method on the loop object.
  def run
    raise RuntimeError, "no watchers for this loop" if @watchers.empty?

    @running = true
    while @running
      run_once
    end
    watchers.each {|w| w.detach }

    @running = false
  end

  # Stop the event loop if it's running
  def stop
    raise RuntimeError, "loop not running" unless @running
    @running = false
  end

  # Does the loop have any active watchers?
  def has_active_watchers?
    @active_watchers > 0
  end

  # All watchers attached to the current loop
  def watchers
    @watchers.keys
  end
end


end
end
