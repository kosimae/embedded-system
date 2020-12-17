#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <termios.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#include "cv.h"
#include "highgui.h"

#include <linux/input.h>
#include "svm.h"
#include "bmp_header.h"

#define RGB565(r, g, b) ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)) //r,g,b를 16비트 short형으로 만들어줌.

struct svm_node *x;
int max_nr_attr = 64;
struct svm_model *model;
int predict_probability = 0;
static char *line = NULL;
static int max_line_len;

//svm predict
int print_null(const char *s, ...) { return 0; }
static int (*info)(const char *fmt, ...) = &printf;

int write_input_file(float *input)
{
    int x, y;
    FILE *mnist_input_f;
    if ((mnist_input_f = fopen("mnist_input.txt", "w")) == NULL)
    {
        return 0;
    }
    fprintf(mnist_input_f, "%d", 3);
    for (y = 0; y < 28; y++)
    {
        for (x = 0; x < 28; x++)
        {
            if (input[y * 28 + x] > 0)
            {
                fprintf(mnist_input_f, " %d:%f", (y * 28 + x), input[y * 28 + x]);
            }
        }
        //printf("\n");
    }
    fclose(mnist_input_f);
    return 1;
}

// RGB 형태의 image을 lplImage 형태로 변환
void RGB2cvIMG(unsigned short *rgb, IplImage *img, int ex, int ey)
{
    int x, y;

    for (y = 0; y < ey; y++)
    {
        for (x = 0; x < ex; x++)
        {
            (img->imageData[(y * img->widthStep) + x * 3]) = (rgb[y * ex + x] & 0x1F) << 3;               //b
            (img->imageData[(y * img->widthStep) + x * 3 + 1]) = ((rgb[y * ex + x] & 0x07E0) >> 5) << 2;  //g
            (img->imageData[(y * img->widthStep) + x * 3 + 2]) = ((rgb[y * ex + x] & 0xF800) >> 11) << 3; //r
        }
    }
}

void read_bmp(char *filename, char **pDib, char **data, int *cols, int *rows)
{
    BITMAPFILEHEADER bmpHeader;
    BITMAPINFOHEADER *bmpInfoHeader;
    unsigned int size;
    unsigned char ID[2];
    int nread;
    FILE *fp;
    fp = fopen(filename, "rb");

    if (fp == NULL)
    {
        printf("ERROR\n");
        return;
    }

    ID[0] = fgetc(fp);
    ID[1] = fgetc(fp);
    if (ID[0] != 'B' && ID[1] != 'M')
    {
        fclose(fp);
        return;
    }

    nread = fread(&bmpHeader.bfSize, 1, sizeof(BITMAPFILEHEADER), fp);
    size = bmpHeader.bfSize - sizeof(BITMAPFILEHEADER);
    *pDib = (unsigned char *)malloc(size);
    fread(*pDib, 1, size, fp);
    bmpInfoHeader = (BITMAPINFOHEADER *)*pDib;

    if (24 != bmpInfoHeader->biBitCount)
    {
        printf("It supports only 24bit bmp!\n");
        fclose(fp);
        return;
    }

    *cols = bmpInfoHeader->biWidth;
    *rows = bmpInfoHeader->biHeight;
    *data = (char *)(*pDib + bmpHeader.bfOffBits - sizeof(bmpHeader) - 2);
    fclose(fp);
}

void BMP2RGB565(unsigned short *rgbdata, char *data, int cols, int rows)
{
    int i, j, k, t;
    int r, g, b;
    int pixel;

    for (j = 0; j < rows; j++)
    {
        k = j * cols * 3;
        t = (rows - 1 - j) * cols;
        for (i = 0; i < cols; i++)
        {
            b = *(data + (k + i * 3));
            g = *(data + (k + i * 3 + 1));
            r = *(data + (k + i * 3 + 2));
            rgbdata[t + i] = (unsigned short)RGB565(r, g, b);
            // pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
            // bmpdata[t+i] = pixel;
        }
    }
}

void close_bmp(char **pDib)
{
    free(*pDib);
}

IplImage *Color_to_Grayscale_Resize(IplImage *img) //input : resize 후 colorimage 3channel
{
    int grayVal;
    IplImage *imgGray = cvCreateImage(cvSize(img->width, img->height), 8, 1);
    IplImage *small_img = cvCreateImage(cvSize(28, 28), 8, 1);
    int x, y = 0;
    cvCvtColor(img, imgGray, CV_BGR2GRAY);
    cvResize(imgGray, small_img, CV_INTER_LINEAR);
    cvReleaseImage(&imgGray);
    return small_img;
}

