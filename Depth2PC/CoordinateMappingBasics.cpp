//------------------------------------------------------------------------------
// <copyright file="CoordinateMappingBasics.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <strsafe.h>
#include <math.h>
#include <limits>
#include <Wincodec.h>
#include "resource.h"
#include "CoordinateMappingBasics.h"

#include "opencv2/opencv.hpp"

#include "boost/filesystem.hpp"

#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <limits>

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

/// <summary>
/// Entry point for the application
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="hPrevInstance">always 0</param>
/// <param name="lpCmdLine">command line arguments</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
/// <returns>status</returns>
int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    CCoordinateMappingBasics application;
    application.Run(hInstance, nShowCmd);
}

/// <summary>
/// Constructor
/// </summary>
CCoordinateMappingBasics::CCoordinateMappingBasics() :
    m_hWnd(NULL),
    m_nStartTime(0),
    m_nLastCounter(0),
    m_nFramesSinceUpdate(0),
    m_fFreq(0),
    m_nNextStatusTime(0LL),
    m_bSaveScreenshot(false),
    m_pKinectSensor(NULL),
    m_pCoordinateMapper(NULL),
    m_pMultiSourceFrameReader(NULL),
    m_pDepthCoordinates(NULL),
	m_pCameraCoordianets(NULL),
    m_pD2DFactory(NULL),
    m_pDrawCoordinateMapping(NULL),
    m_pOutputRGBX(NULL),
    m_pBackgroundRGBX(NULL),
    m_pColorRGBX(NULL),
	m_nFrameSaverCounter(0)
{
    LARGE_INTEGER qpf = {0};
    if (QueryPerformanceFrequency(&qpf))
    {
        m_fFreq = double(qpf.QuadPart);
    }

    // create heap storage for composite image pixel data in RGBX format
    m_pOutputRGBX = new RGBQUAD[cColorWidth * cColorHeight];

    // create heap storage for background image pixel data in RGBX format
    m_pBackgroundRGBX = new RGBQUAD[cColorWidth * cColorHeight];

    // create heap storage for color pixel data in RGBX format
    m_pColorRGBX = new RGBQUAD[cColorWidth * cColorHeight];

    // create heap storage for the coorinate mapping from color to depth
    m_pDepthCoordinates = new DepthSpacePoint[cColorWidth * cColorHeight];

	m_pCameraCoordianets = new CameraSpacePoint[cDepthWidth * cDepthHeight];
}
  

/// <summary>
/// Destructor
/// </summary>
CCoordinateMappingBasics::~CCoordinateMappingBasics()
{
    // clean up Direct2D renderer
    if (m_pDrawCoordinateMapping)
    {
        delete m_pDrawCoordinateMapping;
        m_pDrawCoordinateMapping = NULL;
    }

    if (m_pOutputRGBX)
    {
        delete [] m_pOutputRGBX;
        m_pOutputRGBX = NULL;
    }

    if (m_pBackgroundRGBX)
    {
        delete [] m_pBackgroundRGBX;
        m_pBackgroundRGBX = NULL;
    }

    if (m_pColorRGBX)
    {
        delete [] m_pColorRGBX;
        m_pColorRGBX = NULL;
    }

    if (m_pDepthCoordinates)
    {
        delete[] m_pDepthCoordinates;
        m_pDepthCoordinates = NULL;
    }

	if (m_pCameraCoordianets)
	{
		delete[] m_pCameraCoordianets;
		m_pCameraCoordianets = NULL;
	}

    // clean up Direct2D
    SafeRelease(m_pD2DFactory);

    // done with frame reader
    SafeRelease(m_pMultiSourceFrameReader);

    // done with coordinate mapper
    SafeRelease(m_pCoordinateMapper);

    // close the Kinect Sensor
    if (m_pKinectSensor)
    {
        m_pKinectSensor->Close();
    }

    SafeRelease(m_pKinectSensor);
}

