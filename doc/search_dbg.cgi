#!/usr/bin/perl -w
use strict;
use CGI;
use CGI::Util qw(unescape escape);

use Benchmark qw(:hireswallclock);
use LWP::UserAgent;
use HTTP::Request::Common;

my $cvs_id = q( $Id$ );
my $q = new CGI;
my $target = $q->param("target") || "http://localhost:3000/search/search";
my $query  = $q->param("query")  || "";
my $xslt   = $q->param("xslt")   || "";
my $submit = $q->param("submit") || "";
my $result = "";

if ($submit eq "ok")
{
  my $ua = LWP::UserAgent->new;
  $ua->agent("WiseBot Client/0.1");

  my $escaped_query = escape($query);
  my $t1 = new Benchmark;
  my $r = $ua->request(GET $target . "?q=" . $escaped_query);
  my $t2 = new Benchmark;
  my $output;
  if ($r->is_success)
  {
    $output = $r->content;
    $output =~ s/(<\?xml [^>]*\?>)/$1\n<?xml-stylesheet type="text\/xsl" href="$xslt"?>/i if $xslt;

    #$output = $r->as_string;
  } else {
    $output = $r->error_as_HTML;
  }
  my $elapsed_time = "<loading_time>" . timestr(timediff($t2,$t1), 'nop') . "</loading_time>";
  $output =~ s/(<xml[^>]*>|<html[^>]*>)/$1 $elapsed_time/i;
  my $content_type = "text/plain";
  $content_type = "text/xml"  if (substr($output,0,150) =~ m/<xml/i);
  $content_type = "text/html" if (substr($output,0,150) =~ m/<html/i);

  print $q->header(-type=>$content_type, -charset=>'cp949');
  print <<END;
$output
END
  exit;
} else {

print $q->header(-type=>"text/html", -charset=>'x-windows-949', -expires => '-1y');
print <<END;
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<title>WiseBot Search Debug</title>
<style type="text/css"><!--
BODY {
 font-family : Helvetica, sans-serif;
 font-size   : 10pt;
}

DIV  {
 margin: 3px;
}

/* FIXME uncomment when you debug.
* { outline: 1px dotted red; padding: 1px}
* * { outline: 1px dotted green; padding: 2px }
* * * { outline: 1px dotted orange; padding: 3px }
* * * * { outline: 1px dotted blue; padding: 3px }
* * * * * { outline: 2px solid red; padding: 3px }
* * * * * * { outline: 2px solid green }
* * * * * * * { outline: 2px solid orange }
* * * * * * * * { outline: 2px solid blue }
*/

--></style>
<script language="JavaScript" type="text/javascript"><!--

function hide(id)
{
  var div = eval(document.getElementById(id));
  if (div) div.style.display = "none";
}

function show_hint(id)
{
  hide("search_hint");
  hide("ma_hint");

  var div = document.getElementById(id);
  if (div) div.style.display = "inline";
}
//--></script>
</head>
<body>
<h3>WiseBot Search Debug <small>($cvs_id)</small></h3>

<form id="search" action="#" style="float:left; width:100%" target="result" method="post">
<div style="float:left;"> <input type="text" name="target" size="50" value="$target"/> </div> <br style="clear:left">
<div style="float:left; width:100%;"> <textarea name="query" rows="10" cols="80" style="width:100%;">$query</textarea> </div> <br style="clear:left">
<div style="float:left"> 
  <select name="xslt" style="width:10em">
  <option value="">no xsl</option>
  <option value="search.xsl">search.xsl</option>
  <option value="search.xsl">search.xsl</option>
  <option value="search.xsl">search.xsl</option>
  </select>
</div>
<div style="float:left"> <input type="submit" value=" Search "/> </div>
<input type="hidden" name="submit" value="ok"/>
</form>

<form name="select_hint">
<div id="htabmenu" style="position:absolute; right:0; float:right;">
<strong>Hints: </strong>
<input type="radio" name="hint" value="none"        onClick="show_hint(this.value);"/>none &nbsp;
<input type="radio" name="hint" value="search_hint" onClick="show_hint(this.value);"/>search &nbsp;
<input type="radio" name="hint" value="ma_hint"     onClick="show_hint(this.value);"/>morph analyze &nbsp;
</div> 
</form>
<div id="search_hint" style="position:absolute; top:10ex; right:0; float:right; display:none;">
<textarea name="example" rows="15" cols="50">
----------------------------------------------
본문검색
----------------------------------------------
SELECT *
SEARCH body:거울
ORDER_BY RELEVANCY DESC
LIMIT 0, 10

----------------------------------------------
통합검색
----------------------------------------------
SELECT *
SEARCH body:거울
VIRTUAL_ID BOOKID
(
GROUP_BY BOOKID LIMIT 3
ORDER_BY RELEVANCY DESC
)
ORDER_BY RELEVANCY DESC
LIMIT 2

--------------
SELECT *
SEARCH pattern:softwisezzz
VIRTUAL_ID BOOKID
(
WHERE bookid=2307
GROUP_BY BOOKID LIMIT 2, 5
ORDER_BY RELEVANCY DESC, page
)
OUTPUT_STYLE SOFTBOT4
</textarea>
</div>

<div id="ma_hint" style="position:absolute; top:10ex; right:0; float:right; display:none;">
<textarea name="example" rows="15" cols="50">
형태소 분석
</textarea>
</div>



<br style="clear:left">
<iframe name="result" width="100%" height="50%"> </iframe>
</body>
END

print q( <address> $Id$ </address> </html> );

}
