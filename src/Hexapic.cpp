#include <QtCore>
#include <QImage>

#include "Version.hpp"
#include "utils/Verbose.hpp"
#include "HexaCrawler.hpp"
#include "HexaMosaic.hpp"

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
  printf("Usage: hexapic [-i DIR1 -o DIR2 -t N [-g GAMMA] [-v] [-f]]\n");
  printf("               [-i IMG -d DIR -p DIMS -w WIDTH -r RADIUS -c COLOR_RATIO [-v]]\n\n");
  printf("This program can do two things, create a database\n");
  printf("of images or create a hexagonal mosaic.\n");
  printf("Examples: hexapic -i images/ -o database/ -t 100 -v\n");
  printf("          hexapic -i input.jpg -d database/ -w 20 -r 8 -c 0.5\n\n");

  printf("General options:\n");
  printf(" -h\tdisplay this help message\n");
  printf(" -v\tdisplay version\n\n");

  printf("Crawling options:\n");
  printf(" -i\tinput directory DIR1 to crawl for images\n");
  printf(" -o\toutput directory DIR2 to store database\n");
  printf(" -t\ttile size N of database images, N > 0\n");
  printf(" -g\tapply gamma correction GAMMA\n");
  printf(" -f\tuse fast resizing algorithm\n");
  printf(" -v\tenable verbosity information\n\n");

  printf("Mosaic options:\n");
  printf(" -i\timage IMG to create mosaic from\n");
  printf(" -d\tdatabase directory DIR containing the images\n");
  printf(" -w\twidth WIDTH of the output image in hexagons, WIDTH > 0\n");
  printf(" -r\tradius RADIUS in hexagons between equal images, RADIUS >= 0\n");
  printf(" -c\tcolor balancing ratio COLOR_RATIO in [0, 1]\n");
  printf(" -p\tpca dimensions DIMS in {1,...,100}\n");
  printf(" -v\tenable verbosity information\n");
  exit(code);
}

int main(int argc, char *argv[])
{
  char *image_dir       = 0;
  char *input_image     = 0;
  char *output_dir      = 0;
  char *database_dir    = 0;
  int tile_size         = 0;
  int width_in_hexagons = 0;
  int min_radius        = 0;
  int pca_dimensions    = 0;
  float cb_ratio        = 0.0f;
  float gamma           = 1.0f;
  bool fast             = false;

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
  if (argc >= 7 && argc <= 11)
  {
    while ((c = getopt(argc, argv, "i:o:t:g:vf")) != -1)
    {
      switch(c)
      {
        case 'i': image_dir = optarg; break;
        case 'o': output_dir = optarg; break;
        case 't': tile_size = atoi(optarg); break;
        case 'v': Verbose::SetVerbosity(Verbose::DBG); break;
        case 'f': fast = true; break;
        case 'g': gamma = atof(optarg); break;
        case '?': default: PrintHelpAndExit(EXIT_FAILURE);
      }
    }

    QDir input(image_dir);
    if (!input.exists())
      FatalLine("Error: Directory `" << image_dir << "' does not exist");

    QDir output(output_dir);
    if (output.exists())
      WarningLine("Warning: Directory `" << output_dir << "' already exists");
    else
    if (!QDir().mkpath(output.absolutePath()))
      FatalLine("Error: Directory `" << output_dir << "' could not be created");

    if (tile_size <= 0)
      FatalLine("Error: Tile size `" << tile_size << "' should be > 0");

    HexaCrawler hc;
    hc.Crawl(input, output, tile_size, fast, gamma);
  }
  else
  if (argc >= 12 && argc <= 14)
  {
    while ((c = getopt(argc, argv, "i:d:w:r:c:p:v")) != -1)
    {
      switch(c)
      {
        case 'i': input_image = optarg; break;
        case 'd': database_dir = optarg; break;
        case 'w': width_in_hexagons = atoi(optarg); break;
        case 'r': min_radius = atoi(optarg); break;
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

    if (min_radius < 0)
      FatalLine("Error: Minimal radius should be >= 0");

    if (cb_ratio < 0.0f || cb_ratio > 1.0f)
      FatalLine("Error: Color balance ratio should be between 0 and 1");

    if (width_in_hexagons <= 0)
      FatalLine("Error: Width should be > 0");

    if (pca_dimensions < 1 || pca_dimensions > 100)
      FatalLine("Error: PCA dimensions should be between 1 and 100");

    //HexaMosaic hm(input_image, database_dir, width_in_hexagons, min_radius, cb_ratio);
    //hm.Create();
  }
  else PrintHelpAndExit(EXIT_FAILURE);

  return EXIT_SUCCESS;
}
