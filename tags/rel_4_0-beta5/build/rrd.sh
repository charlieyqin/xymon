	echo "Checking for RRDtool ..."

	RRDINC=""
	RRDLIB=""
	PNGLIB=""
	ZLIB=""
	for DIR in /opt/rrdtool* /usr/local/rrdtool* /usr/local /usr
	do
		if test -f $DIR/include/rrd.h
		then
			RRDINC=$DIR/include
		fi

		if test -f $DIR/lib/librrd.so
		then
			RRDLIB=$DIR/lib
		fi
		if test -f $DIR/lib/librrd.a
		then
			RRDLIB=$DIR/lib
		fi

		if test -f $DIR/lib/libpng.so
		then
			PNGLIB="-L$DIR/lib -lpng"
		fi
		if test -f $DIR/lib/libpng.a
		then
			PNGLIB="-L$DIR/lib -lpng"
		fi

		if test -f $DIR/lib/libz.so
		then
			ZLIB="-L$DIR/lib -lz"
		fi
		if test -f $DIR/lib/libz.a
		then
			ZLIB="-L$DIR/lib -lz"
		fi
	done

	if test -z "$RRDINC" -o -z "$RRDLIB"; then
		echo "RRDtool include- or library-files not found. These are REQUIRED for hobbitd"
		echo "RRDtool can be found at http://people.ee.ethz.ch/~oetiker/webtools/rrdtool/"
		exit 1
	else
		cd build
		OS=`uname -s` make -f Makefile.test-rrd clean
		OS=`uname -s` RRDINC="-I$RRDINC" make -f Makefile.test-rrd test-compile
		if [ $? -eq 0 ]; then
			echo "Found RRDtool include files in $RRDINC"
		else
			echo "ERROR: RRDtool include files found in $RRDINC, but compile fails."
			exit 1
		fi

		OS=`uname -s` RRDLIB="-L$RRDLIB" PNGLIB="$PNGLIB" make -f Makefile.test-rrd test-link 2>/dev/null
		if [ $? -ne 0 ]; then
			# Could be that we need -lz for RRD
			PNGLIB="$PNGLIB $ZLIB"
		fi
		OS=`uname -s` RRDLIB="-L$RRDLIB" PNGLIB="$PNGLIB" make -f Makefile.test-rrd test-link 2>/dev/null
		if [ $? -ne 0 ]; then
			# Could be that we need -lm for RRD
			PNGLIB="$PNGLIB -lm"
		fi
		OS=`uname -s` RRDLIB="-L$RRDLIB" PNGLIB="$PNGLIB" make -f Makefile.test-rrd test-link 2>/dev/null
		if [ $? -eq 0 ]; then
			echo "Found RRDtool libraries in $RRDLIB"
			if test "$PNGLIB" != ""; then
				echo "Linking RRD with PNG library: $PNGLIB"
			fi
		else
			echo "ERROR: RRDtool library files found in $RRDLIB, but link fails."
			exit 1
		fi
		OS=`uname -s` make -f Makefile.test-rrd clean
		cd ..
	fi


