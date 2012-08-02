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
module Windows

class Supervisor
  class LoggerInitializer
    def initialize(path, level, chuser, chgroup)
      @path = path
      @level = level
      @chuser = chuser
      @chgroup = chgroup
    end

    def init
      if @path && @path != "-"
        @io = File.open(@path, "a")
        if @chuser || @chgroup
          chuid = @chuser ? `id -u #{@chuser}`.to_i : nil
          chgid = @chgroup ? `id -g #{@chgroup}`.to_i : nil
          File.chown(chuid, chgid, @path)
        end
      else
        @io = STDOUT
      end

      $log = Fluent::Log.new(@io, @level)

      $log.enable_color(false) if @path
      $log.enable_debug if @level <= Fluent::Log::LEVEL_DEBUG
    end

    def stdout?
      @io == STDOUT
    end

    def reopen!
      if @path && @path != "-"
        @io.reopen(@path, "a")
      end
      self
    end
  end

  def initialize(opt)
    @config_path = opt[:config_path]
    @log_path = opt[:log_path]
    @log_level = opt[:log_level]
    @daemonize = opt[:daemonize]
    @chgroup = opt[:chgroup]
    @chuser = opt[:chuser]
    @libs = opt[:libs]
    @plugin_dirs = opt[:plugin_dirs]
    @inline_config = opt[:inline_config]

    @log = LoggerInitializer.new(@log_path, @log_level, @chuser, @chgroup)
    @finished = false
    @main_thread = nil
  end

  def start
    require 'fluent/load'
    @log.init
    start_daemonize if @daemonize
    until @finished
      supervise do
        read_config
        change_privilege
        init_engine
        install_main_process_signal_handlers
        run_configure
        run_engine
        exit 0
      end
    end
  end

  private
  def start_daemonize
    $log.error "can not run daemon at Windows. Use a service mode"
  end

  def supervise(&block)
    start_time = Time.now

    $log.info "starting fluentd-#{Fluent::VERSION}"
    @main_thread = Thread.new(&block)
    @main_thread.join
    @main_thread = nil
    ecode = $?.to_i

    $log.info "process finished", :code=>ecode

    if !@finished && Time.now - start_time < 1
      $log.warn "process died within 1 second. exit."
      exit ecode
    end
  end

  def main_process(&block)
    begin
      block.call

    rescue Fluent::ConfigError
      $log.error "config error", :file=>@config_path, :error=>$!.to_s
      $log.debug_backtrace
      unless @log.stdout?
        console = Fluent::Log.new(STDOUT, @log_level).enable_debug
        console.error "config error", :file=>@config_path, :error=>$!.to_s
        console.debug_backtrace
      end

    rescue
      $log.error "unexpected error", :error=>$!.to_s
      $log.error_backtrace
      unless @log.stdout?
        console = Fluent::Log.new(STDOUT, @log_level).enable_debug
        console.error "unexpected error", :error=>$!.to_s
        console.error_backtrace
      end
    end

    exit! 1
  end

  def read_config
    $log.info "reading config file", :path=>@config_path
    @config_fname = File.basename(@config_path)
    @config_basedir = File.dirname(@config_path)
    @config_data = File.read(@config_path)
    if @inline_config == '-'
      @config_data << "\n" << STDIN.read
    elsif @inline_config
      @config_data << "\n" << @inline_config.gsub("\\n","\n")
    end
  end

  def run_configure
    Fluent::Engine.parse_config(@config_data, @config_fname, @config_basedir)
  end

  def change_privilege
    if @chgroup
      chgid = @chgroup.to_i
      if chgid.to_s != @chgroup
        chgid = `id -g #{@chgroup}`.to_i
        if $?.to_i != 0
          exit 1
        end
      end
      Process::GID.change_privilege(chgid)
    end

    if @chuser
      chuid = @chuser.to_i
      if chuid.to_s != @chuser
        chuid = `id -u #{@chuser}`.to_i
        if $?.to_i != 0
          exit 1
        end
      end
      Process::UID.change_privilege(chuid)
    end
  end

  def init_engine
    require 'fluent/load'
    Fluent::Engine.init

    @libs.each {|lib|
      require lib
    }

    @plugin_dirs.each {|dir|
      if Dir.exist?(dir)
        dir = File.expand_path(dir)
        Fluent::Engine.load_plugin_dir(dir)
      end
    }
  end

  def install_main_process_signal_handlers
    trap :INT do
      Fluent::Engine.stop
    end

    trap :TERM do
      Fluent::Engine.stop
    end
  end

  def run_engine
    Fluent::Engine.run
  end
end

end
end

