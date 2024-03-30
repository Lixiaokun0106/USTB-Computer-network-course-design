#include "AES.h"

// 四个字节合成一个字
_word AES::Word(_byte& k1, _byte& k2, _byte& k3, _byte& k4) {
	_word res;
	for (int i = 0; i < 8; i++) {
		res[i] = k1[i];
		res[i + 8] = k2[i];
		res[i + 16] = k3[i];
		res[i + 24] = k4[i];
	}
	return res;
}

// 对输入word中的每一个字节进行S-盒变换
_word AES::SubWord(_word sw) {
	_word res;
	for (int i = 0; i < 32; i += 8) {
		int row = sw[i + 7] * 8 + sw[i + 6] * 4 + sw[i + 5] * 2 + sw[i + 4];
		int col = sw[i + 3] * 8 + sw[i + 2] * 4 + sw[i + 1] * 2 + sw[i];
		_byte temp = S_Box[row][col];
		for (int j = 0; j < 8; j++)
			res[i + j] = temp[j];
	}
	return res;
}

// 按字节 循环左移一位,即把[a0, a1, a2, a3]变成[a1, a2, a3, a0]
_word AES::RotWord(_word rw) {
	return rw >> 8 | rw << 24;
}

// 密钥扩展函数 - 对128位密钥进行扩展得到 w[4 * (Nr + 1)]
void AES::KeyExpansion(_byte key[4 * Nk], _word w[4 * (Nr + 1)]) {
	int i = 0;
	while (i < 4) {
		w[i] = Word(key[4 * i], key[4 * i + 1], key[4 * i + 2], key[4 * i + 3]);
		i++;
	}

	_word temp;
	while (i < 4 * (Nr + 1)) {
		temp = w[i - 1];
		if (i % Nk == 0)
			temp = SubWord(RotWord(temp)) ^ Rcon[i / Nk];
		w[i] = w[i - Nk] ^ temp;
		i++;
	}

}

// S盒变换 - 前4位为行号，后4位为列号
void AES::SubBytes(_byte mtx[4 * 4]) {
	for (int i = 0; i < 16; i++) {
		int row = mtx[i][7] * 8 + mtx[i][6] * 4 + mtx[i][5] * 2 + mtx[i][4];
		int col = mtx[i][3] * 8 + mtx[i][2] * 4 + mtx[i][1] * 2 + mtx[i][0];
		mtx[i] = S_Box[row][col];
	}
}


// 行变换 - 按字节循环移位
void AES::ShiftRows(_byte mtx[4 * 4]) {
	_byte temp = mtx[4];
	for (int i = 4; i < 7; i++) {
		mtx[i] = mtx[i + 1];
	}
	mtx[7] = temp;

	for (int i = 8; i < 10; i++) {
		temp = mtx[i];
		mtx[i] = mtx[i + 2];
		mtx[i + 2] = temp;
	}

	/*******需要从后往前赋值*******/
	temp = mtx[15];
	for (int i = 15; i > 12; i--) {
		mtx[i] = mtx[i - 1];
	}
	mtx[12] = temp;
}


// 有限域上的乘法 GF(2^8)
_byte AES::GFMul(_byte a, _byte b) {
	_byte res(0x00), temp;
	for (int i = 0; i < 8; i++) {
		// 说明要加a
		if ((b & _byte(0x1)) != 0) res ^= a;

		// 取最高位判断是否为1，需要进位
		temp = a & _byte(0x80);
		a <<= 1;

		// 如果非0，说明a要进位，模2加0x1b
		if (temp != 0) a ^= _byte(0x1b);
		b >>= 1;
	}
	return res;
}


// 列变换
void AES::MixColumns(_byte mtx[4 * 4]) {
	_byte arr[4];
	for (int i = 0; i < 4; i++) {
		// 提取每一列
		for (int j = 0; j < 4; j++)
			arr[j] = mtx[i + 4 * j];

		// 为下一步计算做准备而清0
		for (int j = 0; j < 4; j++)
			mtx[i + 4 * j] = 0;

		// 按照列变换表按列相乘
		for (int j = 0; j < 4; j++)
			for (int k = 0; k < 4; k++)
				mtx[i + j * 4] ^= GFMul(coff[j * 4 + k], arr[k]);

	}
}

// 轮密钥加变换 - 将每一列与扩展密钥进行异或
void AES::AddRoundKey(_byte mtx[4 * 4], _word k[4]) {
	_word k1, k2, k3, k4;
	for (int i = 0; i < 4; i++) {
		// 取出0-7位
		k1 = k[i] >> 24;
		// 取出8-15位
		k2 = (k[i] << 8) >> 24;
		// 取出16-23位
		k3 = (k[i] << 16) >> 24;
		// 取出24-31位
		k4 = (k[i] << 24) >> 24;

		mtx[i] ^= _byte(k1.to_ulong());
		mtx[i + 4] ^= _byte(k2.to_ulong());
		mtx[i + 8] ^= _byte(k3.to_ulong());
		mtx[i + 12] ^= _byte(k4.to_ulong());
	}
}


