//heap size is 86kb
//bmp header size 54b

#include "xsdps.h"  //sd card
#include "xgpio.h"  //gpio
#include "xparameters.h"    //configuration parameters
#include "xstatus.h"    //status codes
#include <stdint.h>
#include <stdio.h> 
#include <sys/types.h>
#include <xil_cache.h>
#include <xil_printf.h>
#include <xil_types.h>
#include <xsdps_hw.h>
#include "xilffs.h"
#include <ff.h>
#include "xaxidma.h"

#define LED_CHANNEL 1
#define BUTTON_CHANNEL 1

#define HEAP_SIZE 3584
#define DMA_TRANSFER_SIZE 512

XGpio xgpioLed;
XGpio xgpioButton;

XSdPs sdCard;
XSdPs_Config* sdConfig;

FATFS FatFs;
FRESULT FResult;

XAxiDma dma;
XAxiDma_Config * dmaConfig;

FIL imageFile;
int IMAGE_WIDTH = 0;
int IMAGE_HEIGHT = 0;
int IMAGE_PIXELS = 0;
int PIXELS_OFFSET = 0;
int FILE_SIZE = 0;
int BLOCK_SIZE = 512;
int BLOCK_COUNT = 0;

u32 buttonValues = 0x0;
u8* imageBuffer;

int imageNotFound = 0;

int statusSdInit;
int statusSdCfgInit;

int initGpio(){
    int statusLed = XGpio_Initialize(&xgpioLed, XPAR_AXI_GPIO_LEDS_BASEADDR); //initialize GPIO
    int statusButton = XGpio_Initialize(&xgpioButton, XPAR_AXI_GPIO_BUTTONS_BASEADDR);

    if(statusLed != XST_SUCCESS || statusButton != XST_SUCCESS){
        xil_printf("Failed to init buttons leds");
        return XST_FAILURE;
    }

    XGpio_SetDataDirection(&xgpioLed, LED_CHANNEL, 0x0);   //0 output, 1 input
    XGpio_SetDataDirection(&xgpioButton, BUTTON_CHANNEL, 0xF);

    return XST_SUCCESS;
}

