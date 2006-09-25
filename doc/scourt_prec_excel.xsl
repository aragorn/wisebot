<?xml version="1.0" encoding="euc-kr"?>
<!DOCTYPE xsl:stylesheet [
	<!ENTITY nbsp "&#160;">
]> 

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="html" version="4.0" encoding="euc-kr" indent="yes"/>
<xsl:param name="target"/>

<xsl:template match="xml">
<html xmlns:o="urn:schemas-microsoft-com:office:office"
xmlns:x="urn:schemas-microsoft-com:office:excel"
xmlns="http://www.w3.org/TR/REC-html40">

<head>
<meta http-equiv="Content-Type" content="text/html; charset=ks_c_5601-1987"/>
<style><xsl:comment>
table
	{mso-displayed-decimal-separator:"\.";
	mso-displayed-thousand-separator:"\,";}
@page
	{margin:1.0in .75in 1.0in .75in;
	mso-header-margin:.5in;
	mso-footer-margin:.5in;}
tr
	{mso-height-source:auto;
	mso-ruby-visibility:none;}
col
	{mso-width-source:auto;
	mso-ruby-visibility:none;}
br
	{mso-data-placement:same-cell;}
.style0
	{mso-number-format:General;
	text-align:general;
	vertical-align:middle;
	white-space:nowrap;
	mso-rotate:0;
	mso-background-source:auto;
	mso-pattern:auto;
	color:windowtext;
	font-size:10.0pt;
	font-weight:400;
	font-style:normal;
	text-decoration:none;
	font-family:돋움, monospace;
	mso-font-charset:129;
	border:none;
	mso-protection:locked visible;
	mso-style-name:표준;
	mso-style-id:0;}
.style21
	{color:blue;
	font-size:10.0pt;
	font-weight:400;
	font-style:normal;
	text-decoration:underline;
	text-underline-style:single;
	font-family:돋움, monospace;
	mso-font-charset:129;
	mso-style-name:하이퍼링크;
	mso-style-id:8;}
a:link
	{color:blue;
	font-size:10.0pt;
	font-weight:400;
	font-style:normal;
	text-decoration:underline;
	text-underline-style:single;
	font-family:돋움, monospace;
	mso-font-charset:129;}
a:visited
	{color:purple;
	font-size:10.0pt;
	font-weight:400;
	font-style:normal;
	text-decoration:underline;
	text-underline-style:single;
	font-family:돋움, monospace;
	mso-font-charset:129;}
td
	{mso-style-parent:style0;
	padding-top:3px;
	padding-right:3px;
	padding-left:3px;
	padding-bottom:3px;
	mso-ignore:padding;
	color:windowtext;
	font-size:10.0pt;
	font-weight:400;
	font-style:normal;
	text-decoration:none;
	font-family:돋움, monospace;
	mso-font-charset:129;
	mso-number-format:General;
	text-align:general;
	vertical-align:middle;
	border:none;
	mso-background-source:auto;
	mso-pattern:auto;
	mso-protection:locked visible;
	mso-text-control:shrinktofit;
	white-space:normal;
	mso-rotate:0;}
.xl24
	{mso-style-parent:style0;
	font-size:10.0pt;}
.xl25
	{mso-style-parent:style0;
	font-size:10.0pt;
	font-weight:700;
	text-align:right;}
.xl26
	{mso-style-parent:style0;
	font-size:10.0pt;
	text-align:center;}
.xl27
	{mso-style-parent:style0;
	font-size:10.0pt;
	font-weight:700;
	text-align:center;
	border-top:.5pt solid windowtext;
	border-right:.5pt dotted windowtext;
	border-bottom:.5pt dotted windowtext;
	border-left:.5pt solid windowtext;}
.xl28
	{mso-style-parent:style0;
	font-size:10.0pt;
	border-top:.5pt solid windowtext;
	border-right:.5pt solid windowtext;
	border-bottom:.5pt dotted windowtext;
	border-left:.5pt dotted windowtext;}
.xl29
	{mso-style-parent:style0;
	font-size:10.0pt;
	font-weight:700;
	text-align:center;
	border-top:.5pt dotted windowtext;
	border-right:.5pt dotted windowtext;
	border-bottom:.5pt dotted windowtext;
	border-left:.5pt solid windowtext;}
.xl30
	{mso-style-parent:style0;
	font-size:10.0pt;
	border-top:.5pt dotted windowtext;
	border-right:.5pt solid windowtext;
	border-bottom:.5pt dotted windowtext;
	border-left:.5pt dotted windowtext;
	white-space:normal;
	mso-text-control:shrinktofit}
