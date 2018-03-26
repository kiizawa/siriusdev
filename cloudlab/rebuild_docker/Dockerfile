FROM dummy

## ssh setup
#RUN apt -y install openssh-server
#
## setup OpenSSH Server
#
#RUN mkdir /var/run/sshd
#RUN echo "root:password" | chpasswd
#RUN sed -i 's/PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config
#RUN sed -i 's|#AuthorizedKeysFile|AuthorizedKeysFile|' /etc/ssh/sshd_config
##RUN sed -i 's|%h/.ssh/authorized_keys|/share/authorized_keys|' /etc/ssh/sshd_config
#
## Skip authentication
#RUN sed -i 's|#   StrictHostKeyChecking ask|StrictHostKeyChecking no|' /etc/ssh/ssh_config
#
## SSH login fix. Otherwise user is kicked off after login
#RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd
#
#EXPOSE 22

CMD ["/root/start_ceph.sh"]
