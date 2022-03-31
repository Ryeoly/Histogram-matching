#include <iostream>
#include <math.h>
#include <windows.h>
#pragma warning(disable : 4996)

HWND hwnd;
HDC hdc;

using namespace std;

UCHAR** memory_alloc2D(int width, int height)
{
	UCHAR** ppMem2D = 0;
	int	i;

	ppMem2D = (UCHAR**)calloc(sizeof(UCHAR*), height);
	if (ppMem2D == 0) {
		return 0;
	}

	*ppMem2D = (UCHAR*)calloc(sizeof(UCHAR), height * width);
	if ((*ppMem2D) == 0) {    
		free(ppMem2D);
		return 0;
	}

	for (i = 1; i < height; i++) {
		ppMem2D[i] = ppMem2D[i - 1] + width;
	}

	return ppMem2D;
}

void MemoryClear(UCHAR** buf) {
	if (buf) {
		free(buf[0]);
		free(buf);
		buf = NULL;
	}
}

//A function that draws a graph using histogram
void DrawHistogram(float histogram[256], int x_origin, int y_origin) {
	int cnt = 256;
	MoveToEx(hdc, x_origin, y_origin, 0);
	LineTo(hdc, x_origin + 255, y_origin);

	MoveToEx(hdc, x_origin, 100, 0);
	LineTo(hdc, x_origin, y_origin);

	for (int CurX = 0; CurX < cnt; CurX++) {
		for (int CurY = 0; CurY < histogram[CurX]; CurY++) {
			MoveToEx(hdc, x_origin + CurX, y_origin, 0);
			LineTo(hdc, x_origin + CurX, y_origin - histogram[CurX]*4000);
		}
	}
}

//A function that draws a graph using CDF
void cdf_DrawHistogram(float histogram[256], int x_origin, int y_origin) {
	int cnt = 256;
	MoveToEx(hdc, x_origin, y_origin, 0);
	LineTo(hdc, x_origin + 255, y_origin);

	MoveToEx(hdc, x_origin, 100, 0);
	LineTo(hdc, x_origin, y_origin);

	for (int CurX = 0; CurX < cnt; CurX++) {
		for (int CurY = 0; CurY < histogram[CurX]; CurY++) {
			MoveToEx(hdc, x_origin + CurX, y_origin, 0);
			LineTo(hdc, x_origin + CurX, y_origin - histogram[CurX]*150);
		}
	}
}

//A function to create a histogram using imgbuf
void imgbuf_to_Histogram(int width, int height, UCHAR** img, float histogram[256]) {
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			int idx = img[i][j];
			histogram[idx]++;							//count event
		}
	}
	
	for (int i = 0; i < 256; i++) {
		histogram[i] /= width * height;					//divide the entire event because it is a probability
	}
}

//A function to create a CDF using histogram
void his_to_cdf(float histogram[256], float cdf[256]) {
	float tmp = 0;
	for (int i = 0; i < 256; i++) {
		tmp += histogram[i];
		cdf[i] = tmp;
	}
}

int main(void) {
	system("color F0");
	hwnd = GetForegroundWindow();
	hdc = GetWindowDC(hwnd);

	/* width , height 정해야됨 중요! , 이미지를 바꿀 때마다 맞는 것으로 맞춰준다. */
	int width = 350;										//input_image width
	int height = 555;										//input_image height
	int t_width = 512;										//target_image width
	int t_height = 512;										//target_image height
	/* !!!!!!!!!!!!!!!중요!!!!!!!!!!!!*/

	float input_histogram[256] = { 0, };					//input image histogram
	float want_histogram[256] = { 0, };						//target image histogram
	float input_cdf[256] = { 0, };							//input image cdf
	float want_cdf[256] = { 0, };							//target image cdf
	UCHAR** result_imgbuf = memory_alloc2D(width, height);	//result image. 
	float result_histogram[256] = { 0, };					//result image histogram
	float result_cdf[256] = { 0, };							//result image cdf

	/* input image */
	FILE* input_img = fopen("gray\\gAirplane350_555.raw", "rb");
	if (!input_img) {
		printf("fail open(input_image)");
		return 0;
	}
	UCHAR** input_imgbuf = memory_alloc2D(width, height);
	fread(&input_imgbuf[0][0], sizeof(UCHAR), width * height, input_img);
	imgbuf_to_Histogram(width, height, input_imgbuf, &input_histogram[0]);		//r histogram 구함


	/* target image */
	FILE* want_img = fopen("gray\\barbara(512x512).raw", "rb");
	if (!want_img) {
		printf("fail open(target_image)");
		return 0;
	}
	UCHAR** want_imgbuf = memory_alloc2D(t_width, t_height);
	fread(&want_imgbuf[0][0], sizeof(UCHAR), t_width * t_height, want_img);
	imgbuf_to_Histogram(t_width, t_height, want_imgbuf, &want_histogram[0]);		//z histogram 구함

	/* input cdf, target cdf 만드는 중 */
	his_to_cdf(input_histogram, &input_cdf[0]);
	his_to_cdf(want_histogram, &want_cdf[0]);

	float distance = 999;
	float tmp1 = 0;
	int trans_value = 999;
	/* G^-1(T(r))을 순차적으로 적용하는 부분 */
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			UCHAR r = input_imgbuf[i][j];
			int int_r = (int)r;
			float s = input_cdf[int_r];						// T(r)

			/* 역함수 구하는 부분 */
			for (int p = 0; p < 256; p++) {
				tmp1 = fabs( (double)(s - want_cdf[p] ));	//근사치로 역함수 값을 구함
				if (tmp1 < distance) {
					distance = tmp1;
					trans_value = p;
				}
			}
			result_imgbuf[i][j] = (UCHAR)trans_value;		//G^-1[T(r)] value store
			distance = 999;
			tmp1 = 0;
			trans_value = 999;
		}
	}
	imgbuf_to_Histogram(width, height, result_imgbuf, &result_histogram[0]);
	his_to_cdf(result_histogram, &result_cdf[0]);

	/* Draw Histogram and CDF */
	DrawHistogram(input_histogram, 30, 200);
	DrawHistogram(want_histogram, 300, 200);
	DrawHistogram(result_histogram, 600, 200);
	cdf_DrawHistogram(input_cdf, 30, 400);
	cdf_DrawHistogram(want_cdf, 300, 400);
	cdf_DrawHistogram(result_cdf, 600, 400);
	

	FILE* fp_outputImg = fopen("gray\\output.raw", "wb");	//result store
	fwrite(&result_imgbuf[0][0], sizeof(UCHAR), width * height, fp_outputImg);

	MemoryClear(input_imgbuf);
	MemoryClear(want_imgbuf);
	MemoryClear(result_imgbuf);

	fclose(input_img);
	fclose(want_img);
	fclose(fp_outputImg);
	return 0;
}
