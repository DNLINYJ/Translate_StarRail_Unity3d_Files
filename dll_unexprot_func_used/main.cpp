#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

#include "translate.h"
#include "translateBlock.h"


void main(int argc, char** argv) {
	if (argc == 1) {
		cout << endl;

		cout << "将星穹铁道Unity3d文件转为正常Unity3d文件 By:Sakura Nyoru" << endl;
		//cout << "本程序仅在内群传播，禁止外传！！！" << endl;
		cout << endl;
		cout << "参数: <inpath> <outpath> <isblock>" << endl;
		cout << "inpath : 加密文件路径" << endl;
		cout << "outpath : 解密文件路径" << endl;
		cout << "isblock : 如果是block文件 请输入任意字符串 :P [针对三测之后版本的修补]" << endl;
		cout << endl;

	}
	else {
		string args[4];
		for (int i = 0; i < argc; i++) {
			args[i] = argv[i];
		}
		if (!args[3].empty()) {
			tranlateBlock_to_normal_unity3d_file(args[1], args[2]);
		}
		else {
			tranlate_to_normal_unity3d_file(args[1], args[2]);
		}
	}
}
