<?xml version="1.0" encoding="cp949"?>
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
<xsl:apply-templates select="query"/>
<xsl:apply-templates select="parsed_query"/>
<xsl:apply-templates select="result_count"/>
<ol>
<xsl:for-each select="vdocs/vdoc">
  <xsl:apply-templates select="vdoc"/> 
</xsl:for-each>
</ol>
</body>
</html>
</xsl:template>

<xsl:template match="vdocs">
  <xsl:for-each select="vdoc">
  <h1><xsl:value-of select="text()"/></h1>
</xsl:template>

<xsl:template match="query">
  <xsl:value-of select="text()"/>
</xsl:template> 

</xsl:stylesheet>
