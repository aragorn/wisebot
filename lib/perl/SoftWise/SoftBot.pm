# $Id$
##############################################################################
# SoftBot Search Client v0.9
# First Coded on (Oct-01-2000)
# Last Modified on (May-28-2001)
# by Jung-gyum Kim <aragorn@softwise.co.kr>

# History
# v0.95 - added send_nonb(), recv_nonb() for better timeout control
# v0.9b - removed a bug in regist_doc
# v0.9 - connect,recv,send timeout and debug log
# v0.5 - low-level socket manipulation for timeout

package SoftWise::SoftBot;

BEGIN {
	use Exporter	();
	use vars	qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

	$VERSION = do { my @r=("0.9"=~/\d+/g); sprintf "%d."."%02d"x$#r, @r };
	@ISA = qw(Exporter);
	@EXPORT = qw();
	@EXPORT_OK  = qw();
	%EXPORT_TAGS = ();

}

use strict;
use IO::File;
use Carp;
use IO::Socket;
use Socket;
use Fcntl;
use Errno qw(:POSIX);
##############################################################################
#$SIG{PIPE} = 'IGNORE';

my $OS_AIX5 = 1;
my $DEBUG_LEVEL = 0;  # 0-nothing 1-ERROR 2-INFO 3-DEBUG
my $DEBUG_LOG = "/tmp/softbot_api.log";
my $DEBUG_LOG_FH; local *local_fh;
	$DEBUG_LOG_FH = *local_fh; undef *local_fh;
my $TCP_TIMEOUT = 90;
my $EINTR = 4;
my $SYS_gettimeofday = 78;

my $OP_REGISTER_DOC = "103";
my $OP_REGISTER_DOC_M = "153";
my $OP_DELETE_DOC = "402";
my $OP_DELETE_OID = "404";
my $OP_IQ_OID = "205";
my $OP_SR = "700";
my $OP_SR_FILTER = "710";
my $OP_AS_RQ_TXT = "903";
my $OP_AS_RQ_WORDS = "906";
my $OP_GET_DOCID = "920";
my $OP_ACK = "ACK";
my $OP_NAK = "NAK";
my $TAG_CONT = 'C';
my $TAG_END = 'E';
my $DSC_DEF = "7";
my $MAX_SENDSZ = 1024;

local $| = 1;

###############################################################################
sub get_caller {
	my ($package, $filename, $line, undef) = caller(1);
	my (undef, undef, undef, $subroutine) = caller(2);

	$subroutine =~ s|.*::||g;
	$filename =~ s|.*/||g;

	return ($package, $filename, $line, $subroutine);
}

sub debug {
    return if $DEBUG_LEVEL < 3;
	return unless $DEBUG_LOG_FH;

	my ($package, $filename, $line, $subroutine) = get_caller();
	printf $DEBUG_LOG_FH "%s %s %d %s ", (timestr(gettimeofday()), $filename, $line, $subroutine);
	printf $DEBUG_LOG_FH @_;
	printf $DEBUG_LOG_FH "\n";
}

sub info {
    return if $DEBUG_LEVEL < 2;
	return unless $DEBUG_LOG_FH;

	my ($package, $filename, $line, $subroutine) = get_caller();
	printf $DEBUG_LOG_FH "%s %s %d %s ", (timestr(gettimeofday()), $filename, $line, $subroutine);
	printf $DEBUG_LOG_FH @_;
	printf $DEBUG_LOG_FH "\n";
}

sub error {
    return if $DEBUG_LEVEL < 1;
	return unless $DEBUG_LOG_FH;

	my ($package, $filename, $line, $subroutine) = get_caller();
	printf $DEBUG_LOG_FH "%s %s %d %s ", (timestr(gettimeofday()), $filename, $line, $subroutine);
	printf $DEBUG_LOG_FH @_;
	printf $DEBUG_LOG_FH "\n";
}
###############################################################################

sub new {
	my $class = shift;
	my $host = shift;
	my $port = shift;
	my $self = {
		_host => $host,
		_port => $port,
		listcnt => 10,
		wordlist => undef,
		totcnt => undef,
		recvcnt => undef,
		endcnt => undef
	};

	open_debug_log() if $DEBUG_LEVEL;

	$self->{_sock} = tcp_connect($host, $port) or return;

	@{$self->{docinfo}} = ();

	bless $self, $class;
}

sub DESTROY {
	my $self = shift;
	close $self->{_sock};
	debug("-DESTROYED---------------------------");
	close_debug_log() if $DEBUG_LEVEL;
}

##############################################################################
sub tcp_connect {
	my $host = shift;
	my $port = shift;
	local *local_fh;
	my $sock = *local_fh; undef *local_fh;

	debug("TCPConnect");
	if ( $port =~ /\D/ ) { $port = getservbyname($port, 'tcp') }
	return unless $port;
	my $iaddr = inet_aton($host);
	my $paddr = sockaddr_in($port, $iaddr);

	my $proto = getprotobyname('tcp');
	socket($sock, PF_INET, SOCK_STREAM, $proto) or return;
	setsockopt($sock, 6, 0x01, 1) or return; # TCP_NODELAY
	return if set_nonblock($sock) < 0;

	my $rc = connect($sock, $paddr);
	if ( defined $rc and $rc ) {
		debug("connect completed.");
	} elsif ( $! == EINPROGRESS ) { #EINPROGRESS
		info("connect in progress.");
	} else {
		error("connect failed.");
		close $sock;
		return undef;
	}

	my $rset = '';
	vec($rset,fileno($sock),1) = 1; 
	my $wset = $rset;
	$rc = select(undef,$rset,$wset,$TCP_TIMEOUT);
	if ( defined $rc and $rc ) {
		debug("connect(2) completed");
	} else {
		info("connect(2) failed. timeout?");
		close($sock);
		return undef;
	}
	return $sock;
}


sub set_nonblock {
	my $fd = shift;
	my $flags = fcntl($fd, F_GETFL, 0);
	return -1 if ( $flags == -1 );

	return fcntl($fd, F_SETFL, $flags | O_NONBLOCK | O_NDELAY);
}

sub open_debug_log {
	return open($DEBUG_LOG_FH, ">>$DEBUG_LOG");
}

sub log_error {
	return unless $DEBUG_LOG_FH;
	my ($package, $filename, $line, $subroutine) = get_caller();
	printf $DEBUG_LOG_FH "%s %s %d %s ", (timestr(gettimeofday()), $filename, $line, $subroutine);
	printf $DEBUG_LOG_FH @_;
}


sub close_debug_log {
	return close($DEBUG_LOG_FH);
}

sub gettimeofday {
    my $TIMEVAL_T = "LL";
	my $time = pack($TIMEVAL_T, ());
	syscall( $SYS_gettimeofday, $time, 0) != -1 or return;

	return $time;
}

sub timediff {
	my $TIMEVAL_T = "LL";
	my @start = unpack($TIMEVAL_T, shift);
	my @end = unpack($TIMEVAL_T, shift);

	$start[1] /= 1_000_000; $end[1] /= 1_000_000;

	return sprintf("%.2f", ($end[0] + $end[1])-($start[0]+$start[1]));
}

sub timestr {
	my $TIMEVAL_T = "LL";
	my @timeval = unpack($TIMEVAL_T, shift);
	my @time = localtime($timeval[0]);
	my @month = qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec);

	return sprintf("%s %d %02d:%02d:%02d %3d",
			$month[$time[4]], $time[3], $time[2], $time[1], $time[0],
			int($timeval[1]/1000));
}

