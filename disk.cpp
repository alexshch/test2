#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <math.h>
#include <io.h>
//#include "writeinfile.h"

using namespace std;
//размер диска 10 КБайт
//размер сегмента 512 Байт -> количество сегментов 10 *1024 /512 = 20
//в начале каждого сегмента пишется структура с битовыми полями
// 0 - 4 бит - номер сегмента 0 - 32 (но у нас тольколько 0 -19) 
// 5-6 бит - 0 - сободен	1 - файл 2 - папка 
// 7 - 31 бит - размер записей в файле/папке. 2^25 -  максимальный допустимый размер файла/папки 33 554 432 байт
// на смом деле понятно что максимальный размер записи 19*506 = 9614 символа
//размер int 32 bits
//в конце каждого сегмента последние 2 байта  - это или FFFF, если не требуется перенос данных в другой сегмент 
//или адресс следующего сегмента (1 - 20) 
// 512 - 4 - 2 = 506 байта (505 символа в кодах ASCII) помещается в сегмент. В принципе дохера. 
// текущая директория сохраняется в list<unsigned short> currentDir
struct Head
{
	unsigned int numSegment		: 5;
	unsigned int segmentState	: 2;
	unsigned int dataSize		: 25;
};
struct FileName
{
	char name[8];					//длина имени файла или папки 7 символов + нулевой. в имени файла присутствует точку - 
									//это будет отличать его от папки
	unsigned short segment;			// 2 байта на адресс сегмента
									//10 байт на запись
};
const unsigned short ENDOFSEGNENT	= 0xFFFF;
const unsigned short BUFSIZE		= 506;
const unsigned short SIZEOFCLUSTER	= 512;
short int currentDir				= 0;
vector <short int> directories;
void createDisk();
int searchFreeSegment();
int createFile(string _fileName);
int writeInFile(string fileNameShouldBeOpen);
int readFromFile(string fileShouldBeOpen);
void showFilesInCurrentDirectory();
int createFolder(string _folderName);
int changeFolder(string _folderName);
int goPriveousFolder();
void tryParseCommand(string command);
void showHelp();
void showMemory();
void showHead(Head h);
void showRecordInFolder(FileName fn);
int searchNeedNumberOfSegments(unsigned int numbedOfSegment);

int main()
{
	if (0){
		if (_access("disk.dat",0) == -1){
			createDisk();
		}
		else {
			cout<<"disk already exist"<<endl;
		}
		string command;
		for(;;){
			getline(cin, command);
			if ((command.compare("exit") == 0) || (command.compare("quit") == 0))
				break;
			tryParseCommand(command);
		}
	}
	else
	{
		createDisk();
		createFile("txt.txt");
		writeInFile("txt.txt");
		showMemory();
		/*string str1 = "I love Mom";
		string str2 = str1.substr(0, 3);
		string str3 = str1.substr(3, str1.size());
		cout<<str1.size()<<endl<<
			str2<< " :"<<str2.size()<<endl<<
			str3<<" :"<<str3.size()<<endl;*/
	}
	system("pause");
	return 0;
}

void showMemory()
{
	ifstream ifs;
	ifs.open("disk.dat", ios::binary);
	Head headFirst;
	unsigned short val1;
	for (int i =0; i < 20; i++){
		ifs.read(reinterpret_cast<char*>(&headFirst), sizeof(headFirst));
		cout<<"num segment: "<<headFirst.numSegment<<endl<<"seg mode:    "<<headFirst.segmentState<<endl<<"seg size:    "<<headFirst.dataSize<<endl;
		ifs.seekg(BUFSIZE, ios::cur);
		ifs.read(reinterpret_cast<char*>(&val1), sizeof(val1));
		cout<<"last 2 bites: "<<val1<<endl;
	}
	ifs.close();
}

void showHead(Head h)
{
	cout<<"num segment: "<<h.numSegment<<endl<<"seg mode:    "<<h.segmentState<<endl<<"seg size:    "<<h.dataSize<<endl;
}

void showRecordInFolder(FileName fn)
{
	cout<<fn.name<<endl;
}

int searchFreeSegment()
{
	//возвращаем номер первого свободного сегмента или -1 в случае все заняты
	ifstream ifsfree;
	ifsfree.open("disk.dat", ios::binary);
	Head headFree;
	for (int i =0; i < 20; i++){
		ifsfree.read(reinterpret_cast<char*>(&headFree), sizeof(headFree));
		if(headFree.segmentState == 0){
			//cout<<"free segment: "<< headFree.numSegment<<endl;
			ifsfree.close();
			return headFree.numSegment;
		}
		else {
			ifsfree.seekg(BUFSIZE + sizeof(unsigned short), ios::cur);
		}
	}
	ifsfree.close();
	return -1;
}

