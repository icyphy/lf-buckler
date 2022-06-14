import asyncio

from bleak import BleakClient

from ble_utils import parse_ble_args, handle_sigint
args = parse_ble_args('Prints all service and characteristic data from specified BLE device')
addr = args.addr.lower()
timeout = args.timeout
handle_sigint()

async def main(address):
    print(f"searching for device {address} ({timeout}s timeout)")
    async with BleakClient(address,timeout=timeout) as client:
        print(f"Connected: {client.is_connected}")        
        for service in client.services:
            print(f"[Service] {service}")
            for char in service.characteristics:
                if "read" in char.properties:
                    try:
                        value = bytes(await client.read_gatt_char(char.uuid))
                        print(f"\t[Characteristic] {char} ({','.join(char.properties)}), Value: {value}")
                    except Exception as e:
                        print(f"\t[Characteristic] {char} ({','.join(char.properties)}), Value: {e}")

                else:
                    value = None
                    print(f"\t[Characteristic] {char} ({','.join(char.properties)}), Value: {value}")

                for descriptor in char.descriptors:
                    try:
                        value = bytes(
                            await client.read_gatt_descriptor(descriptor.handle)
                        )
                        print(f"\t\t[Descriptor] {descriptor}) | Value: {value}")
                    except Exception as e:
                        print(f"\t\t[Descriptor] {descriptor}) | Value: {e}")


if __name__ == "__main__":
    asyncio.run(main(addr))
