# BallySternOS

This repository contains the files needed program an Arduino (Nano) for several different pinball machines.

Each machine is represented here with a (name).ino and (name).h that are specific to a machine. 

In addition, this repo contains several files that are needed for any machine (support functionality):
BallySternOS.cpp - interface to the machine hardware. 
BallySternOS.h   
SelfTestAndAudit.cpp - base-level self-test modes & audit functions. 
SelfTestAndAudit.h. 

At the moment, code is available for the following machines:  
Stars (Stern, 1978)  
Black Jack (Bally, 1977)  
PinballBaseMachine - basic framework to build a new game from  
MachineDiagnostics - this standalone project will test the Arduino's ability to talk to the MPU board
[This repo contains project files for several projects dependent on BallySternOS, and you must create a separate directory for each project. Arduino requires the project (.ino) file be in a directoy of that same name]

Example instructions to get started with Stars:  
1) Create a Stars2020 directory (the directory has to be the same name as the project's .ino file for some reason) 
2) Get the files by hitting "Clone or Download" and then "Download Zip" 
3) Unpack the zip 
4) Put these files (and ONLY these files) in the Stars2020 directory (these are all you need for this game):  
Stars2020.ino  
BallySternOS.cpp  
BallySternOS.h  
SelfTestAndAudit.cpp  
SelfTestAndAudit.h  
SternStarsDefinitions.h  
5) Open Stars2020.ino in the Arduino IDE (https://www.arduino.cc/en/Main/Software)  
6) You may need a driver for your Nano. If it's a 3rd party Nano (opposed to one from Arduino), then it might use the "Old Bootloader". This is under Tools->Processor. I use a Mac, so I had to install the CH340 driver for my cheap boards  
7) Compile the project and upload via USB to the board  
8) Build the hardware (documented here: http://ikehamill.com/wp-content/uploads/2020/05/Stars2020.pdf , or in the PDF included in the ZIP if available)

