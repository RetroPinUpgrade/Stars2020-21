MachineDiagnostics
==================

This is a standalone project. Create a directory called MachineDiagnostics
and put this .ino file in it. When connected (via USB) to a computer, this
program will make the Arduino output diagnostics to verify that the
Arduino can talk to MPU board.

Example of output:

Start of program
Monitoring VMA signal, looking for presence of M6800 processor
Saw no sign of M6800 -- program will continue
Attempting to read & write from U10
Testing U10A Control Register
The U10A control register passed
Testing U10B Control Register
The U10B control register passed
Testing U11A Control Register
The U11A control register passed
Testing U11B Control Register
The U11B control register passed
Testing the clock cycle (if this hangs here, the conncection to clock is faulty)
It took 19080 microseconds for 10000 clock cycles
Clock frequency (very) approximately = 524 kHz
DIP Bank 0 = 0xA2
DIP Bank 1 = 0xE2
DIP Bank 2 = 0xDF
DIP Bank 3 = 0x13
Starting interrupt tests - this is going to take 5 seconds to test
In 5 seconds, saw 602 U10B interrupts and 1490 U11A interrupts
Zero-crossing approx. 120.40 times a second
Display interrupt approx. 298.00 times a second
Interrupt tests done - if frequencies aren't approx 120 & 320, there's a problem with the interrupt line.
END OF TESTS
