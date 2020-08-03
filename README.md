## Stars 2020

Note: This code has a dependency on BallySternOS - it won't build without those files. They're located in a repository here:
https://github.com/BallySternOS/BallySternOS
Read on for basic instructions on how to build this code.


### To use this code
* Download the zip file (Code > Download ZIP) or clone the repository to your hard drive.  
* Get the BallySternOS files ( BallySternOS.* and SelfTestAndAudit.* ) from the repository here:  https://github.com/BallySternOS/BallySternOS/tree/master
* Put all the files from this repository in the same folder as the BallySternOS files and name that folder:
** Stars2020  
* Open the Stars2020.ino in Arduino's IDE
* Look at the top of Stars2020.ino and you'll see a couple of parameters in #define statements
** if you want to use your machine's chimes, make sure this line is uncommented:  #define USE_CHIMES  
** if you have a Wav Trigger installed, uncomment this line #define USE_WAV_TRIGGER or #define USE_WAV_TRIGGER_1p3 

Refer to the PDF or Wiki for instructions on how to build the hardware.  
