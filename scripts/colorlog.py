#!/usr/bin/env python
#$Id$


import re
import sys
import string

BOLD = "\x1B[1m"
BLINK = "\x1B[5m"
RESET = '\x1B[0m'
CLEAR = '\x1B[0m'

RED = '\x1B[31m'
GREEN = '\x1B[32m'
YELLOW = '\x1B[33m'
BLUE = '\x1B[34m'
MAGENTA = '\x1B[35m'
CYAN = '\x1B[36m'
WHITE = '\x1B[37m'
ON_BLACK = '\x1B[40m'
ON_RED = '\x1B[41m'
ON_GREEN = '\x1B[42m'
ON_YELLOW = '\x1B[43m'
ON_BLUE = '\x1B[44m'
ON_MAGENTA = '\x1B[45m'
ON_CYAN = '\x1B[46m'
ON_WHITE = '\x1B[47m'

log_color = {'[debug]':CLEAR, '[info]':CYAN,
			 '[notice]':CYAN, '[warn]':YELLOW,
			 '[error]':RED,   '[crit]': BOLD+WHITE+ON_RED,
			 '[alert]':BOLD+BLINK+WHITE+ON_RED,
			 '[emerg]':BOLD+BLINK+WHITE+ON_RED}

filename=0
function=1
message=2
def colorize(line):
	orig_line = line
	try:
		time = get_first_bracket(line)
		line = remove_first_bracket(line)

		found = 0
		for log in log_color.keys():
			if string.find(line, log) >= 0:
				found = 1
				line = remove_first_bracket(line)
				line = string.strip(line)
				line = string.split(line, ' ',2) ## XXX: magic number 2
				line[filename] = GREEN + line[filename] + CLEAR
				line[message] = log_color[log] + line[message] + CLEAR
				break

		if not found:
			return string.rstrip(orig_line)

		if show_time:
			line.insert(0,time)
	except:
		return string.rstrip(orig_line)

	return string.join(line, ' ')

def get_first_bracket(line):
	return re.findall(r'^[ \t]*\[[^\]]+\]',line)[0]

def remove_first_bracket(line):
	return re.sub(r'^[ \t]*\[[^\]]+\]','',line)

def pretty_output(line):
	line = colorize(line)
	sys.stdout.write(line+'\n')

if __name__ == '__main__':
	global show_time
	import getopt

	usage = 'usage: %s -t(shows time) -h(shows help)'%sys.argv[0]

	try:
		opts, args = getopt.getopt(sys.argv[1:],"th",['time','help'])
	except:
		print usage
		sys.exit(1)

	show_time=0
	for opt in opts:
		if opt[0]=='-h':
			print usage
			sys.exit(0)
		elif opt[0]=='-t':
			show_time=1
		else:
			print usage
			sys.exit(1)

	while 1:
		line = sys.stdin.readline()
		if not line: break
		pretty_output(line)
