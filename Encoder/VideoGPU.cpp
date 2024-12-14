#include "VideoGPU.h"
#include "Image.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <time.h>
#include <algorithm>
#include <chrono>
#include <amp.h> 
using namespace concurrency;
MyVideoGPU::MyVideoGPU() {
	Data.clear();
	Width = -1;
	Height = -1;
	Length = 0;
	N1 = -1;
	N2 = -1;
	VideoPath.clear();
}

MyVideoGPU::~MyVideoGPU() {
	Data.clear();
}

// MyImage::ReadImage
// Function to read the image given a path
bool MyVideoGPU::ReadVideo()
{
	// Verify VideoPath
	if (VideoPath[0] == 0 || Width < 0 || Height < 0)
	{
		fprintf(stderr, "Video or Video properties not defined");
		fprintf(stderr, "Usage is `Image.exe Imagefile w h`");
		return false;
	}

	// Create a valid output file pointer
	FILE* IN_FILE;
	IN_FILE = fopen(VideoPath.c_str(), "rb");
	if (!IN_FILE) {
		fprintf(stderr, "Error: Cannot open video file.\n");
		return false;
	}

	std::cout << "Start read\n";

	// Calculate total size and read data into memory
	int frameSize = Width * Height * 3; // RGB channels
	fseek(IN_FILE, 0, SEEK_END);
	long fileSize = ftell(IN_FILE);
	fseek(IN_FILE, 0, SEEK_SET);
	std::cout << "End read\n";

	std::cout << Length << " " << fileSize << "\n";

	if (fileSize % frameSize != 0) {
		fprintf(stderr, "Error: File size is not a multiple of frame size.\n");
		fclose(IN_FILE);
		return false;
	}

	Length = fileSize / frameSize;
	std::cout << Length << " " << fileSize << "\n";
	unsigned char* buffer = new unsigned char[frameSize];
	//fread(buffer, 1, fileSize, IN_FILE);

	// Construct images from memory buffer
	Data.clear();
	auto start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < Length; ++i) {
		if (fread(buffer, 1, frameSize, IN_FILE) != frameSize) {
			fprintf(stderr, "Error: Failed to read frame %d.\n", i);
			break;
		}

		MyImage image;
		image.Initialize(Width, Height, buffer);
		Data.push_back(std::move(image));
		// Show progress
		int progress = static_cast<int>((static_cast<double>(i + 1) / Length) * 50);
		std::cout << "\rRead Video\t[";
		for (int j = 0; j < 50; ++j) {
			if (j < progress) std::cout << "=";
			else std::cout << " ";
		}
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << "] " << ((i + 1) * 100 / Length) << "% " << i + 1 << "/" << Length;
		std::cout << " | " << duration.count() / 1000 << "." << duration.count() % 1000 << "s (" << duration.count() / (i + 1) << "ms/F)";
		std::cout.flush();

		if (i == Length - 1) {
			std::cout << std::endl;
		}
	}
	// Cleanup
	delete[] buffer;
	fclose(IN_FILE);

	return true;
}

