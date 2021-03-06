#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <SDKDDKVer.h>
#include "Windows.h"
#include "winuser.h"
#include "conio.h"

using namespace cv;
using namespace std;

const char *faceCascadeFilename = "haarcascades\\haarcascade_frontalface_default.xml";
const char * eyeCascadeFilename = "haarcascades\\haarcascade_lefteye_2splits.xml";
Rect rect;


void load_classifier(CascadeClassifier &cascade, string filename) {
	try {
		string opencv_classifier_folder = "C:/opencv/sources/data/";
		cascade.load(opencv_classifier_folder + filename);
	}
	catch (Exception e) {}
	if (cascade.empty()) {
		cerr << "에러: [" << filename << "] 파일을 읽을 수 없습니다!" << endl;
		exit(1);
	}
	cout << "[" << filename << "] 읽기 완료." << endl;
}

VideoCapture init_camera(int width, int height, int cameraNumber = 0) {
	VideoCapture capture;
	try {
		capture.open(cameraNumber);
	}
	catch (Exception e) {}
	if (!capture.isOpened()) {
		cerr << "에러 : 카메라를 열 수 없습니다." << endl;
		exit(1);
	}
	capture.set(CV_CAP_PROP_FRAME_WIDTH, width);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, height);

	cout << "카메라 준비 완료 (카메라 번호 : " << cameraNumber << ")" << endl;
	return capture;
}



Mat get_videoframe(VideoCapture capture) {
	Mat frame;
	capture.read(frame);
	if (frame.empty()) {
		cerr << "ERROR: 카메라를 열 수 없습니다. " << endl;
		exit(1);
	}
	return frame;
}


Vec3f getEyeball(Mat &eye, vector<Vec3f> &firstpupil)      // 동공 찾기
{
	vector<int> sums(firstpupil.size(), 0);      //         
	for (int y = 0; y < eye.rows; y++) {
		uchar *ptr = eye.ptr<uchar>(y);
		for (int x = 0; x < eye.cols; x++) {
			int value = static_cast<int>(*ptr);
			for (int i = 0; i < firstpupil.size(); i++) {
				Point center((int)round(firstpupil[i][0]), (int)round(firstpupil[i][1]));
				int radius = (int)round(firstpupil[i][2]);
				if (pow(x - center.x, 2) + pow(y - center.y, 2) < pow(radius, 2))
					sums[i] += value;
			}
			++ptr;
		}
	}
	int smallestSum = UCHAR_MAX;
	int smallestSumIndex = 0;
	for (int i = 0; i < firstpupil.size(); i++)
	{
		if (sums[i] < smallestSum)
		{
			smallestSum = sums[i];
			smallestSumIndex = i;
		}
	}
	return firstpupil[smallestSumIndex];
}

Rect getLeftmostEye(vector<Rect> &eyes)
{
	int leftmost = UCHAR_MAX;
	int leftmostIndex = 0;
	for (int i = 0; i < eyes.size(); i++)
	{
		if (eyes[i].tl().x < leftmost)
		{
			leftmost = eyes[i].tl().x;
			leftmostIndex = i;
		}
	}
	return eyes[leftmostIndex];
}



Mat subImage;
vector<Point> centers, centers2;
Point firstlastPoint, secondlastPoint;
Point mousePoint;
SIZE t;
bool left_c = false, right_c = false, on = false;


Point detectpupil(Mat &img_input) {
	Point ret;


	int sum_x = 0, sum_y = 0;
	int count = 0;
	for (int y = 10; y < img_input.rows; y++) {
		for (int x = 0; x < img_input.cols; x++) {

			if (!img_input.at<uchar>(y, x)) {
				sum_x += x;
				sum_y += y;
				count++;
			}
		}

	}
	if (count <= 0)
		return NULL;


	ret.x = sum_x / count;
	ret.y = sum_y / count;
	return ret;

}

Mat image_binary(Mat &image) {
	Mat t_ret, t;

	threshold(image, t_ret, 4, 255, THRESH_BINARY);

	return t_ret;

}

void moveRect(int key)
{
	switch (key)
	{
	case 2490368:
		rect.y -= 5;
		break;
	case 2621440:
		rect.y += 5;
		break;
	case 2424832:
		rect.x -= 5;
		break;
	case 2555904:
		rect.x += 5;
		break;
	case 32:
		mousePoint = Point(t.cx / 2, t.cy / 2);
	default:
		break;
	}

}

bool include_rect(Rect rect, Rect lefteye) {
	if (rect.tl().x > lefteye.br().x || rect.br().x < lefteye.tl().x || rect.tl().y > lefteye.br().y ||
		rect.br().y < lefteye.tl().y)
		return false;
}

