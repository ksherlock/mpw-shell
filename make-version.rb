#!/usr/bin/env ruby -w

# make-version version

def q(x)
	# quote a string
	'"' + x.gsub(/[\"]/, '\\\1' ) + '"'
end

unless ARGV.length == 1
	puts("Usage: make-version version")
	exit(1)
end


VERSION = ARGV[0]

File.open("version.h", "w") {|f|

	f.puts("#ifndef __version_h__")
	f.puts("#define __version_h__")
	f.puts("#define VERSION #{q(VERSION)}")
	f.puts("#define VERSION_DATE #{q(Time.new.ctime)}")
	f.puts("#endif")
}

ok = system(*%w(cmake --build build))
ok = system(*%w(git add version.h))
ok = system(*%w(git commit -m), "Bump Version: #{VERSION}")
ok = system(*%w(git tag), "r#{VERSION}")
exit 0