/*
Note: Before getting started, Basler recommends reading the Programmer's Guide topic
in the pylon C++ API documentation that gets installed with pylon.
If you are upgrading to a higher major version of pylon, Basler also
strongly recommends reading the Migration topic in the pylon C++ API documentation.

This Sample will demostrate freerunning synchronized acquisition from dual cameras.
It will also stitch the images side by side and save them to a .mp4 or .avi movie.

Author: mbreit

*/

// Include files to use the PYLON API
#include <pylon/PylonIncludes.h>

// Some Pylon tools for Displaying and Recording are only available in Windows.
// For Linux, we use alternatives, like OpenCV
#ifdef PYLON_WIN_BUILD
#include <pylon/PylonGUI.h>
#include <pylon/AviCompressionOptions.h>
#endif
#ifdef PYLON_LINUX_BUILD
#include <opencv.hpp>
#endif

// Additional Libraries
#include <thread> // for sleeping
#include <StitchImage.h> // for stitching the Left Camera image and the Right Camera image side-by-side

// Namespace for using pylon objects.
using namespace Pylon;

// Which type of camera to use for this application
#define USE_GIGE

#if defined( USE_1394 )
// Settings for using Basler IEEE 1394 cameras.
#include <pylon/1394/Basler1394InstantCamera.h>
typedef Pylon::CBasler1394InstantCamera Camera_t;
typedef Pylon::CBasler1394GrabResultPtr GrabResultPtr_t; // Or use Camera_t::GrabResultPtr_t
using namespace Basler_IIDC1394CameraParams;
#elif defined ( USE_GIGE )
// Settings for using Basler GigE cameras.
#include <pylon/gige/BaslerGigEInstantCamera.h>
typedef Pylon::CBaslerGigEInstantCamera Camera_t;
typedef Pylon::CBaslerGigEGrabResultPtr GrabResultPtr_t; // Or use Camera_t::GrabResultPtr_t
using namespace Basler_GigECameraParams;
#elif defined( USE_USB )
// Settings for using Basler USB cameras.
#include <pylon/usb/BaslerUsbInstantCamera.h>
typedef Pylon::CBaslerUsbInstantCamera Camera_t;
typedef Pylon::CBaslerUsbGrabResultPtr GrabResultPtr_t; // Or use Camera_t::GrabResultPtr_t
using namespace Basler_UsbCameraParams;
#else
#error Camera type is not specified. For example, define USE_GIGE for using GigE cameras.
#endif

// Namespace for using cout.
using namespace std;

// ******************************* Program settings **********************************
// CAMERAS TO USE
const String_t c_leftCameraSN = "22167541";
const String_t c_rightCameraSN = "22226680";
// INSTANT CAMERA: PHYSICAL CAMERA ACQUISITION SETTINGS
const int c_frameRate = 30;
const int c_width = 640;
const int c_height = 480;
const int c_exposureTime = 30000;
const String_t c_pixelFormat = "Mono8";
// INSTANT CAMERA: PHYSICAL CAMERA GIGE TRANSMISSION SETTINGS (note: these will probably need to be adjusted based on actual use case to prevent packet collisions and dropped frames (buffers incompletely grabbed))
const int c_packetSize_LeftCamera = 1500;
const int c_interpacketDelay_LeftCamera = 0;
const int c_frameTransmissionDelay_LeftCamera = 0;
const int c_packetSize_RightCamera = 1500;
const int c_interpacketDelay_RightCamera = 0;
const int c_frameTransmissionDelay_RightCamera = 0;
// INSTANT CAMERA: PYLON GRAB ENGINE SETTINGS
const int c_imagesToGrab = 1000;
const int c_maxNumBuffer = 200; // If writing a video, the more buffers the better, as writing could cause a bottleneck in the Grab Loop, leading to a Buffer Undderrun condition in the Grab Engine
const int c_maxNumQueuedBuffer = c_maxNumBuffer; // Queue up all the allocated buffers to make as many as possible ready to receive images.
// PTP SETTINGS 
const bool c_usingPTP = true;
const int c_timeToSyncPTP = 60; // PTP requires some setup time to find the synchronization between the clocks.
// VIDEO RECORDING SETTINGS
const bool c_recordingToMp4 = false;
const bool c_recordingToAvi = true;
const String_t c_mp4FileName = "Video.mp4";
const String_t c_aviFileName = "Video.avi";
const uint32_t c_imageQuality = 100;
const int c_playBackFrameRate = c_frameRate;
// ***********************************************************************************

