#!/usr/bin/perl

use lib "../lib/perl";
use lib "./lib/perl";
use strict;
use Getopt::Std;
use SoftWise::SoftBot;

sub BEGIN {
	$Getopt::Std::STANDARD_HELP_VERSION = 1;
	our $VERSION = "1.0";
}

sub HELP_MESSAGE {
	print <<END;
Usage: $0 [-p] [-q] [-l] [-t sleep_time] <host> <port> [-/file]
    host  - hostname of search server
    port  - port number that softbot use
    file  - contains query samples. - means stdin
Options:
    -p    - print search result
    -q    - print query
    -l    - restart search infinitely
    -t sleep_time - sleep time(seconds) between query search [default:1]
END
	exit;
}

###################################
#         options Á¤¸®

my ( $print_query, $print_result, $loop );
my $sleep_time = 0;

HELP_MESSAGE() unless( getopts( "qplt:" ) );
our ( $opt_q, $opt_p, $opt_l, $opt_t );

$print_query = ( $opt_q or $opt_p );
$print_result = $opt_p;
$sleep_time = $opt_t if ( $opt_t eq "0" or $opt_t );
$loop = $opt_l;

my $host = shift;
my $port = shift;
my $file = shift;

$file = "-" if ( !$file );

HELP_MESSAGE() unless ($host and $port and $file);
print "host: $host, port: $port, file: $file\n";

do {
	if ( not $file eq "-" ) {
		open STDIN, "<$file" or die $!;
	}

	my $query;
	while ( $query = <STDIN> ) {
		my $sb = new SoftWise::SoftBot($host, $port) or die "cannot connect to softbot: $!\n";

		print "query: $query" if ( $print_query );
		my $ret = $sb->search( $query );
		if ( !$ret ) {
			print "query failed: $query, ret:$ret\n";
			sleep( 1 );
		}

		print_sb( $sb ) if ( $ret and $print_result );
		sleep( $sleep_time );
	}

	close STDIN;
} while ( $loop );

sub print_sb()
{
	my $sb = shift;
	print "==========================================\n";
	print "word list: $sb->{wordlist}\n";
	print "total count: $sb->{totalcnt}\n";
	print "recv count: $sb->{recvcnt}\n";

	foreach my $i ( 0..($sb->{recvcnt}-1) ) {
		print "docinfo[$i] = $sb->{docinfo}[$i]\n";
	}
	print "==========================================\n";
}

