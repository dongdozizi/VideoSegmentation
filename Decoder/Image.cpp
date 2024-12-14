//*****************************************************************************
//
// Image.cpp : Defines the class operations on images
//
// Author - Parag Havaldar
// Code used by students as starter code to display and modify images
//
//*****************************************************************************

#include "Image.h"
#include <vector>
#include <utility>
#include <algorithm>
#include <iostream>

// Constructor and Desctructors
MyImage::MyImage()
{
	Data = NULL;
	Width = -1;
	Height = -1;
}

MyImage::~MyImage()
{
	if (Data)
		delete Data;
}


// Copy constructor
MyImage::MyImage(const MyImage& otherImage) {
	Height = otherImage.Height;
	Width = otherImage.Width;
	Data = otherImage.Data;
	block_type = otherImage.block_type;
	MotionVec = otherImage.MotionVec;
}

// = operator overload
MyImage& MyImage::operator= (const MyImage& otherImage)
{
	Height = otherImage.Height;
	Width = otherImage.Width;
	if (Data) delete[]Data;
	Data = new unsigned char[Width * Height * 3];
	block_type = otherImage.block_type;
	MotionVec = otherImage.MotionVec;
	memcpy(Data, otherImage.Data, Width * Height * 3);
	return *this;
}

void MyImage::Initialize(int width, int height, const unsigned char* data) {
	Width = width;
	Height = height;
	int size = width * height * 3;
	delete[] Data;
	Data = new unsigned char[size];

	memcpy(Data, data, size);
	for (int i = 0; i < size; i += 3) {
		std::swap(Data[i], Data[i + 2]); // Switch R & B
	}
}


MyImage::MyImage(MyImage&& other) noexcept
	: Data(other.Data), Width(other.Width), Height(other.Height) {
	other.Data = nullptr;
	other.Width = 0;
	other.Height = 0;
}

MyImage& MyImage::operator=(MyImage&& other) noexcept {
	if (this != &other) {
		delete[] Data;
		Data = other.Data;
		Width = other.Width;
		Height = other.Height;
		block_type = other.block_type;
		MotionVec = other.MotionVec;
		other.Data = nullptr;
		other.Width = 0;
		other.Height = 0;
	}
	return *this;
}

void MyImage::dilate(std::vector<bool>& block_type, int rows, int cols, int kernel_size) {
	std::vector<bool> temp_block_type = block_type;
	std::vector<std::pair<int, int>> pair_vec;
	for (int i = -kernel_size; i <= kernel_size; i++) {
		for (int j = -kernel_size; j <= kernel_size; j++) {
			if (abs(i) + abs(j) > kernel_size) continue;
			pair_vec.emplace_back(i, j);
		}
	}
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			if (block_type[i * cols + j]) {
				for (const auto& diff : pair_vec) {
					int ni = i + diff.first;
					int nj = j + diff.second;
					if (ni >= 0 && ni < rows && nj >= 0 && nj < cols) {
						temp_block_type[ni * cols + nj] = true;
					}
				}
			}
		}
	}
	block_type = temp_block_type;
}

void MyImage::erode(std::vector<bool>& block_type, int rows, int cols, int kernel_size) {
	std::vector<bool> temp_block_type = block_type;
	std::vector<std::pair<int, int>> pair_vec;

	for (int i = -kernel_size; i <= kernel_size; i++) {
		for (int j = -kernel_size; j <= kernel_size; j++) {
			if (abs(i) + abs(j) > kernel_size) continue;
			pair_vec.emplace_back(i, j);
		}
	}

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			if (block_type[i * cols + j]) {
				bool should_erode = false;
				for (const auto& diff : pair_vec) {
					int ni = i + diff.first;
					int nj = j + diff.second;
					if (ni >= 0 && ni < rows && nj >= 0 && nj < cols) {
						if (!block_type[ni * cols + nj]) {
							should_erode = true;
							break;
						}
					}
				}
				if (should_erode) {
					temp_block_type[i * cols + j] = false;
				}
			}
		}
	}
	block_type = temp_block_type;
}

void MyImage::Segmentation() {
	if (MotionVec.size() == -1) {
		block_type.resize(MotionVec.size() * MotionVec[0].size(), 0);
		return;
	}
	block_type.resize(MotionVec.size() * MotionVec[0].size(), 0);
	int minX = 0, minY = 0, maxX = 0, maxY = 0;
	for (int i = 0; i < MotionVec.size(); i++) {
		for (int j = 0; j < MotionVec[0].size(); j++) {
			minX = min(minX, MotionVec[i][j].first);
			maxX = max(maxX, MotionVec[i][j].first);
			minY = min(minY, MotionVec[i][j].second);
			maxY = max(maxY, MotionVec[i][j].second);
		}
	}
	for (int i = 0; i < MotionVec.size(); i++) {
		for (int j = 0; j < MotionVec[0].size(); j++) {
			MotionVec[i][j].first -= minX;
			MotionVec[i][j].second -= minY;
		}
	}
	int height = maxX - minX + 1;
	int width = maxY - minY + 1;
	std::vector<std::vector<int>> count(height, std::vector<int>(width, 0));
	std::vector<std::vector<bool>> isBack(height, std::vector<bool>(width, 0));
	for (int i = 0; i < MotionVec.size(); i++) {
		for (int j = 0; j < MotionVec[0].size(); j++) {
			count[MotionVec[i][j].first][MotionVec[i][j].second]++;
		}
	}
	std::vector<std::pair<int, std::pair<int, int>>> invCount;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			invCount.emplace_back(-count[i][j], std::make_pair(i, j));
		}
	}
	std::sort(invCount.begin(), invCount.end());
	//printf("%d %d %d\n", invCount.size(),count.size(),count[0].size());
	for (int i = -2; i <= 2; i++) {
		for (int j = -2; j <= 2; j++) {
			int x = invCount[0].second.first + i;
			int y = invCount[0].second.second + j;
			if (x < 0 || x >= height || y < 0 || y >= width) continue;
			isBack[x][y] = 1;
		}
	}
	for (int i = 0; i < MotionVec.size(); i++) {
		for (int j = 0; j < MotionVec[0].size(); j++) {
			if (isBack[MotionVec[i][j].first][MotionVec[i][j].second]) {
				block_type[i * MotionVec[0].size() + j] = 1;
			}
		}
	}
	dilate(block_type, MotionVec.size(), MotionVec[0].size(), 1);
	erode(block_type, MotionVec.size(), MotionVec[0].size(), 2);
	//erode(block_type, MotionVec.size(), MotionVec[0].size(), 1);
	//erode(block_type, MotionVec.size(), MotionVec[0].size(), 1);
}

