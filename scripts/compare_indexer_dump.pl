#!/usr/bin/perl

use strict;
use Carp;        # detailed die message

select STDERR;   # flush output immediately

my @opts;
my @filenames;
my $arg;

while ( $arg = shift ) {
	if ( substr( $arg, 0, 1 ) eq "-" ) { push @opts, $arg; }
	else { push @filenames, $arg; }
}

#######################################
#          options 정리

my $print_data = 0;
my $print_all = 0;
my $print_docid = undef;
my $silence_status = 0;
my $cr = "\n";

while ( $arg = shift @opts ) {
	$print_data = 1 if ( $arg eq "-d" );
	$print_all = 1 if ( $arg eq "-a" );
	$cr = "\r" if ( $arg eq "-r" );
	$silence_status = 1 if ( $arg eq "-s" );

	if ( $arg =~ /^\-docid=(\d*)$/ ) {
		( $print_docid ) = ( $arg =~ /^\-docid=(\d*)$/ ) ;
		$print_all = 1;
	}
}
#######################################

my $file1 = {
	name  => shift @filenames,
	FH    => undef,
	docid => -1,
	size  => -1,
	data  => undef
};

my $file2 = {
	name  => shift @filenames,
	FH    => undef,
	docid => -1,
	size  => -1,
	data  => undef
};

my $current_docid = 1;

if ( !$file1->{name} or !$file2->{name} ) {
	print << "END";
usage: $0 [-d] [-a] [-docid=<docid>] [-r] file1 file2
options
 -d : print detail message
 -a : print all dump data. including normal part.
 -docid=[docid] : print detail data of selected docid.
                  other docid's detail message is not printed.
                  it's meaningless unless used with -a or -d.
 -r : print message using \\r
END
	exit;
}

open_file( $file1 ) or exit;
open_file( $file2 ) or exit;

my $have_error;
my $print_message;
my $last_printed_time = 0;
my $error_doc_count = 0;

while ( load() ) {
	$have_error = 0;
	$print_message = 0;
	my $current_time = time;

	# 1초에 한 번씩만 출력하려고...
	if ( !$silence_status and $current_time != $last_printed_time ) {
		$print_message = 1;
		$last_printed_time = $current_time;
	}
	
	print "cur[$current_docid] err[$error_doc_count] " if ( $print_message );

	if ( $file1->{docid} != $current_docid ) {
		print "cur[$current_docid] err[$error_doc_count] " if ( !$print_message );
		print "$file1->{name}\[$file1->{docid}] is invalid. ";
		$have_error = 1;
	}
	else {
		print "$file1->{name}\[$current_docid] " if ( $print_message );
	}

	if ( $file2->{docid} != $current_docid ) {
		if ( !$have_error and !$print_message ) {
			print "cur[$current_docid] err[$error_doc_count] $file1->{name}\[$file1->{docid}] ";
		}
		print "$file2->{name}\[$file2->{docid}] is invalid\n";
		$have_error = 1;
	}
	else {
		print "$file2->{name}\[$current_docid]$cr" if ( $have_error or $print_message );
	}

	if ( !$have_error and compare_data() > 0 ) {
		print "$file1->{name}\[$current_docid] $file2->{name}\[$current_docid] is different.\n";
		$have_error = 1;
	}

	$current_docid++
		if ( ( !$file1->{docid} or $file1->{docid} >= $current_docid )
				and ( !$file2->{docid} or $file2->{docid} >= $current_docid ) );
	$error_doc_count++ if ( $have_error );
}

if ( !$have_error and !$print_message ) {
		print "$file1->{name}\[$current_docid] $file2->{name}\[$current_docid]\n";
}

print "error count: $error_doc_count\n";

close_file( $file1 );
close_file( $file2 );

sub load
{
	my $result = 0;

	# 둘중에 하나만 성공하면 성공이다.
	$result += load_of( $file1 );
	$result += load_of( $file2 );

	return $result;
}

sub load_of
{
	my $file = shift;

	if ( $file->{docid} >= $current_docid ) { return 1; }

	if ( !$file->{FH} ) {
		$file->{docid} = undef;
		return 0;
	}

	if ( eof $file->{FH} ) {
		close_file( $file );
		set_nextfile( $file );
		open_file( $file ) or return 0;
	}

	read_int( $file->{FH}, \$file->{docid} ) or do { close_file( $file ); return 0; };
	read_int( $file->{FH}, \$file->{size} ) or do { close_file( $file ); return 0; };
	read_data( $file->{FH}, \$file->{data}, $file->{size} ) or do { close_file( $file ); return 0; };

	return 1;
}

sub read_int
{
	my $FH = shift;
	my $rdata = shift;
	my $result;
	
	$result = read_fully( $FH, $rdata, 4 );
	return 0 if ( !$result );

	$$rdata = unpack( "l*", $$rdata );
	return 1;
}

sub read_data
{
	return read_fully( shift, shift, shift );
}

