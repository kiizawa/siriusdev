#!/bin/sh

docker run -it --rm neurodebian bash -c 'apt-get update -qqq; apt-cache policy singularity-container; exit'
wget -O- http://neuro.debian.net/lists/stretch.us-ca.full | sudo tee /etc/apt/sources.list.d/neurodebian.sources.list
sudo apt-key adv --recv-keys --keyserver hkp://pool.sks-keyservers.net:80 0xA5D32F012649A5A9
sudo apt-get update
sudo apt-get install -y singularity-container
