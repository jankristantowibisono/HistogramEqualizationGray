#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdio.h>
#include <Math.h>
#include "Bmp.h" 
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

using namespace std;
// CONSTANTS //////////////////////////////////////////////////////////////////
const char *FILE_NAME = "lake_gray.bmp";
const int MAX_NAME = 512;               // max length of string
const int HISTOGRAM_SIZE = 256;         // histogram bin size
const int WINDOW_SIZE = 3;				// window size
struct Histo{
	unsigned long data[HISTOGRAM_SIZE];
};
// global variables ///////////////////////////////////////////////////////////
char    fileName[MAX_NAME];
char    newFileName[MAX_NAME];
void    *font = GLUT_BITMAP_HELVETICA_10;    // GLUT font type
int     imageX, imageY;                 // image resolution
const unsigned char *inBuf;             // image buffer
unsigned char *outBuf;                  // image buffer for histogram equalization
Histo histogram;                // histogram bin
Histo transf;
Histo equalHistogram;           // histogram bin after equalization
float   histoScale;                     // to make histogram graph fit into window
float   sumHistoScale;                  // to make histogram graph fit into window
Image::Bmp bmp;                         // BMP loader object
double E = 4.0;
double k0 = 0.4;
double k1 = 0.02;
double k2 = 0.4;
int * perubahX;
int * perubahY;
int mode;

unsigned int getMaxHistogram(struct Histo h, int size){
	unsigned int maxFreq = 0;
	for (int i = 0; i < size; ++i){
		if (h.data[i] > maxFreq)
			maxFreq = h.data[i];
	}
	return maxFreq;
}

double getMeanPixel(struct Histo his,int w,int h){
	double sum = 0.0;
	int minus = 0;
	for (int i = 0; i < HISTOGRAM_SIZE; ++i){
		sum += i * his.data[i];
	}
	double rata2 = (double) sum / (w * h);
	return rata2;
}
double getStdv(struct Histo his,double m, int w, int h){
	double sum = 0.0;

	for (int i = 0; i < HISTOGRAM_SIZE; ++i){
		if (his.data[i] != 0){
			sum += ((double)i - m) * ((double)i - m) *  his.data[i];
		}
	}
	double variance = (double)sum / (w * h);
	return sqrt(variance);
}

