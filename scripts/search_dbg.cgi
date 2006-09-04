#!/usr/bin/perl -w
use strict;
use CGI;

my $cvs_id = q( $Id$ );
my $q = new CGI;
my $target = $q->param("target") || "http://localhost:3000/search/search";
my $query  = $q->param("query")  || "";
my $submit = $q->param("submit") || "";
my $result = "";

if ($submit eq "ok")
{
  $result = "$target + $query";
  print $q->header(-type=>"text/xml", -charset=>'euc-kr');
  print <<END;
hello, world, submit = $submit;
END
  exit;
} else {

print $q->header(-type=>"text/html", -charset=>'euc-kr');
print <<END;
<html>
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

* { outline: 1px dotted red; padding: 1px}
* * { outline: 1px dotted green; padding: 3px }
* * * { outline: 1px dotted orange; padding: 3px }
* * * * { outline: 1px dotted blue; padding: 3px }
* * * * * { outline: 2px solid red; padding: 3px }
/*
* * * * * * { outline: 2px solid green }
* * * * * * * { outline: 2px solid orange }
* * * * * * * * { outline: 2px solid blue }
*/

--></style>
</head>
<body>
<h3>WiseBot Search Debug <small>($cvs_id)</small></h3>

<form id="search" action="#" style="float:left;" target="result">
<!--div style="float:left; width:60px"> URL </div-->
<div style="float:left;"> <input type="text" name="target" size="50" value="$target"/> </div>
<br style="clear:left">

<!--div style="float:left; width:60px"> Query </div-->
<div style="float:left;"> <textarea name="query" rows="10" cols="50">$query</textarea> </div>
<br style="clear:left">

<!--div style="float:left; width:60px"> </div-->
<div style="float:left"> <input type="submit" value=" Search "/> </div>
<input type="hidden" name="submit" value="ok"/>
</form>

<div style="float:right;">
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



<br style="clear:left">
<iframe name="result" width="100%" height="100%"> </iframe>
</body>
END

print q( <address> $Id$ </address> </html> );

}
