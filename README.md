# photo-fingerprint

Extremely naive attempt to find duplicate images, from one smaller set to a
larger set of images (required, as all of the "fingerprints" need to fit in
memory currently).

Very similar to the thumbnail compare methology listed here:
http://www.imagemagick.org/Usage/compare/#methods

## Pre-requisites

* cmake 3.11+
* ImageMagick 7 installed from Homebrew
* ufraw installed from Homebrew (for converting CR2 files)
* boost installed from Homebrew

## Compiling

```
cmake .
make
```

## Running

There are several modes:
* generate fingerprints (`-g`)
* find duplicates (`-f`)
* extract metadata (`-m`)

All modes require a source directory, and the first two also require a destination.
All modes support concurrency via C++ threads, and the concurrency will default
to the number of system cores (returned by `std::thread::hardware_concurrency()`)
or can be set with `-n`.

Traversing the source and destination directories for reads will always descend into
subdirectories.

For duplicate finding, you can set the "fuzz factor" (distance between two colours
to treat them as the same colour) with `-u`. I'm still not certain what the units are
exactly.

### Examples

Generate some fingerprints. The destination directory must already exist.
```
./photo-fingerprint -g -s ~/Photos/duplicates/ -d ~/fingerprints/
```

Check the fingerprints against your corpus of images.
```
./photo-fingerprint -f -d ~/Photos/ -s ~/fingerprints/
```

# Problems

There are numerous challenges with this approach to finding duplicates.
* Differences in resolution, image format, colour space, colour depth,
  etc which make direct comparison between two binary files difficult.
* Re-encoding and adding or removing metadata which changes the binary
  structure of the images (i.e. you can't do a simple hash of the file
  and compare them bit-for-bit).
* Resizing and saving a "fingerprint" is dependent on the compile options
  of ImageMagick - if HDRI is enabled, and depending on the colour depth
  option it may save the images with different precision, making
  comparisons on pixel colour not be equivalent between the fingerprint
  and another image. I've tried to work around this by saving as TIF format
  without any compression and forcing floating-point "quantum" format.
* Raw images (e.g. CR2 format) may have all kinds of colour corrections
  applied that may not be part of another JPG image that is a duplicate.
  I've noticed that even in a couple of mild cases the colours seemed quite
  different, and there was some small amount of camera rotation correction
  applied between the CR2 and the JPG, making pixel-by-pixel comparisons
  difficult. Even structural similarity then wouldn't take into account
  all the differences. Maybe both SSIM and absolute error metrics should
  be used together?
* In general it would be nice to apply all of the profile/rotation/etc
  transformations from the source file so that the actual raw pixel data
  in the fingerprint and the image to be compared was identical, but I'm
  not sure ImageMagick has such an operation. It would require understanding
  that such transformations should be looked for, retrieving them and then
  applying them consistently while processing the images.
