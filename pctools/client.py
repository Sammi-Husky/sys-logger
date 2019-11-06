import sys, socket, select

serverAddressPort   = (sys.argv[1], 28280)
bufferSize          = 256

sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
sock.settimeout(5)

while True:
    connected = False
    while not connected:
        print('Sending handshake.')
        # Send to server using created UDP socket
        sock.sendto(str.encode("Hello Switch"), serverAddressPort)
        try:
            reply, addr = sock.recvfrom(bufferSize)
            if reply.decode('utf-8') == 'Hello From sys-logger!':
                print(f'Switch at {addr[0]} accepted our friend request')
                connected = True
        except socket.timeout:
            continue

    print('Entering loop.')
    while connected:
        try:
            ready_to_read, ready_to_write, in_error = select.select([sock,], [sock,], [], 5)
        except select.error:
            sock.shutdown(2)
            sock.close()
            print('Client has disconnected')
            connected = False
        if len(ready_to_read) > 0:
            data = sock.recvfrom(bufferSize)
            print(data[0].decode('utf-8'))