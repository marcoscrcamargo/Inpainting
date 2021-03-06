/* Victor Forbes - 9293394
   Marcos Camargo - 9278045 */

#include <opencv2/opencv.hpp>
#include <bits/stdc++.h>

using namespace cv;

/* Parâmetros para a extração de máscaras. */
#define RATIO 0.01 // Porcentagem de pixels mínima de uma certa cor para considerar como rabisco.
#define MAX_HSV_DISTANCE 500 // Distância máxima para considerar um pixel como "penumbra" do outro.
#define MOST_FREQUENT 0
#define MINIMIUM_FREQUENCY 1
#define RED 2

/* Parâmetros do Inpainting. */
#define RADIUS 50 // Raio do Local Brute Force.
#define MAX_WINDOW_DISTANCE 256.0 * sqrt(3.0) // Distância máxima entre duas janelas.
#define WINDOW_DISTANCE_THRESHOLD 5.0 // Distância máxima entre duas janelas para recalcular tudo no Smart Brute Force.
#define WINDOW_LIST_SIZE 50 // Tamanho da lista de janelas.
#define ELITE_SIZE 10 // Quantidade de pixels similares para fazer a média.

/* Caminhos das pastas com as imagens. */
#define ORIGINAL_PATH "./images/original/"
#define DETERIORATED_PATH "./images/deteriorated/"
#define BRUTE_INPAINTED_PATH "./images/inpainted/Brute Force/"
#define BRUTE_DIFFERENCE_PATH "./images/difference/Brute Force/"
#define LOCAL_INPAINTED_PATH "./images/inpainted/Local Brute Force/"
#define LOCAL_DIFFERENCE_PATH "./images/difference/Local Brute Force/"
#define SMART_INPAINTED_PATH "./images/inpainted/Smart Brute Force/"
#define SMART_DIFFERENCE_PATH "./images/difference/Smart Brute Force/"
#define DYNAMIC_INPAINTED_PATH "./images/inpainted/Local Dynamic Brute Force/"
#define DYNAMIC_DIFFERENCE_PATH "./images/difference/Local Dynamic Brute Force/"
#define MASKS_PATH "./images/masks/"

struct KWindow{
	double dist;
	int x, y;

	KWindow(){}

	KWindow(int x, int y, double dist){
		this->x = x;
		this->y = y;
		this->dist = dist;
	}

	bool operator < (const KWindow &b) const{
		return this->dist < b.dist;
	}
};

/* Função que retorna true se o pixel (x, y) está dentro da imagem. */
bool inside(int, int, int, int);

/* Função que retorna a distância entre dois pixels. */
double pixel_distance(Vec3b, Vec3b);

/* Função que retorna true se um pixel é branco e false caso contrário. */
bool white(std::vector<unsigned char>);

/* Função que retorna true se um pixel é totalmente vermelho (255, 0, 0). */
bool red(std::vector<unsigned char>);

/* Função que converte um Vec3b para um vector<unsigned char>. */
std::vector<unsigned char> from_vec3b(Vec3b &);

/* Função que retorna um map com as frequências de cada pixel RGB. */
std::map<std::vector<unsigned char>, int> rgb_frequency(Mat &);

/* Função que retorna o pixel em RGB mais frequente (que não seja branco). */
std::vector<unsigned char> most_frequent(std::map<std::vector<unsigned char>, int>);

/* Função que retorna uma métrica para avaliar se um pixel é a "penumbra" do outro. */
int hsv_distance(Vec3b, Vec3b);

/* Função que preenche a "penumbra" ao redor do risco "duro" com uma Multi-Source BFS. */
void fill_blur(Mat &, Mat &);

/* Função que retorna a máscara contendo apenas o ruído (rabisco). */
Mat extract_mask(Mat &, int);

/* Função que extrai o tamanho ideal da janela para o Brute Force com uma Multi-Source BFS. */
int extract_ideal_window_size(Mat &);

/* Retorna a distância (medida de similaridade) entre duas janelas KxK centradas em (xi, yi) e em (xf, yf). */
double window_distance(Mat &, Mat &, int, int, int, int, int);

/* Função que faz um Brute Force para fazer Inpainting em cada pixel. */
Mat brute_force(Mat &, Mat &);

/* Função que faz um Brute Force Local para fazer Inpainting em cada pixel. */
Mat local_brute_force(Mat &, Mat &);

/* Função que faz um Brute Force mantendo uma lista das janelas mais similares e passando adiante para os pixels vizinhos. */
Mat smart_brute_force(Mat &, Mat &);

/* Função que faz um Brute Force Local com K dinâmico e usando a média entre as melhores janelas. */
Mat local_dynamic_brute_force(Mat &, Mat &);

/* Retorna a média dos pixels da priority_queue */
Vec3b pixel_mean(Mat &, std::priority_queue<KWindow> &);

/* Extracts a single channel from an image. */
Mat extract_channel(Mat &, int);

/* Função que dá um merge dos canais RGB. */
Mat merge_channels(Mat &, Mat &, Mat &);

/* Função que normaliza uma imagem entre [l, r]. */
void normalize(Mat &, double, double);

/* Algoritmo Gerchberg-Papoulis com filtragem espacial. */
Mat gerchberg_papoulis(Mat &, Mat &, int);

/* Função que uma imagem grayscale que representa a diferença entre duas imagens. */
Mat extract_difference(Mat &, Mat &);

/* Função que retorna o RMSE de duas imagens na região da máscara. */
double rmse(Mat &, Mat &, Mat &);

/* Função que retorna por referência os melhores candidatos em um raio definido por RADIUS. */
void get_local_candidates(Mat &, Mat &, int, int, int, std::vector<std::pair<int, int> > &);

