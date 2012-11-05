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


module Meta
  # Use an alternate watcher with the attach/detach/enable/disable methods
  # if it is presently assigned.  This is useful if you are waiting for
  # an event to occur before the current watcher can be used in earnest,
  # such as making an outgoing TCP connection.
  def watcher_delegate(proxy_var)
    %w{attach detach enable disable}.each do |method|
      module_eval <<-EOD
          def #{method}(*args)
            if defined? #{proxy_var} and #{proxy_var}
            #{proxy_var}.#{method}(*args)
              return self
            end

            super
          end
            EOD
    end
  end

  # Define callbacks whose behavior can be changed on-the-fly per instance.
  # This is done by giving a block to the callback method, which is captured
  # as a proc and stored for later.  If the method is called without a block,
  # the stored block is executed if present, otherwise it's a noop.
  def event_callback(*methods)
    methods.each do |method|
      module_eval <<-EOD
          def #{method}(*args, &block)
            if block
              @#{method}_callback = block
              return
            end

            if defined? @#{method}_callback and @#{method}_callback
              instance_exec(*args, &@#{method}_callback)
            end
          end
      EOD
    end
  end
end


end
end
