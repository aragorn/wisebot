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
my $download    = $q->param("download")    || "";
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
  my $elapsed_time = "<loading_time>" . timestr(timediff($t2,$t1), 'nop') . "</loading_time>";
  my $output;
  if ($r->is_success)
  {
    $output = $r->content;
    $output =~ s/(<xml[^>]*>|<html[^>]*>)/$1 $elapsed_time/i;
    if ($xsl)
    {
      eval {
        my $parser = new XML::LibXML;
        my $xslt = new XML::LibXSLT;
        my $source = $parser->parse_string($output) or die "cannot parse xml result: $!";
        my $style_doc = $parser->parse_file($xsl) or die "cannot parse xsl: $!";
        my $stylesheet = $xslt->parse_stylesheet($style_doc) or die "cannot parse style_doc[$style_doc]: $!";
        my $results = $stylesheet->transform($source, target => "'$target'");
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
  my $content_type = "text/plain";
  $content_type = "text/xml"  if (substr($output,0,150) =~ m/<xml/i);
  $content_type = "text/html" if (substr($output,0,150) =~ m/<html/i);
  $content_type = "application/vnd.ms-excel" if ($download eq "checked");

  print $q->header(-type=>$content_type, -charset=>'euc-kr') unless $download eq "yes";
  print $q->header(-type=>"application/vnd.ms-excel",
                   -charset=>'euc-kr',
                   -content_disposition=>'attachment; filename="scourt_bmt.xls"')
                                                                if $download eq "yes";
  print <<END;
$output
END
  exit;
} else {

print $q->header(-type=>"text/html", -charset=>'cp949', -expires => '-1y');
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

TEXTAREA {
 font-size   : 9pt;
}

/* FIXME uncomment when you debug.
* { border: 1px dotted red; padding: 0px}
* * { border: 1px dotted green; padding: 1px }
* * * { border: 1px dotted orange; padding: 1px }
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

<form id="search" action="#" style="float:left; width:98%" target="result" method="post">
<div style="float:left;"> <input type="text" name="target" size="90" value="$target"/> </div> <br style="clear:left"/>
<div style="float:left; width:100%;">
  <textarea name="query" rows="10" cols="80" style="width:100%;">$query</textarea> </div> <br style="clear:left"/>
<div style="float:left">
  <input type="text" name="param_name"  size="10" value="metadata"/>
  <input type="text" name="param_value" size="30" value="Abstract#0:11^JudgementNote#1:11^Body#2:11^"/> <br style="clear:left"/>
</div> <!--br style="clear:left"/-->
<div style="float:left">  
  <select name="xsl" style="width:15em">
  <option value="">no xsl</option>
  <option value="search.xsl">search.xsl</option>
  <option value="scourt_prec.xsl">scourt_prec.xsl</option>
  <option value="scourt_prec_excel.xsl">scourt_prec_excel.xsl</option>
  <option value="ma.xsl">ma.xsl</option>
  </select>

  <input type="hidden" name="submit" value="ok"/>
  <input type="submit" value=" Search "/>
  <input type="checkbox" name="download" value="yes"> Download
</div>
</form>

<form name="select_hint">
<div id="htabmenu" style="position:absolute; top:6ex; right:1ex;">
<strong>Hints: </strong>
<input type="radio" name="hint" value="none" checked onClick="show_hint(this.value);"/>none &nbsp;
<input type="radio" name="hint" value="search_hint"  onClick="show_hint(this.value);"/>search &nbsp;
<input type="radio" name="hint" value="ma_hint"      onClick="show_hint(this.value);"/>morph analyze &nbsp;
</div> 
</form>
<div id="search_hint" align="right"
     style="position:absolute; width:90%; top:10ex; right:1ex; float:right; display:none;">
<textarea name="example" rows="15" cols="60" style="width:100%">
SELECT *
SEARCH ���ΰǼ�
ORDER_BY PronounceDate DESC, CaseNum1 DESC, CaseNum2 ASC, CaseNum3 DESC
LIMIT 0,10

SELECT *
SEARCH BIGRAM:���ΰǼ�
ORDER_BY PronounceDate DESC, CaseNum1 DESC, CaseNum2 ASC, CaseNum3 DESC
LIMIT 0,10

SELECT *
SEARCH body:�ſ�
ORDER_BY RELEVANCY DESC
LIMIT 0, 10

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

<div id="ma_hint" align="right"
     style="position:absolute; width:90%; top:10ex; right:1ex; float:right; display:none;">
<textarea name="example" rows="15" cols="60" style="width:100%">
http://192.168.10.21:8600/document/ma?contenttype=xml&detail=&
http://192.168.10.21:8600/document/ma?contenttype=xml&
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
   ���� �� ������ ���մϴ�.

]]></body>
</Document>

<Document>
<body><![CDATA[


]]></body>
</Document>
</textarea>
</div>



<br style="clear:left">
<iframe name="result" width="99%" height="100%"> </iframe>
</body>
END

print q( <address> $Id$ </address> </html> );

}
