#include <iostream>
#include <fstream>
using namespace std;
//размер диска 10 КБайт
//размер сегмента 512 Байт -> количество сегментов 10 *1024 /512 = 20
//в начале каждого сегмента пишется структура с битовыми полями
// 0 - 4 бит - номер сегмента 0 - 32 (но у нас тольколько 0 -19) 
// 5 бит - 0 - сободен	1 - занятъ 
// 6 бит - 0 - папка	1 - файл
//размер int 32 bits
struct Head
{
	unsigned int numSegment		: 5;
	unsigned int freeSegment	: 1;
	unsigned int isFileOrFolder	: 1;
	unsigned int				: 0;
};
int main()
{
	Head h1;
	h1.freeSegment		=0;
	h1.isFileOrFolder	=1;
	h1.numSegment		=0;
	cout<<"size of bitmap: "<<sizeof(h1)<<endl;
	ofstream ofs("disk.dat", std::ios::binary);
	ofs.write(reinterpret_cast<char*>(&h1), sizeof(h1));
	int  i = 0;
	ofs.close();
	system("pause");
	return 0;
}
