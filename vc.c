//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de fun��es n�o seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <stdbool.h>
#include "vc.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Alocar mem�ria para uma imagem
IVC *vc_image_new(int width, int height, int channels, int levels)
{
	IVC *image = (IVC *) malloc(sizeof(IVC));

	if(image == NULL) return NULL;
	if((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char *) malloc(image->width * image->height * image->channels * sizeof(char));

	if(image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar mem�ria de uma imagem
IVC *vc_image_free(IVC *image)
{
	if(image != NULL)
	{
		if(image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char *netpbm_get_token(FILE *file, char *tok, int len)
{
	char *t;
	int c;
	
	for(;;)
	{
		while(isspace(c = getc(file)));
		if(c != '#') break;
		do c = getc(file);
		while((c != '\n') && (c != EOF));
		if(c == EOF) break;
	}
	
	t = tok;
	
	if(c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));
		
		if(c == '#') ungetc(c, file);
	}
	
	*t = 0;
	
	return tok;
}


long int unsigned_char_to_bit(unsigned char *datauchar, unsigned char *databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char *p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);
				
				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}


void bit_to_unsigned_char(unsigned char *databit, unsigned char *datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char *p = databit;

	countbits = 1;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;
				
				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC *vc_read_image(char *filename)
{
	FILE *file = NULL;
	IVC *image = NULL;
	unsigned char *tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;
	
	// Abre o ficheiro
	if((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if(strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if(strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if(strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
			#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
			#endif

			fclose(file);
			return NULL;
		}
		
		if(levels == 1) // PBM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			if((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			size = image->width * image->height * image->channels;

			if((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}
		
		fclose(file);
	}
	else
	{
		#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
		#endif
	}
	
	return image;
}


int vc_write_image(char *filename, IVC *image)
{
	FILE *file = NULL;
	unsigned char *tmp;
	long int totalbytes, sizeofbinarydata;
	
	if(image == NULL) return 0;

	if((file = fopen(filename, "wb")) != NULL)
	{
		if(image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;
			
			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);
			
			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if(fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);
		
			if(fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				return 0;
			}
		}
		
		fclose(file);

		return 1;
	}
	
	return 0;
}

//Gerar negativo da imagem gray
int vc_gray_negative(IVC* srcdst) {
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	//Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 1) return 0;

	//Investe a imagem gray
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;

			data[pos] = 255 - data[pos];
		}
	}

	return 1;
}

//Gerar negativo da imagem RGB
int vc_rgb_negative(IVC* srcdst) {
    unsigned char *data = (unsigned char *) srcdst->data;
    int width = srcdst->width;
    int height = srcdst->height;
    int bytesperline = srcdst->bytesperline;
    int channels = srcdst->channels;
    int x, y;
    long int pos;

    //Verificação de erros
    if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
    if (channels != 3) return 0;

    for(y = 0; y < height; y++) {
        for(x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;

            data[pos] = 255 - data[pos];
            data[pos + 1] = 255 - data[pos + 1];
            data[pos + 2] = 255 - data[pos + 2];
        }
    }

    return 1;
}

//Extrair componente RED da imagem RGB para gray
int vc_rgb_get_red_gray(IVC *srcdst) {
    unsigned char *data = (unsigned char *) srcdst->data;
    int width = srcdst->width;
    int height = srcdst->height;
    int bytesperline = srcdst->bytesperline;
    int channels = srcdst->channels;
    int x, y;
    long int pos;

    //Verificação de erros
    if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
    if (channels != 3) return 0;

    //Extrai a componente RED
    for(y = 0; y < height; y++) {
        for(x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;

            data[pos + 1] = data[pos]; //Green
            data[pos + 2] = data[pos]; //Blue
        }
    }

    return 1;
}

//Extrair componente GREEN da imagem RGB para gray
int vc_rgb_get_green_gray(IVC *srcdst) {
    unsigned char *data = (unsigned char *) srcdst->data;
    int width = srcdst->width;
    int height = srcdst->height;
    int bytesperline = srcdst->bytesperline;
    int channels = srcdst->channels;
    int x, y;
    long int pos;

    //Verificação de erros
    if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
    if (channels != 3) return 0;

    //Extrai a componente RED
    for(y = 0; y < height; y++) {
        for(x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;

            data[pos] = data[pos + 1]; //RED
            data[pos + 2] = data[pos + 1]; //BLUE
        }
    }

    return 1;
}

//Extrair componente BLUE da imagem RGB para gray
int vc_rgb_get_blue_gray(IVC *srcdst) {
    unsigned char *data = (unsigned char *) srcdst->data;
    int width = srcdst->width;
    int height = srcdst->height;
    int bytesperline = srcdst->bytesperline;
    int channels = srcdst->channels;
    int x, y;
    long int pos;

    //Verificação de erros
    if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
    if (channels != 3) return 0;

    //Extrai a componente RED
    for(y = 0; y < height; y++) {
        for(x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;

            data[pos] = data[pos + 2]; //RED
            data[pos + 1] = data[pos + 2]; //GREEN
        }
    }

    return 1;
}

//Converter de RGB para Gray
int vc_rgb_to_gray(IVC *src, IVC *dst) {
    unsigned char *datasrc = (unsigned char*) src->data;
    int bytesperline_src = src->width * src->channels;
    int channels_src = src->channels;
    unsigned char *datadst = (unsigned  char*) dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;
    float rf, gf, bf;

    //Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 3) || (dst->channels != 1)) return 0;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos_src = y * bytesperline_src + x * channels_src;
            pos_dst = y * bytesperline_dst + x * channels_dst;

            rf = (float) datasrc[pos_src];
            gf = (float) datasrc[pos_src + 1];
            bf = (float) datasrc[pos_src + 2];

            datadst[pos_dst] = (unsigned char) ((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
        }
    }

    return 1;
}

//Converter rgb para hsv
int vc_rgb_to_hsv(IVC *src, IVC *dst) {
    unsigned char *datasrc = (unsigned char*) src->data;
    int channels_src = src->channels;
    unsigned char *datadst = (unsigned  char*) dst->data;
    int width = src->width;
    int height = src->height;
    int x;
    float rf, gf, bf;
    float max, min, delta;
    float h, s, v;

    //Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 3) || (dst->channels != 3)) return 0;

    float size = width * height * channels_src;

    for (x = 0; x < size; x += channels_src) {
        rf = (float)datasrc[x];
        gf = (float)datasrc[x + 1];
        bf = (float)datasrc[x + 2];

        max = (rf > gf ? (rf > bf ? rf : bf) : (gf > bf ? gf : bf));
        min = (rf < gf ? (rf < bf ? rf : bf) : (gf < bf ? gf : bf));
        delta = max - min;

        v = max;
        if (v == 0.0f) {
            h = 0.0f;
            s = 0.0f;
        } else {
            // Saturation toma valores entre [0,255]
            s = (delta / max) * 255.0f;

            if (s == 0.0f) {
                h = 0.0f;
            } else {
                // Hue toma valores entre [0,360]
                if ((max == rf) && (gf >= bf)) {
                    h = 60.0f * (gf - bf) / delta;
                } else if ((max == rf) && (bf > gf)) {
                    h = 360.0f + 60.0f * (gf - bf) / delta;
                } else if (max == gf) {
                    h = 120.0f + 60.0f * (bf - rf) / delta;
                } else {
                    h = 240.0f + 60.0f * (rf - gf) / delta;
                }
            }
        }

        datadst[x] = (unsigned char) ((h / 360.0f) * 255.0f);
        datadst[x + 1] = (unsigned char) (s);
        datadst[x + 2] = (unsigned char) (v);
    }

    return 1;
}

// hmin,hmax = [0, 360]; smin,smax = [0, 100]; vmin,vmax = [0, 100]
int vc_hsv_segmentation(IVC *src, IVC *dst, int hmin, int hmax, int smin,
                        int smax, int vmin, int vmax, int *found) {
    unsigned char *datasrc = src->data;
    unsigned char *datadst = dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline_src = width * src->channels;
    int bytesperline_dst = width * dst->channels;
    int channels_src = src->channels;
    int channels_dst = dst->channels;

    if (src->width <= 0 || src->height <= 0 || src->data == NULL) return 0;
    if (src->width != dst->width || src->height != dst->height) return 0;
    if (src->channels != 3 || dst->channels != 1) return 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int pos_src = y * bytesperline_src + x * channels_src;
            int pos_dst = y * bytesperline_dst + x * channels_dst;

            float rf = datasrc[pos_src] / 255.0f;
            float gf = datasrc[pos_src + 1] / 255.0f;
            float bf = datasrc[pos_src + 2] / 255.0f;

            float max = (rf > gf ? (rf > bf ? rf : bf) : (gf > bf ? gf : bf));
            float min = (rf < gf ? (rf < bf ? rf : bf) : (gf < bf ? gf : bf));
            float delta = max - min;

            float hue = 0, sat = 0, valor = max;

            if (max != 0) {
                sat = delta / max;
                if (delta != 0) {
                    if (max == rf) {
                        hue = 60.0f * (gf - bf) / delta + (gf < bf ? 360.0f : 0.0f);
                    } else if (max == gf) {
                        hue = 60.0f * (bf - rf) / delta + 120.0f;
                    } else {
                        hue = 60.0f * (rf - gf) / delta + 240.0f;
                    }
                }
            }

            hue = fmod(hue, 360.0f);
            sat *= 100.0f;
            valor *= 100.0f;

            bool hue_in_range = (hmin > hmax) ? (hue >= hmin || hue <= hmax) : (hue >= hmin && hue <= hmax);
            bool sat_in_range = sat >= smin && sat <= smax;
            bool val_in_range = valor >= vmin && valor <= vmax;

            if (hue_in_range && sat_in_range && val_in_range) {
                datadst[pos_dst] = 255;
                *found = 1;
            } else {
                datadst[pos_dst] = 0;
            }
        }
    }

    return 1;
}

int vc_scale_gray_to_color_palette(IVC *src, IVC *dst) {
    unsigned char* datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->channels * src->width;
    int channels_src = src->channels;
    unsigned char* datadst = (unsigned char*)dst->data;
    int bytesperline_dst = dst->channels * dst->width;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;

    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 1) || (dst->channels != 3)) return 0;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos_src = y * bytesperline_src + x * channels_src;
            pos_dst = y * bytesperline_dst + x * channels_dst;

            if (datasrc[pos_src] < 64) { // Primeiro quarteto -> cores verdes
                datadst[pos_dst] = 0;
                datadst[pos_dst + 1] = datasrc[pos_src] * 4;
                datadst[pos_dst + 2] = 255;
            } else if (datasrc[pos_src] < 128) { // Segundo quarteto -> verde a azul
                datadst[pos_dst] = 0;
                datadst[pos_dst + 1] = 255;
                datadst[pos_dst + 2] = 255 - ((datasrc[pos_src] - 64) * 4);
            } else if (datasrc[pos_src] < 192) { // Terceiro quarteto -> azul a vermelho
                datadst[pos_dst] = (datasrc[pos_src] - 128) * 4;
                datadst[pos_dst + 1] = 255;
                datadst[pos_dst + 2] = 0;
            } else { // Quarto quarteto -> vermelho para verde
                datadst[pos_dst] = 255;
                datadst[pos_dst + 1] = 255 - ((datasrc[pos_src] - 192) * 4);;
                datadst[pos_dst + 2] = 0;
            }
        }
    }

    return 1;
}

