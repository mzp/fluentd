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


class StatWatcher
  # The actual implementation of this class resides in the C extension
  # Here we metaprogram proper event_callbacks for the callback methods
  # These can take a block and store it to be called when the event
  # is actually fired.

  extend Meta
  event_callback :on_stat
  def on_stat(filename, event)
    @prev = @current
    @current = File.stat @path
    on_change(@prev, @current)
  end

  def on_change(prev,current)
  end
end


end
end
