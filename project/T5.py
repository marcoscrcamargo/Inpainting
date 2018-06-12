# Trabalho 5 - Inpainting
# Victor Forbes - 9293394
# SCC0251 - 2018/1

import numpy as np
import imageio

# Função que retorna o Root Mean Squared Error da imagem A com a imagem B.
def rmse(A, B):
	return np.sqrt(((A - B)**2).sum() / (A.shape[0] * A.shape[1]))

# Função que retorna a imagem f normalizada em [l, r].
def normalize(f, l, r):
	return ((f - f.min()) / (f.max() - f.min())) * (r - l) + l

# Algoritmo Gerchberg-Papoulis com filtragem espacial.
def gerchberg_papoulis(img, mask, T):
	# Alocando arrays para as imagens e transformadas de cada iteração.
	g = [0 for i in range(T + 1)] # Domínio espacial.
	G = [0 for i in range(T + 1)] # Domínio da frequência.

	# Obtendo o filtro da média 7x7 no domínio das frequências.
	mean = np.zeros(img.shape)
	mean[:7, :7] = 1 / (7**2)
	MEAN = np.fft.fft2(mean)

	# Fazendo com que g[0] seja a imagem passada como parâmetro.
	g[0] = img

	# Obtendo a Transformada de Fourier da máscara.
	M = np.fft.fft2(mask)

	# Realizando as T iterações.
	for k in range(1, T + 1):
		# Obtendo a transformada da imagem obtida na iteração anterior.
		G[k] = np.fft.fft2(g[k - 1])

		# Zerando coeficientes.
		G[k][(G[k] >= 0.9 * M.max()) & (G[k] <= 0.01 * G[k].max())] = 0

		# Realizando a convolução com o filtro de média 7x7 e obtendo a parte real da inversa.
		g[k] = np.fft.ifft2(np.multiply(G[k], MEAN)).real

		# Renormalizando a imagem entre 0 e 255.
		g[k] = normalize(g[k], 0, 255)

		# Inserindo pixels conhecidos.
		g[k] = np.multiply(1 - mask / 255, g[0]) + np.multiply(mask / 255, g[k])

	# Retornando o resultado da última iteração.
	return g[T]

def main():
	# Lendo as entradas.
	img_o_filename = input().rstrip() # Lendo nome do arquivo da imagem original.
	img_i_filename = input().rstrip() # Lendo nome do arquivo da imagem deteriorada.
	img_m_filename = input().rstrip() # Lendo nome do arquivo da máscara.
	T = int(input()) # Lendo o número de iterações.

	# Carregando as imagens.
	img_o = imageio.imread(img_o_filename) # Original.
	img_i = imageio.imread(img_i_filename) # Deteriorada.
	img_m = imageio.imread(img_m_filename) # Máscara.

	# Aplicando o algoritmo de Gerchberg-Papoulis.
	img_i = gerchberg_papoulis(img_i, img_m, T)

	# Imprimindo o RMSE da imagem processada com a imagem original.
	print("%.5f" % (rmse(img_i.astype(np.uint8), img_o.astype(np.uint8))))

if __name__ == "__main__":
	main()