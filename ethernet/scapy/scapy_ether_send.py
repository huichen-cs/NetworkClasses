import scapy.layers
import scapy.layers.l2


def main():
  intf = "enp0s9"

  msg = input("Enter a message to send: ")
  packet = scapy.layers.l2.Ether()
  packet.src = "08:00:27:08:0d:a1"
  packet.dst = "08:00:27:cb:67:1d"
  packet.payload = scapy.packet.Raw(msg)
  scapy.layers.l2.sendp(packet, iface=intf)

if __name__ == "__main__":
  main()
