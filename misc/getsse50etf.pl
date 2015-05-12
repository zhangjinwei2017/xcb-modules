#!/usr/bin/perl -w
#
# Copyright (c) 2013-2015, Dalian Futures Information Technology Co., Ltd.
#
# Xiaoye Meng <mengxiaoye@dce.com.cn>
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