void MyVideoGPU::MAD(std::vector<float>& lastFrame, std::vector<float>& curFrame, std::vector<std::vector<std::pair<int, int>>>& motionVec, int k) {
	// Searching from center
	std::vector<std::pair<int, int>> offsets;
	for (int i = -k; i <= k; i++) {
		for (int j = -k; j <= k; j++) {
			offsets.emplace_back(i, j);
		}
	}
	std::sort(offsets.begin(), offsets.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
		return std::abs(a.first) + std::abs(a.second) < std::abs(b.first) + std::abs(b.second);
		});
	std::vector<int> offsetX;
	std::vector<int> offsetY;
	for (int i = 0; i < offsets.size(); i++) {
		offsetX.push_back(offsets[i].first);
		offsetY.push_back(offsets[i].second);
	}
	int Height = this->Height;
	int Width = this->Width;
	int height = (Height + 15) / 16;
	int width = (Width + 15) / 16;
	std::vector<int> motionX(height * width);
	std::vector<int> motionY(height * width);
	array_view<const float, 2> avLast(Height, Width, lastFrame);
	array_view<const float, 2> avCur(Height, Width, curFrame);
	array_view<int, 1> avOffsetX((int)offsetX.size(), offsetX);
	array_view<int, 1> avOffsetY((int)offsetY.size(), offsetY);
	array_view<int, 2> avMotionX(height, width, motionX);
	array_view<int, 2> avMotionY(height, width, motionY);
	avMotionX.discard_data();
	avMotionY.discard_data();
	parallel_for_each(
		extent<2>(height, width),
		[=](index<2> idx) restrict(amp) {
			int mb_i = idx[0];
			int mb_j = idx[1];

			int start_i = mb_i * 16;
			int start_j = mb_j * 16;

			int h = (start_i + 16 > Height) ? (Height - start_i) : 16;
			int w = (start_j + 16 > Width) ? (Width - start_j) : 16;

			float blockCur[16][16];
			float blockLast[48][48];

			for (int li = 0; li < h; li++) {
				for (int lj = 0; lj < w; lj++) {
					blockCur[li][lj] = avCur(start_i + li, start_j + lj);
				}
			}

			for (int rr = 0; rr < h + 2 * k; rr++) {
				for (int cc = 0; cc < w + 2 * k; cc++) {
					int global_i = start_i - k + rr;
					int global_j = start_j - k + cc;

					if (global_i >= 0 && global_i < Height && global_j >= 0 && global_j < Width) {
						blockLast[rr][cc] = avLast(global_i, global_j);
					}
					else {
						blockLast[rr][cc] = 1e10f;
					}
				}
			}

			float bestSum = 1e9f;
			int bestP = 0;
			int bestQ = 0;
			int numOffsets = avOffsetX.extent[0];

			for (int o = 0; o < numOffsets; o++) {
				int p = avOffsetX[o];
				int q = avOffsetY[o];

				float tmp = 0;
				bool exceed = false;

				for (int rr = 0; rr < h && !exceed; rr++) {
					for (int cc = 0; cc < w; cc++) {
						float diff = blockCur[rr][cc] - blockLast[rr + p + k][cc + q + k];
						tmp += diff > 0 ? diff : -diff;
						if (tmp > bestSum) {
							exceed = true;
							break;
						}
					}
				}

				if (tmp < bestSum) {
					bestSum = tmp;
					bestP = p;
					bestQ = q;
				}
			}

			avMotionX[mb_i][mb_j] = bestP;
			avMotionY[mb_i][mb_j] = bestQ;
		}
	);

	avMotionX.synchronize();
	avMotionY.synchronize();

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			motionVec[i][j] = { motionX[i * width + j], motionY[i * width + j] };
		}
	}
}

void MyVideoGPU::FME(std::vector<std::vector<int>>& lastFrame, std::vector<std::vector<int>>& curFrame, std::vector<std::vector<std::pair<int, int>>>& motionVec, int k) {

}

void MyVideoGPU::CalcMotion() {

	int height = (Height + 15) / 16;
	int width = (Width + 15) / 16;

	std::vector<float> curFrame(Height * Width), lastFrame(Height * Width);
	std::vector<std::vector<std::pair<int, int>>> motionVec(height, std::vector<std::pair<int, int>>(width, std::make_pair(0, 0)));
	Data[0].setMotionVec(motionVec);
	Data[0].RGB2Y(curFrame);

	std::ofstream outFile("motion_vectors.txt");
	if (!outFile) {
		std::cerr << "Error opening file for writing." << std::endl;
		return;
	}

	// Calculating MAD
	auto start = std::chrono::high_resolution_clock::now();
	for (int i = 1; i < Length; i++) {
		swap(curFrame, lastFrame);
		Data[i].RGB2Y(curFrame);
		MAD(lastFrame, curFrame, motionVec, K);
		// FME(lastFrame, curFrame, motionVec, k);
		Data[i].setMotionVec(motionVec);

		for (int i = 0; i < motionVec.size(); i++) {
			for (int j = 0; j < motionVec[i].size(); j++)
				outFile << motionVec[i][j].first << " " << motionVec[i][j].second << " ";
		}
		outFile << "\n";

		// Show progress
		int progress = static_cast<int>((static_cast<double>(i + 1) / Length) * 50);
		std::cout << "\rCalc Motion Vec\t[";
		for (int j = 0; j < 50; ++j) {
			if (j < progress) std::cout << "=";
			else std::cout << " ";
		}
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << "] " << ((i + 1) * 100 / Length) << "% " << i + 1 << "/" << Length;
		std::cout << " | " << duration.count() / 1000 << "." << duration.count() % 1000 << "s (" << duration.count() / (i + 1) << "ms/F)";
		std::cout.flush();

		if (i == Length - 1) {
			std::cout << std::endl;
		}
	}

	//Segmentation
	start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < Length; i++) {
		Data[i].Segmentation();

		// Show progress
		int progress = static_cast<int>((static_cast<double>(i + 1) / Length) * 50);
		std::cout << "\rSegmentation\t[";
		for (int j = 0; j < 50; ++j) {
			if (j < progress) std::cout << "=";
			else std::cout << " ";
		}
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << "] " << ((i + 1) * 100 / Length) << "% " << i + 1 << "/" << Length;
		std::cout << " | " << duration.count() / 1000 << "." << duration.count() % 1000 << "s (" << duration.count() / (i + 1) << "ms/F)";
		std::cout.flush();
		if (i == Length - 1) {
			std::cout << std::endl;
		}
	}
	outFile.close();
}

