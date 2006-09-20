<?xml version="1.0" encoding="euc-kr"?>
<!DOCTYPE xsl:stylesheet [
	<!ENTITY nbsp "&#160;">
]> 

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="html" version="4.0" encoding="x-windows-949" indent="yes"/>

<xsl:template match="xml">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<title>WiseBot Search Results</title>
<style type="text/css"><!--
BODY {
 font-family : Helvetica, sans-serif;
 font-size   : 10pt;
}

table tr td {
  border: 1px solid #444488;
  padding: 1px 3px 3px 1px;;
  spacing: 1px;
}

* { outline: 1px dotted red; padding: 1px}
* * { outline: 1px dotted green; padding: 3px }
* * * { outline: 1px dotted orange; padding: 3px }
* * * * { outline: 1px dotted blue; padding: 3px }
/* FIXME uncomment when you debug.
* * * * * { outline: 2px solid red; padding: 3px }
* * * * * * { outline: 2px solid green }
* * * * * * * { outline: 2px solid orange }
* * * * * * * * { outline: 2px solid blue }
*/

--></style>
</head>

<body>
<table>
<tr><th>소요시간</th><td><xsl:value-of select="loading_time"/></td></tr>
<tr><th>엔진시간</th><td><xsl:value-of select="elapsed_time"/> ms</td></tr>
<tr><th>검색질의</th><td><xsl:value-of select="query"/></td></tr>
<tr><th>분석단어</th><td><xsl:value-of select="parsed_query"/></td></tr>
<tr><th>문서수</th><td><xsl:value-of select="number(result_count)"/></td></tr>
</table>
<xsl:apply-templates select="vdocs"/>
<!--
<xsl:apply-templates select="query"/>
<xsl:apply-templates select="parsed_query"/>
<xsl:apply-templates select="elapsed_time"/>
-->
</body>
</html>
</xsl:template>

<xsl:template match="vdocs">
<ol>
  <xsl:for-each select="vdoc">
  <li> vid = <xsl:value-of select="@vid"/>,
       node_id = <xsl:value-of select="@vid"/>,
       relevance = <xsl:value-of select="@relevance"/> <br/>
       <xsl:apply-templates select="docs"/> 
  </li>
  </xsl:for-each>
</ol>
</xsl:template>

<xsl:template match="docs">
  <xsl:for-each select="doc">
    <b>doc_id</b> <xsl:value-of select="@doc_id"/> <br/>
    <xsl:apply-templates/> 
  </xsl:for-each>
</xsl:template>

<xsl:template match="field">
  <b><xsl:value-of select="@name"/></b> - 
  <xsl:value-of select="text()"/>
  <!--
  <xsl:variable name="field_text" select="text()"/>
  <xsl:value-of disable-output-escaping="yes" select="{$field_text}"/>
  -->
<br/>
</xsl:template>

</xsl:stylesheet>

