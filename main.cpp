#include <iostream>
#include <string>
#include <chrono>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>

extern "C"
{
#include "vc.h"
}

// Defenir array com os valores do HSV
HSV hsv_values[10] = {
    {0, 360, 0, 15, 20, 30, 15, 15, 30},    // Preto -> 0
    {15, 25, 40, 56, 30, 50, 15, 15, 30},  // Castanho -> 1
    {340, 14, 60, 90, 67, 100, 11, 11, 15}, // Vermelho -> 2
    {25, 26, 70, 75, 80, 85, -1, -1, -1},  // Laranja -> 3
    {64, 67, 100, 102, 100, 102, -1, -1, -1},  // Amarelo -> 4
    {100, 165, 30, 100, 30, 100, 19, 19, 31}, // Verde -> 5
    {165, 255, 15, 100, 35, 100, 5, 5, 10}, // Azul -> 6
    {270, 320, 30, 100, 30, 100, -1, -1, -1}, // Roxo -> 7
    {75, 80, 8, 10, 17, 30, -1, -1, -1},    // Cinza -> 8
    {0, 360, 0, 0, 100, 100, -1, -1, -1}   // Branco -> 9
};

int hsvColorsInt = sizeof(hsv_values) / sizeof(HSV);

void opencvClose(IVC *image, IVC *imageClosed, int erosao, int dilatacao) {
	// Cria a imagem no opencv com base na imagem IVC
    cv::Mat opencvSegmentation(image->height, image->width, CV_8UC1, image->data);

    // Criar o trashold para a imagem
    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(erosao, dilatacao));

    // Aplicar a operação de close do opencv
    cv::Mat opencvClose;
    cv::morphologyEx(opencvSegmentation, opencvClose, cv::MORPH_CLOSE, element);

    // Copiar a imagem gerada pelo close para a imagem IVC
    memcpy(imageClosed->data, opencvClose.data, image->width * image->height);
}

void vc_timer(void) {
	static bool running = false;
	static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

	if (!running) {
		running = true;
	} else {
		std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration elapsedTime = currentTime - previousTime;

		// Tempo em segundos.
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime);
		double nseconds = time_span.count();

		std::cout << "Tempo decorrido: " << nseconds << "segundos" << std::endl;
		std::cout << "Pressione qualquer tecla para continuar...\n";
		std::cin.get();
	}
}

