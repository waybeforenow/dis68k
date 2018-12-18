% DIS68K(1) | Human68k disk utility

# NAME

dis68k - utility for packing various X68000 disk image files

# SYNOPSIS

dis68k l _file_ \ \ \ \ \ # list the contents of _file_\
dis68k x _file_ \ \ # extract the contents of _file_

# DESCRIPTION

dis68k is a utility that can list the contents of, and unpack DIM and XDF image
files, which are readable by the _Human68k_ operating system for _Sharp X68000_
personal computers.

# OPTIONS

l _file_

> Prints the full directory structure of _file_ to the terminal.

p _file_

> Extracts the full structure of _file_ to the local disk.

> The files insile _file_ are written to a directory that is named according to 
  _file_ without its file extension. This directory is created if it does not
  exist.

# BUILDING

The dis68k source comes bundled with a modified version of _FatFs_, a framework
for writing libraries that can interact with the FAT filesystem. There are no
external dependencies.

To build dis68k from source, run
```
make
```

To build an alternate dis68k binary with verbose logging and debug symbols
included, run
```
make DEBUG=true
```

The manual page for dis68k is built from this Markdown document using the
_pandoc_ utility, which is not included. To build the manual page, run
```
make doc
```

# TODO

dis68k should have the ability to write to (pack) image files in the future.

dis68k should support packing and unpacking HDS (SCSI hard disk) image files,
and (in the farther future) HDF (SxSI hard disk) image files as well.

# SEE ALSO

FatFs library:
_[http://elm-chan.org/fsw/ff/00index\_e.html](http://elm-chan.org/fsw/ff/00index_e.html)_

fathuman utility, from which this codebase was forked:
_[https://github.com/vampirefrog/fathuman](https://github.com/vampirefrog/fathuman)_

# LICENSE

Please see the LICENSE file for more information.

