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

module ServerAdapter
  INPUT_SIZE = 16384

  def run(klass, args)
    @klass = klass
    @args = args
    begin
      loop { handle_connection! accept }
    rescue IOError
      $log.warn "accept error", :error => $!.to_s
    end
  end

  def handle_connection(socket)
    handler = @klass.new(socket, *@args)
    handler.on_connect
    loop { handler.on_read socket.readpartial(INPUT_SIZE) }
  rescue EOFError
    handler.on_close
    socket.close
  end
end

class TCPServerAdapter < Celluloid::IO::TCPServer
  include Celluloid::IO
  include ServerAdapter

  def initialize(bind, port, klass, *args)
    if port != nil then
      super(bind, port)
    else
      @server = bind
    end
    run!(klass, args)
  end

  def close
    terminate!
    super
  end

  def handle_connection(socket)
    _, port, host = socket.peeraddr
    $log.info "#{host}:#{port} disconnected"
    super(socket)
  end
end

class UNIXServer
  extend Forwardable
  def_delegators :@server, :listen, :sysaccept, :close, :closed?
  def initialize(path)
    @server = ::UNIXServer.new(path)
  end

  def accept
    actor = Thread.current[:actor]
    if evented?
      Celluloid.current_actor.wait_readable @server
      accept_nonblock
    else
      Celluloid::IO::TCPSocket.from_ruby_socket @server.accept
    end
  end

  def accept_nonblock
    Celluloid::IO::TCPSocket.from_ruby_socket @server.accept_nonblock
  end

  def to_io
    @server
  end

  # Are we inside a Celluloid ::IO actor?
  def evented?
    actor = Thread.current[:actor]
    actor && actor.mailbox.is_a?(Celluloid::IO::Mailbox)
  end
end

class UNIXServerAdapter < UNIXServer
  include Celluloid::IO
  include ServerAdapter

  def initialize(path, klass, *args)
    super(path)
    run!(klass, args)
  end
end

class UDPServer
  def initialize(io, callback)
    @io = io
    @callback = callback

    run!
  end

  def run
    begin
      loop do
        @io.wait_readable
        on_readable
      end
    rescue IOError
      $log.warn "accept error", :error => $!.to_s
    end
  end
end


end
