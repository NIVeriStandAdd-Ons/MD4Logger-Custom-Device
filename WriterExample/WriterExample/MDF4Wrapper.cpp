// WriterExample.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "WriterExample.h"
#include "mdf4.h"
//#include <vld.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MDF41

void* mymemcpy(void* pDst, const void* pSrc, size_t nSize)
{
	memcpy(pDst, pSrc, nSize);
	return pDst;
}

// Helper to load a link and delete the parent
m4Block* LoadLink(MDF4File& m4, m4Block* pParent, int linkNo, M_UINT16 id)
{
	m4Block* pResult = m4.LoadBlock(pParent->getLink(linkNo));
	if (pResult && id && pResult->hdrID() != id)
	{
		delete pResult;
		pResult = NULL;
	}
	delete pParent;
	return pResult;
}

// Helper to find the last group in a MDF4 file
M4DGBlock* FindLastGroup(MDF4File& m4, M4HDBlock* pHdr)
{
	M4DGBlock* dg = NULL;
	M4CGBlock* cg;
	dg = (M4DGBlock*)m4.LoadLink(*pHdr, M4HDBlock::hd_dg_first, M4ID_DG);
	while (dg)
	{
		cg = (M4CGBlock*)m4.LoadLink(*dg, M4DGBlock::dg_cg_first, M4ID_CG);
		while (cg)
		{
			// if (cg->cg_cycle_count > 0) ????
			cg = (M4CGBlock*)LoadLink(m4, cg, M4CGBlock::cg_cg_next, M4ID_CG); // would be unsorted!
			if (cg)
			{
				delete cg;
				delete dg;
				return NULL;
			}
		}
		if (!dg->hasLink(M4DGBlock::dg_dg_next))
			return dg;
		dg = (M4DGBlock*)LoadLink(m4, dg, M4DGBlock::dg_dg_next, M4ID_DG);
	}
	return NULL;
}

void WriteMDF4(void)
{
	M_UINT64 i, i64LastTime = 0;
	// Find a reasonable time stamp:
	struct M_DATE now;
	memset(&now, 0, sizeof(M_DATE));
	time_t tt;
	time(&tt);
	now.time_ns = tt;
	now.time_ns *= 1000000000; // s -> ns

  // Create the Test-File
	MDF4File m4;
	if (m4.Create(L"C:\\Temp\\testDoubleV4.mf4", "WrtrEx",
#ifdef MDF41
		410)) // v4.1
#else
		400)) // v4.0