int initSdCard(){
    sdConfig = XSdPs_LookupConfig(XPAR_XSDPS_0_BASEADDR);
    statusSdCfgInit = XSdPs_CfgInitialize(&sdCard, sdConfig,sdConfig->BaseAddress);
    statusSdInit = XSdPs_CardInitialize(&sdCard);

    if(statusSdInit != XST_SUCCESS || statusSdCfgInit != XST_SUCCESS){
        xil_printf("Failed to init sd");
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}

int initFatFs(){
    FResult = f_mount(&FatFs, "0:/", 1);  
    
    if(FResult != FR_OK){
        xil_printf("Failed to mount sd");
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}

long get_info_about_image(FIL* imageFile){

    imageBuffer = (u8*)(malloc(54));

    if(imageBuffer == NULL){
        return XST_FAILURE;
    }

    int noOfBytes;    
    FRESULT imageResult = f_read(imageFile, imageBuffer, 54, &noOfBytes);

    if(imageResult != XST_SUCCESS || noOfBytes != 54){
        return XST_FAILURE;
    }

    FILE_SIZE = f_size(imageFile);
    PIXELS_OFFSET = imageBuffer[10] + (imageBuffer[11] << 8) + (imageBuffer[12] << 16) + (imageBuffer[13] << 24);
    IMAGE_WIDTH = imageBuffer[18] + (imageBuffer[19] << 8) + (imageBuffer[20] << 16) + (imageBuffer[21] << 24);
    IMAGE_HEIGHT = imageBuffer[22] + (imageBuffer[23] << 8) + (imageBuffer[24] << 16) + (imageBuffer[25] << 24);
    IMAGE_PIXELS = IMAGE_WIDTH * IMAGE_HEIGHT;

    free(imageBuffer);

    return XST_SUCCESS;
}

long open_image_sd(){ // 0 - read  1 - write

    DIR dir;
    FILINFO imageInfo;
    FRESULT imageResult;
    int imageFound = 0;

    imageResult = f_opendir(&dir, "0:/");
    
    while(!f_readdir(&dir, &imageInfo)){
        if(strstr(imageInfo.fname, ".bmp") || strstr(imageInfo.fname, ".BMP")){
            imageFound = 1;
            break;       
        }   
    }

    if(imageResult || imageFound == 0){
        xil_printf("Image not found on SD");
        return XST_FAILURE;        
    }
    else{
        imageResult = f_open(&imageFile, imageInfo.fname, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
    }

    if(imageResult){
        xil_printf("Failed opening file from sd");
        return XST_FAILURE;
    }

    get_info_about_image(&imageFile);

    if(imageResult){
        xil_printf("Failed to readWrite file from sd");
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}

int sendDataToPL(int index, int imageBufferSize){
    
    int dmaSize = 0;

    if(DMA_TRANSFER_SIZE <= imageBufferSize){
        dmaSize = DMA_TRANSFER_SIZE;        
    }    
    else{
        dmaSize = imageBufferSize;
    }

    __attribute__((aligned(32))) u32 dmaRead[dmaSize];    //what PL reads
    __attribute__((aligned(32))) u32 dmaWrite[dmaSize];   //where PL writes

    dmaConfig = XAxiDma_LookupConfig(XPAR_AXI_DMA_0_BASEADDR);

    if(dmaConfig == NULL){
        xil_printf("Failed dma lookup config");
        return XST_FAILURE;
    }

    int status = XAxiDma_CfgInitialize(&dma, dmaConfig);

    if(status == XST_FAILURE){
        xil_printf("Failed dma config");
        return XST_FAILURE;
    }

    for(int i = 0; i < dmaSize; i++){
        dmaRead[i] = imageBuffer[index * DMA_TRANSFER_SIZE + i];
        dmaWrite[i] = 0;
    }

    Xil_DCacheFlushRange((UINTPTR)dmaRead, dmaSize * 4);
    Xil_DCacheFlushRange((UINTPTR)dmaWrite, dmaSize * 4);

    status = XAxiDma_SimpleTransfer(&dma, (UINTPTR)dmaRead, dmaSize * 4, XAXIDMA_DMA_TO_DEVICE);

    if(status == XST_FAILURE){
        xil_printf("Failed dma send");
        return XST_FAILURE;
    }

    while (XAxiDma_Busy(&dma, XAXIDMA_DMA_TO_DEVICE)) {}

    usleep(1);

    status = XAxiDma_SimpleTransfer(&dma, (UINTPTR)dmaWrite, dmaSize * 4, XAXIDMA_DEVICE_TO_DMA);

    if(status == XST_FAILURE){
        xil_printf("Failed dma receive");
        return XST_FAILURE;
    }

    while (XAxiDma_Busy(&dma, XAXIDMA_DEVICE_TO_DMA)) {}

    Xil_DCacheInvalidateRange((UINTPTR)dmaWrite, dmaSize * 4);

    for(int i = 0; i < dmaSize; i++){
        //xil_printf(" (%d, %d) ", dmaRead[i], dmaWrite[i]);
        imageBuffer[index * DMA_TRANSFER_SIZE + i] = dmaWrite[i];
    }  

    return XST_SUCCESS;
}

int main(){

    if(initGpio() == XST_FAILURE){
        return 0;
    }

    if(initSdCard() == XST_FAILURE){
        return 0;
    }

    if(initFatFs() == XST_FAILURE){
        return 0;
    }

    while(1){
        
        buttonValues = XGpio_DiscreteRead(&xgpioButton, BUTTON_CHANNEL);
        
        if(buttonValues == 1){
            XGpio_DiscreteWrite(&xgpioLed, LED_CHANNEL, 0x1);  

            if(open_image_sd() == XST_SUCCESS){                

                int imageBlockCnt = IMAGE_PIXELS / HEAP_SIZE + ((IMAGE_PIXELS % HEAP_SIZE) % 2);
                int remainingPixels = IMAGE_PIXELS;
                int offset = PIXELS_OFFSET;
                FRESULT dataSentValid = XST_SUCCESS;

                for(int i = 0; i < imageBlockCnt; i++){

                    int noOfBytesReadWrite = 0;
                    int imageBufferSize = (remainingPixels > HEAP_SIZE) ? HEAP_SIZE : remainingPixels;
                    imageBuffer = (u8*)(malloc(imageBufferSize));

                    if(imageBuffer == NULL){
                        xil_printf("Failure allocating imageBuffer");
                        break;                    
                    }

                    if(f_lseek(&imageFile, offset) == FR_OK){
                        f_read(&imageFile, imageBuffer, imageBufferSize, &noOfBytesReadWrite);
                    }

                    if(noOfBytesReadWrite != imageBufferSize){
                        xil_printf("Failed reading ok");
                        break;
                    }  

                    for(int index = 0; index < imageBufferSize / DMA_TRANSFER_SIZE + ((imageBufferSize % DMA_TRANSFER_SIZE) % 2); index++){
                        dataSentValid = sendDataToPL(index, imageBufferSize);

                        if(dataSentValid != XST_SUCCESS){
                            break;
                        }                        
                    }

                    if(dataSentValid != XST_SUCCESS){
                        break;
                    }

                    noOfBytesReadWrite = 0;              

                    if(f_lseek(&imageFile, offset) == FR_OK){
                        f_write(&imageFile, imageBuffer, imageBufferSize, &noOfBytesReadWrite);
                    }

                    if(noOfBytesReadWrite != imageBufferSize){
                        xil_printf("Failed writing ok");
                        break;
                    }                 

                    offset += imageBufferSize;
                    remainingPixels -= imageBufferSize;

                    free(imageBuffer);                                                                        
                } 

                XGpio_DiscreteWrite(&xgpioLed, LED_CHANNEL, 0x0); 
                XGpio_DiscreteWrite(&xgpioLed, LED_CHANNEL, 0x2); 
                xil_printf("Success");                      
            }
        }                

        usleep(100000);
    }

    f_close(&imageFile);

    return 0;
}