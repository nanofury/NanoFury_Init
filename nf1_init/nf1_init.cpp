/*
 *   ==== NanoFury Project ====
 *
 *   NanoFury device test program
 *
 *   Copyright (c) 2013 Vladimir Strinski
 *
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
*/
#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	printf("nf1_init.exe [command] [setting]\r\n");
	printf("Commands:\r\n");
	printf("test [device serial number] - runs bitfury tests to that device\r\n");
	printf("fixID [ignored] - Tests for NanoFury devices and fixes Product String\r\n\r\n");

	switch(argc){
		case 2:
			if (wcscmp(argv[1],L"fixID")==0){
				printf("fixID mode specified.\r\n\r\n");
				spi_init(NULL, L"NanoFury NF1 v0.6");
				printf("\r\nfixID mode completed. Please run command again to read new values.\r\n\r\n");
			}
		case 3:
			if (wcscmp(argv[1],L"test")==0){
				printf("test mode specified for device %s.\r\n\r\n", argv[2]);
				if (spi_init(argv[2], NULL)){
					printf("DEVICE FOUND! Proceeding with testing... Press ESC to cancel testing at any time\r\n\r\n", argv[3]);
					spitest_main(osc50);
					nanofury_device_off(hndNanoFury);
					ReleaseMCP2210(hndNanoFury);
				}
				else{
					printf("Device not found. No tests have been performed.\r\n\r\n");
				}
				
			}
		case 4:
			if (wcscmp(argv[1],L"test")==0){
				int n = _wtoi(argv[3]);
				if ((n<48)||(n>56)) n=48;
				printf("test mode specified for device %s with %u bit speed.\r\n\r\n", argv[2], n);
				if (spi_init(argv[2], NULL)){
					printf("DEVICE FOUND! Proceeding with testing... Press ESC to cancel testing at any time\r\n\r\n", argv[3]);
					switch (n){
						case 49: spitest_main(osc49); break;
						case 50: spitest_main(osc50); break;
						case 51: spitest_main(osc51); break;
						case 52: spitest_main(osc52); break;
						case 53: spitest_main(osc53); break;
						case 54: spitest_main(osc54); break;
						case 55: spitest_main(osc55); break;
						case 56: spitest_main(osc56); break;
						default: spitest_main(osc48); break;
					}					
					nanofury_device_off(hndNanoFury);
					ReleaseMCP2210(hndNanoFury);
				}
				else{
					printf("Device not found. No tests have been performed.\r\n\r\n");
				}
				
			}
			break;
	default: spi_init(NULL, NULL);
	}

	//spi_init(true, L"NanoFury NF1 v0.6");
	//spi_init(true, NULL);

	printf("Press [ENTER] to exit...");
	getchar();

	return 0;
}