/* Depth-First Search para preencher os pixels deteriorados passando adiante uma lista de candidatos. */
void dfs(Mat &deteriorated, Mat &, Mat &, std::vector<std::vector<bool> > &, int, int, int, std::vector<std::pair<int, int> >);

/* Função que atualiza os melhores candidatos por referência em relação a uma certa janela e retorna o melhor pixel para ocupar essa posição. */
Vec3b update_candidates(Mat &, Mat &, int, int, int, std::vector<std::pair<int, int> > &);

bool inside(int x, int y, int n, int m){
	return 0 <= x and x < n and 0 <= y and y < m;
}

double pixel_distance(Vec3b p1, Vec3b p2){
	double dist;
	int i;

	for (i = 0, dist = 0.0; i < 3; i++){
		dist += ((int)p1[i] - (int)p2[i]) * ((int)p1[i] - (int)p2[i]);
	}

	return sqrt(dist);
}

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

bool white(std::vector<unsigned char> p){
	return p[0] >= 250 and p[1] >= 250 and p[2] >= 250;
}

bool red(std::vector<unsigned char> p){
	return p[0] == 0 and p[1] == 0 and p[2] == 255;
}

std::map<std::vector<unsigned char>, int> rgb_frequency(Mat &img){
	std::map<std::vector<unsigned char>, int> freq;
	int x, y;

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

std::vector<unsigned char> most_frequent(std::map<std::vector<unsigned char>, int> freq){
	std::map<std::vector<unsigned char>, int>::iterator it;
	std::vector<unsigned char> ans;

	// Inicializando o mais frequente.
	ans = freq.begin()->first;

	// Para cada elemento do map.
	for (it = freq.begin(); it != freq.end(); it++){
		// Se a frequência desse elemento for maior que a do mais frequente, atualize-o.
		if (!white(it->first) and it->second > freq[ans]){
			ans = it->first;
		}
	}

	// Retornando o mais frequente.
	return ans;
}

int hsv_distance(Vec3b p1, Vec3b p2){
	int d0, d1;

	if (p1[0] < p2[0]){
		d0 = min((int)p2[0] - (int)p1[0], (int)p1[0] + 255 - (int)p2[0]);
	}
	else{
		d0 = min((int)p1[0] - (int)p2[0], (int)p2[0] + 255 - (int)p1[0]);
	}

	if (p1[1] < p2[1]){
		d1 = min((int)p2[1] - (int)p1[1], (int)p1[1] + 255 - (int)p2[1]);
	}
	else{
		d1 = min((int)p1[1] - (int)p2[1], (int)p2[1] + 255 - (int)p1[1]);
	}

	return d0 * d0 + d1 * d1;
}

void fill_blur(Mat &deteriorated, Mat &mask){
	std::vector<std::vector<bool> > seen;
	std::queue<std::pair<int, int> > q;
	Mat deteriorated_hsv;
	int x, y;

	// Transformando para HSV.
	cvtColor(deteriorated, deteriorated_hsv, COLOR_BGR2HSV);

	// Alocando matriz de visitados.
	seen.resize(deteriorated_hsv.rows);

	// Para cada pixel (x, y).
	for (x = 0; x < deteriorated_hsv.rows; x++){
		seen[x].assign(deteriorated_hsv.cols, false);

		for (y = 0; y < deteriorated_hsv.cols; y++){
			if (mask.at<uchar>(x, y) != 0){
				q.push(std::make_pair(x, y));
				seen[x][y] = true;
			}
		}
	}

	// Multi-Source Breadth-First Search a partir dos pixels deteriorados.
	while (!q.empty()){
		x = q.front().first;
		y = q.front().second;
		q.pop();

		// Up.
		if (inside(x - 1, y, mask.rows, mask.cols) and !seen[x - 1][y] and hsv_distance(deteriorated_hsv.at<Vec3b>(x - 1, y), deteriorated_hsv.at<Vec3b>(x, y)) <= MAX_HSV_DISTANCE){ // Up.
			printf("Filling (%d, %d)\n", x - 1, y);
			seen[x - 1][y] = true;
			mask.at<uchar>(x - 1, y) = 255;
			q.push(std::make_pair(x - 1, y));
		}

		// Down.
		if (inside(x + 1, y, mask.rows, mask.cols) and !seen[x + 1][y] and hsv_distance(deteriorated_hsv.at<Vec3b>(x + 1, y), deteriorated_hsv.at<Vec3b>(x, y)) <= MAX_HSV_DISTANCE){ // Down.
			printf("Filling (%d, %d)\n", x + 1, y);
			seen[x + 1][y] = true;
			mask.at<uchar>(x + 1, y) = 255;
			q.push(std::make_pair(x + 1, y));
		}

		// Left.
		if (inside(x, y - 1, mask.rows, mask.cols) and !seen[x][y - 1] and hsv_distance(deteriorated_hsv.at<Vec3b>(x, y - 1), deteriorated_hsv.at<Vec3b>(x, y)) <= MAX_HSV_DISTANCE){ // Left.
			printf("Filling (%d, %d)\n", x, y - 1);
			seen[x][y - 1] = true;
			mask.at<uchar>(x, y - 1) = 255;
			q.push(std::make_pair(x, y - 1));
		}

		// Right.
		if (inside(x, y + 1, mask.rows, mask.cols) and !seen[x][y + 1] and hsv_distance(deteriorated_hsv.at<Vec3b>(x, y + 1), deteriorated_hsv.at<Vec3b>(x, y)) <= MAX_HSV_DISTANCE){ // Right.
			printf("Filling (%d, %d)\n", x, y + 1);
			seen[x][y + 1] = true;
			mask.at<uchar>(x, y + 1) = 255;
			q.push(std::make_pair(x, y + 1));
		}
	}
}

Mat extract_mask(Mat &deteriorated, int mode){
	std::map<std::vector<unsigned char>, int> freq;
	std::vector<unsigned char> p;
	int threshold, det, x, y;
	Mat mask;

	// Alocando a máscara com zeros.
	mask = Mat::zeros(deteriorated.rows, deteriorated.cols, CV_8UC1);
	det = 0;

	if (mode == RED){
		// Para cada pixel (x, y).
		for (x = 0; x < deteriorated.rows; x++){
			for (y = 0; y < deteriorated.cols; y++){
				// Se o pixel for vermelho.
				if (red(from_vec3b(deteriorated.at<Vec3b>(x, y)))){
					mask.at<uchar>(x, y) = 255;
					det++;
				}
			}
		}
	}
	else{
		printf("Counting frequency...\n");
		
		// Contando a frequência das cores.
		freq = rgb_frequency(deteriorated);

		printf("Extracting mask...\n");

		if (mode == MOST_FREQUENT){
			p = most_frequent(freq);

			// Para cada pixel (x, y).
			for (x = 0; x < deteriorated.rows; x++){
				for (y = 0; y < deteriorated.cols; y++){
					// Se a frequência dessa cor RGB for menor ou igual ao threshold, marque com branco. Caso contrário, marque com preto.
					if (from_vec3b(deteriorated.at<Vec3b>(x, y)) == p){
						mask.at<uchar>(x, y) = 255;
						det++;
					}
				}
			}
		}
		else if (mode == MINIMIUM_FREQUENCY){
			threshold = RATIO * (deteriorated.rows * deteriorated.cols);

			// Para cada pixel (x, y).
			for (x = 0; x < deteriorated.rows; x++){
				for (y = 0; y < deteriorated.cols; y++){
					// Se a frequência dessa cor RGB for menor ou igual ao threshold, marque com branco. Caso contrário, marque com preto.
					if (!white(from_vec3b(deteriorated.at<Vec3b>(x, y))) and freq[from_vec3b(deteriorated.at<Vec3b>(x, y))] > threshold){
						mask.at<uchar>(x, y) = 255;
						det++;
					}
				}
			}
		}

		// (A medida de decisão para detectar a penumbra ainda está ruim) Preenchendo a "penumbra" dos rabiscos.
		// fill_blur(deteriorated, mask);
	}

	printf("%d pixels to inpaint!\n", det);

	return mask;
}

std::vector<std::vector<int> > extract_window_sizes(Mat &mask){
	std::vector<std::vector<int> > dist;
	std::queue<std::pair<int, int> > q;
	int x, y;

	// Alocando matriz de distâncias.
	dist.resize(mask.rows);

	// Inicializando.
	for (x = 0; x < mask.rows; x++){
		dist[x].assign(mask.cols, -1);

		for (y = 0; y < mask.cols; y++){
			// Se o pixel (x, y) é bom.
			if (mask.at<uchar>(x, y) == 0){
				dist[x][y] = 0;
				q.push(std::make_pair(x, y));
			}
		}
	}

	// Breadth-First Search.
	while (!q.empty()){
		x = q.front().first;
		y = q.front().second;
		q.pop();

		if (inside(x - 1, y, mask.rows, mask.cols) and dist[x - 1][y] == -1){ // Up.
			dist[x - 1][y] = dist[x][y] + 1;
			q.push(std::make_pair(x - 1, y));
		}

		if (inside(x + 1, y, mask.rows, mask.cols) and dist[x + 1][y] == -1){ // Down.
			dist[x + 1][y] = dist[x][y] + 1;
			q.push(std::make_pair(x + 1, y));
		}

		if (inside(x, y - 1, mask.rows, mask.cols) and dist[x][y - 1] == -1){ // Left.
			dist[x][y - 1] = dist[x][y] + 1;
			q.push(std::make_pair(x, y - 1));
		}

		if (inside(x, y + 1, mask.rows, mask.cols) and dist[x][y + 1] == -1){ // Right.
			dist[x][y + 1] = dist[x][y] + 1;
			q.push(std::make_pair(x, y + 1));
		}
	}

	// Atualizando os valores para representar o "diâmetro" + 3.
	for (x = 0; x < mask.rows; x++){
		for (y = 0; y < mask.cols; y++){
			dist[x][y] = 2 * dist[x][y] + 3;
		}
	}

	return dist;
}

int extract_ideal_window_size(Mat &mask){
	std::vector<std::vector<int> > dist;
	int ans, x, y;

	// Alocando matriz de distâncias e incializando o "raio".
	dist = extract_window_sizes(mask);
	ans = 0;

	// Recuperando a distância máxima.
	for (x = 0; x < mask.rows; x++){
		for (y = 0; y < mask.cols; y++){
			ans = max(ans, dist[x][y]);
		}
	}

	// Retornando o "diâmetro" + 3.
	return ans;
}

double window_distance(Mat &deteriorated, Mat &mask, int xi, int yi, int xf, int yf, int k){
	int used, a, i, j;
	Vec3b pi, pf;
	double dist;
	bool use;

	// Definindo o "raio" da janela K x K.
	a = (k - 1) / 2;
	dist = 0.0;
	used = 0;

	// Percorrendo as janelas (xf + i, yf + j) e (xi + i, yi + j).
	for (i = -a; i <= a; i++){
		for (j = -a; j <= a; j++){
			// Flag que dirá se eu vou usar esse pixel para o cálculo da distância ou não.
			use = true;

			// Se o pixel (xi + i, yi + j) estiver dentro da imagem.
			if (inside(xi + i, yi + j, deteriorated.rows, deteriorated.cols)){
				// Se o pixel (xi + i, yi + j) for ruim, não usaremos ele.
				if (mask.at<uchar>(xi + i, yi + j) != 0){
					use = false;
				}
				else{
					pi = deteriorated.at<Vec3b>(xi + i, yi + j);
				}
			}
			else{
				// Se o pixel (xi + i, yi + j) estiver fora da imagem, consideramos que é um pixel preto.
				pi[0] = pi[1] = pi[2] = 0;
			}

			// Se o pixel (xf + i, yf + j) estiver dentro da imagem.
			if (inside(xf + i, yf + j, deteriorated.rows, deteriorated.cols)){
				// Se o pixel (xf + i, yf + j) for ruim, não usaremos ele.
				if (mask.at<uchar>(xf + i, yf + j) != 0){
					use = false;
				}
				else{
					pf = deteriorated.at<Vec3b>(xf + i, yf + j);
				}
			}
			else{
				// Se o pixel (xf + i, yf + j) estiver fora da imagem, consideramos que é um pixel preto.
				pf[0] = pf[1] = pf[2] = 0;
			}

			// Se o pudermos calcular a distância entre (xi + i, yi + j) e (xf + i, yf + j).
			if (use){
				// Soma a distância entre dois pixels e incrementa o número de pixels usados.
				dist += pixel_distance(pi, pf);
				used++;
			}
		}
	}

	// Se algum pixel foi usado, retorne a distância normalizada.
	if (used){
		return dist / (double)used;
	}

	// Se não, retorne a máxima distância sqrt(3 * 256^2)
	return MAX_WINDOW_DISTANCE;
}

Mat brute_force(Mat &deteriorated, Mat &mask){
	int x, y, bad_x, bad_y, k;
	double dist, best_dist;
	Mat inpainted;

	// Inicializando a imagem final.
	inpainted = Mat(deteriorated.rows, deteriorated.cols, CV_8UC3);

	// Obtendo as dimensões da janela ideal para o Brute Force.
	k = extract_ideal_window_size(mask);

	printf("k = %d\n", k);

	// Para cada pixel ruim (bad_x, bad_y).
	for (bad_x = 0; bad_x < deteriorated.rows; bad_x++){
		printf("Inpainting line %d\n", bad_x);
		
		for (bad_y = 0; bad_y < deteriorated.cols; bad_y++){
			// Se o pixel for ruim.
			if (mask.at<uchar>(bad_x, bad_y) != 0){
				// Inicializando a melhor distância como a distância máxima.
				best_dist = MAX_WINDOW_DISTANCE;

				// Para cada pixel bom (x, y).
				for (x = 0; x < deteriorated.rows; x++){
					for (y = 0; y < deteriorated.cols; y++){
						// Se o pixel for bom.
						if (mask.at<uchar>(x, y) == 0){
							// Inicializando a distância da janela centrada em (x, y) para a janela centrada em (bad_x, bad_y) com 0.0.
							dist = window_distance(deteriorated, mask, bad_x, bad_y, x, y, k);

							// Se o pixel atual tiver uma distância menor do que a menor distância obtida até agora, atualize o melhor pixel.
							if (dist < best_dist){
								// Preenchendo o pixel (bad_x, bad_y) com o pixel (x, y) cuja distância de sua janela K x K é mínima.
								inpainted.at<Vec3b>(bad_x, bad_y) = deteriorated.at<Vec3b>(x, y);
								best_dist = dist;
							}
						}
					}
				}
			}
			else{
				// Se o pixel (bad_x, bad_y) for bom, basta copiar da imagem deteriorated para a imagem final.
				inpainted.at<Vec3b>(bad_x, bad_y) = deteriorated.at<Vec3b>(bad_x, bad_y);
			}
		}
	}

	// Retornando a imagem final.
	return inpainted;
}

Mat local_brute_force(Mat &deteriorated, Mat &mask){
	int x, y, bad_x, bad_y, k;
	double dist, best_dist;
	Mat inpainted;

	// Inicializando a imagem final.
	inpainted = Mat(deteriorated.rows, deteriorated.cols, CV_8UC3);

	// Obtendo as dimensões da janela ideal para o Brute Force.
	k = extract_ideal_window_size(mask);

	printf("k = %d\n", k);

	// Para cada pixel ruim (bad_x, bad_y).
	for (bad_x = 0; bad_x < deteriorated.rows; bad_x++){
		printf("Inpainting line %d\n", bad_x);
		
		for (bad_y = 0; bad_y < deteriorated.cols; bad_y++){
			// Se o pixel for ruim.
			if (mask.at<uchar>(bad_x, bad_y) != 0){
				// Inicializando a melhor distância como a distância máxima.
				best_dist = MAX_WINDOW_DISTANCE;

				// Para cada pixel (x, y) em uma área quadrada definida pelo raio "RADIUS".
				for (x = max(0, bad_x - RADIUS); x < min(deteriorated.rows, bad_x + RADIUS + 1); x++){
					for (y = max(0, bad_y - RADIUS); y < min(deteriorated.cols, bad_y + RADIUS + 1); y++){
						// Se o pixel for bom.
						if (mask.at<uchar>(x, y) == 0){
							// Recuperando a distância entre a janela centrada em (bad_x, bad_y) e a janela centrada em (x, y).
							dist = window_distance(deteriorated, mask, bad_x, bad_y, x, y, k);

							// Se o pixel atual tiver uma distância menor do que a menor distância obtida até agora, atualize o melhor pixel.
							if (dist < best_dist){
								// Preenchendo o pixel (bad_x, bad_y) com o pixel (x, y) cuja distância de sua janela K x K é mínima.
								inpainted.at<Vec3b>(bad_x, bad_y) = deteriorated.at<Vec3b>(x, y);
								best_dist = dist;
							}
						}
					}
				}
			}
			else{
				// Se o pixel (bad_x, bad_y) for bom, basta copiar da imagem deteriorated para a imagem final.
				inpainted.at<Vec3b>(bad_x, bad_y) = deteriorated.at<Vec3b>(bad_x, bad_y);
			}
		}
	}

	// Retornando a imagem final.
	return inpainted;
}

Vec3b pixel_mean(Mat &deteriorated, std::priority_queue<KWindow> &pq){
	double r, g, b;
	int x, y, n;
	Vec3b p;

	b = g = r = 0.0;
	n = (int)pq.size();

	// Para cada pixel.
	while (!pq.empty()){
		x = pq.top().x;
		y = pq.top().y;
		pq.pop();

		// Somando o valor de seus canais.
		b += (double)deteriorated.at<Vec3b>(x, y)[0];
		g += (double)deteriorated.at<Vec3b>(x, y)[1];
		r += (double)deteriorated.at<Vec3b>(x, y)[2];
	}

	// Tirando a média e arredondando.
	p[0] = round(b / (double)n);
	p[1] = round(g / (double)n);
	p[2] = round(r / (double)n);

	return p;
}

Mat local_dynamic_brute_force(Mat &deteriorated, Mat &mask){
	std::vector<std::vector<int> > k;
	std::priority_queue<KWindow> pq;
	int x, y, bad_x, bad_y;
	Mat inpainted;
	double dist;

	// Inicializando a imagem final.
	inpainted = Mat(deteriorated.rows, deteriorated.cols, CV_8UC3);

	// Obtendo as dimensões das janelas ideais para o Brute Force.
	k = extract_window_sizes(mask);

	// Para cada pixel ruim (bad_x, bad_y).
	for (bad_x = 0; bad_x < deteriorated.rows; bad_x++){
		printf("Inpainting line %d\n", bad_x);
		
		for (bad_y = 0; bad_y < deteriorated.cols; bad_y++){
			// Se o pixel for ruim.
			if (mask.at<uchar>(bad_x, bad_y) != 0){
				// Para cada pixel (x, y) em uma área quadrada definida pelo raio "RADIUS".
				for (x = max(0, bad_x - RADIUS); x < min(deteriorated.rows, bad_x + RADIUS + 1); x++){
					for (y = max(0, bad_y - RADIUS); y < min(deteriorated.cols, bad_y + RADIUS + 1); y++){
						// Se o pixel for bom.
						if (mask.at<uchar>(x, y) == 0){
							// Recuperando a distância entre a janela centrada em (bad_x, bad_y) e a janela centrada em (x, y).
							dist = window_distance(deteriorated, mask, bad_x, bad_y, x, y, k[bad_x][bad_y]);

							pq.push(KWindow(x, y, dist));

							// Queremos apenas os ELITE_SIZE melhores.
							if ((int)pq.size() > ELITE_SIZE){
								pq.pop();
							}
						}
					}
				}

				// Atribuindo a média dos ELITE_SIZE melhores.
				inpainted.at<Vec3b>(bad_x, bad_y) = pixel_mean(deteriorated, pq);
			}
			else{
				// Se o pixel (bad_x, bad_y) for bom, basta copiar da imagem deteriorated para a imagem final.
				inpainted.at<Vec3b>(bad_x, bad_y) = deteriorated.at<Vec3b>(bad_x, bad_y);
			}
		}
	}

	// Retornando a imagem final.
	return inpainted;
}

void get_local_candidates(Mat &deteriorated, Mat &mask, int bad_x, int bad_y, int k, std::vector<std::pair<int, int> > &candidates){
	std::priority_queue<KWindow> pq;
	int x, y, i;
	double dist;

	// Para cada pixel (x, y) em uma área quadrada definida pelo raio "RADIUS".
	for (x = max(0, bad_x - RADIUS); x < min(deteriorated.rows, bad_x + RADIUS + 1); x++){
		for (y = max(0, bad_y - RADIUS); y < min(deteriorated.cols, bad_y + RADIUS + 1); y++){
			// Se o pixel for bom.
			if (mask.at<uchar>(x, y) == 0){
				// Recuperando a distância entre a janela centrada em (bad_x, bad_y) e a janela centrada em (x, y).
				dist = window_distance(deteriorated, mask, bad_x, bad_y, x, y, k);

				pq.push(KWindow(x, y, dist));

				// Mantendo apenas as 100 melhores janelas.
				if ((int)pq.size() > WINDOW_LIST_SIZE){
					pq.pop();
				}
			}
		}
	}

	// Colocando os 100 melhores em um vector em ordem crescente de distância.
	for (i = (int)pq.size() - 1; i >= 0; i--){
		candidates[i] = std::make_pair(pq.top().x, pq.top().y);
		pq.pop();
	}
}


Vec3b update_candidates(Mat &deteriorated, Mat &mask, int bad_x, int bad_y, int k, std::vector<std::pair<int, int> > &candidates){
	std::priority_queue<KWindow> pq;
	double dist, b, g, r;
	std::vector<KWindow> elite;
	Vec3b p, ans;
	int i;

	// Buscando o candidato com mínima distância.
	for (i = 0; i < (int)candidates.size(); i++){
		dist = window_distance(deteriorated, mask, bad_x, bad_y, candidates[i].first, candidates[i].second, k);

		pq.push(KWindow(candidates[i].first, candidates[i].second, dist));

		if ((int)pq.size() > ELITE_SIZE){
			pq.pop();
		}
	}

	// Colocando os 5 melhores em "elite".
	elite.resize(ELITE_SIZE);

	for (i = ELITE_SIZE - 1; i >= 0; i--){
		elite[i] = pq.top();
		pq.pop();
	}

	b = g = r = 0.0;

	if (elite[0].dist > WINDOW_DISTANCE_THRESHOLD){
		// A melhor janela não é boa o suficiente, buscando uma melhor.
		get_local_candidates(deteriorated, mask, bad_x, bad_y, k, candidates);
		
		// Média dos 5 melhores candidatos.
		for (i = 0; i < ELITE_SIZE; i++){
			p = deteriorated.at<Vec3b>(candidates[i].first, candidates[i].second);

			b += p[0];
			g += p[1];
			r += p[2];
		}
	}
	else{
		// Média dos 5 melhores candidatos.
		for (i = 0; i < ELITE_SIZE; i++){
			p = deteriorated.at<Vec3b>(elite[i].x, elite[i].y);

			b += p[0];
			g += p[1];
			r += p[2];
		}
	}

	ans[0] = round(b / (double)ELITE_SIZE);
	ans[1] = round(g / (double)ELITE_SIZE);
	ans[2] = round(r / (double)ELITE_SIZE);

	return ans;
}

void dfs(Mat &deteriorated, Mat &inpainted, Mat &mask, std::vector<std::vector<bool> > &seen, int x, int y, int k, std::vector<std::pair<int, int> > candidates){
	inpainted.at<Vec3b>(x, y) = update_candidates(deteriorated, mask, x, y, k, candidates);
	seen[x][y] = true;

	printf("Filled (%d, %d)\n", x, y);

	if (inside(x - 1, y, mask.rows, mask.cols) and !seen[x - 1][y] and mask.at<uchar>(x - 1, y) != 0){ // Up.
		dfs(deteriorated, inpainted, mask, seen, x - 1, y, k, candidates);
	}

	if (inside(x + 1, y, mask.rows, mask.cols) and !seen[x + 1][y] and mask.at<uchar>(x + 1, y) != 0){ // Down.
		dfs(deteriorated, inpainted, mask, seen, x + 1, y, k, candidates);
	}

	if (inside(x, y - 1, mask.rows, mask.cols) and !seen[x][y - 1] and mask.at<uchar>(x, y - 1) != 0){ // Left.
		dfs(deteriorated, inpainted, mask, seen, x, y - 1, k, candidates);
	}

	if (inside(x, y + 1, mask.rows, mask.cols) and !seen[x][y + 1] and mask.at<uchar>(x, y + 1) != 0){ // Right.
		dfs(deteriorated, inpainted, mask, seen, x, y + 1, k, candidates);
	}
}

Mat smart_brute_force(Mat &deteriorated, Mat &mask){
	std::vector<std::pair<int, int> > candidates;
	std::vector<std::vector<bool> > seen;
	int x, y, k, i;
	Mat inpainted;

	// Alocando matriz de posições visitadas.
	seen.resize(deteriorated.rows);

	// Inicializando as posições como não visitadas.
	for (i = 0; i < deteriorated.rows; i++){
		seen[i].assign(deteriorated.cols, false);
	}

	// Inicializando a lista de janelas.
	candidates.assign(WINDOW_LIST_SIZE, std::make_pair(0, 0));

	// Inicializando a imagem final.
	inpainted = Mat(deteriorated.rows, deteriorated.cols, CV_8UC3);

	// Obtendo as dimensões da janela ideal para o Brute Force.
	k = extract_ideal_window_size(mask);

	printf("k = %d\n", k);

	// Para cada pixel ruim (x, y).
	for (x = 0; x < deteriorated.rows; x++){		
		for (y = 0; y < deteriorated.cols; y++){
			// Se o pixel for ruim e ainda não tiver sido visitado.
			if (mask.at<uchar>(x, y) != 0){
				if (!seen[x][y]){
					// Rodando uma DFS a partir de (x, y).
					dfs(deteriorated, inpainted, mask, seen, x, y, k, candidates);
				}
			}
			else{
				// Se o pixel (x, y) for bom, basta copiar da imagem deteriorated para a imagem final.
				inpainted.at<Vec3b>(x, y) = deteriorated.at<Vec3b>(x, y);
			}
		}
	}

	// Retornando a imagem final.
	return inpainted;
}

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

Mat gerchberg_papoulis(Mat &deteriorated, Mat &mask, int T){
	Mat M, mean, mask_term_1, mask_term_2;
	double M_max, M_min, G_max, G_min;
	std::vector<Mat> g[3], G[3];
	int k, x, y, c, i;

	// Obtendo a máscara no domínio das frequências.
	mask.convertTo(M, CV_32FC1);
	dft(M, M);

	// Obtendo o filtro da média.
	mean = Mat::zeros(deteriorated.rows, deteriorated.cols, CV_32FC1);

	// Obtendo o tamanho ideal para o filtro.
	k = extract_ideal_window_size(mask);

	// Preenchendo o filtro.
	for (x = 0; x < k; x++){
		for (y = 0; y < k; y++){
			mean.at<float>(x, y) = 1.0 / ((double)(k * k));
		}
	}

	// Obtendo o filtro no domínio das frequências.
	dft(mean, mean);
	
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
		// Alocando arrays para as imagens e transformadas de cada iteração.
		g[c].resize(T + 1); // Domínio espacial.
		G[c].resize(T + 1); // Domínio da frequência.

		// Extraindo o canal c.
		g[c][0] = extract_channel(deteriorated, c);

		// Para cada iteração.
		for (i = 1; i <= T; i++){
			// Obtendo a transformada da imagem obtida na iteração anterior.
			G[c][i] = Mat(deteriorated.rows, deteriorated.cols, CV_32FC1);
			dft(g[c][i - 1], G[c][i]);

			// Recuperando o G_max e o M_max.
			minMaxLoc(G[c][i], &G_min, &G_max);
			minMaxLoc(M, &M_min, &M_max);

			// Zerando coeficientes.
			for (x = 0; x < deteriorated.rows; x++){
				for (y = 0; y < deteriorated.cols; y++){
					if (G[c][i].at<float>(x, y) >= 0.9 * M_max and G[c][i].at<float>(x, y) <= 0.01 * G_max){
						G[c][i].at<float>(x, y) = 0.0;
					}
				}
			}

			// Realizando a convolução com o filtro de média KxK e obtendo a parte real da inversa.
			// dft(G[c][i].mul(mean), g[c][i], DFT_REAL_OUTPUT);
			// dft(G[c][i].mul(mean), g[c][i], DFT_INVERSE);
			idft(G[c][i].mul(mean), g[c][i]);

			// Renormalizando a imagem entre 0 e 255.
			normalize(g[c][i], 0, 255);

			// Inserindo pixels conhecidos.
			g[c][i] = mask_term_1.mul(g[c][0]) + mask_term_2.mul(g[c][i]);
		}
	}

	return merge_channels(g[0][T], g[1][T], g[2][T]);
}

