/*
 *	Copyright (c) 2013, Nenad Markus
 *	All rights reserved.
 *
 *	This is an implementation of the algorithm described in the following paper:
 *		N. Markus, M. Frljak, I. S. Pandzic, J. Ahlberg and R. Forchheimer,
 *		A method for object detection based on pixel intensity comparisons,
 *		http://arxiv.org/abs/1305.4537
 *
 *	Redistribution and use of this program as source code or in binary form, with or without modifications, are permitted provided that the following conditions are met:
 *		1. Redistributions may not be sold, nor may they be used in a commercial product or activity without prior permission from the copyright holder (contact him at nenad.markus@fer.hr).
 *		2. Redistributions may not be used for military purposes.
 *		3. Any published work which utilizes this program shall include the reference to the paper available at http://arxiv.org/abs/1305.4537
 *		4. Redistributions must retain the above copyright notice and the reference to the algorithm on which the implementation is based on, this list of conditions and the following disclaimer.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>

#include <cv.h>
#include <highgui.h>

#include "rotodet.h"

int minfacesize = 0;
float qcutoff = 0.0f;

int find_rotated_faces(float rs[], float cs[], float ss[], float qs[], int maxndetections,
						unsigned char pixels[], int nrows, int ncols, int ldim)
{
	static char facefinder[] =
		#include "facefinder.array"
		;

	return find_rotated_objects(rs, cs, ss, qs, maxndetections, facefinder, pixels, nrows, ncols, ldim, 1.2f, 0.1f, minfacesize, MIN(nrows, ncols), qcutoff, 1);
}

void process_image(IplImage* frame, int draw, int print)
{
	int i;

	unsigned char* pixels;
	int nrows, ncols, ldim;

	#define MAXNDETECTIONS 2048
	int ndetections;
	float qs[MAXNDETECTIONS], rs[MAXNDETECTIONS], cs[MAXNDETECTIONS], ss[MAXNDETECTIONS];

	static IplImage* gray = 0;

	// grayscale image
	if(!gray)
		gray = cvCreateImage(cvSize(frame->width, frame->height), frame->depth, 1);
	if(frame->nChannels == 3)
		cvCvtColor(frame, gray, CV_RGB2GRAY);
	else
		cvCopy(frame, gray, 0);

	// get relevant image data
	pixels = (unsigned char*)gray->imageData;
	nrows = gray->height;
	ncols = gray->width;
	ldim = gray->widthStep;

	// actually, all the smart stuff happens in the following function
	ndetections = find_rotated_faces(rs, cs, ss, qs, MAXNDETECTIONS, pixels, nrows, ncols, ldim);

	// if the flag is set, draw each detection
	if(draw)
		for(i=0; i<ndetections; ++i)
			// we draw circles here since height-to-width ratio of the detected face regions is 1.0f
			cvCircle(frame, cvPoint(cs[i], rs[i]), ss[i]/2, CV_RGB(255, 0, 0), 4, 8, 0);

	// if the flag is set, print the results to standard output
	if(print)
	{
		printf("%d\n", ndetections);

		for(i=0; i<ndetections; ++i)
			printf("%d %d %d %f\n", (int)rs[i], (int)cs[i], (int)ss[i], qs[i]);
	}
}

void process_webcam_frames()
{
	CvCapture* capture;

	IplImage* frame;
	IplImage* framecopy;
	
	int stop;

	const char* windowname = "--------------------";

	// try to initialize video capture from the default webcam
	capture = cvCaptureFromCAM(0);
	if(!capture)
	{
		printf("Cannot initialize video capture!\n");
		return;
	}

	// start the main loop in which we'll process webcam output
	framecopy = 0;
	stop = 0;
	while(!stop)
	{
		// wait 5 miliseconds
		int key = cvWaitKey(5);

		// retrieve a pointer to the image acquired from the webcam
		if(!cvGrabFrame(capture))
			break;
		frame = cvRetrieveFrame(capture, 1);

		// we terminate the loop if we don't get any data from the webcam or the user has pressed 'q'
		if(!frame || key=='q')
			stop = 1;
		else
		{
			// we mustn't tamper with internal OpenCV buffers and that's the reason why we're making a copy of the current frame
			if(!framecopy)
				framecopy = cvCreateImage(cvSize(frame->width, frame->height), frame->depth, frame->nChannels);
			cvCopy(frame, framecopy, 0);

			// webcam outputs mirrored frames (at least on my machines); you can safely comment out this line if you find it unnecessary
			cvFlip(framecopy, framecopy, 1);

			// all the smart stuff happens in the following function
			process_image(framecopy, 1, 0);

			// display the image to the user
			cvShowImage(windowname, framecopy);
		}
	}

	// cleanup
	cvReleaseImage(&framecopy);
	cvReleaseCapture(&capture);
	cvDestroyWindow(windowname);
}

int main(int argc, char* argv[])
{
	IplImage* img = 0;

	precompute_rotluts();

	if(argc==1)
	{
		printf("Copyright (c) 2013, Nenad Markus\n");
		printf("All rights reserved.\n\n");

		minfacesize = 100;
		///qcutoff = 7.5f;
		qcutoff = 10.0f;

		process_webcam_frames();
	}
	else if(argc==2)
	{
		printf("Copyright (c) 2013, Nenad Markus\n");
		printf("All rights reserved.\n\n");

		sscanf(argv[1], "%d", &minfacesize);
		qcutoff = 7.5f;

		process_webcam_frames();
	}
	else if(argc==3)
	{
		printf("Copyright (c) 2013, Nenad Markus\n");
		printf("All rights reserved.\n\n");

		sscanf(argv[1], "%d", &minfacesize);
		sscanf(argv[2], "%f", &qcutoff);

		process_webcam_frames();
	}
	else if(argc==4)
	{

		sscanf(argv[1], "%d", &minfacesize);
		sscanf(argv[2], "%f", &qcutoff);

		img = cvLoadImage(argv[3], CV_LOAD_IMAGE_COLOR);
		if(!img)
		{
			printf("Cannot load image!\n");
			return 1;
		}

		process_image(img, 0, 1);

		cvReleaseImage(&img);
	}
	else if(argc==5)
	{
		sscanf(argv[1], "%d", &minfacesize);
		sscanf(argv[2], "%f", &qcutoff);

		img = cvLoadImage(argv[3], CV_LOAD_IMAGE_COLOR);
		if(!img)
		{
			printf("Cannot load image!\n");
			return 1;
		}

		process_image(img, 1, 0);

		cvSaveImage(argv[4], img, 0);

		cvReleaseImage(&img);
	}
	else
		return 1;

	return 0;
}