int vc_image_white_pixel_count(IVC *src) {
    int x, y, count = 0;
    long int pos;

    for (y = 0; y < src->height; y++) {
        for (x = 0; x < src->width; x++) {
            pos = y * src->bytesperline + x * src->channels;

            if (src->data[pos] == 255) {
                count++;
            }
        }
    }

    return count;
}

int vc_gray_to_binary(IVC *src, IVC *dst, int threshold) {
    unsigned char* datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->channels * src->width;
    int channels_src = src->channels;
    unsigned char* datadst = (unsigned char*)dst->data;
    int bytesperline_dst = dst->channels * dst->width;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;

    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 1) || (dst->channels != 1)) return 0;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos_src = y*bytesperline_src + x*channels_src;
            pos_dst = y*bytesperline_dst + x*channels_dst;

            if (datasrc[pos_src] > threshold) {
                datadst[pos_dst] = 255;
            } else {
                datadst[pos_dst] = 0;
            }

        }
    }

    return 1;
}

int vc_gray_to_binary_global_mean(IVC *src, IVC *dst) {
    unsigned char* datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->channels * src->width;
    int channels_src = src->channels;
    unsigned char* datadst = (unsigned char*)dst->data;
    int bytesperline_dst = dst->channels * dst->width;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;
    int total=0;
    float threshold;

    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 1) || (dst->channels != 1)) return 0;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos_src = y*bytesperline_src + x*channels_src;

            total += datasrc[pos_src];
        }
    }

    threshold = (float)total/(width*height);

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos_src = y*bytesperline_src + x*channels_src;
            pos_dst = y*bytesperline_dst + x*channels_dst;

            if (datasrc[pos_src] > threshold) {
                datadst[pos_dst] = 255;
            }
            else
            {
                datadst[pos_dst] = 0;
            }

        }
    }

    return 1;
}

