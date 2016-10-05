#include "opencv2\opencv.hpp"

#undef min

#include <Windows.h>

#include <iomanip>
#include <vector>
#include <string>
#include <iostream>

static const int        cDepthWidth  = 512;
static const int        cDepthHeight = 424;
static const int        cColorWidth  = 1920;
static const int        cColorHeight = 1080;

std::vector<std::string> ListDirectoryContents(const char *sDir, char *filename_pattern)
{
	std::vector<std::string> allfiles;

	WIN32_FIND_DATA fdFile;
	HANDLE hFind = NULL;

	char sPath[2048];

	//Specify a file mask. *.* = We want everything!
	sprintf(sPath, "%s\\*.*", sDir);

	if((hFind = FindFirstFile(sPath, &fdFile)) == INVALID_HANDLE_VALUE)
	{
		printf("Path not found: [%s]\n", sDir);
		//return false;
		return allfiles;
	}

	do
	{
		//Find first file will always return "."
		//    and ".." as the first two directories.
		if(strcmp(fdFile.cFileName, ".") != 0
			&& strcmp(fdFile.cFileName, "..") != 0)
		{
			//Build up our file path using the passed in
			//  [sDir] and the file/foldername we just found:
			sprintf(sPath, "%s\\%s", sDir, fdFile.cFileName);

			////Is the entity a File or Folder?
			//if(fdFile.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY)
			//{
			//    printf("Directory: %s\n", sPath);
			//    ListDirectoryContents(sPath); //Recursion, I love it!
			//}
			//else{
			//    printf("File: %s\n", sPath);
			//}
			if (strstr(sPath, filename_pattern))
				allfiles.push_back(sPath);
		}
	}
	while(FindNextFile(hFind, &fdFile)); //Find the next file.

	FindClose(hFind); //Always, Always, clean things up!

	return allfiles;
}

int main(void)
{
	std::string datafolder = "./data";

	std::vector<std::string> rgb_filenames = ListDirectoryContents(datafolder.c_str(), "raw_rgb");
	std::vector<std::string> depth_filenames = ListDirectoryContents(datafolder.c_str(), "raw_depth");
	std::vector<std::string> mapper_filenames = ListDirectoryContents(datafolder.c_str(), "depth2rgb_mapper");

	int file_number = min((int)rgb_filenames.size(), min((int)depth_filenames.size(), (int)mapper_filenames.size()));
	rgb_filenames.resize(file_number);
	depth_filenames.resize(file_number);
	mapper_filenames.resize(file_number);

	cv::namedWindow("Original Color Image", CV_WINDOW_AUTOSIZE);
	cv::moveWindow("Original Color Image", 0, 500);
	cv::namedWindow("Original Depth Image", CV_WINDOW_AUTOSIZE);
	cv::moveWindow("Original Depth Image", 0, 0);
	cv::namedWindow("Aligned Color Image", CV_WINDOW_AUTOSIZE);
	cv::moveWindow("Aligned Color Image", 500, 0);

	for (int i = 0; i < file_number; i++) {

		// load rgb image
		cv::Mat cvColorOriImage(cColorHeight, cColorWidth, CV_8UC3, 3);
		cvColorOriImage = cv::imread(rgb_filenames[i], CV_LOAD_IMAGE_COLOR);

		{
			cv::Mat cvColorShowImage;
			cv::resize(cvColorOriImage, cvColorShowImage, cv::Size(), 0.25, 0.25);
			cv::imshow("Original Color Image", cvColorShowImage);
		}

		// load depth image
		IplImage* cvDepthImage = cvCreateImage(cvSize(cDepthWidth, cDepthHeight), IPL_DEPTH_16U, 1);
		cvDepthImage = cvLoadImage(depth_filenames[i].c_str(), CV_LOAD_IMAGE_ANYDEPTH);

		{
			cv::Mat cvDepthDispImage(cvDepthImage);
			//double min, max;
			//cv::minMaxIdx(cvDepthDispImage, &min, &max);
			cv::Mat adjMap;
			cv::convertScaleAbs(cvDepthDispImage, adjMap, 255 / 1000.0);
			cv::imshow("Original Depth Image", adjMap);
		}

		// load mapper file
		IplImage* cvMapperImage = cvCreateImage(cvSize(cDepthWidth, cDepthHeight), IPL_DEPTH_16U, 3);
		cvMapperImage = cvLoadImage(mapper_filenames[i].c_str(), CV_LOAD_IMAGE_UNCHANGED);

		// ailgned rgb image
		cv::Mat cvColorMapperImage(cDepthHeight, cDepthWidth, CV_8UC3, 3);
		for (int iRow = 0; iRow < cDepthHeight; iRow++) {
			for (int iCol = 0; iCol < cDepthWidth; iCol++) {
				int colorY = CV_IMAGE_ELEM(cvMapperImage, INT16, iRow, iCol*3);
				int colorX = CV_IMAGE_ELEM(cvMapperImage, INT16, iRow, iCol*3+1);

				if (colorX > 0 && colorX < 1920 && colorY > 0 && colorY < 1080) {
					cvColorMapperImage.at<cv::Vec3b>(iRow, iCol)[0] = (unsigned char)(cvColorOriImage.at<cv::Vec3b>(colorY, colorX)[0]);
					cvColorMapperImage.at<cv::Vec3b>(iRow, iCol)[1] = (unsigned char)(cvColorOriImage.at<cv::Vec3b>(colorY, colorX)[1]);
					cvColorMapperImage.at<cv::Vec3b>(iRow, iCol)[2] = (unsigned char)(cvColorOriImage.at<cv::Vec3b>(colorY, colorX)[2]);
				}
			}
		}

		cv::imshow("Aligned Color Image", cvColorMapperImage);

		// write the aligned rgb image
		{
			std::stringstream colorImageFilename;
			colorImageFilename << "data/aligned_rgb_" << std::setfill('0') << std::setw(5) << i << ".png";
			cv::imwrite(colorImageFilename.str().c_str(), cvColorMapperImage);
		}

		cv::waitKey(33);
	}

	return 0;
}