#endif
	{
#pragma region  gago


		// There MUST be at least one FileHistory (FH)
		// which MUST have an MD-Block
		wchar_t* pszFileHistory =
			L"<FHcomment>"
			L"<TX>First Test of MDF4 library</TX>"
			L"<tool_id>toolblabla</tool_id>"
			L"<tool_vendor>MDZ Bührer&amp;Partner</tool_vendor>"
			L"<tool_version>0.1</tool_version>"
			L"</FHcomment>\0";
		M4MDBlock fc(pszFileHistory);
		// Create the FH-Block
		M4FHBlock* fh = new M4FHBlock();
		fh->Create(&m4);
		// add the comment
		fh->setComment(fc);
		fh->fh_time = now;
		// and add to the HD Block
		m4.addHistory(fh);

		// MD-Block XML test
		wchar_t* pszHeaderComment =
			L"<HDcomment>"
			L"<TX>nothing special</TX>"
			L"<time_source>local PC reference timer</time_source>"
			L"<common_properties>"
			L"<e name=\"author\">Autor</e>"
			L"<e name=\"department\">Department</e>"
			L"<e name=\"project\">Project</e>"
			L"<e name=\"subject\">Subject</e>"
			L"</common_properties>"
			L"</HDcomment>\0";
		M4MDBlock hdComment(pszHeaderComment);
		m4.setComment(hdComment);

		m4.setFileTime(now);
		////////////////////////////////////////////////////////////////////////////////
		// Create one DG with one CG with two CN's
			// Note: hd.addDataGroup, dg.addChannelGroup, cg.addChannel
			// must be called with an allocated object; they will be deleted
			// when the next object is added or in the destructor!
		M4DGBlock* dg = m4.addDataGroup(new M4DGBlock());
		m4.GetHdr()->Save();

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// CG1 (simple Block)
		M4CGBlock* cg = dg->addChannelGroup(new M4CGBlock(1));
		dg->Save();
		// Record size in bytes
		cg->setRecordSize(sizeof(M_UINT64) * 5, 0UL);
		//cg->setSource(*fromBUS);

		M_UINT64 uiOffset = 0;

		/////////////////////////////////////////////////////////////////////////////////

		// Add the TIME channel
		M4CNBlock* cn = cg->addChannel(new M4CNBlock(CN_T_MASTER, CN_S_TIME));
		cg->Save();
		cn->setName(M4TXBlock("TIME"));
		//cn->setComment(M4MDBlock(szTimeComment));
		//todo commented
		//cn->setConversion(M4CCLinear(0.001, 0.0));
		//cn->setSource(*fromTime);
		// Offset = 0, size = 8 octets
		cn->setLocationBytes(CN_D_FLOAT_LE, uiOffset, sizeof(M_UINT64));
		cn->setUnit(M4TXBlock("s"));
		cn->Save();
		m4.addRecordCount(cg, cg->cg_record_id);
		// Set channel group record count (100001 points)
		//cg->setRecordCount(10);
		cg->Save();
		/////////////////////////////////////////////////////////////////////////////////
		
		/////////////////////////////////////////////////////////////////////////////////

		uiOffset += sizeof(M_UINT64);


		// Add a value channel; CAUTION this will delete the cn!
		cn = cg->addChannel(new M4CNBlock(CN_T_FIXEDLEN, CN_S_NONE));
		cg->Save();
		cn->setName(M4TXBlock("SPEED"));
		//cn->setComment(M4MDBlock(szSpeedComment));
		//todo commented
		//cn->setConversion(M4CCLinear(0.1, 0.0));
		//cn->setSource(*fromECU1);
		//cn->setRange(0, 100);
		// Offset = 8, size = 8 octets
		cn->setLocationBytes(CN_D_FLOAT_LE, uiOffset, sizeof(M_UINT64));
		cn->setUnit(M4TXBlock("m/s"));
		cn->Save();
		m4.addRecordCount(cg, cg->cg_record_id);
		// Set channel group record count (100001 points)
		//cg->setRecordCount(10);
		cg->Save();
		/////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////////////

		uiOffset += sizeof(M_UINT64);

		// Add a value channel; CAUTION this will delete the cn!
		M4CNBlock* cn1 = cg->addChannel(new M4CNBlock(CN_T_FIXEDLEN, CN_S_NONE));
		cg->Save();
		//cn->setComment(M4MDBlock(szSpeedComment));
		//todo commented
		//cn->setConversion(M4CCLinear(0.1, 0.0));
		//cn->setSource(*fromECU1);
		//cn1->setRange(0, 100);
		cn1->setUnit(M4TXBlock("jamov"));
		// Offset = 16, size = 8 octets
		cn1->setLocationBytes(CN_D_FLOAT_LE, uiOffset, sizeof(M_UINT64));
		cn1->setName(M4TXBlock("gago"));
		cn1->Save();
		m4.addRecordCount(cg, cg->cg_record_id);
		// Set channel group record count (100001 points)
		//cg->setRecordCount(10);
		cg->Save();
		/////////////////////////////////////////////////////////////////////////////////


		/////////////////////////////////////////////////////////////////////////////////

		uiOffset += sizeof(M_UINT64);

		// Add a value channel; CAUTION this will delete the cn!
		M4CNBlock* cn2 = cg->addChannel(new M4CNBlock(CN_T_FIXEDLEN, CN_S_NONE));
		cg->Save();
		cn2->setName(M4TXBlock("mago"));
		//cn->setComment(M4MDBlock(szSpeedComment));
		//todo commented
		//cn->setConversion(M4CCLinear(0.1, 0.0));
		//cn->setSource(*fromECU1);
		//cn2->setRange(0, 100);
		// Offset = 16, size = 8 octets
		cn2->setLocationBytes(CN_D_FLOAT_LE, uiOffset, sizeof(M_UINT64));
		cn2->setUnit(M4TXBlock("orov"));
		cn2->Save();
		m4.addRecordCount(cg, cg->cg_record_id);
		// Set channel group record count (100001 points)
		//cg->setRecordCount(10);
		cg->Save();
		/////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////////////

		uiOffset += sizeof(M_UINT64);

		// Add a value channel; CAUTION this will delete the cn!
		M4CNBlock* cn3 = cg->addChannel(new M4CNBlock(CN_T_FIXEDLEN, CN_S_NONE));
		cg->Save();
		cn3->setName(M4TXBlock("dado"));
		//cn->setComment(M4MDBlock(szSpeedComment));
		//todo commented
		//cn->setConversion(M4CCLinear(0.1, 0.0));
		//cn->setSource(*fromECU1);
		//cn3->setRange(0, 100);
		// Offset = 16, size = 8 octets
		cn3->setLocationBytes(CN_D_FLOAT_LE, uiOffset, sizeof(M_UINT64));
		cn3->setUnit(M4TXBlock("amsov"));
		cn3->Save();
		m4.addRecordCount(cg, cg->cg_record_id);
		// Set channel group record count (100001 points)
		//cg->setRecordCount(10);
		cg->Save();

		/////////////////////////////////////////////////////////////////////////////////


#pragma endregion


	

		// Taille du groupe de voies fixes
		M_UINT32 uiDataRecordSize = cg->getRecordSize(dg->dg_rec_id_size);
		printf("uiDataRecordSize: %d \n", uiDataRecordSize);
		// the following code is crucial for the effectiveness of storing/reading the data:
		M_UINT32 uiNoOfRecords = (M_UINT32)(cg->cg_cycle_count); // Number of records per block
		printf("uiNoOfRecords: %d \n", uiNoOfRecords);
		//M_UINT32 uiBlkSize = uiDataRecordSize * 10024;
		//printf("uiBlkSize: %d \n", uiBlkSize);

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Fixed data Stream
//#ifdef MDF41
//		m4DZStreamEx* DTStream = new m4DZStreamEx(dg, uiBlkSize, uiDataRecordSize, 5);
//#else
		m4DataStreamEx* DTStream = new m4DataStreamEx(dg, 1000000, 5);
//#endif

		//M_UINT64* data = new M_UINT64[3]{432,433,434};

		// //Buffer for fixed values
		//LPBYTE pRecord = (LPBYTE)calloc(uiDataRecordSize, 1);
		//// Pour chaque enregistrement, on stocke les valeurs
		//for (i = 0; i < 100; i++)
		//{
		//	LPBYTE p = (LPBYTE)pRecord;
		//	*((M_UINT64*&)p)++ = data[0] +i; // Time
		//	*((M_UINT64*&)p)++ = i+500;
		//	*((M_UINT64*&)p)++ = i+150;

		//	// Write the data values (fixed length)
		//	DTStream->Write(uiDataRecordSize, pRecord); // append
		//	DTStream->Write(sizeof(LPBYTE), pRecord); // append								   s

		//	printf("index: %d %d %d \n", i, uiDataRecordSize, pRecord);


		//	DTStream->AddRecords(1, 1); // group 1	

		//	printf("cg_record_id: %d \n", cg->cg_record_id);

		//}


		//i64LastTime = i;
		//free(pRecord);

		//DTStream->AddRecords(uiNoOfRecords, cg->cg_record_id); // group 1	

		//M_UINT64* data = new M_UINT64[5]{432,433,434,444,445 };
		//for (size_t i = 0; i < 100; i++)
		//{
		//	data[0] = data[0] + i;
		//	data[1] = data[1] + i;
		//	data[2] = data[2] + i;
		//	data[3] = data[3] + i;
		//	data[4] = data[4] + i;
		//		DTStream->Write(sizeof(M_UINT64)*5, data); // append
		//		DTStream->AddRecords(1, 1); // group 1
		//}

		DOUBLE* data = new DOUBLE[5]{ 101.1,102.2,103.3,104.4,105.5 };
		for (size_t i = 0; i < 100; i++)
		{
			data[0] = data[0] + i;
			data[1] = data[1] + i;
			data[2] = data[2] + i;
			data[3] = data[3] + i;
			data[4] = data[4] + i;


			DTStream->Write(sizeof(DOUBLE) * 5, data); // append
			DTStream->AddRecords(1, 1); // group 1
		}


		DTStream->Flush(TRUE); // keep full block size to append later
		DTStream->Close();
		delete DTStream;



		m4.Save();
		m4.Close();
	}