void cvIMG2GRAY565(IplImage *img, unsigned short *cv_rgb, int ex, int ey)
{
    int x, y;
    unsigned char r, g, b;

    for (y = 0; y < ey; y++)
    {
        for (x = 0; x < ex; x++)
        {
            b = (img->imageData[(y * img->widthStep) + x]);
            g = (img->imageData[(y * img->widthStep) + x]);
            r = (img->imageData[(y * img->widthStep) + x]);
            cv_rgb[y * 320 + x] = (unsigned short)RGB565(r, g, b);
        }
    }
}

float *Normalization(IplImage *src, float *dst)
{
    int x, y, min, max;
    min = 255;
    max = 0;

    for (y = 0; y < src->height; y++)
    {
        for (x = 0; x < src->width; x++)
        {
            if (min > src->imageData[x + y * src->widthStep])
            {
                min = src->imageData[x + y * src->widthStep];
            }
        }
    }
    for (y = 0; y < src->height; y++)
    {
        for (x = 0; x < src->width; x++)
        {
            src->imageData[x + y * src->widthStep] -= min;
        }
    }
    for (y = 0; y < src->height; y++)
    {
        for (x = 0; x < src->width; x++)
        {
            // printf("%3d ", src->imageData[x + y * src->widthStep]);
            if (max < src->imageData[x + y * src->widthStep])
            {
                max = src->imageData[x + y * src->widthStep];
            }
        }
        // printf("\n");
    }
    // printf("MAX: %d\n", max);

    for (y = 0; y < src->height; y++)
    {
        for (x = 0; x < src->width; x++)
        {
            if ((float)(src->imageData[x + y * src->widthStep]) / (float)max > 0.35)
            {
                dst[(y * src->widthStep) + x] = 0;
            }
            else
            {
                dst[(y * src->widthStep) + x] = 1 - (float)(src->imageData[x + y * src->widthStep]) / (float)max;
            }
            // printf("%4.3f ", dst[x + y * src->widthStep]);
        }
        // printf("\n");
    }
    return dst;
} //bigger than 100 ->0

static char *readline(FILE *input)
{
    int len;

    if (fgets(line, max_line_len, input) == NULL)
        return NULL;

    while (strrchr(line, '\n') == NULL)
    {
        max_line_len *= 2;
        line = (char *)realloc(line, max_line_len);
        len = (int)strlen(line);
        if (fgets(line + len, max_line_len - len, input) == NULL)
            break;
    }
    return line;
}

void exit_input_error(int line_num)
{
    fprintf(stderr, "Wrong input format at line %d\n", line_num);
    exit(1);
}

int predict(FILE *input, FILE *output)
{
    int correct = 0;
    int total = 0;
    double error = 0;
    double sump = 0, sumt = 0, sumpp = 0, sumtt = 0, sumpt = 0;

    int svm_type = svm_get_svm_type(model);
    int nr_class = svm_get_nr_class(model);
    double *prob_estimates = NULL;
    int j;

    int player_num = 0;

    if (predict_probability)
    {
        if (svm_type == NU_SVR || svm_type == EPSILON_SVR)
            info("Prob. model for test data: target value = predicted value + z,\nz: Laplace distribution e^(-|z|/sigma)/(2sigma),sigma=%g\n", svm_get_svr_probability(model));
        else
        {
            int *labels = (int *)malloc(nr_class * sizeof(int));
            svm_get_labels(model, labels);
            prob_estimates = (double *)malloc(nr_class * sizeof(double));
            fprintf(output, "labels");
            for (j = 0; j < nr_class; j++)
                fprintf(output, " %d", labels[j]);
            fprintf(output, "\n");
            free(labels);
        }
    }

    max_line_len = 1024;
    line = (char *)malloc(max_line_len * sizeof(char));
    while (readline(input) != NULL)
    {
        int i = 0;
        double target_label, predict_label;
        char *idx, *val, *label, *endptr;
        int inst_max_index = -1; // strtol gives 0 if wrong format, and precomputed kernel has <index> start from 0

        label = strtok(line, " \t\n");
        if (label == NULL) // empty line
            exit_input_error(total + 1);

        target_label = strtod(label, &endptr);
        if (endptr == label || *endptr != '\0')
            exit_input_error(total + 1);

        while (1)
        {
            if (i >= max_nr_attr - 1) // need one more for index = -1
            {
                max_nr_attr *= 2;
                x = (struct svm_node *)realloc(x, max_nr_attr * sizeof(struct svm_node));
            }

            idx = strtok(NULL, ":");
            val = strtok(NULL, " \t");

            if (val == NULL)
                break;
            errno = 0;
            x[i].index = (int)strtol(idx, &endptr, 10);
            if (endptr == idx || errno != 0 || *endptr != '\0' || x[i].index <= inst_max_index)
                exit_input_error(total + 1);
            else
                inst_max_index = x[i].index;

            errno = 0;
            x[i].value = strtod(val, &endptr);
            if (endptr == val || errno != 0 || (*endptr != '\0' && !isspace(*endptr)))
                exit_input_error(total + 1);

            ++i;
        }
        x[i].index = -1;

        if (predict_probability && (svm_type == C_SVC || svm_type == NU_SVC))
        {
            predict_label = svm_predict_probability(model, x, prob_estimates);
            fprintf(output, "%g", predict_label);
            for (j = 0; j < nr_class; j++)
                fprintf(output, " %g", prob_estimates[j]);
            fprintf(output, "\n");
        }
        else
        {
            predict_label = svm_predict(model, x);
            fprintf(output, "%.17g\n", predict_label);
        }
        player_num = (int)predict_label;
        if (predict_label == target_label)
            ++correct;
        error += (predict_label - target_label) * (predict_label - target_label);
        sump += predict_label;
        sumt += target_label;
        sumpp += predict_label * predict_label;
        sumtt += target_label * target_label;
        sumpt += predict_label * target_label;
        ++total;
    }

    if (svm_type == NU_SVR || svm_type == EPSILON_SVR)
    {
        info("Mean squared error = %g (regression)\n", error / total);
        info("Squared correlation coefficient = %g (regression)\n",
             ((total * sumpt - sump * sumt) * (total * sumpt - sump * sumt)) /
                 ((total * sumpp - sump * sump) * (total * sumtt - sumt * sumt)));
    }
    else
        //	info("Accuracy = %g%% (%d/%d) (classification)\n",
        //		(double)correct/total*100,correct,total);
        if (predict_probability)
        free(prob_estimates);

    return player_num;
}

