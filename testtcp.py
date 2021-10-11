import socket
import json
import time

add = "127.0.0.1"
badd = "0.0.0.0"
port = 6543
bport = 6544
ss = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
ss.bind((add, port))
#broadcast socket
bss = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
bss.bind((badd, bport))

while True:
    try:
        data, addr = ss.recvfrom(1000, socket.MSG_DONTWAIT)
        #print(addr, data)
        jres = json.loads(data)
        if('gps' in jres):
            print(jres)
        if('ang' in jres):
            print(jres)
    except BlockingIOError:
        pass
    try:
        data, addr = bss.recvfrom(1000, socket.MSG_DONTWAIT)
        print(addr, data)
    except BlockingIOError:
        pass
    #time.sleep(0.001)
    

