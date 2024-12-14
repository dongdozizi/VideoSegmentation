//*****************************************************************************
//
// Image.h : Defines the class operations on images
//
// Author - Parag Havaldar
// Main Image class structure 
//
//*****************************************************************************

#ifndef IMAGE_DISPLAY
#define IMAGE_DISPLAY

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"
#include "afxwin.h"

// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <vector>
#include <utility>

// Class structure of Image 
// Use to encapsulate an RGB image
class MyImage
{

private:
	int		K = 16;
	int		Width;					// Width of Image
	int		Height;					// Height of Image
	unsigned char* Data;			// RGB data of the image
	std::vector<bool> block_type;	// block_type
	std::vector<std::vector<std::pair<int, int>>> MotionVec; // Motion Vector

public:
	// Constructor
	MyImage();
	// Copy Constructor
	MyImage(const MyImage& otherImage);
	// Destructor
	~MyImage();

	// operator overload
	MyImage& operator= (const MyImage& otherImage);

	// Move Constructor
	MyImage(MyImage&& other) noexcept;

	// operator overload
	MyImage& operator=(MyImage&& other) noexcept;

	// Reader & Writer functions
	void	setWidth(const int w) { Width = w; };
	void	setHeight(const int h) { Height = h; };
	void	setImageData(const unsigned char* img) { Data = (unsigned char*)img; };
	int		getWidth() { return Width; };
	int		getHeight() { return Height; };
	std::vector<std::vector<std::pair<int, int>>> getMotionVec() { return MotionVec; };
	std::vector<bool> getBlockType() { return block_type; };
	void	setMotionVec(std::vector<std::vector<std::pair<int, int>>> motionVec) { MotionVec = motionVec; };
	unsigned char* getImageData() { return Data; };


	// Input Output operations
	void    Initialize(int width, int height, const unsigned char* data);
	void	Segmentation();
	void	Segmentation2(int threshold=10);
	void	ShowSegmentation();
	void	ShowMotionVec();
	void	ShowMotionPlot();
	void	erode(std::vector<bool>& block_type, int rows, int cols, int kernel_size);
	void	dilate(std::vector<bool>& block_type, int rows, int cols, int kernel_size);
	void	RGB2Y(std::vector<std::vector<float>>& img);
	void	RGB2Y(std::vector<float>& img);
};

#endif //IMAGE_DISPLAY