int vc_gray_to_binary_midpoint(IVC *src, IVC *dst, int kernel) {
    unsigned char* datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->channels * src->width;
    int channels_src = src->channels;
    unsigned char* datadst = (unsigned char*)dst->data;
    int bytesperline_dst = dst->channels * dst->width;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y, vY, vX;
    long int pos_src, pos_dst;
    float threshold;
    float vMax = 0, vMin = 256;

    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 1) || (dst->channels != 1)) return 0;

    //Percorre uma imagem
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos_src = y * bytesperline_src + x * channels_src;
            pos_dst = y * bytesperline_dst + x * channels_dst;

            vY = kernel / 2;
            vX = kernel / 2;

            int xx, yy;

            for (vMax = 0, vMin = 255, yy = y - vY; yy <= y + vY; yy++) {
                for (xx = x - vX ; xx <= x + vX; xx++) {
                    if(yy >= 0 && yy < height && xx >=0 && xx < width) {
                        pos_src = yy * bytesperline_src + xx * channels_src;

                        if (datasrc[pos_src] > vMax) {
                            vMax = datasrc[pos_src];
                        }

                        if (datasrc[pos_src] < vMin)
                        {
                            vMin = datasrc[pos_src];
                        }
                    }
                }
            }

            threshold = (vMin + vMax) / 2;

            if (datasrc[pos_src] > threshold) {
                datadst[pos_dst] = 255;
            } else {
                datadst[pos_dst] = 0;
            }
        }
    }

    return 1;
}