.xl31
	{mso-style-parent:style0;
	font-size:10.0pt;
	font-weight:700;
	text-align:center;
	border-top:.5pt dotted windowtext;
	border-right:.5pt dotted windowtext;
	border-bottom:.5pt solid windowtext;
	border-left:.5pt solid windowtext;}
.xl32
	{mso-style-parent:style0;
	font-size:10.0pt;
	border-top:.5pt dotted windowtext;
	border-right:.5pt solid windowtext;
	border-bottom:.5pt solid windowtext;
	border-left:.5pt dotted windowtext;}
.xl33
	{mso-style-parent:style0;
	font-size:10.0pt;
	text-align:center;
	border-top:none;
	border-right:.5pt dotted windowtext;
	border-bottom:none;
	border-left:.5pt dotted windowtext;}
.xl34
	{mso-style-parent:style0;
	font-size:10.0pt;
	text-align:center;
	border-top:none;
	border-right:.5pt dotted windowtext;
	border-bottom:.5pt dotted windowtext;
	border-left:.5pt dotted windowtext;}
.xl35
	{mso-style-parent:style0;
	font-size:10.0pt;
	border-top:none;
	border-right:.5pt dotted windowtext;
	border-bottom:none;
	border-left:.5pt dotted windowtext;
	white-space:normal;
	mso-text-control:shrinktofit;}
.xl36
	{mso-style-parent:style0;
	font-size:10.0pt;
	border-top:none;
	border-right:.5pt dotted windowtext;
	border-bottom:.5pt dotted windowtext;
	border-left:.5pt dotted windowtext;
	white-space:normal;
	mso-text-control:shrinktofit;}
.xl38
	{mso-style-parent:style0;
	color:white;
	font-size:10.0pt;
	font-weight:700;
	text-align:center;
	border:.5pt solid windowtext;
	background:#00CCFF;
	mso-pattern:auto none;}
ruby
	{ruby-align:left;}
rt
	{color:windowtext;
	font-size:8.0pt;
	font-weight:400;
	font-style:normal;
	text-decoration:none;
	font-family:돋움, monospace;
	mso-font-charset:129;
	mso-char-type:none;
	display:none;}

</xsl:comment>
</style>
</head>

<body link="blue" vlink="purple" class="xl24">

<table border="0" cellpadding="0" cellspacing="0" width="700px"
 style='border-collapse:collapse;table-layout:fixed;width:700px'>
 <col class="xl24" Xwidth="100px" style='mso-width-source:userset;mso-width-alt:3757; width:100px'/>
 <col class="xl24" Xwidth="600px" style='mso-width-source:userset;mso-width-alt:22722; width:600px'/>
 <tr height="20" style='mso-height-source:userset;height:15.0pt'>
  <td class="xl27" style='height:15.0pt;'>소요시간</td>
  <td class="xl28" style='border-left:none;'><xsl:value-of select="loading_time"/></td>
 </tr>
 <tr height="20" style='mso-height-source:userset;height:15.0pt'>
  <td height="20" class="xl29" style='height:15.0pt;border-top:none'>엔진시간</td>
  <td class="xl30" style='border-top:none;border-left:none'><xsl:value-of select="elapsed_time"/> ms</td>
 </tr>
 <tr height="60">
  <td height="60" class="xl29" style='border-top:none'>검색질의</td>
  <td class="xl30" style='border-top:none;border-left:none'><xsl:value-of select="query"/></td>
 </tr>
 <tr height="20" style='mso-height-source:userset;height:15.0pt'>
  <td height="20" class="xl29" style='height:15.0pt;border-top:none'>분석단어</td>
  <td class="xl30" style='border-top:none;border-left:none'><xsl:value-of select="parsed_query"/></td>
 </tr>
 <tr height="20" style='mso-height-source:userset;height:15.0pt'>
  <td height="20" class="xl31" style='height:15.0pt;border-top:none'>문서수</td>
  <td class="xl32" style='border-top:none;border-left:none'>총 <xsl:value-of select="number(result_count)"/> 건</td>
 </tr>
 <tr height="27" style='mso-height-source:userset;height:20.25pt'>
  <td height="27" class="xl24" style='height:20.25pt'></td>
  <td class="xl25">㈜소프트와이즈</td>
 </tr>
 <tr height="20" style='mso-height-source:userset;height:15.0pt'>
  <td height="20" class="xl38" style='height:15.0pt'>번호</td>
  <td class="xl38" style='border-left:none'>판시사항 / 판결요지 / 전문</td>
 </tr>

