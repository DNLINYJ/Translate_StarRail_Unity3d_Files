#include <iostream>
#include <Windows.h>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

using namespace std;

#include "magic_constants.h"
#include "AES-128.h"
#include "lz4.h"
#include "translate.h"

struct StorageBlock {
	unsigned __int32 compressedSize = 0;
	unsigned __int32 uncompressedSize = 0;
	unsigned __int16 flags = 0;
};

struct Node
{
	unsigned __int64 offset = 0;
	unsigned __int64 size = 0;
	unsigned __int16 flags = 0;
	string path;
};

struct BlockIndex
{
	int id;
	string path;
};

std::vector<std::string> split(const std::string& input, char delimiter) {
	std::vector<std::string> tokens;
	std::istringstream input_stream(input);
	std::string token;

	while (std::getline(input_stream, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

int tranlateBlock_to_normal_unity3d_file(string inpath, string outpath) {

	cout << "[Debug] �����ļ�Ŀ¼:" << inpath << endl;

	std::vector<BlockIndex> block_indices;
	int loopId = 0;

	string folderPath = outpath;
	std::filesystem::create_directory(folderPath);

	cout << "[Debug] �����ļ�Ŀ¼:" << folderPath << endl;

	fstream mhy_unity3d_file;
	mhy_unity3d_file.open(inpath, ios::in | ios::binary);

	// https://blog.huihut.com/2017/04/15/CppConditionStateAndEOF/
	while (mhy_unity3d_file.peek() != EOF) {

		cout << "[Debug] loopId:" << loopId << endl;

		// д��BlockIndex
		BlockIndex blockIndex = { loopId , folderPath + "\\" + std::to_string(loopId) + ".unity3d" };
		block_indices.push_back(blockIndex);

		// ReadHeader
		unsigned char ENCR[5];
		unsigned char bundleSize[sizeof(__int64)];
		unsigned char compressedBlocksInfoSize[sizeof(__int32)];
		unsigned char uncompressedBlocksInfoSize[sizeof(__int32)];
		unsigned char flag[sizeof(__int32)];

		mhy_unity3d_file.read(reinterpret_cast<char*>(ENCR), sizeof(ENCR));
		mhy_unity3d_file.read(reinterpret_cast<char*>(bundleSize), sizeof(bundleSize));
		mhy_unity3d_file.read(reinterpret_cast<char*>(compressedBlocksInfoSize), sizeof(compressedBlocksInfoSize));
		mhy_unity3d_file.read(reinterpret_cast<char*>(uncompressedBlocksInfoSize), sizeof(uncompressedBlocksInfoSize));
		mhy_unity3d_file.read(reinterpret_cast<char*>(flag), sizeof(flag));

		// https://stackoverflow.com/questions/34780365/how-do-i-convert-an-unsigned-char-array-into-an-unsigned-long-long
		unsigned int bundleSize_unsigned_int;
		memcpy(&bundleSize_unsigned_int, bundleSize, sizeof bundleSize);
		bundleSize_unsigned_int = _byteswap_ulong(bundleSize_unsigned_int);

		unsigned int compressedBlocksInfoSize_unsigned_int;
		memcpy(&compressedBlocksInfoSize_unsigned_int, compressedBlocksInfoSize, sizeof compressedBlocksInfoSize);
		compressedBlocksInfoSize_unsigned_int = _byteswap_ulong(compressedBlocksInfoSize_unsigned_int);

		unsigned int uncompressedBlocksInfoSize_unsigned_int = _byteswap_ulong(*(unsigned int*)uncompressedBlocksInfoSize);

		unsigned int flag_unsigned_int;
		memcpy(&flag_unsigned_int, flag, sizeof flag);
		flag_unsigned_int = _byteswap_ulong(flag_unsigned_int);

		cout << "[Debug] compressedBlocksInfoSize: " << compressedBlocksInfoSize_unsigned_int << endl;
		cout << "[Debug] uncompressedBlocksInfoSize: " << uncompressedBlocksInfoSize_unsigned_int << endl;

		std::byte* blocksInfoBytes = new std::byte[compressedBlocksInfoSize_unsigned_int];
		std::byte* uncompressedBlocksInfo = new std::byte[uncompressedBlocksInfoSize_unsigned_int];
		mhy_unity3d_file.read(reinterpret_cast<char*>(blocksInfoBytes), compressedBlocksInfoSize_unsigned_int);

		//unsigned int blocksInfoBytes_D_size = 0;
		if ((flag_unsigned_int & 0x3F) == 0x5 && compressedBlocksInfoSize_unsigned_int > 0xFF) {
			Decrypt(blocksInfoBytes, compressedBlocksInfoSize_unsigned_int);
			blocksInfoBytes += 0x14;
			compressedBlocksInfoSize_unsigned_int -= 0x14;
		}

		long long v1 = compressedBlocksInfoSize_unsigned_int;
		long long v2 = uncompressedBlocksInfoSize_unsigned_int;

		LZ4_decompress_safe((char*)blocksInfoBytes, (char*)uncompressedBlocksInfo, compressedBlocksInfoSize_unsigned_int, uncompressedBlocksInfoSize_unsigned_int);

		// �ҵ�������ڴ���� FUCK (�������ڴ渲�ǵ��¶���)
		// Ϊ�����´�������ͷ��ڴ�
		// delete[] blocksInfoBytes;

		// ReadBlocksInfoAndDirectory

		unsigned int blocksInfoCount;
		memcpy(&blocksInfoCount, uncompressedBlocksInfo, 4i64);
		blocksInfoCount = (((blocksInfoCount << 16) | blocksInfoCount & 0xFF00) << 8) | ((HIWORD(blocksInfoCount) | blocksInfoCount & 0xFF0000) >> 8);

		cout << "[Debug] blocksInfoCount:" << blocksInfoCount << endl;

		StorageBlock* m_BlocksInfo = new StorageBlock[blocksInfoCount];

		uncompressedBlocksInfo = uncompressedBlocksInfo + 4;

		for (int i = 0; i < blocksInfoCount; i++) {
			// ��С��ת��
			memcpy(&m_BlocksInfo[i].uncompressedSize, uncompressedBlocksInfo, 4i64);
			m_BlocksInfo[i].uncompressedSize = _byteswap_ulong(m_BlocksInfo[i].uncompressedSize);
			uncompressedBlocksInfo += 4i64;

			memcpy(&m_BlocksInfo[i].compressedSize, uncompressedBlocksInfo, 4i64);
			m_BlocksInfo[i].compressedSize = _byteswap_ulong(m_BlocksInfo[i].compressedSize);
			uncompressedBlocksInfo += 4i64;

			memcpy(&m_BlocksInfo[i].flags, uncompressedBlocksInfo, 2i64);
			m_BlocksInfo[i].flags = _byteswap_ushort(m_BlocksInfo[i].flags);
			uncompressedBlocksInfo += 2i64;
		}


		unsigned int nodesCount;
		if (blocksInfoCount) {
			memcpy(&nodesCount, uncompressedBlocksInfo, 4i64);
			uncompressedBlocksInfo += 4i64;
			nodesCount = (((nodesCount << 16) | nodesCount & 0xFF00) << 8) | ((HIWORD(nodesCount) | nodesCount & 0xFF0000) >> 8);
			cout << "[Debug] nodesCount:" << nodesCount << endl;
		}
		else
			return 0;

		Node* m_DirectoryInfo = new Node[nodesCount];

		unsigned int path_size = 0;
		char* v3 = new char;
		std::byte* uncompressedBlocksInfo_set;

		for (int i = 0; i < nodesCount; i++)
		{
			memcpy(&m_DirectoryInfo[i].offset, uncompressedBlocksInfo, 8i64);
			uncompressedBlocksInfo += 8i64;

			memcpy(&m_DirectoryInfo[i].size, uncompressedBlocksInfo, 8i64);
			uncompressedBlocksInfo += 8i64;

			memcpy(&m_DirectoryInfo[i].flags, uncompressedBlocksInfo, 4i64);
			m_DirectoryInfo[i].flags = _byteswap_ulong(m_DirectoryInfo[i].flags);
			uncompressedBlocksInfo += 4i64;

			uncompressedBlocksInfo_set = uncompressedBlocksInfo;

			// ReadStringToNull()
			for (int a = 0;; a++) {
				memcpy(v3, uncompressedBlocksInfo, 1i64);
				if ((int)*v3 == 0) {
					path_size++;
					uncompressedBlocksInfo++;
					break;
				}
				path_size++;
				uncompressedBlocksInfo++;
			}
			char* path_v1 = new char[path_size];
			strcpy_s(path_v1, path_size, (const char*)uncompressedBlocksInfo_set);
			m_DirectoryInfo[i].path = path_v1;
		}

		// CreateBlocksStream

		// Ϊ�����´����ΪcompressedSizeSum
		/*
		unsigned __int32 uncompressedSizeSum = 0;
		for (int i = 0; i < blocksInfoCount; i++) {
			uncompressedSizeSum += m_BlocksInfo[i].uncompressedSize;
		}

		std::byte* blocksStream = new std::byte[uncompressedSizeSum];
		*/

		unsigned __int32 compressedSizeSum = 0;
		for (int i = 0; i < blocksInfoCount; i++) {
			compressedSizeSum += m_BlocksInfo[i].compressedSize;
		}

		std::byte* blocksStream = new std::byte[compressedSizeSum];
		std::byte* blocksStream_start_address = blocksStream;

		// ReadBlocks(DecryptBlocks)

		unsigned int compressedSize;
		std::byte* compressedBytes;

		for (int i = 0; i < blocksInfoCount; i++) {
			compressedSize = m_BlocksInfo[i].compressedSize;
			compressedBytes = new std::byte[compressedSize];
			mhy_unity3d_file.read(reinterpret_cast<char*>(compressedBytes), compressedSize);
			if ((m_BlocksInfo[i].flags & 0x3F) == 0x5 && compressedSize > 0xFF) {
				Decrypt(compressedBytes, compressedSize);
				compressedBytes += 0x14;
				compressedSize -= 0x14;
				m_BlocksInfo[i].compressedSize = compressedSize;
			}
			memcpy(blocksStream, compressedBytes, compressedSize);
			blocksStream += compressedSize;

			//https://stackoverflow.com/questions/61443910/what-is-causing-invalid-address-specified-to-rtlvalidateheap-01480000-014a290
			//delete[] compressedBytes;
		}

		// ���¼�����ܺ��compressedSizeSum
		compressedSizeSum = 0;
		for (int i = 0; i < blocksInfoCount; i++) {
			compressedSizeSum += m_BlocksInfo[i].compressedSize;
		}

		// Output Unity Standard *.unity3d Files
		string unityFS = "556e697479465300"; // UnityFS\0 (8bit)
		string ArchiveVersion = "00000006"; // 6 (4bit)
		string UnityBundleVersion = "352e782e7800"; // 5.x.x\0 (6bit)
		string ABPackVersion = "323031392E342E38663100"; // 2019.4.8f1\0 (24bit)

		// ��ȡ·����Ϣ�ܴ�С
		unsigned int nodesSize = 0;
		for (int i = 0; i < nodesCount; i++)
		{
			nodesSize = nodesSize + 20 + m_DirectoryInfo[i].path.length() + 1; // +1 ����ɱ��\00
		}

		// 42 Ϊ �Ϸ�header�Ĵ�С
		// 20 Ϊ ABPackSize + compressedBlocksInfoSize + uncompressedBlocksInfoSize + flags
		// 20 Ϊ������Ϣ��16bit hash / 4bit blockCount��
		// 10 * blocksInfoCount Ϊ ��������Ϣ�ܴ�С
		// nodesSize Ϊ ·����Ϣ�ܴ�С
		// compressedSizeSum Ϊ blocks �ܴ�С
		unsigned __int64 ABPackSize = 42 + 20 + 20 + (10 * blocksInfoCount) + nodesSize + compressedSizeSum;

		fstream testoutput;
		testoutput.open(folderPath + "\\" + std::to_string(loopId) + ".unity3d", std::ios::binary | std::ios::out);
		loopId++;

		// д�� Unity3d�ļ���׼ͷ
		testoutput << translate_hex_to_write_to_file(unityFS);
		testoutput << translate_hex_to_write_to_file(ArchiveVersion);
		testoutput << translate_hex_to_write_to_file(UnityBundleVersion);
		testoutput << translate_hex_to_write_to_file(ABPackVersion);

		// ����µ�BlocksInfoBytes
		unsigned int NewBlocksInfoBytes_Size = 20 + (10 * blocksInfoCount) + 4 + nodesSize;
		std::byte* NewBlocksInfoBytes = new std::byte[NewBlocksInfoBytes_Size];
		std::byte* NewBlocksInfoBytes_Start_Address = NewBlocksInfoBytes;

		// ����ԭblockHash ��00���
		unsigned __int64 blockHash = 0;
		memcpy(NewBlocksInfoBytes, &blockHash, 8i64);
		NewBlocksInfoBytes += 8i64;
		memcpy(NewBlocksInfoBytes, &blockHash, 8i64);
		NewBlocksInfoBytes += 8i64;

		// д��blocksInfoCount
		blocksInfoCount = _byteswap_ulong(blocksInfoCount);
		memcpy(NewBlocksInfoBytes, &blocksInfoCount, 4i64);
		NewBlocksInfoBytes += 4i64;

		// д��blocksInfo
		for (int i = 0; i < _byteswap_ulong(blocksInfoCount); i++) {
			// ��С��ת��
			m_BlocksInfo[i].uncompressedSize = _byteswap_ulong(m_BlocksInfo[i].uncompressedSize);
			memcpy(NewBlocksInfoBytes, &m_BlocksInfo[i].uncompressedSize, 4i64);
			NewBlocksInfoBytes += 4i64;

			m_BlocksInfo[i].compressedSize = _byteswap_ulong(m_BlocksInfo[i].compressedSize);
			memcpy(NewBlocksInfoBytes, &m_BlocksInfo[i].compressedSize, 4i64);
			NewBlocksInfoBytes += 4i64;

			// ���flags��LZ4_MHY תΪ������LZ4 flags
			if ((m_BlocksInfo[i].flags & 0x3F) == 0x5) {
				m_BlocksInfo[i].flags = 0x43;
			}
			m_BlocksInfo[i].flags = _byteswap_ushort(m_BlocksInfo[i].flags);
			memcpy(NewBlocksInfoBytes, &m_BlocksInfo[i].flags, 2i64);
			NewBlocksInfoBytes += 2i64;
		}

		// д��nodesCount
		nodesCount = _byteswap_ulong(nodesCount);
		memcpy(NewBlocksInfoBytes, &nodesCount, 4i64);
		NewBlocksInfoBytes += 4i64;

		// д��nodes
		for (int i = 0; i < _byteswap_ulong(nodesCount); i++)
		{
			// ����nodes��offset��sizeûת�������Ϊǰ�������תС���� ���üӴ����ˣ�O��
			memcpy(NewBlocksInfoBytes, &m_DirectoryInfo[i].offset, 8i64);
			NewBlocksInfoBytes += 8i64;

			memcpy(NewBlocksInfoBytes, &m_DirectoryInfo[i].size, 8i64);
			NewBlocksInfoBytes += 8i64;
			//m_DirectoryInfo[i].size = _byteswap_uint64(m_DirectoryInfo[i].size);

			m_DirectoryInfo[i].flags = _byteswap_ulong(m_DirectoryInfo[i].flags);
			memcpy(NewBlocksInfoBytes, &m_DirectoryInfo[i].flags, 4i64);
			NewBlocksInfoBytes += 4i64;

			char* path_v1 = new char[m_DirectoryInfo[i].path.length()];
			path_v1 = (char*)m_DirectoryInfo[i].path.c_str();
			memcpy(NewBlocksInfoBytes, path_v1, m_DirectoryInfo[i].path.length() + 1);// +1 ����ɱ��\00
			NewBlocksInfoBytes += m_DirectoryInfo[i].path.length() + 1;
		}

		// ���µ�BlocksInfoBytes��LZ4ѹ��
		unsigned int max_dst_size = LZ4_compressBound(NewBlocksInfoBytes_Size);
		std::byte* NewBlocksInfoBytes_LZ4 = new std::byte[max_dst_size];
		unsigned int NewBlocksInfoBytes_compSize = LZ4_compress_default((char*)NewBlocksInfoBytes_Start_Address, (char*)NewBlocksInfoBytes_LZ4, NewBlocksInfoBytes_Size, max_dst_size);

		// �ͷ��ڴ� ����ڴ汬ը
		delete[] NewBlocksInfoBytes_Start_Address;
		delete[] blocksInfoBytes;
		delete[] m_DirectoryInfo;
		delete[] m_BlocksInfo;

		// С��ת���
		ABPackSize = _byteswap_uint64(ABPackSize);
		testoutput.write((const char*)&ABPackSize, sizeof(unsigned __int64));

		NewBlocksInfoBytes_compSize = _byteswap_ulong(NewBlocksInfoBytes_compSize);
		testoutput.write((const char*)&NewBlocksInfoBytes_compSize, sizeof(unsigned int));

		NewBlocksInfoBytes_Size = _byteswap_ulong(NewBlocksInfoBytes_Size);
		testoutput.write((const char*)&NewBlocksInfoBytes_Size, sizeof(unsigned int));

		string ArchiveFlags = "00000043";// LZ4ѹ��flags
		testoutput << translate_hex_to_write_to_file(ArchiveFlags);

		// д��blocksInfoBytes
		testoutput.write((const char*)NewBlocksInfoBytes_LZ4, _byteswap_ulong(NewBlocksInfoBytes_compSize));
		// �ͷ��ڴ� ����ڴ汬ը
		delete[] NewBlocksInfoBytes_LZ4;

		// д��blocksStream
		testoutput.write((const char*)blocksStream_start_address, compressedSizeSum);
		// �ͷ��ڴ� ����ڴ汬ը
		delete[] blocksStream_start_address;

	}

	std::string blockIndexFilename = folderPath + "\\" + "BlockIndex.bin";
	std::ofstream blockIndexFile(blockIndexFilename, std::ios::binary);

	if (blockIndexFile.is_open()) {
		for (const auto& blockIndex : block_indices) {
			int path_length = static_cast<int>(blockIndex.path.size());

			// д��id
			blockIndexFile.write(reinterpret_cast<const char*>(&blockIndex.id), sizeof(int));

			// д��path�ĳ���
			blockIndexFile.write(reinterpret_cast<const char*>(&path_length), sizeof(int));

			// д��path
			blockIndexFile.write(blockIndex.path.data(), path_length);
		}

		blockIndexFile.close();
		std::cout << "[-] Saved BlockIndex instances to file: " << blockIndexFilename << std::endl;
	}

	return 0;
}