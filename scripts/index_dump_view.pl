#!/usr/bin/perl

use strict;
use Carp;         # detailed die message

my @opts;
my @values;
my $arg;

while ( $arg = shift ) {
	if ( substr( $arg, 0, 1 ) eq "-" ) { push @opts, $arg; }
	else { push @values, $arg; }
}

###############################
#      options 정리

my $print_seek = 0;

while ( $arg = shift @opts ) {
	$print_seek = 1 if ( $arg eq "-s" );
}

my $filename = @values[0];
my $docid = @values[1];

if ( !$filename or !$docid ) {
	print << "END";
usage: $0 [-s] file docid
options
 -s : print seek message
END
	exit;
}

die "invalid docid: $docid\n" if ( $docid <= 0 );

open FH, $filename or die $!;

my $data = {
	docid => 0,
	size => 0,
	data => ""
};

my $last_printed_time = 0;
my $current_time = 0;

while ( load() ) {
	last if ( $docid == $data->{docid} );

	$current_time = time;
	if ( $last_printed_time != $current_time ) {
		print STDERR "seek[$data->{docid}]\r";
		$last_printed_time = $current_time;
	}
}

my $filepos = tell FH;
close FH;

if ( $docid != $data->{docid} ) {
	print "no data[$docid] in $filename\n";
	exit;
}

print_data();

sub load()
{
	read_int( \$data->{docid} ) or return 0;
	read_int( \$data->{size} ) or return 0;

	if ( $data->{docid} == $docid ) {
		read_data( \$data->{data}, $data->{size} ) or return 0;
	}
	else {
		seek FH, $data->{size}, 1;
	}
		
}

sub read_int()
{
	my $rdata = shift;
	my $reuslt;

	read_fully( $rdata, 4 ) or return 0;
	$$rdata = unpack( "l*", $$rdata );

	return 1;
}

sub read_data()
{
	return read_fully( shift, shift );
}

sub read_fully()
{
    # $len = 읽은 전체 길이, $read_len = read 함수가 읽은 길이
    # $data_len = 전체 읽어야 할 길이

    my ( $len, $read_len, $data ) = 0;
    my $rresult = shift;
    my $data_len = shift;

    if ( $data_len <= 0 ) {
        print "invalid length: $data_len\n";
        return 0;
    }

    while ( $len < $data_len ) {
        $read_len = read FH, $data, ( $data_len - $len ), $len;

        if ( $read_len == 0 ) { return 0; }
        elsif ( $read_len < 0 ) {
            print $!, "\n";
            return 0;
        }

        $len += $read_len;
    }

    $$rresult = $data;

	return 1;
}

sub print_data()
{
	my $data_length = 56;
	my $word = {
		raw_data  => "",
		word      => "",
		pos       => "",
		bytepos   => "",
		attribute => "",
		field     => "",
		len       => ""
	};

	my $i = 0;
	while ( $i*$data_length < $data->{size} ) {
		$word->{raw_data} = substr $data->{data}, $i*$data_length, $data_length;
		load_data( $word );

		print "docid[$docid] word index: $i ";
		print "file posision: ", ( $filepos - $data->{size} + $i*$data_length ), "\n";
   
		printf "word:      [%40s]\n", $word->{word};
		printf "pos:       [%40s]\n", $word->{pos};
		printf "bytepos:   [%40s]\n", $word->{bytepos};
		printf "attribute: [%40s]\n", $word->{attribute};
		printf "field:     [%40s]\n", $word->{field};
		printf "len:       [%40s]\n", $word->{len};
		$i++;
	}
}

sub print_raw
{
	print unpack( "c*", $data->{raw_data} );
}

sub load_data
{
    my $word = shift;

    $word->{word}       = substr $word->{raw_data}, 0, 40;
    $word->{pos}        = substr $word->{raw_data}, 40, 4;
    $word->{bytepos}    = substr $word->{raw_data}, 44, 4;
    $word->{attribute}  = substr $word->{raw_data}, 48, 4;
    $word->{field}      = substr $word->{raw_data}, 52, 2;
    $word->{len}        = substr $word->{raw_data}, 54, 2;

    $word->{word}      = remove_zero( $word->{word} );
    $word->{pos}       = unpack( "l", $word->{pos}       ) if ( $word->{pos}       );
    $word->{bytepos}   = unpack( "l", $word->{bytepos}   ) if ( $word->{bytepos}   );
    $word->{attribute} = remove_zero( $word->{attribute} );
    $word->{field}     = unpack( "v", $word->{field}     ) if ( $word->{field}     );
    $word->{len}       = unpack( "v", $word->{len}       ) if ( $word->{len}       );
}

sub remove_zero
{
	my $string = shift;
	my $zero_index = index $string, chr(0);
	$string = substr $string, 0, $zero_index if ( $zero_index >= 0 );
	return $string;
}
