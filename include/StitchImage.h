// StitchImage.h
// Stitches multiple CPylonImage's into a single image, either vertically or horizontally.
// Also can make collages of images.
// Copyright (c) 2019 Matthew Breit - matt.breit@baslerweb.com or matt.breit@gmail.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef STITCHIMAGE_H
#define STITCHIMAGE_H

#ifndef LINUX_BUILD
#define WIN_BUILD
#endif

#ifdef WIN_BUILD
#define _CRT_SECURE_NO_WARNINGS // suppress fopen_s warnings for convinience
#endif

// Include Pylon libraries (if needed)
#include <pylon/PylonIncludes.h>

namespace StitchImage
{
	int StitchToBottom(Pylon::CPylonImage &topImage, Pylon::CPylonImage &bottomImage, Pylon::CPylonImage *stitchedImage, std::string &errorMessage);
	int StitchToRight(Pylon::CPylonImage &leftImage, Pylon::CPylonImage &rightImage, Pylon::CPylonImage *stitchedImage, std::string &errorMessage);

	class CollageMaker
	{
	private:
		Pylon::CPylonImage m_collageImage;
		Pylon::CPylonImage m_tempImage;
		Pylon::CPylonImage m_collageRow;
		std::vector<Pylon::CPylonImage> m_collageRows;
		int m_collageWidth = 0;
		int m_collageHeight = 0;
		int m_collageImagesCounter = 0;
		bool m_collageComplete = false;

	public:
		CollageMaker();
		~CollageMaker();

		int StitchToCollage(Pylon::CPylonImage &image, std::string &errorMessage);
		int GetLatestCollage(Pylon::CPylonImage *collageImage, std::string &errorMessage);
		int ResetCollage(std::string &errorMessage);
		int GetWidth();
		int GetHeight();
		void SetWidth(int numImages);
		void SetHeight(int numImages);
		bool IsCollageComplete();
	};

}

// *********************************************************************************************************
// DEFINITIONS
int StitchImage::StitchToBottom(Pylon::CPylonImage &topImage, Pylon::CPylonImage &bottomImage, Pylon::CPylonImage *stitchedImage, std::string &errorMessage)
{
	errorMessage = "ERROR: ";
	errorMessage.append(__FUNCTION__);
	errorMessage.append("(): ");

	try
	{
		Pylon::CPylonImage tempImage;
		Pylon::EPixelType tempPixelType;
		int tempWidth;

		if (topImage.GetPixelType() == Pylon::EPixelType::PixelType_Undefined)
		{
			if (bottomImage.GetPixelType() == Pylon::EPixelType::PixelType_Undefined)
			{
				errorMessage.append("Both images have undefined pixel types!");
				return 1;
			}
			else
				tempPixelType = bottomImage.GetPixelType();
		}
		else
		{
			if (topImage.GetPixelType() != bottomImage.GetPixelType())
			{
				errorMessage.append("Images must be same PixelType");
				return 1;
			}
			else
				tempPixelType = topImage.GetPixelType();
		}


		if (topImage.GetWidth() == 0)
		{
			if (bottomImage.GetWidth() == 0)
			{
				errorMessage.append("Both Images have Width = 0!");
				return 1;
			}
			else
				tempWidth = bottomImage.GetWidth();
		}
		else
		{
			if (topImage.GetWidth() != bottomImage.GetWidth())
			{
				errorMessage.append("Images must be same Width!");
				return 1;
			}
			else
				tempWidth = topImage.GetWidth();
		}

		int topImageHeight = topImage.GetHeight();
		int bottomImageHeight = bottomImage.GetHeight();
		size_t topImageSize = topImage.GetImageSize();
		size_t bottomImageSize = bottomImage.GetImageSize();
		int tempHeight = topImageHeight + bottomImageHeight;

		tempImage.Reset(tempPixelType, tempWidth, tempHeight);

		uint8_t *pTopImage = (uint8_t*)topImage.GetBuffer();
		uint8_t *pBottomImage = (uint8_t*)bottomImage.GetBuffer();
		uint8_t *pTempImage = (uint8_t*)tempImage.GetBuffer();

		memcpy(&pTempImage[0], &pTopImage[0], topImageSize);
		memcpy(&pTempImage[0 + topImageSize], &pBottomImage[0], bottomImageSize);

		stitchedImage->CopyImage(tempImage);

		return 0;

	}
	catch (GenICam::GenericException &e)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append(e.GetDescription());
		return 1;
	}
	catch (std::exception &e)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append(e.what());
		return 1;
	}
	catch (...)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append("UNKNOWN.");
		return 1;
	}
}

