<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE xsl:stylesheet [
	<!ENTITY nbsp "&#160;">
]> 

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="html" version="4.0" encoding="utf-8" indent="yes"/>
<!--xsl:param name="target"/-->
<xsl:param name="q"/><!-- 질의식 -->
<xsl:param name="eq"/><!-- URL Encoded 질의식 -->
<xsl:param name="p">1</xsl:param><!-- 페이지 -->
<xsl:param name="c">10</xsl:param><!-- 결과수 -->
<xsl:param name="sort">date_desc</xsl:param><!-- 정렬 -->

<xsl:template match="xml">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<title><xsl:value-of select="parsed_query"/> [<xsl:value-of select="number(result_count)"/>]</title>
<link href="/x/skin/default/default.css" rel="stylesheet" type="text/css"/>
<script language="JavaScript" type="text/javascript"><xsl:comment>
function note(id)
{
    var url = "/note.pl?to_default=" + id;
    var options = 'toolbar=0,location=0,status=0,menubar=0,scrollbars=1,resizable=1,width=480,height=300';
    var w = window.open(url, 'bw_message', options);
    w.focus();
}
function view_id(id) {
    var url = "/user/profile.cgi?id="+id;
    var options = 'toolbar=0,location=0,status=0,menubar=0,scrollbars=1,resizable=1,width=600,height=500';
    var w = window.open(url,'bw_view',options);
}
function user(url) {
    var options = 'toolbar=0,location=0,status=0,menubar=0,scrollbars=1,resizable=1,width=600,height=500';
    var w = window.open(url,'bw_view',options);
}
</xsl:comment></script>
<script src="skin/default/bx.js" type="text/javascript"></script>
<script src="http://www.google-analytics.com/urchin.js" type="text/javascript">
</script>
<script type="text/javascript"><xsl:comment>
_uacct = "UA-620537-1";
urchinTracker();
</xsl:comment></script>
<style type="text/css"><xsl:comment>

/* FIXME uncomment when you debug.
* { border: 1px dotted red; padding: 1px}
* * { border: 1px dotted green; padding: 3px }
* * * { border: 1px dotted orange; padding: 3px }
* * * * { border: 1px dotted blue; padding: 3px }
* * * * * { border: 2px solid red; padding: 3px }
* * * * * * { border: 2px solid green }
* * * * * * * { border: 2px solid orange }
* * * * * * * * { border: 2px solid blue }
*/

</xsl:comment></style>
</head>
<body>
<div id="bx">

<table class="summary">
<!--tr><th width="100">소요시간</th><td><xsl:value-of select="loading_time"/></td></tr-->
<tr><th width="100">엔진시간</th><td><xsl:value-of select="elapsed_time"/> ms</td></tr>
<tr><th>검색질의</th><td><xsl:value-of select="query"/></td></tr>
<tr><th>분석단어</th><td><xsl:value-of select="parsed_query"/></td></tr>
<tr><th>문서수</th><td>총 <xsl:value-of select="number(result_count)"/> 건</td></tr>
<!--tr><th>파라메터</th><td>
   q=<xsl:value-of select="$q"/>,
   eq=<xsl:value-of select="$eq"/>,
   p=<xsl:value-of select="$p"/></td></tr-->
</table>
<br/>
<xsl:call-template name="query_form">
  <xsl:with-param name="help">no</xsl:with-param>
</xsl:call-template>
<br/>

<xsl:apply-templates select="vdocs"/>
<br/>
<xsl:call-template name="query_form">
  <xsl:with-param name="help">yes</xsl:with-param>
</xsl:call-template>

<div id="bxmenu">
<a href="bookmark.cgi">즐겨찾기</a> |
<a href="boards.cgi">게시판</a> |
<a href="online.cgi">접속자</a> |
<a href="userlist.cgi">회원</a> |

<a href="adduser.cgi">회원 추가</a> |
<a href="passwd.cgi">비밀번호</a> |
<a href="edsig.cgi">시그너쳐</a> |
<a href="logout.cgi">나가기</a> |
<a href="http://x.bawi.org/bx/" target="_blank">BawiX</a>
</div>

</div>
</body>
</html>
</xsl:template>

<xsl:template match="vdocs">
  <xsl:variable name="start" select="($p - 1) * $c"/>
<!--
last = <xsl:value-of select="ceiling(../result_count div $c)"/>,
start = <xsl:value-of select="$start"/>
-->

<xsl:call-template name="list_header"/>
<table border="0" cellpadding="0" cellspacing="0" width="100%">
<tr valign="bottom">
  <td class="tshead">no</td>
  <td class="tshead" width="99%">title</td>
  <td class="tshead" width="125">name</td>
  <td class="tshead" width="40">date</td>
</tr>
  <xsl:for-each select="vdoc">
    <xsl:call-template name="list_docs">
      <xsl:with-param name="docid" select="@vid"/>
      <xsl:with-param name="relevance" select="@relevance"/>
      <xsl:with-param name="no" select="$start + position()"/>
    </xsl:call-template>
  </xsl:for-each>
