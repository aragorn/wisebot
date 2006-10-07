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
SEARCH 도로건설
ORDER_BY PronounceDate DESC, CaseNum1 DESC, CaseNum2 ASC, CaseNum3 DESC
LIMIT 0,10

SELECT *
SEARCH BIGRAM:도로건설
ORDER_BY PronounceDate DESC, CaseNum1 DESC, CaseNum2 ASC, CaseNum3 DESC
LIMIT 0,10

SELECT *
SEARCH body:거울
ORDER_BY RELEVANCY DESC
LIMIT 0, 10

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

원심이 피고 성남시에 대하여 위와 같은 이유로 불법행위 책임의 성립을 인정하여 이 사건 냉해로 인한 손해배상 책임을 인정한 것은 다음과 같은 이유로 수긍하기 어렵다.
  원심 판시 자체에 의하더라도 피고 성남시는, 약정에 기하여 부담하는 '1996. 10.경까지 새로운 용수공급시설 공사를 완료하여 줄 채무'를 불이행한 것에 불과하다 할 것이고, 어떠한 위>
법한 행위를 하였다는 것이 아니므로, 채무불이행 책임을 지게 됨은 별론으로 하고, 위법한 행위로 타인에게 손해를 가한 경우에만 인정되는 불법행위 책임을 진다고는 볼 수 없을 뿐만 아>
니라(나아가 기록을 살펴보아도 피고 성남시가 1996. 6. 11.경 원고들에게 위와 같은 새로운 용수공급시설을 같은 해 10.까지 완공하여 주기로 약정하였다는 점에 부합하는 증거로는 원심에
서의 원고 본인 정일호 및 이승재의 각 본인신문 결과 이외에는 없어 보이는데, 이들 증거들 역시 같은 원고 본인 정일호의 &quot;원고들은 1996. 10. 26.경 에어쇼가 끝날 때까지 새로운 >용수공급시설을 해 주는 것에 동의할지 여부에 관한 통일된 의견을 피고 성남시나 피고 삼대건설에 제시한 바가 없다.

삼한지 9초판 1쇄 인쇄 2006년 3월 6일 | 초판 1쇄 발행 2006년 3월 10일지은이 김정산 | 펴낸이 김태영상무 신화섭 | 편집장
 박선영 | 책임편집 양은하기획편집 1팀 이효선 도은주 성화현 | 2팀 오유미 가정실 | 3팀 최혜진 정지연 한수미디자인 김정숙 하은혜 차기윤
  | 콘텐츠기획 노진선미 이유정 이화진 | 제작 이재승 송현주마케팅 신민식 정덕식 권대관 송재광 임태순 박신용 김형준 | 영업관리 이재희 
  김은실인터넷사업 정은선 왕인정 김미애 | 홍보 김현종 허형식 | 광고 김정민 이세윤 임효구경영지원 하인숙 김범수 봉소아 김성자 | 인사교
  육 송진혁펴낸곳 (주)위즈덤하우스 | 출판등록 2000년 5월 23일 제13-1071호주소 (121-763)서울시 마포구 도화1동 22번지 창강빌딩 15층전화 
  (02)704-3861 | 팩스 (02)704-3891전자우편 yedam1\@wisdomhouse.co.kr | 홈페이지 www.yedamco.co.kr출력 엔터 | 종이 화인페이퍼 | 인쇄·제
  본 (주) 현문값 8, 500원ISBN 89-5913-143-1 04810ISBN 89-5913-134-2 (전10권)R잘못된 책은 바꿔드립니다.R이 책의 내용과 편집 체재의 무단
   전재 및 복제를 금합니다.

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
