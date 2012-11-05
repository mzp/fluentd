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
module EventIO


class UNIXServer < Watcher
  # Here we metaprogram proper event_callbacks for the callback methods
  # These can take a block and store it to be called when the event
  # is actually fired.
  extend Meta

  def initialize(path, klass, *args, &block)
    @path = path
    @klass, @block = klass, block
    @args = [ nil ] + args
  end

  def bind(path)
    @path = path
  end

  event_callback :on_connection
  def on_connection(socket)
    connection = socket.attach(evloop)
    connection.__send__(:on_connect)
    @block.call(connection) if @block
  end

  event_callback :on_close
  def on_close
  end
end


end
end
