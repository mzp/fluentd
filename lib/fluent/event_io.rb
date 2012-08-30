# -*- coding: utf-8 -*-
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
class Loop
  class << self
    def default
      self.new Foolio::Loop.default
    end

    def create
      self.new Foolio::Loop.create
    end
  end

  def initialize(loop)
    @loop = loop
  end

  def idle(&block)
    @loop.idle.start(&block)
  end

  def timer(interval, &block)
    @loop.timer.start(interval,&block)
  end

  def tcp(ip, port)
    @loop.tcp(ip,port)
  end

  def udp
    @loop.udp
  end

  def unix(path)
    @loop.unix(path)
  end

  def file_stat(path, &block)
    @loop.file_stat(path, &block)
  end

  def run
    @loop.run
  end

  def stop
    @loop.stop
  end
end

class Handler < Foolio::Handler
end

end
end
