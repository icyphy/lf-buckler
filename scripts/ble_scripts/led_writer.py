import asyncio

from bleak import BleakClient, BleakError

from ble_utils import parse_ble_args, handle_sigint, LAB11
args = parse_ble_args('Communicates with buckler LED toggle characteristic')
addr = args.addr.lower()
timeout = args.timeout
handle_sigint()

# TODO: Make sure these UUIDs match the ones in the LF files.
LED_SERVICE_UUID = "0322efc0-c024-4ad5-b1b2-6295bec40bfa"
LED_CHAR_UUID    = "0322728a-c024-4ad5-b1b2-6295bec40bfa" # LED Char UUID: 728a 

LAB11 = 0x02e0

async def main(address):
    print(f"searching for device {address} ({timeout}s timeout)")
    try:
        async with BleakClient(address,timeout=timeout) as client:
            print(f"Connected to device {client.address}: {client.is_connected}")
            sv = client.services[LED_SERVICE_UUID]
            try:
                while True:
                    input("Press Enter key to toggle the LED")
                    value = bool(int(bytes(await client.read_gatt_char(LED_CHAR_UUID)).hex()))
                    await client.write_gatt_char(LED_CHAR_UUID, bytes([not value]))
            except Exception as e:
                print(f"\t{e}")
    except BleakError as e:
        print("not found")

if __name__ == "__main__":
    while True:
        asyncio.run(main(addr))