##############################################################################

sub search {
	my $self = shift;
	my $query = shift;
	my $ACK;
	my $flags = 0;
	my $s = $self->{_sock};

	# send OP CODE
	$self->TCPSendData($OP_SR) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# send query string
	$self->TCPSendData($query) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;

	return 0 if $ACK eq $OP_NAK;

	# recv word list from server
	( $self->{wordlist} = $self->TCPRecvData or ( $self->{wordlist} eq "" ) ) or return;

	# recv total list count from server
	( $self->{totalcnt} = $self->TCPRecvData or ( $self->{totalcnt} == 0 ) ) or return;

	# recv list count from server
	( $self->{recvcnt} = $self->TCPRecvData or ( $self->{recvcnt} == 0 ) ) or return;

	# recv each list from server
	foreach my $i ( 1..$self->{recvcnt} ) {
		${$self->{docinfo}}[$i-1] = $self->TCPRecvData or return;
	}

	return 1;
	
}

sub search_filter {
	my $self = shift;
	my $query = shift;
	my $ACK;
	my $flags = 0;
	my $s = $self->{_sock};

	# send OP CODE
	$self->TCPSendData($OP_SR_FILTER) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# send query string
	$self->TCPSendData($query) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;

	return 0 if $ACK eq $OP_NAK;

	# recv word list from server
	$self->{wordlist} = $self->TCPRecvData or return;

	# recv total list count from server
	$self->{totcnt} = $self->TCPRecvData or return;

	# recv list count from server
	$self->{recvcnt} = $self->TCPRecvData or return;

	# recv end count from server
	$self->{endcnt} = $self->TCPRecvData or return;

	# recv each list from server
	foreach my $i ( 1..$self->{recvcnt} ) {
		${$self->{docinfo}}[$i-1] = $self->TCPRecvData or return;
	}

	return 1;
}

