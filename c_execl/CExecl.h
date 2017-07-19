#ifndef __CEXECL_H__
#define __CEXECL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <unistd.h>
extern "C"
{
#include <xls.h>
}
using namespace std;

class CExecl
{
private:
	string m_strPath;	
	xlsWorkBook * m_pXWB;			//
	xlsWorkSheet * m_pXWS;			//页
	xlsCell * m_pCell;				//单元格
	string m_strErrMsg;
	int m_iRow;
	int m_iCol;
	string m_strCharSet;			//字符集，默认为UFT-8
	//vector<vector<vector<string> > > m_vecCell;		//第一层表示sheet,第二层表示row，第三层表示每个单元格里的数据
	
public:
	CExecl();
	CExecl(const char * sPath);
	CExecl(const char * sPath,const char * sCharSet);
	~CExecl();
	int OpenXLS(const char * sPath);		//打开XLS文件
	int OpenXLS(const char * sPath,const char * sCharSet);		//打开XLS文件
	int GetAllInfo(vector<vector<vector<string> > > & vecCell);		//获取所有数据并保存容器中
	int GetSheetInfo(vector<vector<string> > & vecSheet);		//获取当前页的数据
	int OpenSheet(int n);				//打开文件中的某一页，从0开始计算
	int GetSheet();				//获取有多少页
	int GetRow();				//获取有多少行，前提是必须打开了某一页
	int GetCol();				//获取有多少列，前提是必须打开了某一页
	int GetCell(char * sStr,int iRow,int iCol);				//获取单元格里的数据
	int GetCell(char * sStr,int iSheet,int iRow,int iCol);				//获取单元格里的数据
	
	
	int CloseHandle();
	
private:
	int SetCharSet();			//设置字符集
};

#endif //__CEXECL_H__