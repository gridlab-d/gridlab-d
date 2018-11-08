FROM centos
WORKDIR /tmp
RUN curl ${ORIGIN:-https://raw.githubusercontent.com/dchassin/gridlabd}/${BRANCH:-master}/utilities/docker/centos-gridlabd/setup.sh -o setup.sh
RUN ( bash setup.sh || true )
RUN rm -rf /tmp/setup.sh /usr/local/src/gridlabd
#EXPOSE 80/tcp 6267/tcp
