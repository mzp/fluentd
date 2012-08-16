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
      self.new Foolio::UV.default_loop
    end

    def create
      self.new Foolio::UV.loop_new
    end
  end

  def initialize(loop)
    @loop = loop

    idle = self.idle {|_|
      if @stop
        $log.info "close all handles"
        Foolio::UV.close_all @loop
      end
    }
  end

  def idle(&block)
    handle = Foolio::UV.idle_init(@loop)
    Foolio::UV.idle_start(handle,
                          lambda {|_| block[] })
  end

  def timer(interval, repeat, &block)
    handle = Foolio::UV.timer_init(@loop)
    sec    = (interval * 1000).to_i
    Foolio::UV.timer_start(handle,
                           proc {|_| block[] },
                           sec,
                           sec)
  end

  def tcp(ip, port)
    handle = Foolio:UV.tcp_init(@loop)
    Foolio::UV.tcp_bind(handle, Foolio::UV.ip4_addr(ip, port))
    Fluent::EventIO::TCP.new @loop, handle
  end

  def udp
    handle = Foolio:UV.udp_init(@loop)
    Fluent::EventIO::UDP.new @loop, handle
  end

  def unix(path)
    pipe = @loop.pipe.bind(path)
    at_close pipe
    Fluent::EventIO::TCP.new self, pipe
  end

  def file_stat(path, &block)
    fs = @loop.fs_event(path, &block)
    at_close fs
    fs
  end

  def run
    Foolio::UV.run @loop
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
    callback = proc do|req, status|
      on_write_complete()
    end
    @io.write(data, callback)
    Foolio::UV.write(nil, @io, data, callback)
  end

  def close
    Foolio::UV.close(@io, proc{})
  end

  def on_write_complete; end
  def on_connect; end
  def on_close; end
end

class TCP
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
    @loop   = loop
    @stream = stream
  end

  def listen_handler(backlog, klass, *args)
    callback = proc do|status|
      client = Foolio:UV.tcp_init(@loop)
      @stream.accept(@server, client)
      $log.info "accept #{client}"
      handle = klass.new(client, *args)
      handle.on_connect

      on_read = proc {|data|
        handle.on_read(data)
      }
      Foolio::UV.read_start(client, on_read)
    end
    FoolIO::UV.listen(@stream, backlog, callback)
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
    Foolio::UV.udp_bind(handle, Foolio::UV.ip4_addr(ip, port), 0)
    self
  end

  def start(&block)
    on_recv = proc do|data, addr, flags|
      ip = Foolio::UV.ip_name addr
      port = Foolio::UV.port addr
      block.call(ip, port, data)
    end
    Foolio::UV.udp_recv_start(@udp, on_recv)
  end

  def send(ip, port, data)
    on_send = proc {|req, status|}
    Foolio::UV.udp_send(nil, @udp, data, Foolio::UV.ip4_addr(ip, port), on_send)
  end
end

end
end
