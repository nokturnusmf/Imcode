# Imcode

I recently received an email offering me unlimited, full resolution photo storage. This made me wonder if it would be possible to save arbitrary data inside an image, then upload that, giving unlimited general purpose cloud storage. I therefore created this program, and can confirm that the method did in fact work. Note however that I do not recommend using this in practice.

The general idea of the program is to read a file, then calculate an approximately square shape large enough to hold the entire data, and finally output a BMP header, followed by the data, followed by some extra padding to fill the unused part of the image.

Additionally, data is automatically compressed using [Zstandard](https://github.com/facebook/zstd) before saving, and automatically decompressed when later extracting.

## The Program Source Code, Encoded by Itself

![image](https://github.com/nokturnusmf/Imcode/assets/10729925/18bb13c7-235f-45c0-a311-28d88dbaff70)

This is an example of what files encoded by this program generally look like. Note that the image has been scaled up manually to avoid browsers attempting to upscale with interpolation, and was additionally exported as a PNG, rather than the BMP files that this program creates.

## Compiling and Running

This program requires a C++20 compiler. As data is compressed using Zstandard before saving, this must also be installed. An example compilation command might be:

```g++ imcode.cpp -std=c++20 -lzstd```

The program is run from the command line. The process is run for each argument, and will automatically extract files with a ```.bmp``` extension, and store otherwise. Note that the program does not perform any other checks. Attempting to extract from an image not created by this program will almost certainly lead to an error or crash. Existing files will be overwritten unconditionally, so use with caution.

## Disclaimer

The author does not condone, support or approve of using this tool to abuse photo storage services (or similar). Images created by this tool are very obviously not real photos, and so you may run the risk of being detected and banned, potentially causing you to lose all saved data, defeating the point of using this tool in the first place.
