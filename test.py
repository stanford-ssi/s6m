# import serial
# import time
# import csv
# import json

# ser = serial.Serial('COM17')
# ser.flushInput()

# ser.write('{"id":"tx","data":"TWFuIGl"}\n'.encode('utf-8'))

# while True:
#     ser_bytes = ser.readline()
#     decoded_bytes = ser_bytes.decode("utf-8")
#     data = json.loads(decoded_bytes)
#     if data["id"] == 
#     print(data)
import sys
from serial import *
from tkinter import *
import json
import base64

serialPort = sys.argv[1]
baudRate = 9600
ser = Serial(serialPort , baudRate, timeout=0, writeTimeout=0) #ensure non-blocking

#make a TkInter Window
root = Tk()
root.wm_title("S6M Chat")

# make a scrollbar
scrollbar = Scrollbar(root)
scrollbar.pack(side=RIGHT, fill=Y)

# make a text box to put the serial output
console = Text ( root, width=120, height=20, takefocus=0)
console.pack()

log = Text ( root, width=120, height=20, takefocus=0)
log.pack()

# attach text box to scrollbar
log.config(yscrollcommand=scrollbar.set)
scrollbar.config(command=log.yview)

e = Entry(root)
e.pack()

#make our own buffer
#useful for parsing commands
#Serial.readline seems unreliable at times too
serBuffer = ""

def process(str):
    data = json.loads(str)
    if data["id"] == "radio" :
        if data["msg"] == "RX" :
            msg = base64.b64decode(data["data"]).decode("utf-8")
            msg = "RX>> " + msg + "\n"
            console.insert(END, msg)
        elif data["msg"] == "TX" :
            msg = base64.b64decode(data["data"]).decode("utf-8")
            msg = "TX>> " + msg + "\n"
            console.insert(END, msg)

def readSerial():
    while True:
        c = ser.read() # attempt to read a character from Serial
        
        #was anything read?
        if len(c) == 0:
            break
        
        # get the buffer from outside of this function
        global serBuffer
        
        # check if character is a delimeter
        if c == b'\r':
            c = b'' # don't want returns. chuck it
            
        if c == b'\n':
            serBuffer += "\n" # add the newline to the buffer
            #add the line to the TOP of the log
            process(serBuffer)
            log.insert(END, serBuffer)
            serBuffer = "" # empty the buffer
        else:
            try:
                serBuffer += c.decode("utf-8")  # add to the buffer
            except UnicodeDecodeError as a:
                print("rip")

    root.after(10, readSerial) # check serial again soon

def callback(event):
    msg = e.get()
    data = base64.b64encode(msg.encode("utf-8")).decode("utf-8")
    data = json.dumps({"id":"tx","data":data}, separators=(',', ':'))
    data += '\n'
    ser.write(data.encode('utf-8'))
    e.delete(0, END)

# after initializing serial, an arduino may need a bit of time to reset
root.after(100, readSerial)

root.bind('<Return>', callback)

root.mainloop()


