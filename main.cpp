#include <opencv2/opencv.hpp>
#include <cstdio>
#include <cmath>
#include <vector>
#include <map>

using namespace cv;

#define RATIO 0.01

std::vector<unsigned char> get_bgr(Vec3b &v){
	std::vector<unsigned char> bgr;

	bgr.push_back(v[0]);
	bgr.push_back(v[1]);
	bgr.push_back(v[2]);

	return bgr;
}

double pixel_distance(Vec3b p1, Vec3b p2){
	double dist;
	int i;

	for (i = 0, dist = 0.0; i < 3; i++){
		dist += ((int)p1[i] - (int)p2[i]) * ((int)p1[i] - (int)p2[i]);
	}

	return sqrt(dist);
}

bool inside(int x, int y, int n, int m){
	return 0 <= x and x < n and 0 <= y and y < m;
}

int main(int argc, char *argv[]){
	int threshold, x, y, bad_x, bad_y, used, k, a, i, j;
	std::map<std::vector<unsigned char>, int> freq;
	std::vector<unsigned char> bgr;
	Vec3b p, bad_p, best_p;
	Mat original, ans, mask;
	double dist, best_dist;
	bool use;

	// Wrong usage.
	if (argc != 3){
		printf("Usage: ./main <image_in.bmp> <image_out.bmp>\n");
	}

	printf("Reading image...\n");

	// Reading image.
	original = imread(argv[1], 1);

	// Image with no data.
	if (!original.data){
		printf("No image data.\n");
		return -1;
	}

	printf("Counting frequency...\n");

	// Counting color frequency.
	for (x = 0; x < original.rows; x++){
		for (y = 0; y < original.cols; y++){
			freq[get_bgr(original.at<Vec3b>(x, y))]++;
		}
	}

	// Defining threshold and allocating mask.
	threshold = RATIO * (original.rows * original.cols);
	mask = Mat(original.rows, original.cols, CV_8UC3);

	printf("Obtaining mask...\n");

	// Obtaining mask.
	for (x = 0; x < original.rows; x++){
		for (y = 0; y < original.cols; y++){
			if (freq[get_bgr(original.at<Vec3b>(x, y))] <= threshold){
				mask.at<Vec3b>(x, y)[0] = mask.at<Vec3b>(x, y)[1] = mask.at<Vec3b>(x, y)[2] = 255;
			}
			else{
				mask.at<Vec3b>(x, y)[0] = mask.at<Vec3b>(x, y)[1] = mask.at<Vec3b>(x, y)[2] = 0;
			}
		}
	}

	printf("Writing mask...\n");

	// Writing mask.
	imwrite("mask.bmp", mask);

	printf("Inpainting image...\n");
	
	// Initializing.
	ans = Mat(original.rows, original.cols, CV_8UC3);
	k = 7;
	a = (k - 1) / 2;

	// For every bad pixel.
	for (bad_x = 0; bad_x < original.rows; bad_x++){
		for (bad_y = 0; bad_y < original.cols; bad_y ++){
			printf("Inpainting pixel (%d, %d)\n", bad_x, bad_y);

			// If it's bad.
			if (mask.at<Vec3b>(bad_x, bad_y)[0] == 0){
				best_dist = -1.0;

				// For every good pixel.
				for (x = 0; x < original.rows; x++){
					for (y = 0; y < original.cols; y++){
						// If it's good.
						if (mask.at<Vec3b>(x, y)[0] != 0){
							dist = 0.0;
							used = 0;

							for (i = -a; i <= a; i++){
								for (j = -a; j <= a; j++){
									use = true;

									// Retrieving a good pixel from a bad pixel area.
									if (inside(bad_x + i, bad_y + j, original.rows, original.cols)){
										if (mask.at<Vec3b>(bad_x + i, bad_y + j)[0] == 0){
											use = false;
										}
										else{
											bad_p = original.at<Vec3b>(bad_x + i, bad_y + j);
										}
									}
									else{
										bad_p[0] = bad_p[1] = bad_p[2] = 0;
									}

									// Retrieving a good pixel from a bad pixel area.
									if (inside(x + i, y + j, original.rows, original.cols)){
										if (mask.at<Vec3b>(x + i, y + j)[0] == 0){
											use = false;
										}
										else{
											p = original.at<Vec3b>(x + i, y + j);
										}
									}
									else{
										p[0] = p[1] = p[2] = 0;
									}

									if (use){
										used++;
										dist += pixel_distance(p, bad_p);
									}
								}
							}

							dist /= (double)used;

							if (best_dist == -1.0 or dist < best_dist){
								best_dist = dist;
								best_p = original.at<Vec3b>(x, y);
							}
						}
					}
				}

				ans.at<Vec3b>(bad_x, bad_y) = best_p;
			}
			else{
				ans.at<Vec3b>(bad_x, bad_y) = original.at<Vec3b>(bad_x, bad_y);
			}
		}
	}

	// Writing image.
	imwrite(argv[2], ans);

	// namedWindow("Display Image", WINDOW_AUTOSIZE);
	// imshow("Display Image", ans);

	// waitKey(0);

	return 0;
}