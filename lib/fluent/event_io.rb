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

module Listener
  private
  def callbacks
    @callbacks ||= Set.new
  end

  def callback(&f)
    const_name = "Callback_#{f.object_id}"
    unless self.class.const_defined?(const_name)
      callbacks << const_name
      self.class.const_set(const_name, f)
    end
    self.class.const_get(const_name)
  end

  def clear_callbacks
    callbacks.each do |name|
      self.class.send(:remove_const, name)
    end
    callbacks.clear
  end
end

class Loop
  include Listener

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

    idle do
      if @stop
        $log.info "closing libuv handlers"
        Foolio::UV.close_all(@loop, callback{
                               clear_callbacks
                             })
      end
    end
  end

  def idle(&block)
    handle = Foolio::UV.idle_init(@loop)
    Foolio::UV.idle_start(handle, callback{|_| block[] })
  end

  def timer(interval, repeat, &block)
    handle = Foolio::UV.timer_init(@loop)
    sec    = (interval * 1000).to_i
    Foolio::UV.timer_start(handle,
                           callback{|_| block[] },
                           sec,
                           sec)
  end

  def tcp(ip, port)
     handle = Foolio::UV.tcp_init(@loop)
     Foolio::UV.tcp_bind(handle, Foolio::UV.ip4_addr(ip, port))
     Fluent::EventIO::TCP.new @loop, handle
  end

  def udp
    handle = Foolio::UV.udp_init(@loop)
    Fluent::EventIO::UDP.new @loop, handle
  end

  # def unix(path)
  #   pipe = @loop.pipe.bind(path)
  #   at_close pipe
  #   Fluent::EventIO::TCP.new self, pipe
  # end

  # def file_stat(path, &block)
  #   fs = @loop.fs_event(path, &block)
  #   at_close fs
  #   fs
  # end

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
  include Listener
  def initialize(io)
    @io = io
  end

  def write(data)
    Foolio::UV.write(nil, @io, data, callback do|status|
                       on_write_complete(status)
                     end)
  end

  def close
    $log.info "close #{@io}"
    Foolio::UV.close(@io, callback{})
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
      client = Foolio::UV.tcp_init(@loop)
      Foolio::UV.accept(@stream, client)
      $log.info "accept #{client}"
      handle = klass.new(client, *args)
      handle.on_connect

      on_read = proc {|data|
        if data then
          handle.on_read(data)
        else
          handle.close
        end
      }
      Foolio::UV.read_start(client, on_read)
    end
    Foolio::UV.listen(@stream, backlog, callback)
    self
  end

  def listen(backlog=5, &block)
    listen_handler(backlog, BlockHandler, block)
  end
end

class UDP
  include Listener
  def initialize(loop, udp)
    @loop = loop
    @udp = udp
  end

  def bind(ip,port)
    Foolio::UV.udp_bind(@udp, Foolio::UV.ip4_addr(ip, port), 0)
    self
  end

  def start(&block)
    on_recv = callback do|data, addr, flags|
      ip = Foolio::UV.ip_name addr
      port = Foolio::UV.port addr
      block.call(ip, port, data) if ip
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