/* get histogram for 8 bits images */
struct Histo calculate_histogram(const unsigned char* data, int width, int height){
	long i;
	int k;
	struct Histo h;
	fill_n(h.data, HISTOGRAM_SIZE, 0);
	for (i = 0; i<width * height; i++){
		k = (int)data[i];
		h.data[k] += 1;
	}
	return h;
}
/********************************************/
/*  HISTOGRAM EQUALIZATION of A GRAY IMAGE  */
/********************************************/
void perform_histogram_equalization(struct Histo hist, int height, int width){
	double s_hist_eq[256] = { 0.0 }, sum_of_hist[256] = { 0.0 };
	long i, k, n, state_hst[256] = { 0 };

	n = width * height;

	for (i = 0; i<256; i++)  // pdf of image
	{
		s_hist_eq[i] = (double)hist.data[i] / (double)n;
	}
	sum_of_hist[0] = s_hist_eq[0];
	for (i = 1; i<256; i++)        // cdf of image
	{
		sum_of_hist[i] = sum_of_hist[i - 1] + s_hist_eq[i];
	}
	outBuf = new unsigned char[height*width];
	fill_n(transf.data, HISTOGRAM_SIZE, 0);
	for (i = 0; i<height*width; i++)
	{
		k = inBuf[i];
		outBuf[i] = (unsigned char)round(sum_of_hist[k] * 255.0);
		transf.data[k] = outBuf[i];
	}
} 
/* ends perform_histogram_equalization */
unsigned char get_pixel(int x, int y, int width, int height){
	//cout << (y * width) + (x);
	if ((x >= 0 && x< width) && (y >= 0 && y< height)){
		return inBuf[(y * width) + (x)];
	}
	return 0;
}
unsigned char perform_local_histogram_equalization(int pixel_index, int window, int width, int length,double gm,double gstdv){
	unsigned char * buffer;
	int n = window*window;
	int index_result = n / 2;
	buffer = new unsigned char[n];
	struct Histo lh; fill_n(lh.data, HISTOGRAM_SIZE, 0);
	int y = pixel_index / width; 
	int x = pixel_index % width;
	int peng = window / 2;
	int pengNeg = -1 * peng;
	//cout << imageX << " " << imageY << endl;
	int counter = 0;
	for (int i = pengNeg; i <= peng; i++){
		for (int j = pengNeg; j <= peng; j++){
			int a = x + j;int b = y + i;
			buffer[counter] = get_pixel(a,b, width, length);
			counter++;
		}
	}
	lh = calculate_histogram(buffer, window, window);
	double lm = getMeanPixel(lh, window, window); 
	double lstdv = getStdv(lh, lm, window, window);
	if (lm <= k0*gm && ((lstdv >= k1*gstdv) && (lstdv <= k2*gstdv))){
		return (unsigned char)(E * (double)buffer[index_result]);
	}else{
		return (unsigned char)((double)buffer[index_result]);
	}
}
void local_histogram_equalization(struct Histo gh, int width, int height, int window_size){
	double gm = getMeanPixel(gh, imageX, imageY);// hitung nilai rata2 global histogram 
	double gstdv = getStdv(gh, gm, imageX, imageY);// hitung stdv global histogram 
	int i = 0;
	outBuf = new unsigned char[height*width];
	for (i = 0; i < height*width; i++){
		outBuf[i] = perform_local_histogram_equalization(i, window_size, width, height,gm,gstdv);
	}
}
void drawString(const char *str, int x, int y, float color[4], void *font)
{
	//glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); // lighting and color mask
	//glDisable(GL_LIGHTING);     // need to disable lighting for proper text color

	glColor4fv(color);          // set text color
	glRasterPos2i(x, y);        // place text position

	// loop all characters in the string
	while (*str)
	{
		glutBitmapCharacter(font, *str);
		++str;
	}

	//glEnable(GL_LIGHTING);
	//glPopAttrib();
}
void Draw() {
	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_LINES);

	glColor3f(1.0, 0.0, 0.0); // original histogram
	for (int i = 0; i < HISTOGRAM_SIZE; i++){
		glVertex2f(i, 0);
		glVertex2f(i, histogram.data[i] * histoScale);
	}

	glColor3f(0.0, 0.0, 1.0); // equalize histogram
	for (int i = HISTOGRAM_SIZE; i < 2 * HISTOGRAM_SIZE; i++){
		glVertex2f(i, 0);
		glVertex2f(i, equalHistogram.data[i - HISTOGRAM_SIZE - 1] * histoScale);
	}
	if (mode == 1){
		glColor3f(0.0, 1.0, 1.0);
		for (int i = 2 * HISTOGRAM_SIZE; i < 3 * HISTOGRAM_SIZE; i++){
			glVertex2f(i, 0);
			glVertex2f(i, transf.data[i - 2 * HISTOGRAM_SIZE - 1]);
		}
	}
	glEnd();
	GLfloat color[4] = { 1, 0, 1, 0 };
	drawString("Original Image", (128), imageY * 4 / 5, color, font);
	if (mode != 0){
		GLfloat color2[4] = { 0, 0, 1, 0 };
		drawString("Equalization Image", 384, imageY * 4 / 5, color2, font);
	}
	if (mode == 1){
		GLfloat color3[4] = { 1, 0, 0, 1 };
		drawString("Transformation Histogram", 640, imageY * 4 / 5, color3, font);
	}
	
	glFlush();
}

void Initialize() {
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1024, 0, 1024, -1, 1);
}
int main(int argc, char **argv){
	mode = 1;
	cout << "Enter image name : "; cin >> fileName;
	cout << "===================MODE==================\n";
	cout << "(0)shows histogram \n(1)global histogram equalization \n(2)local histogram equalization (window size 3)\n";
	cout << "Please enter mode : "; cin >> mode;
	if (bmp.read(fileName)){
		inBuf = bmp.getData();
		imageX = bmp.getWidth();
		imageY = bmp.getHeight();
	}else{
		cout << "==============================\n";
		cout << "|- Please check image name -|\n";
		cout << "==============================\n";
		exit(0);
	}

	histogram = calculate_histogram(inBuf, imageX, imageY); // hitung histogram 
	if (mode != 0){
		if (mode == 1){
			strcpy_s(newFileName, "global_equalization_");
			strcat_s(newFileName, fileName);
			perform_histogram_equalization(histogram, imageX, imageY); // histrogram equalization
		}
		else{
			strcpy_s(newFileName, "local_equalization_");
			strcat_s(newFileName, fileName);
			local_histogram_equalization(histogram, imageX, imageY, WINDOW_SIZE);
		}
		bmp.save(newFileName, imageX, imageY, 1, outBuf); // save new image 
		equalHistogram = calculate_histogram(outBuf, imageX, imageY); // new histogram
	}

	histoScale = (float)(imageY - 1) / getMaxHistogram(histogram, HISTOGRAM_SIZE);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize(imageX, imageY);
	glutInitWindowPosition(10, 10);
	glutCreateWindow("Histogram");
	Initialize();
	glutDisplayFunc(Draw);
	glutMainLoop();

	
	return 0;
}

