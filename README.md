## Stars 2020

### To use this code
* Download the zip file (Code > Download ZIP) or clone the repository to your hard drive.  
* Unzip the Stars2020 repository and name the folder that it's in as:
  * Stars2020  
* REMOVE the file Stars2021.ino from the folder (separate instructions below if you want to build Stars2021)  
* Open the Stars2020.ino in Arduino's IDE
* At the top of Stars2020.ino and you'll see a couple of parameters in #define statements
  * if you want to use your machine's chimes, make sure this line is uncommented:  
   * #define USE_CHIMES  
  * if you have a Wav Trigger installed, uncomment this line 
    * #define USE_WAV_TRIGGER
    * or 
    * #define USE_WAV_TRIGGER_1p3 
  * NOTE: the code is pushing space limits with both Wav Trigger & Chimes uncommented. You may experience issues if you have both options compiled into your code. If you intend to use the chimes, comment out the Wav Trigger lines (they're commented by default), and if you want to use a Wav Trigger, please comment out the line 
    * #define USE_CHIMES  

  
A zip file with all the WAV files for the sound effects is here:  
https://drive.google.com/file/d/10W-ejHtfAnJoimFzfkmMeYZWIZv4Plgj/view?usp=sharing  
  
Refer to the PDF or Wiki for instructions on how to build the hardware.  
  
  
## Stars 2021
If you'd like to try a different version, follow the steps above except name the folder "Stars2021" and remove the Stars2020.ino file. That will compile Stars2021, which has different rules and deeper challenges.  
Note: Stars2021 doesn't support Chimes. For audio, you'll have to use a Wav Trigger board.  
