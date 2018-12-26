#include <iostream>
#include <Windows.h>
#include <string>
#include <stack>
#include <algorithm>

std::wstring ws;

std::wstring ToStr(int val)
{
	std::stack<wchar_t> wstack;
	while (val > 0);
	{
		wstack.push('0' + (val % 10));
		val /= 10;
	}
	std::for_each(wstack._Get_container().begin(), wstack._Get_container().end(),
		[](const wchar_t c)
	{
		wchar_t wstr[2] = { c };
		::ws.append(wstr);
	});
	
	return ws;
}


std::wstring BitmapToStr(int bitmap)
{
	std::wstring bitLine;
	for (int i = 0; i < 8; i++)
		if (bitmap & (1 << i))
			bitLine += '1';
		else
			bitLine += '0';
	return bitLine;
}

struct VERSION {
	int version;
	int revision;
	int IDEDeviceMap;
	int capabilities;
} VERSION;

struct ATTRIBUTE {
	int id;                                                                     // ������������� ��������
	int value;                                                                  // ��������
	int worst;                                                                  // ������ ��������������� ��������
	int threshold;                                                              // ��������� ��������
	unsigned __int64 data;                                                      // ������
	int statusFlags;                                                            // ��� ��������
} ATTRIBUTE;

struct TEMPERATURE {
	int _current;                                                                // ������� �����������, ������ ��� �������� 194
	int _min;                                                                    // ����������� �����������, ������ ��� �������� 194
	int _max;                                                                    // ������������ �����������, ������ ��� �������� 194
} TEMPERATURE;

