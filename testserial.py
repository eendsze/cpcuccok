import serial
import json

try:
    ser = serial.Serial("/dev/ttyACM0", 115200)
except Exception as e:
    print(str(e))
    exit()

ser.timeout = 2

if ser.isOpen():
    ser.close()

try:
    ser.open()
except Exception as e:
    print(str(e))
    exit()
ser.flushInput()

ser.write("start 1000 2000 3000 4000 end \n".encode())

res = ser.readline()
print(res)
jres = json.loads(res)
print(jres)


ser.close()