int main()
{
    //mnist variable
    int mnist_val = 0;
    FILE *input, *output;

    int mnistC = 0, mnistR = 0;
    char *pData_mnist, *data_mnist;
    read_bmp("mnist_test.bmp", &pData_mnist, &data_mnist, &mnistC, &mnistR);
    unsigned short mnist_rgb[mnistC * mnistR * 2];
    BMP2RGB565(mnist_rgb, data_mnist, mnistC, mnistR);
    close_bmp(&pData_mnist);

    IplImage *mnist_ipl = cvCreateImage(cvSize(mnistC, mnistR), 8, 3);
    IplImage *small_mnist = NULL;
    //mnist
    float mnist_input[28 * 28] = {0};

    printf("RGB2cvIMG\n");
    RGB2cvIMG(mnist_rgb, mnist_ipl, mnistC, mnistR);

    printf("small_mnist\n");
    small_mnist = Color_to_Grayscale_Resize(mnist_ipl);
    // cvIMG2GRAY565(small_mnist, mnist_rgb, small_mnist->width, small_mnist->width);

    printf("Normalization\n");
    Normalization(small_mnist, &mnist_input);

    printf("write_input_file\n");
    write_input_file(&mnist_input);

    printf("predict_probability\n");
    predict_probability = atoi("mnist_input.txt");
    input = fopen("mnist_input.txt", "r");
    if (input == NULL)
    {
        fprintf(stderr, "can't open input file %s\n", "mnist_input.txt");
        exit(1);
    }
    output = fopen("mnist_output.file", "w");
    if (output == NULL)
    {
        fprintf(stderr, "can't open output file %s\n", "mnist_output.file");
        exit(1);
    }
    if ((model = svm_load_model("mnist.model")) == 0)
    {
        fprintf(stderr, "can't open model file %s\n", "mnist.model");
        exit(1);
    }
    x = (struct svm_node *)malloc(max_nr_attr * sizeof(struct svm_node));
    if (predict_probability)
    {
        if (svm_check_probability_model(model) == 0)
        {
            fprintf(stderr, "Model does not support probabiliy estimates\n");
            exit(1);
        }
     }
    else
    {
        if (svm_check_probability_model(model) != 0)
            info("Model supports probability estimates, but disabled in prediction.\n");
    }

    printf("predict\n");
    mnist_val = predict(input, output);
    printf("mnist val : %d\n", mnist_val);
    svm_free_and_destroy_model(&model);
  

    free(x);
    free(line);
    fclose(input);
    fclose(output);

    // ipl release
    cvReleaseImage(&mnist_ipl);
    cvReleaseImage(&small_mnist);
}