/// <summary>
/// Creates the main window and begins processing
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
int CCoordinateMappingBasics::Run(HINSTANCE hInstance, int nCmdShow)
{
    if (m_pBackgroundRGBX)
    {
        if (FAILED(LoadResourceImage(L"Background", L"Image", cColorWidth, cColorHeight, m_pBackgroundRGBX)))
        {
            const RGBQUAD c_green = {0, 255, 0}; 

            // Fill in with a background colour of green if we can't load the background image
            for (int i = 0 ; i < cColorWidth * cColorHeight ; ++i)
            {
                m_pBackgroundRGBX[i] = c_green;
            }
        }
    }

    MSG       msg = {0};
    WNDCLASS  wc;

    // Dialog custom window class
    ZeroMemory(&wc, sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbWndExtra    = DLGWINDOWEXTRA;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APP));
    wc.lpfnWndProc   = DefDlgProcW;
    wc.lpszClassName = L"CoordinateMappingBasicsAppDlgWndClass";

    if (!RegisterClassW(&wc))
    {
        return 0;
    }

    // Create main application window
    HWND hWndApp = CreateDialogParamW(
        NULL,
        MAKEINTRESOURCE(IDD_APP),
        NULL,
        (DLGPROC)CCoordinateMappingBasics::MessageRouter, 
        reinterpret_cast<LPARAM>(this));

    // Show window
    ShowWindow(hWndApp, nCmdShow);

    // Main message loop
    while (WM_QUIT != msg.message)
    {
        Update();

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // If a dialog message will be taken care of by the dialog proc
            if (hWndApp && IsDialogMessageW(hWndApp, &msg))
            {
                continue;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}

/// <summary>
/// Main processing function
/// </summary>
void CCoordinateMappingBasics::Update()
{
    if (!m_pMultiSourceFrameReader)
    {
        return;
    }

    IMultiSourceFrame* pMultiSourceFrame = NULL;
    IDepthFrame* pDepthFrame = NULL;
    IColorFrame* pColorFrame = NULL;
    IBodyIndexFrame* pBodyIndexFrame = NULL;

    HRESULT hr = m_pMultiSourceFrameReader->AcquireLatestFrame(&pMultiSourceFrame);

    if (SUCCEEDED(hr))
    {
        IDepthFrameReference* pDepthFrameReference = NULL;

        hr = pMultiSourceFrame->get_DepthFrameReference(&pDepthFrameReference);
        if (SUCCEEDED(hr))
        {
            hr = pDepthFrameReference->AcquireFrame(&pDepthFrame);
        }

        SafeRelease(pDepthFrameReference);
    }

    if (SUCCEEDED(hr))
    {
        IColorFrameReference* pColorFrameReference = NULL;

        hr = pMultiSourceFrame->get_ColorFrameReference(&pColorFrameReference);
        if (SUCCEEDED(hr))
        {
            hr = pColorFrameReference->AcquireFrame(&pColorFrame);
        }

        SafeRelease(pColorFrameReference);
    }

    if (SUCCEEDED(hr))
    {
        IBodyIndexFrameReference* pBodyIndexFrameReference = NULL;

        hr = pMultiSourceFrame->get_BodyIndexFrameReference(&pBodyIndexFrameReference);
        if (SUCCEEDED(hr))
        {
            hr = pBodyIndexFrameReference->AcquireFrame(&pBodyIndexFrame);
        }

        SafeRelease(pBodyIndexFrameReference);
    }

    if (SUCCEEDED(hr))
    {
        INT64 nDepthTime = 0;
        IFrameDescription* pDepthFrameDescription = NULL;
        int nDepthWidth = 0;
        int nDepthHeight = 0;
        UINT nDepthBufferSize = 0;
        UINT16 *pDepthBuffer = NULL;

        IFrameDescription* pColorFrameDescription = NULL;
        int nColorWidth = 0;
        int nColorHeight = 0;
        ColorImageFormat imageFormat = ColorImageFormat_None;
        UINT nColorBufferSize = 0;
        RGBQUAD *pColorBuffer = NULL;

        IFrameDescription* pBodyIndexFrameDescription = NULL;
        int nBodyIndexWidth = 0;
        int nBodyIndexHeight = 0;
        UINT nBodyIndexBufferSize = 0;
        BYTE *pBodyIndexBuffer = NULL;

        // get depth frame data

        hr = pDepthFrame->get_RelativeTime(&nDepthTime);

        if (SUCCEEDED(hr))
        {
            hr = pDepthFrame->get_FrameDescription(&pDepthFrameDescription);
        }

        if (SUCCEEDED(hr))
        {
            hr = pDepthFrameDescription->get_Width(&nDepthWidth);
        }

        if (SUCCEEDED(hr))
        {
            hr = pDepthFrameDescription->get_Height(&nDepthHeight);
        }

        if (SUCCEEDED(hr))
        {
            hr = pDepthFrame->AccessUnderlyingBuffer(&nDepthBufferSize, &pDepthBuffer);            
        }

        // get color frame data

        if (SUCCEEDED(hr))
        {
            hr = pColorFrame->get_FrameDescription(&pColorFrameDescription);
        }

        if (SUCCEEDED(hr))
        {
            hr = pColorFrameDescription->get_Width(&nColorWidth);
        }

        if (SUCCEEDED(hr))
        {
            hr = pColorFrameDescription->get_Height(&nColorHeight);
        }

        if (SUCCEEDED(hr))
        {
            hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
        }

        if (SUCCEEDED(hr))
        {
            if (imageFormat == ColorImageFormat_Bgra)
            {
                hr = pColorFrame->AccessRawUnderlyingBuffer(&nColorBufferSize, reinterpret_cast<BYTE**>(&pColorBuffer));
            }
            else if (m_pColorRGBX)
            {
                pColorBuffer = m_pColorRGBX;
                nColorBufferSize = cColorWidth * cColorHeight * sizeof(RGBQUAD);
                hr = pColorFrame->CopyConvertedFrameDataToArray(nColorBufferSize, reinterpret_cast<BYTE*>(pColorBuffer), ColorImageFormat_Bgra);
            }
            else
            {
                hr = E_FAIL;
            }
        }

        // get body index frame data

        if (SUCCEEDED(hr))
        {
            hr = pBodyIndexFrame->get_FrameDescription(&pBodyIndexFrameDescription);
        }

        if (SUCCEEDED(hr))
        {
            hr = pBodyIndexFrameDescription->get_Width(&nBodyIndexWidth);
        }

        if (SUCCEEDED(hr))
        {
            hr = pBodyIndexFrameDescription->get_Height(&nBodyIndexHeight);
        }

        if (SUCCEEDED(hr))
        {
            hr = pBodyIndexFrame->AccessUnderlyingBuffer(&nBodyIndexBufferSize, &pBodyIndexBuffer);            
        }

        if (SUCCEEDED(hr))
        {
            ProcessFrame(nDepthTime, pDepthBuffer, nDepthWidth, nDepthHeight, 
                pColorBuffer, nColorWidth, nColorHeight,
                pBodyIndexBuffer, nBodyIndexWidth, nBodyIndexHeight);
        }

        SafeRelease(pDepthFrameDescription);
        SafeRelease(pColorFrameDescription);
        SafeRelease(pBodyIndexFrameDescription);
    }

    SafeRelease(pDepthFrame);
    SafeRelease(pColorFrame);
    SafeRelease(pBodyIndexFrame);
    SafeRelease(pMultiSourceFrame);
}

/// <summary>
/// Handles window messages, passes most to the class instance to handle
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CCoordinateMappingBasics::MessageRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CCoordinateMappingBasics* pThis = NULL;
    
    if (WM_INITDIALOG == uMsg)
    {
        pThis = reinterpret_cast<CCoordinateMappingBasics*>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<CCoordinateMappingBasics*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->DlgProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

/// <summary>
/// Handle windows messages for the class instance
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CCoordinateMappingBasics::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Bind application window handle
            m_hWnd = hWnd;

            // Init Direct2D
            D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);

            // Create and initialize a new Direct2D image renderer (take a look at ImageRenderer.h)
            // We'll use this to draw the data we receive from the Kinect to the screen
            m_pDrawCoordinateMapping = new ImageRenderer(); 
            HRESULT hr = m_pDrawCoordinateMapping->Initialize(GetDlgItem(m_hWnd, IDC_VIDEOVIEW), m_pD2DFactory, cColorWidth, cColorHeight, cColorWidth * sizeof(RGBQUAD)); 
            if (FAILED(hr))
            {
                SetStatusMessage(L"Failed to initialize the Direct2D draw device.", 10000, true);
            }

            // Get and initialize the default Kinect sensor
            InitializeDefaultSensor();
        }
        break;

        // If the titlebar X is clicked, destroy app
        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            // Quit the main message pump
            PostQuitMessage(0);
            break;

        // Handle button press
        case WM_COMMAND:
            // If it was for the screenshot control and a button clicked event, save a screenshot next frame 
            if (IDC_BUTTON_SCREENSHOT == LOWORD(wParam) && BN_CLICKED == HIWORD(wParam))
            {
                m_bSaveScreenshot = true;
            }
            break;
    }

    return FALSE;
}