void MyVideoGPU::DCT(std::vector<int>& tmpImg, std::vector<int>& iBlockType) {
	float DCTCoeffX[8][8];
	float DCTCoeffY[8][8];

	int N1 = this->N1, N2 = this->N2;
	for (int v = 0; v < 8; v++) {
		float cv = (v == 0) ? (1. / sqrt(2)) : 1;
		for (int y = 0; y < 8; y++) {
			DCTCoeffY[v][y] = 0.5 * cv * cos((2 * y + 1) * v * M_PI / 16.0);
		}
	}
	for (int u = 0; u < 8; u++) {
		float cu = (u == 0) ? (1. / sqrt(2)) : 1;
		for (int x = 0; x < 8; x++) {
			DCTCoeffX[u][x] = 0.5 * cu * cos((2 * x + 1) * u * M_PI / 16.0);
		}
	}

	int bWidth = (Width + 7) / 8 * 8;
	int bHeight = (Height + 7) / 8 * 8;
	
	int totalPixels = bWidth * bHeight;
	array_view<const int, 1> av_iBlockType(iBlockType.size(), iBlockType);
	array_view<int, 1> av_tmpImg(3 * totalPixels, tmpImg);
//	av_tmpImg.discard_data();
	array_view<const float, 2> av_DCTCoeffX(8, 8, &DCTCoeffX[0][0]);
	array_view<const float, 2> av_DCTCoeffY(8, 8, &DCTCoeffY[0][0]);
	extent<3> comp_ext(3, bHeight / 8, bWidth / 8);
	parallel_for_each(comp_ext, [=](index<3> idx) restrict(amp) {
		int k = idx[0];
		int h = idx[1];
		int w = idx[2]; 

		int bWidth_half = bWidth / 16;
		int h2 = h / 2;
		int w2 = w / 2;
		int idxBT = w2 + h2 * bWidth_half;
		int blockType = av_iBlockType[idxBT];
		float n = (blockType == 0) ? (1 << N1) : (1 << N2);

		float temp[8][8];

		for (int x = 0; x < 8; x++) {
			for (int vv = 0; vv < 8; vv++) {
				float sumY = 0.0f;
				for (int y = 0; y < 8; y++) {
					sumY += av_DCTCoeffY(vv, y) * av_tmpImg[((x + h * 8) * bWidth + y + w * 8) * 3 + k];
				}
				temp[x][vv] = sumY;
			}
		}

		for (int uu = 0; uu < 8; uu++) {
			for (int vv = 0; vv < 8; vv++) {
				float sumX = 0.0f;
				for (int x = 0; x < 8; x++) {
					sumX += av_DCTCoeffX(uu, x) * temp[x][vv];
				}
				sumX = sumX / n;
				av_tmpImg[((uu + h * 8) * bWidth + w * 8 + vv) * 3 + k] = sumX > 0.0f ? sumX + 0.5f : sumX - 0.5f;
			}
		}
		});
	av_tmpImg.synchronize();
}
void MyVideoGPU::WriteInt(std::string& buffer, int x) {
	if (x < 0) {
		x = -x;
		buffer.push_back('-');
	}
	int startPos = buffer.size();
	int temp = x;
	do {
		buffer.push_back('0' + (temp % 10));
		temp /= 10;
	} while (temp > 0);
	int endPos = buffer.size() - 1;
	while (startPos < endPos) {
		std::swap(buffer[startPos++], buffer[endPos--]);
	}
}

