#!/bin/bash

cd $(dirname "${BASH_SOURCE[0]}")
source venv/bin/activate

echo "========" >> server.log
date >> server.log
echo "------" >> server.log
./app.py --host=0.0.0.0 --port=4040 --debug 2>&1 | tee -a server.log
