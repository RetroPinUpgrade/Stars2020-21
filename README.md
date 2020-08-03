## Stars 2020

Note: This code has a dependency on BallySternOS - it won't build without those files. They're located in a repository here:
https://github.com/BallySternOS/BallySternOS
Read on for basic instructions on how to build this code.


### To use this code
* Download the zip file (Code > Download ZIP) or clone the repository to your hard drive.  
* Get the BallySternOS files ( BallySternOS.* and SelfTestAndAudit.* ) from the repository here:  
 * https://github.com/BallySternOS/BallySternOS/tree/master
 * (Code > Download ZIP)
* Unzip the Stars2020 repository and name the folder that it's in as:
  * Stars2020  
* Copy BallySternOS.* and SelfTestAndAudit.* into the Stars2020 folder
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
  
Refer to the PDF or Wiki for instructions on how to build the hardware.  
