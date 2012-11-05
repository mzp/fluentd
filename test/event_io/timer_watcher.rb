require 'test/unit'
require 'fluent/event_io'

class TimerWatcherTest < Test::Unit::TestCase
  Interval = 0.010

  class MyTimerWatcher < Fluent::EventIO::TimerWatcher
    TMP = '0'
    def on_timer
      TMP.succ!
    end
  end

  def setup
    @loop = Fluent::EventIO::Loop.default
    @watcher = MyTimerWatcher.new(Interval, false)
    @loop.attach(@watcher)
  end

  def teardown
    @watcher.detach
  end

  def test_timer
   assert_equal '0', MyTimerWatcher::TMP
   sleep Interval

   Fluent::EventIO::Loop.default.run_once
   assert_equal '1', MyTimerWatcher::TMP
  end
end
