import sys, socket, select
SERVER_ADDR = ''
SERVER_PORT = 28280

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = ('',28280)
sock.bind(server_address)
print(f'Starting up on {sock.getsockname()}')

sock.listen(1)

while True:
    print('Waiting for connection..')
    connection,client_address = sock.accept()
    connection.setblocking(False)
    
    print(f'Connected to {client_address}')
    while True:
        try:
            ready_to_read, ready_to_write, in_error = select.select([connection,], [connection,], [], 5)
        except select.error:
            connection.shutdown(2)
            connection.close()
            print('Client has disconnected')
            break
        if len(ready_to_read) > 0:
            data = connection.recv(1024)
            print(data.decode('utf-8'))