sub regist_doc_m {
    debug("Regist_Doc_M");
	return regist_doc(shift,shift,shift,$OP_REGISTER_DOC_M);
}

sub regist_doc {
	my $self = shift;
	my $dit = shift;	# DIT
	my $text = shift;	# TEXT
	my $OP_CODE = shift || $OP_REGISTER_DOC;
	debug("Regist_Doc");

	my $ACK;
	my $flags = 0;
	my $s = $self->{_sock};

	# send OP CODE
	$self->TCPSendData($OP_CODE) or return;
	debug("sent OP CODE");

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;
    debug("received ACK for OP CODE");

	# send DIT
	$self->TCPSendData($dit) or return;
	debug("sent DIT");

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# send document text
	$text = "--" unless $text;
	my $str;
	my $i=0; my $n = int ((length($text)-1)/$MAX_SENDSZ)-1;
	if ( length($text) > $MAX_SENDSZ ) {
		foreach $i ( 0..$n ) {
			$str = substr($text,$i*$MAX_SENDSZ,$MAX_SENDSZ);
			$self->TCPSendData($str,$TAG_CONT) or return;
		}
		$str = substr($text,($n+1)*$MAX_SENDSZ,$MAX_SENDSZ);
		$self->TCPSendData($str,$TAG_END) or return;
	} else {
		$self->TCPSendData($text,$TAG_END) or return;
	}
	debug("sent TEXT");

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;
	debug("Regist_Doc ended.");

	return 1;
	
}



sub request_text {
	my $self = shift;
	my $did = shift; # Document ID
	my $file = shift; # Temporary File Name

	# DID가 0보다 큰 정수인지 확인
	return unless $did >= 0;

	my $ACK;
	my $flags = 0;
	my $s = $self->{_sock};

	# send OP CODE
	$self->TCPSendData($OP_AS_RQ_TXT) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# send document id
	$self->TCPSendData($did) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;

	return 0 if $ACK eq $OP_NAK;

	my $fh = IO::File->new_tmpfile() or return;

	# recv text file from server
	$ACK = $self->TCPRecvFile($fh) or return;

	$self->{file} = $fh;
	$self->{wordlist} = "";
	$self->{totcnt} = 0;
	$self->{recvcnt} = 0;

	return 1;
	
}



sub request_words {
	my $self = shift;
	my $did = shift; # Document ID

	# DID가 0보다 큰 정수인지 확인
	return unless $did >= 0;

	my $ACK;
	my $flags = 0;
	my $s = $self->{_sock};

	# send OP CODE
	$self->TCPSendData($OP_AS_RQ_WORDS) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# send document id
	$self->TCPSendData($did) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;

	return 0 if $ACK eq $OP_NAK;

	my $data = $self->TCPRecvData or return;
	$self->{words} = $data;

	return 1;
	
}


sub delete_doc {
	my $self = shift;
	my $did = shift; # Document ID

	# DID가 0보다 큰 정수인지 확인
	return unless $did >= 0;

	my $ACK;
	my $flags = 0;
	my $s = $self->{_sock};

	# send OP CODE
	$self->TCPSendData($OP_DELETE_DOC) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# send document id
	$self->TCPSendData($did) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# recv ACK NAK again for API return value
	$ACK = $self->TCPRecvData or return;

	return 0 if $ACK eq $OP_NAK;

	return 1;
	
}

sub delete_oid {
	my $self = shift;
	my $oid = shift; # Foreign Key, OID

	my $ACK;
	my $flags = 0;
	my $s = $self->{_sock};

	# send OP CODE
	$self->TCPSendData($OP_DELETE_OID) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# send document id
	$self->TCPSendData($oid) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# recv ACK NAK again for API return value
	$ACK = $self->TCPRecvData or return;

	return 0 if $ACK eq $OP_NAK;

	return 1;
	
}

sub inquire_oid {
	my $self = shift;
	my $oid = shift; # Other ID

	my $ACK;
	my $flags = 0;
	my $s = $self->{_sock};

	# send OP CODE
	$self->TCPSendData($OP_IQ_OID) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# send other id
	$self->TCPSendData($oid) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# recv Document ID
	my $did = $self->TCPRecvData or return;
	return $did;
}

