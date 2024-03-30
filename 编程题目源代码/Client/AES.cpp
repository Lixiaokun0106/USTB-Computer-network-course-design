#include "AES.h"

// �ĸ��ֽںϳ�һ����
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

// ������word�е�ÿһ���ֽڽ���S-�б任
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

// ���ֽ� ѭ������һλ,����[a0, a1, a2, a3]���[a1, a2, a3, a0]
_word AES::RotWord(_word rw) {
	return rw >> 8 | rw << 24;
}

// ��Կ��չ���� - ��128λ��Կ������չ�õ� w[4 * (Nr + 1)]
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

// S�б任 - ǰ4λΪ�кţ���4λΪ�к�
void AES::SubBytes(_byte mtx[4 * 4]) {
	for (int i = 0; i < 16; i++) {
		int row = mtx[i][7] * 8 + mtx[i][6] * 4 + mtx[i][5] * 2 + mtx[i][4];
		int col = mtx[i][3] * 8 + mtx[i][2] * 4 + mtx[i][1] * 2 + mtx[i][0];
		mtx[i] = S_Box[row][col];
	}
}


// �б任 - ���ֽ�ѭ����λ
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

	/*******��Ҫ�Ӻ���ǰ��ֵ*******/
	temp = mtx[15];
	for (int i = 15; i > 12; i--) {
		mtx[i] = mtx[i - 1];
	}
	mtx[12] = temp;
}


// �������ϵĳ˷� GF(2^8)
_byte AES::GFMul(_byte a, _byte b) {
	_byte res(0x00), temp;
	for (int i = 0; i < 8; i++) {
		// ˵��Ҫ��a
		if ((b & _byte(0x1)) != 0) res ^= a;

		// ȡ���λ�ж��Ƿ�Ϊ1����Ҫ��λ
		temp = a & _byte(0x80);
		a <<= 1;

		// �����0��˵��aҪ��λ��ģ2��0x1b
		if (temp != 0) a ^= _byte(0x1b);
		b >>= 1;
	}
	return res;
}


// �б任
void AES::MixColumns(_byte mtx[4 * 4]) {
	_byte arr[4];
	for (int i = 0; i < 4; i++) {
		// ��ȡÿһ��
		for (int j = 0; j < 4; j++)
			arr[j] = mtx[i + 4 * j];

		// Ϊ��һ��������׼������0
		for (int j = 0; j < 4; j++)
			mtx[i + 4 * j] = 0;

		// �����б任�������
		for (int j = 0; j < 4; j++)
			for (int k = 0; k < 4; k++)
				mtx[i + j * 4] ^= GFMul(coff[j * 4 + k], arr[k]);

	}
}

// ����Կ�ӱ任 - ��ÿһ������չ��Կ�������
void AES::AddRoundKey(_byte mtx[4 * 4], _word k[4]) {
	_word k1, k2, k3, k4;
	for (int i = 0; i < 4; i++) {
		// ȡ��0-7λ
		k1 = k[i] >> 24;
		// ȡ��8-15λ
		k2 = (k[i] << 8) >> 24;
		// ȡ��16-23λ
		k3 = (k[i] << 16) >> 24;
		// ȡ��24-31λ
		k4 = (k[i] << 24) >> 24;

		mtx[i] ^= _byte(k1.to_ulong());
		mtx[i + 4] ^= _byte(k2.to_ulong());
		mtx[i + 8] ^= _byte(k3.to_ulong());
		mtx[i + 12] ^= _byte(k4.to_ulong());
	}
}


// ����
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


// ��S�б任
void AES::InvSubBytes(_byte mtx[4 * 4])
{
	for (int i = 0; i < 16; ++i)
	{
		int row = mtx[i][7] * 8 + mtx[i][6] * 4 + mtx[i][5] * 2 + mtx[i][4];
		int col = mtx[i][3] * 8 + mtx[i][2] * 4 + mtx[i][1] * 2 + mtx[i][0];
		mtx[i] = Inv_S_Box[row][col];
	}
}


