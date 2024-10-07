import socket

def main():
  ETH_P_ALL = 3
  sock = socket.socket(
    socket.AF_PACKET,
    socket.SOCK_RAW,
    socket.htons(ETH_P_ALL))
  sock.bind(("enp0s9", 0))

  packet = sock.recv(3000)
  sock.close()

  print(f"{repr(packet):s}")

  dst_addr = packet[:6]
  src_addr = packet[6:12]
  type_value = packet[12:14]
  payload = packet[14:]
  print(dst_addr.hex())
  print(src_addr.hex())
  print(type_value.hex())
  print(payload)
          

if __name__ == "__main__":
  main()
