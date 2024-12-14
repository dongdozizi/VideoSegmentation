Purpose:
	This project is an interesting application of block-based motion detection and compression in video-based communication industries including entertainment, security, defense etc. Here you are required to separate an input video into different layers based on motion characteristics - those that represent the foreground layers and the background layer. Each layer is then compressed differently according to input parameters provided. The theory taught in class dealt with computing motion vectors to help better compression of video, here we use it to segment a moving region and then use the same for compression. 

Encoder compile:
	Image.exe rgb_file N1 N2

Decoder compile:
	Image.exe cmp_file wav_file

CMP file description:
	FrameCount N1 N2
	block_type r_coeff[0] ... r_coeff[63] g_coeff[0] ... g_coeff[63] b_coeff[0] ... b_coeff[63]
	...
	block_type r_coeff[0] ... r_coeff[63] g_coeff[0] ... g_coeff[63] b_coeff[0] ... b_coeff[63]

MultiThread:
	Enable OpenMP to get faster result.

For GPU:
	add /openmp /EHsc /D_SILENCE_AMP_DEPRECATION_WARNINGS to command line 