<xsl:apply-templates select="vdocs"/>

</table>
</body>
</html>
</xsl:template>

<xsl:template match="vdocs">
  <xsl:for-each select="vdoc">
    <xsl:call-template name="list_docs">
      <xsl:with-param name="docid" select="@vid"/>
      <xsl:with-param name="relevance" select="@relevance"/>
      <xsl:with-param name="docnum" select="position()"/>
    </xsl:call-template>
  </xsl:for-each>
</xsl:template>

<xsl:template name="list_docs">
  <xsl:param name="docid" select="$docid"/>
  <xsl:param name="relevance" select="$relevance"/>
  <xsl:param name="docnum" select="$docnum"/>

  <xsl:for-each select="docs/doc">
 <tr height="30" style='mso-height-source:userset;height:22.5pt'>
  <td rowspan="4" height="120" class="xl33" style='border-bottom:.5pt dotted black; height:90.0pt'>
    <xsl:value-of select="$docnum"/></td>
  <td class="xl36" style='border-left:none'>
    <xsl:element name="a">
      <xsl:attribute name="target">_blank</xsl:attribute>
      <xsl:attribute name="href"><xsl:value-of select="$target"/>/../../document/select?did=<xsl:value-of select="$docid"/></xsl:attribute>
      <span style="font-size:10.0pt">
      <xsl:value-of select="fields/field[@name='Court']"/>&nbsp;
      <xsl:value-of select="substring(fields/field[@name='PronounceDate'],1,4)"/>.<xsl:value-of
             select="substring(fields/field[@name='PronounceDate'],5,2)"/>.<xsl:value-of
             select="substring(fields/field[@name='PronounceDate'],7,2)"/>&nbsp;
      <xsl:value-of select="fields/field[@name='CaseNum']"/>&nbsp;
      <xsl:value-of select="fields/field[@name='DecisionType']"/>
      <xsl:text>【</xsl:text><xsl:value-of select="fields/field[@name='CaseName']"/><xsl:text>】</xsl:text>
      DocID:<xsl:value-of select="$docid"/> Hit:<xsl:value-of select="$relevance"/>
      </span>
    </xsl:element>
  </td>
 </tr>
    <xsl:apply-templates/>
  </xsl:for-each>
</xsl:template>

<xsl:template match="fields">
 <tr height="30">
  <td height="30" class="xl36" style='border-left:none'>
  판시사항 - <xsl:value-of disable-output-escaping="yes" select="field[@name='Abstract']"/> </td>
 </tr>
 <tr height="30">
  <td height="30" class="xl36" style='border-left:none'>
  판결요지 - <xsl:value-of disable-output-escaping="yes" select="field[@name='JudgementNote']"/> </td>
 </tr>
 <tr height="30">
  <td height="30" class="xl36" style='border-left:none'>
  전    문 - <xsl:value-of disable-output-escaping="yes" select="field[@name='Body']"/> </td>
 </tr>
</xsl:template>

<!--
<table>
  <tr class="header">
    <td width="30">번호</td><td>판시사항/판결요지/전문</td>
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
      <xsl:value-of select="fields/field[@name='Court']"/>&nbsp;
      <xsl:value-of select="substring(fields/field[@name='PronounceDate'],1,4)"/>.<xsl:value-of
             select="substring(fields/field[@name='PronounceDate'],5,2)"/>.<xsl:value-of
             select="substring(fields/field[@name='PronounceDate'],7,2)"/>&nbsp;
      <xsl:value-of select="fields/field[@name='CaseNum']"/>&nbsp;
      <xsl:value-of select="fields/field[@name='DecisionType']"/>
    </xsl:element>
    <xsl:text>【</xsl:text><xsl:value-of select="fields/field[@name='CaseName']"/><xsl:text>】</xsl:text>
    DocID:<xsl:value-of select="$docid"/> Hit:<xsl:value-of select="$relevance"/>
    <br/>
    <xsl:apply-templates/>
  </xsl:for-each>
</xsl:template>

<xsl:template match="fields">
    판시사항 - <xsl:value-of disable-output-escaping="yes" select="field[@name='Abstract']"/><br/>
    판결요지 - <xsl:value-of disable-output-escaping="yes" select="field[@name='JudgementNote']"/><br/>
    전 &nbsp; &nbsp; &nbsp; &nbsp;문 - <xsl:value-of disable-output-escaping="yes" select="field[@name='Body']"/><br/>
</xsl:template>


-->
</xsl:stylesheet>

