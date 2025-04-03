openocd -c "gdb_port 50000" -c "tcl_port 50004" -c "telnet_port 50008" -f ./openOCD/lpc4337.cfg -c "init" -c "halt 0" -c "flash write_image erase ./build/REMA_plusplus.bin 0x1A000000 bin" -c "reset run" -c "shutdown" 2>&1

bash -c 'read -n 1 -s -r -p "Press any key to continue"'