int StitchImage::StitchToRight(Pylon::CPylonImage &leftImage, Pylon::CPylonImage &rightImage, Pylon::CPylonImage *stitchedImage, std::string &errorMessage)
{
	errorMessage = "ERROR: ";
	errorMessage.append(__FUNCTION__);
	errorMessage.append("(): ");

	try
	{
		Pylon::CPylonImage tempImage;
		Pylon::EPixelType tempPixelType;
		int tempHeight;

		if (Pylon::IsPacked(leftImage.GetPixelType()) == true || Pylon::IsPacked(rightImage.GetPixelType()) == true)
		{
			errorMessage.append("Packed pixel formats are not supported yet");
			return 1;
		}

		if (leftImage.GetPixelType() == Pylon::EPixelType::PixelType_Undefined)
		{
			if (rightImage.GetPixelType() == Pylon::EPixelType::PixelType_Undefined)
			{
				errorMessage.append("Both images have undefined pixel types!");
				return 1;
			}
			else
				tempPixelType = rightImage.GetPixelType();
		}
		else
		{
			if (leftImage.GetPixelType() != rightImage.GetPixelType())
			{
				errorMessage.append("Images must be same PixelType");
				return 1;
			}
			else
				tempPixelType = leftImage.GetPixelType();
		}


		if (leftImage.GetHeight() == 0)
		{
			if (rightImage.GetHeight() == 0)
			{
				errorMessage.append("Both Images have Height = 0!");
				return 1;
			}
			else
				tempHeight = rightImage.GetHeight();
		}
		else
		{
			if (leftImage.GetHeight() != rightImage.GetHeight())
			{
				errorMessage.append("Images must be same Height!");
				return 1;
			}
			else
				tempHeight = leftImage.GetHeight();
		}


		int BytesPerPixel = Pylon::BitPerPixel(tempPixelType) / 8;
		int LeftImageWidth = leftImage.GetWidth();
		int RightImageWidth = rightImage.GetWidth();
		int tempWidth = LeftImageWidth + RightImageWidth;

		tempImage.Reset(tempPixelType, tempWidth, tempHeight);

		uint8_t *pLeftImage = (uint8_t*)leftImage.GetBuffer();
		uint8_t *pRightImage = (uint8_t*)rightImage.GetBuffer();
		uint8_t *pTempImage = (uint8_t*)tempImage.GetBuffer();

		for (int i = 0; i < tempHeight; i++)
		{
			memcpy(&pTempImage[(tempWidth * i * BytesPerPixel)], &pLeftImage[LeftImageWidth * i * BytesPerPixel], LeftImageWidth * BytesPerPixel);
			memcpy(&pTempImage[(tempWidth * i * BytesPerPixel) + (LeftImageWidth * BytesPerPixel)], &pRightImage[RightImageWidth * i * BytesPerPixel], RightImageWidth * BytesPerPixel);
		}

		stitchedImage->CopyImage(tempImage);

		return 0;

	}
	catch (GenICam::GenericException &e)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append(e.GetDescription());
		return 1;
	}
	catch (std::exception &e)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append(e.what());
		return 1;
	}
	catch (...)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append("UNKNOWN.");
		return 1;
	}
}

StitchImage::CollageMaker::CollageMaker()
{
	// nothing
}

StitchImage::CollageMaker::~CollageMaker()
{
	// nothing
}

int StitchImage::CollageMaker::StitchToCollage(Pylon::CPylonImage &image, std::string &errorMessage)
{
	errorMessage = "ERROR: ";
	errorMessage.append(__FUNCTION__);
	errorMessage.append("(): ");

	try
	{
		if (StitchImage::StitchToRight(m_collageRow, image, &m_collageRow, errorMessage) != 0)
			return 1;
		
		m_collageComplete = false;

		m_collageImagesCounter++;

		if (m_collageImagesCounter % m_collageWidth == 0 && m_collageImagesCounter > 0)
		{
			m_collageRows.push_back(m_collageRow);
			m_collageRow.Release();
		}

		if (m_collageImagesCounter % (m_collageWidth * m_collageHeight) == 0 && m_collageImagesCounter > 0)
		{
			for (size_t i = 0; i < m_collageRows.size(); i++)
			{
				std::string errorMessage = "";
				if (StitchImage::StitchToBottom(m_tempImage, m_collageRows[i], &m_tempImage, errorMessage) != 0)
					return 1;
			}
			m_collageImage.CopyImage(m_tempImage);
			m_tempImage.Release();
			m_collageRow.Release();
			m_collageRows.clear();
			m_collageImagesCounter = 0;
			m_collageComplete = true;
		}

		return 0;
	}
	catch (GenICam::GenericException &e)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append(e.GetDescription());
		return 1;
	}
	catch (std::exception &e)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append(e.what());
		return 1;
	}
	catch (...)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append("UNKNOWN.");
		return 1;
	}
}

int StitchImage::CollageMaker::GetLatestCollage(Pylon::CPylonImage *collageImage, std::string &errorMessage)
{
	errorMessage = "ERROR: ";
	errorMessage.append(__FUNCTION__);
	errorMessage.append("(): ");

	try
	{
		if (m_collageImage.GetImageSize() == 0)
		{
			errorMessage.append("No Collage available yet");
			return 1;
		}
		else
		{
			collageImage->CopyImage(m_collageImage);
			return 0;
		}
	}
	catch (GenICam::GenericException &e)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append(e.GetDescription());
		return 1;
	}
	catch (std::exception &e)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append(e.what());
		return 1;
	}
	catch (...)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append("UNKNOWN.");
		return 1;
	}
}

