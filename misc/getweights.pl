#!/usr/bin/perl -w
#
# Copyright (c) 2013-2017, Dalian Futures Information Technology Co., Ltd.
#
# Bo Wang
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
# cat 20140718ashare_weightnextday.txt | perl getweights.pl 000300
#

use strict;

exit 1 if not defined $ARGV[0];
print "[weights]\n";
while (<STDIN>) {
	chomp;
	my @a = split /\|/;
	next if $#a < 7;
	s/\s+$//g foreach @a;
	$a[2] =~ s/\..+$//g;
	print $a[2], "=", $a[7], "\n" if $a[1] eq $ARGV[0];
}
print "\n";

