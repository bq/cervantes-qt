#! /usr/bin/perl
#
#   This file is part of the WebKit project
#
#   Copyright (C) 1999 Waldo Bastian (bastian@kde.org)
#   Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
#   Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
#   Copyright (C) 2010 Andras Becsi (abecsi@inf.u-szeged.hu), University of Szeged
#
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Library General Public
#   License as published by the Free Software Foundation; either
#   version 2 of the License, or (at your option) any later version.
#
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Library General Public License for more details.
#
#   You should have received a copy of the GNU Library General Public License
#   along with this library; see the file COPYING.LIB.  If not, write to
#   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#   Boston, MA 02110-1301, USA.
use strict;
use warnings;

open NAMES, "<CSSPropertyNames.in" || die "Could not find CSSPropertyNames.in";
my @names = ();
my @aliases = ();
while (<NAMES>) {
  next if (m/(^#)|(^\s*$)/);
  # Input may use a different EOL sequence than $/, so avoid chomp.
  $_ =~ s/[\r\n]+$//g;
  if ($_ =~ /=/) {
    push @aliases, $_;
  } else {
    push @names, $_;
  }
}
close(NAMES);

open GPERF, ">CSSPropertyNames.gperf" || die "Could not open CSSPropertyNames.gperf for writing";
print GPERF << "EOF";
%{
/* This file is automatically generated from CSSPropertyNames.in by makeprop, do not edit */
#include \"CSSPropertyNames.h\"
#include \"HashTools.h\"
#include <string.h>

namespace WebCore {
%}
%struct-type
struct Property;
%omit-struct-type
%language=C++
%readonly-tables
%global-table
%compare-strncmp
%define class-name CSSPropertyNamesHash
%define lookup-function-name findPropertyImpl
%define hash-function-name propery_hash_function
%define word-array-name property_wordlist
%enum
%%
EOF

foreach my $name (@names) {
  my $id = $name;
  $id =~ s/(^[^-])|-(.)/uc($1||$2)/ge;
  print GPERF $name . ", CSSProperty" . $id . "\n";
}

foreach my $alias (@aliases) {
  $alias =~ /^([^\s]*)[\s]*=[\s]*([^\s]*)/;
  my $name = $1;
  my $id = $2;
  $id =~ s/(^[^-])|-(.)/uc($1||$2)/ge;
  print GPERF $name . ", CSSProperty" . $id . "\n";
}

print GPERF<< "EOF";
%%
const Property* findProperty(register const char* str, register unsigned int len)
{
    return CSSPropertyNamesHash::findPropertyImpl(str, len);
}

const char* getPropertyName(CSSPropertyID id)
{
    if (id < firstCSSProperty)
        return 0;
    int index = id - firstCSSProperty;
    if (index >= numCSSProperties)
        return 0;
    return propertyNameStrings[index];
}

} // namespace WebCore
EOF

open HEADER, ">CSSPropertyNames.h" || die "Could not open CSSPropertyNames.h for writing";
print HEADER << "EOF";
/* This file is automatically generated from CSSPropertyNames.in by makeprop, do not edit */

#ifndef CSSPropertyNames_h
#define CSSPropertyNames_h

#include <string.h>

namespace WebCore {

enum CSSPropertyID {
    CSSPropertyInvalid = 0,
EOF

my $first = 1001;
my $i = 1001;
my $maxLen = 0;
foreach my $name (@names) {
  my $id = $name;
  $id =~ s/(^[^-])|-(.)/uc($1||$2)/ge;
  print HEADER "    CSSProperty" . $id . " = " . $i . ",\n";
  $i = $i + 1;
  if (length($name) > $maxLen) {
    $maxLen = length($name);
  }
}
my $num = $i - $first;

print HEADER "};\n\n";
print HEADER "const int firstCSSProperty = $first;\n";
print HEADER "const int numCSSProperties = $num;\n";
print HEADER "const size_t maxCSSPropertyNameLength = $maxLen;\n";

print HEADER "const char* const propertyNameStrings[$num] = {\n";
foreach my $name (@names) {
  print HEADER "\"$name\",\n";
}
print HEADER "};\n";

print HEADER << "EOF";

const char* getPropertyName(CSSPropertyID);

} // namespace WebCore

#endif // CSSPropertyNames_h

EOF

close HEADER;

system("gperf --key-positions=\"*\" -D -n -s 2 CSSPropertyNames.gperf > CSSPropertyNames.cpp") == 0 || die "calling gperf failed: $?";
