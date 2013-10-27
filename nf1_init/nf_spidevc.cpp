/*
 *   ==== NanoFury Project ====
 *
 *   nf_spidevc.c - SPI library for NanoFury/bitfury chip/board
 *
 *   Copyright (c) 2013 bitfury
 *   Copyright (c) 2013 Vladimir Strinski
 *
 *
 *  Originally written by Bitfury
 *  with major modifications for the NanoFury project by Vladimir Strinski
 *
 *  Link to original source can be found at:
 *  https://bitcointalk.org/index.php?topic=183368.msg2515577#msg2515577
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

#include <string.h>

#include "nf_spidevc.h"


#include "MCP2210-Library/mcp2210.h"
#include <Windows.h>

hid_device *hndNanoFury = NULL;

ChipSettingsDef nf_chipDef;
SPITransferSettingsDef nf_spiDef;

bool nanofury_checkport(hid_device *handle){
	ChipSettingsDef chipDef;
	SPITransferSettingsDef spiDef;
	GPPinDef pinDef;
	int r;
	byte spiCmdBuffer[16];
	SPIDataTransferStatusDef dtstat;

	spiCmdBuffer[0]=0;

	chipDef = GetChipSettings(handle);
	if (chipDef.ErrorCode != 0) {
		printf("Error in GetChipSettings [%u].\n",chipDef.ErrorCode);
		return false;
	}

	// default: set everything to input
	for (int i = 0; i < 9; i++) {
		chipDef.GP[i].PinDesignation = GP_PIN_DESIGNATION_GPIO;
		chipDef.GP[i].GPIODirection = GPIO_DIRECTION_INPUT;
		chipDef.GP[i].GPIOOutput = 0;
	}
	
	// configure the pins that we need:

	//LED
	chipDef.GP[NANOFURY_GP_PIN_LED].GPIODirection = GPIO_DIRECTION_OUTPUT;
	chipDef.GP[NANOFURY_GP_PIN_LED].PinDesignation = GP_PIN_DESIGNATION_GPIO;
	chipDef.GP[NANOFURY_GP_PIN_LED].GPIOOutput = 1;	// turn LED ON

	//SCK_OVRRIDE
	//chipDef.GP[NANOFURY_GP_PIN_SCK_OVR].GPIODirection = GPIO_DIRECTION_INPUT;		// already initialized as input = no override
	//chipDef.GP[NANOFURY_GP_PIN_SCK_OVR].PinDesignation = GP_PIN_DESIGNATION_GPIO;

	//PWR_EN
	chipDef.GP[NANOFURY_GP_PIN_PWR_EN].GPIODirection = GPIO_DIRECTION_OUTPUT;
	chipDef.GP[NANOFURY_GP_PIN_PWR_EN].PinDesignation = GP_PIN_DESIGNATION_GPIO;
	chipDef.GP[NANOFURY_GP_PIN_PWR_EN].GPIOOutput = 1;		// turn power ON

	// GP4= Chip Select (not used; just for debugging)
	chipDef.GP[4].GPIODirection = GPIO_DIRECTION_OUTPUT;
	chipDef.GP[4].PinDesignation = GP_PIN_DESIGNATION_CS;

	r = SetChipSettings(handle, chipDef);
	if (r != 0) {
		printf("Error in SetChipSettings [%u].\n",r);
		return false;
	}

	//configure SPI
	spiDef = GetSPITransferSettings(handle);
	if (spiDef.ErrorCode != 0) {
		printf("Error in GetSPITransferSettings [%u].\n",spiDef.ErrorCode);
		return false;
	}

	// This is the only place where speed, mode and other settings are configured!!!
	// TODO: This should probably be a const
	spiDef.ActiveChipSelectValue = 0xFFEF;//chip select is GP4
	spiDef.IdleChipSelectValue = 0xffff;
	spiDef.BitRate = 200000l;		// default to 200kHz
	spiDef.BytesPerSPITransfer = 1;
	spiDef.CSToDataDelay = 0;
	spiDef.LastDataByteToCSDelay = 0;
	spiDef.SubsequentDataByteDelay = 0;
	spiDef.SPIMode = 0;

	r = SetSPITransferSettings(handle, spiDef);
	if (r != 0) {
		printf("Error in SetSPITransferSettings [%u].\n",r);
		return false;
	}
	dtstat = SPISendReceive(handle, spiCmdBuffer, 1, 1);	//send a dummy byte (buff[0]=0) to force new settings
	if (dtstat.ErrorCode != 0) {
		printf("Error in SPISendReceive [%u].\n",dtstat.ErrorCode);
		return false;
	}

	// after this command SCK_OVRRIDE should read the same as current SCK value (which for mode 0 should be 0)

	pinDef = GetGPIOPinValue(handle);
	if (pinDef.ErrorCode != 0) {
		printf("Error in GetGPIOPinValue [%u].\n",pinDef.ErrorCode);
		return false;
	}
	if (pinDef.GP[NANOFURY_GP_PIN_SCK_OVR].GPIOOutput!=0){
		printf("Error : Expected reading pin value 0 but read [%u].\n",pinDef.GP[NANOFURY_GP_PIN_SCK_OVR].GPIOOutput);
		return false;
	}

	// switch SCK to polarity (default SCK=1 in mode 2)
	spiDef.SPIMode = 2;
	r = SetSPITransferSettings(handle, spiDef);
	if (r != 0) {
		printf("Error in SetSPITransferSettings [%u].\n",r);
		return false;
	}
	dtstat = SPISendReceive(handle, spiCmdBuffer, 1, 1);	//send a dummy byte (buff[0]=0)
	if (dtstat.ErrorCode != 0) {
		printf("Error in SPISendReceive [%u].\n",dtstat.ErrorCode);
		return false;
	}

	// after this command SCK_OVRRIDE should read the same as current SCK value (which for mode 2 should be 1)

	pinDef = GetGPIOPinValue(handle);
	if (pinDef.ErrorCode != 0) {
		printf("Error in GetGPIOPinValue [%u].\n",pinDef.ErrorCode);
		return false;
	}
	if (pinDef.GP[NANOFURY_GP_PIN_SCK_OVR].GPIOOutput!=1){
		printf("Error : Expected reading pin value 1 but read [%u].\n",pinDef.GP[NANOFURY_GP_PIN_SCK_OVR].GPIOOutput);
		return false;
	}


	// switch SCK to polarity (default SCK=0 in mode 0)
	spiDef.SPIMode = 0;
	r = SetSPITransferSettings(handle, spiDef);
	if (r != 0) {
		printf("Error in SetSPITransferSettings [%u].\n",r);
		return false;
	}
	dtstat = SPISendReceive(handle, spiCmdBuffer, 1, 1);	//send a dummy byte (buff[0]=0)
	if (dtstat.ErrorCode != 0) {
		printf("Error in SPISendReceive [%u].\n",dtstat.ErrorCode);
		return false;
	}

	pinDef = GetGPIOPinValue(handle);
	if (pinDef.ErrorCode != 0) {
		printf("Error in GetGPIOPinValue [%u].\n",pinDef.ErrorCode);
		return false;
	}
	if (pinDef.GP[NANOFURY_GP_PIN_SCK_OVR].GPIOOutput!=0){
		printf("Error : Expected reading pin value 0 but read [%u].\n",pinDef.GP[NANOFURY_GP_PIN_SCK_OVR].GPIOOutput);
		return false;
	}

	return true;
}

void nanofury_device_off(hid_device *handle){
	ChipSettingsDef chipDef;
	int r;
	byte spiCmdBuffer[16];

	spiCmdBuffer[0]=0;

	chipDef = GetChipSettings(handle);
	if (chipDef.ErrorCode != 0) {
		printf("Error in GetChipSettings [%u].\n",chipDef.ErrorCode);
		return;
	}

	chipDef.GP[NANOFURY_GP_PIN_PWR_EN].GPIOOutput = 0;		// turn power OFF
	chipDef.GP[NANOFURY_GP_PIN_LED].GPIOOutput = 0;		// turn LED OFF
	chipDef.GP[NANOFURY_GP_PIN_SCK_OVR].GPIODirection = GPIO_DIRECTION_INPUT;		// make sure SCK_OVR is deactivated
	r = SetChipSettings(handle, chipDef);
}

bool spi_init(wchar_t* szTestDeviceID, wchar_t* szFixProductString)
{
	// just list all devices
	int dev_count; 
	struct hid_device_info *devs, *cur_dev;
	hid_device *handle = NULL;

	devs = EnumerateMCP2210();
	cur_dev = devs;
	dev_count = 0;
	while (cur_dev) {
		dev_count++;
		wprintf(L"Testing device %d : SERIAL=%s, ", dev_count, cur_dev->serial_number);
		printf(" Path=[%s]\r\n",cur_dev->path);

		handle = InitMCP2210(cur_dev->serial_number);
		if (nanofury_checkport(handle)){
			if ((szTestDeviceID!=NULL) && (wcscmp(szTestDeviceID,cur_dev->serial_number)==0)){
				hndNanoFury = handle;		// okay, we've found a good device! Stop looking any further.
				break;
			}
			wprintf(L"NanoFury Test: PASS, Product String=[%s], Manufacturer=[%s]\r\n", cur_dev->product_string, cur_dev->manufacturer_string);

			if (szFixProductString!=NULL){
				ManufacturerProductNameDef def = GetManufacturerProductName(handle, CMDSUB_USB_PRODUCT_NAME);

				def.USBStringDescriptorLength=wcslen(szFixProductString);
				if (def.USBStringDescriptorLength<59){				
					wcscpy((wchar_t*) &def.ManufacturerProductName, szFixProductString);
					def.USBStringDescriptorLength++;
					def.USBStringDescriptorLength*=2;
					def.USBStringDescriptorID = 0x03;

					int res = SetManufacturerProductName(handle,CMDSUB_USB_PRODUCT_NAME,def);
					switch (res){
					case 0: printf("Product Name Set = OK\r\n"); break;
					default: printf("Product Name Set = FAIL, error=0x%08x\r\n", res);
					}
				}

			}

			nanofury_device_off(handle);
			ReleaseMCP2210(handle);
		}
		else{
			wprintf(L"NanoFury Test: FAIL, Product String=[%s]\r\n", cur_dev->product_string);
			ReleaseMCP2210(handle);
		}

		cur_dev = cur_dev->next;
	}

	hid_free_enumeration(devs);

	return hndNanoFury!=NULL;
}

// Bit-banging reset, to reset more chips in chain - toggle for longer period... Each 3 reset cycles reset first chip in chain
void spi_reset(hid_device *handle)
{
	ChipSettingsDef chipDef;
	SPITransferSettingsDef spiDef;
	int r;
	byte spiCmdBuffer[16];
	SPIDataTransferStatusDef dtstat;

	if (handle==NULL){		// this functon should never be called unless we have a device handle;
		printf("Error in spi_reset : handle is NULL! This function should never have been called without an initialized device!\n");
		printf("Something bad has happened. Program will exit. Press [ENTER] to exit...");
		getchar();
      exit(1);
		return;	
	}

	chipDef = GetChipSettings(handle);
	//SCK_OVRRIDE
	chipDef.GP[NANOFURY_GP_PIN_SCK_OVR].GPIODirection = GPIO_DIRECTION_OUTPUT;			// set override
	chipDef.GP[NANOFURY_GP_PIN_SCK_OVR].PinDesignation = GP_PIN_DESIGNATION_GPIO;
	chipDef.GP[NANOFURY_GP_PIN_SCK_OVR].GPIOOutput = 1;
	r = SetChipSettings(handle, chipDef);
	if (r != 0) {
		printf("Error in spi_reset : SetChipSettings = [%u].\n",r);
		return;
	}

	//configure SPI
	spiDef = GetSPITransferSettings(handle);
	if (spiDef.ErrorCode != 0) {
		printf("Error in spi_reset : GetSPITransferSettings = [%u].\n",spiDef.ErrorCode);
		return;
	}
	spiDef.BytesPerSPITransfer = 1;		// we'll be sending 1 byte at a time
	r = SetSPITransferSettings(handle, spiDef);
	if (r != 0) {
		printf("Error in spi_reset : SetSPITransferSettings = [%u].\n",r);
		return;
	}

	spiCmdBuffer[0]=0x81;		// will send this waveform: - _ _ _  _ _ _ -
	for (r=0; r<16; r++){
		dtstat = SPISendReceive(handle, spiCmdBuffer, 1, 1);
		if (dtstat.ErrorCode != 0) {
			printf("Error in spi_reset : SPISendReceive = [%u].\n",dtstat.ErrorCode);
			return;
		}
	}

	//SCK_OVRRIDE
	chipDef.GP[NANOFURY_GP_PIN_SCK_OVR].GPIODirection = GPIO_DIRECTION_INPUT;			// deactivate override
	r = SetChipSettings(handle, chipDef);
	if (r != 0) {
		printf("Error in spi_reset : SetChipSettings = [%u].\n",r);
		return;
	}

}

int spi_txrx(hid_device *handle, const char *wrbuf, char *rdbuf, int bufsz)
{
	SPITransferSettingsDef spiDef;
	int r;
	SPIDataTransferStatusDef dtstat;

	byte* ptrwrbuf = (byte*) wrbuf;
	byte* ptrrdbuf = (byte*) rdbuf;

	if (handle==NULL){		// this functon should never be called unless we have a device handle;
		printf("Error in spi_txrx : handle is NULL! This function should never have been called without an initialized device!\n");
		printf("Something bad has happened. Program will exit. Press [ENTER] to exit...");
		getchar();
      exit(1);
		return -1;	
	}

	spi_reset(handle);

	//configure SPI
	spiDef = GetSPITransferSettings(handle);
	if (spiDef.ErrorCode != 0) {
		printf("Error in spi_reset : GetSPITransferSettings = [%u].\n",spiDef.ErrorCode);
		return -1;
	}

	if (bufsz>=NANOFURY_MAX_BYTES_PER_SPI_TRANSFER){		// start by sending chunks of 60 bytes...
		spiDef.BytesPerSPITransfer = NANOFURY_MAX_BYTES_PER_SPI_TRANSFER;		// we'll be sending 60 byte at a time
		r = SetSPITransferSettings(handle, spiDef);
		if (r != 0) {
			printf("Error in spi_reset : SetSPITransferSettings = [%u].\n",r);
			return -1;
		}
		while (bufsz>=NANOFURY_MAX_BYTES_PER_SPI_TRANSFER){
			dtstat = SPISendReceive(handle, ptrwrbuf, NANOFURY_MAX_BYTES_PER_SPI_TRANSFER);
			while (dtstat.ErrorCode==0xF8){	// 0xF8 = busy, try (polling for returned data) again
				//Sleep(1);
				dtstat = SPISendReceive(handle, ptrwrbuf, NANOFURY_MAX_BYTES_PER_SPI_TRANSFER);
			}
			if (dtstat.ErrorCode==0xF7){
				printf("Failed to transfer data [error=%u].\r\n",dtstat.ErrorCode);
				return dtstat.ErrorCode;
			}
			if (dtstat.NumberOfBytesReceived!=NANOFURY_MAX_BYTES_PER_SPI_TRANSFER){
				printf("Failed to transfer data [expecting 60 bytes, got %u].\r\n",dtstat.NumberOfBytesReceived);
				return -1;
			}

			memcpy(ptrrdbuf, dtstat.DataReceived, NANOFURY_MAX_BYTES_PER_SPI_TRANSFER);
			
			ptrrdbuf += NANOFURY_MAX_BYTES_PER_SPI_TRANSFER;
			ptrwrbuf += NANOFURY_MAX_BYTES_PER_SPI_TRANSFER;
			bufsz -= NANOFURY_MAX_BYTES_PER_SPI_TRANSFER;
		}
	}

	if (bufsz>0){		// send any remaining bytes...
		spiDef.BytesPerSPITransfer = bufsz;
		r = SetSPITransferSettings(handle, spiDef);
		if (r != 0) {
			printf("Error in spi_reset : SetSPITransferSettings = [%u].\n",r);
			return -1;
		}

		dtstat = SPISendReceive(handle, ptrwrbuf, bufsz);
		while (dtstat.ErrorCode==0xF8){	// 0xF8 = busy, try (polling for returned data) again
			//Sleep(1);
			dtstat = SPISendReceive(handle, ptrwrbuf, bufsz);
		}
		if (dtstat.ErrorCode==0xF7){
			printf("Failed to transfer data [error=%u].\r\n",dtstat.ErrorCode);
			return dtstat.ErrorCode;
		}
		if (dtstat.NumberOfBytesReceived!=bufsz){
			printf("Failed to transfer data [expecting %u bytes, got %u].\r\n",bufsz,dtstat.NumberOfBytesReceived);
			return -1;
		}

		memcpy(ptrrdbuf, dtstat.DataReceived, bufsz);
			
		ptrrdbuf += bufsz;
		ptrwrbuf += bufsz;
		bufsz -= bufsz;
		
	}

	return 0;
}

#define SPIMAXSZ 256*1024
static unsigned char spibuf[SPIMAXSZ], spibuf_rx[SPIMAXSZ];
static unsigned spibufsz;

void spi_clear_buf(void) { spibufsz = 0; }
unsigned char *spi_getrxbuf(void) { return spibuf_rx; }
unsigned char *spi_gettxbuf(void) { return spibuf; }
unsigned spi_getbufsz(void) { return spibufsz; }

void spi_emit_buf_reverse(const char *str, unsigned sz)
{
	unsigned i;
	if (spibufsz + sz >= SPIMAXSZ) return;
	for (i = 0; i < sz; i++) { // Reverse bit order in each byte!
		unsigned char p = str[i];
		p = ((p & 0xaa)>>1) | ((p & 0x55) << 1);
		p = ((p & 0xcc)>>2) | ((p & 0x33) << 2);
		p = ((p & 0xf0)>>4) | ((p & 0x0f) << 4);
		spibuf[spibufsz+i] = p;
	}
	spibufsz += sz;
}

void spi_emit_buf(const char *str, unsigned sz)
{
	if (spibufsz + sz >= SPIMAXSZ) return;
	memcpy(&spibuf[spibufsz], str, sz); spibufsz += sz;
}

/* TODO: in production, emit just bit-sequences! Eliminate padding to byte! */
void spi_emit_break(void) { spi_emit_buf("\x4", 1); }
void spi_emit_fsync(void) { spi_emit_buf("\x6", 1); }
void spi_emit_fasync(void) { spi_emit_buf("\x5", 1); }

void spi_emit_data(unsigned addr, const char *buf, unsigned len)
{
	char otmp[3];
	if (len < 4 || len > 128) return; /* This cannot be programmed in single frame! */
	len /= 4; /* Strip */
	otmp[0] = (len - 1) | 0xE0;
	otmp[1] = (addr >> 8)&0xFF; otmp[2] = addr & 0xFF;
	spi_emit_buf(otmp, 3);
	spi_emit_buf_reverse(buf, len*4);
}
