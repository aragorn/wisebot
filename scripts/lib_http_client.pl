#!/usr/bin/perl -w

use CGI::Util qw(unescape escape);
use LWP::UserAgent;
use HTTP::Request::Common;
use XML::LibXSLT;
use XML::LibXML;

=rem
lib_search
  - hostname
  - port
  - query

    SELECT *
    SEARCH $str
    ORDER_BY PronounceDate DESC, CaseNum1 DESC, CaseNum2 ASC, CaseNum3 DESC
    LIMIT 0,4
=cut
sub lib_search
{
	my $host = shift;
	my $port = shift;
	my $query = shift;

	my $target = "http://" . $host . ":" . $port . "/search/search";

	my $ua = LWP::UserAgent->new;
	$ua->agent("WiseBot Client/0.1");

	my $escaped_query = escape($query);
	my $r = $ua->request(GET $target . "?q=" . $escaped_query);
	my $output;
	if ($r->is_success)
	{
		$output = $r->content;
		my $xsl = "../doc/search_test.xsl";
		eval {
		  my $parser = new XML::LibXML;
		  my $xslt = new XML::LibXSLT;
		  my $source = $parser->parse_string($output) or die "cannot parse xml result: $!";
			my $style_doc = $parser->parse_file($xsl) or die "cannot parse xsl: $!";
			my $stylesheet = $xslt->parse_stylesheet($style_doc) or die "cannot parse style_doc[$style_doc]: $!";
			my $results = $stylesheet->transform($source);
			$output = $stylesheet->output_string($results);
			#$output =~ s/(<\?xml [^>]*\?>)/$1\n<?xml-stylesheet type="text\/xsl" href="$xslt"?>/i if $xslt;

		};
		$output = "XSL Error: $@" if $@;
		#$output = $r->as_string;
	} else {
		$output = $r->error_as_HTML;
	}
	return $output;
}

sub lib_morph_analyze
{
  my ($host, $port, $header, $body) = @_;

  my $ua = LWP::UserAgent->new;
  $ua->agent("WiseBot Client/0.1");
  my $target = "http://" . $host . ":" . $port . "/document/ma?contenttype=text";
  $target .= "&rawkomatext=" if $USE_RAW_KOMA;

  my $escaped_header = escape($header);
  my $r = $ua->request(POST $target . "&metadata=" . $escaped_header, [ body => $body ] );
  my $output;
  if ($r->is_success)
  {
    $output = $r->content;
    #$output =~ s/(<\?xml [^>]*\?>)/$1\n<?xml-stylesheet type="text\/xsl" href="$xslt"?>/i if $xslt;

    #$output = $r->as_string;
  } else {
    $output = $r->error_as_HTML;
  }

  return $output;

}

1;
