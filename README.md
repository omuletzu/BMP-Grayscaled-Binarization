# BMP Binarization for grayscaled image

**The main problem** to be solved within this project is the acceleration of processing a BMP image stored on an SD card by binarizing the image using an FPGA. Binarization involves converting each pixel of the image into a single bit (black or white) based on a predefined threshold, which can be useful for shape recognition applications or image processing for artificial vision.

The main objectives of the project are as follows:

- Reading a BMP file from an SD card: Initially, the image must be read from a file stored on the SD card.

- Processing the image in FPGA: After reading the image, it will be sent to the FPGA for binarization, in accordance with a specified threshold.

- Writing the processed image back to the SD card: After processing, the binarized image will be saved to the SD card for later use.

- Using DMA technology: The transfer of data between PS and PL will be done via DMA, allowing for fast and efficient data transfer between the two sections.

  <img src="https://github.com/user-attachments/assets/10082521-1c77-4448-bb08-5db3d1491958" width=600> (Before)
  <img src="https://github.com/user-attachments/assets/937fe249-75f2-4db5-b2d0-ef0b3cf6cd72" width=600> (After)

**Projection and implementation**

(a) **Experimental method used**: The implementation of the project is carried out using a hybrid hardware-software architecture based on FPGA and an ARM processor integrated into the Zynq platform. The experimental method consists of using a hardware module for processing the image pixels and a software module for managing the data flow and interaction with the user.

- Hardware: The hardware part is implemented on the FPGA and is responsible for image processing (binarization). The main hardware module is a logic converter that processes the data received via DMA. This converter acts as an intermediary between an AXI4-Stream FIFO and DMA, allowing the necessary signals for the AXI4 Stream protocol to pass through it. The blocks of pixels coming from the processor will be accumulated in this FIFO memory and will be processed in turn by the converter, which transforms them either into pixel values of 0 or 255, depending on the threshold set at 128, after which they will be sent back to the processor via DMA.

- Software: The software part runs on the ARM processor within the Zynq system, responsible for initializing the hardware components, managing the SD card, and transferring data to/from the FPGA using DMA. The entire process will begin when the user presses the first button on the board, and upon completion, the first LED will light up. In case of errors, they will be displayed in the terminal.

(b) **Chosen solution and reasons for selection**: For this project, several possible solutions were considered. The final chosen solution involves using a hardware module for processing pixels, controlled by a software module that manages communication and user interface. The analyzed solutions are as follows:

- Complete software processing: The ARM processor would handle all pixel processing operations, but it cannot process large images with the required speed, and latency would increase with the file size (PL is not used for hardware acceleration).

- Complete hardware processing: The complexity of the entire process would significantly increase due to the implementation of a method for managing the SD card.

- Data transfer without DMA: Data would be sent and received through a direct protocol, but the complexity of the software code would be higher, and performance would be significantly poorer.

- Hybrid hardware-software solution: This method has only advantages, as FPGA parallelism significantly accelerates image processing, DMA provides superior performance, and the software allows flexibility in managing files and interacting with the user.

(c) **Block diagram and overall architecture**: 

- ZYNQ7 Processing System: As can be seen in the diagram, PS is represented by the ZYNQ7 Processing System. Its role is to manage reading and writing the image on the SD card, initializing GPIOs, and sending and receiving data through DMA.

- AXI GPIO: These are used to work with the buttons and LEDs on the board. The 2 GPIOs are connected to the processor via an AXI-Interconnect.

- AXI Interconnect: This is the intermediary connection between the GPIOs and the processor, as well as the connection between the processor and DMA.

- AXI4-Stream FIFO: This component serves to temporarily store (accumulate) the blocks of pixels received from the processor, and later they will be output and processed by the PL-CONVERTER.

- PL-CONVERTER: A separately realized component, whose role is to convert the pixel to a value of 0 or 255, depending on the specified threshold in it (128).

- DMA: Provides a mechanism for transferring data between hardware devices and PS memory without the direct involvement and continuous presence of the processor (thus improving performance).

- Processor System Reset: Provides a reset mechanism for the entire system.

- Concat: The role of this component is to concatenate the interrupts coming from the buttons, as they were not handled in PS (thus the Concat component is not used).

(d) **Implemented algorithms**: 

- PL-CONVERTER: The only value that changes is that of the signal m-axis-tdata-out, which receives 0 or 255 following the comparison with the threshold signal, which has the value x”00000080” (i.e., 128). The binarization algorithm is represented by this line of code.