void createDisk()
{
	currentDir = 0;
	directories.clear();
	Head folder;
	folder.numSegment		= 0;
	folder.segmentState		= 2;
	folder.dataSize			= 0;
	Head freeSegment;
	freeSegment.numSegment		= 1;
	freeSegment.segmentState	= 0;
	freeSegment.dataSize		= 0;
	char buf[BUFSIZE];

	ofstream ofs("disk.dat", std::ios::binary);
	ofs.write(reinterpret_cast<char*>(&folder), sizeof(folder));
	memset(buf,'\0', BUFSIZE);
	ofs.write(reinterpret_cast<char*>(buf),BUFSIZE);
	ofs.write(reinterpret_cast<char*>(const_cast<unsigned short*>(&ENDOFSEGNENT)), sizeof(ENDOFSEGNENT));
	for (int i =1; i<20; i++){ // всего 20 сегментов
		freeSegment.numSegment = i;
		ofs.write(reinterpret_cast<char*>(&freeSegment), sizeof(freeSegment));
		memset(buf,'\0', BUFSIZE);
		ofs.write(reinterpret_cast<char*>(buf),BUFSIZE);
		ofs.write(reinterpret_cast<char*>(const_cast<unsigned short*>(&ENDOFSEGNENT)), sizeof(ENDOFSEGNENT));
	}

	ofs.close();//seekg(506);
}

int createFile(string _fileName)
{
	Head headCurrentFolder;
	fstream fstr;
	fstr.open("disk.dat", ios::binary |ios::in | ios::out);
	fstr.seekg(currentDir * SIZEOFCLUSTER);
	fstr.read(reinterpret_cast<char*>(&headCurrentFolder), sizeof(headCurrentFolder));
	//проверка на существование
	vector<FileName> fileVector;
	unsigned short numberOfRecords = (unsigned short) (headCurrentFolder.dataSize /sizeof(FileName));
	FileName fileRead;
	for (int i = 0; i< numberOfRecords; i++){
		fstr.read(reinterpret_cast<char*>(&fileRead), sizeof(fileRead));
		fileVector.push_back(fileRead);
	}
	vector<FileName>::iterator iter;
	unsigned short adress =0;
	for(iter = fileVector.begin(); iter!=fileVector.end(); iter++){
		string tempStr((*iter).name);
		if (tempStr.compare(0,8,_fileName, 0,8) == 0){
			cout<<"file already exist"<<endl;
			return -1;
		}
	}
	//конец проверки
	int resultOfSearch = searchFreeSegment();
	unsigned short offset = 0; 
	if (resultOfSearch == -1)
		return -1;
	if (headCurrentFolder.dataSize >= 500){
		cerr<<"Disk Error";
		return -1;
	}
	else {
		offset = headCurrentFolder.dataSize;
		//cout<<"offset: "<<offset<<endl;
		headCurrentFolder.dataSize += 10;
		fstr.seekp(currentDir * SIZEOFCLUSTER);//currentDir
		//showHead(headCurrentFolder);
		fstr.write(reinterpret_cast<char*>(&headCurrentFolder), sizeof(headCurrentFolder));
		fstr.seekp(offset, ios::cur);
		FileName filename;
		char* str = const_cast<char*>(_fileName.c_str());
		strcpy_s(filename.name, str);
		filename.segment = resultOfSearch;
		fstr.write(reinterpret_cast<char*>(&filename),sizeof(filename));
		//fstr.seekp(0);
		fstr.seekp(resultOfSearch * SIZEOFCLUSTER, ios::beg);
		Head headCurrentFile;
		headCurrentFile.dataSize		= 0;
		headCurrentFile.numSegment		= resultOfSearch;
		headCurrentFile.segmentState	= 1;
		fstr.write(reinterpret_cast<char*>(&headCurrentFile),sizeof(headCurrentFile));

	}
	fstr.close();
	return 1;
}

