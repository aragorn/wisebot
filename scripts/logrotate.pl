#!/usr/bin/perl

use Getopt::Std;
use strict;

my ( $file, $file_size, $count );
$count = 5;

sub BEGIN {
	$Getopt::Std::STANDARD_HELP_VERSION = 1;
	our $VERSION = "1.0";
}

sub HELP_MESSAGE {
	print <<END;
Usage $0 [-s size[k|M|G]] file
    log_file - lotate target file
Options:
	-n count - lotate count. default is $count
    -s size  - execute lotate when file size is larger then size.
	           k: kilo, M: mega, G: giga bytes
END
	exit;
}

HELP_MESSAGE unless( getopts( "n:s:" ) );
our ( $opt_n, $opt_s );

$count = $opt_n if $opt_n;
die "invalid count: $count\n" unless ( $count =~ /\d+/);

$file = shift;
die "need file\n" unless $file;

open( FH, $file ) || die "file is not exists\n";
close FH;

if ( $opt_s ) {
	my ( $unit, $current_size );
	( $file_size, $unit ) = ( $opt_s =~ /^(\d+)(K|k|M|m|G|g)*$/);
	$file_size or die "invalid size format: $opt_s\n";

	if ( $unit eq "K" or $unit eq "k" ) { $file_size = $file_size * 1024; }
	elsif ( $unit eq "M" or $unit eq "m" ) { $file_size = $file_size * 1024 * 1024; }
	elsif ( $unit eq "G" or $unit eq "g" ) { $file_size = $file_size * 1024 * 1024 * 1024; }

	open( FH, $file ) || die "file is not exists\n";
	seek( FH, 0, 2 );
	$current_size = tell FH;
	close FH;

	print "$file_size, $current_size\n";
	die "no need to rotate\n" if ( $current_size < $file_size );
}
else { $file_size = 0; }

my $i;

system( "rm -f $file.$count > /dev/null 2>&1" );
for ( $i = $count; $i > 0; $i-- ) {
	my $j = $i-1;
	system( "mv $file.$j $file.$i > /dev/null 2>&1" );
}

system( "mv $file $file.0 > /dev/null 2>&1" );

