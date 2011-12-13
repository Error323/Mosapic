Hexapic
=======

Constructs a hexagonal mosaic from images.


Generic options:
----------------
* -v [ --version ]  print version string
* -h [ --help ]     produce help message


Crawler options:
----------------
* -i [ --image-dir ]   arg image directory
* -o [ --output-dir ]  arg cache directory and database
* -t [ --tile-size ]   arg (=100) tile size


Hexapic options:
----------------
* --input-image  arg       source image
* --database     arg       database directory
* --width        arg       width in tile size
* --height       arg       height in tile size
* --grayscale              use grayscale
* --dimensions   arg (=8)  pca dimensions
* --min-radius   arg (=5)  min radius between duplicates