int writeInFile(string fileNameShouldBeOpen)
{
	Head headCurrentFolder;
	fstream fstr;
	fstr.open("disk.dat", ios::binary |ios::in | ios::out );
	vector<FileName> fileVector;
	fstr.seekg(currentDir * SIZEOFCLUSTER, ios::beg);
	fstr.read(reinterpret_cast<char*>(&headCurrentFolder), sizeof(headCurrentFolder));
	unsigned short numberOfRecords = (unsigned short) (headCurrentFolder.dataSize /sizeof(FileName));
	FileName fileRead;
	for (int i = 0; i< numberOfRecords; i++){
		fstr.read(reinterpret_cast<char*>(&fileRead), sizeof(fileRead));
		fileVector.push_back(fileRead);
	}
	vector<FileName>::iterator iter;
	unsigned short adress =0;
	for(iter = fileVector.begin(); iter!=fileVector.end(); iter++){
		string tempStr((*iter).name);
		if (tempStr.compare(0,8,fileNameShouldBeOpen, 0,8) == 0){
			adress = iter->segment;
			break;
		}
	}
	if (adress == 0){
		cout<<"No such file"<<endl;
		fstr.close();
		return -1; //
	}
	cout<<"Type a text"<<endl;
	string textInFile;
	getline(cin,textInFile);
	if (textInFile.size() <= BUFSIZE -1){
		fstr.seekg(adress * SIZEOFCLUSTER, ios::beg);
		Head headFileForWriting;
		fstr.read(reinterpret_cast<char*>(&headFileForWriting), sizeof(headFileForWriting));
		headFileForWriting.dataSize = textInFile.size() + 1; // + null symbol
		fstr.seekp(adress * SIZEOFCLUSTER, ios::beg);
		fstr.write(reinterpret_cast<char*>(&headFileForWriting), sizeof(headFileForWriting));
		fstr.write(textInFile.c_str(), textInFile.size());	
	}
	else {
		unsigned int blocksNumber= (unsigned int)(ceil((double)textInFile.size()/BUFSIZE));
		if (searchNeedNumberOfSegments(blocksNumber) == -1){
			cout<<"It is haven't enough memory for writing"<<endl;
			fstr.close();
			return -1;
		}

		for(unsigned int i =0; i < blocksNumber; i++ ){
			//записываем первый кусок по найденному адресу
			fstr.seekg(adress * SIZEOFCLUSTER, ios::beg);
			Head headFileForWriting;
			fstr.read(reinterpret_cast<char*>(&headFileForWriting), sizeof(headFileForWriting));
			headFileForWriting.dataSize = textInFile.size() + 1; // + null symbol
			headFileForWriting.segmentState = 1;
			fstr.seekp(adress * SIZEOFCLUSTER, ios::beg);
			fstr.write(reinterpret_cast<char*>(&headFileForWriting), sizeof(headFileForWriting));
			string currentWritingStr;
			if (textInFile.size()>BUFSIZE -1){
				currentWritingStr = textInFile.substr(0, BUFSIZE);
			}
			else {
				currentWritingStr =  textInFile;
			}
			fstr.write(currentWritingStr.c_str(), currentWritingStr.size());
			unsigned short nextAddress =  searchFreeSegment();
			cout<<"nextAddress: "<<nextAddress<<endl;
			// пишем адресс следущего сегмента в конец текущего сегмента
			if(textInFile.size()> (BUFSIZE - 1)){ 
				//fstr.seekp(0);
				//fstr.seekp((adress * SIZEOFCLUSTER) + BUFSIZE);			
				fstr.write(reinterpret_cast<char* >(&nextAddress), 2);
				adress = nextAddress;
				//уменьшаем строку
				textInFile = textInFile.substr(BUFSIZE-1, textInFile.size());
				Head headExtensionSegment;
				headExtensionSegment.numSegment =	adress;
				headExtensionSegment.dataSize =		textInFile.size();
				headExtensionSegment.segmentState = 1;
				fstr.seekp(adress * SIZEOFCLUSTER , ios::beg);	
				fstr.write(reinterpret_cast<char*>(&headExtensionSegment), sizeof(headExtensionSegment));		
			}
			adress = nextAddress;

		}
		cout<<"the owerflow segment was"<<endl;
	}


	fstr.close();
	return 1;
}


