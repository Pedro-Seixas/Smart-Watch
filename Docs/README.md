# How to get it working

## Softwares needed:
- STM32CubeIDE
- STM32CubeProgrammer

## Install necessary files
- Clone the repository to a folder and in STM32CubeIDE open the project

## Upload to the device
- Build the project and open STM32CubeProgrammer.
- Select USB since we are doing DFU (See 1).
- Click "Open File" and navigate to your project folder, go to the Debug folder and select the file with the .elf extension (See 2).
- Remove the device from the USB, hold the boot button (SW2) and connect the device to the computer. Release the boot button.
- Click the refresh icon in the STM32CubeProgrammer software (See 3), it should find the device. Now, click in "Connect" (See 4). Finally, click in "Download" (See 5).
- After the download is complete, press the Reset button (SW1) or remove and connect the USB.

<img width="1200" height="697" alt="image" src="https://github.com/user-attachments/assets/4385ec0f-5ed3-4a44-a202-5fec2f3eec21" />

