<?xml version="1.0" encoding="euc-kr"?>
<!DOCTYPE xsl:stylesheet [
	<!ENTITY nbsp "&#160;">
]> 
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="text" version="4.0" encoding="euc-kr" indent="yes"/>
<xsl:template match="xml">
엔진시간: <xsl:value-of select="elapsed_time"/> ms
분석단어: <xsl:value-of select="parsed_query"/>
문서수  : <xsl:value-of select="number(result_count)"/>
.
<xsl:apply-templates select="vdocs"/>
</xsl:template>

<xsl:template match="vdocs">
  <xsl:for-each select="vdoc">
[<xsl:value-of select="position()"/>] <!--vid = <xsl:value-of select="@vid"/>, node_id = <xsl:value-of select="@vid"/>, relevance = <xsl:value-of select="@relevance"/> -->
  <xsl:apply-templates select="docs"/> 
  <xsl:text>
</xsl:text>
  </xsl:for-each>
</xsl:template>

<xsl:template match="docs">
<xsl:for-each select="doc">
<!--  docid = <xsl:value-of select="@doc_id"/> -->
 <xsl:value-of select="fields/field[@name='Court']"/> - <xsl:value-of select="fields/field[@name='PronounceDate']"/> - <xsl:value-of select="fields/field[@name='CaseNum']"/> - <xsl:value-of select="fields/field[@name='DecisionType']"/> [<xsl:value-of select="fields/field[@name='CaseName']"/>]
 Abstract: <xsl:value-of select="normalize-space(fields/field[@name='B_Abstract'])"/>
 JudgementNote: <xsl:value-of select="normalize-space(fields/field[@name='B_JudgementNote'])"/>
 Body: <xsl:value-of select="normalize-space(fields/field[@name='B_Body'])"/>
</xsl:for-each>
</xsl:template>

<xsl:template match="field">
  [<xsl:value-of select="@name"/>] <xsl:value-of disable-output-escaping="yes" select="text()"/>
</xsl:template>

</xsl:stylesheet>

