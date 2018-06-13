#include "cv.h"
#include "highgui.h"
#include <iostream>
using namespace cv;
using namespace std;

int main(int argc, char ** argv) {
    const char* filename = (argc >= 2) ? argv[1] : "cameraman.tif";

    Mat inputImage = imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
    Mat filter = imread(argv[2], CV_LOAD_IMAGE_GRAYSCALE);
    if (inputImage.empty() || filter.empty())
        return -1;    
    
    // Go float
    Mat fImage;
    inputImage.convertTo(fImage, CV_32F);
    
    Mat fFilter;
    filter.convertTo(fFilter, CV_32F);
    resize(fFilter, fFilter, fImage.size(), 0, 0);
    normalize(fFilter, fFilter, 0, 1, CV_MINMAX);
    
    // FFT
    cout << "Direct transform...\n";
    Mat fourierTransform;
    dft(fImage, fourierTransform, DFT_SCALE|DFT_COMPLEX_OUTPUT);

    Mat planes[] = {Mat_<float>(fFilter), Mat_<float>(fFilter)};
    merge(planes, 2, fImage);

    Mat fourierProc;    
    // Some processing
    mulSpectrums(fourierTransform, fImage, fourierProc, DFT_COMPLEX_OUTPUT);
    //multiply(fourierTransform, fImage, fourierProc);
    
    // IFFT
    cout << "Inverse transform...\n";
    Mat inverseTransform;
    dft(fourierTransform, inverseTransform, DFT_INVERSE|DFT_REAL_OUTPUT);

    Mat inverseTransformProc;
    dft(fourierProc, inverseTransformProc, DFT_INVERSE|DFT_REAL_OUTPUT);

    // Back to 8-bits
    Mat finalImage;
    cout << "Normalization...\n";
    normalize(inverseTransformProc, finalImage, 0, 255, CV_MINMAX);
    inverseTransformProc.convertTo(finalImage, CV_8U);

    Mat finalInverse;
    inverseTransform.convertTo(finalInverse, CV_8U);
    
    imshow("Image", finalInverse);
    imshow("Image Proc", finalImage);
    imwrite("filtered.png", finalImage);
    waitKey();
    return 0;
}
