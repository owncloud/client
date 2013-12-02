#!/bin/bash
L10NDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )"
SCRIPTDIR="$L10NDIR/bin"
PODIR="$L10NDIR/pofiles"
MIRALLROOT="$( cd ${L10NDIR}/../../../.. && pwd)"

if [ "$1" == "pot" ] ; then
	python $SCRIPTDIR/build_gettext_catalog_nsi.py -i $MIRALLROOT/cmake/modules/NSIS.template.in -o $PODIR/messages.pot -p "ownCloud client installer" -v "0.1" -l "English"
	exit
fi

if [ "$1" == "merge" ] ; then
	python $SCRIPTDIR/build_locale_nsi.py -i $MIRALLROOT/cmake/modules/NSIS.template.in -o $L10NDIR/project_multilang.nsi -p $PODIR -l "English"
	exit
fi

echo "use 'pot' or 'merge' as commands"