//	/////////////////////////////////////////////////////////////////////////////////////////////////
//	// add more data to the group (file is closed!)
//	MDF4File m4More;
//	if (m4More.Open(L"C:\\Temp\\Test.mf4", TRUE)) // open for append
//	{
//		// Find the last group in the file:
//		M4DGBlock* dg = FindLastGroup(m4More, m4More.GetHdr());
//		if (dg == NULL)
//		{
//			m4More.Close();
//			return;
//		}
//		// Tell header about the last group
//		m4More.GetHdr()->m_dgNext = dg;
//		M4CGBlock* cg = (M4CGBlock*)m4More.LoadLink(*dg, M4DGBlock::dg_cg_first, M4ID_CG);
//		M_UINT32 uiDataRecordSize = cg->getRecordSize(dg->dg_rec_id_size);
//		M_UINT32 uiBlkSize = uiDataRecordSize * 10024;
//		M_UINT32 uiNoOfRecords = 10000;
//
//		m4More.addRecordCount(cg, cg->cg_record_id);
//		M_UINT64 value1 = 0;
//		// Fixed data Stream
//#ifdef MDF41
//		m4DZStreamEx* DTStream = dg->DZWriteStreamEx(uiBlkSize, uiDataRecordSize, 5);
//		//m4DZStreamEx *DTStream = new m4DZStreamEx(dg, uiBlkSize, uiDataRecordSize, 5);
//#else
//		m4DataStreamEx* DTStream = new m4DataStreamEx(dg, uiBlkSize, 5);
//#endif
//		cg->Save();
//
//		i64LastTime += 1000;
//		// Buffer for fixed values
//		LPBYTE pRecord = (LPBYTE)calloc(uiDataRecordSize, 1);
//		// Pour chaque enregistrement, on stocke les valeurs
//		for (M_UINT64 i = 0; i < uiNoOfRecords; i++)
//		{
//			M_UINT64* p = (M_UINT64*)pRecord;
//			*p = i64LastTime + i; // Time
//			*++p = value1;
//			// Write the data values (fixed length)
//			DTStream->Write(uiDataRecordSize, pRecord); // append
//			// Increase the speed value
//			value1 += 20;
//			value1 %= 1000;
//		}
//		free(pRecord);
//		m4More.AddRecords(uiNoOfRecords, cg->cg_record_id);
//		DTStream->Close();
//		delete DTStream;
//		m4More.Save();
//		m4More.Close();
//		delete cg;
//	}
}



// The one and only application object

CWinApp theApp;

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(NULL);

	if (hModule != NULL)
	{
		// initialize MFC and print and error on failure
		if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
		{
			// TODO: change error code to suit your needs
			_tprintf(_T("Fatal Error: MFC initialization failed\n"));
			nRetCode = 1;
		}
		else
		{
			WriteMDF4();
		}
	}
	else
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
		nRetCode = 1;
	}

	return nRetCode;
}