// 加密
void AES::encrypt(_byte in[4 * 4], _byte usekey[4 * 4]) {
	KeyExpansion(usekey, K);
	_word key[4];
	for (int i = 0; i < 4; i++) {
		key[i] = K[i];
	}
	AddRoundKey(in, key);

	for (int i = 1; i < Nr; i++) {
		SubBytes(in);
		ShiftRows(in);
		MixColumns(in);
		for (int j = 0; j < 4; j++) {
			key[j] = K[4 * i + j];
		}
		AddRoundKey(in, key);
	}

	SubBytes(in);
	ShiftRows(in);
	for (int i = 0; i < 4; i++) {
		key[i] = K[4 * Nr + i];
	}
	AddRoundKey(in, key);
}


// 逆S盒变换
void AES::InvSubBytes(_byte mtx[4 * 4])
{
	for (int i = 0; i < 16; ++i)
	{
		int row = mtx[i][7] * 8 + mtx[i][6] * 4 + mtx[i][5] * 2 + mtx[i][4];
		int col = mtx[i][3] * 8 + mtx[i][2] * 4 + mtx[i][1] * 2 + mtx[i][0];
		mtx[i] = Inv_S_Box[row][col];
	}
}


// 逆行变换 - 以字节为单位循环右移
void AES::InvShiftRows(_byte mtx[4 * 4])
{
	// 第二行循环右移一位
	_byte temp = mtx[7];
	for (int i = 3; i > 0; --i)
		mtx[i + 4] = mtx[i + 3];
	mtx[4] = temp;
	// 第三行循环右移两位
	for (int i = 0; i < 2; ++i)
	{
		temp = mtx[i + 8];
		mtx[i + 8] = mtx[i + 10];
		mtx[i + 10] = temp;
	}
	// 第四行循环右移三位
	temp = mtx[12];
	for (int i = 0; i < 3; ++i)
		mtx[i + 12] = mtx[i + 13];
	mtx[15] = temp;
}

void AES::InvMixColumns(_byte mtx[4 * 4])
{
	_byte arr[4];
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
			arr[j] = mtx[i + j * 4];

		mtx[i] = GFMul(0x0e, arr[0]) ^ GFMul(0x0b, arr[1])
			^ GFMul(0x0d, arr[2]) ^ GFMul(0x09, arr[3]);
		mtx[i + 4] = GFMul(0x09, arr[0]) ^ GFMul(0x0e, arr[1])
			^ GFMul(0x0b, arr[2]) ^ GFMul(0x0d, arr[3]);
		mtx[i + 8] = GFMul(0x0d, arr[0]) ^ GFMul(0x09, arr[1])
			^ GFMul(0x0e, arr[2]) ^ GFMul(0x0b, arr[3]);
		mtx[i + 12] = GFMul(0x0b, arr[0]) ^ GFMul(0x0d, arr[1])
			^ GFMul(0x09, arr[2]) ^ GFMul(0x0e, arr[3]);
	}
}

// 解密
void AES::decrypt(_byte in[4 * 4], _byte usekey[4 * 4]) {
	KeyExpansion(usekey, K);
	_word key[4];
	for (int i = 0; i < 4; i++) {
		key[i] = K[4 * Nr + i];
	}
	AddRoundKey(in, key);

	for (int i = Nr - 1; i > 0; i--) {
		InvShiftRows(in);
		InvSubBytes(in);
		for (int j = 0; j < 4; j++) {
			key[j] = K[4 * i + j];
		}
		AddRoundKey(in, key);
		InvMixColumns(in);
	}

	InvShiftRows(in);
	InvSubBytes(in);
	for (int i = 0; i < 4; ++i)
		key[i] = K[i];
	AddRoundKey(in, key);
}

// 将一个char字符数组转化为二进制,存到一个 byte 数组中
void AES::charToByte(_byte out[16], const char s[16])
{
	for (int i = 0; i < 16; ++i)
		for (int j = 0; j < 8; ++j)
			out[i][j] = ((s[i] >> j) & 1);
}

// 将连续的128位分成16组，存到一个 byte 数组中
void AES::divideToByte(_byte out[16], bitset<128>& data)
{
	bitset<128> temp;
	for (int i = 0; i < 16; ++i)
	{
		temp = (data << 8 * i) >> 120;
		out[i] = temp.to_ulong();
	}
}

// 将16个 byte 合并成连续的128位 
bitset<128> AES::mergeByte(_byte in[16])
{
	bitset<128> res;
	res.reset();  // 置0
	bitset<128> temp;
	for (int i = 0; i < 16; ++i)
	{
		temp = in[i].to_ulong();
		temp <<= 8 * (15 - i);
		res |= temp;
	}
	return res;
}

