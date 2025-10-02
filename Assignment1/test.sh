#!/usr/bin/bash

telnet localhost 12345 &
echo "GET http://www.jhu.edu/ HTTP/1.0" &
echo ""