/**
 * PS3EYEDriver Simple SDL 2 example, using OpenGL where available.
 * Thomas Perl <m@thp.io>; 2014-01-10
 * Joseph Howse <josephhowse@nummist.com>; 2014-12-26
 **/
#include "ps3eye.h"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/calib3d.hpp"

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

double gGetTimeSeconds()
{
	static bool sInitialized = false;
	static double PCFreq = 0.0f;
	if (!sInitialized)
	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);

		PCFreq = double(freq.QuadPart);
	}

	LARGE_INTEGER freq;
	QueryPerformanceCounter(&freq);
	return freq.QuadPart / PCFreq;
}


struct ps3eye_context {
    ps3eye_context(int width, int height, int fps) :
          eye(0)
        , devices(ps3eye::PS3EYECam::getDevices())
        , running(true)
        , last_time(0)
        , last_frames(0)
    {
        if (hasDevices()) {
            eye = devices[0];
            eye->init(width, height, (uint8_t)fps);
        }
    }

    bool hasDevices()
    {
        return (devices.size() > 0);
    }

    std::vector<ps3eye::PS3EYECam::PS3EYERef> devices;
    ps3eye::PS3EYECam::PS3EYERef eye;

    bool running;
    double last_time;
    uint32_t last_frames;
};

void run_camera(int width, int height, int fps, bool show_output, double duration)
{
	if (!show_output)
		cvDestroyAllWindows();
	
	ps3eye_context ctx(width, height, fps);
	if (!ctx.hasDevices())
		return;

	ctx.eye->setFlip(true); /* mirrored left-right */
	ctx.eye->start();

	printf("Camera mode: %dx%d@%d\n", ctx.eye->getWidth(), ctx.eye->getHeight(), ctx.eye->getFrameRate());

	IplImage* bayer_image = cvCreateImage(cvSize(ctx.eye->getWidth(), ctx.eye->getHeight()), IPL_DEPTH_8U, 1);
	IplImage* rgb_image = cvCreateImage(cvSize(ctx.eye->getWidth(), ctx.eye->getHeight()), IPL_DEPTH_8U, 4);

	double start_time = gGetTimeSeconds();
	while (ctx.running)
	{
		uint8_t* new_pixels = ctx.eye->getFrame();

		{
			double now_time = gGetTimeSeconds();

			ctx.last_frames++;

			if (now_time - ctx.last_time > 1.0f)
			{
				printf("-> FPS: %.2f\n", ctx.last_frames / (float(now_time - ctx.last_time)));
				ctx.last_time = now_time;
				ctx.last_frames = 0;
			}
		}

		if (show_output)
		{
			memcpy(bayer_image->imageData, new_pixels, ctx.eye->getWidth() * ctx.eye->getHeight());
			cvCvtColor(bayer_image, rgb_image, CV_BayerGB2BGR);

			//cvShowImage("original", bayer_image);
			cvShowImage("converted_gb", rgb_image);
		}

		free(new_pixels);
		
		if ((show_output && cvWaitKey(1) == 27) || (duration != 0.0f && (gGetTimeSeconds() - start_time) >= duration))
			break;
	}

	cvReleaseImage(&bayer_image);
	cvReleaseImage(&rgb_image);

	ctx.eye->stop();
}

void run_fps_test()
{
	int rates_qvga[] = { /*15, 20, 30, 40, 50, 60,*/ 75, 90, 100, 125, 137, 150, 187 };
	int num_rates_qvga = sizeof(rates_qvga) / sizeof(int);

	int rates_vga[] = { 15, 20, 30, 40, 50, 60, 75 };
	int num_rates_vga = sizeof(rates_vga) / sizeof(int);
	
	// Note: open cv isn't fast enough to display at rates > 60fps; we hide the output window in those case

	for (int index = 0; index < num_rates_qvga; ++index)
		run_camera(320, 240, rates_qvga[index], rates_qvga[index] <= 60, 5.0f);
	
	for (int index = 0; index < num_rates_vga; ++index)
		run_camera(640, 480, rates_vga[index], rates_vga[index] <= 60, 5.0f);
}


int
main(int argc, char *argv[])
{
	// Run mode test
	//run_fps_test();

	// Run infinitely
	run_camera(640, 480, 60, true, 0.0f);

    return EXIT_SUCCESS;
}
