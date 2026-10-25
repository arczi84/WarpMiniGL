#!/usr/bin/env perl
use strict;
use warnings;
    open my $fd, '<', 'Warp3D.library/Warp3D.fd' or die $!;
    my @names;
    while (<$fd>) { push @names, $1 if /^(W3D_\w+)\(/; }
    die "Unexpected vector count: " . @names . "\n" unless @names == 84;
    print "extern void $_(void);\n" for @names;
    print "const APTR qt_vectors[] = { (APTR)LibOpen, (APTR)LibClose, (APTR)LibExpunge, (APTR)LibReserved,\n";
    print "    (APTR)$_,\n" for @names;
    print "    (APTR)-1 };\n";
