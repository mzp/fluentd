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
      self.new UV::Loop.default
    end

    def create
      self.new UV::Loop.new
    end
  end

  def initialize(loop)
    @loop = loop
    @handles = []

    idle = @loop.idle
    idle.start {|x|
      if @stop
        $log.info "close all handles"
        @handles.each {|t| t.close{} if t.active? }
        idle.close {}
      end
    }
  end

  def at_close(handle)
    @handles << handle
  end

  def timer(interval, repeat, &block)
    timer = @loop.timer
    timer.start((interval * 1000).to_i, repeat ? 1 : 0){|x|
      block.call
    }
    at_close timer
    timer
  end

  def tcp(ip, port)
    tcp = @loop.tcp.bind(ip, port)
    at_close tcp
    Fluent::EventIO::Stream.new self, tcp
  end

  def udp
    require 'ipaddr'
    udp_ptr = UV.create_handle(:uv_udp)
    UV.udp_init(@loop.to_ptr, udp_ptr)
    server = UV::UDP.new(@loop, udp_ptr)

    at_close server
    Fluent::EventIO::UDP.new self, server
  end

  def unix(path)
    pipe = @loop.pipe.bind(path)
    at_close pipe
    Fluent::EventIO::Stream.new self, pipe
  end

  def file_stat(path, &block)
    fs = @loop.fs_event(path, &block)
    at_close fs
    fs
  end

  def run
    @loop.run
  end

  def alive?
    not @stop
  end

  def stop
    @stop = true
  end

  def force_stop
    @stop = true
    UV.loop_delete @loop
  end
end

class Handler
  def initialize(io)
    @io = io
  end

  def write(data)
    @io.write(data, &method(:on_write_complete))
  end

  def close
    @io.close {} if @io.active?
  end

  def on_write_complete; end
  def on_connect; end
  def on_close; end
end

class Stream
  class BlockHandler < Fluent::EventIO::Handler
    def initialize(io, block)
      super(io)
      @block = block
    end

    def on_connect; end
    def on_close; end

    def on_read(data)
      @block.call data
    end
  end

  def initialize(loop, stream)
    @loop = loop
    @stream = stream
  end

  def listen_handler(backlog, klass, *args)
    @stream.listen(backlog) do|client|
      unless @loop.alive?
        @stream.close {}
        next
      end
      client = @stream.accept

      $log.info "accept #{client}"
      handle = klass.new(client, *args)
      handle.on_connect

      client.start_read do |err, data|
        if err
          $log.trace { "closed fluent socket" }
          handle.on_close
          client.close {}
        else
          handle.on_read(data)
        end
      end
    end
    self
  end

  def listen(backlog=5, &block)
    listen_handler(backlog, BlockHandler, block)
  end
end

class UDP
  def initialize(loop, udp)
    @loop = loop
    @udp = udp
  end

  def bind(ip,port)
    @udp.bind(ip, port)
    self
  end

  def start(&block)
    @udp.start_recv{|_,data,ip,port|
       if @loop.alive?
        block.call(ip, port, data) if ip
      else
        @udp.close {}
      end
    }
  end

  def send(ip, port, data)
    @udp.send(ip, port, data){|_|}
  end
end

end
end
