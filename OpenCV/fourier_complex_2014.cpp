/* Transformada de Fourier usando OpenCV
 * Moacir Ponti 2014
 */
/*#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"*/
#include "cv.h"
#include "highgui.h"
#include <iostream>
using namespace cv;
using namespace std;

void dft_complex(Mat img, Mat &imdft) {
    Mat padded; // image expanded to optimal size of FFT
    int m = getOptimalDFTSize( img.rows );
    int n = getOptimalDFTSize( img.cols );
    // copy image and fill the border with zero values
    copyMakeBorder (img, padded, 0, m - img.rows, 0, n - img.cols, BORDER_CONSTANT, Scalar::all(0));
    cout << " Criacao de imagem com borda expandida " << endl;
    // O resultado da FFT eh uma matriz complexa
    // O intervalo dinamico da imagem no dominio da frequencia tambem eh bem maior, por isso precisamos de um float
    // cria um plano com a imagem expandida e mais uma matriz
    Mat planes[] = {Mat_<float>(padded), Mat::zeros(padded.size(), CV_32F)};
    // cria uma Matriz com dois planos e armazena em imdft
    merge(planes, 2, imdft);
    cout << " Criacao de matriz complexa" << endl;
    dft(imdft, imdft);
    cout << " Transformada de Fourier" << endl;
} 

void fourierMagnitude(Mat imdft, Mat &magI) {
  
    // cria dois planos
    Mat planes[] = {Mat::zeros(imdft.size(), CV_32F), Mat::zeros(imdft.size(), CV_32F)};
    // separa a matriz complexa em planos
    //   planes[0] = magnitude, planes[1] = fase
    //ou planes[0] = Re(DFT(I)), planes[1] = Im(DFT(I)) 
    split(imdft, planes);

    //a magnitude de uma TF eh M = sqrt( Re(DFT(I))^2 + Im(DFT(I))^2 ) ou:
    magnitude(planes[0], planes[1], planes[0]); // calcula a magnitude e armazena em planes[0]
    magI = planes[0];
    
    cout << " ...obtem magnitude:" << endl; // << magI << endl;
    // o intervalo dinamico da magnitude eh muito grande para exibir na tela
    // por isso eh preciso calcular o logaritmo da magnitude para visualizar
    magI += Scalar::all(1); // garante todo valor diferente de 0
    log (magI, magI);
    cout << " ...calcula log"  << endl; // << magI << endl;;
    
    // para visualizar ainda temos mais dois passos importantes:
    // 1) eliminar as bordas criadas para obter o tamanho otimo
    // 2) rearranjar os quadrantes para exibir no centro as frequencias mais baixas e nas bordas as frequencias mais altas (FFT-Shift)
    magI = magI(Rect(0,0, magI.cols & -2, magI.rows & -2));
    int cx = magI.cols/2;
    int cy = magI.rows/2;
    
    Mat q0(magI, Rect( 0, 0,cx,cy)); //Alto-esquerda
    Mat q1(magI, Rect(cx, 0,cx,cy)); //Alto-direita
    Mat q2(magI, Rect( 0,cy,cx,cy)); //Baixo-esquerda
    Mat q3(magI, Rect(cx,cy,cx,cy)); //Baixo-direita
    
    Mat qswap; // matriz para substituir os quadrantes
    q0.copyTo(qswap);
    q3.copyTo(q0);
    qswap.copyTo(q3);
    
    q1.copyTo(qswap);
    q2.copyTo(q1);
    qswap.copyTo(q2);
    cout << " ...fft shift"  << endl; // << magI << endl;;
    
    // normalizamos os valores para intervalo entre 0 e 1
    normalize(magI, magI, 0, 1, CV_MINMAX);
    cout << " ...normaliza"  << endl; // << magI << endl;;
}

int main(int argc, char ** argv)
{
    const char* filename = argc >=2 ? argv[1] : "lena.jpg";

    Mat I = imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
    if (I.empty())
        return -1;
    
    Mat dftI;
    dft_complex(I, dftI);
    
    Mat magI;
    fourierMagnitude(dftI, magI);

    imshow("Image", I);
    imshow("Power Spectrum (DFT)", magI);
    
    Mat save;
    normalize(magI, save, 0, 255, CV_MINMAX);
    imwrite("DFT.png", save);
    waitKey();
    
    return 0;
}