sub get_docid {
	my $self = shift;
	my $oid = shift; # Other ID

	my $ACK;
	my $flags = 0;
	my $s = $self->{_sock};

	# send OP CODE
	$self->TCPSendData($OP_GET_DOCID) or return;

	# recv ACK NAK
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# send other id
	$self->TCPSendData($oid) or return;

	# recv ACK NAK (1)
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# recv ACK NAK (2)
	$ACK = $self->TCPRecvData or return;
	return if $ACK eq $OP_NAK;

	# recv Document ID
	my $did = $self->TCPRecvData or return;
	return $did;
}

#############################################################################
sub send_nonb {
	my $sock = shift;
	my $data = shift;
	my $len = shift;
	my $timeout = shift;
	while ( 1 ) {
		#my $n = send($sock, $data, 0|MSG_DONTWAIT);
		my $n = send($sock, $data, 0);
		if ( defined $n and $n > 0 ) {
			last if ($n == $len);
			# partly sent
			$data = substr($data, $n);
			$len -= $n;
			next;
		} elsif ( defined $n and $n == 0 ) {
			# maybe connection error
			debug("connection error?");
			return;
		} else {
			# retry or handle error
			if ($! != EAGAIN && $! != EWOULDBLOCK) {
				error("error: %s", $!);
				return;
			}

			my $wset = ''; vec($wset,fileno($sock),1) = 1; 
			my $rc = select(undef,$wset,undef,$TCP_TIMEOUT);
			if ( $rc == 0 ) {
				# timeout
				error("select() timeout: %s", $!);
				return undef;
			} elsif ( $rc < 0 ) {
				next if $! == $EINTR;
				error("select() error: %s", $!);
				return undef;
			}
		}
	} # while ( 1 )

	return 1;
}

# This code is rewritten in perl from mod_tcp of SoftBotNext.
# Please see also tcp_recv_nonb() in mod_tcp.
sub recv_nonb {
	my $sock = shift;
	my $len = shift;
	my $timeout = shift;

	my ($ret, $data);
	while ( 1 ) {
		my $n;
		if ($OS_AIX5) {
			$n = recv($sock, $data, $len, 0x4000); # 0x4000 = MSG_NONBLOCK
		} else {
			$n = recv($sock, $data, $len, MSG_DONTWAIT);
		}
		#my $n = recv($sock, $data, $len, $flag);
		if ( defined $n and 
			(($n eq "") or ($n and $^V le 5.6.0)) ) {
			if (length($data) == $len) {
				$ret = $data;
				last;
			}
			# partly received
			$ret .= $data;
			$len -= length($data);
			if ( length($data) == 0 ) {
				debug("recv(2) got zero data");
				last;
			}
			next;
		} else {
			# retry or handle error
			if ($! != EAGAIN && $! != EWOULDBLOCK) {
				error("recv(2) failed: %s", $!);
				return;
			}
			my $rset = ''; vec($rset,fileno($sock),1) = 1; 
			my $rc = select($rset,undef,undef,$TCP_TIMEOUT);
			if ( $rc == 0 ) {
				# timeout
				error("recv() timeout(timeout = %d)", $TCP_TIMEOUT);
				return undef;
			} elsif ( $rc < 0 ) {
				next if $! == $EINTR;
				error("select() error: %s", $!);
				return undef;
			}
			# socket is now readable, so continue the loop
		}
	} # while ( 1 )

	return $data;
}

sub TCPSend {
	my $sock = shift;
	my $header = shift;
	my $data = shift;

	send_nonb($sock, $header, length($header), $TCP_TIMEOUT) or return;
	my ($tag, $size) = unpack("aa7", $header);
	$size =~ /(\d+)/; $size = int($1);

	return send_nonb($sock, $data, length($data), $TCP_TIMEOUT);
}

sub TCPRecv {
	my $sock = shift;

	my $header = recv_nonb($sock, 8, $TCP_TIMEOUT) or return;
	my ($tag, $size) = unpack("aa7", $header);
	$size =~ /(\d+)/; $size = int($1);

	return recv_nonb($sock, $size, $TCP_TIMEOUT) or return;
}

sub TCPSendData {
	my $self = shift;
	my $data = shift;
	my $tag = shift || $TAG_END;
	my $sock = $self->{_sock};

	my $header = pack("aa7", $tag, length($data));
	return TCPSend($sock, $header, $data);
}

sub TCPRecvData {
	my $self = shift;
	my $sock = $self->{_sock};

	return TCPRecv($sock);
}


