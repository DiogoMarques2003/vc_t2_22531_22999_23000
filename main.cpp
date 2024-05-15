#include <iostream>
#include <string>
#include <chrono>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>

extern "C" {
#include "vc.h"
}

// Defenir array com os valores do HSV
HSV hsv_values[10] = {
    {0, 360, 0, 100, 0, 0},    // Preto -> 0
    {10, 50, 30, 100, 20, 100},  // Castanho -> 1
    {340, 20, 40, 100, 40, 100}, // Vermelho -> 2
    {10, 50, 50, 100, 40, 100},  // Laranja -> 3
    {45, 75, 60, 100, 50, 100},  // Amarelo -> 4
    {35, 85, 30, 100, 30, 100}, // Verde -> 5
    {85, 135, 30, 100, 30, 100}, // Azul -> 6 (ajustado)
    {220, 320, 30, 100, 30, 100}, // Roxo -> 7
    {0, 360, 0, 0, 20, 80},    // Cinza -> 8
    {0, 360, 0, 0, 100, 100}   // Branco -> 9
};








void vc_timer(void) {
	static bool running = false;
	static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

	if (!running) {
		running = true;
	}
	else {
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


int main(void) {
	// V deo
	char videofile[100] = "../../video_resistors.mp4"; //Trocar para o caminho do video
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
	//capture.open(0, cv::CAP_DSHOW); // Pode-se utilizar apenas capture.open(0);

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
	while (key != 'q') {
		/* Leitura de uma frame do v deo */
		capture.read(frame);

		/* Verifica se conseguiu ler a frame */
		if (frame.empty()) break;

		/* N mero da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		/* Exemplo de inser  o texto na frame */
		str = std::string("RESOLUCAO: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("TOTAL DE FRAMES: ").append(std::to_string(video.ntotalframes));
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("FRAME RATE: ").append(std::to_string(video.fps));
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("N. DA FRAME: ").append(std::to_string(video.nframe));
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);


		// Fa a o seu c digo aqui...
		// Cria uma nova imagem IVC
		IVC *imageOutput = vc_image_new(video.width, video.height, 3, 255);
		// Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
		memcpy(imageOutput->data, frame.data, video.width * video.height * 3);

		// Converter a imagem de bgr para rgb
		IVC *imageRGB = vc_image_new(imageOutput->width, imageOutput->height, 3, 255);
		vc_convert_bgr_to_rgb(imageOutput, imageRGB);

		// Segmentar a imagem para obter o corpo das resistencias
		IVC *imageSegmented = vc_image_new(imageOutput->width, imageOutput->height, 1, 255);
		vc_hsv_segmentation(imageRGB, imageSegmented, 30, 40, 30, 100, 45, 100);

		// Reconhecer as riscas das resistencias (hsv_values) e adicionar a imagem segmentada
		IVC *imageTemp = vc_image_new(imageOutput->width, imageOutput->height, 1, 255);
		for (int i = 0; i < sizeof(hsv_values) / sizeof(HSV); i++) {
			vc_hsv_segmentation(imageRGB, imageTemp, hsv_values[i].hmin, hsv_values[i].hmax, hsv_values[i].smin, hsv_values[i].smax, hsv_values[i].vmin, hsv_values[i].vmax);
			for (int y = 0; y < imageOutput->height; y++) {
				for (int x = 0; x < imageOutput->width; x++) {
					if (imageTemp->data[y * imageTemp->width + x] == 255) {
						imageSegmented->data[y * imageSegmented->width + x] = 255;
					}
				}
			}
		}
		vc_image_free(imageTemp);


		// Dilatar e Erodir a imagem para remover o espaço em branco por causa das linhas a cor da resistencia
		// IVC *imageClosed = vc_image_new(imageOutput->width, imageOutput->height, 1, 255);
		// vc_binary_close(imageSegmented, imageClosed, 5, 23);

		// //Obter os blobs das resistencias
		// int nblobs;
    	// OVC *blobs;
		// IVC *imageBlobs = vc_image_new(imageOutput->width, imageOutput->height, 1, 255);
		// blobs = vc_binary_blob_labelling(imageClosed, imageBlobs, &nblobs);

		// // Se tiver encontrado blobs de resistencias
		// if (blobs != NULL) {
		// 	std::cerr << "Resistencias encontradas: " << nblobs << "\n";

		// 	// Obter informação dos blobs
		// 	vc_binary_blob_info(imageBlobs, blobs, nblobs);

		// 	// Peercorrer os blobs
		// 	for (int i = 0; i < nblobs; i++) {
		// 		// Se o blob estiver a menos de 1% do inicio e 10% fim da imagem ignorar para garantir que a resitencia esta toda na imagem
		// 		if (blobs[i].y < 0.001 * imageBlobs->height || blobs[i].y > 0.90 * imageBlobs->height) {
		// 			continue;
		// 		}

		// 		// Desenhar o centro de gravidade
		// 		vc_draw_center_of_gravity(imageOutput, &blobs[i], 5);
		// 		// Desenhar o bounding box
		// 		vc_draw_bounding_box(imageOutput, &blobs[i]);
		// 	}

		// 	// Libertar a memoria dos blobs
		// 	free(blobs);
		// }

		// Apenas debug para conseguir ver a imagem a preto e branco
		cv::Mat imageToShow = cv::Mat(imageSegmented->height, imageSegmented->width, CV_8UC3);
        for (int y = 0; y < imageSegmented->height; y++) {
            for (int x = 0; x < imageSegmented->width; x++) {
                uchar value = imageSegmented->data[y * imageSegmented->width + x];
                imageToShow.at<cv::Vec3b>(y, x) = cv::Vec3b(value, value, value); // Replicar valor para os três canais
            }
        }
		memcpy(frame.data, imageToShow.data, video.width * video.height * 3);

		// Copia dados de imagem da estrutura IVC para uma estrutura cv::Mat
		// memcpy(frame.data, imageOutput->data, video.width * video.height * 3);
		// Libertar a memoria das imagens IVC
		vc_image_free(imageOutput);
		vc_image_free(imageRGB);
		vc_image_free(imageSegmented);
		// vc_image_free(imageClosed);
		// vc_image_free(imageBlobs);
		// +++++++++++++++++++++++++

		/* Exibe a frame */
		cv::imshow("VC - VIDEO", frame);

		/* Sai da aplica  o, se o utilizador premir a tecla 'q' */
		key = cv::waitKey(1);
	}

	/* Para o timer e exibe o tempo decorrido */
	vc_timer();

	/* Fecha a janela */
	cv::destroyWindow("VC - VIDEO");

	/* Fecha o ficheiro de v deo */
	capture.release();

	return 0;
}
