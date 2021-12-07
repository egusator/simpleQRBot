#include <stdio.h>
#include <tgbot/tgbot.h>
#include "opencv2/objdetect.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/imgcodecs.hpp"
#include <string>
#include <iostream>
#include <curl/curl.h>

using namespace std;
using namespace cv;
using namespace TgBot;
string s;
static bool g_modeMultiQR = false;
static bool g_detectOnly = false;

static string g_out_file_name, g_out_file_ext;
static int g_save_idx = 0;

static bool g_saveDetections = false;
static bool g_saveAll = false;

size_t callbackFunction(void*, size_t , size_t , void*);

bool downloadJpegFromUrl(string);

void drawQRCodeResults(Mat &, const vector<Point> &,
		const vector<cv::String> &, double );

static void runQR(QRCodeDetector&, const Mat&, vector<Point>&,
		vector<cv::String>&);

int imageQRCodeDetect(string filePath);


int main() {
	string token;
	ifstream tokenFile;
	tokenFile.open("token.txt"); 
	getline(tokenFile, token);


	Bot bot(token);
	bot.getEvents().onCommand("start",
			[&bot](TgBot::Message::Ptr message) {
				bot.getApi().sendMessage(message->chat->id,
						"This bot detects the QR-codes from photos and decodes them. Please send me photos.");
			});
	bot.getEvents().onAnyMessage(
			[&bot](TgBot::Message::Ptr message) {
				if (message->photo.size() > 0) {
					for (PhotoSize::Ptr p: message->photo) {
						cout << p->width << " "<< p->height << "\n";
					}
					File::Ptr sourceImage = bot.getApi().getFile(
							message->photo[3]->fileId);
					cout << sourceImage->filePath;
					imageQRCodeDetect(sourceImage->filePath);
					if (s.length() > 0)
					bot.getApi().sendMessage(message->chat->id, s);
				}
			});
	try {
		printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
		TgBot::TgLongPoll longPoll(bot);
		while (true) {
			printf("Long poll started\n");
			longPoll.start();
		}
	} catch (TgBot::TgException &e) {
		printf("error: %s\n", e.what());
	}
	return 0;
}

size_t callbackFunction(void *ptr, size_t size, size_t nmemb, void* userdata)
{
    FILE* stream = (FILE*)userdata;
    if (!stream)
    {
        printf("!!! No stream\n");
        return 0;
    }

    size_t written = fwrite((FILE*)ptr, size, nmemb, stream);
    return written;
}


bool downloadJpegFromUrl(string u)
{

	const char* url = u.c_str();
    sleep(1);
    FILE* fp = fopen("pic.jpg", "wb");
    if (!fp)
    {
        printf("!!! Failed to create file on the disk\n");
        return false;
    }
     	cout << "opened file "<< fp << "\n";


    CURL* curlCtx = curl_easy_init();

    curl_easy_setopt(curlCtx, CURLOPT_URL, url);

    curl_easy_setopt(curlCtx, CURLOPT_WRITEDATA, fp);

    curl_easy_setopt(curlCtx, CURLOPT_WRITEFUNCTION, callbackFunction);

    curl_easy_setopt(curlCtx, CURLOPT_FOLLOWLOCATION, 1);

    CURLcode rc = curl_easy_perform(curlCtx);
    if (rc)
    {
        printf("!!! Failed to download: %s\n", url);
        return false;
    }

    long res_code = 0;
    curl_easy_getinfo(curlCtx, CURLINFO_RESPONSE_CODE, &res_code);
    if (!((res_code == 200 || res_code == 201) && rc != CURLE_ABORTED_BY_CALLBACK))
    {
        printf("!!! Response code: %d\n", res_code);
        return false;
    }

    curl_easy_cleanup(curlCtx);

    fclose(fp);

    return true;
}

void drawQRCodeResults(Mat &frame, const vector<Point> &corners,
		const vector<cv::String> &decode_info, double fps) {
	if (!corners.empty()) {
		for (size_t i = 0; i < corners.size(); i += 4) {
			size_t qr_idx = i / 4;
			vector<Point> qrcode_contour(corners.begin() + i,
					corners.begin() + i + 4);

			if (decode_info.size() > qr_idx) {
				if (!decode_info[qr_idx].empty()) {
					cout << "'" << decode_info[qr_idx] << "'" << endl;
					 s = string(decode_info[qr_idx]);

				}
				else {
					cout << "can't decode QR code" << endl;
					s = "can't decode QR code";
				}
			} else {
				cout << "decode information is not available (disabled)"
						<< endl;
			}
		}
	} else {
		cout << "QR code is not detected" << endl;
		s = "QR code is not detected";
	}

}


static
void runQR(QRCodeDetector &qrcode, const Mat &input, vector<Point> &corners,
		vector<cv::String> &decode_info
// +global: bool g_modeMultiQR, bool g_detectOnly
		) {
	if (!g_modeMultiQR) {
		if (!g_detectOnly) {
			String decode_info1 = qrcode.detectAndDecode(input, corners);
			decode_info.push_back(decode_info1);
		} else {
			bool detection_result = qrcode.detect(input, corners);
			CV_UNUSED(detection_result);
		}
	} else {
		if (!g_detectOnly) {
			bool result_detection = qrcode.detectAndDecodeMulti(input,
					decode_info, corners);
			CV_UNUSED(result_detection);
		} else {
			bool result_detection = qrcode.detectMulti(input, corners);
			CV_UNUSED(result_detection);
		}
	}
}


int imageQRCodeDetect(string filePath) {
	const int count_experiments = 10;
	downloadJpegFromUrl("https://api.telegram.org/file/bot2014061180:AAFmtKOdBI-EAf6GN9V3x8Zu4YliLeOk-Iw/" + filePath);
	Mat input = imread("pic.jpg", IMREAD_COLOR);
	QRCodeDetector qrcode;
	vector<Point> corners;
	vector<cv::String> decode_info;

	TickMeter timer;
	for (size_t i = 0; i < count_experiments; i++) {
		corners.clear();
		decode_info.clear();

		timer.start();
		runQR(qrcode, input, corners, decode_info);
		timer.stop();
	}
	double fps = count_experiments / timer.getTimeSec();
	cout << "FPS: " << fps << endl;

	Mat result;
	input.copyTo(result);
	drawQRCodeResults(result, corners, decode_info, fps);



	if (!g_out_file_name.empty()) {
		string out_file = g_out_file_name + g_out_file_ext;
		cout << "Saving result: " << out_file << endl;
		imwrite(out_file, result);
	}
}
