<?xml version="1.0" encoding="euc-kr"?>
<!DOCTYPE xsl:stylesheet [
	<!ENTITY nbsp "&#160;">
]> 

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="html" version="4.0" encoding="euc-kr" indent="yes"/>
<xsl:param name="target"/>

<xsl:template match="items">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<title>형태소분석 결과 [<xsl:value-of select="@count"/>]</title>

<style type="text/css"><xsl:comment>
BODY {
 font-family : Helvetica, sans-serif;
 font-size   : 9pt;
}

table tr th,td {
  text-align: center;
}

table tr td.part {
  text-align: left;
}
</xsl:comment></style>
</head>
<body>
색인어 수: <xsl:value-of select="@count"/> <br/>

<table>
<tr>
<th> 단어 </th> <th> 품사 </th> <th> 어절위치 </th> <th> 필드ID </th>
</tr>
<xsl:for-each select="word">
<xsl:sort select="@field" data-type="number"/>
<xsl:sort select="@pos"   data-type="number"/>
<tr>
<td class="word"> 
 <xsl:choose>
   <xsl:when test="substring(@tag,1,2)  = 'NN'"><big><strong><xsl:value-of select="text()"/></strong></big></xsl:when>
   <xsl:when test="substring(@tag,1,4)  = 'COMP'"><big><strong><xsl:value-of select="text()"/></strong></big></xsl:when>
   <xsl:otherwise> <xsl:value-of select="text()"/> </xsl:otherwise>
 </xsl:choose>
</td>
<td class="part">
 <xsl:choose>
   <xsl:when test="contains($target, 'detail')">
     <xsl:call-template name="part_name"><xsl:with-param name="tag" select="@tag"/></xsl:call-template>
   </xsl:when>
   <xsl:otherwise> <xsl:value-of select="@tag"/> </xsl:otherwise>
 </xsl:choose>
</td>
<td class="position"> <xsl:value-of select="@pos"/> </td>
<td class="field_id"> <xsl:value-of select="@field"/> </td>
</tr>
</xsl:for-each>
</table>
</body>
</html>

</xsl:template>


