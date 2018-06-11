/* Victor Forbes - 9293394
   Marcos Camargo - 9278045 */

#include <opencv2/opencv.hpp>
#include <bits/stdc++.h>

using namespace cv;

/* Porcentagem de pixels mínima de uma certa cor para considerar como rabisco. */
#define RATIO 0.01
#define MINIMIUM_FREQUENCY 0
#define MOST_FREQUENT 1

/* Função que retorna a distância entre dois pixels. */
double pixel_distance(Vec3b p1, Vec3b p2){
	double dist;
	int i;

	for (i = 0, dist = 0.0; i < 3; i++){
		dist += ((int)p1[i] - (int)p2[i]) * ((int)p1[i] - (int)p2[i]);
	}

	return sqrt(dist);
}

/* Função que retorna true se o pixel (x, y) está dentro da imagem. */
bool inside(int x, int y, int n, int m){
	return 0 <= x and x < n and 0 <= y and y < m;
}

/* Função que converte um Vec3b para um vector<unsigned char>. */
std::vector<unsigned char> from_vec3b(Vec3b &v){
	std::vector<unsigned char> p;
	int i;

	// Alocando memória para os bytes RGB.
	p.resize(3);

	// Copiando os bytes.
	for (i = 0; i < 3; i++){
		p[i] = v[i];
	}

	return p;
}

/* Função que retorna um map com as frequências de cada pixel RGB. */
std::map<std::vector<unsigned char>, int> rgb_frequency(Mat &img){
	std::map<std::vector<unsigned char>, int> freq;
	int x, y, i;

	// Para cada pixel.
	for (x = 0; x < img.rows; x++){
		for (y = 0; y < img.cols; y++){
			// Incrementando a frequência desse pixel.
			freq[from_vec3b(img.at<Vec3b>(x, y))]++;
		}
	}

	// Retornando o map.
	return freq;
}

/* Função que retorna o pixel em RGB mais frequente. */
std::vector<unsigned char> most_frequent(std::map<std::vector<unsigned char>, int> freq){
	std::map<std::vector<unsigned char>, int>::iterator it;
	std::vector<unsigned char> ans;

	// Inicializando o mais frequente.
	ans = freq.begin()->first;

	// Para cada elemento do map.
	for (it = freq.begin(); it != freq.end(); it++){
		// Se a frequência desse elemento for maior que a do mais frequente, atualize-o.
		if (it->second > freq[ans]){
			ans = it->first;
		}
	}

	// Retornando o mais frequente.
	return ans;
}

/* Função que retorna a máscara contendo apenas o ruído (rabisco). */
Mat extract_mask(Mat &original, int mode){
	std::map<std::vector<unsigned char>, int> freq;
	std::vector<unsigned char> p;
	int threshold, x, y;
	Mat mask;

	// Alocando a máscara.
	mask = Mat(original.rows, original.cols, CV_8UC3);
		
	printf("Counting frequency...\n");
	
	// Contando a frequência das cores.
	freq = rgb_frequency(original);

	printf("Extracting mask...\n");

	if (mode == MOST_FREQUENT){
		p = most_frequent(freq);

		// Para cada pixel (x, y).
		for (x = 0; x < original.rows; x++){
			for (y = 0; y < original.cols; y++){
				// Se a frequência dessa cor RGB for menor ou igual ao threshold, marque com branco. Caso contrário, marque com preto.
				if (from_vec3b(original.at<Vec3b>(x, y)) == p){
					mask.at<Vec3b>(x, y)[0] = mask.at<Vec3b>(x, y)[1] = mask.at<Vec3b>(x, y)[2] = 0;
				}
				else{
					mask.at<Vec3b>(x, y)[0] = mask.at<Vec3b>(x, y)[1] = mask.at<Vec3b>(x, y)[2] = 255;
				}
			}
		}
	}
	else if (mode == MINIMIUM_FREQUENCY){
		threshold = RATIO * (original.rows * original.cols);

		// Para cada pixel (x, y).
		for (x = 0; x < original.rows; x++){
			for (y = 0; y < original.cols; y++){
				// Se a frequência dessa cor RGB for menor ou igual ao threshold, marque com branco. Caso contrário, marque com preto.
				if (freq[from_vec3b(original.at<Vec3b>(x, y))] <= threshold){
					mask.at<Vec3b>(x, y)[0] = mask.at<Vec3b>(x, y)[1] = mask.at<Vec3b>(x, y)[2] = 255;
				}
				else{
					mask.at<Vec3b>(x, y)[0] = mask.at<Vec3b>(x, y)[1] = mask.at<Vec3b>(x, y)[2] = 0;
				}
			}
		}
	}

	return mask;
}

