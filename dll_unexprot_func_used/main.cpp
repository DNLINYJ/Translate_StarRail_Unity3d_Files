#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

#include "translate.h"
#include "translateBlock.h"


void main(int argc, char** argv) {
	if (argc == 1) {
		cout << endl;

		cout << "���������Unity3d�ļ�תΪ����Unity3d�ļ� By:����С����" << endl;
		cout << "�����������Ⱥ��������ֹ�⴫������" << endl;
		cout << endl;
		cout << "����: <inpath> <outpath> <isblock>" << endl;
		cout << "inpath : �����ļ�·��" << endl;
		cout << "outpath : �����ļ�·��" << endl;
		cout << "isblock : �����block�ļ� �����������ַ��� :P [�������֮��汾���޲�]" << endl;
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