void motionVectorToColor(int dx, int dy, unsigned char& r, unsigned char& g, unsigned char& b, double max_magnitude) {
	double angle = atan2(dy, dx);
	double magnitude = sqrt(dx * dx + dy * dy);

	double M_PI = 3.14159265358979323846;
	double hue = (angle + M_PI) / (2 * M_PI);

	double saturation = min(magnitude / max_magnitude, 1.0);

	double value = 1.0;

	double c = value * saturation;
	double x = c * (1 - fabs(fmod(hue * 6, 2) - 1));
	double m = value - c;

	double r1, g1, b1;
	if (hue >= 0 && hue < 1.0 / 6) {
		r1 = c; g1 = x; b1 = 0;
	}
	else if (hue >= 1.0 / 6 && hue < 2.0 / 6) {
		r1 = x; g1 = c; b1 = 0;
	}
	else if (hue >= 2.0 / 6 && hue < 3.0 / 6) {
		r1 = 0; g1 = c; b1 = x;
	}
	else if (hue >= 3.0 / 6 && hue < 4.0 / 6) {
		r1 = 0; g1 = x; b1 = c;
	}
	else if (hue >= 4.0 / 6 && hue < 5.0 / 6) {
		r1 = x; g1 = 0; b1 = c;
	}
	else {
		r1 = c; g1 = 0; b1 = x;
	}
	r = static_cast<unsigned char>((r1 + m) * 255);
	g = static_cast<unsigned char>((g1 + m) * 255);
	b = static_cast<unsigned char>((b1 + m) * 255);
}
void MyImage::ShowMotionVec() {

	int num_block_rows = (Height + 15) / 16;
	int num_block_cols = (Width + 15) / 16;
	double max_magnitude = 0.0;
	for (int i = 0; i < num_block_rows; i++) {
		for (int j = 0; j < num_block_cols; j++) {
			int dx = MotionVec[i][j].first;
			int dy = MotionVec[i][j].second;
			double magnitude = sqrt(dx * dx + dy * dy);
			if (magnitude > max_magnitude) {
				max_magnitude = magnitude;
			}
		}
	}
	max_magnitude = 65.0;

	for (int i_block = 0; i_block < num_block_rows; i_block++) {
		for (int j_block = 0; j_block < num_block_cols; j_block++) {
			int dx = MotionVec[i_block][j_block].first;
			int dy = MotionVec[i_block][j_block].second;
			unsigned char r, g, b;
			motionVectorToColor(dx, dy, r, g, b, max_magnitude);
			int start_i = i_block * 16;
			int end_i = min((i_block + 1) * 16, Height);
			int start_j = j_block * 16;
			int end_j = min((j_block + 1) * 16, Width);
			for (int i = start_i; i < end_i; i++) {
				for (int j = start_j; j < end_j; j++) {
					int cnt = (i * Width + j) * 3;
					Data[cnt] = r;
					Data[cnt + 1] = g;
					Data[cnt + 2] = b;
				}
			}
		}
	}
}

void MyImage::ShowSegmentation() {
	//printf("%d\n", block_type.size());
	int height = Height % 16 == 0 ? Height / 16 : Height / 16 + 1;
	int width = Width % 16 == 0 ? Width / 16 : Width / 16 + 1;
	int cnt = 0;
	for (int i = 0; i < Height; i++) {
		for (int j = 0; j < Width; j++) {
			if (block_type[i / 16 * width + j / 16] == 1) {
				Data[cnt] = Data[cnt + 1] = Data[cnt + 2] = 255;
			}
			cnt += 3;
		}
	}
}

void MyImage::RGB2Y(std::vector<std::vector<int>>& img) {
	int cnt = 0;
	for (int i = 0; i < Height; i++) {
		for (int j = 0; j < Width; j++) {
			img[i][j] = 0.299 * Data[cnt] + 0.587 * Data[cnt + 1] + 0.114 * Data[cnt + 2] + 0.5;
			cnt += 3;
		}
		for (int j = Width; j < img[0].size(); j++) {
			img[i][j] = -1;
		}
	}
	for (int i = Height; i < img.size(); i++) {
		for (int j = 0; j < img[0].size(); j++) {
			img[i][j] = -1;
		}
	}
}