#include "writeinfile.h"
#include <iostream>
#include <fstream>
#include "headsstruct.h"
#include <vector>
#include "globalvariable.h"
#include "fileinfo.h"
using namespace std;

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
		headFileForWriting.dataSize = textInFile.size()+1; //
		headFileForWriting.segmentState = 1;
		fstr.seekp(adress * SIZEOFCLUSTER, ios::beg);
		fstr.write(reinterpret_cast<char*>(&headFileForWriting), sizeof(headFileForWriting));
		string currentWritingStr;
		if (textInFile.size()>BUFSIZE -1){
			currentWritingStr = textInFile.substr(0, BUFSIZE-1);
		}
		else {
			currentWritingStr =  textInFile;
		}
		fstr.write(currentWritingStr.c_str(), currentWritingStr.size());
		unsigned short nextAddress =  searchFreeSegment();
		cout<<"nextAddress: "<<nextAddress<<endl;
		// пишем адресс следущего сегмента в конец текущего сегмента
		if(textInFile.size()> (BUFSIZE - 1)){ 
			fstr.seekp(1, ios::cur);
			fstr.write(reinterpret_cast<char* >(&nextAddress), sizeof(nextAddress));
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

	fstr.close();
	return 1;
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
		std::cout<<"Disk Error";
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
