<h1 align="center">Inpainting</h1>


# Sumário

# Introdução
*Inpaiting* é o processo de reconstrução digital de partes perdidas ou deterioradas de imagens ou vídeos.
Neste trabalho são estudadas duas técnicas de *inpainting* para a remoção de rabiscos inseridos artificialmente em fotos.


# Amostra de imagens utilizadas

Para testar os algoritmos implementados foram utilizadas imagens com rabiscos inseridos artificialmente. Segue abaixo alguns exemplos das imagens utilizadas no trabalho.


|<img src="./project/images/deteriorated/dogo2.bmp"   width="200px" alt="dogo2"/>|<img src="./project/images/deteriorated/forbes.bmp"   height="200px" alt="forbes"/>|<img src="./project/images/deteriorated/momo_fino.bmp"   width="200px" alt="momo_fino"/>|
|------------|------------|------------|
| Imagem retirada da internet | Foto do Forbes | Foto do prof. Moacir |

# Algoritmos
## Gerchberg Papoulis

## Inpainting por exemplos

# Extração da máscara

Foi implementada a extração automática das máscaras para o *inpainting* das imagens.


|<img src="./project/images/deteriorated/momo.bmp"   width="200px" alt="momo"/>|<img src="./project/images/masks/momo.bmp"   width="200px" alt="momo"/>|
|:-----------------------------------:|:-----------------------------------:|
| Foto deteriorada | Máscara extraída|

# Inpainting das imagens

|<img src="./project/images/deteriorated/forbes.bmp" width="200px" alt="forbes"/>|<img src="./project/images/inpainted/Gerchberg Papoulis/forbes.bmp" width="200px" alt="forbes_gerchberg"/>|<img src="./project/images/inpainted/Local Brute Force/forbes.bmp" width="200px" alt="forbes_examplebf"/>|
|------------|------------|------------|
| Foto deteriorada | Gerchberg Papoulis | Inpainting por exemplos|

# Analise dos resultados

# Próximos Passos

# Instruções para execução do código
A imagem de entrada deve estar na pasta project/images/deteriorated/, a máscara será salva em project/images/masks/ e a imagem de saída na pasta project/images/deteriorated/<inpaiting_algorithm>/.

A compilação do código em C++ foi feita utilizando o cmake com o arquivo CMakeLists.txt dentro da pasta Project, então para gerar o Makefile e compilar o executável é preciso executar os comandos: 

	cd project/
	mkdir build/
	cd build/
	cmake ../
	make


A execução do código em **C++** é feita pelo comando:

	./main <image_in.bmp> <image_out.bmp> <mask_extraction_algorithm> <inpainting_algorithm>


A execução do código em **Python** é feita pelo comando:

	python3 main.py <image_in.bmp> <image_out.bmp> <mask_extraction_algorithm>

O código em python só contém a implementação do algoritmo *Gerchberg Papoulis*, por isso não é necessário escolher o algoritmo de *inpainting*.

Os argumentos dos programas são:

 * <image_in.bmp> - imagem de entrada.
 * <image_out.bmp> - imagem de saída.
 * <mask_extraction_algorithm> - algoritmo de extração da máscara (*most_frequent* ou *minimum_frequency*).
 * <inpainting_algorithm> - algoritmo de *inpainting* (*brute*, *local*, *smart* ou *papoulis*).





### Autores

| [![victorxjoey](https://avatars1.githubusercontent.com/u/13484548?s=200&v=4)](https://github.com/VictorXjoeY/) |               [![marcoscrcamargo](https://avatars0.githubusercontent.com/u/13886241?s=200&v=4)](https://github.com/marcoscrcamargo/) |
|:-----------------------------------------------------------------------------------------------------------------:|:-------------------------------------------------------------------------------------------------------:|
|[Marcos Cesar Ribeiro de Camargo](https://github.com/marcoscrcamargo/)|[Victor Luiz Roquete Forbes](https://github.com/VictorXjoeY/)|
| 9278045 | 9293394|



