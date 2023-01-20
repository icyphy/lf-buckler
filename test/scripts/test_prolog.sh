# Start JLink server
echo STARTING JLINK SERVER  
case $( uname -s ) in 
Darwin) osascript -e 'tell application "Terminal" to do script "JLinkExe -device nrf52832_xxaa -if swd -speed 4000 -AutoConnect 1"';; 
*) x-terminal-emulator -e "JLinkExe -device nrf52832_xxaa -if swd -speed 4000 -AutoConnect 1";;
esac

echo STARTING JLINK RTT CLIENT
JLinkRTTClient > $1 &