int vc_gray_to_binary_niblac(IVC *src, IVC *dst, int kernel, float k) {
    unsigned char* datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->channels * src->width;
    int channels_src = src->channels;
    unsigned char* datadst = (unsigned char*)dst->data;
    int width = src->width;
    int height = src->height;
    int x, y, vY, vX, counter;
    long int pos, pos_v;
    unsigned char threshold;
    int offset = (kernel - 1) / 2;

    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 1) || (dst->channels != 1)) return 0;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos = y * bytesperline_src + x * channels_src;

            float mean = 0.0f;

            //Vizinhos
            for (counter = 0, vY = -offset; vY <= offset; vY++) {
                for (vX = -offset; vX <= offset; vX++) {
                    if ((y + vY > 0) && (y + vY < height) && (x + vX > 0) && (x + vX < width)) {
                        pos_v = (y + vY) * bytesperline_src + (x + vX) * channels_src;

                        mean += (float) datasrc[pos_v];

                        counter++;
                    }
                }
            }
            mean /= counter;

            float sdeviation = 0.0f;

            //Vizinhos
            for (counter = 0, vY = -offset; vY <= offset; vY++) {
                for (vX = -offset; vX <= offset; vX++) {
                    if ((y + vY > 0) && (y + vY < height) && (x + vX > 0) && (x + vX < width)) {
                        pos_v = (y + vY) * bytesperline_src + (x + vX) * channels_src;

                        sdeviation += powf(((float)datasrc[pos_v]) - mean, 2);

                        counter++;
                    }
                }
            }
            sdeviation = sqrt(sdeviation / counter);

            threshold = mean + k * sdeviation;

            if (datasrc[pos] > threshold) datadst[pos] = 255;
            else datadst[pos] = 0;
        }
    }

    return 1;
}

