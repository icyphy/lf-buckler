from asyncore import write
import keyboard
import asyncio
import sys
from getpass import getpass

from bleak import BleakClient, BleakError

from ble_utils import parse_ble_args, handle_sigint
args = parse_ble_args('Print advertisement data from a BLE device')
addr = args.addr.lower()
timeout = args.timeout
handle_sigint()

SERVICE_UUID = "85e43f4d-b4a7-4c6f-ba86-2db3c40a2c83"
DRIVE_UUID = "85e47182-b4a7-4c6f-ba86-2db3c40a2c83"

class RobotController():
    def __init__(self, client, main_loop):
        self.robot = client
        # keep state for keypresses
        self.pressed = {"up": False, "down": False, "left": False, "right": False} 
        self.loop = main_loop
        keyboard.hook(self.on_key_event)

    def on_key_event(self, event):
        # if a key unrelated to direction keys is pressed, ignore
        if event.name not in self.pressed: return
        # if a key is pressed down
        if event.event_type == keyboard.KEY_DOWN:
            # if that key is already pressed down, ignore
            if self.pressed[event.name]: return
            # set state of key to pressed
            self.pressed[event.name] = True
        else:
            # set state of key to released
            self.pressed[event.name] = False

        # This function runs on a separate thread,
        # so we need to use a threadsafe function.
        print("\n" + str(self.pressed.values()))
        future = asyncio.run_coroutine_threadsafe(
          self.robot.write_gatt_char(DRIVE_UUID, bytes(self.pressed.values())),
          self.loop)

    def __enter__(self):
        return self

async def main(address):
    print(f"searching for device {address} ({timeout}s timeout)")
    try:
        async with BleakClient(address,timeout=timeout) as client:
            print("connected")
            loop = asyncio.get_event_loop()
            print("Current event loop from the main thread" + str(loop))
            robot = RobotController(client, loop)
            try:
                print("Use arrow keys to control robot")
                # Suspend the main task indefinitely and
                # allow write_gatt_char() subroutine to use
                # the event loop.
                while True:
                    await asyncio.sleep(1)
            except KeyboardInterrupt as e:
                sys.exit(0)
            finally:
                await client.disconnect()
                print("The client has disconnected.")
    except BleakError as e:
        print("not found")

if __name__ == "__main__":
    while True:
        # Start a loop in the main thread.
        asyncio.run(main(addr))
