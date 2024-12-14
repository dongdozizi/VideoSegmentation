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