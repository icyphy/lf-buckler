import asyncio
import struct

from bleak import BleakScanner, BleakError

from ble_utils import parse_ble_args, handle_sigint, LAB11
args = parse_ble_args('Print light value from Buckler')
addr = args.addr.lower()
timeout = args.timeout
handle_sigint()

async def main(address):
    print(f"searching for device {address} ({timeout}s timeout)")
    try:
        dev = await BleakScanner.find_device_by_address(address,timeout=timeout)
        if dev:
            print(f"found device {dev.address}")
            for company,data in dev.metadata['manufacturer_data'].items():
                if company == LAB11:
                    try:
                        # here we are unpacking a little-endian (<) float (f), as well as 20 dummy bytes (20x)
                        values = struct.unpack('<f20x', data)
                        print("Value: " + str(values[0]) + " lux")
                    except:
                        print("Got bad packet format")
        else:
            print("not found")
    except BleakError as e:
        print("error while scanning")


if __name__ == "__main__":
    while True:
        asyncio.run(main(addr))
