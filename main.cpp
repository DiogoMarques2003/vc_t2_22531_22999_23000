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
	{0, 360, 0, 100, 0, 0},		  // Preto -> 0
	{10, 50, 30, 100, 20, 100},	  // Castanho -> 1
	{340, 20, 40, 100, 40, 100},  // Vermelho -> 2
	{10, 50, 50, 100, 40, 100},	  // Laranja -> 3
	{45, 75, 60, 100, 50, 100},	  // Amarelo -> 4
	{75, 165, 30, 100, 30, 100},  // Verde -> 5
	{165, 255, 15, 100, 35, 100}, // Azul -> 6 (ajustado)
	{220, 320, 30, 100, 30, 100}, // Roxo -> 7
	{0, 360, 0, 10, 20, 80},	  // Cinza -> 8
	{0, 360, 0, 0, 100, 100}	  // Branco -> 9
};

void vc_timer(void)
{
	static bool running = false;
	static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

	if (!running)
	{
		running = true;
	}
	else
	{
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
	if (!capture.isOpened())
	{
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

	int hsv_count = sizeof(hsv_values) / sizeof(HSV);

	while (key != 'q') {
		/* Leitura de uma frame do vídeo */
		capture.read(frame);

		/* Verifica se conseguiu ler a frame */
		if (frame.empty())
			break;

		/* Número da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		// Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
		memcpy(imageOutput->data, frame.data, video.width * video.height * 3);

		// Converter a imagem de BGR para RGB
		vc_convert_bgr_to_rgb(imageOutput, imageRGB);

		// Segmentar a imagem para obter o corpo das resistências
		vc_hsv_segmentation(imageRGB, imageSegmented, 30, 40, 30, 100, 45, 100);

		// Segmentar as riscas das resistências (HSV values) e adicionar à imagem segmentada
		for (int i = 0; i < hsv_count; i++) {
			vc_hsv_segmentation(imageRGB, imageTemp, hsv_values[i].hmin, hsv_values[i].hmax, hsv_values[i].smin, hsv_values[i].smax, hsv_values[i].vmin, hsv_values[i].vmax);
			for (int y = 0; y < imageOutput->height; y++) {
				for (int x = 0; x < imageOutput->width; x++) {
					if (imageTemp->data[y * imageTemp->width + x] == 255) {
						imageSegmented->data[y * imageSegmented->width + x] = 255;
					}
				}
			}
		}

		// Dilatar e erodir a imagem para remover o espaço em branco por causa das linhas a cor da resistência
		vc_binary_close(imageSegmented, imageClosed, 3, 5);

		// Obter os blobs das resistências
		int nblobs;
		OVC *blobs = vc_binary_blob_labelling(imageClosed, imageBlobs, &nblobs);

		// Se encontrou blobs
		if (blobs != NULL) {
			// Obter informação dos blobs
			vc_binary_blob_info(imageBlobs, blobs, nblobs);

			// Percorrer os blobs
			for (int i = 0; i < nblobs; i++) {
				// Se o blob estiver a menos de 1% do início e 10% do fim da imagem, ignorar para garantir que a resistência está toda na imagem
				if (blobs[i].y < 0.001 * imageBlobs->height || blobs[i].y > 0.90 * imageBlobs->height) {
					continue;
				}

				// Pegar na imagem original do blob
				IVC *imageBlob = vc_image_new(blobs[i].width, blobs[i].height, 3, 255);

				// Copiar o blob para a nova imagem
				for (int y = 0; y < blobs[i].height; y++) {
					for (int x = 0; x < blobs[i].width; x++) {
						for (int band = 0; band < 3; band++) {
							imageBlob->data[(y * imageBlob->width + x) * 3 + band] =
                    imageRGB->data[((blobs[i].y + y) * imageRGB->width + (blobs[i].x + x)) * 3 + band];
						}
					}
				}

				int countColors = 0;
				CoresEncontradas colors[4];

				// Segmentar pelas cores das riscas das resistências
				for (int j = 0; j < hsv_count; j++) {
					vc_hsv_segmentation(imageRGB, imageTemp, hsv_values[j].hmin, hsv_values[j].hmax, hsv_values[j].smin, hsv_values[j].smax, hsv_values[j].vmin, hsv_values[j].vmax);

					// Fazer close para juntar os espaços que possam estar pretos
					vc_binary_close(imageTemp, imageClosed, 3, 5);

					// Obter os blobs das resistências
					int nblobsBlob;
					OVC *blobsBlob = vc_binary_blob_labelling(imageClosed, imageBlobs, &nblobsBlob);

					// Se encontrou blobs
					if (blobsBlob != NULL) {
						// Obter informação dos blobs
						vc_binary_blob_info(imageBlobs, blobsBlob, nblobsBlob);

						// Percorrer os blobs
                   		for (int k = 0; k < nblobsBlob; k++) {
							printf("Cor encontrada: %d\n", j);
                        	colors[countColors].color = j;
                        	colors[countColors].x = blobsBlob[k].xc;
                        	countColors++;
                    	}

						// Libertar a memória dos blobs
						free(blobsBlob);
					}

					// Se encontrou 4 cores, sair do loop
					if (countColors >= 4) {
						break;
					}
				}

				// Desenhar o centro de gravididade e boundingbox se tiver 3 ou 4 cores
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

					printf("Resistencia: %d Ohms\n", resistencia);

					// Adicionar o texto com o valor da resistência
					str = std::to_string(resistencia) + " Ohms";
                    cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y - 20), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
					
					// Desenhar o centro de gravidade e o bounding box
                    cv::Point pt1(blobs[i].x, blobs[i].y);
                    cv::Point pt2(blobs[i].x + blobs[i].width, blobs[i].y + blobs[i].height);
                    cv::rectangle(frame, pt1, pt2, cv::Scalar(0, 255, 0), 2);

                    cv::Point pt3(blobs[i].xc, blobs[i].yc);
                    cv::Point pt4(blobs[i].xc + 5, blobs[i].yc + 5);
                    cv::rectangle(frame, pt3, pt4, cv::Scalar(0, 255, 0), 2);
				}

				vc_image_free(imageBlob);
			}

			// Libertar a memória dos blobs
			free(blobs);
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

	/* Para o timer e exibe o tempo decorrido */
	vc_timer();

	/* Fecha a janela */
	cv::destroyWindow("VC - VIDEO");

	/* Fecha o ficheiro de v deo */
	capture.release();

	return 0;
}
