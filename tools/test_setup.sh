#!/bin/bash

function error_out {
  [[ $? -ne 0 ]] && exit 1
}

bin="python net-modeler-ctrl/net_modeler.py"

sudo $bin modelinfo set 21 "Test" 4 3
error_out
sudo $bin hoptable set 0 100 10 100
error_out
sudo $bin hoptable set 1 100 10 100
error_out
sudo $bin hoptable set 2 20 30 100
error_out
sudo $bin hoptable set 3 20 30 100
error_out
sudo $bin pathtable set 10.0.0.1 10.0.0.2 0
error_out
sudo $bin pathtable set 10.0.0.2 10.0.0.1 1
error_out
sudo $bin pathtable set 10.0.0.3 10.0.0.1 0 2
error_out
sudo $bin pathtable set 10.0.0.1 10.0.0.3 3 1
error_out                                   


sudo ifconfig eth0:1 10.0.0.1
sudo ifconfig eth0:2 10.0.0.2
sudo ifconfig eth0:3 10.0.0.3
