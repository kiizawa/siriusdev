FROM docker.io/ubuntu

# create a rm alias

RUN echo "alias rm='rm -i'" >> ~/.bash_aliases

# set PATH

RUN echo "export LD_LIBRARY_PATH=/usr/local/lib" >> ~/.bashrc

# install packages

RUN apt -y update && \
    apt -y install libboost-all-dev \
                   emacs \
                   g++ \
                   git \
                   net-tools \
                   openssh-server \
                   uuid-runtime

# install ceph

WORKDIR /home
RUN git clone git://github.com/ceph/ceph && \
    cd /home/ceph && \
    git fetch origin pull/18211/head:wip_bluestore_tiering && \
    git checkout wip_bluestore_tiering && \
    ./install-deps.sh && \
    ./do_cmake.sh && \
    cd /home/ceph/build && \
    make && \
    make install && \
    cd /home && \
    rm -rf /home/ceph    

# setup entrypoint

COPY start_ceph.sh /root/
COPY crushmap /root/

# setup OpenSSH Server

RUN mkdir /var/run/sshd
RUN echo "root:password" | chpasswd
RUN sed -i 's/PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config
RUN sed -i 's|#AuthorizedKeysFile|AuthorizedKeysFile|' /etc/ssh/sshd_config
#RUN sed -i 's|%h/.ssh/authorized_keys|/share/authorized_keys|' /etc/ssh/sshd_config

# Skip authentication

RUN sed -i 's|#   StrictHostKeyChecking ask|StrictHostKeyChecking no|' /etc/ssh/ssh_config

# SSH login fix. Otherwise user is kicked off after login

RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd

# Generate keys

RUN ssh-keygen -t rsa -f /root/.ssh/id_rsa -N ''
RUN cat /root/.ssh/id_rsa.pub >> /root/.ssh/authorized_keys
RUN chmod 600 /root/.ssh/authorized_keys

EXPOSE 22

CMD ["/root/start_ceph.sh"]
