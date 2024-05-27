#!/bin/bash
#
# get today data

HOSTNAME=esp-h2o-0000d42c.fritz.box

wget http://$HOSTNAME/api/v1/today

