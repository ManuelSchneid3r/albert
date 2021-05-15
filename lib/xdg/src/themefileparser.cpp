// Copyright (C) 2014-2021 Manuel Schneider

#include "themefileparser.h"


XDG::ThemeFileParser::ThemeFileParser(const QString& iniFilePath)
    : iniFile_(iniFilePath, QSettings::IniFormat) {
}



QString XDG::ThemeFileParser::path() {
  return iniFile_.fileName();
}



QString XDG::ThemeFileParser::name() {
  return iniFile_.value("Icon Theme/Name").toString();
}



QString XDG::ThemeFileParser::comment() {
  return iniFile_.value("Icon Theme/Comment").toString();
}



QStringList XDG::ThemeFileParser::inherits() {
  QStringList inherits = iniFile_.value("Icon Theme/Inherits").toStringList();
  if ( inherits.isEmpty() && name() != "hicolor" )
      inherits << "hicolor";
  return iniFile_.value("Icon Theme/Inherits").toStringList();
}



QStringList XDG::ThemeFileParser::directories() {
  return iniFile_.value("Icon Theme/Directories").toStringList();
}



bool XDG::ThemeFileParser::hidden() {
  return iniFile_.value("Icon Theme/Hidden").toBool();
}



int XDG::ThemeFileParser::size(const QString& directory) {
  iniFile_.beginGroup(directory);
  int result = iniFile_.value("Size").toInt();
  iniFile_.endGroup();
  return result;
}



QString XDG::ThemeFileParser::context(const QString& directory) {
  iniFile_.beginGroup(directory);
  QString result = iniFile_.value("Context").toString();
  iniFile_.endGroup();
  return result;
}



QString XDG::ThemeFileParser::type(const QString& directory) {
  iniFile_.beginGroup(directory);
  QString result = iniFile_.contains("Type") ? iniFile_.value("Type").toString()
                                             : "Threshold";
  iniFile_.endGroup();
  return result;
}



int XDG::ThemeFileParser::maxSize(const QString& directory) {
  iniFile_.beginGroup(directory);
  int result = iniFile_.contains("MaxSize") ? iniFile_.value("MaxSize").toInt()
                                            : size(directory);
  iniFile_.endGroup();
  return result;
}



int XDG::ThemeFileParser::minSize(const QString& directory) {
  iniFile_.beginGroup(directory);
  int result = iniFile_.contains("MinSize") ? iniFile_.value("MinSize").toInt()
                                            : size(directory);
  iniFile_.endGroup();
  return result;
}



int XDG::ThemeFileParser::threshold(const QString& directory) {
  iniFile_.beginGroup(directory);
  int result =
      iniFile_.contains("Threshold") ? iniFile_.value("Threshold").toInt() : 2;
  iniFile_.endGroup();
  return result;
}
