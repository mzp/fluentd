require "mkmf"

$CFLAGS << " -std=gnu99 -fPIC"

$DLDFLAGS << " -fPIC"
$warnflags.gsub!("-Wdeclaration-after-statement","")

if RUBY_PLATFORM =~ /darwin/
  $DLDFLAGS << " -framework CoreServices"
end

libuv_dir = File.expand_path("../../libuv", __FILE__)
foolio_dir = File.expand_path("../", __FILE__)

Dir.chdir(libuv_dir) do
  system "make CFLAGS='-fPIC'"
end

Dir.chdir(foolio_dir) do
  require 'fileutils'
  FileUtils.cp "#{libuv_dir}/uv.a", "#{foolio_dir}/libuv.a"
end

dir_config "uv", "#{libuv_dir}/include", foolio_dir
case RbConfig::CONFIG['host_os'].downcase
when /mingw|mswin/
  have_library "ws2_32"
  have_library "psapi"
  have_library "iphlpapi"
  $defs << "-D_WIN32_WINNT=0x600"
when /darwin/
  # don't use flat_namespace to avoid symbol conflict
  # with Cool.io's libev.
  $DLDFLAGS.gsub!("-Wl,-flat_namespace","")
end
have_library "uv"

create_makefile "event_io/event_io_ext"
