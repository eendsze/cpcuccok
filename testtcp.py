import socket
import json

add = "127.0.0.1"
#port = socket.htons(6543)
port = 6543
ss = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
ss.bind((add, port))

while True:

    data, addr = ss.recvfrom(1000)
    #print(addr, data)
    jres = json.loads(data)
    if('gps' in jres):
        print(jres)
    if('ang' in jres):
        print(jres)
        

