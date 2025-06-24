<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:wix="http://schemas.microsoft.com/wix/2006/wi">

  <xsl:output method="xml" indent="yes" />

  <!-- Copy all attributes and elements to the output. -->
  <xsl:template match="@*|*">
    <xsl:copy>
      <xsl:apply-templates select="@*" />
      <xsl:apply-templates select="*" />
    </xsl:copy>
  </xsl:template>

  <!-- Identify MainExecutable -->
  <xsl:key name="exe-search" match="wix:File[contains(@Source, 'owncloud.exe')]" use="@Id" />
  <xsl:template match="wix:File[key('exe-search', @Id)]">
    <xsl:copy>
      <xsl:apply-templates select="@*" />
      <xsl:attribute name="Id">
        <xsl:text>MainExecutable</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates/>
    </xsl:copy>
  </xsl:template>

  <!-- Exclude Shell Extensions -->
  <xsl:key name="shellext-search" match="wix:Component[contains(wix:File/@Source, 'OCContextMenu.dll') or contains(wix:File/@Source, 'OCOverlays.dll')]" use="@Id" />
  <xsl:template match="wix:Component[key('shellext-search', @Id)]" />
  <xsl:template match="wix:ComponentRef[key('shellext-search', @Id)]" />

</xsl:stylesheet>
