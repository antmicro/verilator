#!/usr/bin/env perl
if (!$::Driver) { use FindBin; exec("$FindBin::Bin/bootstrap.pl", @ARGV, $0); die; }
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# Copyright 2003 by Wilson Snyder. This program is free software; you
# can redistribute it and/or modify it under the terms of either the GNU
# Lesser General Public License Version 3 or the Perl Artistic License
# Version 2.0.
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

# Similar to t_jsontree.pl but rather than using artifitial test input, it actually makes real dump

scenarios(vlt => 1); # NOTE: not sure about scenario

{
  my $cmd = qq{jq --version >/dev/null 2>&1};
  print "\t$cmd\n" if $::Debug;
  system($cmd) and do { skip("No jq installed"); return 1}
}
{
  my $cmd = qq{python3 -c "from deepdiff import DeepDiff; DeepDiff([],[1],view='tree')['iterable_item_added'][0].path(output_format='list')" >/dev/null 2>&1};
  print "\t$cmd\n" if $::Debug;
  system($cmd) and do { skip("No DeepDiff installed or you have version without output_format='list'\n"); return 1 }
}

top_filename("t/t_EXAMPLE.v");

lint(
    v_flags => ["--lint-only --dump-tree-json"],
    );


# editNum, file and all pointer fields are filtered out in order to increase test stability
run(cmd => ["cd $Self->{obj_dir} && $ENV{VERILATOR_ROOT}/bin/verilator_jsontree *001*.json -d '.editNum, .file, ptrs' > show.log"],
    check_finished => 0);

files_identical("$Self->{obj_dir}/show.log", $Self->{golden_filename}, 'logfile');

ok(1);

1;
