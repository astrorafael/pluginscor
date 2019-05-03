AC_PREREQ([2.59])

AC_INIT([pluginscor], [0.1], [astrorafael@yahoo.es])
AC_CONFIG_AUX_DIR([config])
dnl AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.9])

AC_PROG_CXX
AC_PROG_LIBTOOL

## This piece of code is copied verbatim to the configure script
## and tests the installation of the INDICOR SDK

# script that checks the location of the indi library
echo -n "checking for an INDICOR SDK installed ... "
home=`echo $HOME`
if test -d "$home/indicor/include/indicor" 
then 
     indicor_dir="$home/indicor"
     indicor_incdir="$home/indicor/include" 
     indicor_libdir="$home/indicor/lib" 
     indicor_datadir="$home/indicor/share/indicor" 
else
	if test -d "$home/include/indicor"
	then
		indicor_dir="$home/indicor" 
                indicor_incdir="$home/indicor/include" 
                indicor_libdir="$home/indicor/lib" 
                indicor_datadir="$home/share/indicor" 
	else
		if test -d "/usr/local/include/indicor"
		then
			indicor_dir="/usr/local" 
                        indicor_incdir="/usr/local/include" 
                        indicor_libdir="/usr/local/lib" 
                        indicor_datadir="/usr/local/share/indicor" 
		else
			indicor_dir=""
                        indicor_incdir="" 
                        indicor_libdir="" 
                        indicor_datadir="" 
		fi
	fi
fi

if test -z $indicor_dir
then
	echo "no."
	echo "ERROR: INDICOR library not found in expected locations"
	exit 255
else
	echo "yes."
fi

## Passes the values found to the makefiles
AC_SUBST([indicor_dir])
AC_SUBST([indicor_incdir])
AC_SUBST([indicor_libdir])
AC_SUBST([indicor_datadir])


AC_CONFIG_FILES([Makefile weather/Makefile ccds/Makefile scopes/Makefile \
			  hubs/Makefile power/Makefile weather/meteo/Makefile \
			  ccds/audine/Makefile scopes/lx200/Makefile \
			  scopes/trackscope/Makefile \
			  hubs/cor/Makefile power/cor/Makefile])

AC_OUTPUT
