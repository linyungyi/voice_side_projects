bin_PROGRAMS = asrclient_examplefor200
asrclient_examplefor200_SOURCES = asrclient_examplefor200.cpp \
	ExampleOfTLAsrEngCln.cpp

# set the include path found by configure
INCLUDES = -I/home/cch/work/tlasrlib/ $(all_includes)

# the library search path.
asrclient_examplefor200_LDFLAGS = $(all_libraries) 
asrclient_examplefor200_LDADD = -lpthread /home/cch/lib/libtlasrlib_deb.a