</table>
<xsl:call-template name="list_header"/>
<br/>
<!-- 페이지 네비게이션 링크 -->
<xsl:call-template name="page_navi">
  <xsl:with-param name="last_page" select="ceiling(../result_count div $c)"/>
  <xsl:with-param name="this_page"><xsl:value-of select="$p"/></xsl:with-param>
</xsl:call-template>

</xsl:template><!--match="vdocs"-->

<xsl:template name="list_docs">
  <xsl:param name="docid" select="$docid"/>
  <xsl:param name="relevance" select="$relevance"/>
  <xsl:param name="no" select="$no"/>
  <xsl:for-each select="docs/doc">
<tr valign="top">
    <td width="1%">
    <!-- 게시물 번호 링크 -->
    <xsl:element name="a">
      <xsl:attribute name="href">/x/read.cgi?bid=<xsl:value-of select="fields/field[@name='board_id']"/>&amp;aid=<xsl:value-of select="fields/field[@name='article_id']"/></xsl:attribute>
	  <xsl:value-of select="$no"/>
    </xsl:element>
	</td>
    <!-- 게시물 제목 링크 -->
	<td width="99%">
    <xsl:element name="a">
      <xsl:attribute name="href">/x/read.cgi?bid=<xsl:value-of select="fields/field[@name='board_id']"/>&amp;aid=<xsl:value-of select="fields/field[@name='article_id']"/></xsl:attribute>
	  <xsl:value-of select="fields/field[@name='article_no']"/> -
	  <xsl:value-of select="fields/field[@name='title']"/>
    </xsl:element>
	</td>
	<td style="white-space:nowrap">
	<!-- 작성자 이름 링크 - 개인정보 조회 -->
    <xsl:element name="a">
      <xsl:attribute name="href">javascript:view_id('<xsl:value-of select="fields/field[@name='userid']"/>')</xsl:attribute>
	  <xsl:value-of select="fields/field[@name='author']"/>
    </xsl:element><!-- 작성자 아이디 링크 - 쪽지 보내기 -->(<xsl:element name="a">
      <xsl:attribute name="href">javascript:note('<xsl:value-of select="fields/field[@name='userid']"/>')</xsl:attribute>
	  <xsl:value-of select="fields/field[@name='userid']"/>
    </xsl:element>)
	</td>
	<td style="white-space:nowrap">
	<xsl:value-of select="substring(fields/field[@name='date'],3,2)"/>/<xsl:value-of select="substring(fields/field[@name='date'],5,2)"/>/<xsl:value-of select="substring(fields/field[@name='date'],7,2)"/>
	</td>
</tr>
<tr>
	<td> </td>
	<td colspan="3">
    ... <xsl:value-of disable-output-escaping="no" select="fields/field[@name='body']"/> ...
	</td>
</tr>
<tr>
	<td class="itemr" colspan="4">
    <xsl:element name="a">
      <xsl:attribute name="href">/x/read.cgi?bid=<xsl:value-of select="fields/field[@name='board_id']"/></xsl:attribute>
	  <xsl:value-of select="fields/field[@name='board']"/>
    </xsl:element>
	-
    <xsl:element name="a">
      <xsl:attribute name="href">/x/boards.cgi?gid=<xsl:value-of select="fields/field[@name='group_id']"/></xsl:attribute>
	  <xsl:value-of select="fields/field[@name='group']"/>
    </xsl:element>
	&nbsp; &nbsp;
    <font color="gray">DocID:<xsl:value-of select="$docid"/> Hit:<xsl:value-of select="$relevance"/></font>
    <xsl:apply-templates/>
	</td>
</tr>
  </xsl:for-each>
</xsl:template>

<xsl:template match="fields">
<!--
  <xsl:for-each select="field">
    <xsl:value-of select="@name"/> - <xsl:value-of disable-output-escaping="no" select="."/><br/>
  </xsl:for-each>
  -->
</xsl:template>

<xsl:template name="list_header">
<table border="0" cellpadding="0" cellspacing="0" width="100%">
<tr>
  <td class="thead" style="white-space:nowrap"><a href="/x/bookmark.cgi">즐겨찾기</a></td>
  <td class="thead" style="white-space:nowrap">--</td>
  <td class="thead" style="white-space:nowrap">--</td>
  <td class="thead" width="98%">&nbsp;</td>
  <td class="thead" style="white-space:nowrap">--</td>
</tr>
</table>
</xsl:template>

<xsl:template name="page_navi">
  <xsl:param name="last_page" select="$last_page"/>
  <xsl:param name="this_page" select="$this_page"/>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
