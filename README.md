NanoFury_Init
=============

Test and init application for NanoFury NF1 devices

Summary
=======

This is a simple test program to excercise the NanoFury devices.

Portions of the program (spitest) were originally written by Bitfury as part of his initial chip testing.
Link to original source can be found at the links below (it appeared twice - once in each of the Russian and English forums):
https://bitcointalk.org/index.php?topic=228677.msg2515472#msg2515472
https://bitcointalk.org/index.php?topic=183368.msg2515577#msg2515577

This application can exercise individual NanoFury devices and can also fix the "Product String" which is a required change so that ''bfgminer'' can recognize all devices automatically (as opposed to having to individually supply the serial numbers).

Dependencies
============

Note that it depends on the MCP2210-Library library as modified to compile for Windows.
Clone it from https://github.com/nanofury/MCP2210-Library

The library and all files need to go to: \NanoFury_Init\nf1_init\MCP2210-Library


This project requires Microsoft Visual Studio 2010 or newer.


Authors
=======

The following individuals had major contributions to this project:
 * Bitfury - initial sample applications (for Raspberry Pi) excercising the bitfiry chips
 * Valeria Pelova - port of the MCP2210 library and making everything actually compile under Windows and Visual Studio 2010
 * Vladimir Strinski - NanoFury-specific drivers and low-level software development
