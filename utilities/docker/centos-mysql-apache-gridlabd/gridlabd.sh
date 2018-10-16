REPO=${REPO:-https://github.com/dchassin/gridlabd}
BRANCH=${BRANCH:-master}
echo "
#####################################
# DOCKER BUILD
#   gridlabd <- $REPO/$BRANCH
#####################################
"

cd /usr/local/src
git clone $REPO gridlabd -b $BRANCH
# https://github.com/dchassin/gridlabd/archive/powernet/master.zip
#curl -L $REPO/archive/$BRANCH.zip -o gridlabd.zip && unzip gridlabd.zip -d tmp
#if [ -d tmp ]; then
#	mv tmp/* gridlabd
#else
#	echo "ERROR: unable to download gridlabd source into ./$BRANCH"
#	exit 1
#fi

# install xercesc
cd /usr/local/src/gridlabd/third_party
XERCES=xerces-c-src_2_8_0
if [ -f ${XERCES}.tar.gz ]; then
	# use older version of xerces if still available
	gunzip ${XERCES}.tar.gz
	tar xf ${XERCES}.tar
	cd ${XERCES}
	export XERCESCROOT=`pwd`
	cd src/xercesc
	./runConfigure -plinux -cgcc -xg++ -minmem -nsocket -tnative -rpthread
	make
	cd ${XERCESCROOT}
	cp -r include/xercesc /usr/include
	chmod -R a+r /usr/include/xercesc
	ln lib/* /usr/lib 
	/sbin/ldconfig
else # use newer version
	XERCES=xerces-c-3.2.0
	if [ ! -f ${XERCES}.tar.gz ]; then
		curl -L https://archive.apache.org/dist/xerces/c/3/sources/${XERCES}.tar.gz > ${XERCES}.tar.gz
	fi
	tar xvfz ${XERCES}.tar.gz
	cd ${XERCES}
	./configure --disable-static 'CFLAGS=-O2' 'CXXFLAGS=-O2'
	make install
	echo "${XERCES} installed ok"
fi
	
# install mysql 
cd /usr/local/src/gridlabd/third_party
MYSQL=mysql-connector-c-6.1.11-linux-glibc2.12-x86_64
if [ ! -f ${MYSQL}.tar.gz ]; then
	curl -L https://downloads.mysql.com/archives/get/file/mysql-connector-c-6.1.11-linux-glibc2.12-x86_64.tar.gz -o ${MYSQL}.tar.gz
fi
if [ -f ${MYSQL}.tar.gz ]; then
	gunzip ${MYSQL}.tar.gz
	tar xf ${MYSQL}.tar
	cp -u ${MYSQL}/bin/* /usr/local/bin
	cp -Ru ${MYSQL}/include/* /usr/local/include
	cp -Ru ${MYSQL}/lib/* /usr/local/lib
	MYSQLOPT=/usr/local
	echo "${MYSQL} installed into ${MYSQLOPT}"
else
	echo "WARNING: ${MYSQL} not found -- mysql will not be included in this build" > /dev/stderr
	MYSQLOPT=no
fi

# install armadillo
cd /usr/local/src/gridlabd/third_party
ARMA=armadillo-7.800.1
if [ -f ${ARMA}.tar.gz ]; then
	gunzip ${ARMA}.tar.gz
	tar xf ${ARMA}.tar
	cd ${ARMA}
	cmake .
	make install
else
	echo "WARNING: ${ARMA} not found -- armadillo will not be included in this build" > /dev/stderr
fi

# install gridlabd
cd /usr/local/src/gridlabd
autoreconf -isf
./configure --enable-silent-rules --prefix=/usr/local --with-mysql=$MYSQLOPT 'CXXFLAGS=-w -O3' 'CFLAGS=-w -O3'
make install

