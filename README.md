MPW Shell
---------

MPW Shell is a re-implementation of the Macintosh Programmer's Workshop shell.
The primary reason is to support MPW Make (which generated shell script). It
may also be useful for other things.

Supported features
------------------
* If ... [Else If] ... [Else] ... End
* Begin ... End
* Loop ... End
* For name In [word...] ... End
* Break [If], Continue [If], Exit [If]
* ( ... )
* ||
* &&
* Redirection
* | "pipes" (via a temporary file. Presumably, that's what MPW did as well.)
* Subshells (`...`, ``...``)


Not (yet) supported
-------------
* aliases
* regular expressions
* text-editing commands (search forward/backward, et cetera)

Builtin Commands
----------------
* AboutBox
* Alias
* Catenate
* Directory
* Echo
* Evaluate
* Execute
* Exists
* Export
* Parameters
* Quit
* Quote
* Set
* Shift
* Unalias
* Unexport
* Unset
* Version
* Which


Setup
-----
1. Install MPW.  The mpw binary should be somewhere in your `$PATH`.
It also checks `/usr/local/bin/mpw` and `$HOME/mpw/bin/mpw`.  You can
use mpw-shell without it but only with builtin commands.
2. Copy the `Startup` script to `$HOME/mpw/`.  This script is executed
when mpw-shell (or mpw-make) starts up (imagine that) and should
be used to set environment variables.


Command Line Flags
------------------

    -D name=value  Define environment variable
    -v             Be verbose (equivalent to -Decho=1)
    -f             Ignore the Startup script
    -c string      Execute string
    -h             Display help


Build
-----

Standard CMake build sequence:

```bash
mkdir build
cd build
cmake ..
make
```

After that, do the standard CMake install sequence in the same folder:

```bash
cmake --install
```

to install `mpw-shell` and `mpw-make` in `/usr/bin/local`.