int vc_binary_dilate(IVC *src, IVC *dst, int kernel){
    unsigned char *datasrc = (unsigned char *)src->data;
    unsigned char *datadst = (unsigned char *)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y, xk, yk, i, j;
    long int pos, posk;
    int s1, s2;

    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
    if (channels != 1) return 0;

    s2 = (kernel - 1) / 2;
    s1 = -(s2);

    memset(datadst, 0, bytesperline * height);

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;
            unsigned char pixel = 0;

            for (yk = s1; yk <= s2; yk++) {
                j = y + yk;
                if (j < 0 || j >= height) continue;
                for (xk = s1; xk <= s2; xk++) {
                    i = x + xk;
                    if (i < 0 || i >= width) continue;
                    posk = j * bytesperline + i * channels;
                    pixel |= datasrc[posk];
                }
            }

            if (pixel == 255) {
                datadst[pos] = 255;
            }
        }
    }

    return 1;
}

// Erosão binária
int vc_binary_erode(IVC *src, IVC *dst, int kernel)
{
   unsigned char *datasrc = (unsigned char *)src->data;
    unsigned char *datadst = (unsigned char *)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y, xk, yk, i, j;
    long int pos, posk;
    int s1, s2;

    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
    if (channels != 1) return 0;

    s2 = (kernel - 1) / 2;
    s1 = -(s2);

    memset(datadst, 255, bytesperline * height);

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;
            unsigned char pixel = 255;

            for (yk = s1; yk <= s2; yk++) {
                j = y + yk;
                if (j < 0 || j >= height) continue;
                for (xk = s1; xk <= s2; xk++) {
                    i = x + xk;
                    if (i < 0 || i >= width) continue;
                    posk = j * bytesperline + i * channels;
                    pixel &= datasrc[posk];
                }
            }

            if (pixel == 0) {
                datadst[pos] = 0;
            }
        }
    }

    return 1;
}

//serve para fazer contornos ( erodir a imagem para depois contornar para simplificar a imagem )
int vc_binary_open(IVC *src, IVC *dst, int kernelErode, int kernelDilate){

    IVC *temp;
    temp = vc_image_new(src->width,src->height,1,255);

    vc_binary_erode(src,temp,kernelErode);

    vc_binary_dilate(temp,dst,kernelDilate);

    vc_image_free(temp);

    return 1;
}

int vc_binary_close(IVC *src, IVC *dst, int kernelErode, int kernelDilate){
    IVC *temp = vc_image_new(src->width, src->height, 1, 255);

    vc_binary_dilate(src, temp, kernelDilate);
    vc_binary_erode(temp, dst, kernelErode);

    vc_image_free(temp);

    return 1;
}