int main(void)
{
	// V deo
	char videofile[100] = "../../video_resistors.mp4"; // Trocar para o caminho do video
	cv::VideoCapture capture;
	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;
	// Outros
	std::string str;
	int key = 0;

	/* Leitura de v deo de um ficheiro */
	/* NOTA IMPORTANTE:
	O ficheiro video.avi dever  estar localizado no mesmo direct rio que o ficheiro de c digo fonte.
	*/
	capture.open(videofile);

	/* Em alternativa, abrir captura de v deo pela Webcam #0 */
	// capture.open(0, cv::CAP_DSHOW); // Pode-se utilizar apenas capture.open(0);

	/* Verifica se foi poss vel abrir o ficheiro de v deo */
	if (!capture.isOpened()) {
		std::cerr << "Erro ao abrir o ficheiro de v deo!\n";
		return 1;
	}

	/* N mero total de frames no v deo */
	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	/* Frame rate do v deo */
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	/* Resolu  o do v deo */
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	/* Cria uma janela para exibir o v deo */
	cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

	/* Inicia o timer */
	vc_timer();

	cv::Mat frame;
	IVC *imageOutput = vc_image_new(video.width, video.height, 3, 255);
	IVC *imageRGB = vc_image_new(video.width, video.height, 3, 255);
	IVC *imageSegmented = vc_image_new(video.width, video.height, 1, 255);
	IVC *imageTemp = vc_image_new(video.width, video.height, 1, 255);
	IVC *imageClosed = vc_image_new(video.width, video.height, 1, 255);
	IVC *imageBlobs = vc_image_new(video.width, video.height, 1, 255);
	int foundPixeis = 0;
	int countColors = 0;
	CoresEncontradas colors[4];
	// Limpar array de cores encontradas para garantir que não tem lixo
	for (int i = 0; i < 4; i++) {
		colors[i].color = INT_MAX;
		colors[i].x = INT_MAX;
	}

	// Lista para armazenar as resistências encontradas
	std::vector<ResistenciasEncontradas> resistenciasEncontradas;

	int hsv_count = sizeof(hsv_values) / sizeof(HSV);

	while (key != 'q') {
		/* Leitura de uma frame do vídeo */
		capture.read(frame);

		/* Verifica se conseguiu ler a frame */
		if (frame.empty()) break;

		/* Número da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		// Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
		memcpy(imageOutput->data, frame.data, video.width * video.height * 3);

		// Converter a imagem de BGR para RGB
		vc_convert_bgr_to_rgb(imageOutput, imageRGB);

		// Setar o found a 0
		foundPixeis = 0;

		// Segmentar a imagem para obter o corpo das resistências
		vc_hsv_segmentation(imageRGB, imageSegmented, 30, 40, 30, 100, 45, 100, &foundPixeis);

		// // Se encontrou pixeis, segmentar pelas cores das riscas das resistências
		if (foundPixeis == 1) {
			for (int i = 0; i < hsv_count; i++) {
				// Se o hsv tiver erosão ou dilatação com -1 é porque não aparece no video
				if (hsv_values[i].erosao == -1 || hsv_values[i].dilatacao == -1) {
					continue;
				}

				foundPixeis = 0;
				vc_hsv_segmentation(imageRGB, imageTemp, hsv_values[i].hmin, hsv_values[i].hmax, hsv_values[i].smin, hsv_values[i].smax, hsv_values[i].vmin, hsv_values[i].vmax, &foundPixeis);
				
				// Se encontrou pixeis, adicionar ao imageSegmented
				if (foundPixeis == 1) {
					for (int y = 0; y < imageOutput->height; y++) {
						for (int x = 0; x < imageOutput->width; x++) {
							if (imageTemp->data[y * imageTemp->width + x] == 255) {
								imageSegmented->data[y * imageSegmented->width + x] = 255;
							}
						}
					}
				}
			}

			// Dilatar e erodir a imagem para remover o espaço em branco por causa das linhas a cor da resistência
			// Usada a função de dilatação do opencv por ser mais eficiente quando tem valores/imagens grandes
			opencvClose(imageSegmented, imageClosed, 19, 19);

			// Obter os blobs das resistências
			int nblobs;
			OVC *blobs = vc_binary_blob_labelling(imageClosed, imageBlobs, &nblobs);

			// Se encontrou blobs
			if (blobs != NULL) {
				// Obter informação dos blobs
				vc_binary_blob_info(imageBlobs, blobs, nblobs);

				// Percorrer os blobs
				for (int i = 0; i < nblobs; i++) {
					// Se o blob estiver a menos de 8% do início e 10% do fim da imagem, ignorar para garantir que a resistência está toda na imagem
					if (blobs[i].y < 0.08 * imageBlobs->height || blobs[i].y > 0.90 * imageBlobs->height) {
						continue;
					}

					//Se a área do blob não estiver entre 4800 e 9500, ignorar porque não é uma resistencia
					if (blobs[i].area < 4800 || blobs[i].area > 9500) {
						continue;
					}

					// Pegar na imagem original do blob (Recortar a imagem do blob)
					IVC *imageBlob = vc_image_new(blobs[i].width, blobs[i].height, 3, 255);
					IVC *imageBlobSegmentation = vc_image_new(blobs[i].width, blobs[i].height, 1, 255);
					IVC *imageBlobsClosed = vc_image_new(blobs[i].width, blobs[i].height, 1, 255);
					IVC *imageBlobsBlobs = vc_image_new(blobs[i].width, blobs[i].height, 1, 255);

					// Copiar o blob para a nova imagem
					for (int y = 0; y < blobs[i].height; y++) {
						for (int x = 0; x < blobs[i].width; x++) {
							for (int band = 0; band < 3; band++) {
								imageBlob->data[(y * imageBlob->width + x) * 3 + band] =
									imageRGB->data[((blobs[i].y + y) * imageRGB->width + (blobs[i].x + x)) * 3 + band];
							}
						}
					}

					// Variáveis para guardar as cores encontradas para depois gerar o valor da resistência
					countColors = 0;
					// Limpar array de cores encontradas para garantir que não tem lixo
					for (int i = 0; i < 4; i++) {
						colors[i].color = INT_MAX;
						colors[i].x = INT_MAX;
					}

					// Percorrer as cores do HSV
					for (int j = 0; j < hsv_count; j++) {
						// Se o hsv tiver erosão ou dilatação com -1 é porque não aparece no video
						if (hsv_values[j].erosao == -1 || hsv_values[j].dilatacao == -1) {
							continue;
						}

						foundPixeis = 0;
						vc_hsv_segmentation(imageBlob, imageBlobSegmentation, hsv_values[j].hmin, hsv_values[j].hmax, hsv_values[j].smin, hsv_values[j].smax, hsv_values[j].vmin, hsv_values[j].vmax, &foundPixeis);

						// Se não encontrou pixeis, passar para a próxima cor
						if (foundPixeis == 0) {
							continue;
						}

						// Fazer close na imagem
						opencvClose(imageBlobSegmentation, imageBlobsClosed, hsv_values[j].erosao, hsv_values[j].dilatacao);

						// Obter os blobs das resistências
						int nblobsBlob;
						OVC *blobsBlob = vc_binary_blob_labelling(imageBlobsClosed, imageBlobsBlobs, &nblobsBlob);

						// Se encontrou blobs
						if (blobsBlob != NULL) {
							// Obter informação dos blobs
							vc_binary_blob_info(imageBlobsBlobs, blobsBlob, nblobsBlob);

							// Percorrer os blobs
							for (int k = 0; k < nblobsBlob; k++) {
								if (blobsBlob[k].area < hsv_values[j].minBlobArea) {
									continue;
								}

								colors[countColors].color = j;
								colors[countColors].x = blobsBlob[k].xc;
								countColors++;
                    		}

							// Libertar a memória dos blobs
							free(blobsBlob);
						}

						// Se encontrou 4 cores ou mais, sair do loop
						if (countColors >= 4) {
							break;
						}
					}

					// Validar se tem 3 ou 4 cores para calcular a resistência
					if (countColors == 3 || countColors == 4) {
						// Ordenar as cores de forma crescente
						std::sort(colors, colors + 4, [](CoresEncontradas &a, CoresEncontradas &b) { 
							return a.x < b.x;
						});

						// Calcular o valor da resistência
						int resistencia = 0;
						for (int j = 0; j < countColors - 1; j++) {
							resistencia += colors[j].color * pow(10, countColors - j - 2);
						}

						// Aplicar o multiplicador
						resistencia *= pow(10, colors[countColors - 1].color);

						// Adicionar o texto com o valor da resistência
						str = std::to_string(resistencia) + " Ohms +/-5%";
						cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y - 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 2);
						cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y - 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);

						// Adicionar texto da posição da resistência
						str = "X: " + std::to_string(blobs[i].x) + " Y: " + std::to_string(blobs[i].y);
						cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y - 40), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 2);
						cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y - 40), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);

						// Denhar o centro de gravidade e o bounding box
						vc_draw_center_of_gravity(frame.data, &blobs[i], video.width, video.height, 5);
						vc_draw_bounding_box(frame.data, &blobs[i], video.width, video.height);

						// Validar se é a primeira vez que está a ser processada
						if (blobs[i].y > 127 && blobs[i].y < 133) {
							// Verificar se já existe uma resistência com o mesmo valor
							int foundResistencia = 0;
							for (auto& resistenciaEncontrada: resistenciasEncontradas) {
								// Validar se é a mesma potencia
								if (resistenciaEncontrada.potencia == resistencia) {
									// Adicionar um ao contador
									resistenciaEncontrada.count++;
									foundResistencia = 1;
									break;
								}
							}

							// Se não encontrou a resistência, adicionar à lista
							if (foundResistencia == 0) {
								ResistenciasEncontradas resistenciaEncontrada;
								resistenciaEncontrada.potencia = resistencia;
								resistenciaEncontrada.count = 1;
								resistenciasEncontradas.push_back(resistenciaEncontrada);
							}
						}
					}

					vc_image_free(imageBlob);
					vc_image_free(imageBlobSegmentation);
					vc_image_free(imageBlobsClosed);
					vc_image_free(imageBlobsBlobs);
				}
				// Libertar a memória dos blobs
				free(blobs);
			}
		}

		// Adicionar os textos das informações das frames
		str = "RESOLUCAO: " + std::to_string(video.width) + "x" + std::to_string(video.height);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = "TOTAL DE FRAMES: " + std::to_string(video.ntotalframes);
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = "FRAME RATE: " + std::to_string(video.fps);
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = "N. DA FRAME: " + std::to_string(video.nframe);
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

		/* Exibe a frame */
		cv::imshow("VC - VIDEO", frame);

		/* Sai da aplicação se o utilizador premir a tecla 'q' */
		key = cv::waitKey(1);
	}

	// Libertar a memória das imagens IVC
	vc_image_free(imageOutput);
	vc_image_free(imageRGB);
	vc_image_free(imageSegmented);
	vc_image_free(imageTemp);
	vc_image_free(imageClosed);
	vc_image_free(imageBlobs);

	// Criar um frame novo
	cv::Mat resultadoFinal(video.height, video.width, CV_8UC3, cv::Scalar(0, 0, 0));

	// Ordenar as resistências por ordem crescente
	std::sort(resistenciasEncontradas.begin(), resistenciasEncontradas.end(), [](ResistenciasEncontradas &a, ResistenciasEncontradas &b) {
		return a.potencia < b.potencia;
	});

	// Adicionar os textos das resistências encontradas
	int y = 30;
	for (auto& resistenciaEncontrada: resistenciasEncontradas) {
		str = "Resistencia: " + std::to_string(resistenciaEncontrada.potencia) + " Ohms - Quantidade: " + std::to_string(resistenciaEncontrada.count);
		cv::putText(resultadoFinal, str, cv::Point(20, y), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		y += 30;
	}

	// Exibir a frame com as resistências encontradas
	cv::imshow("VC - VIDEO", resultadoFinal);

	cv::waitKey(0);

	/* Para o timer e exibe o tempo decorrido */
	vc_timer();

	/* Fecha a janela */
	cv::destroyWindow("VC - VIDEO");
	/* Fecha o ficheiro de v deo */
	capture.release();

	return 0;
}