/// <summary>
/// Initializes the default Kinect sensor
/// </summary>
/// <returns>indicates success or failure</returns>
HRESULT CCoordinateMappingBasics::InitializeDefaultSensor()
{
    HRESULT hr;

    hr = GetDefaultKinectSensor(&m_pKinectSensor);
    if (FAILED(hr))
    {
        return hr;
    }

    if (m_pKinectSensor)
    {
        // Initialize the Kinect and get coordinate mapper and the frame reader

        if (SUCCEEDED(hr))
        {
            hr = m_pKinectSensor->get_CoordinateMapper(&m_pCoordinateMapper);
        }

        hr = m_pKinectSensor->Open();

        if (SUCCEEDED(hr))
        {
            hr = m_pKinectSensor->OpenMultiSourceFrameReader(
                FrameSourceTypes::FrameSourceTypes_Depth | FrameSourceTypes::FrameSourceTypes_Color | FrameSourceTypes::FrameSourceTypes_BodyIndex,
                &m_pMultiSourceFrameReader);
        }
    }

    if (!m_pKinectSensor || FAILED(hr))
    {
        SetStatusMessage(L"No ready Kinect found!", 10000, true);
        return E_FAIL;
    }

    return hr;
}

/// <summary>
/// Handle new depth and color data
/// <param name="nTime">timestamp of frame</param>
/// <param name="pDepthBuffer">pointer to depth frame data</param>
/// <param name="nDepthWidth">width (in pixels) of input depth image data</param>
/// <param name="nDepthHeight">height (in pixels) of input depth image data</param>
/// <param name="pColorBuffer">pointer to color frame data</param>
/// <param name="nColorWidth">width (in pixels) of input color image data</param>
/// <param name="nColorHeight">height (in pixels) of input color image data</param>
/// <param name="pBodyIndexBuffer">pointer to body index frame data</param>
/// <param name="nBodyIndexWidth">width (in pixels) of input body index data</param>
/// <param name="nBodyIndexHeight">height (in pixels) of input body index data</param>
/// </summary>
void CCoordinateMappingBasics::ProcessFrame(INT64 nTime, 
                                            const UINT16* pDepthBuffer, int nDepthWidth, int nDepthHeight, 
                                            const RGBQUAD* pColorBuffer, int nColorWidth, int nColorHeight,
                                            const BYTE* pBodyIndexBuffer, int nBodyIndexWidth, int nBodyIndexHeight)
{
    // Make sure we've received valid data
    if (m_pCoordinateMapper && m_pDepthCoordinates && m_pOutputRGBX && 
        pDepthBuffer && (nDepthWidth == cDepthWidth) && (nDepthHeight == cDepthHeight) && 
        pColorBuffer && (nColorWidth == cColorWidth) && (nColorHeight == cColorHeight) &&
        pBodyIndexBuffer && (nBodyIndexWidth == cDepthWidth) && (nBodyIndexHeight == cDepthHeight))
    {
		try {
			// check data folder
			if (!boost::filesystem::exists("data")) {
				std::cout << "cannot find data folder";
				exit(-1);
			}

			// load depth image
			std::stringstream ss;
			ss << "raw_depth_" << std::setfill('0') << std::setw(5) << m_nFrameSaverCounter;

			boost::filesystem::directory_iterator it("data");
			boost::filesystem::directory_iterator end;

			std::string depth_filename;
			while (it != end) {
				std::string current_filename = it->path().filename().string();
				if (current_filename.substr(0, 15).compare(ss.str()) == 0) {
					depth_filename = current_filename;
					break;
				}
				it++;
			}
			if (depth_filename.empty()) {
				std::cout << "cannot find depth image";
				exit(-1);
			}

			UINT16* pBuffer = new UINT16[nDepthWidth*nDepthHeight];
			memset(pBuffer, 0, nDepthWidth*nDepthHeight);
			
			cv::Mat depthImage = cv::imread(("./data/" + depth_filename).c_str(), CV_16U);
			cv::Mat depthDebug = cvCreateMat(depthImage.rows, depthImage.cols, CV_8U);
			depthImage.convertTo(depthDebug, CV_8U, 255.0/1000);
			cv::imshow("DepthImage", depthDebug);

			UINT16* pBufferIndex = pBuffer;
			for (int i = 0; i < depthImage.rows; i++) {
				for (int j = 0; j < depthImage.cols; j++) {
					*pBufferIndex++ = depthImage.at<UINT16>(i, j) << 3;
				}
			}

			UINT16* pBufferBak = pBuffer;

			HRESULT hr = m_pCoordinateMapper->MapDepthFrameToCameraSpace(nDepthWidth * nDepthHeight, (UINT16*)pBuffer, nDepthWidth * nDepthHeight, m_pCameraCoordianets);
			if (SUCCEEDED(hr))
			{
				{
					const UINT16* pDepthBufferEnd = pBuffer + (cDepthWidth * cDepthHeight);
					int pixelCounter = 0;

					// write as ply file
					{
						std::stringstream depthFilename;
						depthFilename << "data/pc_" << std::setfill('0') << std::setw(5) << m_nFrameSaverCounter << ".ply";

						std::ofstream pcOutputFile(depthFilename.str());
						std::stringstream pcOutputData;

						pcOutputData << "ply\n" << "format ascii 1.0\n"
							<< "element vertex " << nDepthHeight * nDepthWidth
							<< "\nproperty float x\n" << "property float y\n" << "property float z\n"
							<< "end_header\n";

						while (pBuffer < pDepthBufferEnd)
						{
							CameraSpacePoint singlePoint = m_pCameraCoordianets[pixelCounter];

							if (singlePoint.X < INT_MIN) {
								singlePoint.X = 0;
							}
							if (singlePoint.Y < INT_MIN) {
								singlePoint.Y = 0;
							}
							if (singlePoint.Z < INT_MIN) {
								singlePoint.Z = 0;
							}

							int x = pixelCounter % cDepthWidth;
							int y = pixelCounter / cDepthWidth;

							pcOutputData << singlePoint.X << " " << singlePoint.Y << " " << singlePoint.Z << "\n";

							++pixelCounter;
							++pBuffer;
						}

						pcOutputFile << pcOutputData.str();
						pcOutputFile.close();
					}

					pixelCounter = 0;
					pBuffer = pBufferBak;
				}

				++m_nFrameSaverCounter;
			}
		}
		catch (...) {
			exit(0);
		}
	} 
}