int main(int argc, char* argv[])
{
	// The exit code of the sample application.
	int exitCode = 0;

	// Before using any pylon methods, the pylon runtime must be initialized. 
	PylonInitialize();

	try
	{
		// Our Pylon Instant Camera Objects (these contain the phyiscal camera and the pylon Grab Engine)
		Camera_t LeftCamera;
		Camera_t RightCamera;

		// We will use specific devices defined by their serial numbers.
		CDeviceInfo LeftCameraInfo;
		CDeviceInfo RightCameraInfo;
		LeftCameraInfo.SetSerialNumber(c_leftCameraSN);
		RightCameraInfo.SetSerialNumber(c_rightCameraSN);

		// Attach the instant camera objects to the appropriate hardware devices.
		LeftCamera.Attach(CTlFactory::GetInstance().CreateFirstDevice(LeftCameraInfo));
		RightCamera.Attach(CTlFactory::GetInstance().CreateFirstDevice(RightCameraInfo));

		// Print the model name of the camera.
		cout << "Left Camera  : " << LeftCamera.GetDeviceInfo().GetModelName() << " : " << LeftCamera.GetDeviceInfo().GetSerialNumber() << endl;
		cout << "Right Camera : " << RightCamera.GetDeviceInfo().GetModelName() << " : " << RightCamera.GetDeviceInfo().GetSerialNumber() << endl;


		// *********************** SETUP THE PHYSICAL CAMERAS ***********************
		// Open the camera so we can configure the hardware
		LeftCamera.Open();
		RightCamera.Open();

		// Reset cameras to defaults
		cout << "Resetting Cameras to Defaults..." << endl;
		LeftCamera.UserSetSelector.SetValue(UserSetSelectorEnums::UserSetSelector_Default);
		LeftCamera.UserSetLoad.Execute();
		RightCamera.UserSetSelector.SetValue(UserSetSelectorEnums::UserSetSelector_Default);
		RightCamera.UserSetLoad.Execute();

		// configure the cameras
		cout << "Configuring the Left Camera's hardware..." << endl;
		// image acquisition settings
		LeftCamera.ExposureTimeAbs.SetValue(c_exposureTime);
		LeftCamera.Width.SetValue(c_width);
		LeftCamera.Height.SetValue(c_height);
		LeftCamera.CenterX.SetValue(true);
		LeftCamera.CenterY.SetValue(true);
		LeftCamera.PixelFormat.FromString(c_pixelFormat);
		// Optional: Chunk features for timestamp and framecounter image metadata can be used to keep track of images
		LeftCamera.ChunkModeActive.SetValue(true);
		LeftCamera.ChunkSelector.SetValue(ChunkSelectorEnums::ChunkSelector_Timestamp);
		LeftCamera.ChunkEnable.SetValue(true);
		LeftCamera.ChunkSelector.SetValue(ChunkSelectorEnums::ChunkSelector_Framecounter);
		LeftCamera.ChunkEnable.SetValue(true);
		LeftCamera.CounterSelector.SetValue(CounterSelector_Counter2);
		LeftCamera.CounterResetSource.SetValue(Basler_GigECamera::CounterResetSourceEnums::CounterResetSource_Software);
		LeftCamera.CounterReset.Execute(); // reset the framecounter
		// Optional: If using PTP, configure those settings
		LeftCamera.SyncFreeRunTimerTriggerRateAbs.SetValue(c_frameRate);
		LeftCamera.SyncFreeRunTimerStartTimeHigh.SetValue(0);
		LeftCamera.SyncFreeRunTimerStartTimeLow.SetValue(0);
		LeftCamera.SyncFreeRunTimerUpdate.Execute();
		LeftCamera.SyncFreeRunTimerEnable.SetValue(true);
		// RECOMMENDED: If using GigE, configure the packet size, interpacket delay, and frame transmission delays to avoid packet collisions.
		LeftCamera.GevSCPSPacketSize.SetValue(c_packetSize_LeftCamera); // Packet Size
		LeftCamera.GevSCPD.SetValue(c_interpacketDelay_LeftCamera); // Interpacket Delay 
		LeftCamera.GevSCFTD.SetValue(c_frameTransmissionDelay_LeftCamera); // Frame Transmission Delay.

		cout << "Configuring the Right Camera's hardware..." << endl;
		// image acquisition settings
		RightCamera.ExposureTimeAbs.SetValue(c_exposureTime);
		RightCamera.Width.SetValue(c_width);
		RightCamera.Height.SetValue(c_height);
		RightCamera.CenterX.SetValue(true);
		RightCamera.CenterY.SetValue(true);
		RightCamera.PixelFormat.FromString(c_pixelFormat);
		// Optional: Chunk features for timestamp and framecounter image metadata can be used to keep track of images
		RightCamera.ChunkModeActive.SetValue(true);
		RightCamera.ChunkSelector.SetValue(ChunkSelectorEnums::ChunkSelector_Timestamp);
		RightCamera.ChunkEnable.SetValue(true);
		RightCamera.ChunkSelector.SetValue(ChunkSelectorEnums::ChunkSelector_Framecounter);
		RightCamera.ChunkEnable.SetValue(true);
		RightCamera.CounterSelector.SetValue(CounterSelector_Counter2);
		RightCamera.CounterResetSource.SetValue(Basler_GigECamera::CounterResetSourceEnums::CounterResetSource_Software);
		RightCamera.CounterReset.Execute(); // reset the framecounter
		// Optional: If using PTP, configure those settings
		RightCamera.SyncFreeRunTimerTriggerRateAbs.SetValue(c_frameRate);
		RightCamera.SyncFreeRunTimerStartTimeHigh.SetValue(0);
		RightCamera.SyncFreeRunTimerStartTimeLow.SetValue(0);
		RightCamera.SyncFreeRunTimerUpdate.Execute();
		RightCamera.SyncFreeRunTimerEnable.SetValue(true);
		// RECOMMENDED: If using GigE, configure the packet size, interpacket delay, and frame transmission delays to avoid packet collisions.
		RightCamera.GevSCPSPacketSize.SetValue(c_packetSize_LeftCamera); // Packet Size
		RightCamera.GevSCPD.SetValue(c_interpacketDelay_LeftCamera); // Interpacket Delay 
		RightCamera.GevSCFTD.SetValue(c_frameTransmissionDelay_LeftCamera); // Frame Transmission Delay.

		// Synchronize the camera clocks using PTP if desired
		if (c_usingPTP == true)
		{
			// configure IEEE1588 (PTP)
			cout << endl << "Enabling the IEEE1588 PTP Feature on both cameras..." << endl;

			// enable IEEE1588 (PTP)
			LeftCamera.GevIEEE1588.SetValue(true);
			RightCamera.GevIEEE1588.SetValue(true);

			// We need to wait some time to let the PTP mechanism synchronize the clocks.
			cout << "Allowing time for clock synchronization..." << endl;
			for (int i = 0; i < c_timeToSyncPTP; i++) // give them a little time to sync
			{
				cout << "Time Left: " << c_timeToSyncPTP - i << " seconds." << endl;
				LeftCamera.GevIEEE1588DataSetLatch.Execute();
				RightCamera.GevIEEE1588DataSetLatch.Execute();
				cout << "Left Camera Status  : " << std::setw(8) << LeftCamera.GevIEEE1588Status.ToString() << ". Offset from Master: " << LeftCamera.GevIEEE1588OffsetFromMaster.GetValue() << endl;
				cout << "Right Camera Status : " << std::setw(8) << RightCamera.GevIEEE1588Status.ToString() << ". Offset from Master: " << RightCamera.GevIEEE1588OffsetFromMaster.GetValue() << endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
		}
		// **************************************************************************

		// *********************** SETUP THE HOST-SIDE GRAB ENGINE AND GRAB LOOP ***********************
		// The Grab Engine receives data from the camera and fills buffers with it. It then provides Grab Results that are retrieved by the Grab Loop
		cout << "Configuring the Left Camera's Pylon Grab Engine..." << endl;
		// how many buffers should we use? Any image processing in the grab loop will take time, so use enough such that no images are dropped between RetrieveResult() calls
		LeftCamera.MaxNumBuffer.SetValue(c_maxNumBuffer);
		// allow pylon to queue up all the buffers if possible to enhance grabbing performance
		LeftCamera.MaxNumQueuedBuffer.SetValue(c_maxNumQueuedBuffer);
		cout << "Configuring the Right Camera's Pylon Grab Engine..." << endl;
		RightCamera.MaxNumBuffer.SetValue(c_maxNumBuffer);
		RightCamera.MaxNumQueuedBuffer.SetValue(c_maxNumQueuedBuffer);

		// The Grab Loop will retrieve Grab Results and uses smart pointers to access the image and information inside them.
		// The smart pointer holds information about the result (pass/fail), about the image (width/height), and is used to access the memory buffer containing the image.
		GrabResultPtr_t ptrGrabResult_Left;
		GrabResultPtr_t ptrGrabResult_Right;
		// *********************************************************************************************
		
		// *********************** SETUP THE VIDEO RECORDERS ***********************
		// MP4 RECORDING SETUP
		CVideoWriter videoWriter;
		if (c_recordingToMp4 == true)
		{
			// Check if CVideoWriter is supported and all DLLs are available.
			if (!CVideoWriter::IsSupported())
			{
				std::cout << "VideoWriter is not supported at the moment. Please install the pylon Supplementary Package for MPEG-4 which is available on the Basler website." << endl;
				LeftCamera.Close();
				RightCamera.Close();
				// Releases all pylon resources. 
				PylonTerminate();
				// Return with error code 1.
				return 1;
			}

			cout << "We will record the images on-the-fly to an .mp4 video (Display is disabled to increase performance)" << endl;

			// Map the pixelType
			CEnumParameter pixelFormat(LeftCamera.GetNodeMap(), "PixelFormat");
			CPixelTypeMapper pixelTypeMapper(&pixelFormat);
			EPixelType videoPixelType = pixelTypeMapper.GetPylonPixelTypeFromNodeValue(pixelFormat.GetIntValue());

			// Set parameters before opening the video writer.
			videoWriter.SetParameter(
				(uint32_t)c_width * 2,
				(uint32_t)c_height,
				videoPixelType,
				c_playBackFrameRate,
				c_imageQuality);

			// Open the video writer.
			videoWriter.Open(c_mp4FileName);
		}

		// AVI RECORDING SETUP
#ifdef PYLON_WIN_BUILD
		// Create an AVI writer object.
		CAviWriter aviWriter;
		if (c_recordingToAvi == true)
		{
			// Map the pixelType
			CEnumParameter pixelFormat(LeftCamera.GetNodeMap(), "PixelFormat");
			CPixelTypeMapper pixelTypeMapper(&pixelFormat);
			EPixelType videoPixelType = pixelTypeMapper.GetPylonPixelTypeFromNodeValue(pixelFormat.GetIntValue());

			// Optionally set up compression options.
			SAviCompressionOptions* pCompressionOptions = NULL;
			// Uncomment the two code lines below to enable AVI compression.
			// A dialog will be shown for selecting the codec.
			//SAviCompressionOptions compressionOptions( "MSVC", true);
			//pCompressionOptions = &compressionOptions;

			// Open the AVI writer.
			aviWriter.Open(
				c_aviFileName,
				c_playBackFrameRate,
				videoPixelType,
				(uint32_t)c_width * 2,
				(uint32_t)c_height,
				ImageOrientation_BottomUp, // Some compression codecs will not work with top down oriented images.
				pCompressionOptions);
		}
#endif
#ifdef PYLON_LINUX_BUILD
		// Create an OpenCV AVI writer object.
		// we will need to convert the image from the camera to BGR format for use in OpenCV
		CImageFormatConverter FormatConverter;
		cv::VideoWriter cvVideoCreator;
		if (c_recordingToAvi == true)
		{

			// we need to know the width and height of the image we will write 
			int stitchedWidth = c_width * 2;
			cv::Size frameSize = cv::Size(stitchedWidth, c_height);

			// there are various compression options defined by the FourCC code. Consult OpenCV docs for more info
			cvVideoCreator.open(c_aviFileName.c_str(), CV_FOURCC('M', 'J', 'P', 'G'), c_frameRate, frameSize, true); // MJPG
			
			// OpenCV uses BGR format
			FormatConverter.OutputPixelFormat = PixelType_BGR8packed;
		}
#endif
		// *************************************************************************
		
		
		// *********************** START THE GRAB ENGINE AND PHYSICAL CAMERA IMAGE ACQUISITION ***********************
		// TIP: At this point, we turn on the camera's trigger mode to prevent image acquisition while we call StartGrabbing();
		//      This is because StartGrabbing() allocates the memory buffers, configures the grab engine, and then calls AcquisitionStart() on the camera hardware.
		//      The allocation & setup can take a moment, so in some cases the cameras start Acquiring images at slightly different times (even if using PTP for clock sync).
		//      Even though the subsequent calls to turn off the trigger modes and "release" the cameras are also sequential, they are closer in time than sequential calls to StartGrabbing(). 
		LeftCamera.TriggerMode.SetValue(TriggerMode_On);
		RightCamera.TriggerMode.SetValue(TriggerMode_On);

		cout << "Starting the Pylon Grab Engines..." << endl;
		LeftCamera.StartGrabbing(c_imagesToGrab);
		RightCamera.StartGrabbing(c_imagesToGrab);

		// Pylon's Grab Engine is now ready to receive incoming images...

		cout << "Releasing the Cameras to start Free-Running Acquisition..." << endl;
		LeftCamera.TriggerMode.SetValue(TriggerMode_Off);
		RightCamera.TriggerMode.SetValue(TriggerMode_Off);

		cout << "Cameras are now Acquiring and Transmitting images to the Pylon Grab Engines..." << endl;
		// ***********************************************************************************************************

		// *********************** RUN A GRAB LOOP TO RETRIEVE GRAB RESULTS FROM GRAB ENGINE ***********************
		// Here we retrieve grabbed images and process them.
		cout << "Running the \"Grab Loop\" to Retrieve and process images from the Grab Engines..." << endl;
		cout << "We will grab " << c_imagesToGrab << " images..." << endl;

		while (LeftCamera.IsGrabbing() && RightCamera.IsGrabbing())
		{
			// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
			// RetrieveResult calls the image event handler's OnImageGrabbed method.
			LeftCamera.RetrieveResult(5000, ptrGrabResult_Left, TimeoutHandling_ThrowException);
			RightCamera.RetrieveResult(5000, ptrGrabResult_Right, TimeoutHandling_ThrowException);

			if (ptrGrabResult_Left->GrabSucceeded() && ptrGrabResult_Right->GrabSucceeded())
			{
				// We have a good image from each camera

				// Stitch the images side by side
				CPylonImage leftImage;
				CPylonImage rightImage;
				CPylonImage stitchedImage;

				leftImage.AttachGrabResultBuffer(ptrGrabResult_Left);
				rightImage.AttachGrabResultBuffer(ptrGrabResult_Right);
							
				std::string errorMessage = "";
				if (StitchImage::StitchToRight(leftImage, rightImage, &stitchedImage, errorMessage) != 0)
					cout << errorMessage << endl;

				// Either add them to the .mp4 video, add to a .avi video, or just display them
				if (c_recordingToMp4 == true)
				{
					// Write the image to the mp4
					videoWriter.Add(stitchedImage);
#ifdef PYLON_WIN_BUILD
					Pylon::DisplayImage(0, stitchedImage); // comment out to improve performance
#endif
#ifdef PYLON_LINUX_BUILD
					// There is no pylon image display in linux, so just cout the framecounters and timestamps of the images
					// (or use opencv to display them, as below)
					int64_t frameCounter_left = ptrGrabResult_Left->ChunkFramecounter.GetValue();
					int64_t frameCounter_right = ptrGrabResult_Right->ChunkFramecounter.GetValue();
					int64_t timestamp_left = ptrGrabResult_Left->ChunkTimestamp.GetValue();
					int64_t timestamp_right = ptrGrabResult_Right->ChunkTimestamp.GetValue();

					cout << "Left Camera  : FrameCounter: " << frameCounter_left << " TimeStamp: " << timestamp_left << endl;
					cout << "Right Camera : FrameCounter: " << frameCounter_right << " TimeStamp: " << timestamp_right << endl;
#endif
				}
				else if (c_recordingToAvi == true)
				{
#ifdef PYLON_WIN_BUILD
					// Write the image to the AVI
					aviWriter.Add(stitchedImage);
					// Display the image (comment out to improve performance)
					Pylon::DisplayImage(0, stitchedImage);
#endif
#ifdef PYLON_LINUX_BUILD
					// OpenCV needs BGR format, so use pylon to convert the image.
					CPylonImage ocvImage;
					FormatConverter.Convert(ocvImage, stitchedImage);
					// create an OpenCV Mat from the Pylon Image
					cv::Mat cv_img = cv::Mat(ocvImage.GetHeight(), ocvImage.GetWidth(), CV_8UC3, (uint8_t*)ocvImage.GetBuffer());
					// Write the image to the AVI
					cvVideoCreator.write(cv_img); 
					// Display the image (comment out to improve performance)
					cv::imshow("window", cv_img);
					cv::waitKey(1); // opencv needs this for display
#endif
				}
				else
				{
#ifdef PYLON_WIN_BUILD
					// Display the image
					Pylon::DisplayImage(0, stitchedImage);
#endif
#ifdef PYLON_LINUX_BUILD
					// There is no pylon image display in linux, so just cout the framecounters and timestamps of the images
					// (or use something like opencv to display them, as above)
					int64_t frameCounter_left = ptrGrabResult_Left->ChunkFramecounter.GetValue();
					int64_t frameCounter_right = ptrGrabResult_Right->ChunkFramecounter.GetValue();
					int64_t timestamp_left = ptrGrabResult_Left->ChunkTimestamp.GetValue();
					int64_t timestamp_right = ptrGrabResult_Right->ChunkTimestamp.GetValue();

					cout << "Left Camera  : FrameCounter: " << frameCounter_left << " TimeStamp: " << timestamp_left << endl;
					cout << "Right Camera : FrameCounter: " << frameCounter_right << " TimeStamp: " << timestamp_right << endl;
#endif
				}
			}
			else
			{
				cout << "Grab Failed: " << endl;
				if (ptrGrabResult_Left->GrabSucceeded() == false)
				{
					cout << "Left Camera: " << "(" << ptrGrabResult_Left->GetErrorCode() << ") " << ptrGrabResult_Left->GetErrorDescription() << endl;
				}
				if (ptrGrabResult_Right->GrabSucceeded() == false)
				{
					cout << "Right Camera: " << "(" << ptrGrabResult_Right->GetErrorCode() << ") " << ptrGrabResult_Right->GetErrorDescription() << endl;
				}
			}

			// Since image processing takes time, we could build up a backlog of images in the Grab Engines if processing framerate is slower than camera framerate.
			// This is one way to check if we've run out of buffers in the Grab Engines.
			// (if the input queue is empty and the output queue is empty, then grabbing is complete. If the input queue is empty and the output queue has images, we have an underrun)
			if ((LeftCamera.NumQueuedBuffers.GetValue() == 0 || RightCamera.NumQueuedBuffers.GetValue() == 0) && (LeftCamera.NumReadyBuffers.GetValue() != 0 && RightCamera.NumReadyBuffers.GetValue() != 0))
					cout << "Warning! Buffer underrun detected. Increase MaxNumBuffer or make the image processing run faster." << endl;
		}
		cout << "Grabbing Complete." << endl;
#ifdef PYLON_LINUX_BUILD
		cvVideoCreator.release();
#endif
		// *********************************************************************************************************
	}
	catch (const GenericException &e)
	{
		// Error handling.
		cerr << "An exception occurred." << endl
			<< e.GetDescription() << endl;
		exitCode = 1;
	}
	catch (std::exception &e)
	{
		// Error handling.
		cerr << "An exception occurred." << endl
			<< e.what() << endl;
		exitCode = 1;
	}

	// Comment the following two lines to disable waiting on exit.
	cerr << endl << "Press Enter to exit." << endl;
	while (cin.get() != '\n');

	// Releases all pylon resources. 
	PylonTerminate();

	return exitCode;
}

