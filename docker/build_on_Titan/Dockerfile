FROM olcf/titan:ubuntu-16.04_2018-01-18

# create a rm alias

RUN echo "alias rm='rm -i'" >> ~/.bash_aliases

# set PATH

RUN echo "export LD_LIBRARY_PATH=/usr/local/lib" >> ~/.bashrc

# install packages

RUN apt -y update && \
    apt -y install emacs \
		   fuse \
                   g++ \
                   git \
		   libattr1-dev \
		   libboost-all-dev \
		   libfuse-dev \
                   net-tools \
                   openssh-server \
                   uuid-runtime

# install fuse_xattrs

COPY fuse_xattrs /home/fuse_xattrs
RUN cd /home/fuse_xattrs && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j2 && \
    make install -j2 && \
    cd /home && \
    rm -rf /home/fuse_xattrs

# install ceph

COPY ceph /home/ceph
RUN cd /home/ceph && \
    ./install-deps.sh && \
    ./do_cmake.sh && \
    cd build && \
    make -j2 && \
    make install -j2 && \
    cd /home && \
    rm -rf /home/ceph

# setup entrypoint

COPY start_ceph.sh /root/
COPY check_ceph.sh /root/
COPY crushmap /root/

# create mount points

RUN mkdir /ceph_dir_with_xattr

# set permissions

RUN chmod a+x /root && \
    chmod a+x /root/start_ceph.sh && \
    chmod u+s /root/start_ceph.sh && \
    chmod a+x /root/check_ceph.sh && \
    chmod u+s /root/check_ceph.sh

CMD ["/root/start_ceph.sh"]