// Etiquetagem de blobs
// src		: Imagem binária de entrada
// dst		: Imagem grayscale (irá conter as etiquetas)
// nlabels	: Endereço de memória de uma variável, onde será armazenado o numero de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. é necessário libertar posteriormente esta memória.
OVC* vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels) {
    unsigned char *datasrc = (unsigned char *)src->data;
    unsigned char *datadst = (unsigned char *)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y, a, b;
    long int i, size;
    long int posX, posA, posB, posC, posD;
    int labeltable[256] = { 0 };
    int labelarea[256] = { 0 };
    int label = 1; // Etiqueta inicial.
    int num, tmplabel;
    OVC *blobs; // Apontador para array de blobs (objectos) que será retornado desta função.

    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
    if (channels != 1) return NULL;

    // Copia dados da imagem binária para imagem grayscale
    memcpy(datadst, datasrc, bytesperline * height);

    // Todos os pixeis de plano de fundo devem obrigatoriamente ter valor 0
    // Todos os pixeis de primeiro plano devem obrigatoriamente ter valor 255
    // Serão atribuidas etiquetas no intervalo [1,254]
    // Este algoritmo esta assim limitado a 254 labels
    for (i = 0, size = bytesperline * height; i<size; i++) {
        if (datadst[i] != 0) datadst[i] = 255;
    }

    // Limpa os rebordos da imagem binária
    for (y = 0; y<height; y++) {
        datadst[y * bytesperline + 0 * channels] = 0;
        datadst[y * bytesperline + (width - 1) * channels] = 0;
    }

    for (x = 0; x<width; x++) {
        datadst[0 * bytesperline + x * channels] = 0;
        datadst[(height - 1) * bytesperline + x * channels] = 0;
    }

    // Efectua a etiquetagem
    for (y = 1; y<height - 1; y++) {
        for (x = 1; x<width - 1; x++) {
            // Kernel:
            // A B C
            // D X

            posA = (y - 1) * bytesperline + (x - 1) * channels; // A
            posB = (y - 1) * bytesperline + x * channels; // B
            posC = (y - 1) * bytesperline + (x + 1) * channels; // C
            posD = y * bytesperline + (x - 1) * channels; // D
            posX = y * bytesperline + x * channels; // X

            // Se o pixel foi marcado
            if (datadst[posX] != 0) {
                if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0)) {
                    datadst[posX] = label;
                    labeltable[label] = label;
                    label++;
                } else {
                    num = 255;

                    // Se A est� marcado
                    if (datadst[posA] != 0) num = labeltable[datadst[posA]];
                    // Se B est� marcado, e � menor que a etiqueta "num"
                    if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
                    // Se C est� marcado, e � menor que a etiqueta "num"
                    if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
                    // Se D est� marcado, e � menor que a etiqueta "num"
                    if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

                    // Atribui a etiqueta ao pixel
                    datadst[posX] = num;
                    labeltable[num] = num;

                    // Actualiza a tabela de etiquetas
                    if (datadst[posA] != 0) {
                        if (labeltable[datadst[posA]] != num) {
                            for (tmplabel = labeltable[datadst[posA]], a = 1; a<label; a++) {
                                if (labeltable[a] == tmplabel) {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }

                    if (datadst[posB] != 0) {
                        if (labeltable[datadst[posB]] != num) {
                            for (tmplabel = labeltable[datadst[posB]], a = 1; a<label; a++) {
                                if (labeltable[a] == tmplabel) {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }

                    if (datadst[posC] != 0) {
                        if (labeltable[datadst[posC]] != num) {
                            for (tmplabel = labeltable[datadst[posC]], a = 1; a<label; a++) {
                                if (labeltable[a] == tmplabel) {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }

                    if (datadst[posD] != 0) {
                        if (labeltable[datadst[posD]] != num) {
                            for (tmplabel = labeltable[datadst[posD]], a = 1; a<label; a++) {
                                if (labeltable[a] == tmplabel) {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Volta a etiquetar a imagem
    for (y = 1; y<height - 1; y++) {
        for (x = 1; x<width - 1; x++) {
            posX = y * bytesperline + x * channels; // X

            if (datadst[posX] != 0) {
                datadst[posX] = labeltable[datadst[posX]];
            }
        }
    }

    // Contagem do número de blobs
    // Passo 1: Eliminar, da tabela, etiquetas repetidas
    for (a = 1; a<label - 1; a++) {
        for (b = a + 1; b<label; b++) {
            if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
        }
    }

    // Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que não hajam valores vazios (zero) entre etiquetas
    *nlabels = 0;
    for (a = 1; a<label; a++) {
        if (labeltable[a] != 0) {
            labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
            (*nlabels)++; // Conta etiquetas
        }
    }

    // Se não ha blobs
    if (*nlabels == 0) return NULL;

    // Cria lista de blobs (objectos) e preenche a etiqueta
    blobs = (OVC *)calloc((*nlabels), sizeof(OVC));
    if (blobs != NULL) {
        for (a = 0; a<(*nlabels); a++) blobs[a].label = labeltable[a];
    } else return NULL;

    return blobs;
}

int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs) {
    unsigned char *data = (unsigned char *)src->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y, i;
    long int pos;
    int xmin, ymin, xmax, ymax;
    long int sumx, sumy;

    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if (channels != 1) return 0;

    // Conta área de cada blob
    for (i = 0; i<nblobs; i++) {
        xmin = width - 1;
        ymin = height - 1;
        xmax = 0;
        ymax = 0;

        sumx = 0;
        sumy = 0;

        blobs[i].area = 0;

        for (y = 1; y<height - 1; y++) {
            for (x = 1; x<width - 1; x++) {
                pos = y * bytesperline + x * channels;

                if (data[pos] == blobs[i].label) {
                    // área
                    blobs[i].area++;

                    // Centro de Gravidade
                    sumx += x;
                    sumy += y;

                    // Bounding Box
                    if (xmin > x) xmin = x;
                    if (ymin > y) ymin = y;
                    if (xmax < x) xmax = x;
                    if (ymax < y) ymax = y;

                    // Perímetro
                    // Se pelo menos um dos quatro vizinhos não pertence ao mesmo label, então é um pixel de contorno
                    if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label)) {
                        blobs[i].perimeter++;
                    }
                }
            }
        }

        // Bounding Box
        blobs[i].x = xmin;
        blobs[i].y = ymin;
        blobs[i].width = (xmax - xmin) + 1;
        blobs[i].height = (ymax - ymin) + 1;

        // Centro de Gravidade
        blobs[i].xc = sumx / MAX(blobs[i].area, 1);
        blobs[i].yc = sumy / MAX(blobs[i].area, 1);
    }

    return 1;
}

int vc_blob_to_gray_scale(IVC *src, IVC *dst, int nlabels) {
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y;
    long int pos;

    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
    if (channels != 1) return 0;

    float labelIntens = (float)255 / nlabels;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;

            dst->data[pos] = src->data[pos] * labelIntens;
        }
    }

    return 0;
}

RGB* vc_generate_colors(int numColors) {
    RGB* colors = malloc(numColors * sizeof(RGB));
    if (colors == NULL) return NULL;

    int n = ceil(pow(numColors, 1.0 / 3.0));
    int increment = (n > 1) ? (255 / (n - 1)) : 255;

    int index = 0;
    for (int r = 0; r < n && index < numColors; r++) {
        for (int g = 0; g < n && index < numColors; g++) {
            for (int b = 0; b < n && index < numColors; b++) {
                colors[index].red = r * increment;
                colors[index].green = g * increment;
                colors[index].blue = b * increment;
                index++;
                if (index >= numColors) break;
            }
            if (index >= numColors) break;
        }
        if (index >= numColors) break;
    }
    return colors;
}

int vc_blob_to_gray_rgb(IVC *src, IVC *dst, int nlabels) {
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y;
    long int pos, pos_dst;
    RGB* colors;
    RGB color;

    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != 1) || (dst->channels != 3)) return 0;

    colors = vc_generate_colors(nlabels);
    if (colors == NULL) return 0;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;
            pos_dst = y * dst->bytesperline + x * dst->channels;

            color = colors[src->data[pos]];

            dst->data[pos_dst] = color.red;
            dst->data[pos_dst + 1] = color.green;
            dst->data[pos_dst + 2] = color.blue;
        }
    }

    free(colors);

    return 0;
}

int vc_draw_center_of_gravity(unsigned char *data, OVC *blob, int width, int height, int comp) {
    int x, y;
    int bytesperline = width * 3;

    // Desenhar linha horizontal
    for (x = blob->xc - comp; x <= blob->xc + comp; x++) {
        int index = blob->yc * bytesperline + x * 3;
        data[index] = 0;
        data[index + 1] = 255;
        data[index + 2] = 0;
    }

    // Desenhar linha vertical
    for (y = blob->yc - comp; y <= blob->yc + comp; y++) {
        int index = y * bytesperline + blob->xc * 3;
        data[index] = 0;
        data[index + 1] = 255;
        data[index + 2] = 0;
    }

    return 1;
}

int vc_draw_bounding_box(unsigned char *data, OVC *blob, int width, int height) {
    int x, y;
    int bytesperline = width * 3;

    // Desenhar linhas verticais esquerda e direita
    for (y = blob->y; y < blob->y + blob->height; y++) {
        int linhaEsquerda = y * bytesperline + blob->x * 3;
        data[linhaEsquerda] = 0;
        data[linhaEsquerda + 1] = 255;
        data[linhaEsquerda + 2] = 0;

        int linhaDireita = y * bytesperline + (blob->x + blob->width - 1) * 3;
        data[linhaDireita] = 0;
        data[linhaDireita + 1] = 255;
        data[linhaDireita + 2] = 0;
    }

    // Desenhar linhas horizontais superior e inferior
    for (x = blob->x; x < blob->x + blob->width; x++) {
        int linhaCima = blob->y * bytesperline + x * 3;
        data[linhaCima] = 0;
        data[linhaCima + 1] = 255;
        data[linhaCima + 2] = 0;

        int linhaBaixo = (blob->y + blob->height - 1) * bytesperline + x * 3;
        data[linhaBaixo] = 0;
        data[linhaBaixo + 1] = 255;
        data[linhaBaixo + 2] = 0;
    }

    return 0;
}

int vc_gray_histogram_show(IVC *src, IVC *dst) {
    unsigned char* datasrc = (unsigned char*)src->data;
    unsigned char* datadst = (unsigned char*)dst->data;
    int x, y;
    int ni[256] = { 0 }; // contador para cada pixel
    float pdf[256]; // Função de probabilidade da densidade
    float pdfnorm[256]; // Nomarlização
    float pdfmax = 0; // Valor do pixel mais alto
    int n = src->width * src->height; // Número de pixéis na imagem

    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if (dst->width != 256 && dst->height != 256) return 0;
    if ((src->channels != 1) || (dst->channels != 1)) return 0;

    // 1º - Contar o brilho dos pixéis
    for (int i = 0; i < n; ni[datasrc[i++]]++);

    // 2º - Calcular a densidade
    for (int i = 0; i < 256; i++) {
        pdf[i] = (float)ni[i] / n;
    }

    // 3º - Encontrar o pixel com valor maior
    for (int i = 0; i < 256; i++) {
        if (pdf[i] > pdfmax) {
            pdfmax = pdf[i];
        }
    }

    // 4º - Normalização
    for (int i = 0; i < 256; i++) {
        pdfnorm[i] = pdf[i] / pdfmax;
    }

    // 5º - Gerar o histograma
    //Limpar a imagem para garantir que fica apenas com o histograma
    memset(datadst, 0, 256 * 256 * sizeof(unsigned char));

    //Desenhar
    for (x = 0; x < 256; x++) {
        for (y = 256 - 1; y >= (256 - 1) - pdfnorm[x] * 255; y--) {
            datadst[y * 256 + x] = 255;
        }
    }

    return 0;
}

int vc_gray_histogram_equalization(IVC *src, IVC *dst) {
    unsigned char *datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->width*src->channels;
    int channels_src = src->channels;
    unsigned char *datadst = (unsigned char*)dst->data;
    int width = src->width;
    int height = src->height;
    int x ,y;
    long int pos_dst;
    int n[256] = {0};
    float pdf[256] = { 0 }, cdf[256] = { 0 };
    float size = width * height;
    float min = 0;

    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if (dst->width != src->width && dst->height != src->height) return 0;
    if ((src->channels != 1) || (dst->channels != 1)) return 0;

    for(x=0; x<height*width ; x++) {
        n[datasrc[x]]++;
    }

    //calcular pdf
    for (x = 0; x <= 255; x++) {
        pdf[x] = (((float)n[x]) / size);
    }

    //calcular cdf
    for(y = 0; y <= 255; y++) {
        if(y == 0) {
            cdf[y] = pdf[y];
        } else {
            cdf[y]= cdf[y - 1]+ pdf[y];
        }
    }

    for(x = 0; x <= 255; x++) {
        if (cdf[x] != 0) {
            min = pdf[x];
            break;
        }
    }

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos_dst = y * bytesperline_src + x * channels_src;
            datadst[pos_dst] = (cdf[datasrc[pos_dst]] - min) / (1.0 - min) * (256-1);
        }
    }

    return 0;
}

int vc_convert_bgr_to_rgb(IVC *src, IVC *dst) {
    unsigned char *datasrc = (unsigned char*) src->data;
    int channels_src = src->channels;
    int bytesperline_src = src->bytesperline;
    unsigned char *datadst = (unsigned  char*) dst->data;
    int width = src->width;
    int height = src->height;
    long int pos;

    //Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 3) || (dst->channels != 3)) return 0;

    // Copiar a data da src para a dst
    memcpy(datadst, datasrc, bytesperline_src * height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pos = y * bytesperline_src + x * channels_src;
            unsigned char blue = datadst[pos];
            datadst[pos] = datadst[pos + 2]; // Azul recebe o valor de Vermelho
            datadst[pos + 2] = blue; // Vermelho recebe o valor de Azul
        }
    }

    return 0;
}