int StitchImage::CollageMaker::ResetCollage(std::string &errorMessage)
{
	errorMessage = "ERROR: ";
	errorMessage.append(__FUNCTION__);
	errorMessage.append("(): ");

	try
	{
		m_tempImage.Release();
		m_collageImage.Release();
		m_collageRow.Release();
		m_collageRows.clear();
		m_collageImagesCounter = 0;
		m_collageComplete = false;
		return 0;
	}
	catch (GenICam::GenericException &e)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append(e.GetDescription());
		return 1;
	}
	catch (std::exception &e)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append(e.what());
		return 1;
	}
	catch (...)
	{
		errorMessage.append("EXCEPTION: ");
		errorMessage.append("UNKNOWN.");
		return 1;
	}
}

int StitchImage::CollageMaker::GetWidth()
{
	return m_collageWidth;
}

int StitchImage::CollageMaker::GetHeight()
{
	return m_collageHeight;
}

void StitchImage::CollageMaker::SetWidth(int numImages)
{
	m_collageWidth = numImages;
}

void StitchImage::CollageMaker::SetHeight(int numImages)
{
	m_collageHeight = numImages;
}

bool StitchImage::CollageMaker::IsCollageComplete()
{
	return m_collageComplete;
}

// *********************************************************************************************************

#endif

// *********************************************************************************************************
// SAMPLE PROGRAM
/*
// Include files to use the PYLON API
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

#include "StitchImage.h"

// Namespace for using pylon objects.
using namespace Pylon;

#define USE_USB

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

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 27;

int main(int argc, char* argv[])
{
// The exit code of the sample application.
int exitCode = 0;

// Before using any pylon methods, the pylon runtime must be initialized.
PylonInitialize();

try
{
// Only look for cameras supported by Camera_t
CDeviceInfo info;
info.SetDeviceClass(Camera_t::DeviceClass());
info.SetSerialNumber("22479283");

// Create an instant camera object with the first found camera device that matches the specified device class.
Camera_t camera(CTlFactory::GetInstance().CreateFirstDevice(info));

// Print the model name of the camera.
cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

// Open the camera.
camera.Open();

camera.PixelFormat.SetValue(PixelFormat_Mono8);
camera.Width.SetValue(640);
camera.Height.SetValue(480);
camera.CenterX.SetValue(true);
camera.CenterY.SetValue(true);
camera.AcquisitionFrameRateEnable.SetValue(true);
camera.AcquisitionFrameRate.SetValue(1);

// This smart pointer will receive the grab result data.
GrabResultPtr_t ptrGrabResult;

// This pylon image will be the vertically stitched image
CPylonImage verticalStitchedImage;

// This pylon image will be the horizontally stitched image
CPylonImage horizontalStitchedImage;

// to make a collage
StitchImage::CollageMaker myCollageMaker;
myCollageMaker.SetWidth(3); // how many images wide do we want the collage to be
myCollageMaker.SetHeight(3); // how many images high do we want the collage to be

camera.StartGrabbing(c_countOfImagesToGrab);

// Camera.StopGrabbing() is called automatically by the RetrieveResult() method
// when c_countOfImagesToGrab images have been retrieved.
while (camera.IsGrabbing())
{
// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
// RetrieveResult calls the image event handler's OnImageGrabbed method.
camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

if (ptrGrabResult->GrabSucceeded())
{
cout << "GrabSucceeded: " << ptrGrabResult->GrabSucceeded() << endl;
cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
const uint8_t *pImageBuffer = (uint8_t *)ptrGrabResult->GetBuffer();
cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl;
cout << endl;

CPylonImage image;
image.AttachGrabResultBuffer(ptrGrabResult);

std::string errorMessage = "";

// make a tall strip of images (reusing the verticalStitchedImage as the Top image gives the effect of adding to the stitchedimage)
if (StitchImage::StitchToBottom(verticalStitchedImage, image, &verticalStitchedImage, errorMessage) == 0)
Pylon::DisplayImage(0, verticalStitchedImage);
else
cout << errorMessage << endl;

// make a wide strip of images  (reusing the horizontalStitchedImage as the Left image gives the effect of adding to the stitchedimage)
if (StitchImage::StitchToRight(horizontalStitchedImage, image, &horizontalStitchedImage, errorMessage) == 0)
Pylon::DisplayImage(1, horizontalStitchedImage);
else
cout << errorMessage << endl;

// stitch to a collage (the images will be added to the collage in top-left to bottom-right order)
if (myCollageMaker.StitchToCollage(image, errorMessage) != 0)
cout << errorMessage << endl;

if (myCollageMaker.IsCollageComplete() == true)
{
CPylonImage myCollage;
if (myCollageMaker.GetCollageImage(&myCollage, errorMessage) == 0)
Pylon::DisplayImage(2, myCollage);
else
cout << errorMessage << endl;
}
}
}
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
*/
// *********************************************************************************************************