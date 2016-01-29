#!/usr/bin/perl -w
#
# Copyright (c) 2013-2016, Dalian Futures Information Technology Co., Ltd.
#
# Xiaoye Meng <mengxiaoye at dce dot com dot cn>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

use strict;
use File::Fetch;

my $ff = File::Fetch->new(uri => 'http://query.sse.com.cn/etfDownload/downloadETF2Bulletin.do?etfType=1');
my $where = $ff->fetch(to => '/tmp') or die $ff->error, ".\n";
open my $fh, '<', $where or die "Could not open $where: $!\n";
my @lines = <$fh>;
close $fh;
unlink $where;
print ";\n";
print "; XCUBE sse50etf configuration\n";
print ";\n";
print "\n";
print "[general]\n";
foreach (@lines) {
	chomp;
	my @a = split /=/;
	s/\s+$//g foreach @a;
	print "CRU=", $a[1], "\n" if $a[0] eq "CreationRedemptionUnit";
	print "ECC=", $a[1], "\n" if $a[0] eq "EstimateCashComponent";
}
print "\n[weights]\n";
foreach (@lines) {
	chomp;
	my @a = split /\|/;
	next if $#a < 3;
	s/^\s+//g foreach @a;
	print "SH", $a[0], "=", $a[2], "\n" if $a[3] eq "1";
}
print "\n[prices]\n";
foreach (@lines) {
	chomp;
	my @a = split /\|/;
	next if $#a < 5;
	s/^\s+//g foreach @a;
	print "SH", $a[0], "=", $a[5], "\n" if $a[3] eq "2";
}
print "\n";