int readFromFile(string fileShouldBeOpen)
{
	Head headCurrentFolder;
	ifstream fstr;
	fstr.open("disk.dat", ios::binary );
	vector<FileName> fileVector;
	fstr.seekg(currentDir * SIZEOFCLUSTER, ios::beg);
	fstr.read(reinterpret_cast<char*>(&headCurrentFolder), sizeof(headCurrentFolder));
	unsigned short numberOfRecords = (unsigned short) (headCurrentFolder.dataSize /sizeof(FileName));
	FileName fileRead;
	for (int i = 0; i< numberOfRecords; i++){
		fstr.read(reinterpret_cast<char*>(&fileRead), sizeof(fileRead));
		//showRecordInFolder(fileRead);
		fileVector.push_back(fileRead);
	}
	vector<FileName>::iterator iter;
	unsigned short adress = 0;
	for(iter = fileVector.begin(); iter!=fileVector.end(); iter++){
		string tempStr((*iter).name);
		if (tempStr.compare(0,8,fileShouldBeOpen, 0,8) == 0){
			adress = iter->segment;
			break;
		}
	}
	if (adress == 0){
		cout<<"No such file"<<endl;
		fstr.close();
		return -1; //
	}
	fstr.seekg(adress * SIZEOFCLUSTER, ios::beg);
	Head headFileForReading;
	fstr.read(reinterpret_cast<char*>(&headFileForReading), sizeof(headFileForReading));
	if (headFileForReading.dataSize == 0) {
		cout<<"file is empty"<<endl;
		fstr.close();
		return -1;
	}
	if (headFileForReading.dataSize <= BUFSIZE){
		char buffer[BUFSIZE];
		fstr.read(reinterpret_cast<char*>(buffer), headFileForReading.dataSize );
		string strFromFile(buffer);
		cout<<strFromFile<<endl;
	}
	else {
		cout<<"Now we haven't such ability"<<endl;
	}

	fstr.close();
	return 1;
}


void showFilesInCurrentDirectory()
{
	Head headCurrentFolder;
	ifstream fstr;
	fstr.open("disk.dat", ios::binary );
	vector<FileName> fileVector;
	fstr.seekg(currentDir * SIZEOFCLUSTER, ios::beg);
	fstr.read(reinterpret_cast<char*>(&headCurrentFolder), sizeof(headCurrentFolder));
	unsigned short numberOfRecords = (unsigned short) (headCurrentFolder.dataSize /sizeof(FileName));
	FileName fileRead;
	for (int i = 0; i< numberOfRecords; i++){
		fstr.read(reinterpret_cast<char*>(&fileRead), sizeof(fileRead));
		showRecordInFolder(fileRead);
	}
	fstr.close();
}

int createFolder(string _folderName)
{

	Head headCurrentFolder;
	fstream fstr;
	fstr.open("disk.dat", ios::binary |ios::in | ios::out);
	fstr.seekg(currentDir * SIZEOFCLUSTER);
	fstr.read(reinterpret_cast<char*>(&headCurrentFolder), sizeof(headCurrentFolder));
	int resultOfSearch = searchFreeSegment();
	//проверка на существование
	vector<FileName> fileVector;
	unsigned short numberOfRecords = (unsigned short) (headCurrentFolder.dataSize /sizeof(FileName));
	FileName fileRead;
	for (int i = 0; i< numberOfRecords; i++){
		fstr.read(reinterpret_cast<char*>(&fileRead), sizeof(fileRead));
		fileVector.push_back(fileRead);
	}
	vector<FileName>::iterator iter;
	unsigned short adress =0;
	for(iter = fileVector.begin(); iter!=fileVector.end(); iter++){
		string tempStr((*iter).name);
		if (tempStr.compare(0,8,_folderName, 0,8) == 0){
			cout<<"folder already exist"<<endl;
			return -1;
		}
	}
	//конец проверки
	unsigned short offset = 0; 
	if (resultOfSearch == -1)
		return -1;
	if (headCurrentFolder.dataSize >= 500){
		cerr<<"Disk Error"<<endl;
		return -1;
	}
	else {
		offset = headCurrentFolder.dataSize;
		//cout<<"offset: "<<offset<<endl;
		headCurrentFolder.dataSize += sizeof(FileName);
		fstr.seekp(currentDir * SIZEOFCLUSTER);//currentDir
		//showHead(headCurrentFolder);
		fstr.write(reinterpret_cast<char*>(&headCurrentFolder), sizeof(headCurrentFolder));
		fstr.seekp(offset, ios::cur);
		FileName filename;
		char* str = const_cast<char*>(_folderName.c_str());
		strcpy_s(filename.name, str);
		filename.segment = resultOfSearch;
		fstr.write(reinterpret_cast<char*>(&filename),sizeof(filename));
		//fstr.seekp(0);
		fstr.seekp(resultOfSearch * SIZEOFCLUSTER, ios::beg);
		Head headCurrentFile;
		headCurrentFile.dataSize		= 0;
		headCurrentFile.numSegment		= resultOfSearch;
		headCurrentFile.segmentState	= 2;
		fstr.write(reinterpret_cast<char*>(&headCurrentFile),sizeof(headCurrentFile));

	}
	fstr.close();
	return 1;
}

