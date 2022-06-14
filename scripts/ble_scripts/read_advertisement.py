import asyncio

from bleak import BleakScanner, BleakError

from ble_utils import parse_ble_args, handle_sigint, LAB11
args = parse_ble_args('Print advertisement data from a BLE device')
addr = args.addr.lower()
timeout = args.timeout
handle_sigint()

async def main(address):
    print(f"searching for device {address} ({timeout}s timeout)")
    try:
        dev = await BleakScanner.find_device_by_address(address,timeout=timeout)
        if dev:
            print(f"Found advertisement from: {dev.address}")
            print(f"Name: {dev.name}")
            for company,data in dev.metadata['manufacturer_data'].items():
                if company == LAB11:
                    print(f"Data: {data.hex()}")
        else:
            print("not found")
    except BleakError as e:
        print("error while scanning")


if __name__ == "__main__":
    while True:
        asyncio.run(main(addr))
