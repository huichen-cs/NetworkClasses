import signal
import sys
import scapy.all as scapy

def signal_handler(sig, frame):
    print('You pressed Ctrl+C!')
    sys.exit(0)

def main():
  signal.signal(signal.SIGINT, signal_handler) 
  while True:
    print("Waiting for message to arrive.")
    packets = scapy.sniff(
            iface="enp0s9",
            filter="ether src 08:00:27:08:0d:a1",
            count=1
    )
    src = packets[0].src
    msg = packets[0].payload.load.decode("utf-8")
    print(f"Received from {src}: {msg}")

if __name__ == "__main__":
    main()
