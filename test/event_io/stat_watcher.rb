require 'test/unit'
require 'fluent/event_io'

class StatWatcherTest < Test::Unit::TestCase
  class MyStatWatcher < Fluent::EventIO::StatWatcher
    attr_reader :called

    def on_change(prev,cur)
      @called = true
    end
  end

  TMP_DIR = File.dirname(__FILE__) + "/../tmp"

  def setup
    @loop = Fluent::EventIO::Loop.new
    File.open("#{TMP_DIR}/stat_watcher.txt","w"){|io|}
    @stat = MyStatWatcher.new("#{TMP_DIR}/stat_watcher.txt")
    @loop.attach @stat

    @loop.attach Fluent::EventIO::TimerWatcher.new(1,true)
  end

  def teardown
    @stat.detach
  end

  def test_not_change
    # for timeout
    3.times { @loop.run_once }
    assert_equal nil, @stat.called
  end

  def test_change
    File.open("#{TMP_DIR}/stat_watcher.txt","w+"){|io|
      io.puts("stat test")
      io.puts("stat test")
      io.puts("stat test")
      io.puts("stat test")
    }
    3.times { @loop.run_once }
    assert_equal true, @stat.called
  end

  def test_fileid
    id1 = id2 = id3 = 0
    File.open("#{TMP_DIR}/stat_watcher.txt") {|io|
      id1 = Fluent::EventIO::StatWatcher.file_id(io)
    }
    File.open("#{TMP_DIR}/stat_watcher.txt","w"){|io|
      io.puts "hoge"
    }
    File.open("#{TMP_DIR}/stat_watcher.txt") {|io|
      id2 = Fluent::EventIO::StatWatcher.file_id(io)
    }
    assert_equal id1, id2
    File.open(__FILE__) {|io|
      id3 = Fluent::EventIO::StatWatcher.file_id(io)
    }
    assert_not_equal id1, id3
  end
end
