#include "Video.h"
#include "Image.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <time.h>
#include <algorithm>
#include <chrono>

MyVideo::MyVideo() {
	Data.clear();
	Width = -1;
	Height = -1;
	Length = 0;
	N1 = -1;
	N2 = -1;
	VideoPath.clear();
}

MyVideo::~MyVideo() {
	Data.clear();
}

// MyImage::ReadImage
// Function to read the image given a path
bool MyVideo::ReadVideo()
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
	long long fileSize = 0;
	fseek(IN_FILE, 0, SEEK_END);
	fileSize = ftell(IN_FILE);
	fseek(IN_FILE, 0, SEEK_SET);
	//fileSize = 50 * frameSize;
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

void MyVideo::MAD(std::vector<std::vector<float>>& lastFrame, std::vector<std::vector<float>>& curFrame, std::vector<std::vector<std::pair<int, int>>>& motionVec, int k) {
	std::vector<std::pair<int, int>> offsets;
	for (int i = -k; i <= k; i++) {
		for (int j = -k; j <= k; j++) {
			offsets.emplace_back(i, j);
		}
	}
	std::sort(offsets.begin(), offsets.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
		return std::abs(a.first) + std::abs(a.second) < std::abs(b.first) + std::abs(b.second);
		});
	int height = lastFrame.size();
	int width = lastFrame[0].size();
#pragma omp parallel for
	for (int i = 0; i < height; i += 16) {
		for (int j = 0; j < width; j += 16) {
			int h = height - i < 16 ? height - i : 16;
			int w = width - j < 16 ? width - j : 16;
			float sum = INT_MAX;
			std::pair<int, int> motion;
			motion.first = 0, motion.second = 0;
			for (const auto& offset : offsets) {
				int p = offset.first;
				int q = offset.second;

				if (p + i < 0 || p + i + h > height || q + j < 0 || q + j + w > width) continue;

				float tmp = 0;
				for (int r = 0; r < h; r++) {
					for (int s = 0; s < w; s++) {
						tmp += std::fabs(curFrame[i + r][j + s] - lastFrame[p + i + r][q + j + s]);
						if (tmp > sum) break;
					}
					if (tmp > sum) break;
				}
				if (tmp < sum) {
					sum = tmp;
					motion.first = p;
					motion.second = q;
				}
			}
			motionVec[i / 16][j / 16] = motion;
		}
	}
}
void MyVideo::FME(std::vector<std::vector<int>>& lastFrame, std::vector<std::vector<int>>& curFrame, std::vector<std::vector<std::pair<int, int>>>& motionVec, int k) {

}

void MyVideo::CalcMotion() {
	int height = Height % 16 == 0 ? Height / 16 : Height / 16 + 1;
	int width = Width % 16 == 0 ? Width / 16 : Width / 16 + 1;
	std::vector<std::vector<float>> curFrame(Height, std::vector<float>(Width, 0)), lastFrame(Height, std::vector<float>(Width, 0));
	std::vector<std::vector<std::pair<int, int>>> motionVec(height, std::vector<std::pair<int, int>>(width, std::make_pair(0, 0)));
	Data[0].setMotionVec(motionVec);
	Data[0].RGB2Y(curFrame);

	std::ofstream outFile("motion_vectors.txt");
	if (!outFile) {
		std::cerr << "Error opening file for writing." << std::endl;
		return;
	}

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

void MyVideo::dct(std::vector<unsigned char>& block, std::vector<int>& coeff, std::vector<std::vector<float>>& temp, int n) {
	for (int x = 0; x < 8; x++) {
		for (int v = 0; v < 8; v++) {
			temp[x][v] = 0;
			for (int y = 0; y < 8; y++) {
				temp[x][v] += DCTCoeffY[v][y] * block[y + x * 8];
			}
		}
	}
	for (int u = 0; u < 8; u++) {
		for (int v = 0; v < 8; v++) {
			float sum = 0;
			for (int x = 0; x < 8; x++) {
				sum += DCTCoeffX[u][x] * temp[x][v];
			}
			coeff[v + u * 8] = roundf(sum / n);
		}
	}
}

void MyVideo::DCT(std::vector<std::vector<std::vector<int>>>& tmpImg, unsigned char* iData, std::vector<bool>& iBlockType) {
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
	int bWidth = (Width + 7) / 8;
	int bHeight = (Height + 7) / 8;
#pragma omp parallel for
	for (int h = 0; h < bHeight; h++) {
		std::vector<unsigned char> block(8 * 8);
		std::vector<int> coeff(8 * 8);
		std::vector<std::vector<float>> temp(8, std::vector<float>(8, 0));
		for (int w = 0; w < bWidth; w++) {
			int idxBT = w / 2 + (h / 2) * (bWidth / 2);
			int blockType = iBlockType[idxBT];
			for (int k = 0; k < 3; k++) {
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < 8; j++) {
						int idx = j + i * 8;
						int idxO = j + 8 * w + (i + 8 * h) * Width;
						if (j + 8 * w >= Width || i + 8 * h >= Height) {
							block[idx] = 0;
						}
						else {
							block[idx] = iData[3 * idxO + k];
						}
					}
				}
				if (blockType == 0) {
					dct(block, coeff, temp, 1 << N1);
				}
				else {
					dct(block, coeff, temp, 1 << N2);
				}
				for (int i = 8 * h, cnt = 0; i < 8 * h + 8; i++) {
					for (int j = 8 * w; j < 8 * w + 8; j++, cnt++) {
						tmpImg[k][i][j] = coeff[cnt];
					}
				}
			}
		}
	}
}

void MyVideo::WriteInt(std::string& buffer, int x) {
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

void MyVideo::writeCoeffToCmp(const std::string& filename) {
	const char* fileName = filename.c_str();
	FILE* file = fopen(fileName, "w");
	fprintf(file, "%d %d %d\n", N1, N2, Length);
	int bWidth = (Width + 7) / 8;
	int bHeight = (Height + 7) / 8;

	std::vector<std::string> buffer(bHeight);
	std::vector<std::vector<std::vector<int>>> tmpImg(3, std::vector<std::vector<int>>(bHeight * 8, std::vector<int>(bWidth * 8)));

	auto start = std::chrono::high_resolution_clock::now();
	for (int f = 0; f < Length; f++) {
		std::vector<bool> iBlockType = Data[f].getBlockType();
		DCT(tmpImg, Data[f].getImageData(), iBlockType);
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
							WriteInt(buffer[h], tmpImg[k][i][j]);
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


void MyVideo::idct(std::vector<unsigned char>& block, std::vector<int>& coeff, std::vector<std::vector<float>>& temp, int n) {
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

int MyVideo::ReadInt(std::string& buffer, int& pos) {
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

void MyVideo::readCmp(const std::string& filename) {
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