Mat extract_difference(Mat &original, Mat &inpainted){
	int x, y, c, cur;
	Mat ans;

	// Alocando uma imagem grayscale.
	ans = Mat(original.rows, original.cols, CV_8UC1);

	// Para cada pixel (x, y).
	for (x = 0; x < original.rows; x++){
		for (y = 0; y < original.cols; y++){
			cur = 0;

			// Somando as diferenças absolutas de cada canal de cor.
			for (c = 0; c < 3; c++){
				cur += abs(((int)original.at<Vec3b>(x, y)[c]) - ((int)inpainted.at<Vec3b>(x, y)[c]));
			}

			// Tirando a média e arredondando.
			ans.at<uchar>(x, y) = round((double)cur / 3.0);
		}
	}

	// Retornando a diferença.
	return ans;
}

double rmse(Mat &original, Mat &inpainted, Mat &mask){
	int x, y, c, n;
	double ans;

	ans = 0.0;
	n = 0;

	// Para cada pixel (x, y).
	for (x = 0; x < original.rows; x++){
		for (y = 0; y < original.cols; y++){
			// Se estiver deteriorado.
			if (mask.at<uchar>(x, y) != 0){
				for (c = 0; c < 3; c++){
					ans += (double)(original.at<Vec3b>(x, y)[c] - inpainted.at<Vec3b>(x, y)[c]) * (original.at<Vec3b>(x, y)[c] - inpainted.at<Vec3b>(x, y)[c]);
					n++;
				}
			}
		}
	}

	// Retornando o RMSE.
	return sqrt(ans / (double)n);
}

