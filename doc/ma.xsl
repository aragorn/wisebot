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
<title>���¼Һм� ��� [<xsl:value-of select="@count"/>]</title>

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
���ξ� ��: <xsl:value-of select="@count"/> <br/>

<table>
<tr>
<th> �ܾ� </th> <th> ǰ�� </th> <th> ������ġ </th> <th> �ʵ�ID </th>
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
  <xsl:when test="$tag = 'COMP'">:���ո��(COMP)</xsl:when>
  <xsl:when test="$tag = 'NNCG'">:ü��(N):���(N):����(C):�Ϲ�(G)</xsl:when>
  <xsl:when test="$tag = 'NNCV'">:ü��(N):���(N):����(C):����(V)</xsl:when>
  <xsl:when test="$tag = 'NNCJ'">:ü��(N):���(N):����(C):�����(J)</xsl:when>
  <xsl:when test="$tag = 'NNCC'">:ü��(N):���(N):����(C):���ո��(C)</xsl:when>
  <xsl:when test="$tag = 'NNB'">:ü��(N):���(N):����(B)</xsl:when>
  <xsl:when test="$tag = 'NNBU'">:ü��(N):���(N):����(B):����(U)</xsl:when>
  <xsl:when test="$tag = 'NNP'">:ü��(N):���(N):����(P)</xsl:when>
  <xsl:when test="$tag = 'NPP'">:ü��(N):����(P):��Ī(P)</xsl:when>
  <xsl:when test="$tag = 'NPI'">:ü��(N):����(P):����(I)</xsl:when>
  <xsl:when test="$tag = 'NU'">:ü��(N):����(U)</xsl:when>
  <xsl:when test="$tag = 'XSNN'">:����(X):����(S):ü��(N):���(N)</xsl:when>
  <xsl:when test="$tag = 'XSNN'">:����(X):����(S):ü��(N):���(N):����(D)-��:*</xsl:when>
  <xsl:when test="$tag = 'XSNP'">:����(X):����(S):ü��(N):����(P)</xsl:when>
  <xsl:when test="$tag = 'XSNU'">:����(X):����(S):ü��(N):����(U)</xsl:when>
  <xsl:when test="$tag = 'XSNL'">:����(X):����(S):ü��(N):����(PL)-��:*</xsl:when>
  <xsl:when test="$tag = 'XPNN'">:����(X):����(P):ü��(N):���(N)</xsl:when>
  <xsl:when test="$tag = 'XPNU'">:����(X):����(P):ü��(N):����(U)</xsl:when>
  <xsl:when test="$tag = 'PS'">:����(P):�ְ�(S)</xsl:when>
  <xsl:when test="$tag = 'PC'">:����(P):����(C)</xsl:when>
  <xsl:when test="$tag = 'PO'">:����(P):������(O)</xsl:when>
  <xsl:when test="$tag = 'PD'">:����(P):������(D)</xsl:when>
  <xsl:when test="$tag = 'PA'">:����(P):�λ��(A)</xsl:when>
  <xsl:when test="$tag = 'PV'">:����(P):ȣ��(V)</xsl:when>
  <xsl:when test="$tag = 'PN'">:����(P):����(N)</xsl:when>
  <xsl:when test="$tag = 'PX'">:����(P):����(X)</xsl:when>
  <xsl:when test="$tag = 'DA'">:������(D):����(A)</xsl:when>
  <xsl:when test="$tag = 'DI'">:������(D):����(I)</xsl:when>
  <xsl:when test="$tag = 'DU'">:������(D):��(U)</xsl:when>
  <xsl:when test="$tag = 'XSD'">:����(X):����(S):������(D)-��</xsl:when>
  <xsl:when test="$tag = 'AA'">:�λ�(A):����(A)</xsl:when>
  <xsl:when test="$tag = 'AP'">:�λ�(A):����(P)</xsl:when>
  <xsl:when test="$tag = 'AI'">:�λ�(A):����(I)</xsl:when>
  <xsl:when test="$tag = 'AC'">:�λ�(A):����(C)</xsl:when>
  <xsl:when test="$tag = 'AV'">:�λ�(A):����(V):*</xsl:when>
  <xsl:when test="$tag = 'AJ'">:�λ�(A):�����(J):*</xsl:when>
  <xsl:when test="$tag = 'XSA'">:����(X):����(S):�λ�(A)</xsl:when>
  <xsl:when test="$tag = 'XSA'">:����(X):����(S):�λ�(A):��(H)-��:*</xsl:when>
  <xsl:when test="$tag = 'C'">:��ź��(C)</xsl:when>
  <xsl:when test="$tag = 'I'">:����������(I)</xsl:when>
  <xsl:when test="$tag = 'VV'">:���(V):����(V)</xsl:when>
  <xsl:when test="$tag = 'VX'">:���(V):����(V):����(X):*</xsl:when>
  <xsl:when test="$tag = 'VJ'">:���(V):�����(J)</xsl:when>
  <xsl:when test="$tag = 'VX'">:���(V):�����(J):����(X):*</xsl:when>
  <xsl:when test="$tag = 'XSVV'">:����(X):����(S):���ȭ(V):����(V)</xsl:when>
  <xsl:when test="$tag = 'XSVJ'">:����(X):����(S):���ȭ(V):������(J)</xsl:when>
  <xsl:when test="$tag = 'XSVJ'">:����(X):����(S):���ȭ(V):������(J):��(D)-��:*</xsl:when>
  <xsl:when test="$tag = 'XSVJ'">:����(X):����(S):���ȭ(V):������(J):��Ÿ(B)-��,����:*</xsl:when>
  <xsl:when test="$tag = 'EFF'">:���(E):�(F):����(F)</xsl:when>
  <xsl:when test="$tag = 'EFC'">:���(E):�(F):����(C)</xsl:when>
  <xsl:when test="$tag = 'EFN'">:���(E):�(F):���(N)</xsl:when>
  <xsl:when test="$tag = 'EFD'">:���(E):�(F):����(D)</xsl:when>
  <xsl:when test="$tag = 'EFA'">:���(E):�(F):�λ�(A)</xsl:when>
  <xsl:when test="$tag = 'EP'">:���(E):���(P)</xsl:when>
  <xsl:when test="$tag = 'NN?'">:ü��(N):���(N):����(?)</xsl:when>
  <xsl:when test="$tag = 'V?'">:���(V):����(?)</xsl:when>
  <xsl:when test="$tag = 'SS.'">:��ȣ(S):����(S):����(.)</xsl:when>
  <xsl:when test="$tag = 'SS?'">:��ȣ(S):����(S):����ǥ(?)</xsl:when>
  <xsl:when test="$tag = 'SS!'">:��ȣ(S):����(S):����ǥ(!)</xsl:when>
  <xsl:when test="$tag = 'SS,'">:��ȣ(S):����(S):����(,)</xsl:when>
  <xsl:when test="$tag = 'SS/'">:��ȣ(S):����(S):����(/)</xsl:when>
  <xsl:when test="$tag = 'SS:'">:��ȣ(S):����(S):����(:)</xsl:when>
  <xsl:when test="$tag = 'SS;'">:��ȣ(S):����(S):�ݽ���(;)</xsl:when>
  <xsl:when test="$tag = 'SS`'">:��ȣ(S):����(S):���ʵ���ǥ(`)</xsl:when>
  <xsl:when test="$tag = 'SS'">:��ȣ(S):����(S):�����ʵ���ǥ(')</xsl:when>
  <xsl:when test="$tag = 'SS('">:��ȣ(S):����(S):���ʰ�ȣ(()</xsl:when>
  <xsl:when test="$tag = 'SS)'">:��ȣ(S):����(S):�����ʰ�ȣ())</xsl:when>
  <xsl:when test="$tag = 'SS-'">:��ȣ(S):����(S):��ǥ(-)</xsl:when>
  <xsl:when test="$tag = 'SSA'">:��ȣ(S):����(S):����ǥ(A)</xsl:when>
  <xsl:when test="$tag = 'SSX'">:��ȣ(S):����(S):��Ÿ(X)</xsl:when>
  <xsl:when test="$tag = 'SCF'">:��ȣ(S):����(C):�ܱ�(F)-ASCII</xsl:when>
  <xsl:when test="$tag = 'SCH'">:��ȣ(S):����(C):����(H)</xsl:when>
  <xsl:when test="$tag = 'SCD'">:��ȣ(S):����(C):����(D)</xsl:when>
  <xsl:when test="$tag = 'SPACE'">:����(SPACE)</xsl:when>
  <xsl:otherwise>:�𸣴� ǰ��(?)</xsl:otherwise>
</xsl:choose>
</xsl:template>

</xsl:stylesheet>