<xsl:template name="part_name">
<xsl:param name="tag" select="$tag"/>
<xsl:choose>
  <xsl:when test="$tag = 'COMP'">:복합명사(COMP)</xsl:when>
  <xsl:when test="$tag = 'NNCG'">:체언(N):명사(N):보통(C):일반(G)</xsl:when>
  <xsl:when test="$tag = 'NNCV'">:체언(N):명사(N):보통(C):동사(V)</xsl:when>
  <xsl:when test="$tag = 'NNCJ'">:체언(N):명사(N):보통(C):형용사(J)</xsl:when>
  <xsl:when test="$tag = 'NNCC'">:체언(N):명사(N):보통(C):복합명사(C)</xsl:when>
  <xsl:when test="$tag = 'NNB'">:체언(N):명사(N):의존(B)</xsl:when>
  <xsl:when test="$tag = 'NNBU'">:체언(N):명사(N):의존(B):단위(U)</xsl:when>
  <xsl:when test="$tag = 'NNP'">:체언(N):명사(N):고유(P)</xsl:when>
  <xsl:when test="$tag = 'NPP'">:체언(N):대명사(P):인칭(P)</xsl:when>
  <xsl:when test="$tag = 'NPI'">:체언(N):대명사(P):지시(I)</xsl:when>
  <xsl:when test="$tag = 'NU'">:체언(N):수사(U)</xsl:when>
  <xsl:when test="$tag = 'XSNN'">:접사(X):접미(S):체언(N):명사(N)</xsl:when>
  <xsl:when test="$tag = 'XSNN'">:접사(X):접미(S):체언(N):명사(N):관형(D)-적:*</xsl:when>
  <xsl:when test="$tag = 'XSNP'">:접사(X):접미(S):체언(N):대명사(P)</xsl:when>
  <xsl:when test="$tag = 'XSNU'">:접사(X):접미(S):체언(N):수사(U)</xsl:when>
  <xsl:when test="$tag = 'XSNL'">:접사(X):접미(S):체언(N):복수(PL)-들:*</xsl:when>
  <xsl:when test="$tag = 'XPNN'">:접사(X):접두(P):체언(N):명사(N)</xsl:when>
  <xsl:when test="$tag = 'XPNU'">:접사(X):접두(P):체언(N):수사(U)</xsl:when>
  <xsl:when test="$tag = 'PS'">:조사(P):주격(S)</xsl:when>
  <xsl:when test="$tag = 'PC'">:조사(P):보격(C)</xsl:when>
  <xsl:when test="$tag = 'PO'">:조사(P):목적격(O)</xsl:when>
  <xsl:when test="$tag = 'PD'">:조사(P):관형격(D)</xsl:when>
  <xsl:when test="$tag = 'PA'">:조사(P):부사격(A)</xsl:when>
  <xsl:when test="$tag = 'PV'">:조사(P):호격(V)</xsl:when>
  <xsl:when test="$tag = 'PN'">:조사(P):접속(N)</xsl:when>
  <xsl:when test="$tag = 'PX'">:조사(P):보조(X)</xsl:when>
  <xsl:when test="$tag = 'DA'">:관형사(D):성상(A)</xsl:when>
  <xsl:when test="$tag = 'DI'">:관형사(D):지시(I)</xsl:when>
  <xsl:when test="$tag = 'DU'">:관형사(D):수(U)</xsl:when>
  <xsl:when test="$tag = 'XSD'">:접사(X):접미(S):관형사(D)-적</xsl:when>
  <xsl:when test="$tag = 'AA'">:부사(A):성상(A)</xsl:when>
  <xsl:when test="$tag = 'AP'">:부사(A):서술(P)</xsl:when>
  <xsl:when test="$tag = 'AI'">:부사(A):지시(I)</xsl:when>
  <xsl:when test="$tag = 'AC'">:부사(A):접속(C)</xsl:when>
  <xsl:when test="$tag = 'AV'">:부사(A):동사(V):*</xsl:when>
  <xsl:when test="$tag = 'AJ'">:부사(A):형용사(J):*</xsl:when>
  <xsl:when test="$tag = 'XSA'">:접사(X):접미(S):부사(A)</xsl:when>
  <xsl:when test="$tag = 'XSA'">:접사(X):접미(S):부사(A):히(H)-히:*</xsl:when>
  <xsl:when test="$tag = 'C'">:감탄사(C)</xsl:when>
  <xsl:when test="$tag = 'I'">:서술격조사(I)</xsl:when>
  <xsl:when test="$tag = 'VV'">:용언(V):동사(V)</xsl:when>
  <xsl:when test="$tag = 'VX'">:용언(V):동사(V):보조(X):*</xsl:when>
  <xsl:when test="$tag = 'VJ'">:용언(V):형용사(J)</xsl:when>
  <xsl:when test="$tag = 'VX'">:용언(V):형용사(J):보조(X):*</xsl:when>
  <xsl:when test="$tag = 'XSVV'">:접사(X):접미(S):용언화(V):동사(V)</xsl:when>
  <xsl:when test="$tag = 'XSVJ'">:접사(X):접미(S):용언화(V):형동사(J)</xsl:when>
  <xsl:when test="$tag = 'XSVJ'">:접사(X):접미(S):용언화(V):형동사(J):답(D)-답:*</xsl:when>
  <xsl:when test="$tag = 'XSVJ'">:접사(X):접미(S):용언화(V):형동사(J):기타(B)-롭,스럽:*</xsl:when>
  <xsl:when test="$tag = 'EFF'">:어미(E):어말(F):종결(F)</xsl:when>
  <xsl:when test="$tag = 'EFC'">:어미(E):어말(F):연결(C)</xsl:when>
  <xsl:when test="$tag = 'EFN'">:어미(E):어말(F):명사(N)</xsl:when>
  <xsl:when test="$tag = 'EFD'">:어미(E):어말(F):관형(D)</xsl:when>
  <xsl:when test="$tag = 'EFA'">:어미(E):어말(F):부사(A)</xsl:when>
  <xsl:when test="$tag = 'EP'">:어미(E):선어말(P)</xsl:when>
  <xsl:when test="$tag = 'NN?'">:체언(N):명사(N):추정(?)</xsl:when>
  <xsl:when test="$tag = 'V?'">:용언(V):추정(?)</xsl:when>
  <xsl:when test="$tag = 'SS.'">:기호(S):문장(S):온점(.)</xsl:when>
  <xsl:when test="$tag = 'SS?'">:기호(S):문장(S):물음표(?)</xsl:when>
  <xsl:when test="$tag = 'SS!'">:기호(S):문장(S):느낌표(!)</xsl:when>
  <xsl:when test="$tag = 'SS,'">:기호(S):문장(S):반점(,)</xsl:when>
  <xsl:when test="$tag = 'SS/'">:기호(S):문장(S):빗금(/)</xsl:when>
  <xsl:when test="$tag = 'SS:'">:기호(S):문장(S):쌍점(:)</xsl:when>
  <xsl:when test="$tag = 'SS;'">:기호(S):문장(S):반쌍점(;)</xsl:when>
  <xsl:when test="$tag = 'SS`'">:기호(S):문장(S):왼쪽따옴표(`)</xsl:when>
  <xsl:when test="$tag = 'SS'">:기호(S):문장(S):오른쪽따옴표(')</xsl:when>
  <xsl:when test="$tag = 'SS('">:기호(S):문장(S):왼쪽괄호(()</xsl:when>
  <xsl:when test="$tag = 'SS)'">:기호(S):문장(S):오른쪽괄호())</xsl:when>
  <xsl:when test="$tag = 'SS-'">:기호(S):문장(S):줄표(-)</xsl:when>
  <xsl:when test="$tag = 'SSA'">:기호(S):문장(S):줄임표(A)</xsl:when>
  <xsl:when test="$tag = 'SSX'">:기호(S):문장(S):기타(X)</xsl:when>
  <xsl:when test="$tag = 'SCF'">:기호(S):문자(C):외국(F)-ASCII</xsl:when>
  <xsl:when test="$tag = 'SCH'">:기호(S):문자(C):한자(H)</xsl:when>
  <xsl:when test="$tag = 'SCD'">:기호(S):문자(C):숫자(D)</xsl:when>
  <xsl:when test="$tag = 'SPACE'">:공백(SPACE)</xsl:when>
  <xsl:otherwise>:모르는 품사(?)</xsl:otherwise>
</xsl:choose>
</xsl:template>

</xsl:stylesheet>