/* Função recursiva (DFS) que preenche a matriz de distâncias até um pixel bom. */
int fill_dist(Mat &mask, std::vector<std::vector<int> > &dist, int x, int y){
	// Casos base.
	if (!inside(x, y, mask.rows, mask.cols) or mask.at<Vec3b>(x, y)[0] != 0){
		return 0;
	}

	// Se essa distância já foi calculada.
	if (dist[x][y] != -1){
		return dist[x][y];
	}

	dist[x][y] = 1 + fill_dist(mask, dist, x - 1, y); // Up.
	dist[x][y] = min(dist[x][y], 1 + fill_dist(mask, dist, x + 1, y)); // Down.
	dist[x][y] = min(dist[x][y], 1 + fill_dist(mask, dist, x, y - 1)); // Left.
	dist[x][y] = min(dist[x][y], 1 + fill_dist(mask, dist, x, y + 1)); // Right.

	return dist[x][y];
}

/* Função que extrai o tamanho ideal da janela para o Brute Force. */
int extract_window_size(Mat &mask){
	std::vector<std::vector<int> > dist;
	int ans, x, y;

	// Alocando matriz de distâncias.
	dist.resize(mask.rows);

	for (x = 0; x < mask.rows; x++){
		dist[x].assign(mask.cols, -1);
	}

	// Inicializando o "raio".
	ans = 0;

	// Para cada pixel (x, y).
	for (x = 0; x < mask.rows; x++){
		for (y = 0; y < mask.cols; y++){
			// Se o pixel (x, y) for um pixel ruim.
			if (mask.at<Vec3b>(x, y)[0] == 0){
				ans = max(ans, fill_dist(mask, dist, x, y));
			}
		}
	}

	// Retornando o "diâmetro" + 1.
	return 2 * ans + 1;
}

