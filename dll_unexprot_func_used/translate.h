#pragma once
#include <string>
using namespace std;
int tranlate_to_normal_unity3d_file(string inpath, string outpath);
void Decrypt(std::byte* bytes, unsigned int size);
std::string translate_hex_to_write_to_file(std::string hex);