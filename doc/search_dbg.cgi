#!/usr/bin/perl -w
use strict;
use CGI;
use CGI::Util qw(unescape escape);

use Benchmark qw(:hireswallclock);
use LWP::UserAgent;
use HTTP::Request::Common;
use XML::LibXSLT;
use XML::LibXML;

my $cvs_id = q( $Id$ );
my $q = new CGI;
my $target = $q->param("target") || "http://localhost:3000/search/search";
my $query  = $q->param("query")  || "";
my $xsl    = $q->param("xsl")    || "";
my $submit = $q->param("submit") || "";
my $param_name  = $q->param("param_name")  || "";
my $param_value = $q->param("param_value") || "";
my $result = "";

if ($submit eq "ok")
{
  my $ua = LWP::UserAgent->new;
  $ua->agent("WiseBot Client/0.1");

  my $escaped_query = escape($query);
  my $escaped_param_name  = escape($param_name);
  my $escaped_param_value = escape($param_value);
  my $t1 = new Benchmark;
  my $r;
  if ( $target =~ m/search\/search/ )
  {
    $r = $ua->request(GET $target . "?q=" . $escaped_query);
  } else {
    $r = $ua->request(POST $target . "&" . $escaped_param_name . "=" . $escaped_param_value, [ q => $query ] );
  }
  my $t2 = new Benchmark;
  my $output;
  if ($r->is_success)
  {
    $output = $r->content;
    if ($xsl)
    {
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
    }
    #$output = $r->as_string;
  } else {
    $output = $r->error_as_HTML;
    $output .= $target . "&" . $escaped_param_name . "=" . $escaped_param_value;
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
* { border: 1px dotted red; padding: 1px}
* * { border: 1px dotted green; padding: 2px }
* * * { border: 1px dotted orange; padding: 3px }
* * * * { border: 1px dotted blue; padding: 3px }
* * * * * { border: 2px solid red; padding: 3px }
* * * * * * { border: 2px solid green }
* * * * * * * { border: 2px solid orange }
* * * * * * * * { border: 2px solid blue }
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
<div style="float:left;"> <input type="text" name="target" size="100" value="$target"/> </div> <br style="clear:left"/>
<div style="float:left; width:100%;">
  <textarea name="query" rows="10" cols="80" style="width:100%;">$query</textarea> </div> <br style="clear:left"/>
<div style="float:left">
  <input type="text" name="param_name"  size="10" value="metadata"/>
  <input type="text" name="param_value" size="30" value="body#0:11^"/> <br style="clear:left"/>
</div> <br style="clear:left"/>
<div style="float:left">  
  <select name="xsl" style="width:10em">
  <option value="">no xsl</option>
  <option value="search.xsl">search.xsl</option>
  <option value="search.xsl">search.xsl</option>
  <option value="search.xsl">search.xsl</option>
  </select>

  <input type="hidden" name="submit" value="ok"/>
  <input type="submit" value=" Search "/>
</div>
</form>

<form name="select_hint">
<div id="htabmenu" style="position:absolute; top:6ex; right:0;">
<strong>Hints: </strong>
<input type="radio" name="hint" value="none" checked onClick="show_hint(this.value);"/>none &nbsp;
<input type="radio" name="hint" value="search_hint"  onClick="show_hint(this.value);"/>search &nbsp;
<input type="radio" name="hint" value="ma_hint"      onClick="show_hint(this.value);"/>morph analyze &nbsp;
</div> 
</form>
<div id="search_hint" style="position:absolute; top:10ex; right:0; float:right; display:none;">
<textarea name="example" rows="15" cols="100">
----------------------------------------------
�����˻�
----------------------------------------------
SELECT *
SEARCH body:�ſ�
ORDER_BY RELEVANCY DESC
LIMIT 0, 10

----------------------------------------------
���հ˻�
----------------------------------------------
SELECT *
SEARCH body:�ſ�
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
<textarea name="example" rows="15" cols="100">
���¼� �м�

http://192.168.10.21:8600/document/ma?contenttype=text&rawkomatext=
http://192.168.10.21:8600/document/ma?contenttype=text&

header - fieldname#fieldnum:morp_id^
metadata = body#0:11^

<Document>
<body><![CDATA[
������ �ǰ� �����ÿ� ���Ͽ� ���� ���� ������ �ҹ����� å���� ������ �����Ͽ� �� ��� ���ط� ���� ���ع�� å���� ������ ���� ������ ���� ������ �����ϱ� ��ƴ�.
  ���� �ǽ� ��ü�� ���ϴ��� �ǰ� �����ô�, ������ ���Ͽ� �δ��ϴ� '1996. 10.����� ���ο� ������޽ü� ���縦 �Ϸ��Ͽ� �� ä��'�� �������� �Ϳ� �Ұ��ϴ� �� ���̰�, ��� ��>
���� ������ �Ͽ��ٴ� ���� �ƴϹǷ�, ä�������� å���� ���� ���� �������� �ϰ�, ������ ������ Ÿ�ο��� ���ظ� ���� ��쿡�� �����Ǵ� �ҹ����� å���� ���ٰ�� �� �� ���� �Ӹ� ��>
�϶�(���ư� ����� ���캸�Ƶ� �ǰ� �����ð� 1996. 6. 11.�� ����鿡�� ���� ���� ���ο� ������޽ü��� ���� �� 10.���� �ϰ��Ͽ� �ֱ�� �����Ͽ��ٴ� ���� �����ϴ� ���ŷδ� ���ɿ�
���� ���� ���� ����ȣ �� �̽����� �� ���νŹ� ��� �̿ܿ��� ���� ���̴µ�, �̵� ���ŵ� ���� ���� ���� ���� ����ȣ�� &quot;������� 1996. 10. 26.�� ���� ���� ������ ���ο� >������޽ü��� �� �ִ� �Ϳ� �������� ���ο� ���� ���ϵ� �ǰ��� �ǰ� �����ó� �ǰ� ���Ǽ��� ������ �ٰ� ����.

������ 9���� 1�� �μ� 2006�� 3�� 6�� | ���� 1�� ���� 2006�� 3�� 10�������� ������ | �쳽�� ���¿��� ��ȭ�� | ������
 �ڼ��� | å������ �����ϱ�ȹ���� 1�� ��ȿ�� ������ ��ȭ�� | 2�� ������ ������ | 3�� ������ ������ �Ѽ��̵����� ������ ������ ������
  | ��������ȹ �������� ������ ��ȭ�� | ���� ����� �����ָ����� �Źν� ������ �Ǵ�� ���籤 ���¼� �ڽſ� ������ | �������� ������ 
  ���������ͳݻ�� ������ ������ ��̾� | ȫ�� ������ ������ | ���� ������ �̼��� ��ȿ���濵���� ���μ� ����� ���Ҿ� �輺�� | �λ米
  �� �������쳽�� (��)������Ͽ콺 | ���ǵ�� 2000�� 5�� 23�� ��13-1071ȣ�ּ� (121-763)����� ������ ��ȭ1�� 22���� â������ 15����ȭ 
  (02)704-3861 | �ѽ� (02)704-3891���ڿ��� yedam1\@wisdomhouse.co.kr | Ȩ������ www.yedamco.co.kr��� ���� | ���� ȭ�������� | �μ⡤��
  �� (��) ������ 8, 500��ISBN 89-5913-143-1 04810ISBN 89-5913-134-2 (��10��)R�߸��� å�� �ٲ�帳�ϴ�.R�� å�� ����� ���� ü���� ����
   ���� �� ������ ���մϴ�.]]></body>
</Document>

</textarea>
</div>



<br style="clear:left">
<iframe name="result" width="100%" height="50%"> </iframe>
</body>
END

print q( <address> $Id$ </address> </html> );

}
