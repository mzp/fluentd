# Generated by jeweler
# DO NOT EDIT THIS FILE DIRECTLY
# Instead, edit Jeweler::Tasks in Rakefile, and run 'rake gemspec'
# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "fluentd"
  s.version = "0.10.25"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Sadayuki Furuhashi"]
  s.date = "2012-08-12"
  s.email = "frsyuki@gmail.com"
  s.executables = ["fluentd", "fluent-cat", "fluent-gem"]
  s.extra_rdoc_files = [
    "ChangeLog",
    "README.rdoc"
  ]
  s.files = [
    "AUTHORS",
    "COPYING",
    "Gemfile",
    "Rakefile",
    "VERSION",
    "bin/fluent-cat",
    "bin/fluent-gem",
    "bin/fluentd",
    "fluent.conf",
    "fluentd.gemspec",
    "lib/fluent/buffer.rb",
    "lib/fluent/command/cat.rb",
    "lib/fluent/command/fluentd.rb",
    "lib/fluent/config.rb",
    "lib/fluent/engine.rb",
    "lib/fluent/env.rb",
    "lib/fluent/event.rb",
    "lib/fluent/input.rb",
    "lib/fluent/load.rb",
    "lib/fluent/log.rb",
    "lib/fluent/match.rb",
    "lib/fluent/mixin.rb",
    "lib/fluent/output.rb",
    "lib/fluent/parser.rb",
    "lib/fluent/plugin.rb",
    "lib/fluent/plugin/buf_file.rb",
    "lib/fluent/plugin/buf_memory.rb",
    "lib/fluent/plugin/buf_zfile.rb",
    "lib/fluent/plugin/in_exec.rb",
    "lib/fluent/plugin/in_forward.rb",
    "lib/fluent/plugin/in_gc_stat.rb",
    "lib/fluent/plugin/in_http.rb",
    "lib/fluent/plugin/in_object_space.rb",
    "lib/fluent/plugin/in_status.rb",
    "lib/fluent/plugin/in_stream.rb",
    "lib/fluent/plugin/in_syslog.rb",
    "lib/fluent/plugin/in_tail.rb",
    "lib/fluent/plugin/out_copy.rb",
    "lib/fluent/plugin/out_exec.rb",
    "lib/fluent/plugin/out_exec_filter.rb",
    "lib/fluent/plugin/out_file.rb",
    "lib/fluent/plugin/out_forward.rb",
    "lib/fluent/plugin/out_null.rb",
    "lib/fluent/plugin/out_roundrobin.rb",
    "lib/fluent/plugin/out_stdout.rb",
    "lib/fluent/plugin/out_stream.rb",
    "lib/fluent/plugin/out_test.rb",
    "lib/fluent/process.rb",
    "lib/fluent/status.rb",
    "lib/fluent/supervisor.rb",
    "lib/fluent/test.rb",
    "lib/fluent/test/base.rb",
    "lib/fluent/test/input_test.rb",
    "lib/fluent/test/output_test.rb",
    "lib/fluent/version.rb",
    "test/config.rb",
    "test/helper.rb",
    "test/match.rb",
    "test/mixin.rb",
    "test/plugin/in_exec.rb",
    "test/plugin/in_forward.rb",
    "test/plugin/in_http.rb",
    "test/plugin/in_object_space.rb",
    "test/plugin/in_stream.rb",
    "test/plugin/in_tail.rb",
    "test/plugin/out_copy.rb",
    "test/plugin/out_exec.rb",
    "test/plugin/out_exec_filter.rb",
    "test/plugin/out_file.rb",
    "test/plugin/out_forward.rb",
    "test/plugin/out_roundrobin.rb",
    "test/plugin/out_stdout.rb",
    "test/plugin/out_stream.rb"
  ]
  s.homepage = "http://fluentd.org/"
  s.require_paths = ["lib"]
  s.required_ruby_version = Gem::Requirement.new("~> 1.9.2")
  s.rubygems_version = "1.8.23"
  s.summary = "Fluent event collector"
  s.test_files = ["test/config.rb", "test/helper.rb", "test/match.rb", "test/mixin.rb", "test/plugin/in_exec.rb", "test/plugin/in_forward.rb", "test/plugin/in_http.rb", "test/plugin/in_object_space.rb", "test/plugin/in_stream.rb", "test/plugin/in_tail.rb", "test/plugin/out_copy.rb", "test/plugin/out_exec.rb", "test/plugin/out_exec_filter.rb", "test/plugin/out_file.rb", "test/plugin/out_forward.rb", "test/plugin/out_roundrobin.rb", "test/plugin/out_stdout.rb", "test/plugin/out_stream.rb"]

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<fluentd>, [">= 0"])
      s.add_runtime_dependency(%q<uvrb>, [">= 0"])
      s.add_development_dependency(%q<rake>, [">= 0.9.2"])
      s.add_development_dependency(%q<rr>, [">= 1.0.0"])
      s.add_development_dependency(%q<timecop>, [">= 0.3.0"])
      s.add_development_dependency(%q<jeweler>, [">= 1.0.0"])
      s.add_development_dependency(%q<rake>, [">= 0.9.2"])
      s.add_development_dependency(%q<rr>, [">= 1.0.0"])
      s.add_development_dependency(%q<timecop>, [">= 0.3.0"])
      s.add_development_dependency(%q<jeweler>, [">= 1.0.0"])
      s.add_development_dependency(%q<rake>, [">= 0.9.2"])
      s.add_development_dependency(%q<rr>, [">= 1.0.0"])
      s.add_development_dependency(%q<timecop>, [">= 0.3.0"])
      s.add_development_dependency(%q<jeweler>, [">= 1.0.0"])
      s.add_runtime_dependency(%q<msgpack>, ["~> 0.4.4"])
      s.add_runtime_dependency(%q<json>, [">= 1.4.3"])
      s.add_runtime_dependency(%q<yajl-ruby>, ["~> 1.0"])
      s.add_runtime_dependency(%q<http_parser.rb>, ["~> 0.5.1"])
      s.add_development_dependency(%q<rake>, [">= 0.9.2"])
      s.add_development_dependency(%q<rr>, [">= 1.0.0"])
      s.add_development_dependency(%q<timecop>, [">= 0.3.0"])
      s.add_development_dependency(%q<jeweler>, [">= 1.0.0"])
    else
      s.add_dependency(%q<fluentd>, [">= 0"])
      s.add_dependency(%q<uvrb>, [">= 0"])
      s.add_dependency(%q<rake>, [">= 0.9.2"])
      s.add_dependency(%q<rr>, [">= 1.0.0"])
      s.add_dependency(%q<timecop>, [">= 0.3.0"])
      s.add_dependency(%q<jeweler>, [">= 1.0.0"])
      s.add_dependency(%q<rake>, [">= 0.9.2"])
      s.add_dependency(%q<rr>, [">= 1.0.0"])
      s.add_dependency(%q<timecop>, [">= 0.3.0"])
      s.add_dependency(%q<jeweler>, [">= 1.0.0"])
      s.add_dependency(%q<rake>, [">= 0.9.2"])
      s.add_dependency(%q<rr>, [">= 1.0.0"])
      s.add_dependency(%q<timecop>, [">= 0.3.0"])
      s.add_dependency(%q<jeweler>, [">= 1.0.0"])
      s.add_dependency(%q<msgpack>, ["~> 0.4.4"])
      s.add_dependency(%q<json>, [">= 1.4.3"])
      s.add_dependency(%q<yajl-ruby>, ["~> 1.0"])
      s.add_dependency(%q<http_parser.rb>, ["~> 0.5.1"])
      s.add_dependency(%q<rake>, [">= 0.9.2"])
      s.add_dependency(%q<rr>, [">= 1.0.0"])
      s.add_dependency(%q<timecop>, [">= 0.3.0"])
      s.add_dependency(%q<jeweler>, [">= 1.0.0"])
    end
  else
    s.add_dependency(%q<fluentd>, [">= 0"])
    s.add_dependency(%q<uvrb>, [">= 0"])
    s.add_dependency(%q<rake>, [">= 0.9.2"])
    s.add_dependency(%q<rr>, [">= 1.0.0"])
    s.add_dependency(%q<timecop>, [">= 0.3.0"])
    s.add_dependency(%q<jeweler>, [">= 1.0.0"])
    s.add_dependency(%q<rake>, [">= 0.9.2"])
    s.add_dependency(%q<rr>, [">= 1.0.0"])
    s.add_dependency(%q<timecop>, [">= 0.3.0"])
    s.add_dependency(%q<jeweler>, [">= 1.0.0"])
    s.add_dependency(%q<rake>, [">= 0.9.2"])
    s.add_dependency(%q<rr>, [">= 1.0.0"])
    s.add_dependency(%q<timecop>, [">= 0.3.0"])
    s.add_dependency(%q<jeweler>, [">= 1.0.0"])
    s.add_dependency(%q<msgpack>, ["~> 0.4.4"])
    s.add_dependency(%q<json>, [">= 1.4.3"])
    s.add_dependency(%q<yajl-ruby>, ["~> 1.0"])
    s.add_dependency(%q<http_parser.rb>, ["~> 0.5.1"])
    s.add_dependency(%q<rake>, [">= 0.9.2"])
    s.add_dependency(%q<rr>, [">= 1.0.0"])
    s.add_dependency(%q<timecop>, [">= 0.3.0"])
    s.add_dependency(%q<jeweler>, [">= 1.0.0"])
  end
end

