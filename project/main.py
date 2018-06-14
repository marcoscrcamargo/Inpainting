# Victor Forbes - 9293394
# Marcos Camargo - 9278045

import numpy as np
import imageio
import sys

RATIO = 0.01
MOST_FREQUENT = 0
MINIMUM_FREQUENCY = 1
ORIGINAL_PATH = "./images/original/"
DETERIORATED_PATH = "./images/deteriorated/"
INPAINTED_PATH = "./images/inpainted/Gerchberg Papoulis/"
MASKS_PATH = "./images/masks/"

# Função que retorna se o pixel é branco.
def white(p):
	return p[0] >= 250 and p[1] >= 250 and p[2] >= 250

# Retorna a frequência das cores de uma imagem.
def rgb_frequency(f):
	freq = {}

	# Counting frequency.
	for x in range(f.shape[0]):
		for y in range(f.shape[1]):
			rgb = (f[x][y][0], f[x][y][1], f[x][y][2])

			if rgb in freq:
				freq[rgb] += 1
			else:
				freq[rgb] = 1
	
	return freq

# Retorna a cor mais frequente.
def most_frequent(freq):
	ans = None

	for rgb in freq:
		if not white(rgb) and (ans is None or freq[rgb] > freq[ans]):
			ans = rgb

	return ans

# Extrai a máscara a partir de uma imagem rabiscada.
def extract_mask(f, mode):
	print("Counting frequency...")

	# Contando a frequência de cores da imagem.
	freq = rgb_frequency(f)

	print("Painting mask...")

	# Alocando a máscara.
	mask = np.zeros((f.shape[0], f.shape[1]), np.uint8)
	
	if mode == MOST_FREQUENT:
		# Recuperando a cor mais frequente.
		p = most_frequent(freq)

		# Pintando.
		for x in range(f.shape[0]):
			for y in range(f.shape[1]):
				rgb = (f[x][y][0], f[x][y][1], f[x][y][2])

				# Pinta se o pixel for da cor mais frequente.
				if rgb == p:
					mask[x][y] = 255

	elif mode == MINIMUM_FREQUENCY:
		threshold = int(RATIO * (f.shape[0] * f.shape[1]))

		# Painting.
		for x in range(f.shape[0]):
			for y in range(f.shape[1]):
				rgb = (f[x][y][0], f[x][y][1], f[x][y][2])

				# Pinta se a frequência da cor desse pixel for muito alta.
				if not white(rgb) and freq[rgb] > threshold:
					mask[x][y] = 255

	return mask

# Função que retorna a imagem f normalizada em [l, r].
def normalize(f, l, r):
	return ((f - f.min()) / (f.max() - f.min())) * (r - l) + l

# Função que retorna True se (x, y) estiver dentro da imagem f.
def inside(x, y, f):
	return 0 <= x and x < f.shape[0] and 0 <= y and y < f.shape[1]

# Retorna o tamanho ideal da janela para o filtro da média.
def extract_window_size(mask):
	# Alocando matriz de distâncias.
	dist = np.array([[0 if mask[i][j] == 0 else mask.shape[0] * mask.shape[1] for j in range(mask.shape[1])] for i in range(mask.shape[0])])
	changes = 1

	# (Bellman-Ford) Enquanto não convergir.
	while changes > 0:
		changes = 0

		# Relaxando as arestas.
		for x in range(mask.shape[0]):
			for y in range(mask.shape[1]):
				if inside(x - 1, y, mask) and dist[x - 1][y] + 1 < dist[x][y]: # Up.
					dist[x][y] = dist[x - 1][y] + 1;
					changes += 1;

				if inside(x + 1, y, mask) and dist[x + 1][y] + 1 < dist[x][y]: # Down.
					dist[x][y] = dist[x + 1][y] + 1;
					changes += 1;

				if inside(x, y - 1, mask) and dist[x][y - 1] + 1 < dist[x][y]: # Left.
					dist[x][y] = dist[x][y - 1] + 1;
					changes += 1;

				if inside(x, y + 1, mask) and dist[x][y + 1] + 1 < dist[x][y]: # Right.
					dist[x][y] = dist[x][y + 1] + 1;
					changes += 1;

	# Retornando o "diâmetro" + 1.
	return 2 * dist.max() + 1

# Algoritmo Gerchberg-Papoulis com filtragem espacial.
def gerchberg_papoulis(img, mask, T):
	# Alocando arrays para as imagens e transformadas de cada iteração.
	g = [0 for i in range(T + 1)] # Domínio espacial.
	G = [0 for i in range(T + 1)] # Domínio da frequência.

	print("Extracting ideal k...")

	# Extraindo o k ideal.
	k = extract_window_size(mask)

	print("k =", k)
	print("Inpainting...")

	# Obtendo o filtro da média kxk no domínio das frequências.
	mean = np.zeros((img.shape[0], img.shape[1]))
	mean[:k, :k] = 1 / (k**2)
	mean = np.fft.fft2(mean)

	# Obtendo a Transformada de Fourier da máscara.
	M = np.fft.fft2(mask)

	# Alocando memória para a imagem final.
	ans = np.empty(img.shape)

	# Para cada canal de cor.
	for c in range(3):
		# Fazendo com que g[0] seja a imagem passada como parâmetro.
		g[0] = img[:,:,c]

		# Realizando as T iterações.
		for k in range(1, T + 1):
			# Obtendo a transformada da imagem obtida na iteração anterior.
			G[k] = np.fft.fft2(g[k - 1])

			# Zerando coeficientes.
			G[k][(G[k] >= 0.9 * M.max()) & (G[k] <= 0.01 * G[k].max())] = 0

			# Realizando a convolução com o filtro de média 7x7 e obtendo a parte real da inversa.
			g[k] = np.fft.ifft2(np.multiply(G[k], mean)).real

			# Renormalizando a imagem entre 0 e 255.
			g[k] = normalize(g[k], 0, 255)

			# Inserindo pixels conhecidos.
			g[k] = np.multiply(1 - mask / 255, g[0]) + np.multiply(mask / 255, g[k])

		# Concatenando o canal.
		ans[:,:,c] = g[T]

	# Retornando o resultado da última iteração.
	return np.round(ans).astype(np.uint8)

def main():
	# Wrong usage.
	if len(sys.argv) != 4:
		print("Usage: python3 main.py <image_in.bmp> <image_out.bmp> <mask_extraction_algorithm>")
		print("<mask_extraction_algorithm> - {most_frequent, minimum_frequency}");

	# Obtendo os nomes dos arquivos de entrada e saída.
	filename_in, filename_out = (sys.argv[1], sys.argv[2])

	print("Reading image...")

	# Lendo a imagem.
	f = imageio.imread(DETERIORATED_PATH + filename_in)

	# Setando o limite da recursão.
	sys.setrecursionlimit(f.shape[0] * f.shape[1] + 10)

	# Extraindo a máscara.
	if sys.argv[3] == "most_frequent":
		mask = extract_mask(f, MOST_FREQUENT)
	elif sys.argv[3] == "minimum_frequency":
		mask = extract_mask(f, MINIMUM_FREQUENCY)
	else:
		assert(False)

	print("Writing mask...")

	# Escrevendo a máscara em um arquivo.
	imageio.imwrite(MASKS_PATH + filename_out, mask)

	# Aplicando o algoritmo de Gerchberg-Papoulis
	ans = gerchberg_papoulis(f, mask, 10)

	print("Writing image...")

	# Escrevendo a imagem resultante em um arquivo.
	imageio.imwrite(INPAINTED_PATH + filename_out, ans)

if __name__ == "__main__":
	main()