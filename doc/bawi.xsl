<?xml version="1.0" encoding="euc-kr"?>
<!DOCTYPE xsl:stylesheet [
	<!ENTITY nbsp "&#160;">
]> 

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="html" version="4.0" encoding="euc-kr" indent="yes"/>
<xsl:param name="target"/>

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
<tr><th width="100">소요시간</th><td><xsl:value-of select="loading_time"/></td></tr>
<tr><th width="100">엔진시간</th><td><xsl:value-of select="elapsed_time"/> ms</td></tr>
<tr><th>검색질의</th><td><xsl:value-of select="query"/></td></tr>
<tr><th>분석단어</th><td><xsl:value-of select="parsed_query"/></td></tr>
<tr><th>문서수</th><td>총 <xsl:value-of select="number(result_count)"/> 건</td></tr>
</table>

<xsl:apply-templates select="vdocs"/>


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
      <xsl:with-param name="no" select="position()"/>
    </xsl:call-template>
  </xsl:for-each>
</table>
<xsl:call-template name="list_header"/>

<xsl:call-template name="page_link">
  <xsl:with-param name="query" select="test"/>
  <xsl:with-param name="page_no" select="1"/>
  <xsl:with-param name="last_page" select="10"/>
  <xsl:with-param name="this_page" select="3"/>
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
  <xsl:param name="query" select="$query"/>
  <xsl:param name="last_page" select="$last_page"/>
  <xsl:param name="this_page" select="$this_page"/>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
<tr>

  <xsl:call-template name="page_link">
    <xsl:with-param name="query" select="test"/>
    <xsl:with-param name="page_no" select="1"/>
    <xsl:with-param name="last_page" select="10"/>
    <xsl:with-param name="this_page" select="3"/>
  </xsl:call-template>

</tr>
</table>
</xsl:template>

<xsl:template name="page_link">
  <xsl:param name="query" select="$query"/>
  <xsl:param name="page_no" select="$page_no"/>
  <xsl:param name="last_page" select="$last_page"/>
  <xsl:param name="this_page" select="$this_page"/>

  <xsl:if test="$page_no = $this_page">
	<strong><xsl:value-of select="$page_no"/></strong>
  </xsl:if>
  <xsl:if test="$page_no != $this_page">
    <xsl:element name="a">
	<xsl:attribute name="href">?q=<xsl:value-of select="$query"/>&amp;p=<xsl:value-of select="$page_no"/></xsl:attribute>
	<xsl:value-of select="$page_no"/>
    </xsl:element>
  </xsl:if>

  <xsl:if test="$page_no &lt; $last_page">
    <xsl:call-template name="page_link">
      <xsl:with-param name="query" select="$query"/>
      <xsl:with-param name="page_no" select="$page_no+1"/>
      <xsl:with-param name="last_page" select="$last_page"/>
      <xsl:with-param name="this_page" select="$this_page"/>
    </xsl:call-template>
  </xsl:if>

</xsl:template>

<xsl:template match="b">
  <italic><xsl:apply-templates/></italic>
</xsl:template>

</xsl:stylesheet>

