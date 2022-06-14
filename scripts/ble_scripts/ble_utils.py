import argparse
import platform
import sys
import signal

def parse_ble_args(desc="Connects to BLE device",add_arg=None):
    addr_format = 'XX:XX:XX:XX:XX:XX'
    addr_len = 17

    # macOS uses 128 bit UUID to connect to BLE devices
    if platform.system() == "Darwin":
        addr_format = 'XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX'
        addr_len = 36
    
    timeout_def = 10.0

    parser = argparse.ArgumentParser(description=desc)

    # Add arguments that all utils will use
    parser.add_argument('addr', metavar='Address', type=str, help=f"Address of the form {addr_format}")
    parser.add_argument('-timeout', '-t', metavar='Timeout', type=float, help=f"Timeout when searching for BLE device. Default: {timeout_def}", nargs="?", default=timeout_def)

    # Allow utils to introduce their own args
    if add_arg:
        add_arg(parser)
    
    args = parser.parse_args()

    # Check formatting
    if args.addr and len(args.addr.lower()) != addr_len:
        raise ValueError("Invalid address supplied")

    return args

def handle_sigint():
    def signal_handler(signal, frame):
        sys.exit(0)
    signal.signal(signal.SIGINT, signal_handler)

LAB11 = 0x02e0