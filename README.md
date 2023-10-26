# REMA Remote Terminal Unit  
Firmware for **REMA** Heat Exchanger Inspection Robot  

## Requirements  
arm-none-eabi  
CMake  
OpenOCD ver > 0.12  
Doxygen  

## Getting Started  
Create a directory  

`mkdir REMA`  
`cd REMA`  

Clone this repos  
`git clone git@github.com:gustavojm/lpc_chip_43xx.git`  
`git clone https://github.com/gustavojm/CIAA_NXP_board.git`  
`git clone https://github.com/gustavojm/rema_plusplus.git`  

Configure the project  
`cd rema_plusplus`  
`cmake -B ./build`  

Compile the project  
`cmake --build ./build`  

Flash the microcontroller  
`cmake --build ./build --target flash`  

Change project settings  
`ccmake ./build`  

Clean the project  
`cmake --build ./build --target clean`  