/// <summary>
/// Set the status bar message
/// </summary>
/// <param name="szMessage">message to display</param>
/// <param name="showTimeMsec">time in milliseconds to ignore future status messages</param>
/// <param name="bForce">force status update</param>
bool CCoordinateMappingBasics::SetStatusMessage(_In_z_ WCHAR* szMessage, DWORD nShowTimeMsec, bool bForce)
{
    INT64 now = GetTickCount64();

    if (m_hWnd && (bForce || (m_nNextStatusTime <= now)))
    {
        SetDlgItemText(m_hWnd, IDC_STATUS, szMessage);
        m_nNextStatusTime = now + nShowTimeMsec;

        return true;
    }

    return false;
}

/// <summary>
/// Get the name of the file where screenshot will be stored.
/// </summary>
/// <param name="lpszFilePath">string buffer that will receive screenshot file name.</param>
/// <param name="nFilePathSize">number of characters in lpszFilePath string buffer.</param>
/// <returns>
/// S_OK on success, otherwise failure code.
/// </returns>
HRESULT CCoordinateMappingBasics::GetScreenshotFileName(_Out_writes_z_(nFilePathSize) LPWSTR lpszFilePath, UINT nFilePathSize)
{
    WCHAR* pszKnownPath = NULL;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Pictures, 0, NULL, &pszKnownPath);

    return hr;
}