void detectEyes(Mat &frame, CascadeClassifier &faceCascade, CascadeClassifier &eyeCascade)
{
	Mat grayscale;
	cvtColor(frame, grayscale, CV_BGR2GRAY); // convert image to grayscale
	equalizeHist(grayscale, grayscale); // enhance image contrast

	vector<Rect> faces;
	faceCascade.detectMultiScale(grayscale, faces, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, Size(150, 150));
	if (faces.size() == 0)
		return; // none face was detected


	Mat face = grayscale(faces[0]); // crop the face
	vector<Rect> eyes;
	eyeCascade.detectMultiScale(face, eyes, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, Size(30, 30)); // same thing as above


	if (eyes.size() != 2)
		return; // both eyes were not detected


	Rect lefteyeRect = getLeftmostEye(eyes);

	Mat lefteye;

	Point lefteyeRect_center;
	lefteyeRect_center.x = (faces[0].tl().x * 2 + lefteyeRect.tl().x + lefteyeRect.br().x) / 2;
	lefteyeRect_center.y = (faces[0].tl().y * 2 + lefteyeRect.tl().y + lefteyeRect.br().y) / 2;


	if (!include_rect(rect, lefteyeRect))
	{
		on = true;
	}
	else
		on = false;

	cvtColor(subImage, subImage, CV_BGR2GRAY);

	equalizeHist(subImage, lefteye);

	vector<Vec3f> b_firstpupil;
	Mat binary_eye = image_binary(lefteye);

	Point t_center = (0, 0);
	t_center = detectpupil(binary_eye);
	circle(binary_eye, t_center, 10, Scalar(0, 0, 0), -1);

	HoughCircles(binary_eye, b_firstpupil, CV_HOUGH_GRADIENT, 1, lefteye.cols / 8, 250, 15, 3, 15);
	resize(binary_eye, binary_eye, Size(300, 300));


	Point a, b;


	if (b_firstpupil.size() > 0 && on) {

		left_c = true;

		Vec3f eyeball = getEyeball(binary_eye, b_firstpupil);

		Point center(eyeball[0], eyeball[1]);



		centers.push_back(center);


		if (centers.size() > 1)
		{
			Point diff;
			diff.x = (center.x - firstlastPoint.x) * 30;
			diff.y = (center.y - firstlastPoint.y) * 35;
			a = diff;
		}
		firstlastPoint = center;

		int radius = (int)eyeball[2];

		circle(binary_eye, center, radius, Scalar(255, 255, 255), 2);


	}
	else
		left_c = false;

	if (left_c) {
		mousePoint.x += (a.x);
		mousePoint.y += (a.y);
	}
}



void changeMouse(Mat &frame, Point &location)
{
	if (location.x > t.cx)
		location.x = t.cx;
	if (location.x < 0)
		location.x = 0;
	if (location.y > t.cy)
		location.y = t.cy;
	if (location.y < 0)
		location.y = 0;
	SetCursorPos(location.x, location.y);

}

int main(int argc, char **argv)
{
	CascadeClassifier faceCascade, eyeCascade;

	load_classifier(faceCascade, faceCascadeFilename);
	load_classifier(eyeCascade, eyeCascadeFilename);

	// Open webcam
	VideoCapture cap = init_camera(640, 480, 0);

	// Check if everything is ok
	if (faceCascade.empty() || eyeCascade.empty() || !cap.isOpened())
		return 1;

	Mat frame;
	bool rect_on = false;
	//화면 크기 구하기
	ZeroMemory(&t, sizeof(SIZE));
	t.cx = GetSystemMetrics(SM_CXFULLSCREEN);
	t.cy = GetSystemMetrics(SM_CYFULLSCREEN);

	mousePoint = Point(t.cx / 2, t.cy / 2);//프로그램 시작시 화면 정 중앙에 마우스 위치
	int i = 0;

	rect.x = 200;
	rect.y = 120;
	rect.width = 80;
	rect.height = 50;


	while (i < 5000)
	{
		cap >> frame; // outputs the webcam image to a Mat
		flip(frame, frame, 1);
		if (!frame.data)
			break;

		subImage = frame(rect);

		if (rect_on) {
			rectangle(frame, rect.tl(), rect.br(), Scalar(0, 255, 0), 2);
			detectEyes(frame, faceCascade, eyeCascade);

			if (on)
				changeMouse(frame, mousePoint);
		}
		else
			rectangle(frame, rect.tl(), rect.br(), Scalar(0, 0, 255), 2);

		imshow("Webcam", frame); // displays the Mat

		int key = waitKey(30);

		if (key == 27)
			break;
		else if (key == 13) {
			if (rect_on)
				rect_on = false;
			else
				rect_on = true;
		}
		else {
			moveRect(key);
		}
		i++;
	}
	return 0;
}
