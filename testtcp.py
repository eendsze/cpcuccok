import socket

add = "127.0.0.1"
#port = socket.htons(6543)
port = 6543
ss = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
ss.bind((add, port))

while True:

    #ss.sendto(b'valakkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkmi', (add, port))

    data, addr = ss.recvfrom(10)
    print(addr, data)

