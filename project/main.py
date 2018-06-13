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
INPAINTED_PATH = "./images/inpainted/"
MASKS_PATH = "./images/masks/"

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
		if ans is None or freq[rgb] > freq[ans]:
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
				if freq[rgb] > threshold:
					mask[x][y] = 255

	return mask

# Função que retorna a imagem f normalizada em [l, r].
def normalize(f, l, r):
	return ((f - f.min()) / (f.max() - f.min())) * (r - l) + l

# Algoritmo Gerchberg-Papoulis com filtragem espacial.
def gerchberg_papoulis(img, mask, T):
	# Alocando arrays para as imagens e transformadas de cada iteração.
	g = [0 for i in range(T + 1)] # Domínio espacial.
	G = [0 for i in range(T + 1)] # Domínio da frequência.

	# Obtendo o filtro da média 7x7 no domínio das frequências.
	# COLOCAR O EXTRACT WINDOW SIZE AQUI PRA DETERMINAR O K.
	mean = np.zeros((img.shape[0], img.shape[1]))
	mean[:7, :7] = 1 / (7**2)
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
	# Lendo os nomes dos arquivos de entrada e saída.
	filename_in = sys.argv[1]
	filename_out = sys.argv[2]

	print("Reading image...")

	# Lendo a imagem.
	f = imageio.imread(DETERIORATED_PATH + filename_in)

	# Extraindo a máscara.
	mask = extract_mask(f, MOST_FREQUENT)

	print("Writing mask...")

	# Escrevendo a máscara em um arquivo.
	imageio.imwrite(MASKS_PATH + filename_out, mask)

	print("Inpainting...")

	# Aplicando o algoritmo de Gerchberg-Papoulis
	ans = gerchberg_papoulis(f, mask, 10)

	print("Writing image...")

	# Escrevendo a imagem resultante em um arquivo.
	imageio.imwrite(INPAINTED_PATH + filename_out, ans)

if __name__ == "__main__":
	main()