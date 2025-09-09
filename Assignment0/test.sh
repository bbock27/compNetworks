#!/bin/bash
set -x
SERVER_EXEC=./server
CLIENT_EXEC=./client
PORT=12341
SERVER_OUTPUT=server_output.txt
TMP_DIR=tmp_test_data

mkdir -p $TMP_DIR
rm -f $SERVER_OUTPUT
echo "start"
# Start server in the background
$SERVER_EXEC $PORT > $SERVER_OUTPUT 2>&1 &
SERVER_PID=$!

# Wait briefly to ensure server is listening
sleep 1

### TEST 1: Alphanumeric Message ###
echo "Hello, world!" | $CLIENT_EXEC 127.0.0.1 $PORT
sleep 0.5

### TEST 2: Large Text Message ###
base64 /dev/urandom | head -c 100000 > $TMP_DIR/large_text.txt
$CLIENT_EXEC 127.0.0.1 $PORT < $TMP_DIR/large_text.txt
sleep 0.5

### TEST 3: Binary File (PNG Header) ###
printf "\x89PNG\r\n\x1a\n" > $TMP_DIR/binary_input.bin
$CLIENT_EXEC 127.0.0.1 $PORT < $TMP_DIR/binary_input.bin
sleep 0.5
echo "half"
### TEST 4: Multiple Clients Sequentially ###
echo "Client 1" | $CLIENT_EXEC 127.0.0.1 $PORT
echo "Client 2" | $CLIENT_EXEC 127.0.0.1 $PORT
echo "Client 3" | $CLIENT_EXEC 127.0.0.1 $PORT
sleep 0.5

### TEST 5: Concurrent Clients ###
(
  timeout 5 bash -c 'echo "Concurrent A" | '"$CLIENT_EXEC"' 127.0.0.1 '"$PORT"
) &
(
  timeout 5 bash -c 'echo "Concurrent B" | '"$CLIENT_EXEC"' 127.0.0.1 '"$PORT"
) &
wait
sleep 0.5

# Kill the server
# kill $SERVER_PID
# wait $SERVER_PID 2>/dev/null

### VERIFY RESULTS ###
echo "=== TEST RESULTS ==="
grep -a -E "Hello, world!" $SERVER_OUTPUT >/dev/null && echo "[✔] Alphanumeric message received"
cmp --silent $TMP_DIR/large_text.txt <(grep -a -o -F -f $TMP_DIR/large_text.txt $SERVER_OUTPUT) && echo "[✔] Large message received"
grep -a -F $'\x89PNG\r\n\x1a\n' $SERVER_OUTPUT >/dev/null && echo "[✔] Binary message received"
grep -q "Client 1" $SERVER_OUTPUT && echo "[✔] Sequential Client 1 received"
grep -q "Client 2" $SERVER_OUTPUT && echo "[✔] Sequential Client 2 received"
grep -q "Client 3" $SERVER_OUTPUT && echo "[✔] Sequential Client 3 received"
grep -q "Concurrent A" $SERVER_OUTPUT && echo "[✔] Concurrent Client A received"
grep -q "Concurrent B" $SERVER_OUTPUT && echo "[✔] Concurrent Client B received"

echo
echo "Check '$SERVER_OUTPUT' for full captured output."
echo "Temporary data is in '$TMP_DIR'."