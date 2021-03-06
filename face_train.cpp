/*
 * Copyright (c) 2011. Philipp Wagner <bytefish[at]gmx[dot]de>.
 * Released to public domain under terms of the BSD Simplified license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the organization nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 *   See <http://www.opensource.org/licenses/bsd-license>
 */

#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>

using namespace cv;
using namespace std;

static void help()
{
    cout << "\nThis program is for training Face Recognition System.\n"
            "It takes Sample images of subject and stores them creating a ext file for same.\n"
            "It's most known use is to train the system for future recognition.\n"
            "Usage:\n"
            "./face_train [--cascade=<cascade_path> this is the primary trained classifier such as frontal face]\n"
               "   [--name=name of subject]\n"
               "   [filename|camera_index]\n\n"
            "./face_train --cascade=\"/usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml\" --name=\"subject\"\n\n"
            "During execution:\n\tHit any key to quit.\n"
            "\tUsing OpenCV version " << CV_VERSION << "\n" << endl;
}

void detectAndDraw( Mat& img, CascadeClassifier& cascade,
                    CascadeClassifier& nestedCascade,
                    double scale, bool tryflip );

string cascadeName;
string nestedCascadeName;
string subjectName;

int main(int argc, const char *argv[]) {

  VideoCapture capture;
  Mat frame, image;
  string inputName;
  bool tryflip = false;
  CascadeClassifier cascade, nestedCascade;
  double scale = 1;
  
  cv::CommandLineParser parser(argc, argv,
			       "{help h||}"
			       "{cascade|/usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml|}"
			       "{name|subject|}"
			       );
  if (parser.has("help")) {
      help();
      return 0;
  }
  cascadeName = parser.get<string>("cascade");
  subjectName = parser.get<string>("name");
  nestedCascadeName = string("/usr/local/share/OpenCV/haarcascades/haarcascade_eye.xml");
  if (!parser.check()) {
    parser.printErrors();
    return 0;
  }
  if ( !nestedCascade.load( nestedCascadeName ) )
    cerr << "WARNING: Could not load classifier cascade for nested objects" << endl;
  if( !cascade.load( cascadeName ) ) {
    cerr << "ERROR: Could not load classifier cascade" << endl;
    help();
    return -1;
  }
  if( inputName.empty() || (isdigit(inputName[0]) && inputName.size() == 1) ) {
    int c = inputName.empty() ? 0 : inputName[0] - '0';
    if(!capture.open(c))
      cout << "Capture from camera #" <<  c << " didn't work" << endl;
  }
  else if( inputName.size() ) {
    image = imread( inputName, 1 );
    if( image.empty() )
      {
	if(!capture.open( inputName ))
	  cout << "Could not read " << inputName << endl;
      }
  }
  if( capture.isOpened() ) {
    cout << "Video capturing has been started ..." << endl;
    for(;;) {
      capture >> frame;
      if( frame.empty() )
	break;

      Mat frame1 = frame.clone();
      detectAndDraw( frame1, cascade, nestedCascade, scale, tryflip );

      int c = waitKey(10);
      if( c == 27 || c == 'q' || c == 'Q' )
	break;
    }
  }
  return 0;
}

void detectAndDraw( Mat& img, CascadeClassifier& cascade,
                    CascadeClassifier& nestedCascade,
                    double scale, bool tryflip )
{
    double t = 0;
    vector<Rect> faces, faces2;
    const static Scalar colors[] =
    {
        Scalar(255,0,0),
        Scalar(255,128,0),
        Scalar(255,255,0),
        Scalar(0,255,0),
        Scalar(0,128,255),
        Scalar(0,255,255),
        Scalar(0,0,255),
        Scalar(255,0,255)
    };
    Mat gray, smallImg;

    cvtColor( img, gray, COLOR_BGR2GRAY );
    double fx = 1 / scale;
    resize( gray, smallImg, Size(), fx, fx, INTER_LINEAR );
    equalizeHist( smallImg, smallImg );

    t = (double)cvGetTickCount();
    cascade.detectMultiScale( smallImg, faces,
        1.1, 2, 0
        //|CASCADE_FIND_BIGGEST_OBJECT
        //|CASCADE_DO_ROUGH_SEARCH
        |CASCADE_SCALE_IMAGE,
        Size(30, 30) );
    if( tryflip )
    {
        flip(smallImg, smallImg, 1);
        cascade.detectMultiScale( smallImg, faces2,
                                 1.1, 2, 0
                                 //|CASCADE_FIND_BIGGEST_OBJECT
                                 //|CASCADE_DO_ROUGH_SEARCH
                                 |CASCADE_SCALE_IMAGE,
                                 Size(30, 30) );
        for( vector<Rect>::const_iterator r = faces2.begin(); r != faces2.end(); r++ )
        {
            faces.push_back(Rect(smallImg.cols - r->x - r->width, r->y, r->width, r->height));
        }
    }
    t = (double)cvGetTickCount() - t;
    printf( "detection time = %g ms\n", t/((double)cvGetTickFrequency()*1000.) );
    for ( size_t i = 0; i < faces.size(); i++ )
    {
        Rect r = faces[i];
        Mat smallImgROI;
        vector<Rect> nestedObjects;
        Point center;
        Scalar color = colors[i%8];
        int radius;

        double aspect_ratio = (double)r.width/r.height;
        if( 0.75 < aspect_ratio && aspect_ratio < 1.3 )
        {
            center.x = cvRound((r.x + r.width*0.5)*scale);
            center.y = cvRound((r.y + r.height*0.5)*scale);
            radius = cvRound((r.width + r.height)*0.25*scale);
            circle( img, center, radius, color, 3, 8, 0 );
        }
        else
            rectangle( img, cvPoint(cvRound(r.x*scale), cvRound(r.y*scale)),
                       cvPoint(cvRound((r.x + r.width-1)*scale), cvRound((r.y + r.height-1)*scale)),
                       color, 3, 8, 0);
        if( nestedCascade.empty() )
            continue;
        smallImgROI = smallImg( r );
        nestedCascade.detectMultiScale( smallImgROI, nestedObjects,
            1.1, 2, 0
            //|CASCADE_FIND_BIGGEST_OBJECT
            //|CASCADE_DO_ROUGH_SEARCH
            //|CASCADE_DO_CANNY_PRUNING
            |CASCADE_SCALE_IMAGE,
            Size(30, 30) );
        for ( size_t j = 0; j < nestedObjects.size(); j++ )
        {
            Rect nr = nestedObjects[j];
            center.x = cvRound((r.x + nr.x + nr.width*0.5)*scale);
            center.y = cvRound((r.y + nr.y + nr.height*0.5)*scale);
            radius = cvRound((nr.width + nr.height)*0.25*scale);
            circle( img, center, radius, color, 3, 8, 0 );
        }
    }
    imshow( "result", img );
}
