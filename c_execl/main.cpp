#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include "CExecl.h"
using namespace std;

int main()
{
	vector<vector<vector<string> > > m_vecCell;
	vector<vector<string> > m_vecSheet;
	CExecl cex("./test.xls");
	cex.OpenSheet(0);	
	cex.GetSheetInfo(m_vecSheet);
	for(int i = 0;i < m_vecSheet.size();i++)
	{
		for(int j = 0; j < m_vecSheet[i].size();j++)
		{
			printf("%s ",m_vecSheet[i][j].c_str());
		}
		printf("\n");
	}
	printf("+++++++++++++++++++++++++++++++++\n");
	cex.GetAllInfo(m_vecCell);
	for(int i = 0;i < m_vecCell.size();i++)
	{
		for(int j = 0;j < m_vecCell[i].size();j++)
		{
			for(int k = 0; k < m_vecCell[i][j].size();k++)
			{
				printf("%s ",m_vecCell[i][j][k].c_str());
			}
			printf("\n");
		}
		printf("---------------\n");
	}
	
	return 0;
}