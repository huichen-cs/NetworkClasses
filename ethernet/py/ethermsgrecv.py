import socket

PROTOCOL_NUM = socket.htons(0x4321)

def main():
  sock = socket.socket(
    socket.AF_PACKET,
    socket.SOCK_RAW,
    PROTOCOL_NUM)
  sock.bind(("enp0s9", 0))

  packet = sock.recv(3000)
  sock.close()

  dst_addr = packet[:6]
  src_addr = packet[6:12]
  type_value = packet[12:14]
  protocol_packet = packet[14:]
  length = int.from_bytes(protocol_packet[0:2], byteorder="big")
  print(f"length={length}")
  payload = protocol_packet[2:2+length]
  print(f"{dst_addr.hex()}")
  print(f"{src_addr.hex()}")
  print(f"{type_value.hex()}")
  print(f"{payload}")
          

if __name__ == "__main__":
  main()
