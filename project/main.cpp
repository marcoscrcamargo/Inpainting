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

	// Alocando a máscara com zeros.
	mask = Mat::zeros(original.rows, original.cols, CV_8UC1);
		
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
					mask.at<uchar>(x, y) = 255;
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
				if (freq[from_vec3b(original.at<Vec3b>(x, y))] > threshold){
					mask.at<uchar>(x, y) = 255;
				}
			}
		}
	}

	return mask;
}

/* Função recursiva (DFS) que preenche a matriz de distâncias até um pixel bom. */
int fill_dist(Mat &mask, std::vector<std::vector<int> > &dist, int x, int y){
	// Casos base.
	if (!inside(x, y, mask.rows, mask.cols) or mask.at<uchar>(x, y) == 0){
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
			if (mask.at<uchar>(x, y) != 0){
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
			if (mask.at<uchar>(bad_x, bad_y) != 0){
				// Inicializando a melhor distância como uma distância inválida.
				best_dist = -1.0;

				// Oara cada pixel bom (x, y).
				for (x = 0; x < original.rows; x++){
					for (y = 0; y < original.cols; y++){
						// Se o pixel for bom.
						if (mask.at<uchar>(x, y) == 0){
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
										if (mask.at<uchar>(bad_x + i, bad_y + j) != 0){
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
										if (mask.at<uchar>(x + i, y + j) != 0){
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

/* Extracts a single channel from an image. */
Mat extract_channel(Mat &img, int c){
	Mat channel;
	int x, y;

	// Aloca o canal.
	channel = Mat(img.rows, img.cols, CV_32FC1);

	// Copia pixel por pixel.
	for (x = 0; x < img.rows; x++){
		for (y = 0; y < img.cols; y++){
			channel.at<float>(x, y) = img.at<Vec3b>(x, y)[c];
		}
	}

	// Retorna o canal.
	return channel;
}

/* Função que dá um merge dos canais RGB. */
Mat merge_channels(Mat &c0, Mat &c1, Mat &c2){
	int x, y;
	Mat img;

	img = Mat(c0.rows, c0.cols, CV_8UC3);

	// Copiando pixel por pixel.
	for (x = 0; x < img.rows; x++){
		for (y = 0; y < img.cols; y++){
			img.at<Vec3b>(x, y)[0] = round(c0.at<float>(x, y));
			img.at<Vec3b>(x, y)[1] = round(c1.at<float>(x, y));
			img.at<Vec3b>(x, y)[2] = round(c2.at<float>(x, y));
		}
	}

	return img;
}

/* Função que normaliza uma imagem entre [l, r]. */
void normalize(Mat &img, double l, double r){
	double img_min, img_max;
	int x, y;

	// Recuperando o mínimo e o máximo da imagem.
	minMaxLoc(img, &img_min, &img_max);

	// Normalizando pixel por pixel.
	for (x = 0; x < img.rows; x++){
		for (y = 0; y < img.cols; y++){
			img.at<float>(x, y) = ((img.at<float>(x, y) - img_min) / (img_max - img_min)) * (r - l) + l;
		}
	}
}

/* Algoritmo Gerchberg-Papoulis com filtragem espacial. */
Mat gerchberg_papoulis(Mat &original, Mat &mask, int T){
	Mat M, mean, mask_term_1, mask_term_2;
	double M_max, M_min, G_max, G_min;
	std::vector<Mat> g[3], G[3];
	int k, x, y, c, i;

	// Alocando arrays para as imagens e transformadas de cada iteração.
	for (c = 0; c < 3; c++){
		g[c].resize(T + 1); // Domínio espacial.
		G[c].resize(T + 1); // Domínio da frequência.
	}

	// Obtendo o filtro da média.
	mean = Mat::zeros(original.rows, original.cols, CV_32FC1);

	// Obtendo o tamanho ideal para o filtro.
	k = extract_window_size(mask);

	// Preenchendo o filtro.
	for (x = 0; x < k; x++){
		for (y = 0; y < k; y++){
			mean.at<float>(x, y) = 1.0 / ((double)(k * k));
		}
	}

	// Obtendo o filtro no domínio das frequências.
	dft(mean, mean);

	// Obtendo a máscara no domínio das frequências.
	mask.convertTo(M, CV_32FC1);
	dft(M, M);

	// Alocando termos auxiliares.
	mask_term_1 = Mat(mask.rows, mask.cols, CV_32FC1);
	mask_term_2 = Mat(mask.rows, mask.cols, CV_32FC1);

	// Preenchendo termos auxiliares constantes.
	for (x = 0; x < mask.rows; x++){
		for (y = 0; y < mask.cols; y++){
			mask_term_1.at<float>(x, y) = 1.0 - (double)mask.at<uchar>(x, y) / 255.0;
			mask_term_2.at<float>(x, y) = (double)mask.at<uchar>(x, y) / 255.0;
		}
	}

	// Para cada canal:
	for (c = 0; c < 3; c++){
		g[c][0] = extract_channel(original, c);

		// Para cada iteração.
		for (i = 1; i <= T; i++){
			// Obtendo a transformada da imagem obtida na iteração anterior.
			G[c][i] = Mat(original.rows, original.cols, CV_32FC1);
			dft(g[c][i - 1], G[c][i]);

			// Recuperando o G_max e o M_max.
			minMaxLoc(G[c][i], &G_min, &G_max);
			minMaxLoc(M, &M_min, &M_max);

			// Zerando coeficientes.
			for (x = 0; x < original.rows; x++){
				for (y = 0; y < original.cols; y++){
					if (G[c][i].at<float>(x, y) >= 0.9 * M_max and G[c][i].at<float>(x, y) <= 0.01 * G_max){
						G[c][i].at<float>(x, y) = 0.0;
					}
				}
			}

			// Realizando a convolução com o filtro de média 7x7 e obtendo a parte real da inversa.
			dft(G[c][i].mul(mean), g[c][i], DFT_REAL_OUTPUT);
			// dft(G[c][i].mul(mean), g[c][i], DFT_INVERSE);
			// idft(G[c][i].mul(mean), g[c][i]);

			// Renormalizando a imagem entre 0 e 255.
			normalize(g[c][i], 0, 255);

			// Inserindo pixels conhecidos, um por um.
			/*for (x = 0; x < original.rows; x++){
				for (y = 0; y < original.cols; y++){

				}
			}*/
			
			// Inserindo pixels conhecidos.
			g[c][i] = mask_term_1.mul(g[c][0]) + mask_term_2.mul(g[c][i]);
		}
	}

	return merge_channels(g[0][T], g[1][T], g[2][T]);
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
	// ans = brute_force(original, mask);
	ans = gerchberg_papoulis(original, mask, 20);

	// Escrevendo a imagem em um arquivo.
	imwrite(argv[2], ans);

	// Mostra a imagem resultante em uma janela.
	// namedWindow("Display Image", WINDOW_AUTOSIZE);
	// imshow("Display Image", ans);
	// waitKey(0);

	return 0;
}