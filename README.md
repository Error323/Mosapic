Mosapic
=======
A program that can construct a mosaic given an input image and a database.
Secondly, it can also produce a database by recursive crawling a directory.

mosacrawler:
------------
Usage: mosacrawler -i DIR1 -o DIR2 -t N [-g GAMMA] [-v] [-f]

Crawls a directory for images and creates a database from them.
Examples: mosacrawler -i images/ -o database/ -t 100
          mosacrawler -i images/ -o database/ -t 128 -v -f
          mosacrawler -i images/ -o database/ -t 256 -g 2.2 -f

General options:
 -h	display this help message
 -v	display version

Crawling options:
 -i	input directory DIR1 to crawl for images
 -o	output directory DIR2 to store database
 -t	tile size N of database images, N > 0
 -g	apply gamma correction GAMMA
 -f	use fast resizing algorithm
 -v	enable verbosity information

mosapic:
--------
Usage: mosamosaic -i IMG -d DIR -p DIMS -w WIDTH [-c COLOR_RATIO] [-v]

Create a tile mosaic from the input image using the database.
Examples: mosapic -i input.jpg -d database/ -w 20 -p 8
          mosapic -i input.jpg -d database/ -w 10 -c 0.8 -p 16 -v

General options:
 -h	display this help message
 -v	display version

Mosaic options:
 -i	image IMG to create mosaic from
 -d	database directory DIR containing the images
 -w	width WIDTH of the output image in tiles, WIDTH > 0
 -c	color balancing ratio COLOR_RATIO in [0, 1]
 -p	pca dimensions DIMS in {1,...,100}
 -v	enable verbosity information