// ���б任 - ���ֽ�Ϊ��λѭ������
void AES::InvShiftRows(_byte mtx[4 * 4])
{
	// �ڶ���ѭ������һλ
	_byte temp = mtx[7];
	for (int i = 3; i > 0; --i)
		mtx[i + 4] = mtx[i + 3];
	mtx[4] = temp;
	// ������ѭ��������λ
	for (int i = 0; i < 2; ++i)
	{
		temp = mtx[i + 8];
		mtx[i + 8] = mtx[i + 10];
		mtx[i + 10] = temp;
	}
	// ������ѭ��������λ
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

// ����
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

// ��һ��char�ַ�����ת��Ϊ������,�浽һ�� byte ������
void AES::charToByte(_byte out[16], const char s[16])
{
	for (int i = 0; i < 16; ++i)
		for (int j = 0; j < 8; ++j)
			out[i][j] = ((s[i] >> j) & 1);
}

// ��������128λ�ֳ�16�飬�浽һ�� byte ������
void AES::divideToByte(_byte out[16], bitset<128>& data)
{
	bitset<128> temp;
	for (int i = 0; i < 16; ++i)
	{
		temp = (data << 8 * i) >> 120;
		out[i] = temp.to_ulong();
	}
}

// ��16�� byte �ϲ���������128λ 
bitset<128> AES::mergeByte(_byte in[16])
{
	bitset<128> res;
	res.reset();  // ��0
	bitset<128> temp;
	for (int i = 0; i < 16; ++i)
	{
		temp = in[i].to_ulong();
		temp <<= 8 * (15 - i);
		res |= temp;
	}
	return res;
}

// �����ļ�
void AES::encryptFile(_byte usekey[4 * 4], string fileName, string en_fileName) {
	bitset<128> data;
	_byte plain[16];
	
	ifstream in;
	ofstream out;
	in.open(fileName, ios::binary);
	out.open(en_fileName, ios::binary);

	// �ƶ��ļ�ָ�뵽�ļ�ĩβ���õ��ļ��ĳ��Ⱥ󣬸�ԭ�ļ�ָ��
	in.seekg(0, ios::end);
	int length = in.tellg(), cur;
	in.seekg(0, ios::beg);

	while (in.read((char*)&data, sizeof(data)))
	{
		divideToByte(plain, data);
		encrypt(plain, usekey);
		data = mergeByte(plain);
		out.write((char*)&data, sizeof(data));

		// �ҵ�Ŀǰ�ļ�ָ������λ�ã��ж�ʣ���ļ����Ƿ���ڵ���һ��data�ֽڴ�С��С�ڵĻ�ֱ������ѭ��
		cur = in.tellg();
		if (cur + 16 > length) break;

		data.reset();  // ��0
	}

	//// �õ�ʣ���ļ����ֽڴ�С
	//int num = length - cur;
	////��ʼ��һ���ڴ�
	//char rest[8] = "1";
	//// ��ʣ����ļ����뵽���ڴ���
	//in.read(rest, num);
	//// �������ڴ�Ĳ���д����һ���ļ�
	//out.write(rest, num);
	// �õ�ʣ���ļ����ֽڴ�С
	int num = length - cur;
	//��ʼ��һ���ڴ�
	char* rest = new char[num];
	// ��ʣ����ļ����뵽���ڴ���
	in.read(rest, num);
	// �������ڴ�Ĳ���д����һ���ļ�
	out.write(rest, num);
	delete[] rest;

	in.close();
	out.close();

}

// �����ļ�
void AES::decryptFile(_byte usekey[4 * 4], string fileName, string de_fileName) {
	bitset<128> data;
	_byte plain[16];
	// ���ļ� flower.jpg ���ܵ� cipher.txt ��
	ifstream in;
	ofstream out;
	// ���� cipher.txt����д��ͼƬ flower1.jpg
	in.open(fileName, ios::binary);
	out.open(de_fileName, ios::binary);

	// �ƶ��ļ�ָ�뵽�ļ�ĩβ���õ��ļ��ĳ��Ⱥ󣬸�ԭ�ļ�ָ��
	in.seekg(0, ios::end);
	int length = in.tellg(), cur;
	in.seekg(0, ios::beg);

	while (in.read((char*)&data, sizeof(data)))
	{
		divideToByte(plain, data);
		decrypt(plain, usekey);
		data = mergeByte(plain);
		out.write((char*)&data, sizeof(data));

		// �ҵ�Ŀǰ�ļ�ָ������λ�ã��ж�ʣ���ļ����Ƿ���ڵ���һ��data�ֽڴ�С��С�ڵĻ�ֱ������ѭ��
		cur = in.tellg();
		if (cur + 16 > length) break;

		data.reset();  // ��0
	}

	//// �õ�ʣ���ļ����ֽڴ�С
	//int num = length - cur;
	////��ʼ��һ���ڴ�
	//char rest[8] = "1";
	//// ��ʣ����ļ����뵽���ڴ���
	//in.read(rest, num);
	//// �������ڴ�Ĳ���д����һ���ļ�
	//out.write(rest, num);
		// �õ�ʣ���ļ����ֽڴ�С
	int num = length - cur;
	//��ʼ��һ���ڴ�
	char* rest = new char[num];
	// ��ʣ����ļ����뵽���ڴ���
	in.read(rest, num);
	// �������ڴ�Ĳ���д����һ���ļ�
	out.write(rest, num);
	delete[] rest;

	in.close();
	out.close();

}

// ���ܺ����
void AES::encryptAndEncode(_byte key[4 * 4], string fileName, string en_fileName)
{
	// AES�����ļ�
	encryptFile(key, fileName, "AESen_" + fileName);

	// ���ܺ���ļ�����Base64����
	char c;
	ifstream encodedFile("AESen_" + fileName, ios::binary);
	ofstream base64File(en_fileName);
	if (!encodedFile || !base64File)
	{
		cout << "�ļ���ʧ��" << endl;
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


	// ɾ���м��ļ�
	system(("del AESen_" + fileName).c_str());

}

// ��������
void AES::decodeAndDecrypt(_byte key[4 * 4], string fileName, string de_fileName)
{
	// fileName���ݽ���Base64���룬�������ļ�����Ϊ "Base64de_"+fileName
	char c;
	ifstream baseFile(fileName, ios::binary);
	ofstream decodedFile("Base64de_" + fileName, ios::binary);
	if (!baseFile || !decodedFile)
	{
		cout << "�ļ���ʧ��" << endl;
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

	// ���������ļ�����AES���ܣ����ܺ���ļ�����Ϊ de_fileName
	decryptFile(key, "Base64de_" + fileName, de_fileName);

	// ɾ���м��ļ�
	system(("del Base64de_" + fileName).c_str());
}


