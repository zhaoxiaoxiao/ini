#include "CExecl.h"

CExecl::CExecl()
{
	m_strCharSet = "UTF-8";
}

CExecl::CExecl(const char * sPath)
{
	m_strCharSet = "UTF-8";
	OpenXLS(sPath);
}

CExecl::CExecl(const char * sPath,const char * sCharSet)
{
	m_strCharSet = sCharSet;
	OpenXLS(sPath);
}

CExecl::~CExecl()
{
	CloseHandle();
}

int CExecl::OpenXLS(const char * sPath)
{
	m_pXWB = xls_open(sPath, m_strCharSet.c_str());
	if(m_pXWB == NULL)
	{
		m_strErrMsg = "Open execl failed!!!";
		return -1;
	}
	xls_parseWorkBook(m_pXWB);
	return 0;
}

int CExecl::OpenXLS(const char * sPath,const char * sCharSet)
{
	m_pXWB = xls_open(sPath, sCharSet);
	if(m_pXWB == NULL)
	{
		m_strErrMsg = "Open execl failed!!!";
		return -1;
	}
	xls_parseWorkBook(m_pXWB);
	return 0;
}

int CExecl::OpenSheet(int n)
{
	m_pXWS = xls_getWorkSheet(m_pXWB, n); 
	if(m_pXWS == NULL)
	{
		m_strErrMsg = "Open Sheet failed!!!";
		return -1;
	}
	xls_parseWorkSheet(m_pXWS);
	return 0;
}

int CExecl::GetAllInfo(vector<vector<vector<string> > > & vecCell)
{
	if(m_pXWB == NULL)
	{
		m_strErrMsg = "WorkBook handle NULL!!!";
		return -1;
	}
	// 读取每个工作表
	for (int iSheetIndex = 0; iSheetIndex < m_pXWB->sheets.count; ++iSheetIndex) 
	{
		vector<vector<string> > vecSheet;		//每页的数据
		// 获取工作表
		m_pXWS = xls_getWorkSheet(m_pXWB, iSheetIndex);
		// 解析工作表
		xls_parseWorkSheet(m_pXWS);
		// 每行
		for (int iRow = 0; iRow <= m_pXWS->rows.lastrow; ++iRow) 
		{
			vector<string> vecRow;		//每行的数据
			// 该行第几列
			for (int iCol = 0; iCol <= m_pXWS->rows.lastcol; ++iCol) 
			{
				// 获取单元格，这里也可以通过xls_row获取到这行的数据，然后，使用row->cells来获取单元格
				xlsCell * cell = xls_cell(m_pXWS, iRow, iCol);
				// 判断单元格及内容是否为空
				if (cell) 
				{
					if(!cell->str)
					{
						vecRow.push_back("");
					}
					else
					{
						vecRow.push_back((char *)cell->str);
					}
				}
			}
			vecSheet.push_back(vecRow);
		}
		vecCell.push_back(vecSheet);
		// 关闭工作表
		xls_close_WS(m_pXWS);
		m_pXWS = NULL;
	}
	return 0;
}

int CExecl::GetSheetInfo(vector<vector<string> > & vecSheet)
{
	if(m_pXWS == NULL)
	{
		m_strErrMsg = "WorkSheet handle NULL!!!";
		return -1;
	}
	for (int iRow = 0; iRow <= m_pXWS->rows.lastrow; ++iRow) 
	{
		vector<string> vecRow;		//每行的数据
		// 该行第几列
		for (int iCol = 0; iCol <= m_pXWS->rows.lastcol; ++iCol) 
		{
			// 获取单元格，这里也可以通过xls_row获取到这行的数据，然后，使用row->cells来获取单元格
			xlsCell * cell = xls_cell(m_pXWS, iRow, iCol);
			// 判断单元格及内容是否为空
			if (cell) 
			{
				if(!cell->str)
				{
					vecRow.push_back("");
				}
				else
				{
					vecRow.push_back((char *)cell->str);
				}
			}
		}
		vecSheet.push_back(vecRow);
	}
}

int CExecl::GetSheet()
{
	if(m_pXWB == NULL)
	{
		m_strErrMsg = "Get the sheet failed,beacuse WorkBook handle NULL!!!";
		return -1;
	}
	return m_pXWB->sheets.count;
}

int CExecl::GetRow()
{
	if(m_pXWS == NULL)
	{
		m_strErrMsg = "Get the row failed,beacuse WorkSheet handle NULL!!!";
		return -1;
	}
	return m_pXWS->rows.lastrow;
}

int CExecl::GetCol()
{
	if(m_pXWS == NULL)
	{
		m_strErrMsg = "Get the row failed,beacuse WorkSheet handle NULL!!!";
		return -1;
	}
	return m_pXWS->rows.lastcol;
}

int CExecl::GetCell(char * sStr,int iRow,int iCol)
{
	if(m_pXWS == NULL)
	{
		m_strErrMsg = "WorkSheet handle NULL!!!";
		return -1;
	}
	if(iRow >= GetRow() || iCol >= GetCol())
	{
		m_strErrMsg = "Row or Col is too large!!!";
		return -1;
	}
	xlsCell * cell = xls_cell(m_pXWS, iRow, iCol);
	if (cell) 
	{
		if(!cell->str)
		{
			strcpy(sStr,"");
		}
		else
		{
			strcpy(sStr,(char *)cell->str);
		}
		return 0;
	}
	m_strErrMsg = "Cell NULL!!!";
	return -1;
}

int CExecl::GetCell(char * sStr,int iSheet,int iRow,int iCol)
{
	if(m_pXWB == NULL)
	{
		m_strErrMsg = "WorkBook handle NULL!!!";
		return -1;
	}
	if(m_pXWS == NULL)
	{
		m_strErrMsg = "WorkSheet handle NULL!!!";
		return -1;
	}
	if(iSheet >= GetSheet() || iRow >= GetRow() || iCol >= GetCol())
	{
		m_strErrMsg = "Sheet Row or Col is too large!!!";
		return -1;
	}
	OpenSheet(iSheet);
	xlsCell * cell = xls_cell(m_pXWS, iRow, iCol);
	if (cell) 
	{
		if(!cell->str)
		{
			strcpy(sStr,"");
		}
		else
		{
			strcpy(sStr,(char *)cell->str);
		}
		return 0;
	}
	m_strErrMsg = "Cell NULL!!!";
	return -1;
}

int CExecl::CloseHandle()
{
	if(m_pXWS)
	{
		
		xls_close_WS(m_pXWS);
		m_pXWS = NULL;
	}
	if(m_pXWB)
	{
		xls_close_WB(m_pXWB);
		m_pXWB = NULL;
	}
	return 0;
}