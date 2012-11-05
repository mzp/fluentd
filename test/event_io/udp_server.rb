require 'test/unit'
require 'fluent/event_io'
require 'helper'
require 'socket'

class UDPServerTest < Test::Unit::TestCase
  class MyUDPServer < Fluent::EventIO::UDPSocket
    def initialize(io,block)
      super(io)
      @block = block
    end

    def on_read(*args)
      @block.call(:on_read, *args)
    end
  end

  def setup
    @expects = []
    @loop = Fluent::EventIO::Loop.new
    @server = MyUDPServer.new(nil, lambda{|*args|
                                @expects << args
                              })
    @server.bind('127.0.0.1', 13998)
    @loop.attach @server

    @client = ::UDPSocket.new
    @client.connect('127.0.0.1', 13998)
  end

  def teardown
    @server.detach
  end

  def send(msg)
    @client.send(msg, 0)
    while @expects.empty?
      @loop.run_once
    end
  end

  def test_send
    send "hoge"

    assert_equal :on_read, @expects[0][0]
    assert_equal "hoge", @expects[0][1]
    family,port, hostname, address =  @expects[0][2]
    assert_equal "127.0.0.1", address
  end

  def test_recv
    send "hoge"
    _,port,_,address =  @expects[0][2]

    @server.send("fuga", 0, address, port)
    @loop.run_once
    recv,_ = @client.recvfrom 10
    assert_equal "fuga", recv
  end
end
