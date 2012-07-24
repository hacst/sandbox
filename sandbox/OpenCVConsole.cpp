//// OpenCVConsole.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
////
//#include "stdafx.h"
//
//#include <opencv/cv.h>
//#include <opencv/cxcore.h>
//#include <opencv/highgui.h>
//
//const int MY_IMAGE_WIDTH  = 640;
//const int MY_IMAGE_HEIGHT = 480;
//const int MY_WAIT_IN_MS   = 20;
//
//
//IplImage* recognizeArea(IplImage* image){
//	IplImage* hsvImage = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 3);
//	// two pictures necessary because red can be at the beginning and at the end
//	IplImage* threshold = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
//	IplImage* threshold2 = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
//
//	// color wrap-around to recognize red
//	CvScalar hsvMinRed1 = cvScalar(0, 200, 130, 0); //(0, 200, 150, 0)
//	CvScalar hsvMaxRed1 = cvScalar(10, 256, 256, 150); //(10, 256, 256, 0)
//	CvScalar hsvMinRed2 = cvScalar(170, 200, 130, 0); //(170, 200, 150, 0)
//	CvScalar hsvMaxRed2 = cvScalar(256, 256, 256, 150); //(256, 256, 256, 0)
//
//	// make RGB to HSV
//	cvCvtColor(image, hsvImage, CV_BGR2HSV);
//
//	// detect all red in a picture and make a binary picture (white == red)
//	// make treshold
//	cvInRangeS(hsvImage, hsvMinRed1, hsvMaxRed1, threshold);
//	cvInRangeS(hsvImage, hsvMinRed2, hsvMaxRed2, threshold2);
//	cvOr(threshold, threshold2, threshold);
//
//	// Background Image
//	IplImage* backGroundImageTemplate = cvLoadImage("Penguins.jpg");
//	IplImage* backGroundImage = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 3);
//	cvResize(backGroundImageTemplate, backGroundImage);
//	
//	// make another background
//	for(int i = 0; i < threshold->imageSize; i++){
//		if(threshold->imageData[i] != 0){
//			// replace
//			for(int j = 0; j < image->nChannels; j++){
//				image->imageData[i*3+j] = backGroundImage->imageData[i*3+j]; 
//			}
//		}
//	}
//
//	// release all
//	cvReleaseImage(&hsvImage);
//	cvReleaseImage(&threshold);
//	cvReleaseImage(&threshold2);
//	cvReleaseImage(&backGroundImageTemplate);
//	cvReleaseImage(&backGroundImage);
//
//
//	return image;
//}
//
//
//// LiveLoop zieht Bilder von der Kamera ein und stellt die Bilder in einem Fenster dar.
//// Beenden der Anwendung: Taste 'e' drücken
//void LiveLoop()
//{
//  IplImage* grabImage = 0;
//  IplImage* resultImage = 0;
//  int key;
//
//  // create window for live video
//  cvNamedWindow("Live", CV_WINDOW_AUTOSIZE);
//  // create connection to camera
//  CvCapture* capture = cvCreateCameraCapture(CV_CAP_OPENNI_ASUS);
//
//  //CvCapture* capture = cvCaptureFromCAM(0);
//  // init camera
//  //cvSetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH, MY_IMAGE_WIDTH ); 
//  //cvSetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT, MY_IMAGE_HEIGHT );
//  
//  // check connection to camera by grabbing a frame
//  if(!cvGrabFrame(capture))
//  {      
//    cvReleaseCapture(&capture);
//    cvDestroyWindow("Live");
//    printf("Could not grab first frame\n\7");
//    exit(0);
//  }
//  
//  // retrieve the captured frame
//  grabImage=cvRetrieveFrame(capture);           
//  // init result image, e.g. with size and depth of grabImage
//  resultImage = cvCreateImage(cvGetSize(grabImage), grabImage->depth, grabImage->nChannels);
// 
//  bool continueGrabbing = true;
//  while (continueGrabbing)
//  {
//    if(!cvGrabFrame(capture))
//    {              
//      cvReleaseCapture(&capture);
//      cvDestroyWindow("Live");
//      cvReleaseImage(&grabImage);
//      printf("Could not grab a frame\n\7");
//      continueGrabbing = false;
//    }
//    else
//    {
//      grabImage = cvRetrieveFrame(capture);          
//    
//      // todo: insert image processing 
//
//	  grabImage = recognizeArea(grabImage);
//
//		
//	  
//		// dummy line: copy grabbed image
//		cvCopy(grabImage, resultImage, NULL );
//        		
//      // end todo
//
//      cvShowImage("Live", resultImage);
//
//      key = cvWaitKey(MY_WAIT_IN_MS);
//
//      if (key == 'e')
//        continueGrabbing = false;
//    }
//  }
// 
//  // release all
//  cvReleaseCapture(&capture);
//  cvDestroyWindow("Live");
//  cvReleaseImage(&resultImage);
//}
//
//
//
//
//void test1(){
//	  /* 1
//	  CvPoint seed_point = cvPoint(305,195);
//	  CvScalar color = CV_RGB(250,0,0);
//	  cvFloodFill( grabImage, seed_point, color, cvScalarAll(5.0), cvScalarAll(5.0), NULL, 4, NULL );
//	  */
//}
//
//void showFaces(){
//    /* Gesichtserkennung
//	  CvMemStorage* storage = cvCreateMemStorage(0);
//	// Note that you must copy C:\Program Files\OpenCV\data\haarcascades\haarcascade_frontalface_alt2.xml
//	// to your working directory
//	CvHaarClassifierCascade* cascade = (CvHaarClassifierCascade*)cvLoad( "haarcascade_frontalface_alt2.xml" );
//	double scale = 1.3;
//	
//	static CvScalar colors[] = { {{0,0,255}}, {{0,128,255}}, {{0,255,255}}, 
//	{{0,255,0}}, {{255,128,0}}, {{255,255,0}}, {{255,0,0}}, {{255,0,255}} };
//	
//	// Detect objects
//	cvClearMemStorage( storage );
//	CvSeq* objects = cvHaarDetectObjects( grabImage, cascade, storage, 1.1, 4, 0, cvSize( 40, 50 ));
//
//	CvRect* r;
//	// Loop through objects and draw boxes
//	for( int i = 0; i < (objects ? objects->total : 0 ); i++ ){
//		r = ( CvRect* )cvGetSeqElem( objects, i );
//		cvRectangle( grabImage, cvPoint( r->x, r->y ), cvPoint( r->x + r->width, r->y + r->height ),
//			colors[i%8]);
//	}
//	*/
//}
//
//void testBild(){
//	IplImage* resultImage = 0;
//	
//	IplImage* img = cvLoadImage( "Spielzeug.bmp" );
//		
//	CvMemStorage* storage = cvCreateMemStorage(0); //needed for Hough circles
//
//	IplImage *  hsv_frame    = cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 3);
//    IplImage*  thresholded    = cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 1);
//    IplImage*  thresholded2    = cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 1);
//
//	CvScalar hsv_min = cvScalar(0, 50, 170, 0);
//    CvScalar hsv_max = cvScalar(10, 180, 256, 0);
//    CvScalar hsv_min2 = cvScalar(170, 50, 170, 0);
//    CvScalar hsv_max2 = cvScalar(256, 180, 256, 0);
//
//	// color detection using HSV
//    cvCvtColor(img, hsv_frame, CV_BGR2HSV);
//    // to handle color wrap-around, two halves are detected and combined
//    cvInRangeS(hsv_frame, hsv_min, hsv_max, thresholded);
//    cvInRangeS(hsv_frame, hsv_min2, hsv_max2, thresholded2);
//    cvOr(thresholded, thresholded2, thresholded);
//
//	cvSaveImage("thresholded.jpg",thresholded);
//	//cvSaveImage("thresholded2.jpg",thresholded2);
//	//cvSaveImage("hsv_frame.jpg",hsv_frame);
//
//	// hough detector works better with some smoothing of the image
//    cvSmooth( thresholded, thresholded, CV_GAUSSIAN, 9, 9 );
//    CvSeq* circles = cvHoughCircles(thresholded, storage, CV_HOUGH_GRADIENT, 2, thresholded->height/4, 100, 40, 20, 200);
// 
//    for (int i = 0; i < circles->total; i++){
//         float* p = (float*)cvGetSeqElem( circles, i );
//         printf("Entdeckt! x=%f y=%f r=%f\n\r",p[0],p[1],p[2] );
//         cvCircle( img, cvPoint(cvRound(p[0]),cvRound(p[1])),
//                    3, CV_RGB(0,255,0), -1, 8, 0 );
//         cvCircle( img, cvPoint(cvRound(p[0]),cvRound(p[1])),
//                    cvRound(p[2]), CV_RGB(255,0,0), 3, 8, 0 );
//    }
// 
//    cvSaveImage("frame.jpg", img);
//
//
//	resultImage = cvCreateImage(cvGetSize(img), img->depth, img->nChannels);
//	cvCopy(img, resultImage, NULL );
//
//	cvNamedWindow( "Example1", CV_WINDOW_AUTOSIZE );
//	cvShowImage("Example1", resultImage);
//	cvWaitKey(0);
//	cvReleaseImage( &resultImage );
//	cvDestroyWindow( "Example1" );
//
//
//}
//
//void alphaKanalTest(){
//	IplImage* src1 = cvLoadImage( "Spielzeug.bmp" );
//	IplImage* src2 = cvLoadImage( "Ellipsen.bmp" );
//
//	int x = 50;
//    int y = 50;
//    int width = 200;
//    int height = 200;
//    double alpha = 0.0;
//    double beta = 1.0;
//    cvSetImageROI(src1, cvRect(x,y,width,height));
//    cvSetImageROI(src2, cvRect(x,y,width,height));
//    cvAddWeighted(src1, alpha, src2, beta, 0.0, src1);
//    cvResetImageROI(src1);
//    cvNamedWindow( "Alpha_blend", 1 );
//    cvShowImage( "Alpha_blend", src1 );
//    cvWaitKey();
//}
//
//int _tmain(int argc, _TCHAR* argv[])
//{
//	LiveLoop();
//	//testBild();
//	//alphaKanalTest();
//	
//	return 0;
//}
//
