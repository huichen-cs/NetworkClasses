"""Send a packet via Ethernet

   Example
     python ethermsgsend.py enp0s9 \
       "08:00:27:cb:67:1d" \
       "08:00:27:08:0d:a1" \
       "Hello, World!"
"""
import socket
import sys
import struct

PROTOCOL_NUM = 0x4321

def help():
  print("Usage: ethersend interface src_addr dst_addr payload")

def protocol_packet(msg):
    return len(msg).to_bytes(length=2, byteorder='big') + msg.encode("utf-8")

def send_msg(intf, src_addr, dst_addr, payload):
  sock = socket.socket(
    socket.AF_PACKET,
    socket.SOCK_RAW,
    PROTOCOL_NUM)
  sock.bind((intf, 0))

  packet = struct.pack(
          "!6s6sh", 
          bytes.fromhex(dst_addr.replace(":", "")),
          bytes.fromhex(src_addr.replace(":", "")),
          PROTOCOL_NUM)

  payload = protocol_packet(payload)
  sock.send(packet + payload)
  sock.close()

def main():
    if len(sys.argv) != 5:
        help()
        return
    intf, src_addr, dst_addr, payload = sys.argv[1:]
    send_msg(intf, src_addr, dst_addr, payload)

if __name__ == "__main__":
  main()