sub read_fully
{
	# $len = 읽은 전체 길이, $read_len = read 함수가 읽은 길이
	# $data_len = 전체 읽어야 할 길이

	my ( $len, $read_len, $data ) = 0;
	my $FH = shift;
	my $rresult = shift;
	my $data_len = shift;

	if ( $data_len <= 0 ) {
		print "invalid length: $data_len\n";
		return 0;
	}

	while ( $len < $data_len ) {
		$read_len = read $FH, $data, ( $data_len - $len ), $len;

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

sub open_file
{
	my $file = shift;
	open $file->{FH}, $file->{name} or
		do {
			print "\ndocid[$current_docid], open failed[$file->{name}]: $!\n";
			$file->{FH} = undef;
			return 0;
		};
	binmode $file->{FH};
	return 1;
}
sub close_file
{
	my $file = shift;

	if ( $file->{FH} ) {
		close $file->{FH};
		$file->{FH} = undef;
	}
}

# $file->{name}의 맨 뒤 2자리 번호를 1증가시킨다.
sub set_nextfile
{
	my $file = shift;
	my $num = ( substr $file->{name}, -2, 2 ) + 1;
	$file->{name} =
		sprintf( "%s%02d", substr( $file->{name}, 0, length( $file->{name} ) - 2 ) , $num );
}

sub compare_data
{
	# 좀 빠르게 해볼라고...
	my $rfile1_data = \$file1->{data};
	my $rfile2_data = \$file2->{data};
	my $rfile1_size = \$file1->{size};
	my $rfile2_size = \$file2->{size};

	return 0 if ( !$print_all and $$rfile1_data eq $$rfile2_data );

	my $data_length = 56;

	my $data1 = {
		raw_data  => "",
		word      => "",
		pos       => "",
		bytepos   => "",
		attribute => "",
		field     => "",
		len       => ""
	};

	my $data2 = {
		raw_data  => "",
		word      => "",
		pos       => "",
		bytepos   => "",
		attribute => "",
		field     => "",
		len       => ""
	};

	my $rdata1_raw_data = \$data1->{raw_data};
	my $rdata2_raw_data = \$data2->{raw_data};

	if ( !$$rfile1_data ) {
		print "$file1->{name} have no data\n";
		return 999999;
	}
	
	if ( !$$rfile2_data ) {
		print "$file2->{name} have no data\n";
		return 999999;
	}

	my $i = 0;
	my $diff_count = 0;
	while ( $i*$data_length < $$rfile1_size or $i*$data_length < $$rfile2_size ) {
		$$rdata1_raw_data = substr $$rfile1_data, $i*$data_length, $data_length;
		$$rdata2_raw_data = substr $$rfile2_data, $i*$data_length, $data_length;

		if ( compare_word( $data1, $data2 ) ) {
			next if ( !$print_all );
		}
		else {
			$diff_count++;
			next if ( !$print_all and !$print_data );
		}

		next if ( $print_docid and $print_docid != $current_docid );

		load_data( $data1 );
		load_data( $data2 );

		print "docid[$current_docid] word index: $i ";
		print "file1 pos: ", ( tell( $file1->{FH} ) - $file1->{size} + $i*$data_length ), " ";
		print "file2 pos: ", ( tell( $file2->{FH} ) - $file2->{size} + $i*$data_length ), "\n";

		printf "word:      [%30s]  [%30s]\n", $data1->{word}, $data2->{word};
		printf "pos:       [%30s]  [%30s]\n", $data1->{pos}, $data2->{pos};
		printf "bytepos:   [%30s]  [%30s]\n", $data1->{bytepos}, $data2->{bytepos};
		printf "attribute: [%30s]  [%30s]\n", $data1->{attribute}, $data2->{attribute};
		printf "field:     [%30s]  [%30s]\n", $data1->{field}, $data2->{field};
		printf "len:       [%30s]  [%30s]\n", $data1->{len}, $data2->{len};
	}
	continue {
		$i++;
	}

	return $diff_count;
}

sub compare_word
{
	my ( $data1, $data2 ) = ( shift, shift );

	my $rdata1_raw_data = \$data1->{raw_data};
	my $rdata2_raw_data = \$data2->{raw_data};

	# bytepos는 0으로 초기화 해버린다.
	# substr말고 raw_data의 45~48을 바로 0으로 초기화 하는 방법을 알면 좋은데...
	$$rdata1_raw_data = substr( $$rdata1_raw_data, 0, 44 )."\0\0\0\0".substr( $$rdata1_raw_data, 48 );
	$$rdata2_raw_data = substr( $$rdata2_raw_data, 0, 44 )."\0\0\0\0".substr( $$rdata2_raw_data, 48 );

	return $$rdata1_raw_data eq $$rdata2_raw_data;

#	return ( $data1->{word} eq $data2->{word} and $data1->{pos} == $data2->{pos}
#	     and $data1->{attribute} eq $data2->{attribute} and $data1->{field} == $data2->{field}
#		 and $data1->{len} == $data2->{len} );
}

sub print_raw
{
	my $data = shift;
	print unpack( "c*", $data->{raw_data} );
}

# data->{raw_data} 를 각 field로...
sub load_data
{
	my $data = shift;

	$data->{word}       = substr $data->{raw_data}, 0, 40;
	$data->{pos}        = substr $data->{raw_data}, 40, 4;
	$data->{bytepos}    = substr $data->{raw_data}, 44, 4;
	$data->{attribute}  = substr $data->{raw_data}, 48, 4;
	$data->{field}      = substr $data->{raw_data}, 52, 2;
	$data->{len}        = substr $data->{raw_data}, 54, 2;

	$data->{word}      = remove_zero( $data->{word} );
	$data->{pos}       = unpack( "l", $data->{pos}       ) if ( $data->{pos}       );
	$data->{bytepos}   = unpack( "l", $data->{bytepos}   ) if ( $data->{bytepos}   );
	$data->{attribute} = remove_zero( $data->{attribute} );
	$data->{field}     = unpack( "v", $data->{field}     ) if ( $data->{field}     );
	$data->{len}       = unpack( "v", $data->{len}       ) if ( $data->{len}       );
}

# \0 제거
sub remove_zero
{
	my $string = shift;
	my $zero_index = index $string, chr(0);
	$string = substr $string, 0, $zero_index if ( $zero_index >= 0 );
	return $string;
}
