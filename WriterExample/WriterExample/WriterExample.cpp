#include "stdafx.h"
#include "mdf4.h"
#include "resource.h"

extern "C" __declspec(dllexport) int32_t _stdcall MDF4CreateFile(char* path, char* file_history, MDF4File * *m4);
extern "C" __declspec(dllexport) int32_t _stdcall MDF4SetDescription(MDF4File * m4, char* description);
extern "C" __declspec(dllexport) int32_t _stdcall MDF4CreateDataGroup(MDF4File * m4, M4DGBlock * *dg);
extern "C" __declspec(dllexport) int32_t _stdcall MDF4AddChannelGroupToDataGroupU64(MDF4File * m4, M4DGBlock * dg, M_UINT64 rec_id, M4CGBlock * *cg, M_UINT32 chan_count);
extern "C" __declspec(dllexport) int32_t _stdcall MDF4AddChannelToChannelGroupU64(MDF4File * m4, M4DGBlock * dg, M4CGBlock * cg, char* channel_name, M_UINT64 chan_index, M4CNBlock * *cn);
extern "C" __declspec(dllexport) int32_t _stdcall MDF4AddChannelToChannelGroupDouble(MDF4File * m4, M4DGBlock * dg, M4CGBlock * cg, char* channel_name, char* channel_unit, M_UINT64 chan_index, M4CNBlock * *cn);
extern "C" __declspec(dllexport) int32_t _stdcall MDF4CreateDataStream(M4DGBlock * dg, M_UINT32 iterations, M_UINT32 num_samples_per_read, m4DataStreamEx * *DTStream);
extern "C" __declspec(dllexport) int32_t _stdcall MDF4WriteStreamU64(m4DataStreamEx * DTStream, M_UINT64 chan_count, M_UINT64 * data);
extern "C" __declspec(dllexport) int32_t _stdcall MDF4WriteStreamDouble(m4DataStreamEx * DTStream, M_UINT64 chan_count, DOUBLE * data);
extern "C" __declspec(dllexport) int32_t _stdcall MDF4CloseStream(m4DataStreamEx * DTStream);
extern "C" __declspec(dllexport) int32_t _stdcall MDF4CloseFile(MDF4File * m4);

