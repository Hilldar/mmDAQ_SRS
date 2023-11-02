#!/bin/bash
for i in {0..1000000};
do
    /usr/bin/rsync -avz -e "ssh -i /home/BesFe/.ssh/id_rsa -p 22" -r /home/BesFe/Desktop/mmdaq_old/BESIIIdata riccardo@srv-lab.fe.infn.it:.
    sleep 30s
done