// 加密文件
void AES::encryptFile(_byte usekey[4 * 4], string fileName, string en_fileName) {
	bitset<128> data;
	_byte plain[16];
	
	ifstream in;
	ofstream out;
	in.open(fileName, ios::binary);
	out.open(en_fileName, ios::binary);

	// 移动文件指针到文件末尾，得到文件的长度后，复原文件指针
	in.seekg(0, ios::end);
	int length = in.tellg(), cur;
	in.seekg(0, ios::beg);

	while (in.read((char*)&data, sizeof(data)))
	{
		divideToByte(plain, data);
		encrypt(plain, usekey);
		data = mergeByte(plain);
		out.write((char*)&data, sizeof(data));

		// 找到目前文件指针所在位置，判断剩余文件流是否大于等于一个data字节大小，小于的话直接跳出循环
		cur = in.tellg();
		if (cur + 16 > length) break;

		data.reset();  // 置0
	}

	//// 得到剩余文件流字节大小
	//int num = length - cur;
	////初始化一段内存
	//char rest[8] = "1";
	//// 将剩余的文件读入到该内存中
	//in.read(rest, num);
	//// 将读入内存的部分写入另一个文件
	//out.write(rest, num);
	// 得到剩余文件流字节大小
	int num = length - cur;
	//初始化一段内存
	char* rest = new char[num];
	// 将剩余的文件读入到该内存中
	in.read(rest, num);
	// 将读入内存的部分写入另一个文件
	out.write(rest, num);
	delete[] rest;

	in.close();
	out.close();

}

// 解密文件
void AES::decryptFile(_byte usekey[4 * 4], string fileName, string de_fileName) {
	bitset<128> data;
	_byte plain[16];
	// 将文件 flower.jpg 加密到 cipher.txt 中
	ifstream in;
	ofstream out;
	// 解密 cipher.txt，并写入图片 flower1.jpg
	in.open(fileName, ios::binary);
	out.open(de_fileName, ios::binary);

	// 移动文件指针到文件末尾，得到文件的长度后，复原文件指针
	in.seekg(0, ios::end);
	int length = in.tellg(), cur;
	in.seekg(0, ios::beg);

	while (in.read((char*)&data, sizeof(data)))
	{
		divideToByte(plain, data);
		decrypt(plain, usekey);
		data = mergeByte(plain);
		out.write((char*)&data, sizeof(data));

		// 找到目前文件指针所在位置，判断剩余文件流是否大于等于一个data字节大小，小于的话直接跳出循环
		cur = in.tellg();
		if (cur + 16 > length) break;

		data.reset();  // 置0
	}

	//// 得到剩余文件流字节大小
	//int num = length - cur;
	////初始化一段内存
	//char rest[8] = "1";
	//// 将剩余的文件读入到该内存中
	//in.read(rest, num);
	//// 将读入内存的部分写入另一个文件
	//out.write(rest, num);
		// 得到剩余文件流字节大小
	int num = length - cur;
	//初始化一段内存
	char* rest = new char[num];
	// 将剩余的文件读入到该内存中
	in.read(rest, num);
	// 将读入内存的部分写入另一个文件
	out.write(rest, num);
	delete[] rest;

	in.close();
	out.close();

}

// 加密后编码
void AES::encryptAndEncode(_byte key[4 * 4], string fileName, string en_fileName)
{
	// AES加密文件
	encryptFile(key, fileName, "AESen_" + fileName);

	// 加密后的文件进行Base64编码
	char c;
	ifstream encodedFile("AESen_" + fileName, ios::binary);
	ofstream base64File(en_fileName);
	if (!encodedFile || !base64File)
	{
		cout << "文件打开失败" << endl;
		return;
	}
	string encodedData;
	while (encodedFile.get(c))
	{
		encodedData.push_back(c);
	}
	string base64Data = base64_encode(encodedData);
	base64File << base64Data;
	encodedFile.close();
	base64File.close();


	// 删除中间文件
	system(("del AESen_" + fileName).c_str());

}

// 解码后解密
void AES::decodeAndDecrypt(_byte key[4 * 4], string fileName, string de_fileName)
{
	// fileName内容进行Base64解码，解码后的文件命名为 "Base64de_"+fileName
	char c;
	ifstream baseFile(fileName, ios::binary);
	ofstream decodedFile("Base64de_" + fileName, ios::binary);
	if (!baseFile || !decodedFile)
	{
		cout << "文件打开失败" << endl;
		return;
	}
	string baseData;
	while (baseFile.get(c))
	{
		baseData.push_back(c);
	}
	string decodedData = base64_decode(baseData);
	decodedFile << decodedData;
	baseFile.close();
	decodedFile.close();

	// 将解码后的文件进行AES解密，解密后的文件命名为 de_fileName
	decryptFile(key, "Base64de_" + fileName, de_fileName);

	// 删除中间文件
	system(("del Base64de_" + fileName).c_str());
}


