# pylint: disable=line-too-long,invalid-name,multiple-statements,missing-function-docstring,missing-class-docstring,missing-module-docstring,no-else-return,too-few-public-methods,unused-argument
import os
import sys
import tempfile

import gdb  # pylint: disable=import-error

# add dir with verilator_jsontree to import path
sys.path.append(os.environ["VERILATOR_ROOT"] + "/bin")


def _get_dump(node):
    return gdb.execute(f'printf "%s", &(AstNode::dumpJsonTreeGdb({node})[0])',
                       to_string=True)


def _tmpfile():
    return tempfile.NamedTemporaryFile(mode="wt")  # write, text mode


def _fwrite(file, s):
    """write to file and flush buf to make sure that data is written before passing file to jsontree"""
    file.write(s)
    file.flush()


class JsonTreeCmd(gdb.Command):
    """Verilator: Pretty print or diff node(s) using jsontree. Node is allowed to be:
    * an actual pointer,
    * an address literal (like 0x55555613dca0),
    * a gdb value (like $1) that stores a dump previously done by the jstash command.
    Besides not taking input from file, it works exactly like `verilator_jsontree`:
    * passing one node gives you a pretty print,
    * passing two nodes gives you a diff,
    * flags like -d work like expected.
    """

    def __init__(self):
        super().__init__("jtree", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)

    def _null_check(self, old, new):
        err = ""
        if old == "<nullptr>\n": err += "old == <nullptr>\n"
        if new == "<nullptr>\n": err += "new == <nullptr>"
        if err: raise gdb.GdbError(err.strip("\n"))

    def invoke(self, arg_str, from_tty):
        import verilator_jsontree as jsontree  # pylint: disable=wrong-import-position
        # jsontree import may fail so we do it here rather than in outer scope

        # We abuse verilator_jsontree's arg parser to find arguments with nodes
        # After finding them, we replace them with proper files
        jsontree_args = jsontree.parser.parse_args(gdb.string_to_argv(arg_str))
        self._null_check(jsontree_args.file, jsontree_args.newfile)
        with _tmpfile() as oldfile, _tmpfile() as newfile:
            if jsontree_args.file:
                _fwrite(oldfile, _get_dump(jsontree_args.file))
                jsontree_args.file = oldfile.name
            if jsontree_args.newfile:
                _fwrite(newfile, _get_dump(jsontree_args.newfile))
                jsontree_args.newfile = newfile.name
            try:
                jsontree.main(jsontree_args)
            except SystemExit:  # jsontree prints nice errmsgs on exit(), so we just catch it to suppress cryptic python trace
                return


JsonTreeCmd()