void MyVideoGPU::writeCoeffToCmp(const std::string& filename) {
	const char* fileName = filename.c_str();
	FILE* file = fopen(fileName, "w");
	fprintf(file, "%d %d %d\n", N1, N2, Length);
	int bWidth = (Width + 7) / 8;
	int bHeight = (Height + 7) / 8;
	int totalPixels = bWidth * 8 * bHeight * 8;

	std::vector<std::string> buffer(bHeight);
	std::vector<int> tmpImg(3 * bHeight * 8 * bWidth * 8);
	std::vector<int> iBlockType;
//	std::vector<std::vector<std::vector<int>>> tmpImg(3, std::vector<std::vector<int>>(bHeight * 8, std::vector<int>(bWidth * 8)));

	auto start = std::chrono::high_resolution_clock::now();
	for (int f = 0; f < Length; f++) {
		unsigned char* iData = Data[f].getImageData();
		for (int k = 0; k < 3; k++) {
			for (int i = 0; i < bHeight * 8; i++) {
				for (int j = 0; j < bWidth * 8; j++) {
					if (i < Height && j < Width) {
						tmpImg[(i * bWidth * 8 + j) * 3 + k] = iData[(i * Width + j) * 3 + k];
					}
					else {
						tmpImg[(i * bWidth * 8 + j) * 3 + k] = 0;
					}
				}
			}
		}
		std::vector<bool> bt = Data[f].getBlockType();
		iBlockType.resize(Data[f].getBlockType().size());
		for (int i = 0; i < bt.size(); i++) {
			iBlockType[i] = bt[i];
		}
		DCT(tmpImg, iBlockType);
#pragma omp parallel for
		for (int h = 0; h < bHeight; h++) {
			for (int w = 0; w < bWidth; w++) {
				int idxBT = w / 2 + (h / 2) * (bWidth / 2);
				int blockType = iBlockType[idxBT];
				WriteInt(buffer[h], blockType);
				for (int k = 0; k < 3; k++) {
					for (int i = 8 * h; i < 8 * h + 8; i++) {
						for (int j = 8 * w; j < 8 * w + 8; j++) {
							buffer[h].push_back(' ');
							WriteInt(buffer[h], tmpImg[(i * bWidth * 8 + j) * 3 + k]);
						}
					}
				}
				buffer[h].push_back('\n');
			}
		}
		for (int h = 0; h < bHeight; h++) {
			fwrite(buffer[h].c_str(), 1, buffer[h].size(), file);
			buffer[h].clear();
		}

		// Show progress
		int i = f;
		int progress = static_cast<int>((static_cast<double>(i + 1) / Length) * 50);
		std::cout << "\rDCT\t[";
		for (int j = 0; j < 50; ++j) {
			if (j < progress) std::cout << "=";
			else std::cout << " ";
		}
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << "] " << ((i + 1) * 100 / Length) << "% " << i + 1 << "/" << Length;
		std::cout << " | " << duration.count() / 1000 << "." << duration.count() % 1000 << "s (" << duration.count() / (i + 1) << "ms/F)";
		std::cout.flush();
		if (i == Length - 1) {
			std::cout << std::endl;
		}
	}
	fclose(file);
}


void MyVideoGPU::idct(std::vector<unsigned char>& block, std::vector<int>& coeff, std::vector<std::vector<float>>& temp, int n) {
	for (int x = 0; x < 8; x++) {
		for (int y = 0; y < 8; y++) {
			temp[x][y] = 0;
			for (int v = 0; v < 8; v++) {
				temp[x][y] += IDCTCoeffV[y][v] * coeff[v + x * 8];
			}
		}
	}
	for (int x = 0; x < 8; x++) {
		for (int y = 0; y < 8; y++) {
			float sum = 0.0;
			for (int u = 0; u < 8; u++) {
				sum += IDCTCoeffU[x][u] * temp[u][y];
			}
			sum *= n; // Scale by normalization factor
			block[y + x * 8] = (unsigned char)max(0.0, min(sum, 255.0)); // Clamp to valid pixel range
		}
	}
}

