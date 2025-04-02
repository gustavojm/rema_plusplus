openocd -c "gdb_port 50000" -c "tcl_port 50004" -c "telnet_port 50008" -f ./openOCD/lpc4337.cfg -c "program ./build/REMA_plusplus.bin verify reset exit"

bash
