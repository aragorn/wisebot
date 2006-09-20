<?xml version="1.0" encoding="euc-kr"?>
<!DOCTYPE xsl:stylesheet [
	<!ENTITY nbsp "&#160;">
]> 

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="html" version="4.0" encoding="windows-949-2000" indent="yes"/>

<xsl:template match="/">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<title>검색결과 - WiseBot</title>
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
/* FIXME uncomment when you debug.
* * * * * { outline: 2px solid red; padding: 3px }
* * * * * * { outline: 2px solid green }
* * * * * * * { outline: 2px solid orange }
* * * * * * * * { outline: 2px solid blue }
*/

--></style>
</head>

<body>
<xsl:apply-templates/>
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
  <li> vid = 
    <!--
    <xsl:for-each select="docs/doc">
    <h3><xsl:value-of select="text()"/></h3>
    </xsl:for-each>
    -->
    <xsl:apply-templates/> 
  </li>
  </xsl:for-each>
</ol>
</xsl:template>

<xsl:template match="loading_time">
<b>소요시간:</b> <xsl:apply-templates/><br/>
</xsl:template>

<xsl:template match="status">
<b>상태:</b> <xsl:apply-templates/><br/>
</xsl:template>

<xsl:template match="query">
<b>검색질의:</b> <xsl:apply-templates/><br/>
</xsl:template>

<xsl:template match="parsed_query">
<b>분석단어:</b> <xsl:apply-templates/><br/>
</xsl:template>

<xsl:template match="result_count">
<b>결과건수:</b> <xsl:apply-templates/><br/>
</xsl:template>

<xsl:template match="elapsed_time">
<b>서버소요시간:</b> <xsl:apply-templates/> ms<br/>
</xsl:template>


</xsl:stylesheet>

