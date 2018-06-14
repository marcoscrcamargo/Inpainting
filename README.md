<h1 align="center">Inpainting</h1>

# Introdução
*Inpaiting* é o processo de reconstrução digital de partes perdidas ou deteriodadas de imagens ou vídeos.
Neste trabalho são estudadas duas técnicas de *inpaiting* para a remoção de rabiscos inseridos artificialmente em fotos.

# Algoritmos

# Imagens utilizadas
Para testar os algoritmos implementados, foram utilizadas as imagens mostradas abaixo.

<img src="./project/images/deteriorated/dogo1.bmp"  width="200px" alt="dogo1"/>

<img src="./project/images/deteriorated/dogo2.bmp"   width="200px" alt="dogo2"/>

<img src="./project/images/deteriorated/forbes.bmp"   height="200px" alt="forbes"/>

<img src="./project/images/deteriorated/momo.bmp"   width="200px" alt="momo"/>

<img src="./project/images/deteriorated/momo_fino.bmp"   width="200px" alt="momo_fino"/>

# Extração da máscara

Foi implementada a extração automática das máscaras para *inpaiting* das imagens.


|<img src="./project/images/deteriorated/momo.bmp"   width="200px" alt="momo"/>|<img src="./project/images/masks/momo.bmp"   width="200px" alt="momo"/>|
|:-----------------------------------:|:-----------------------------------:|
| Foto deteriorada | Máscara extraída|

# Inpainting das imagens


|<img src="./project/images/deteriorated/forbes.bmp" width="200px" alt="forbes"/>|<img src="./project/images/inpainted/Gerchberg Papoulis/forbes.bmp" width="200px" alt="forbes_gerchberg"/>|<img src="./project/images/inpainted/Local Brute Force/forbes.bmp" width="200px" alt="forbes_examplebf"/>|
|------------|------------|------------|
| Foto deteriorada | Gerchberg Papoulis | Inpainting por exemplos|

# Analise dos resultados

# Próximos Passos



### Autores

| [![victorxjoey](https://avatars1.githubusercontent.com/u/13484548?s=200&v=4)](https://github.com/VictorXjoeY/) |               [![marcoscrcamargo](https://avatars0.githubusercontent.com/u/13886241?s=200&v=4)](https://github.com/marcoscrcamargo/) |
|:-----------------------------------------------------------------------------------------------------------------:|:-------------------------------------------------------------------------------------------------------:|
|[Marcos Cesar Ribeiro de Camargo](https://github.com/marcoscrcamargo/)|[Victor Luiz Roquete Forbes](https://github.com/VictorXjoeY/)|
| 9278045 | 9293394|