/* Função que faz um Brute Force para fazer Inpainting em cada pixel. */
Mat brute_force(Mat &original, Mat &mask){
	int threshold, x, y, bad_x, bad_y, used, k, a, i, j;
	double dist, best_dist;
	Vec3b p, bad_p, best_p;
	bool use;
	Mat ans;

	// Inicializando a imagem final.
	ans = Mat(original.rows, original.cols, CV_8UC3);

	// Obtendo as dimensões da janela ideal para o Brute Force.
	k = extract_window_size(mask);
	a = (k - 1) / 2;

	// Para cada pixel ruim (bad_x, bad_y).
	for (bad_x = 0; bad_x < original.rows; bad_x++){
		printf("Inpainting line %d\n", bad_x);
		
		for (bad_y = 0; bad_y < original.cols; bad_y ++){

			// Se o pixel for ruim.
			if (mask.at<Vec3b>(bad_x, bad_y)[0] == 0){
				// Inicializando a melhor distância como uma distância inválida.
				best_dist = -1.0;

				// Oara cada pixel bom (x, y).
				for (x = 0; x < original.rows; x++){
					for (y = 0; y < original.cols; y++){
						// Se o pixel for bom.
						if (mask.at<Vec3b>(x, y)[0] != 0){
							// Inicializando a distância da janela centrada em (x, y) para a janela centrada em (bad_x, bad_y) com 0.0.
							dist = 0.0;
							used = 0;

							// Percorrendo as janelas (x + i, y + j) e (bad_x + i, bad_y + j).
							for (i = -a; i <= a; i++){
								for (j = -a; j <= a; j++){
									// Flag que dirá se eu vou usar esse pixel para o cálculo da distância ou não.
									use = true;

									// Se o pixel (bad_x + i, bad_y + j) estiver dentro da imagem.
									if (inside(bad_x + i, bad_y + j, original.rows, original.cols)){
										// Se o pixel (bad_x + i, bad_y + j) for ruim, não usaremos ele.
										if (mask.at<Vec3b>(bad_x + i, bad_y + j)[0] == 0){
											use = false;
										}
										else{
											bad_p = original.at<Vec3b>(bad_x + i, bad_y + j);
										}
									}
									else{
										// Se o pixel (bad_x + i, bad_y + j) estiver fora da imagem, consideramos que é um pixel preto.
										bad_p[0] = bad_p[1] = bad_p[2] = 0;
									}

									// Se o pixel (x + i, y + j) estiver dentro da imagem.
									if (inside(x + i, y + j, original.rows, original.cols)){
										// Se o pixel (x + i, y + j) for ruim, não usaremos ele.
										if (mask.at<Vec3b>(x + i, y + j)[0] == 0){
											use = false;
										}
										else{
											p = original.at<Vec3b>(x + i, y + j);
										}
									}
									else{
										// Se o pixel (x + i, y + j) estiver fora da imagem, consideramos que é um pixel preto.
										p[0] = p[1] = p[2] = 0;
									}

									// Se o pudermos calcular a distância entre (bad_x + i, bad_y + j) e (x + i, y + j).
									if (use){
										// Soma a distância entre dois pixels e incrementa o número de pixels usados.
										dist += pixel_distance(p, bad_p);
										used++;
									}
								}
							}

							// Se o número de pixels usados para o cálculo da distância for 0, ignore-o.
							if (used){
								// Normalizando a distância. (Talvez haja uma normalização melhor para usar, pegando apenas janelas que usem pelo menos 50% dos pixels)
								dist /= (double)used;

								// Se o pixel atual tiver uma distância menor do que a menor distância obtida até agora, atualize o melhor pixel.
								if (best_dist == -1.0 or dist < best_dist){
									best_dist = dist;
									best_p = original.at<Vec3b>(x, y);
								}
							}
						}
					}
				}

				// Preenchendo o pixel (bad_x, bad_y) com o pixel (x, y) cuja distância de sua janela K x K é mínima.
				ans.at<Vec3b>(bad_x, bad_y) = best_p;
			}
			else{
				// Se o pixel (bad_x, bad_y) for bom, basta copiar da imagem original para a imagem final.
				ans.at<Vec3b>(bad_x, bad_y) = original.at<Vec3b>(bad_x, bad_y);
			}
		}
	}

	// Retornando a imagem final.
	return ans;
}

int main(int argc, char *argv[]){
	Mat original, ans, mask;

	// Uso incorreto.
	if (argc != 3){
		printf("Usage: ./main <image_in.bmp> <image_out.bmp>\n");
	}

	printf("Reading image...\n");

	// Lendo a imagem.
	original = imread(argv[1], 1);

	// Imagem sem conteúdo.
	if (!original.data){
		printf("No image data.\n");
		return -1;
	}

	// Extraindo a imagem.
	mask = extract_mask(original, MOST_FREQUENT);

	printf("Writing mask...\n");

	// Escrevendo a máscara.
	imwrite("mask.bmp", mask);

	printf("Inpainting image...\n");

	// Roda o algoritmo de brute force.
	ans = brute_force(original, mask);

	// Escrevendo a imagem em um arquivo.
	imwrite(argv[2], ans);

	// Mostra a imagem resultante em uma janela.
	// namedWindow("Display Image", WINDOW_AUTOSIZE);
	// imshow("Display Image", ans);
	// waitKey(0);

	return 0;
}