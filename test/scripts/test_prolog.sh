# Start JLink server
echo STARTING JLINK SERVER
x-terminal-emulator -e "JLinkExe -device nrf52832_xxaa -if swd -speed 4000 -AutoConnect 1"
echo STARTING JLINK RTT CLIENT
JLinkRTTClient > $1 &