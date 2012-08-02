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
module Fluent


class GCStatInput < Input
  Plugin.register_input('gc_stat', self)

  def initialize
    super
  end

  config_param :emit_interval, :time, :default => 60
  config_param :tag, :string

  class TimerWatcher
    include Celluloid

    def initialize(interval, &callback)
      every(interval) do
        begin
          callback.call
        rescue
          # TODO log?
         $log.error $!.to_s
         $log.error_backtrace
        end
      end
    end
  end

  def configure(conf)
    super
  end

  def start
    @timer = TimerWatcher.supervise(@emit_interval, &method(:on_timer))
  end

  def shutdown
    @timer.terminate
    @timer.join
  end

  def on_timer
    now = Engine.now
    record = GC.stat
    Engine.emit(@tag, now, record)
  end
end


end
