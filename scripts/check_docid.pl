#!/usr/bin/perl

use strict;
use Cwd;       # cwd() 현재 directory
use Carp;      # die 출력 상세히
select STDERR; # \r 출력을 강제 flush하기 위해...

my $filename = shift;
my ( $current_docid, $docid, $return_value, $comment ) = ( 1, undef, undef, undef );
my $line = "";
my ( $last_printed_time, $current_time ) = ( 0, 0 );

if ( !$filename ) {
	$filename = get_default_filename();
	print "using file: $filename\n";
}

open FH, $filename or die $!;

while ( 1 ) {
	$line = get_line();

	( $docid, $return_value, $comment ) = ( $line =~ /(\d+)\s*(-?\d+)\s*(.*)\n$/ )
		or do { print "invalid line: $line"; next; };

	if ( $current_docid != $docid ) {
		print "invalid docid[$docid] (result[$return_value]), expected[$current_docid]\n";
		$current_docid = $docid;
	}
	elsif ( $return_value != 1 ) {
		print "docid[$docid] returned $return_value, $comment\n";
	}
	else {
		$current_time = time;
		if ( $last_printed_time < $current_time ) {
			print "docid[$docid] OK               \r";
			$last_printed_time = $current_time;
		}
	}

	$current_docid++;
	$line = "";
}

close FH;

sub get_default_filename
{
	my $pwd = cwd();
	if ( $pwd =~ /\/scripts$/ ) { return "../dat/indexer/index.documents"; }
	else { return "dat/indexer/index.documents"; }
}

sub get_line
{
	my $line = "";
	my $last_result_printed = 0;

	while ( 1 ) {
		while ( $line .= <FH> ) {
			if ( $line =~ /.*\n$/ ) { return $line; }
		}

		if ( !$last_result_printed and $return_value == 1 ) {
			print "docid[$docid] OK - waiting\r";
			$last_result_printed = 1;
		}
		
		sleep 1; # retry 대기
	}
}

