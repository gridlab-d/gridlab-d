FROM <<<BASE_SYSTEM>>>
ENV container docker

ENV REPO=<<<GRIDLABD_ORIGIN>>>
ENV BRANCH=<<<GRIDLABD_BRANCH>>>
ENV ENABLE=<<<GRIDLABD_ENABLE>>>

RUN (cd /lib/systemd/system/sysinit.target.wants/; for i in *; do [ $i == \
systemd-tmpfiles-setup.service ] || rm -f $i; done); \
rm -f /lib/systemd/system/multi-user.target.wants/*;\
rm -f /etc/systemd/system/*.wants/*;\
rm -f /lib/systemd/system/local-fs.target.wants/*; \
rm -f /lib/systemd/system/sockets.target.wants/*udev*; \
rm -f /lib/systemd/system/sockets.target.wants/*initctl*; \
rm -f /lib/systemd/system/basic.target.wants/*;\
rm -f /lib/systemd/system/anaconda.target.wants/*;
VOLUME [ "/sys/fs/cgroup" ]
WORKDIR /tmp
COPY *.sh /tmp/
RUN chmod +wx *.sh
RUN bash system.sh
RUN bash gridlabd.sh
EXPOSE 6267/tcp
