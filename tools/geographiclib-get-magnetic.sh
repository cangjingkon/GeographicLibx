#! /bin/sh
#
# Download magnetic models for use by GeographicLib::MagneticModel.
#
# Copyright (c) Charles Karney (2011) <charles@karney.com> and licensed
# under the MIT/X11 License.  For more information, see
# http://geographiclib.sourceforge.net/
#
# $Id$

MAGNETIC_DEFAULT_PATH="@MAGNETIC_DEFAULT_PATH@"
DEFAULTDIR=`dirname "$MAGNETIC_DEFAULT_PATH"`
usage() {
    cat <<EOF
usage: $0 [-p parentdir] [-d] [-h] magneticmodel...

This program downloads and installs the megnetic models used by the
GeographicLib::MagneticModel class and the MagneticField tool to compute
magnetic fields.  magneticmodel is one
of more of the names from this table:

                                  size (kB)
  name     degree    years      tar.bz2  disk
  wmm2010    12    2010-2015     0.5       5
  igrf11     13    1900-2015     0.5      26
  emm2010   740    2010-2015     0.5    4400

The size columns give the download and installed sizes of the models.
In addition you can specify

  all = all of the datasets

If no name is specified then all is assumed.

-p parentdir (default $DEFAULTDIR) specifies where the
datasets should be stored.  The "Default magnetic path" listed when running

  MagneticField -h

should be parentdir/magnetic.  This script must
be run by a user with write access to this directory.

If -d is provided, the temporary directory which holds the downloads,
${TMPDIR:-/tmp}/magnetic-XXXXXXXX, will be saved.  -h prints this help.

For more information on the magnetic datasets, visit

  http://geographiclib.sourceforge.net/html/magnetic.html

EOF
}

PARENTDIR="$DEFAULTDIR"
DEBUG=
while getopts hp:d c; do
    case $c in
        h )
            usage;
            exit 0
            ;;
        p ) PARENTDIR="$OPTARG"
            ;;
        d ) DEBUG=y
            ;;
        * )
            usage 1>&2;
            exit 1
            ;;
    esac
done
shift `expr $OPTIND - 1`

test -d "$PARENTDIR"/magnetic || mkdir -p "$PARENTDIR"/magnetic 2> /dev/null
if test ! -d "$PARENTDIR"/magnetic; then
    echo Cannot create directory $PARENTDIR/magnetic 1>&2
    exit 1
fi

TEMP=
if test -z "$DEBUG"; then
trap 'trap "" 0; test "$TEMP" && rm -rf "$TEMP"; exit 1' 1 2 3 9 15
trap            'test "$TEMP" && rm -rf "$TEMP"'            0
fi
TEMP=`mktemp --tmpdir --quiet --directory magnetic-XXXXXXXX`

if test -z "$TEMP" -o ! -d "$TEMP"; then
    echo Cannot create temporary directory 1>&2
    exit 1
fi

WRITETEST="$PARENTDIR"/magnetic/write-test-`basename $TEMP`
if touch "$WRITETEST" 2> /dev/null; then
    rm -f "$WRITETEST"
else
    echo Cannot write in directory $PARENTDIR/magnetic 1>&2
    exit 1
fi

set -e

cat > $TEMP/all <<EOF
wmm2010
emm2010
igrf11
EOF

test $# -eq 0 && set -- minimal

while test $# -gt 0; do
    if grep "^$1\$" $TEMP/all > /dev/null; then
	echo $1
    else
	case "$1" in
	    all )
		cat $TEMP/all
		;;
	    * )
		echo Unknown magnetic model $1 1>&2
		exit 1
		;;
	esac
    fi
    shift
done > $TEMP/list

sort -u $TEMP/list > $TEMP/todo

while read file; do
    echo download $file.tar.bz2 ...
    URL="http://downloads.sourceforge.net/project/geographiclib/magnetic-distrib/$file.tar.bz2?use_mirror=autoselect"
    ARCHIVE=$TEMP/$file.tar.bz2
    wget -O$ARCHIVE $URL
    echo unpack $file.tar.bz2 ...
    tar vxojf $ARCHIVE -C $PARENTDIR
    echo magnetic $file installed.
done < $TEMP/todo

if test "$DEBUG"; then
    echo Saving temporary directory $TEMP
fi
cat <<EOF

Magnetic datasets `tr '\n' ' ' < $TEMP/todo`
downloaded and installed in $PARENTDIR/magnetic.

EOF
