import asyncio

from bleak import BleakClient, BleakError

from ble_utils import parse_ble_args, handle_sigint
args = parse_ble_args('Communicates with buckler display writer characteristic')
addr = args.addr.lower()
timeout = args.timeout
handle_sigint()

DISPLAY_SERVICE_UUID = "32e61089-2b22-4db5-a914-43ce41986c70"
DISPLAY_CHAR_UUID    = "32e6038a-2b22-4db5-a914-43ce41986c70"

LAB11 = 0x02e0

async def main(address):
    print(f"searching for device {address} ({timeout}s timeout)")
    try:
        async with BleakClient(address) as client:
            print(f"Connected to device {client.address}: {client.is_connected}")
            sv = client.services[DISPLAY_SERVICE_UUID]
            try:
                print("Type message and send with Enter key")
                while True:
                    display = input("")
                    await client.write_gatt_char(DISPLAY_CHAR_UUID, bytes(display, 'utf-8'))
            except Exception as e:
                print(f"\t{e}")
    except BleakError as e:
        print("not found")

if __name__ == "__main__":
    while True:
        asyncio.run(main(addr))
