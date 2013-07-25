noinst_LTLIBRARIES += third_party/CBLAS/libblas.la 

third_party_CBLAS_libblas_la_CPPFLAGS =
third_party_CBLAS_libblas_la_CPPFLAGS = $(AM_CPPFLAGS)

third_party_CBLAS_libblas_la_LDFLAGS =
third_party_CBLAS_libblas_la_LDFLAGS = $(AM_LDFLAGS)

third_party_CBLAS_libblas_la_LIBADD = -lm

third_party_CBLAS_libblas_la_SOURCES =
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/dasum.c
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/daxpy.c
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/dcopy.c
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/ddot.c
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/dgemv.c
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/dger.c
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/dnrm2.c
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/drot.c
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/dscal.c
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/dsymv.c
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/dsyr2.c
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/dtrsv.c
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/f2c.h
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/idamax.c
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/slu_Cnames.h
third_party_CBLAS_libblas_la_SOURCES += third_party/CBLAS/superlu_f2c.h