/// <summary>
/// Load an image from a resource into a buffer
/// </summary>
/// <param name="resourceName">name of image resource to load</param>
/// <param name="resourceType">type of resource to load</param>
/// <param name="nOutputWidth">width (in pixels) of scaled output bitmap</param>
/// <param name="nOutputHeight">height (in pixels) of scaled output bitmap</param>
/// <param name="pOutputBuffer">buffer that will hold the loaded image</param>
/// <returns>S_OK on success, otherwise failure code</returns>
HRESULT CCoordinateMappingBasics::LoadResourceImage(PCWSTR resourceName, PCWSTR resourceType, UINT nOutputWidth, UINT nOutputHeight, RGBQUAD* pOutputBuffer)
{
    IWICImagingFactory* pIWICFactory = NULL;
    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pSource = NULL;
    IWICStream* pStream = NULL;
    IWICFormatConverter* pConverter = NULL;
    IWICBitmapScaler* pScaler = NULL;

    HRSRC imageResHandle = NULL;
    HGLOBAL imageResDataHandle = NULL;
    void *pImageFile = NULL;
    DWORD imageFileSize = 0;

    HRESULT hrCoInit = CoInitialize(NULL);
    HRESULT hr = hrCoInit;

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)&pIWICFactory);
    }

    if (SUCCEEDED(hr))
    {
        // Locate the resource
        imageResHandle = FindResourceW(HINST_THISCOMPONENT, resourceName, resourceType);
        hr = imageResHandle ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        // Load the resource
        imageResDataHandle = LoadResource(HINST_THISCOMPONENT, imageResHandle);
        hr = imageResDataHandle ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        // Lock it to get a system memory pointer.
        pImageFile = LockResource(imageResDataHandle);
        hr = pImageFile ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        // Calculate the size.
        imageFileSize = SizeofResource(HINST_THISCOMPONENT, imageResHandle);
        hr = imageFileSize ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        // Create a WIC stream to map onto the memory.
        hr = pIWICFactory->CreateStream(&pStream);
    }

    if (SUCCEEDED(hr))
    {
        // Initialize the stream with the memory pointer and size.
        hr = pStream->InitializeFromMemory(
            reinterpret_cast<BYTE*>(pImageFile),
            imageFileSize);
    }

    if (SUCCEEDED(hr))
    {
        // Create a decoder for the stream.
        hr = pIWICFactory->CreateDecoderFromStream(
            pStream,
            NULL,
            WICDecodeMetadataCacheOnLoad,
            &pDecoder);
    }

    if (SUCCEEDED(hr))
    {
        // Create the initial frame.
        hr = pDecoder->GetFrame(0, &pSource);
    }

    if (SUCCEEDED(hr))
    {
        // Convert the image format to 32bppPBGRA
        // (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }

    if (SUCCEEDED(hr))
    {
        hr = pIWICFactory->CreateBitmapScaler(&pScaler);
    }

    if (SUCCEEDED(hr))
    {
        hr = pScaler->Initialize(
            pSource,
            nOutputWidth,
            nOutputHeight,
            WICBitmapInterpolationModeCubic
            );
    }

    if (SUCCEEDED(hr))
    {
        hr = pConverter->Initialize(
            pScaler,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.f,
            WICBitmapPaletteTypeMedianCut);
    }

    UINT width = 0;
    UINT height = 0;
    if (SUCCEEDED(hr))
    {
        hr = pConverter->GetSize(&width, &height);
    }

    // make sure the image scaled correctly so the output buffer is big enough
    if (SUCCEEDED(hr))
    {
        if ((width != nOutputWidth) || (height != nOutputHeight))
        {
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pConverter->CopyPixels(NULL, width * sizeof(RGBQUAD), nOutputWidth * nOutputHeight * sizeof(RGBQUAD), reinterpret_cast<BYTE*>(pOutputBuffer));
    }

    SafeRelease(pScaler);
    SafeRelease(pConverter);
    SafeRelease(pSource);
    SafeRelease(pDecoder);
    SafeRelease(pStream);
    SafeRelease(pIWICFactory);

    if (SUCCEEDED(hrCoInit))
    {
        CoUninitialize();
    }

    return hr;
}

