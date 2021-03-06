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

<style type="text/css"><xsl:comment>
BODY {
 font-family : Helvetica, sans-serif;
 font-size   : 9pt;
}

table.summary tr th,td {
  background: #EEEEEE;
  padding: 3px;
  spacing: 0px;
}
table.summary tr th {
  background: #EEEEEE;
}
table.summary tr td {
  background: #F0F0F0;
}

table td {
  border: 0;
}

table tr.header td {
  background: #8CA6D7;
  font-weight: bold;
  color: white;
  text-align: center;
}


table tr.result td.result_header {
  background: #F3F3F3;
  text-align: center;
  border-bottom: 1px dashed #666666;
}

table tr.result td.result_body {
  background: #FFFFFF;
  border-bottom: 1px dashed #666666;
}

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
<table class="summary">
<tr><th width="100">소요시간</th><td><xsl:value-of select="loading_time"/></td></tr>
<tr><th width="100">엔진시간</th><td><xsl:value-of select="elapsed_time"/> ms</td></tr>
<tr><th>검색질의</th><td><xsl:value-of select="query"/></td></tr>
<tr><th>분석단어</th><td><xsl:value-of select="parsed_query"/></td></tr>
<tr><th>문서수</th><td>총 <xsl:value-of select="number(result_count)"/> 건</td></tr>
</table>
<xsl:apply-templates select="vdocs"/>
</body>
</html>
</xsl:template>

<xsl:template match="vdocs">
<table>
  <tr class="header">
    <td width="30">번호</td><td>검색결과</td>
  </tr>
  <xsl:for-each select="vdoc">
  <tr class="result">
    <td class="result_header"> <xsl:value-of select="position()"/> </td>
    <td class="result_body">
       <xsl:call-template name="list_docs">
         <xsl:with-param name="docid" select="@vid"/>
         <xsl:with-param name="relevance" select="@relevance"/>
       </xsl:call-template>
    </td>
  </tr>
  </xsl:for-each>
</table>
</xsl:template>

<xsl:template name="list_docs">
  <xsl:param name="docid" select="$docid"/>
  <xsl:param name="relevance" select="$relevance"/>

  <xsl:for-each select="docs/doc">
    <xsl:element name="a">
      <xsl:attribute name="target">_blank</xsl:attribute>
      <xsl:attribute name="href"><xsl:value-of select="$target"/>/../../document/select?did=<xsl:value-of select="$docid"/></xsl:attribute>
    DocID:<xsl:value-of select="$docid"/> Hit:<xsl:value-of select="$relevance"/>
    </xsl:element>
    <xsl:text>【</xsl:text>
       <xsl:value-of select="fields/field[@name='CaseName']"/>
       <xsl:value-of select="fields/field[@name='Title']"/>
       <xsl:value-of select="fields/field[@name='title']"/>
       <xsl:value-of select="fields/field[@name='Subject']"/>
       <xsl:value-of select="fields/field[@name='subject']"/>
    <xsl:text>】</xsl:text>
    <br/>
    <xsl:apply-templates/>
  </xsl:for-each>
</xsl:template>

<xsl:template match="fields">
  <xsl:for-each select="field">
    <xsl:value-of select="@name"/> - <xsl:value-of disable-output-escaping="yes" select="."/><br/>
  </xsl:for-each>
</xsl:template>

<!--
<xsl:template match="docs">
  <table>
  <xsl:for-each select="doc">
    <tr><th>doc id</th><td><a href="/document/select?did="><xsl:value-of select="@doc_id"/></a></td></tr>
    <xsl:apply-templates/>
  </xsl:for-each>
  </table>
</xsl:template>

<xsl:template match="field">
    <tr><th><xsl:value-of select="@name"/></th>
        <td><xsl:value-of disable-output-escaping="yes" select="text()"/></td>
    </tr>
</xsl:template>
-->

<xsl:template match="b">
  <italic><xsl:apply-templates/></italic>
</xsl:template>

</xsl:stylesheet>

