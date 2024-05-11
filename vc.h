//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#define VC_DEBUG


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
	unsigned char *data;
	int width, height;
	int channels;			// Bin�rio/Cinzentos=1; RGB=3
	int levels;				// Bin�rio=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// widchth * channels
} IVC;

// Estrutura do BLOB
typedef struct {
    int x, y, width, height; // Caixa delimitadora (Bounding Box)
    int area;                // Área
    int xc, yc;              // Centro-de-massa
    int perimeter;           // Perimetro
    int label;               // Etiqueta

    unsigned char *mask;     // Não usado
    unsigned char *data;     // Não usado
    int channels;            // Não usado
    int levels;              // Não usado
} OVC;

typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} RGB;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    Macros
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROTOTIPOS DE FUNÇÕES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
IVC *vc_image_new(int width, int height, int channels, int levels);
IVC *vc_image_free(IVC *image);

// FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC *vc_read_image(char *filename);
int vc_write_image(char *filename, IVC *image);

int vc_gray_negative(IVC *srcdst);
int vc_rgb_negative(IVC* srcdst);

int vc_rgb_get_red_gray(IVC *srcdst);
int vc_rgb_get_green_gray(IVC *srcdst);
int vc_rgb_get_blue_gray(IVC *srcdst);

int vc_rgb_to_gray(IVC *src, IVC *dst);

int vc_rgb_to_hsv(IVC *src, IVC *dst);

// hmin,hmax = [0, 360]; smin,smax = [0, 100]; vmin,vmax = [0, 100]
int vc_hsv_segmentation(IVC *src, IVC *dst, int hmin, int hmax, int smin,
                        int smax, int vmin, int vmax);

int vc_scale_gray_to_color_palette(IVC *src, IVC *dst);

int vc_image_white_pixel_count(IVC *src);

int vc_gray_to_binary(IVC *src, IVC *dst, int threshold);

int vc_gray_to_binary_global_mean(IVC *src, IVC *dst);

int vc_gray_to_binary_midpoint(IVC *src, IVC *dst, int kernel);

int vc_gray_to_binary_niblac(IVC *src, IVC *dst, int kernel, float k);

int vc_binary_erode(IVC *src, IVC *dst, int kernel);

int vc_binary_dilate(IVC *src, IVC *dst, int kernel);

int vc_binary_open(IVC *src, IVC *dst, int kernelErode, int kernelDilate);

int vc_binary_close(IVC *src, IVC *dst, int kernelErode, int kernelDilate);

OVC* vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels);

int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs);

int vc_blob_to_gray_scale(IVC *src, IVC *dst, int nlabels);
int vc_blob_to_gray_rgb(IVC *src, IVC *dst, int nlabels);

int vc_draw_center_of_gravity(IVC *img, OVC *blob, int comp);
int vc_draw_bounding_box(IVC *img, OVC *blob);

int vc_gray_histogram_show(IVC *src, IVC *dst);
int vc_gray_histogram_equalization(IVC *src, IVC *dst);

//converter bgr para rgb
int vc_convert_bgr_to_rgb(IVC *src, IVC *dst);