# my-engineering-thesis
This repository contains the code used as a microcontroller software that controls the operation of the measuring module. Its task is to measure the output voltage of the strain gauge beam, convert it into an applied force and then send the result to the receiving device via Bluetooth. 
To make the measurement of the force possible, the calibration mode has been implemented, which is activated by pressing the SW2 button.
In addition, it is possible to tare a calibrated device by pressing the SW1 button.
The main loop of the program is shown in a diagram below:

![image](https://user-images.githubusercontent.com/53697813/159970123-a16ae531-2431-42c9-919b-fbbb04dd7497.png)

Block diagram of a measuring module with separated parts: analog (measuring), digital and power supply, as well as a photo of the module itself:

![image](https://user-images.githubusercontent.com/53697813/159971524-e5b11fcb-eef1-4472-a9c7-cabcc254a441.png)

![IMG_20170106_242553569](https://user-images.githubusercontent.com/53697813/159972083-29d9157f-8e0b-4e82-9ec8-62d1a30872ec.jpg)

![IMG_20170106_242603682](https://user-images.githubusercontent.com/53697813/159972178-9d6de8ac-f904-45e9-a623-ee8aa3033168.jpg)

At the end there is a photo of the entire measuring set which includes the previously described module:

![DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD](https://user-images.githubusercontent.com/53697813/159972745-a4504359-8dbf-4081-bd09-f246132232e8.jpg)