int MyVideoGPU::ReadInt(std::string& buffer, int& pos) {
	int isNeg = 1;
	if (buffer[pos] == '-') {
		isNeg = -1;
		pos++;
	}
	int ret = 0;
	while (isdigit(buffer[pos])) {
		ret = ret * 10 + buffer[pos++] - '0';
	}
	return isNeg * ret;
}

void MyVideoGPU::readCmp(const std::string& filename) {
	for (int x = 0; x < 8; x++) {
		for (int u = 0; u < 8; u++) {
			double cu = (u == 0) ? (0.5 * sqrt(2)) : 1.0;
			IDCTCoeffU[x][u] = 0.5 * cu * cos((2 * x + 1) * u * M_PI / 16.0);
		}
	}

	for (int y = 0; y < 8; y++) {
		for (int v = 0; v < 8; v++) {
			double cv = (v == 0) ? (0.5 * sqrt(2)) : 1.0;
			IDCTCoeffV[y][v] = 0.5 * cv * cos((2 * y + 1) * v * M_PI / 16.0);
		}
	}
	int bWidth = (Width + 7) / 8;
	int bHeight = (Height + 7) / 8;
	const char* fileName = filename.c_str();
	unsigned char* iData = new unsigned char[Width * Height * 3];

	std::vector<std::vector<std::string>> buffer(bHeight, std::vector<std::string>(bWidth));
	int N1, N2;

	FILE* file = fopen(fileName, "r");
	const size_t lineSize = 8192;
	char line[lineSize];
	//	std::ifstream file(fileName);
	if (!file) {
		return;
	}
	if (fgets(line, lineSize, file)) {
		sscanf(line, "%d %d %d", &N1, &N2, &Length);
	}

	N1 = 1 << N1;
	N2 = 1 << N2;
	std::cout << "N1:" << N1 << " N2:" << N2 << " Length:" << Length << "\n";

	Data.clear();
	auto start = std::chrono::high_resolution_clock::now();
	for (int f = 0; f < Length; f++) {
		for (int h = 0; h < bHeight; h++) {
			for (int w = 0; w < bWidth; w++) {
				if (!fgets(line, lineSize, file)) {
					std::cout << "Read Error\n";
					return;
				}
				buffer[h][w] = line;
			}
		}
#pragma omp parallel for
		for (int h = 0; h < bHeight; h++) {
			std::vector<int> coeff(8 * 8, 0);
			std::vector<unsigned char> block(8 * 8, 0);
			std::vector<std::vector<float>> temp(8, std::vector<float>(8, 0.0));
			for (int w = 0; w < bWidth; w++) {
				int pos = 0;
				int bType = ReadInt(buffer[h][w], pos); pos++;
				for (int k = 2; k >= 0; k--) {
					for (int i = 0; i < 64; i++) {
						coeff[i] = ReadInt(buffer[h][w], pos); pos++;
					}
					if (bType == 0) {
						idct(block, coeff, temp, N1);
					}
					else {
						idct(block, coeff, temp, N2);
					}
					for (int i = 0; i < 8; i++) {
						for (int j = 0; j < 8; j++) {
							if ((i + 8 * h) >= Height) continue;
							iData[3 * (j + 8 * w + (i + 8 * h) * Width) + k] = block[j + i * 8];
						}
					}
				}
			}
		}
		MyImage image;
		image.Initialize(Width, Height, iData);
		Data.push_back(std::move(image));

		// Show progress
		int i = f;
		int progress = static_cast<int>((static_cast<double>(i + 1) / Length) * 50);
		std::cout << "\rIDCT\t[";
		for (int j = 0; j < 50; ++j) {
			if (j < progress) std::cout << "=";
			else std::cout << " ";
		}
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << "] " << ((i + 1) * 100 / Length) << "% " << i + 1 << "/" << Length;
		std::cout << " | " << duration.count() / 1000 << "." << duration.count() % 1000 << "s (" << duration.count() / (i + 1) << "ms/F)";
		std::cout.flush();
		if (i == Length - 1) {
			std::cout << std::endl;
		}
	}
	printf("Finish Read CMP\n");
	delete[] iData;
}