void GetSMART(int driveNumber)
{
	// �������� ����������� �����
	std::wstring drivePath = L"\\\\.\\PhysicalDrive";
	drivePath.append(ToStr(driveNumber));
	HANDLE hPhysicalDrive = CreateFile(
		drivePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hPhysicalDrive == INVALID_HANDLE_VALUE) 
	{
		MessageBox(nullptr, L"Can't open physDrive!", L"Error!", MB_ICONERROR); ///Error
		return;
	}
	// ��������� S.M.A.R.T.
	bool isGetData = false;
	DWORD bytesReturned = 0;
	/* // SMART_GET_VERSION
		GETVERSIONINPARAMS verInParams = {0};
		isGetData = DeviceIoControl(hPhysicalDrive,SMART_GET_VERSION,
			NULL,0,&verInParams,sizeof(verInParams),&bytesReturned,NULL);
		if (!isGetData) {
				ShowMessage(SysErrorMessage(GetLastError()) + " Can't get S.M.A.R.T. ver!"); ///Error
				CloseHandle(hPhysicalDrive);
				return;
		}
		struct VERSION ver;
		ver.version = verInParams.bVersion;
		ver.revision = verInParams.bRevision;
		ver.IDEDeviceMap = verInParams.bIDEDeviceMap;
		ver.capabilities = verInParams.fCapabilities;
		Form1->Memo1->Lines->Add("ver: "+IntToStr(ver.version)+
			"   rev: "+IntToStr(ver.revision)+"   devMap: "+BitmapToStr(ver.IDEDeviceMap)+
			"   cap: "+BitmapToStr(ver.capabilities));
	*/
	const int inParamsSize = sizeof(SENDCMDINPARAMS) - 1;
	SENDCMDINPARAMS inParams = { 0 };
	inParams.cBufferSize = READ_ATTRIBUTE_BUFFER_SIZE;                                      // ������ ������ ��� ��������� ���������
	inParams.irDriveRegs.bFeaturesReg = READ_ATTRIBUTES;                                    // ������ �� ������ ���������
	inParams.irDriveRegs.bSectorCountReg = 1;                                               // ������� ���������� ��������
	inParams.irDriveRegs.bSectorNumberReg = 1;                                              // ������� ������ �������
	inParams.irDriveRegs.bCylLowReg = SMART_CYL_LOW;                                        // ������� ������, ����������� �� ����� ��������
	inParams.irDriveRegs.bCylHighReg = SMART_CYL_HI;                                        // ������� ������, ����������� �� ����� ��������
	inParams.irDriveRegs.bDriveHeadReg = 0xA0 | ((static_cast<BYTE>(driveNumber) & 1) << 4);      // ������� �����/������� IDE
	inParams.irDriveRegs.bCommandReg = SMART_CMD;                                           // ������� IDE
	const int outParamsSize = sizeof(SENDCMDOUTPARAMS) - 1 + READ_ATTRIBUTE_BUFFER_SIZE;
	SENDCMDOUTPARAMS outParamsAttributes[outParamsSize] = { 0 };
	isGetData = DeviceIoControl(hPhysicalDrive, SMART_RCV_DRIVE_DATA,
		&inParams, inParamsSize, &outParamsAttributes, outParamsSize, &bytesReturned, NULL);
	if (!isGetData) {
		MessageBox(nullptr, L"Can't get S.M.A.R.T. attributes!", L"Error", MB_ICONERROR); ///Error
		CloseHandle(hPhysicalDrive);
		return;
	}
	inParams.cBufferSize = READ_THRESHOLD_BUFFER_SIZE;
	inParams.irDriveRegs.bFeaturesReg = READ_THRESHOLDS;
	SENDCMDOUTPARAMS outParamsThresholds[outParamsSize] = { 0 };
	isGetData = DeviceIoControl(hPhysicalDrive, SMART_RCV_DRIVE_DATA,
		&inParams, inParamsSize, &outParamsThresholds, outParamsSize, &bytesReturned, NULL);
	if (!isGetData) {
		MessageBox(nullptr, L"Can't get S.M.A.R.T. thresholds!", NULL, MB_ICONERROR); ///Error
		CloseHandle(hPhysicalDrive);
		return;
	}
	CloseHandle(hPhysicalDrive);

	// ������� �������� S.M.A.R.T.
#define INDEX_ATTRIBUTE_INDEX       0
#define INDEX_ATTRIBUTE_STATUSFLAGS 1
#define INDEX_ATTRIBUTE_VALUE       3
#define INDEX_ATTRIBUTE_WORST       4
#define INDEX_ATTRIBUTE_RAW         5

// ���� �������� S.M.A.R.T.
#define PRE_FAILURE_WARRANTY        0x1                                         // �������� ������ (�����������)
#define ON_LINE_COLLECTION          0x2                                         // ��������� ��������� �������
#define PERFORMANCE_ATTRIBUTE       0x4                                         // �������, ���������� ������������������ �����
#define ERROR_RATE_ATTRIBUTE        0x8                                         // �������, ���������� ������� ��������� ������
#define EVENT_COUNT_ATTRIBUTE       0x10                                        // ������� �������
#define SELF_PRESERVING_ATTRIBUTE   0x20                                        // ����������������� �������

	BYTE *attributes = static_cast<BYTE*>(outParamsAttributes->bBuffer);
	BYTE *thresholds = static_cast<BYTE*>(outParamsThresholds->bBuffer);
	const int maxAttributes = 30;
	for (int i = 0; i < maxAttributes; ++i) {
		BYTE *attribute = &attributes[2 + i * 12];
		if (!attribute[INDEX_ATTRIBUTE_INDEX])                                  // ������� ������ ���������
			continue;
		int *thres = static_cast<int*>(static_cast<void*>(&thresholds[2 + i * 12 + 1]));
		BYTE *threshold = &thresholds[2 + i * 12];
		struct ATTRIBUTE attr;
		struct TEMPERATURE temp;
		CopyMemory(static_cast<void*>(&attr.data), static_cast<void*>(&attribute[INDEX_ATTRIBUTE_RAW]), sizeof(&attribute[INDEX_ATTRIBUTE_RAW]));
		if (attribute[INDEX_ATTRIBUTE_INDEX] == 194)
			if (attr.data > 200) {
				// �������������� ��� Hitachi
				temp._current = attribute[INDEX_ATTRIBUTE_RAW];
				temp._min = attribute[INDEX_ATTRIBUTE_RAW + 2];
				temp._max = attribute[INDEX_ATTRIBUTE_RAW + 4];
			}
			else {
				temp._current = attribute[INDEX_ATTRIBUTE_VALUE];
				temp._min = 404;                                                 // 404 - ��� ������, �.�. ����������� ����� ���� ����� 0
				temp._max = attribute[INDEX_ATTRIBUTE_WORST];
			}
		attribute[INDEX_ATTRIBUTE_RAW + 2] = attribute[INDEX_ATTRIBUTE_RAW + 3] = attribute[INDEX_ATTRIBUTE_RAW + 4] =
			attribute[INDEX_ATTRIBUTE_RAW + 5] = attribute[INDEX_ATTRIBUTE_RAW + 6] = 0;
		threshold[INDEX_ATTRIBUTE_RAW + 2] = threshold[INDEX_ATTRIBUTE_RAW + 3] = threshold[INDEX_ATTRIBUTE_RAW + 4] =
			threshold[INDEX_ATTRIBUTE_RAW + 5] = threshold[INDEX_ATTRIBUTE_RAW + 6] = 0;
		attr.id = attribute[INDEX_ATTRIBUTE_INDEX];
		attr.value = attribute[INDEX_ATTRIBUTE_VALUE];
		attr.worst = attribute[INDEX_ATTRIBUTE_WORST];
		attr.threshold = thres[0];
		attr.statusFlags = attribute[INDEX_ATTRIBUTE_STATUSFLAGS];

		std::wstring status;
		if (attr.statusFlags&PRE_FAILURE_WARRANTY)
			status += L"������.; ";
		if (attr.statusFlags&PERFORMANCE_ATTRIBUTE)
			status += L"��������.; ";

		std::wcout << L"id: " << attr.id << std::endl; //'\t'
		std::wcout << L"value: " << attr.value << std::endl;
		std::wcout << L"worst: " << attr.worst << std::endl;
		std::wcout << L"thres: " << attr.threshold << std::endl;
		std::wcout << L"data: " << attr.data << '\t' << status << std::endl; //std::string

		if (attr.id == 194) {
			std::wstring minTemp;
			if (temp._min != 404)
			{
				minTemp.append(L"Minimal temperature: ");
				minTemp.append(ToStr(temp._min));
				minTemp.append(L"\n");
			}
			minTemp.append(L"Maximal temperature: ");
			minTemp.append(ToStr(temp._max));
			minTemp.append(L"\n");

			minTemp.append(L"Current temperature: ");
			minTemp.append(ToStr(temp._current));
			minTemp.append(L"\n");
		}
	}
	//int driverError = outParamsAttributes->DriverStatus.bDriverError;
/* //bDriverError values
#define SMART_NO_ERROR          0       // No error
#define SMART_IDE_ERROR         1       // Error from IDE controller
#define SMART_INVALID_FLAG      2       // Invalid command flag
#define SMART_INVALID_COMMAND   3       // Invalid command byte
#define SMART_INVALID_BUFFER    4       // Bad buffer (null, invalid addr..)
#define SMART_INVALID_DRIVE     5       // Drive number not valid
#define SMART_INVALID_IOCTL     6       // Invalid IOCTL
#define SMART_ERROR_NO_MEM      7       // Could not lock user's buffer
#define SMART_INVALID_REGISTER  8       // Some IDE Register not valid
#define SMART_NOT_SUPPORTED     9       // Invalid cmd flag set
#define SMART_NO_IDE_DEVICE     10      // Cmd issued to device not present
										// although drive number is valid*/
										//Form1->Memo1->Lines->Add("driverError: "+IntToStr(driverError));*/
	std::wcout << L"Done!" << std::endl;
}

int main(int argc, char** argv)
{
	_wsetlocale(LC_ALL, L".866");

	GetSMART(0);

	system("pause");
	return 0;
}