extern "C" __declspec(dllexport) int32_t _stdcall MDF4CreateFile(char* path, char* file_history, MDF4File * *m4)
{
	(*m4) = new MDF4File();
	//M_UINT32 uiDataRecordSize;
	//M_UINT32 uiNoOfRecords;
	//M_UINT32 uiBlkSize;
	size_t size_path = strlen(path) + 1;
	size_t size_history = strlen(file_history) + 1;
	size_t outsize;
	size_t outhistorysize;
	wchar_t* path_w = new wchar_t[size_path];
	wchar_t* history_w = new wchar_t[size_history];

	// Find a reasonable time stamp:
	struct M_DATE now;
	memset(&now, 0, sizeof(M_DATE));
	time_t tt;
	time(&tt);
	now.time_ns = tt;
	now.time_ns *= 1000000000; // s -> ns

	/*if ((_access(path, 0)) != -1)
	{
		remove(path);
	}*/
	//return 1;

	mbstowcs_s(&outsize, path_w, size_path, path, size_path - 1);
	if ((*m4)->Create(path_w, "WrtrEx", 400))
	{
		mbstowcs_s(&outhistorysize, history_w, size_history, file_history, size_history - 1);
		M4MDBlock fc(history_w);
		// Create the FH-Block
		M4FHBlock* fh = new M4FHBlock();
		fh->Create(*m4);
		// add the comment
		fh->setComment(fc);
		fh->fh_time = now;
		// and add to the HD Block
		(*m4)->addHistory(fh);
		(*m4)->setFileTime(now);
		return 1;
	}
	else
		return 0;
}
extern "C" __declspec(dllexport) int32_t _stdcall MDF4SetDescription(MDF4File * m4, char* description)
{
	size_t size_comment = strlen(description) + 1;
	size_t outsize_comment;
	wchar_t* pszHeaderComment = new wchar_t[size_comment];

	// MD-Block XML test
	mbstowcs_s(&outsize_comment, pszHeaderComment, size_comment, description, size_comment - 1);
	M4MDBlock hdComment(pszHeaderComment);
	if (m4->setComment(hdComment))
	{
		return 1;
	}
	else
		return 0;
}
extern "C" __declspec(dllexport) int32_t _stdcall MDF4CreateDataGroup(MDF4File * m4, M4DGBlock * *dg)
{
	////////////////////////////////////////////////////////////////////////////////
	// Create DG
	// Note: hd.addDataGroup
	// must be called with an allocated object; they will be deleted
	// when the next object is added or in the destructor!
	(*dg) = m4->addDataGroup(new M4DGBlock());
	if (m4->GetHdr()->Save())
		return 1;
	else
		return 0;
}
extern "C" __declspec(dllexport) int32_t _stdcall MDF4AddChannelGroupToDataGroupU64(MDF4File * m4, M4DGBlock * dg, M_UINT64 rec_id, M4CGBlock * *cg, M_UINT32 chan_count)
{
	(*cg) = dg->addChannelGroup(new M4CGBlock(rec_id));
	dg->Save();
	// Record size in bytes
	(*cg)->setRecordSize(sizeof(M_UINT64)* chan_count, 0UL);
	return 1;
}
extern "C" __declspec(dllexport) int32_t _stdcall MDF4AddChannelToChannelGroupU64(MDF4File * m4, M4DGBlock * dg, M4CGBlock * cg, char* channel_name, M_UINT64 chan_index, M4CNBlock * *cn)
{
	(*cn) = cg->addChannel(new M4CNBlock(CN_T_FIXEDLEN, CN_S_NONE));
	cg->Save();
	(*cn)->setName(M4TXBlock(channel_name));
	//(*cn)->setConversion(M4CCLinear(1, 0.0));
	//(*cn)->setRange(0, 100);
	//(*cn)->setUnit(M4TXBlock("m/s"));
	// Offset = 8, size = 8 octets
	(*cn)->setLocationBytes(CN_D_UINT_LE, (chan_index)*sizeof(M_UINT64), sizeof(M_UINT64));
	(*cn)->Save();

	m4->addRecordCount(cg, cg->cg_record_id);
	//cg->setRecordCount(1024000);
	cg->Save();
	return 1;
}
extern "C" __declspec(dllexport) int32_t _stdcall MDF4AddChannelToChannelGroupDouble(MDF4File * m4, M4DGBlock * dg, M4CGBlock * cg, char* channel_name, char* channel_unit, M_UINT64 chan_index, M4CNBlock * *cn)
{
	(*cn) = cg->addChannel(new M4CNBlock(CN_T_FIXEDLEN, CN_S_NONE));
	cg->Save();
	(*cn)->setName(M4TXBlock(channel_name));
	//(*cn)->setConversion(M4CCLinear(1, 0.0));
	//(*cn)->setRange(0, 100);
	(*cn)->setUnit(M4TXBlock(channel_unit));
	// Offset = 8, size = 8 octets
	(*cn)->setLocationBytes(CN_D_FLOAT_LE, (chan_index) * sizeof(DOUBLE), sizeof(DOUBLE));
	(*cn)->Save();

	m4->addRecordCount(cg, cg->cg_record_id);
	//cg->setRecordCount(1024000);
	cg->Save();
	return 1;
}
extern "C" __declspec(dllexport) int32_t _stdcall MDF4CreateDataStream(M4DGBlock * dg, M_UINT32 iterations, M_UINT32 num_samples_per_read, m4DataStreamEx * *DTStream)
{
	//M_UINT32 uiDataRecordSize;
	//M_UINT32 uiBlkSize;
	//uiDataRecordSize = cg->getRecordSize(dg->dg_rec_id_size);
	//uiBlkSize = 8 * num_samples_per_read;
	//uiBlkSize = (M_UINT32)8192000;
	(*DTStream) = new m4DataStreamEx(dg, num_samples_per_read, iterations); //NO M_UINT32 dlEntries as parameter!!!
	return 1;
}
extern "C" __declspec(dllexport) int32_t _stdcall MDF4WriteStreamU64(m4DataStreamEx * DTStream, M_UINT64 chan_count, M_UINT64 * data)
{
	//M_UINT32 uiNoOfRecords = 10000;
	//M_UINT64 value1 = 0;
	//M_UINT32 uiDataRecordSize = 10;

	//// Buffer for fixed values
	//LPBYTE pRecord = (LPBYTE)calloc(uiDataRecordSize, 1);

	//for (M_UINT64 i = 0; i < uiNoOfRecords; i++)
	//{
	//	LPBYTE p = (LPBYTE)pRecord;
	//	*((M_UINT64*&)p)++ = i; // Time
	//	*((M_UINT64*&)p)++ = value1;
	//	// Write the data values (fixed length)
	//	DTStream->Write(uiDataRecordSize, pRecord); // append
	//	// new value:
	//	value1 += 2;
	//	value1 %= 1000;
	//}
	//DTStream->AddRecords(uiNoOfRecords, channel_index); // group 1
	if (DTStream->Write(sizeof(M_UINT64)* chan_count, data))// append
	{
		DTStream->AddRecords(1, 1); // group 1
		return 1;
	}
	return 0;
}
extern "C" __declspec(dllexport) int32_t _stdcall MDF4WriteStreamDouble(m4DataStreamEx * DTStream, M_UINT64 chan_count, DOUBLE * data)
{
	//M_UINT32 uiNoOfRecords = 10000;
	//M_UINT64 value1 = 0;
	//M_UINT32 uiDataRecordSize = 10;

	//// Buffer for fixed values
	//LPBYTE pRecord = (LPBYTE)calloc(uiDataRecordSize, 1);

	//for (M_UINT64 i = 0; i < uiNoOfRecords; i++)
	//{
	//	LPBYTE p = (LPBYTE)pRecord;
	//	*((M_UINT64*&)p)++ = i; // Time
	//	*((M_UINT64*&)p)++ = value1;
	//	// Write the data values (fixed length)
	//	DTStream->Write(uiDataRecordSize, pRecord); // append
	//	// new value:
	//	value1 += 2;
	//	value1 %= 1000;
	//}
	//DTStream->AddRecords(uiNoOfRecords, channel_index); // group 1
	if (DTStream->Write(sizeof(DOUBLE) * chan_count, data))// append
	{
		DTStream->AddRecords(1, 1); // group 1
		return 1;
	}
	return 0;
}
extern "C" __declspec(dllexport) int32_t _stdcall MDF4CloseStream(m4DataStreamEx * DTStream)
{
	DTStream->Flush(TRUE);
	DTStream->Close();
	delete DTStream;
	return 1;
}
extern "C" __declspec(dllexport) int32_t _stdcall MDF4CloseFile(MDF4File * m4)
{
	m4->Save();
	m4->Close();
	return 1;
}