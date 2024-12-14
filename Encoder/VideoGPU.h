// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <vector>
#include <math.h>
#include <string>
#include "Image.h"

// Class structure of Image 
// Use to encapsulate an RGB image
class MyVideoGPU
{

private:
	int		Width;									// Width of Video
	int		Height;									// Height of Video
	int		Length;									// Length of Video
	int		N1;
	int		N2;
	std::string	VideoPath;							// Video location
	std::vector<MyImage> Data;						// Video Data
	float IDCTCoeffU[8][8];
	float IDCTCoeffV[8][8];
	int		K = 16;
	double	M_PI = acos(-1);

public:
	// Constructor
	MyVideoGPU();
	// Destructor
	~MyVideoGPU();

	// Reader & Writer functions
	void	setHeight(const int h) { Height = h; }
	void	setWidth(const int w) { Width = w; }
	void	setN1(const int n1) { N1 = n1; }
	void	setN2(const int n2) { N2 = n2; }
	void	setVideoPath(std::string path) { VideoPath = path; }
	int		getWidth() { return Width; }
	int		getHeight() { return Height; }
	int		getLength() { return Length; }
	MyImage getFrame(int i) { return Data[i]; }
	std::vector<MyImage> getVideoData() { return Data; }
	std::string getVideoPath() { return VideoPath; }

	bool	ReadVideo();
	void	CalcMotion();
	void	MAD(std::vector<float>& lastFrame, std::vector<float>& curFrame, std::vector<std::vector<std::pair<int, int>>>& motionVec, int k);
	void	FME(std::vector<std::vector<int>>& lastFrame, std::vector<std::vector<int>>& curFrame, std::vector<std::vector<std::pair<int, int>>>& motionVec, int k);

	void	DCT(std::vector<int>& tmpImg, std::vector<int>& iBlockType);
	void	WriteInt(std::string& buffer, int x);
	void	writeCoeffToCmp(const std::string& filename);
	void	idct(std::vector<unsigned char>& block, std::vector<int>& coeff, std::vector<std::vector<float>>& temp, int n);
	int		ReadInt(std::string& buffer, int& pos);
	void	readCmp(const std::string& filename);
};
