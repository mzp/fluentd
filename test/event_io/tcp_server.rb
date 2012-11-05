require 'test/unit'
require 'fluent/event_io'
require 'helper'
require 'socket'

class TCPServerTest < Test::Unit::TestCase

  class Handler < Fluent::EventIO::Socket
    def initialize(io, block)
      @block = block
      super(io)
    end

    def on_close
      @block.call(:on_close)
    end

    def on_connect
      @block.call(:on_connect)
    end

    def on_read(data)
      @block.call(:on_read, data)
      write data
    end

    def on_write_complete
      @block.call(:on_write_complete)
    end
  end

  def setup
    @expects = []
    @loop   = Fluent::EventIO::Loop.new
    @server = Fluent::EventIO::TCPServer.new('127.0.0.1', 13998, Handler, lambda {|*args|
      @expects << args
    })
    @loop.attach @server
    @client = ::TCPSocket.new('127.0.0.1', 13998)
    while @expects.empty?
      @loop.run_once
    end
  end

  def teardown
    @client.close rescue nil
    @server.detach
  end

  def test_connection
    assert_equal [:on_connect], @expects[0]
  end

  def test_read
    @expects = []

    @client.write "hoge"
    @client.shutdown
    @client.close

    @loop.run_once

    assert_equal [:on_read, "hoge"], @expects[0]
  end

  def test_echo
    @client.write 'echo'

    @loop.run_once

    buf = @client.recv(4)
    assert_equal "echo",buf

    @expects = []
    @loop.run_once
    assert_equal [:on_write_complete], @expects[0]
  end

  def test_check_close
    @expects = []
    @client.close
    @loop.run_once

    assert_equal [:on_close], @expects[0]
  end
end
