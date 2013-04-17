#include <QtCore>
#include <QImage>
#include <unistd.h>

#include "../utils/Verbose.hpp"
#include "Version.hpp"
#include "MosaCrawler.hpp"

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
  printf("Usage: mosacrawler -i DIR1 -o DIR2 -t N [-g GAMMA] [-v] [-f]\n\n");
  printf("Crawls a directory for images and creates a database from them.\n");
  printf("Examples: mosacrawler -i images/ -o database/ -t 100\n");
  printf("          mosacrawler -i images/ -o database/ -t 128 -v -f\n");
  printf("          mosacrawler -i images/ -o database/ -t 256 -g 2.2 -f\n\n");

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

  exit(code);
}

int main(int argc, char *argv[])
{
  char *image_dir       = 0;
  char *output_dir      = 0;
  int tile_size         = 0;
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

    MosaCrawler hc;
    hc.Crawl(input, output, tile_size, fast, gamma);
  }
  else PrintHelpAndExit(EXIT_FAILURE);

  return EXIT_SUCCESS;
}