int changeFolder(string _folderName)
{
	Head headCurrentFolder;
	ifstream fstr;
	fstr.open("disk.dat", ios::binary );
	vector<FileName> fileVector;
	fstr.seekg(currentDir * SIZEOFCLUSTER, ios::beg);
	fstr.read(reinterpret_cast<char*>(&headCurrentFolder), sizeof(headCurrentFolder));
	unsigned short numberOfRecords = (unsigned short) (headCurrentFolder.dataSize /sizeof(FileName));
	FileName fileRead;
	for (int i = 0; i< numberOfRecords; i++){
		fstr.read(reinterpret_cast<char*>(&fileRead), sizeof(fileRead));
		fileVector.push_back(fileRead);
	}
	vector<FileName>::iterator iter;
	unsigned short adress =0;
	for(iter = fileVector.begin(); iter!=fileVector.end(); iter++){
		string tempStr((*iter).name);
		if (tempStr.compare(0,8,_folderName, 0,8) == 0){
			adress = iter->segment;
			break;
		}
	}
	if (adress == 0){
		cout<<"No such folder"<<endl;
		fstr.close();
		return -1; //
	}
	directories.push_back(currentDir);
	currentDir = adress;
	return 1;
}
int goPriveousFolder()
{
	if (currentDir != 0 && !directories.empty()){
		currentDir = directories.back();  
		directories.pop_back();
	}
	else {
		cout<<"you are in root"<<endl;
	}
	return 1;
}

void showHelp()
{
	cout<<"mkdir <folder> - create folder"<<endl<<
		"touch <file> - create file"<<endl<<
		"write <file> - write information in file"<<endl<<
		"read <file>  - read information from file"<<endl<<
		"ls           - show files and folders in current directory"<<endl<<
		"cd <folder>  - change directory"<<endl<<
		"cd ..        - go to previous directory"<<endl<<
		"format disk  - clear disk"<<endl<<
		"show memory  - memory state"<<endl<<
		"length of <file> and <folder> should be equal 7"<<endl<<
		"in a name of file should be . after 3rd symbol of name. for example <fil.txt>"<<endl<<
		"in a name of folder shouldn't be . for example <folder1>"<<endl<<
		"type <exit> or <quit> for exit from program"<<endl;
}

void tryParseCommand(string command)
{
	if (command.compare("cd ..") == 0){
		goPriveousFolder();
	}
	else if (command.compare("format disk") == 0){
		createDisk();
	}
	else if ((command.compare(0,5,"touch", 0,5) == 0) && (command.find(' ') ==5) && (command.find('.') >=9) && (command.size() == 13))
	{
		string arg = command.substr(6, 12);
		createFile(arg);
	}
	else if ((command.compare(0,5,"mkdir", 0,5)== 0) && (command.find(' ') ==5) && (command.find('.') == -1) && (command.size() == 13))
	{
		string arg = command.substr(6, 12);
		createFolder(arg);
	}
	else if ((command.compare(0,5,"write", 0,5) == 0) && (command.find(' ') ==5) && (command.find('.') >= 9) && (command.size() == 13))
	{
		string arg = command.substr(6, 12);
		writeInFile(arg);
	}
	else if ((command.compare(0,4,"read", 0,4) == 0) && (command.find(' ') ==4) && (command.find('.') >= 8) && (command.size() == 12))
	{
		string arg = command.substr(5, 11);
		readFromFile(arg);
	}
	else if ((command.compare(0,2,"ls",0,2) == 0) && (command.size() == 2)) {
		showFilesInCurrentDirectory();
	}
	else if ((command.compare(0,2,"cd", 0,2) == 0) && (command.find(' ') ==2) && (command.find('.') == -1) && (command.size() == 10))
	{
		string arg = command.substr(3, 10);
		changeFolder(arg);
	}
	else if (command.compare("show memory") == 0) {
		showMemory();
	}
	else if (command.compare("help") == 0) {
		showHelp();
	}
	else {
		cout<<"nonexistent command"<<endl;
	}
}

int searchNeedNumberOfSegments(unsigned int numberOfNeedSegment)
{
	//возвращаем количество свободных сегментов и -1 если их меньше чем нужно для записи
	ifstream ifsfree;
	ifsfree.open("disk.dat", ios::binary);
	Head headFree;
	unsigned int number = 0;
	for (int i =0; i < 20; i++){
		ifsfree.read(reinterpret_cast<char*>(&headFree), sizeof(headFree));
		if(headFree.segmentState == 0){
			//cout<<"free segment: "<< headFree.numSegment<<endl;
			number++;
		}
		ifsfree.seekg(BUFSIZE + sizeof(unsigned short), ios::cur);
	}
	ifsfree.close();
	if (number >= numberOfNeedSegment)
		return number;
	else
		return -1;
}
