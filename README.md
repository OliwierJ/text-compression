# Huffman Compression Tool

A simple C++ implementation of **Huffman coding** for compressing and decompressing text files. The program builds a Huffman tree from character frequencies, encodes the text into a compact binary format, and can fully restore it.

---

## Usage

### Compress
program.exe <input_text_file> [-o <output_file>]

makefile
Copy code
Example:
program.exe input.txt -o output.huff

csharp
Copy code
If `-o` is omitted, the default output is `<input>.huff`.

### Decompress
program.exe --decode <input_huff_file> [-o <output_text_file>]

makefile
Copy code
Example:
program.exe --decode input.huff -o restored.txt

yaml
Copy code

---

## How It Works

- Counts character frequencies  
- Builds a Huffman tree using a priority queue  
- Generates bit codes for each character  
- Serializes the tree + encoded data into a binary stream  
- Fully decodes serialized data back to text  

---

## Build

Requires Windows (uses WinAPI for filename encoding).

### g++
g++ -std=c++17 -O2 -o huffman main.cpp

shell
Copy code

### MSVC
cl /std:c++17 main.cpp

yaml
Copy code

---

## Notes

- Designed for text files, not arbitrary binary data  
- Produces a custom `.huff` file format  
- Windows-specific due to filename encoding logic  