- Initializing the SD card: The function initSdCard is responsible for initializing the SD card, using the specific library for the SD peripheral. It obtains the hardware configuration of the SD card using the function XSdPsLookupConfig with the base address XPAR-XSDPS-0-BASEADDR. Subsequently, the card is configured and initialized by calling the function XSdPsCfgInitialize, followed by XSdPs-CardInitialize. If any of these operations fail, the function displays an error message and returns the code XST-FAILURE. In the case of a successful initialization, it returns XST-SUCCESS.

- Initializing GPIOs: The function initGpio initializes and configures the GPIO peripherals used for LEDs and buttons. It uses XGpio-Initialize to configure the GPIO intended for the LEDs, using the address defined by XPAR-AXI-GPIO-LEDS-BASEADDR, and the GPIO for buttons, using the address XPAR-AXI-GPIO-BUTTONS-BASEADDR. If the initialization of any GPIO fails, the function returns an error message and the code XST-FAILURE. After initialization, the direction of the data is set: the LEDs are configured as outputs (0x0), and the buttons as inputs (0xF). In the case of correct initialization, the function returns XST-SUCCESS.

- Initializing FAT: The function initFatFs is responsible for mounting the FAT file system on the SD card using the FatFs library. It attempts to mount the SD card at address 0:/ and checks whether the operation was successfully completed using the function f-mount. If the file system mounting fails (i.e., FResult is not equal to FR-OK), the function displays an error message and returns XST-FAILURE. If the mounting is successful, the function returns XST-SUCCESS, indicating that the file system is now available for reading and writing.

- Obtaining image information: The function get-info-about-image reads essential information about a BMP-type image file stored on an SD card. In the first stage, the function allocates 54 bytes of memory to read the header of the file, which contains the basic information of the image, such as file size and image dimensions. If memory allocation or reading the file fails, the function returns an error value (XST-FAILURE). After successfully reading the header, the function extracts relevant information from it: the file size (FILE-SIZE), the pixel offset in the image (PIXELS-OFFSET), the image width (IMAGE WIDTH), the image height (IMAGE-HEIGHT), and the total number of pixels in the image (IMAGE-PIXELS). Finally, the memory allocated for reading the header is released, and the function returns XST-SUCCESS to indicate the success of the operation.

- Sending and receiving data via DMA: The function sendDataToPL is responsible for transferring data from the main processor (PS) to the programmable logic (PL) using DMA (Direct Memory Access). First, the function determines the transfer size, choosing either a predetermined DMA_TRANSFER-SIZE or the actual size of the image buffer. Then, the function configures DMA using a base configuration and initializes DMA. The input data is copied into a local buffer dmaRead, and another buffer dmaWrite is prepared to store results from PL. The data is then written to cache and sent to PL using XAxiDma-SimpleTransfer, and after the initial transfer, the function waits for the transfer to complete before proceeding. After the data has been transferred from PL to the main processor's memory buffer, the cache is invalidated and the received data is updated in the original image buffer. If any step fails, the function returns an error value (XST-FAILURE), and in the case of success, it returns XST-SUCCESS.

- Working mode with PL: The image pixels will be read in blocks of 3584 bytes (as reading more would cause an error). At each step of the algorithm, we keep track of how many blocks we have left to process in PL, the offset of the block of pixels to be read in the 3584-byte buffer, and how many pixels we still have to process. Thus, at each iteration, we read the block of pixels, send it via DMA to the PL-CONVERTER, and when we receive the block back, we write it to the same offset from where we read it, thereby modifying the original image. Furthermore, the sending via DMA is not done with the entire block at once, so the block is split into smaller blocks of 512 pixels each, which are then sent. Finally, when all the pixels have been modified, the message “SUCCESS” is displayed in the terminal, and the first LED on the board lights up. In case of failure, only the reason for the error will be displayed in the terminal.

(e) **Implementation details**: 

- PL-CONVERTER: Because this module is an intermediary between FIFO and DMA, the necessary signals for the AXI4-Stream protocol will pass through it. The signal m-axis-tdata-out represents the modified pixel.

- Libraries in PS:
  - xsdps.h: For initializing the SD card.
  - xgpio.h: For configuring and using the LEDs and buttons.
  - xilffs.h, ff.h: For managing the BMP file on the SD card (a file system is mounted on the SD using this library).
  - xaxidma.h: For data transfer via DMA.