int main(int argc, char *argv[]){
	Mat original, deteriorated, inpainted, difference, mask;

	// Uso incorreto.
	if (argc != 5 and argc != 6){
		printf("Compare usage: ./main compare <path/original.bmp> <path/inpainted.bmp> <path/mask.bmp>\n");
		printf("Inpainting usage: ./main <image_in.bmp> <image_out.bmp> <mask_extraction_algorithm> <inpainting_algorithm> (compare)?\n");
		printf("<mask_extraction_algorithm> - {most_frequent, minimum_frequency, red}\n");
		printf("<inpainting_algorithm> - {brute, local, smart, dynamic}\n");
		return -1;
	}

	// ./main compare <path/original.bmp> <path/inpainted.bmp> <path/mask.bmp> <path/diff.bmp>
	if (argc == 6 and !strcmp(argv[1], "compare")){ // Apenas compara duas imagens.
		// Lendo as imagens a serem comparadas.
		printf("Reading original image from: %s\n", argv[2]);
		original = imread(argv[2], 1);
		printf("Reading inpainted image from: %s\n", argv[3]);
		inpainted = imread(argv[3], 1);
		printf("Reading mask from: %s\n", argv[4]);
		mask = imread(argv[4], 0);

		// Sem imagem.
		if (!original.data or !inpainted.data or !mask.data){
			printf("No image data.");
			return -1;
		}

		// Imprimindo o RMSE apenas na região da máscara.
		printf("RMSE = %.3lf\n", rmse(original, inpainted, mask));

		// Extraindo a diferença entre a imagem original e a reconstruída.
		difference = extract_difference(original, inpainted);

		// Escrevendo a diferença em um arquivo.
		printf("Writing difference to: %s\n", argv[5]);
		imwrite(argv[5], difference);

		return 0;
	}

	// Lendo a imagem original e a imagem deteriorada.
	printf("Reading original image from: %s\n", (ORIGINAL_PATH + std::string(argv[1])).c_str());
	original = imread(ORIGINAL_PATH + std::string(argv[1]), 1);
	printf("Reading deteriorated image from: %s\n", (DETERIORATED_PATH + std::string(argv[1])).c_str());
	deteriorated = imread(DETERIORATED_PATH + std::string(argv[1]), 1);

	// Sem imagem.
	if (!deteriorated.data){
		printf("No image data.\n");
		return -1;
	}

	// Extraindo a máscara.
	if (!strcmp(argv[3], "most_frequent")){
		mask = extract_mask(deteriorated, MOST_FREQUENT);
	}
	else if (!strcmp(argv[3], "minimum_frequency")){
		mask = extract_mask(deteriorated, MINIMIUM_FREQUENCY);
	}
	else if (!strcmp(argv[3], "red")){
		mask = extract_mask(deteriorated, RED);
	}
	else{
		printf("There's no %s mask extraction algorithm\n", argv[3]);
		return -1;
	}

	// Escrevendo a máscara.
	printf("Writing mask to: %s\n", (MASKS_PATH + std::string(argv[2])).c_str());
	imwrite(MASKS_PATH + std::string(argv[2]), mask);

	printf("Inpainting image...\n");

	// Fazendo o inpainting.
	if (!strcmp(argv[4], "brute")){
		// Roda o Brute Force e salva na pasta correspondente.
		inpainted = brute_force(deteriorated, mask);
		printf("Writing inpainted image by Brute Force to: %s\n", (BRUTE_INPAINTED_PATH + std::string(argv[2])).c_str());
		imwrite(BRUTE_INPAINTED_PATH + std::string(argv[2]), inpainted);

		// Faz uma comparação da imagem original com a reconstruída.
		if (argc == 6 and !strcmp(argv[5], "compare") and original.data){
			// Imprimindo o RMSE apenas na região da máscara.
			printf("RMSE = %.3lf\n", rmse(original, inpainted, mask));

			// Extraindo a diferença entre a imagem original e a reconstruída.
			difference = extract_difference(original, inpainted);

			// Escrevendo a diferença em um arquivo.
			printf("Writing difference to: %s\n", (BRUTE_DIFFERENCE_PATH + std::string(argv[2])).c_str());
			imwrite(BRUTE_DIFFERENCE_PATH + std::string(argv[2]), difference);
		}
	}
	else if (!strcmp(argv[4], "local")){
		// Roda o Local Brute Force e salva na pasta correspondente.
		inpainted = local_brute_force(deteriorated, mask);
		printf("Writing inpainted image by Local Brute Force to: %s\n", (LOCAL_INPAINTED_PATH + std::string(argv[2])).c_str());
		imwrite(LOCAL_INPAINTED_PATH + std::string(argv[2]), inpainted);

		// Faz uma comparação da imagem original com a reconstruída.
		if (argc == 6 and !strcmp(argv[5], "compare") and original.data){
			// Imprimindo o RMSE apenas na região da máscara.
			printf("RMSE = %.3lf\n", rmse(original, inpainted, mask));

			// Extraindo a diferença entre a imagem original e a reconstruída.
			difference = extract_difference(original, inpainted);

			// Escrevendo a diferença em um arquivo.
			printf("Writing difference to: %s\n", (LOCAL_DIFFERENCE_PATH + std::string(argv[2])).c_str());
			imwrite(LOCAL_DIFFERENCE_PATH + std::string(argv[2]), difference);
		}
	}
	else if (!strcmp(argv[4], "smart")){
		// Roda o Local Brute Force e salva na pasta correspondente.
		inpainted = smart_brute_force(deteriorated, mask);
		printf("Writing inpainted image by Smart Brute Force to: %s\n", (SMART_INPAINTED_PATH + std::string(argv[2])).c_str());
		imwrite(SMART_INPAINTED_PATH + std::string(argv[2]), inpainted);

		// Faz uma comparação da imagem original com a reconstruída.
		if (argc == 6 and !strcmp(argv[5], "compare")  and original.data){
			// Imprimindo o RMSE apenas na região da máscara.
			printf("RMSE = %.3lf\n", rmse(original, inpainted, mask));

			// Extraindo a diferença entre a imagem original e a reconstruída.
			difference = extract_difference(original, inpainted);

			// Escrevendo a diferença em um arquivo.
			printf("Writing difference to: %s\n", (SMART_DIFFERENCE_PATH + std::string(argv[2])).c_str());
			imwrite(SMART_DIFFERENCE_PATH + std::string(argv[2]), difference);
		}
	}
	else if (!strcmp(argv[4], "dynamic")){
		// Roda o Local Brute Force e salva na pasta correspondente.
		inpainted = local_dynamic_brute_force(deteriorated, mask);
		printf("Writing inpainted image by Local Dynamic Brute Force to: %s\n", (DYNAMIC_INPAINTED_PATH + std::string(argv[2])).c_str());
		imwrite(DYNAMIC_INPAINTED_PATH + std::string(argv[2]), inpainted);

		// Faz uma comparação da imagem original com a reconstruída.
		if (argc == 6 and !strcmp(argv[5], "compare")  and original.data){
			// Imprimindo o RMSE apenas na região da máscara.
			printf("RMSE = %.3lf\n", rmse(original, inpainted, mask));

			// Extraindo a diferença entre a imagem original e a reconstruída.
			difference = extract_difference(original, inpainted);

			// Escrevendo a diferença em um arquivo.
			printf("Writing difference to: %s\n", (DYNAMIC_DIFFERENCE_PATH + std::string(argv[2])).c_str());
			imwrite(DYNAMIC_DIFFERENCE_PATH + std::string(argv[2]), difference);
		}
	}
	else if (!strcmp(argv[4], "papoulis")){
		printf("Not fixed yet\n");
		// inpainted = gerchberg_papoulis(deteriorated, mask, 10);
		return -1;
	}
	else{
		printf("There's no %s inpainting algorithm\n", argv[4]);
		return -1;
	}

	return 0;
}