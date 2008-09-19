/****************************************************************************
** $Id: qt/tagreader.cpp   3.3.7   edited Feb 20 2006 $
**
** Copyright (C) 1992-2005 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include "structureparser.h"
#include <qfile.h>
#include <qxml.h>
#include <qwindowdefs.h>

int main( int argc, char **argv )
{
    if ( argc < 2 ) {
	fprintf( stderr, "Usage: %s <xmlfile> [<xmlfile> ...]\n", argv[0] );
	return 1;
    }
    StructureParser handler;
    QXmlSimpleReader reader;
    reader.setContentHandler( &handler );
    for ( int i=1; i < argc; i++ ) {
        QFile xmlFile( argv[i] );
        QXmlInputSource source( &xmlFile );
        reader.parse( source );
    }
    return 0;
}