<tr>
<td align="center">
  <xsl:variable name="max_page" select="100"/>
  <xsl:if test="$last_page &lt; $max_page">
    <xsl:call-template name="page_link">
      <xsl:with-param name="start" select="1"/>
      <xsl:with-param name="end"   select="$last_page"/>
      <xsl:with-param name="this"  select="$this_page"/>
    </xsl:call-template>
  </xsl:if>
  <xsl:if test="$last_page &gt; $max_page">
    <xsl:call-template name="page_link">
      <xsl:with-param name="start" select="1"/>
      <xsl:with-param name="end"   select="$max_page"/>
      <xsl:with-param name="this"  select="$this_page"/>
    </xsl:call-template>
  </xsl:if>
</td>
</tr>
</table>
</xsl:template>

<!-- 페이지 네비게이션 링크 생성 -->
<xsl:template name="page_link">
  <xsl:param name="start" select="$start"/>
  <xsl:param name="end"   select="$end"/>
  <xsl:param name="this"  select="$this"/>
  <xsl:variable name="range" select="8"/>

  <xsl:if test="(1 &lt; $start) and ($this - $start = $range)">
    ...
  </xsl:if>
  <xsl:if test="($start = 1) or (
                  ($start - $this &lt; $range) 
				    and 
					($this - $start &lt; $range)
				) or ($start = $end)">
    <xsl:if test="$start = $this">
      &nbsp;<strong>[<xsl:value-of select="$start"/>]</strong>
    </xsl:if>
    <xsl:if test="($start != $this)">
      &nbsp;<xsl:element name="a">
      <xsl:attribute name="href">?q=<xsl:value-of select="$eq"/>&amp;p=<xsl:value-of select="$start"/>&amp;c=<xsl:value-of select="$c"/></xsl:attribute>[<xsl:value-of select="$start"/>]</xsl:element>
    </xsl:if>
  </xsl:if>
  <xsl:if test="($start &lt; $end) and ($start - $this = $range)">
    ...
  </xsl:if>

  <xsl:if test="$start &lt; $end">
    <xsl:call-template name="page_link">
      <xsl:with-param name="start" select="$start+1"/>
      <xsl:with-param name="end"   select="$end"/>
      <xsl:with-param name="this"  select="$this"/>
    </xsl:call-template>
  </xsl:if>

</xsl:template>

<xsl:template name="query_form">
  <xsl:param name="help"  select="$help"/>
<!--input type="hidden" name="bid" value="2078"-->
<!--
-->
<form name="search" method="get" enctype="application/x-www-form-urlencoded">
<table border="0" cellpadding="0" cellspacing="0" align="center">
<tr>
<td class="itemf" valign="top">
  <input type="text" name="q" value="{$q}" class="text" size="60" maxlength="100" onFocus="select()"/>
</td>
<td class="itemf">
  <input type="submit" class="button" name="submit" value="Search" style="width:60px"/>
</td>
<td class="itemf">
&nbsp; &nbsp; &nbsp;
</td>
<td class="itemf">
  건수
  <!--c=<xsl:value-of select="$c"/-->
  <select name="c">
    <xsl:element name="option">
	  <xsl:if test="$c = 10"><xsl:attribute name="selected">true</xsl:attribute></xsl:if>
	  <xsl:attribute name="value">10</xsl:attribute>10</xsl:element>
    <xsl:element name="option">
	  <xsl:if test="$c = 20"><xsl:attribute name="selected">true</xsl:attribute></xsl:if>
	  <xsl:attribute name="value">20</xsl:attribute>20</xsl:element>
    <xsl:element name="option">
	  <xsl:if test="$c = 30"><xsl:attribute name="selected">true</xsl:attribute></xsl:if>
	  <xsl:attribute name="value">30</xsl:attribute>30</xsl:element>
    <xsl:element name="option">
	  <xsl:if test="$c = 40"><xsl:attribute name="selected">true</xsl:attribute></xsl:if>
	  <xsl:attribute name="value">40</xsl:attribute>40</xsl:element>
  </select>
</td>
</tr>
</table>
</form>
  <xsl:if test="$help = 'yes'">
<table border="0" cellpadding="0" cellspacing="0" align="center">
<tr><td class="itemf" colspan="2">
   본문에서만 검색 <br/>
   제목에서만 검색 <br/>
   이름에서 검색 <br/>
   ID에서 검색 <br/>
   연산자 AND <br/>
   연산자 OR <br/>
   연산자 NOT <br/>
   인접연산자 <br/>
   구문검색 <br/>
   연산자 복합<br/>
    </td>
    <td>
   body:(검색 단어) <br/>
   title:(검색 단어) <br/>
   author:(검색 단어) <br/>
   userid:(검색 단어) <br/>
   &amp; <br/>
   + <br/>
   ! <br/>
   /n/ - n = 0,1,2,3,..<br/>
   "검색 단어"<br/>
   title:("검색 단어") + body:(검색 /3/ 단어)<br/>
    </td>
</tr>
</table>
  </xsl:if>
</xsl:template>

<xsl:template match="b">
  <italic><xsl:apply-templates/></italic>
</xsl:template>

</xsl:stylesheet>

