#include <QtCore>
#include <QImage>

#include "../utils/Verbose.hpp"
#include "Version.hpp"
#include "HexaMosaic.hpp"

#include <unistd.h>

void PrintVersionAndExit(const int code)
{
  printf("%s\n\n", HUMAN_NAME);
  printf("  Compiler %s\n", CXX_COMPILER);
  printf("  Flags    %s\n", CXX_FLAGS);
  printf("  Author   F. Huizinga (Error323)\n");
  exit(code);
}

void PrintHelpAndExit(const int code)
{
  printf("Usage: hexamosaic -i IMG -d DIR -p DIMS -w WIDTH [-c COLOR_RATIO] [-v]\n\n");
  printf("Create a hexagonal mosaic from the input image using the database.\n");
  printf("Examples: hexamosaic -i input.jpg -d database/ -w 20 -p 8\n");
  printf("          hexamosaic -i input.jpg -d database/ -w 10 -c 0.8 -p 16 -v\n\n");

  printf("General options:\n");
  printf(" -h\tdisplay this help message\n");
  printf(" -v\tdisplay version\n\n");

  printf("Mosaic options:\n");
  printf(" -i\timage IMG to create mosaic from\n");
  printf(" -d\tdatabase directory DIR containing the images\n");
  printf(" -w\twidth WIDTH of the output image in hexagons, WIDTH > 0\n");
  printf(" -c\tcolor balancing ratio COLOR_RATIO in [0, 1]\n");
  printf(" -p\tpca dimensions DIMS in {1,...,100}\n");
  printf(" -v\tenable verbosity information\n");

  exit(code);
}

int main(int argc, char *argv[])
{
  char *input_image     = 0;
  char *database_dir    = 0;
  int width_in_hexagons = 0;
  int pca_dimensions    = 0;
  float cb_ratio        = 0.0f;

  int c;
  if (argc == 2)
  {
    while ((c = getopt(argc, argv, "hv")) != -1)
    {
      switch(c)
      {
        case 'h': PrintHelpAndExit(EXIT_SUCCESS);
        case 'v': PrintVersionAndExit(EXIT_SUCCESS);
        case '?': default: PrintHelpAndExit(EXIT_FAILURE);
      }
    }
  }
  else
  if (argc >= 9 && argc <= 12)
  {
    while ((c = getopt(argc, argv, "i:d:w:c:p:v")) != -1)
    {
      switch(c)
      {
        case 'i': input_image = optarg; break;
        case 'd': database_dir = optarg; break;
        case 'w': width_in_hexagons = atoi(optarg); break;
        case 'c': cb_ratio = atof(optarg); break;
        case 'v': Verbose::SetVerbosity(Verbose::DBG); break;
        case 'p': pca_dimensions = atoi(optarg); break;
        case '?': default: PrintHelpAndExit(EXIT_FAILURE);
      }
    }

    QImage image(input_image);
    if (image.isNull())
      FatalLine("Error: Image `" << input_image << "' is invalid");

    QDir dir(database_dir);
    if (!dir.exists())
      FatalLine("Error: Database directory `" << database_dir << "' is invalid");

    if (cb_ratio < 0.0f || cb_ratio > 1.0f)
      FatalLine("Error: Color balance ratio should be between 0 and 1");

    if (width_in_hexagons <= 0)
      FatalLine("Error: Width should be > 0");

    if (pca_dimensions < 1 || pca_dimensions > 100)
      FatalLine("Error: PCA dimensions should be between 1 and 100");

    HexaMosaic hm(image, dir, width_in_hexagons, pca_dimensions, cb_ratio);
    hm.Create();
  }
  else PrintHelpAndExit(EXIT_FAILURE);

  return EXIT_SUCCESS;
}
