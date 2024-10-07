"""Send a packet via Ethernet

   Example
     python ethersend.py enp0s9 \
       "08:00:27:cb:67:1d" \
       "08:00:27:08:0d:a1" \
       "Hello, World!"
"""
import socket
import sys
import struct

def help():
  print("Usage: ethersend interface src_addr dst_addr payload")

def send_msg(intf, src_addr, dst_addr, payload):
  sock = socket.socket(
    socket.AF_PACKET,
    socket.SOCK_RAW,
    0)
  sock.bind((intf, 0))

  packet = struct.pack(
          "!6s6s2s", 
          bytes.fromhex(dst_addr.replace(":", "")),
          bytes.fromhex(src_addr.replace(":", "")),
          b"\x00\x00")

  sock.send(packet + payload.encode("utf-8"))
  sock.close()

def main():
    if len(sys.argv) != 5:
        help()
        return
    intf, src_addr, dst_addr, payload = sys.argv[1:]
    send_msg(intf, src_addr, dst_addr, payload)

if __name__ == "__main__":
  main()
