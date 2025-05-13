#pragma once
#pragma once
//*****
#define NOPARITY            0
#define ODDPARITY           1
#define EVENPARITY          2
#define ONESTOPBIT          0
#define ONE5STOPBITS        1
#define TWOSTOPBITS         2
#define CBR_110             110
#define CBR_300             300
#define CBR_600             600
#define CBR_1200            1200
#define CBR_2400            2400
#define CBR_4800            4800
#define CBR_9600            9600
#define CBR_14400           14400
#define CBR_19200           19200
#define CBR_38400           38400
#define CBR_56000           56000
#define CBR_57600           57600
#define CBR_115200          115200
#define CBR_128000          128000
#define CBR_256000          256000
#define disTime_01 20

#ifdef DLL_XMT_USB_EXPORTS
#define DLL_XMT_SER_API extern "C" _declspec(dllexport)
#else
#define DLL_XMT_SER_API extern "C" _declspec(dllexport)
#endif

#pragma once
#ifndef __SOMEFILE_H__
#define __SOMEFILE_H__
//
//OVERLAPPED m_ov;//��һ�������������첽�����������Ϣ�Ľṹ�� 
//HANDLE hComm, g_hCom;//���ڵľ��  
//HWND hRbox;
//COMSTAT comstat;//�������ڽṹ��Ϣ 
//bool DisplayHEX = false;
//unsigned char *tmp_command;//���յ�������
//bool RxInbool;//1��ʾ������������Ҫ�������� ���û������Ҫ�������� �ñ�����Ҫ�˹����㸴λ�� �������ݺ���Ҫ��Ϊ����Ϊ 0 ����һ�������ٴ�ʹ��

			  //�ڲ�������
float CalData(unsigned char kk0, unsigned char kk1, unsigned char kk2, unsigned char kk3);//����ת�� ������
double XMT_ReDo_pro(unsigned char comand_Arr[]); //���ݲ�ͬ������������
double Res_command_pro(unsigned char T_D_3, unsigned char T_D_4);//�������

#endif

unsigned char* DataAnla_Pro(double f, unsigned char kk[4]);
DLL_XMT_SER_API int EntryXMT(LPCWSTR comname, long int BaudRate, HWND rhbox); //��ʼ�� ���� 
DLL_XMT_SER_API int EntryXMT_labview(int m_com, int BaudRate);//1 com1 2 com2 ,1 9600 2 115200 20190723 ����ʹ��labview���Ʒ���

															  //�趨����ͬʱ�򿪴��� EntryXMT(_T("COM3"),9600,NULL);//�򿪴���3���� ������Ϊ9600 VS2010 

															  //CString strFileName; //ʹ��VC++ 6.0
															  //strFileName = "COM5";//ʹ��VC++ 6.0
															  // LPCWSTR lpcwStrCOM = strFileName.AllocSysString();//ʹ��VC++ 6.0
															  //int i = EntryXMT(lpcwStrCOM,9600,NULL);//ʹ��VC++ 6.0

DLL_XMT_SER_API bool WriteArr(unsigned char * m_szWriteBuffer,//��������
	unsigned char m_nToSend// �������ݳ���
	);
DWORD WINAPI ThreadSendMsg(LPVOID lpParameter);
DLL_XMT_SER_API int ReceiveArr(unsigned char RcBuffArr[], int ReadCharNum); //���������Լ�������� ��ȡ�����ݳ���ReadCharNum

DLL_XMT_SER_API double Res_command_proP(unsigned char T_D_3, unsigned char T_D_4);//�������
DLL_XMT_SER_API int ReceiveArrP(unsigned char RcBuffArr[]); //���������Լ�������� ��ȡ�����ݳ���ReadCharNum
															//�������Ͷ�ȡ����


DLL_XMT_SER_API void XMT_COMMAND_ReadDataP(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num
	);



DLL_XMT_SER_API  bool openport(LPCWSTR portname); //�ڲ�����
DLL_XMT_SER_API  bool setupdcb(int rate_arg); //�ڲ�����
											  //�رմ���-����������������Լ��رգ�
DLL_XMT_SER_API int OpenComWithBit(int com_I, int B_Bound_int);//B_Bound_int 1 9600 2 38400 3 57600 4 115200 
DLL_XMT_SER_API int OpenComConnectRS232(int nPortNr, int iBaudRate);// 
DLL_XMT_SER_API  bool CloseSer(); //�رոô���
								  /****************�豸���ƿ������*********************/ //���� ������rate_arg�� 9600 
DLL_XMT_SER_API void ClearSer();//��մ���
DLL_XMT_SER_API bool setupdcb_BaudRate(int rate_arg); //���ò����� 9600
													  //���ش�������

DLL_XMT_SER_API DCB ReSerDBC();

void XMT_ReadMultReal_Do(unsigned char T_D_3, unsigned char T_D_4,
	unsigned char *OpenOrCloseFlag_0,
	double *Data_0,
	unsigned char *OpenOrCloseFlag_1,
	double *Data_1,
	unsigned char *OpenOrCloseFlag_2,
	double *Data_2
	);
void XMT_ReadMultReal(unsigned char comand_Arr[],
	unsigned char *OpenOrCloseFlag_0,
	double *Data_0,
	unsigned char *OpenOrCloseFlag_1,
	double *Data_1,
	unsigned char *OpenOrCloseFlag_2,
	double *Data_2
	);//���ɼ���������зְ�����
unsigned char XMT_ReDo_pro_Unit(unsigned char comand_Arr[]); //������53��ȡ ��λ�� ��λ����
unsigned char Res_command_pro_Unit(unsigned char T_D_3, unsigned char T_D_4);//�������
void XMT_ReDo_pro_Arr(unsigned char comand_Arr[], unsigned char arrRec[3]); //��ȡ����������ֵ��

																			//��������ת�� ������ת��ΪDA(0-65535)֮��ת��
void ChangeDataToDa(unsigned char TmpDa[2], float TmpSendData, float MaxData, float MinData); //�������� ת��Ϊ����[0] ������ֽ� �ȷ��� [1]������ֽں���
DLL_XMT_SER_API void dis_Num100us(int tmpUs_100us);//100us��������
DLL_XMT_SER_API void ArrDataSend(unsigned char address, unsigned char Channel_Num, double arr[], int ArrLong, unsigned char flagOpenOrClose, int tmpUs_100us);//�������� �Լ����鳤�� flagOpenOrClose���ջ� �趨 'C'��ʾ�ջ� 'O'��ʾ����,���ͼ��100΢��������
DLL_XMT_SER_API double SendDataAndReadDataFormMcu(unsigned char Address, double SendData, int ChannelFlag, unsigned char OpenAndClose, int Time100Us);//����ֱ�ӷ��� ���͵�����
																																					  // Address ��ַ
																																					  // double SendData ���͵�����
																																					  // int ChannelFlag ͨ�� 
																																					  // unsigned char OpenAndClose
																																					  //int Time100Us ���ض�ȡʱ�� 100΢���������

																																			  //������  0 1  
DLL_XMT_SER_API void XMT_COMMAND_SinglePoint(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	double VoltOrMove_Data
	); //
	   //do ��·������ 2 3  
DLL_XMT_SER_API   unsigned char XMT_COMMAND_MultSinglePoint(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	double VoltOrMove_Data_0,
	double VoltOrMove_Data_1,
	double VoltOrMove_Data_2
	);
//�������� 4
DLL_XMT_SER_API void 	XMT_COMMAND_SinglePoint_Clear(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);
//�������� 5 6 
DLL_XMT_SER_API double XMT_COMMAND_ReadData(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num
	);
// 7 8 
DLL_XMT_SER_API void 	XMT_COMMAND_ReadData_TS(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char TimerSet_ms
	);
// 9 10 
DLL_XMT_SER_API void 	XMT_COMMAND_ReadData_TS_MultChannle(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char TimerSet_ms
	);
//ֹͣ��ȡ 11
DLL_XMT_SER_API void 	XMT_COMMAND_ReadData_Stop(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);
//������
//��·����  12 13 
DLL_XMT_SER_API void 	XMT_COMMAND_WaveSetHighSingle(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char WaveType,
	double FengFengZhi,
	double PinLvHz,
	double Pianzhi
	);

//ֹͣ����  14 
DLL_XMT_SER_API void 	XMT_COMMAND_WaveSetHighSingleStop(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);
//��·��׼�ٶ�ģʽ 15 16 
DLL_XMT_SER_API void 	XMT_COMMAND_WaveSetMultWave(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char WaveType,
	double FengFengZhi,
	double PinLvHz,
	double Pianzhi
	);

//ֹͣ���� 17
DLL_XMT_SER_API void 	XMT_COMMAND_WaveSetMultWaveStop(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num
	);

//����Э����
//����  18 20 22 
DLL_XMT_SER_API void 	XMT_COMMAND_Assist_SetFlag(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char SetFlag
	);

//��ȡflag���� 19 21 23 
DLL_XMT_SER_API unsigned char XMT_COMMAND_Assist_ReadFlag(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num
	);


//�궨��������
//�趨ϵͳ��  24 26 28 30 32 34 
DLL_XMT_SER_API void 	XMT_COMMAND_SetSystemInfo(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	double SystemInfo
	);
//�������� 25 27  29
DLL_XMT_SER_API double XMT_COMMAND_ReadSystemInfo(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num
	);
//�趨�ߵ���
//30 32 34 old
DLL_XMT_SER_API void 	XMT_COMMAND_SetSystemHL_Limit(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	double SystemInfo
	);

//��ȡϵͳ�ߵ��� 31 33 35 
DLL_XMT_SER_API double   XMT_COMMAND_ReadSystemHL_Limit(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num
	);
// 36 ����PID ����/ֹͣ 
DLL_XMT_SER_API void 	XMT_COMMAND_SETPID_RorH(
	unsigned char address,
	unsigned char Command_B3,//36
	unsigned char Command_B4,//0
	unsigned char Channel_Num,//0,1,2
	unsigned char PIDSetFlag//���� 'R' ֹͣ 'H'
	); //���� 'R' ֹͣ 'H'
	   //37 ����PID����
DLL_XMT_SER_API void SendArray_PID_Channel(
	int address,//��ַ��
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Channel_Num,
	float PID_P,
	float PID_I,
	float PID_D
	);//��������
	  //38 ��ȡ PID ���� 
DLL_XMT_SER_API void Read_PID_Channel(
	int address,//��ַ��
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Channel_Num,
	float PID_Rc[3]
	); //��������
	   //38 ��ȡ PID ���� 
DLL_XMT_SER_API void Read_PID_Channel(
	int address,//��ַ��
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Channel_Num,
	float PID_Rc[3]
	); //��������
	   //46 �趨��λ����ַ �������ظ�����10��
DLL_XMT_SER_API void 	XMT_COMMAND_SetMCUAddress(
	unsigned char address,//0x00 ����ǹ㲥��ַ �̶���
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char SetAddress////Ԥ���ַ
	);
//47 ��ȥ��λ����ַ
DLL_XMT_SER_API unsigned char 	XMT_COMMAND_ReadMCUAddress(
	unsigned char address,//0x00 ����ǹ㲥��ַ �̶���
	unsigned char Command_B3,
	unsigned char Command_B4
	);
//48
//ʵʱ������(����λ��ָ��ÿ��ͨ�����͵�ѹ��λ��)
DLL_XMT_SER_API void 	XMT_COMMAND_ReadData_TS_UpDoPro(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char TimerSet_ms,
	unsigned char Flag_Channe_OpenOrClose
	);
//��ȡϵͳ�� 49
//ʵʱ������(����λ�����ݿ��ջ�״̬ȷ��ÿ��ͨ����ѹ��λ��)
DLL_XMT_SER_API void 	XMT_COMMAND_ReadData_TS_DownDoPro(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char TimerSet_ms
	);

// 50 ����У��
DLL_XMT_SER_API void XMT_COMMAND_CONTROL_PID(
	int address_ma,//��ַ��
	int bao_long,  //����
	int zhilingma_B3,//ָ����
	int zhilingma_B4,//ָ����
	unsigned char channel_num,//ͨ����
	unsigned char FLAG_CLoseOrOpen//��ʼ����
	);
//51 ��ȡ��·λ�ƻ��ѹ����
DLL_XMT_SER_API void 	XMT_COMMAND_ReadMultChannelMoveOrVolt(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char *OpenOrCloseFlag_0,
	double *Data_0,
	unsigned char *OpenOrCloseFlag_1,
	double *Data_1,
	unsigned char *OpenOrCloseFlag_2,
	double *Data_2
	);
//52 ��ȡ��ѹλ�����ưٷֱ�
DLL_XMT_SER_API float 	XMT_COMMAND_ReadSystem_VoltPer(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);
//53
DLL_XMT_SER_API unsigned char 	XMT_COMMAND_ReadSystem_Unit(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);
//54 ��ȡ��λ��������ͣ�ٶ�
DLL_XMT_SER_API unsigned char  XMT_COMMAND_ReadWaveBeginAndStopSpeed(
	int address_ma,//��ַ��
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char channel_num // ͨ���� 0 1 2 
	);// 'H'�������  'L'������� 
	  //55��λ������ĳһ·��������ֹͣ�ٶȣ�����ֹͣ�ٶ�һ�£�
DLL_XMT_SER_API void XMT_COMMAND_SetWaveBeginAndStopSpeed(
	int address_ma,//��ַ��
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char channel_num, // ͨ���� 0 1 2 
	unsigned char WaveBeginAndStopFlag // 'H'�������  'L'������� 
	);

//56 ��λ��������λ����λ
DLL_XMT_SER_API void XMT_COMMAND_SetMCUMardOrUm(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char MCUMardOrUm //0 �Ƕ� 1 λ��
	);
//57 ��λ��������λ������
DLL_XMT_SER_API void XMT_COMMAND_SetMCUE09orOther(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char MCUDoFlag //0 E709 1 E517
	);
//58 �趨��λ����ѹλ�����ưٷֱ�
DLL_XMT_SER_API void XMT_COMMAND_SetMCUVoltOrUmPP(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	float tmpData //0��1 ��С��
	);

// 59 
DLL_XMT_SER_API  void  XMT_COMMAND_ReadMCU_PIDFlag(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char ChannelFlag[3]//'Y'��Ч 'N'��ʾ��Ч  ��������ֵ
	);
// 61 62 �ڶ�̨���� ����422������ʱ��ʹ��
// 63 
//ͨ��usbͨ�Ŷ˿�������λ������ͨ�Ų�����
DLL_XMT_SER_API void  XMT_COMMAND_SetMCUComBit(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char ComBitFlag
	);
/* //������λ��������
��A��9600 ��B��19200  ��C��38400 ��D��57600
��E��76800��F��115200 ��G��128000��H��230400
��I��256000
//20170509
*/
// 64  AVRר�ÿ���jtag
DLL_XMT_SER_API void  XMT_COMMAND_SetMCUJtag(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char AVRFlag //[5]����AVR Jtag���� ��S��
	);
//65 �����������ת���ɼ�����
DLL_XMT_SER_API void XMT_COMMAND_LetMCUToReadData(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);

//��λ��ģʽ��� 66 67 
DLL_XMT_SER_API void XMT_COMMAND_WaveSetMultWaveXwj(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char WaveType,
	double FengFengZhi,
	double PinLvHz,
	double Pianzhi,
	double Xwjiao
	);
//Mult���� 68 
//[5]ͨ����
//[6]��S��������ǰ·���� ��T��ֹͣ��ǰ·����
DLL_XMT_SER_API void XMT_COMMAND_XWJ_ChannelDoOrStop(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,// 0 1 2 
	unsigned char FlagMult //��S��������ǰ·���� ��T��ֹͣ��ǰ·����
	);

//Mult���� 69 ��S����·ͬ������ 'T'��·ͬʱֹͣ
DLL_XMT_SER_API unsigned char XMT_COMMAND_Assist_Flag(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char FlagMult
	);

//70 ���㲽���洢
DLL_XMT_SER_API unsigned char XMT_COMMAND_SaveDataArrToMCU(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channle_flag,
	unsigned char Flag_AheadOrLeg,//0��ʾǰ48�� 1��ʾ��48��
	float ArrData[],
	unsigned char LongArrData,//���͵����ݵ���0��48,
	float MaxData,//�������ֵ ������� һ��Ϊ150��120
	float MinData//��С����ֵ ����ʵ����Ҫ��������
	);

//71 �趨����ʱ��
//�趨��λ���͵����ʱ��  

//0.2 Ϊ0.2���� 
//0.1 ���Ͳ�����
///�����40����������һ�����Ҳ���
///0.2����40��Ϊ250����
///0.4����40��Ϊ84����
///0.3����40��Ϊ125����
///1��2��4�Ⱥ��뼶��׼ȷ��
////1�뵽5��׼ȷ//
///���˵����� ���е���� û����ϸ������20170814-��һ

DLL_XMT_SER_API void 	XMT_COMMAND_SetMCUSendDataTimer(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	float SendDataTimer//0.1����-9999����֮��
	);


// 72 ����Ԥ���趨�õĳ��� ����/ֹͣ 
DLL_XMT_SER_API void 	XMT_COMMAND_SetMCU_BeginSend(
	unsigned char address,
	unsigned char Command_B3,//36
	unsigned char Command_B4,//0
	unsigned char Channel_Num,//0,1,2
	unsigned char RunFlag//'S'��ʼ    'T' ֹͣ   'P'��ͣ 
	);
// 73 ���ÿ���DA��ʼֵ
DLL_XMT_SER_API void XMT_COMMAND_SetMCU_FlagDa(
	unsigned char address,
	unsigned char Command_B3,//36
	unsigned char Command_B4,//0
	unsigned char Channel_Num,//0,1,2
	unsigned char DaFlag,//'Z' �趨Ϊ0����� 'M'�趨���ֵ���� 
	float FlagForDa//[7][8][9][10] ������ֵ������
	);
// 74 �趨����DA��ʼֵ
DLL_XMT_SER_API void 	XMT_COMMAND_SetMCU_FlagVolt(
	unsigned char address,
	unsigned char Command_B3,//36
	unsigned char Command_B4,//0
	unsigned char Channel_Num,//0,1,2
	float FlagForVolt//[6][7][8][9] DA�����ѹ�ٷֱ� DA��ʼֵ���ֵ�ٷֱ�Ϊ0-1С����0�������0��1�����������ѹֵ
	);

// 75 AD�ɼ�������Ŵ�
DLL_XMT_SER_API void 	XMT_COMMAND_SetMCU_FlagAD(
	unsigned char address,
	unsigned char Command_B3,//36
	unsigned char Command_B4,//0
	unsigned char Channel_Num,//0,1,2
	unsigned char FlagAD,//[6]'Z'��ʼ���� 'M'��ʼ���Ŵ�
	unsigned char FlagCloseOrOpen //[7]'C'�ջ� 'O'����
	);

// 76 ��ȡ��λ�� ��ѹ��λ������
DLL_XMT_SER_API void SendArray_ReadDataChannel_AllVolt(int address_ma,//��ַ��
	int bao_long,  //����
	int zhilingma_B3,//ָ����
	int zhilingma_B4,
	unsigned char DisTimer);//ָ����	

							// 76  ��������ȡ�����ض�8·���� �ڲ�ʹ��8·AD
DLL_XMT_SER_API void SendKeilth(void);//��������
									  //77 ���� �ж��Ƿ���������   
DLL_XMT_SER_API unsigned char CheckConnection(int Now_control_mcu_address,
	int bao_long,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	int WaitTime);//��ȡ�Ƿ��������� ������ӡ�K'��ʾ��������//��ȡ�Ƿ��������� ������ӡ�K'��ʾ��������


short CalData_8(unsigned char tmp_A, unsigned char tmp_B); //����ת��

void DoData(unsigned char tmp_arr[], unsigned char ArrLong, float tmpData_Arr[8]); // tmp_arr[] �������� ���鳤�� ���ص�����������

DLL_XMT_SER_API void SendArray_ReadDataChannel_AllVoltAndRead(int address_ma,//��ַ��
	int bao_long,  //����
	int Command_B3,//ָ����
	int Command_B4,
	unsigned char DisTimer,
	float TmpF[8]);//ָ����



unsigned char XMT_ReDo_proNew(unsigned char comand_Arr[]); //���ݲ�ͬ������������
unsigned char Res_command_proNew(unsigned char T_D_3, unsigned char T_D_4);

//B3=100 B4 =0
//�궨��λ������ //0 1 2 3 4 5 6 7����E17 E18 E72 E73 E70 E53D E709

//20180629 ��֤���
//B3=0 B4 =57 �궨��λ���ͺ�
//�궨��λ��MCU����
//0 E70.S3 1 E18 2 E53 3 E18 24bit 4E51.D12S //20180629

DLL_XMT_SER_API void  XMT_COMMAND_SetMCUNum(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char FlagMCUFlag
	);//0 1 2 3 4 5 6 7����E17 E18 E72 E73 E70 E53D E709
	  //B3=100 B4 =0  78
	  //��ȡ��λ������ //0 1 2 3 4 5 6 7����E17 E18 E72 E73 E70 E53D E709 //20180820 E63 ���� 6 
DLL_XMT_SER_API unsigned char XMT_COMMAND_ReadMCUNum(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);

// 79 �궨��Դ��Ϣ �Ƚ���ϸ����Ϣ
DLL_XMT_SER_API void  XMT_COMMAND_SetPowerConfig(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Flag_SheBeiNum_1,//CUP�ͺ� ARM DSP AVR 0 1 2 3 
	unsigned char Flag_SheBeiNum_2,//������ͨ����;  1 2 3 4 5
	unsigned char Flag_SheBeiNum_3,//��λ���汾���趨 ��ʽV2.0.2 �������趨 ��λ�������� 
	unsigned char Flag_SheBeiNum_4,//
	unsigned char Flag_SheBeiNum_5,// 
	unsigned char Flag_SheBeiNum_6,//ͨ�Ŷ˿���Ϣ  ����/485/USB/422/RS232��0001 1111 ����ȫ֧��  ��X' ��λ
	unsigned char Flag_SheBeiNum_7,//'X'
	unsigned char Flag_SheBeiNum_8,//�Ƿ��ѹ Y�� N��
	unsigned char Flag_SheBeiNum_9,//ȫ�� ����ȫ�� ����  0 1 2 3 
	unsigned char Flag_SheBeiNum_10,//ģ������� ģ������� -3.3V-��+3.3V����궨Ϊ33 33 һ�� ��λ
	unsigned char Flag_SheBeiNum_11,//ģ������� ģ������� -3.3V-��+3.3V����궨Ϊ33 33 һ�� ��λ
	unsigned char Flag_SheBeiNum_12,//���� ��X'��λ
	unsigned char Flag_SheBeiNum_13,//���� ��X'��λ
	unsigned char Flag_SheBeiNum_14,//���� ��X'��λ
	unsigned char Flag_SheBeiNum_15,//���� ��X'��λ
	unsigned char Flag_SheBeiNum_16,//���������� R C L 
	unsigned char Flag_SheBeiNum_17,//���ִ��� 1K 2K 10K 30K
	unsigned char Flag_SheBeiNum_18,//DA�ֱ��� ����ʵ�ʷֱ�������  8 16 18 20 24
	unsigned char Flag_SheBeiNum_19,//AD�ֱ��� ����ʵ�ʷֱ��� 8 16 18 20 24 
	unsigned char commandFlagArr[32],// 0 �� 255 ������
	unsigned char Flag_SheBeiNum_20,//'X' ��λ
	unsigned char Flag_SheBeiNum_21,//'X' ��λ
	unsigned char Flag_SheBeiNum_22,//'X' ��λ
	unsigned char Flag_SheBeiNum_23//'X' ��λ
	);

// B3 = 80,B4 = 0; //20180629 ��ȡ��Դ�����Ϣ
DLL_XMT_SER_API void XMT_COMMAND_ReadPowerConfig(char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char RcArr[61]);

//unsigned char commandFlagArr[32],// 0 �� 255 ������

//B3=81 B4 =21  81 ������ �궨��λ������̨����Ϣ
//�궨̨������ //���� XP 611 P93 N12 ������

DLL_XMT_SER_API void  XMT_COMMAND_SetMoveMNum(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Flag_SheBeiNum_1,//X,N,P
	unsigned char Flag_SheBeiNum_2,//P
	unsigned char Flag_SheBeiNum_3,
	unsigned char Flag_SheBeiNum_4, // ���� 0 �� 255 
	unsigned char Flag_SheBeiNum_5, // ���� 0 �� 255  �������ֵ� �� ���� XP 611 P93 N12 ������
	unsigned char Flag_SheBeiNum_6, //�豸�� ͨ���� 1 2 3 4 5 6ͨ��
	unsigned char Flag_SheBeiNum_lei,//�豸�� Z���˶� B��̨�� �ۺ�����
	unsigned char Flag_SheBeiNum_HengYa,
	unsigned char Flag_YingPianJi//���� ���� ���  0 1 2 
	);
/*
unsigned char Flag_SheBeiNum_1;//�豸�� �ͺ�  X 33
unsigned char Flag_SheBeiNum_2;//�豸�� �ͺ�  P 33
unsigned char Flag_SheBeiNum_3;//�豸�� ���� 0 �� 255 ���� XP 611 P93 N12 ������
unsigned char Flag_SheBeiNum_4;//�豸�� ͨ���� 1 2 3 4 5 6ͨ��
unsigned char Flag_SheBeiNum_lei;//�豸�� Z���˶� B��̨�� �ۺ�����
unsigned char Flag_SheBeiNum_HengYa;//�豸�� �Ƿ��ѹ 1 ����ѹ��0 ����Ǻ�ѹ
unsigned char Flag_YingPianJi;//���� ���� ���  0 1 2
*/


//B3=100 B4 =22
//�궨̨������ //���� XP 611 P93 N12 ������
//unsigned char Flag_SheBeiNum_1,//X,N,P
//unsigned char Flag_SheBeiNum_2,//P
//unsigned char Flag_SheBeiNum_3, // ���� 0 �� 255 
//unsigned char Flag_SheBeiNum_4, // ���� 0 �� 255  �������ֵ� �� ���� XP 611 P93 N12 ������
//unsigned char Flag_SheBeiNum_5, //�豸�� ͨ���� 1 2 3 4 5 6ͨ��
//unsigned char Flag_SheBeiNum_lei,//�豸�� Z���˶� B��̨�� �ۺ�����
//unsigned char Flag_SheBeiNum_HengYa,
//unsigned char Flag_YingPianJi//���� ���� ���  0 1 2 


// B3 = 84��83,B4 = 0; //4·ʵʱ��ȡ
DLL_XMT_SER_API void XMT_COMMAND_ReadDataFourChannel_DisTimer(int address_ma,//��ַ��
	int Command_B3,//ָ����
	int Command_B4,//ָ����	
	unsigned char DisTimer);

//��λ��ģʽ��� T 91 92
DLL_XMT_SER_API void XMT_COMMAND_WaveSetMultWaveXwj_T(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char WaveType,
	double FengFengZhi,
	double PinLvHz,
	double Pianzhi,
	double Xwjiao,
	double Zhouqi
	);
//Mult���� 93 
//[6]��S��������ǰ·���� ��T��ֹͣ��ǰ·����
DLL_XMT_SER_API void XMT_COMMAND_XWJ_ChannelDoOrStop_T(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char FlagMult //��S��������ǰ·���� ��T��ֹͣ��ǰ·����
	);

//��·������ 94 95  
DLL_XMT_SER_API unsigned char  XMT_COMMAND_MultSinglePointT(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	double VoltOrMove_Data_0,
	double VoltOrMove_Data_1,
	double VoltOrMove_Data_2,
	double VoltOrMove_Data_3
	);


//98 ������ ǿ���趨��ǰ��ͨ������״̬��Ϊ����Ĭ�ϲ���
DLL_XMT_SER_API void XMT_COMMAND_MakeWaveBeginSet_Ser(
	unsigned char address_ma,
	unsigned char zhilingma_B3,
	unsigned char zhilingma_B4,
	unsigned char SendFlag
	); //SendFlag 1 Ĭ�ϲ��ι��� 0��ʾ����Ĭ�� �����������Ͳ��ι���


DLL_XMT_SER_API void  XMT_COMMAND_ReadMoveMNum(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char ReadArrTmp[9]//��ȡ��������
	);
//unsigned char address, ��ַ����
//unsigned char Channel_Num, ��Ӧͨ����
//unsigned char flagOpenOrClose,���ջ�����
//double Point_A,//��ʼ������
//double Point_B,//��ֹ������
//int ArrLong,//���ݵļ����
//int tmpUs_100us//�����֮���ʱ����


// 101  �Զ�У׼���� 
DLL_XMT_SER_API void XMT_COMMAND_Mudify_MorZ(
	int address,//��ַ��
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char ModFlag//'Z'�趨0����� ��M'�趨���ֵ����
	);

//113  ��ȡ4·��ѹ����ֵ ��λ������ͬ���Ŀ���״̬
//114  ��ȡ4·λ���ƾ�ֵ ��λ������ͬ���Ŀ���״̬
	DLL_XMT_SER_API void 	XMT_COMMAND_ReadMultChannelVolt(
		unsigned char address,
		unsigned char Command_B3,
		unsigned char Command_B4,
		double *OpenOrCloseData_0, //���ջ����� 
		double *OpenOrCloseData_1, //���ջ�����
		double *OpenOrCloseData_2, //���ջ�����
		double *OpenOrCloseData_3  //���ջ�����
		);

//115 �л���Ӣ�Ľ���  ShowFlag 0 ��λ 1 Ӣ��
DLL_XMT_SER_API void 	XMT_COMMAND_ChangeShow(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char ShowFlag
	);

//B3=118 B4 =0 E80 ���ưھ���Բ�Լ�ֱ��
DLL_XMT_SER_API void  XMT_COMMAND_LineABAndWRound(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	double Apoint_1,//��λ������
	double Apoint_2,//��λ������
	double Bpoint_1,//��λ������
	double Bpoint_2,//��λ������
	int DisTime,//��λ΢��
	double R,//
	double PHz,//Ƶ��,
	double RInt//��Բ����,
	);

//119
//ֹͣ 118���߹���
DLL_XMT_SER_API void XMT_COMMAND_LineABAndWRound_Stop(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);


//�궨IP��ַ 120 
DLL_XMT_SER_API void XMT_COMMAND_Net_SetPI(
	unsigned char address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char MacArr[6],//�����ַ(MAC) 
	unsigned char MacIP[4],//������IP��ַ
	unsigned char MacIPzw[4],//���������������ַ
	unsigned char MacIPwg[4],//����������IP��ַ 
	unsigned char MacIPDNS[4],//DNS��ַ��ַ
	unsigned char MacUDPPort[2]//UDP�˿ں�
	);

//��ȡIP��ַ 121 ��ȡ����������
DLL_XMT_SER_API void XMT_COMMAND_Net_ReadPI(
	unsigned char address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char MacArr[6],//�����ַ(MAC) 
	unsigned char MacIP[4],//������IP��ַ
	unsigned char MacIPzw[4],//���������������ַ
	unsigned char MacIPwg[4],//����������IP��ַ 
	unsigned char MacIPDNS[4],//DNS��ַ��ַ
	int MacUDPPort[1]//UDP�˿ں�
	);


//Ӳ��IO�������ܿ��� 126
DLL_XMT_SER_API void XMT_COMMAND_SetIO_OpenOrClose(
	unsigned char address,
	int Command_B3,//ָ����
	int Command_B4,//ָ���� 
	unsigned char FlagOPenOrClose//[5]��������     ='O'����Ӳ��IO���� ='C'�ر�Ӳ��IO���� 
	);


//�����ⲿIO���� ��δ������ߵ��δ���   127   
//[5]����ģʽ
//= 0x01 ���δ���
//= 0x02 ���
DLL_XMT_SER_API void XMT_COMMAND_SetIO_MultOrSingle(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ���� 
	unsigned char FlagMultOrSingle//[5]����ģʽ =0x01 ���δ��� =0x02 ��δ��� 
	);


//Ӳ��IO�������ܴ���ͨ��ѡ��   128
//[5]ͨ����  =0x00 �����һͨ��=0x01 ����ڶ�ͨ�� =0x02 �������ͨ��='N' ����û��ѡ�� ='A' ����ѡ������ͨ�� 
DLL_XMT_SER_API void XMT_COMMAND_SetIO_ChChose(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ���� 
	unsigned char tmpCf//  =0x00 �����һͨ��=0x01 ����ڶ�ͨ�� =0x02 �������ͨ��='N' ����û��ѡ�� ='A' ����ѡ������ͨ��
	);


// 129  Ӳ��IO����������ʱ������
DLL_XMT_SER_API void XMT_COMMAND_SetIO_ConfigDisTimer(
	unsigned char address,
	int Command_B3,//ָ����
	int Command_B4,//ָ���� 
	int DisTimer //���ʱ�� ÿ����֮��ʱ��������λΪus����С���Ϊ100us
	);
	//[5][6][7][8]��һ��int�����ݵ�4���ֽ���ɡ���ȷ��ʱ����Ϊ100us, �����int������Ϊ100��ת��Ϊʮ������Ϊ0x00000064, �ֳ��ĸ��ֽ������ݣ���Ϊ0x00, 0x00, 0x00, 0x64


//���͵�ͨ������������  130 
DLL_XMT_SER_API void  XMT_COMMAND_saveDataToCh_Run(
	unsigned char address,
	int Command_B3,//ָ����
	int Command_B4,//ָ���� 
	unsigned char FlagCh,//0x00 0x01 0x02
	unsigned char FlagVoltOrMove,//ͨ������λ�� 0��ѹ 1λ��
	unsigned char NumBao,//��ǰ���ݰ�Ϊ�ڼ������ݰ�
	unsigned char SendDatalong,//�������ݵ���
	float SendArrUse[50],
	unsigned char FlagSendEndOrNot//�����Ƿ����
	);

//���д������ 131 
DLL_XMT_SER_API void XMT_COMMAND_ClearIO_Config(
	unsigned char address,
	int Command_B3,//ָ����
	int Command_B4,//ָ���� 
	unsigned char FlagCh//0x00 0x01 0x02  'A'����ѡ������ͨ�� 
	);

// Ӳ��IO���ܴ��� ����ֹͣ/��ͣ  132
DLL_XMT_SER_API void XMT_COMMAND_StopIO_Run(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ���� 
	unsigned char FlagCh,//0x00 0x01 0x02  'A'����ѡ������ͨ�� 
	unsigned char FlagStop//ֹͣ��ʽ 'T'����ֹͣ  'P'������ͣ
	);

//133 �趨ָ��ͨ����ǰλ��Ϊλ����㣨�ջ�״̬�£�����״̬������
DLL_XMT_SER_API void XMT_COMMAND_ZeroSet(
	unsigned char address,
	int Command_B3,//ָ����
	int Command_B4,//ָ���� 
	unsigned char FlagCh//0x00 0x01 0x02
	);

//134 �趨��ǰͨ����Դ����  
//[6]��S��������
//��D��˫����
DLL_XMT_SER_API void XMT_COMMAND_SetIO_ChReInit(
	unsigned char address,
	int Command_B3,//ָ����
	int Command_B4,//ָ���� 
	unsigned char tmpCf,//  =0x00 �����һͨ��=0x01 ����ڶ�ͨ�� =0x02 �������ͨ��='N' ����û��ѡ�� ='A' ����ѡ������ͨ��
	unsigned char Flag
	);

// 135 ��E18ʹ��
//1.A��λ�ơ�B��λ�ơ����ռ��λ�ơ�A�㵽B���˶�ʱ��Ϊfloat�����ݣ�������ʽ�͹�˾�ڲ����ڵ�ѹ��λ�ƵĴ���ʽһ��.
//2.����Ƶ�ʺ����մ���Ϊuint32�ͣ��޷������͡�
DLL_XMT_SER_API void XMT_COMMAND_Set_PoitATimeAndTimes(
	unsigned char address,
	int Command_B3,//ָ����
	int Command_B4,//ָ���� 
	unsigned char FlagCh,//0x00 0x01 0x02  
	double PointA,//A��λ�� 
	double DisMove,//���ռ��λ��
	int PZHz,//����Ƶ��(�� / S)
	int PZCishu,//���մ���
	unsigned char *Rech,//����ͨ��
	double *pointB, //����B��ֵ
	int *runtime//A��B������ʱ��
	);

//136
//��ʼ��ֹͣ���ն��� ��E18ʹ��
DLL_XMT_SER_API void XMT_COMMAND_Begin_PoitATimeAndTimes(
	unsigned char address,
	int Command_B3,//ָ����
	int Command_B4,//ָ���� 
	unsigned char FlagCh,//0x00 0x01 0x02  'A'����ѡ������ͨ�� 
	unsigned char BeginFlag //'R'����'H'ֹͣ
	);

//137 ���ô���IO�ڵ�ƽ ��E18ʹ��
DLL_XMT_SER_API void XMT_COMMAND_SetFlagOut_PoitATimeAndTimes(
	unsigned char address,
	int Command_B3,//ָ����
	int Command_B4,//ָ���� 
	unsigned char FlagCh,//0x00 0x01 0x02  
	unsigned char SetFlagOut //��H���ߵ�ƽ��L���͵�ƽ
	);


//138 ��·���λ��
DLL_XMT_SER_API void XMT_COMMAND_XD_SinglePoint(
	unsigned char address,
	int Command_B3,//ָ����
	int Command_B4,//ָ���� 
	unsigned char Channel_Num,
	float VoltOrMove_Data
	);

//139
DLL_XMT_SER_API void XMT_COMMAND_SaveDataArrToMCU_pro(
		unsigned char address,
		unsigned char Command_B3,
		unsigned char Command_B4,
		unsigned char  Channle_Useflag, //���뱾�����ݴ����ͨ���ż��Ƿ���Ҫ��E2PROM    
		int Ch1_Num,//ͨ������
		int Ch2_Num,//ͨ������
		int Ch3_Num,//ͨ������
		int Ch4_Num,//ͨ������
		float ArrData_Ch1[],
		float ArrData_Ch2[],
		float ArrData_Ch3[],
		float ArrData_Ch4[],
		float MaxData1,//�������ֵ ������� һ��Ϊ150��120
		float MinData1,//��С����ֵ ����ʵ����Ҫ��������
		float MaxData2,//�������ֵ ������� һ��Ϊ150��120
		float MinData2,//��С����ֵ ����ʵ����Ҫ��������
		float MaxData3,//�������ֵ ������� һ��Ϊ150��120
		float MinData3,//��С����ֵ ����ʵ����Ҫ��������
		float MaxData4,//�������ֵ ������� һ��Ϊ150��120
		float MinData4//��С����ֵ ����ʵ����Ҫ��������
		);

//140
DLL_XMT_SER_API void XMT_COMMAND_SetMCUSendDataTimer_Pro(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char  Channle_Useflag, //��4Ϊ�����ͨ���Ƿ������� 
	float ChUseTime_1,
	float ChUseTime_2,
	float ChUseTime_3,
	float ChUseTime_4
	);

//141
DLL_XMT_SER_API void XMT_COMMAND_SetMCU_BeginStopOrPauseSend(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char  Channle_Useflag, //��4Ϊ�����ͨ���Ƿ������� 
	unsigned char ChUseFlag_1,
	unsigned char ChUseFlag_2,
	unsigned char ChUseFlag_3,
	unsigned char ChUseFlag_4
	);//20230423

//149 ����IAP ָ��
DLL_XMT_SER_API void XMT_COMMAND_IAP(unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4);


////////////////////////////////////////////////////////////////////





DLL_XMT_SER_API void ArrDataSendAToB(unsigned char address, unsigned char Channel_Num, unsigned char flagOpenOrClose, double Point_A, double Point_B, int ArrLong, int tmpUs_100us);


//unsigned char address;��ַ����
//unsigned char Channel_Num;��Ӧͨ��
//unsigned char flagOpenOrClose;���ջ����� O ���� C �ջ�
//double Point_A;//��ʼ������
//double Point_B;//��ֹ������
//float AToBRunTime;A�㵽B�������ʱ�� 
//int BTL;//������ 9600 115200 ��ǰ�Ĳ�����
DLL_XMT_SER_API void PointAToPointBAndRunTime(unsigned char address, unsigned char Channel_Num, unsigned char flagOpenOrClose, double Point_A, double Point_B, float AToBRunTime, int BTL);//

unsigned char* DataAnla_ProYD(double f, unsigned char kk[4]);
unsigned char* DataAnla_ProXZ(double f, unsigned char kk[4]);
DLL_XMT_SER_API void  DataAnla_ProXZ_20210910(double f, unsigned char kk[4]);

long CalDataYD(unsigned char kk0, unsigned char kk1, unsigned char kk2, unsigned char kk3);// 20200529 �����ĸ���㷨���� 
float CalDataXZ_20210910(unsigned char kk0, unsigned char kk1, unsigned char kk2, unsigned char kk3);// 20210910 �����ĸ���㷨����

																									 //unsigned char receive_usb_info_CheckTimeOut(int NumOfLibusbDevice, unsigned char receive_arr[], int TimeOutUse); //��ȡ��USB������ֵ //TimeOutUse 20200624 Ĭ�ϵĲ�ѯʱ��
unsigned char receive_usb_info_CheckTimeOut(unsigned char receive_arr[], int Arrlong, int TimeOutUse); //��ȡ��USB������ֵ //TimeOutUse 20200624 Ĭ�ϵĲ�ѯʱ��







																									   //2022������ ͬʱ������ͨ����ѹ
																									   ///////////////////////////////////////////////////////////////////
unsigned int FloatToDa_16bit(float SendData, float MaxDataSend, float MinDataSend);//�������� ���ֵ ��Сֵ 
void ChangIntToTwoUChar(int tmpInt, unsigned char TmpChar[2]);//����������ת��Ϊ����Char���͵�����
DLL_XMT_SER_API void SendSixVolt(unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	double TmpVolt1, double TmpVolt2, double TmpVolt3, double TmpVolt4, double TmpVolt5, double TmpVolt6,
	float MaxDataSend, float MinDataSend);
//ͨ�����Ƶ�ѹ
DLL_XMT_SER_API void SendSixChVolt(unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char ChUse,
	double TmpVolt1,
	float MaxDataSend, float MinDataSend);

//��ȡ3ͨ����ѹ ͬʱ��ȡ
DLL_XMT_SER_API long ReadCh3Volt(unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	double TmpVoltArr[3],
	float MaxDataSend, float MinDataSend,
	int disTimeus);

//������Ŀ
//���ò����� ��ַ 01 ��ʼ; ͨ�� 01 ��ʼ 
//DB1����ֵ		    A	     B	    C	    D	    E
//��Ӧ������		9600	115200	256000	1382400	5250000
DLL_XMT_SER_API void SetBound(unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char FlagBound
	);
//��ȡ��ͨ����ѹ��
DLL_XMT_SER_API double ReadChOneVolt(unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char ChUse,
	float MaxDataSend, float MinDataSend,
	int disTimeus);


///////////////////////////////////////////////////////////////////


//B3 = 0 �����ٶ����� B4 = 1;
DLL_XMT_SER_API void XMT_COMMAND_YDMoveSPD(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, double SPD_f);//���������ٶ�//��ַ ͨ�� 

									////////////////B4 = 1;
									//B3 = 1 ����λ������
DLL_XMT_SER_API float XMT_COMMAND_YDAbMoveX(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, double MoveX_f, unsigned char ReFlag);//����λ��  unsigned char ReFlag 0 ������ 1 ��ʾ����

															// ���λ������
															//B3 = 2 B4 = 1
DLL_XMT_SER_API float XMT_COMMAND_YDReMoveX(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, float MoveX_f, unsigned char ReFlag);//��Զ�λ�� unsigned char ReFlag 0 ������ 1 ��ʾ����


														   //B3 = 3; B4= 1;
DLL_XMT_SER_API float XMT_COMMAND_YDCTUMove(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch,//ͨ����
	double SPD_f,//�������е��ٶ�����
	unsigned char ROrLFlag,//�������־λ:'R'��ʾ����'L'��ʾ����
	unsigned char ReFlag);//�������� float SPD_f �����ٶ� ,unsigned char ROrLFlag L������ R������ unsigned char ReFlag 0 ������ 1 ��ʾ����
						  //
						  //B3 = 4; B4= 1;
DLL_XMT_SER_API float XMT_COMMAND_YDCTUMoveStop(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, unsigned char ReFlag, unsigned char JOrXFlag);
//��������ֹͣ���� unsigned char ReFlag 0 ������ 1 ��ʾ����,unsigned char JOrXFlag 1���� 0���

// ����ǰλ������ 
//B3 = 5; B4= 1;
DLL_XMT_SER_API double XMT_COMMAND_YDReMoveF(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, unsigned char JOrXFlag); //unsigned char JOrXFlag 1���� 0���

	//108 ���������
	 //B3 = 6; B4= 1;

DLL_XMT_SER_API double XMT_COMMAND_YDSetCTZero(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����w
	unsigned char Ch, double ZerX_f, unsigned char ReFlag); //��������� unsigned char ReFlag 0 ������ 1 ��ʾ����

															//�������λ
															//B3 = 7; B4= 1;
DLL_XMT_SER_API float XMT_COMMAND_YDReadCTZero(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch); //��ȡ�����

					   //110 ���ص���λ�� �־��ԣ��������
					   //B3 = 8; B4= 1;
DLL_XMT_SER_API float XMT_COMMAND_YDRBackZero(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, unsigned char JOrXFlag, unsigned char ReFlag); //������λ�� unsigned char JOrXFlag 1���� 0��� unsigned char ReFlag 0 ������ 1 ��ʾ����

																	 //111 ѹ�粨������
																	 //B3 = 9; B4= 1;
DLL_XMT_SER_API void 	XMT_COMMAND_YDWave(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char WaveType,
	double FengFengZhi,
	double PinLvHz,
	double Pianzhi
	);

//112 ѹ�����ֹͣ���� ���ص�ǰ����λ��
//B3 = 10; B4= 1;
DLL_XMT_SER_API double XMT_COMMAND_YDStopAll(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num
	);

//����ѹA��B�������ٶ�
//B3 = 11; B4= 1;
DLL_XMT_SER_API void XMT_COMMAND_YDAbMoveA_BSpeed(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, double MoveX_A, double MoveX_B);
//��usb�ӿ��ж�ȡ A B��ķ����ٶ�
DLL_XMT_SER_API double XMT_COMMAND_RecA_BSpeed(unsigned char Ch, int TimeOutUse);

//B3 = 12 B4 = 1 �������λ��
DLL_XMT_SER_API double XMT_COMMAND_YDAbSetLimit(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch,
	double SetDataLimit);

//������λ��
//B3 = 13 B4 = 1
DLL_XMT_SER_API float XMT_COMMAND_YDReadAbLimit(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch); //��ȡ�����

//B3 = 14 B4 = 1 �������λ��
DLL_XMT_SER_API double XMT_COMMAND_YDAbSetHighLimit(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch,
	double SetDataLimit);

//������λ��
//B3 = 15 B4 = 1
DLL_XMT_SER_API float XMT_COMMAND_YDReadAbHighLimit(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch); //��ȡ�����

//������λУ׼
//B3 = 16 B4 = 1
DLL_XMT_SER_API void XMT_COMMAND_CorrectAbZero(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char ReFlag
	);


//����Э����
//���� ���ջ� 17
//B3 = 17 B4 = 1
DLL_XMT_SER_API void 	XMT_COMMAND_YDZX_Assist_SetFlag(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char SetFlag
	);
//18 
//��ȡ���ջ�
//B3 = 18 B4 = 1
DLL_XMT_SER_API unsigned char XMT_COMMAND_YDZX_Assist_ReadFlag(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num
	);


//B3 = 19 B4 = 1
//���ε��Թ���
DLL_XMT_SER_API void XMT_COMMAND_YDZX_SendWave(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch,
	unsigned char WaveType,
	double FengFengZhi,
	double PinLvHz,
	double Pianzhi,
	unsigned char RunZhengOrFanFlag,//������ 'P' 'N'
	unsigned char FlagRunForever,//һֱ�˶�0x01 ���������˶� 0x00
	long long RunCi,
	unsigned char  Percent
	);

//B3 = 20 B4 = 1
//���ε��Թ���
DLL_XMT_SER_API void XMT_COMMAND_YDZX_StopWave(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch
	);
//ֱ�����
//B3 = 21; B4= 1;  
//����PID����
DLL_XMT_SER_API void XMT_COMMAND_YD_SendArray_PID_Channel(
	int address,//��ַ��
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	float PID_P,
	float PID_I,
	float PID_D
	);

//ֱ�����
//B3 = 22; B4= 1;  
//��ȡPID����
DLL_XMT_SER_API void XMT_COMMAND_YD_Read_PID_Channel(
	int address,//��ַ��
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	float PID_Rc[3]
	);

// ʵʱ���ص�ǰλ��
//B3 = 23 B4 = 1;
DLL_XMT_SER_API void 	XMT_COMMAND_YD_ReadData_TS(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char TimerSet_ms
	);


//ֹͣ��ȡ  ʵʱ���ص�ǰλ��
//B3 = 24 B4 = 1;
DLL_XMT_SER_API void 	XMT_COMMAND_YD_ReadData_Stop(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);

//�趨�ջ�����
//B3 = 25 B4 = 1;
DLL_XMT_SER_API void XMT_COMMAND_YD_JDu(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char FlagJD
	);

/////////////////////////////////////////////////


void DataLongToArr_8(long long RunCi, unsigned char TmpDataArr[]);
unsigned char* DataAnla_ProLD(long IntTmp, unsigned char kk[4]);
int CalDataLD(unsigned char kk0, unsigned char kk1, unsigned char kk2, unsigned char kk3);
int SendArray_ReadDataChannel_LDRcy(int bao_long, unsigned char ArrRec[], unsigned char ChType);
// ѹ���ݶ�
//B3 = 0; B4= 2;
DLL_XMT_SER_API void XMT_COMMAND_YD_LDing_SendWave(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch,
	unsigned char WaveType,
	double FengFengZhi,
	double PinLvHz,
	double Pianzhi,
	unsigned char RunZhengOrFanFlag,//������ 'P' 'N'
	unsigned char FlagRunForever,//һֱ�˶�0x01 ���������˶� 0x00
	long long RunCi,
	unsigned char  Percent
	);
// ѹ���ݶ�
//B3 = 1; B4= 2;
DLL_XMT_SER_API void XMT_COMMAND_YD_LDing_SendWaveStop(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch
	);

// ѹ���ݶ� ѹ���ݶ�����˶�
//B3 = 2; B4= 2;
// ѹ���ݶ� ѹ���ݶ������˶�
//B3 = 3; B4= 2;
DLL_XMT_SER_API void XMT_COMMAND_YD_LDing_Move(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch,
	unsigned char RunZhengOrFanFlag,//[6]'P'����'N'����
	long RunCi
	);

//B3 = 4; B4= 2;
DLL_XMT_SER_API int XMT_COMMAND_YD_LDing_ReadZQ(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch
	);

//B3 = 5; B4= 2;
//ѹ���ݶ� ͨ����������
DLL_XMT_SER_API void XMT_COMMAND_YD_LDing_ZeroCyc(
	unsigned char Address,
	unsigned char Command_B3,//ָ����
	unsigned char Command_B4,//ָ����
	unsigned char Ch);

//B3 = 7; B4= 2;
//ѹ���ݶ� ���ø߷ֱ����Լ��ͷֱ���
DLL_XMT_SER_API void XMT_COMMAND_YD_LDing_HOrLowFBL_S(unsigned char Address,
	unsigned char Command_B3,//ָ����
	unsigned char Command_B4,//ָ����
	unsigned char HorLowFlag// [5]'1'�߷ֱ��� [5]'0'�ͷֱ���
	);

//ѹ���ݶ��ջ�
//B3 = 8 �����ٶ����� B4 = 2;  �ٶ� 0-100��ʾ�ٶ�0-100%
DLL_XMT_SER_API void XMT_COMMAND_YD_LDingMoveSPD(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, double SPD_f);//���������ٶ�//��ַ ͨ�� 

									//ѹ���ݶ�
									//B3 = 9; B4= 2; ����λ������
DLL_XMT_SER_API float XMT_COMMAND_YD_LDingAbMoveX(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, double MoveX_f, unsigned char ReFlag);//����λ��  unsigned char ReFlag 0 ������ 1 ��ʾ����

															//ѹ���ݶ�
															//B3 = 10; B4= 2;����λ������
DLL_XMT_SER_API float XMT_COMMAND_YD_LDingAddMoveX(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, double MoveX_f, unsigned char ReFlag);//����λ������  unsigned char ReFlag 0 ������ 1 ��ʾ����

															//ѹ���ݶ�
															//B3 = 11; B4= 2; ����ǰλ������;
DLL_XMT_SER_API double XMT_COMMAND_YD_LDingReMoveF(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, unsigned char JOrXFlag); //unsigned char JOrXFlag 1���� 0���

											   //ѹ���ݶ�
											   //B3 = 12; B4= 2;  //���ص���λ��
DLL_XMT_SER_API float XMT_COMMAND_YD_LDingRBackZero(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, unsigned char JOrXFlag, unsigned char ReFlag); //������λ�� unsigned char JOrXFlag 1���� 0��� unsigned char ReFlag 0 ������ 1 ��ʾ����
																	 //ѹ���ݶ�
																	 //B3 = 13; B4= 2;  //����ֹͣ����
DLL_XMT_SER_API double XMT_COMMAND_YD_LDingStopAll(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num
	);
//ѹ���ݶ�
//B3 = 14; B4= 2;  //��λУ׼(�ҹ�դ��0λ��)
DLL_XMT_SER_API double XMT_COMMAND_YD_LDingCorrectAbZero(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char ReFlag
	);
//ѹ���ݶ�
//B3 = 15; B4= 2;  ���ÿ��ջ�״̬ 
DLL_XMT_SER_API void 	XMT_COMMAND_Assist_YD_LDingSetFlag(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char SetFlag
	);
//ѹ���ݶ�
//B3 = 16; B4= 2;  ��ȡ���ջ�״̬ 
DLL_XMT_SER_API unsigned char XMT_COMMAND_Assist_YD_LDingReadFlag(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num
	);
//ѹ���ݶ�
//B3 = 17; B4= 2;  �趨��λ����λ  [5] 0���� 1������  2΢���� 3�� 4��դ��һ�̶� 5������������ 6΢�� 7���� 8����
DLL_XMT_SER_API void XMT_COMMAND_YD_LDing_SetUnit(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char UnitFlag
	);//[5] 0���� 1������  2΢���� 3�� 4��դ��һ�̶�	

	  //ѹ���ݶ�
	  //B3 = 18; B4= 2;  ��ȡ��λ����λ [5] 0���� 1������  2΢���� 3�� 4��դ��һ�̶� 5������������ 6΢�� 7���� 8����
DLL_XMT_SER_API unsigned char XMT_COMMAND_YD_LDing_ReadUnit(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);
//ѹ���ݶ�
//B3 = 19; B4= 2;  //����PID����
DLL_XMT_SER_API void XMT_COMMAND_YD_LDing_SendArray_PID_Channel(
	int address,//��ַ��
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	float PID_P,
	float PID_I,
	float PID_D
	);//��������

	  //ѹ���ݶ�
	  //B3 = 20; B4= 2;  
	  //��ȡPID����
DLL_XMT_SER_API void XMT_COMMAND_YD_LDing_Read_PID_Channel(
	int address,//��ַ��
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	float PID_Rc[3]
	); //��������
	   //ѹ���ݶ�
	   //B3 = 21; B4= 2;  // ʵʱ���ص�ǰλ��
DLL_XMT_SER_API void 	XMT_COMMAND_YD_LDing_ReadData_TS(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char TimerSet_ms
	);
//ѹ���ݶ�
//B3 = 22; B4= 2;  //ֹͣ��ȡ  ʵʱ���ص�ǰλ��
DLL_XMT_SER_API void 	XMT_COMMAND_YD_LDing_ReadData_Stop(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);

//�趨��ǰλ��Ϊ�̶���0�̶�
//ѹ���ݶ�
//B3 = 23; B4= 2;  [5]'Z'�궨����0 ��O���궨��ǰλ�� ��д��ĸ'O'
DLL_XMT_SER_API void 	XMT_COMMAND_YD_LDing_SerZeroOrDel(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char FlagZeroOrDelete
	);
//ѹ���ݶ�
//B3 = 24; B4= 2; �����λ��
//B3 = 26; B4= 2; �����λ��
DLL_XMT_SER_API void 	XMT_COMMAND_YD_LDing_SetSystemInfo(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	double SystemInfo
	);
///ѹ���ݶ�
//B3 = 25; B4= 2; 21 ��ȡ����
//B3 = 27; B4= 2; 21 ���ߵ���
//�������� 
DLL_XMT_SER_API double XMT_COMMAND_YD_LDing_ReadSystemInfo(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num
	);

///ѹ���ݶ�
//B3 = 28; B4= 2;  �趨�ջ�����
DLL_XMT_SER_API void XMT_COMMAND_YD_LDing_SetPrecision(unsigned char Address,
	unsigned char Command_B3,//ָ����
	unsigned char Command_B4,//ָ����
	unsigned char PrecisionValue// [5] 0��255
	);

///ѹ���ݶ�
//B3 = 29; B4= 2; �궨ʵ������ٶ�
DLL_XMT_SER_API void XMT_COMMAND_YD_LDing_SetMaxSpeed(unsigned char Address,
	unsigned char Command_B3,//ָ����
	unsigned char Command_B4//ָ����
	);

///ѹ���ݶ�
//B3 = 30; B4= 2;
//��ȡ�궨ʵ������ٶ�
DLL_XMT_SER_API double XMT_COMMAND_YD_LDing_ReadMaxSpeed(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);

///ѹ���ݶ�
//B3 = 31; B4= 2;
//����ʵ���ٶ�
DLL_XMT_SER_API void XMT_COMMAND_YD_LDing_SetReadMaxSpeed(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	double SystemInfo
	);


///ѹ���ݶ�
//B3 = 32; B4= 2;
//���ù�դ�����ݷ��� [5]0���� 1������
DLL_XMT_SER_API void XMT_COMMAND_YD_LDing_SetGSCZorF(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char SetFlag 
	);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//��ת���
//B3 = 0 �����ٶ����� B4 = 3;
DLL_XMT_SER_API void XMT_COMMAND_XZMoveSPD(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, double SPD_f);//���������ٶ�//��ַ ͨ�� 

									//��ת���
									//B3 = 1; B4= 3; ����λ������
DLL_XMT_SER_API float XMT_COMMAND_XZAbMoveX(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, double MoveX_f, unsigned char ReFlag);//����λ��  unsigned char ReFlag 0 ������ 1 ��ʾ����

															//��ת���
															//B3 = 2; B4= 3; ����λ������
DLL_XMT_SER_API float XMT_COMMAND_XZAddMoveX(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, double MoveX_f, unsigned char ReFlag);//����λ��  unsigned char ReFlag 0 ������ 1 ��ʾ����

															//��ת���
															//B3 = 3; B4= 3 ����ǰλ������;
DLL_XMT_SER_API double XMT_COMMAND_XZReMoveF(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, unsigned char JOrXFlag); //unsigned char JOrXFlag 1���� 0���

											   //��ת���
											   //B3 = 4; B4= 3;  //���ص���λ��
DLL_XMT_SER_API float XMT_COMMAND_XZRBackZero(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch, unsigned char JOrXFlag, unsigned char ReFlag); //������λ�� unsigned char JOrXFlag 1���� 0��� unsigned char ReFlag 0 ������ 1 ��ʾ����


																	 //��ת���
																	 //B3 = 5; B4= 3;  //����ֹͣ����
																	 //��ת���
																	 //B3 = 5; B4= 3;  //����ֹͣ����
DLL_XMT_SER_API double XMT_COMMAND_XZStopAll(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char FlagReData
	);

//��ת���
//B3 = 6; B4= 3;  //��λУ׼(�ҹ�դ��0λ��)
DLL_XMT_SER_API double XMT_COMMAND_XZCorrectAbZero(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char SetFlag
	);

//��ת���
//B3 = 7; B4= 3;  ���ÿ��ջ�״̬ 
DLL_XMT_SER_API void 	XMT_COMMAND_Assist_XZSetFlag(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char SetFlag
	);
//��ת���
//B3 = 8; B4= 3;  ��ȡ���ջ�״̬ 
DLL_XMT_SER_API unsigned char XMT_COMMAND_Assist_XZReadFlag(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num
	);
//��ת���
//B3 = 9; B4= 3;  ���Ͳ������� 
DLL_XMT_SER_API void XMT_COMMAND_XZ_SendWave(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch,
	unsigned char WaveType,
	double FengFengZhi,
	double PinLvHz,
	double Pianzhi,
	unsigned char RunZhengOrFanFlag,//������ 'P' 'N'
	unsigned char FlagRunForever,//һֱ�˶�0x01 ���������˶� 0x00
	long long RunCi,
	unsigned char  Percent
	);

//��ת���
//B3 = 10; B4= 3; ͣ��������
DLL_XMT_SER_API void XMT_COMMAND_XZ_SendWaveStop(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char Ch
	);

//��ת���
//B3 = 11; B4= 3; �趨��λ����λ
DLL_XMT_SER_API void XMT_COMMAND_XZ_SetUnit(
	unsigned char Address,
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	unsigned char UnitFlag
	);//[5] 0���� 1������  2΢���� 3�� 4��դ��һ�̶� 5������������ 6΢�� 7���� 8����

	  //��ת���
	  //B3 = 12; B4= 3;  ��ȡ��λ����λ ��ת��� 
DLL_XMT_SER_API unsigned char XMT_COMMAND_XZ_ReadUnit(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);
//��ת���
//B3 = 13; B4= 3;  ��ȡ��λ����λ ��ת��� 
//����PID����
DLL_XMT_SER_API void XMT_COMMAND_XZ_SendArray_PID_Channel(
	int address,//��ַ��
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	float PID_P,
	float PID_I,
	float PID_D
	);//��������
	  //��ת���
	  //B3 = 14; B4= 3;  ��ȡ��λ����λ ��ת��� 
	  //��ȡPID����
DLL_XMT_SER_API void XMT_COMMAND_XZ_Read_PID_Channel(
	int address,//��ַ��
	int Command_B3,//ָ����
	int Command_B4,//ָ����
	float PID_Rc[3]
	); //��������
	   // ʵʱ���ص�ǰλ��
	   //B3 = 15 B4 = 3;
DLL_XMT_SER_API void 	XMT_COMMAND_XZ_ReadData_TS(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	unsigned char TimerSet_ms
	);
//��ת���
//ֹͣ��ȡ  ʵʱ���ص�ǰλ��
//B3 = 16 B4 = 3; 
DLL_XMT_SER_API void 	XMT_COMMAND_XZ_ReadData_Stop(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);
//��ת���
//�趨��ǰλ��Ϊ�̶���0�̶�
//B3 = 17 B4 = 3;  [5]'Z'�궨����0 ��O���궨��ǰλ��
DLL_XMT_SER_API void 	XMT_COMMAND_XZ_SerZeroOrDel(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char FlagZeroOrDelete
	);


//��ת��� 
//B3 = 18�����λ�� 20 �궨����λ�� B4 = 20;
DLL_XMT_SER_API void 	XMT_COMMAND_XZ_SetSystemInfo(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	double SystemInfo
	);
//��ת��� 
//B3 = 19 ��ȡ����  21 ��ȡ����
//�������� 
DLL_XMT_SER_API double XMT_COMMAND_XZ_ReadSystemInfo(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num
	);

//��ת���
//B3 = 22; B4= 3;
//���ø߷ֱ����Լ��ͷֱ���
DLL_XMT_SER_API void XMT_COMMAND_XZ_HOrLowFBL_S(unsigned char Address,
	unsigned char Command_B3,//ָ����
	unsigned char Command_B4,//ָ����
	unsigned char HorLowFlag// [5]'1'�߷ֱ��� [5]'0'�ͷֱ���
	);

//��ת���
//B3 = 23; B4= 3;
//�趨�ջ�����
DLL_XMT_SER_API void XMT_COMMAND_XZ_SetPrecision(unsigned char Address,
	unsigned char Command_B3,//ָ����
	unsigned char Command_B4,//ָ����
	unsigned char PrecisionValue// [5] 0��255
	);

//��ת��� ��λ�� ��һȦ ���ն������У��Զ�������ٶȣ�
//B3 = 24; B4= 3;
//�궨ʵ������ٶ�
DLL_XMT_SER_API void XMT_COMMAND_XZ_SetMaxSpeed(unsigned char Address,
	unsigned char Command_B3,//ָ����
	unsigned char Command_B4//ָ����
	);

//��ת��� 
//B3 = 25 B4 = 3
//��ȡ�궨ʵ������ٶ�
DLL_XMT_SER_API double XMT_COMMAND_XZ_ReadMaxSpeed(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);

//��ת��� 
//B3 = 26 B4 = 3;
//����ʵ���ٶ�
DLL_XMT_SER_API void XMT_COMMAND_XZ_SetReadMaxSpeed(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4,
	unsigned char Channel_Num,
	double SystemInfo
	);

//��ת���
//B3 =27; B4= 3; ������������
DLL_XMT_SER_API void XMT_COMMAND_XZ_ClearZero(
	unsigned char address,
	unsigned char Command_B3,
	unsigned char Command_B4
	);

/////////////////////////////////////////////////////////////////////////////////

//double tmpX,    // ��λ΢�� ��ƽ̨��λ�
//double tmpY,    // ��λ΢�� ��ƽ̨��λ�
//double tmpZ,    // ��λ΢�� ��ƽ̨��λ�
//double tmpROLL, //��Ծ�ƽ̨��  ��X ������
//double tmpPITCH, //��Ծ�ƽ̨�� ��Y ������
//double tmpYAW,   //��Ծ�ƽ̨��   ��Z ������
//double tmp_XZhuan,//˳ʱ����ת ��λ�� ��
//unsigned char flagOpenFalg1, 'O' ���� 'C' �ջ�
//unsigned char flagOpenFalg2, 'O' ���� 'C' �ջ�
//int address //ʹ�õĵ�ַ
//�����ɶȿ����㷨 20211126 
DLL_XMT_SER_API void PNSiXControl(double tmpX,    // ��λ΢�� ��ƽ̨��λ�
	double tmpY,    // ��λ΢�� ��ƽ̨��λ�
	double tmpZ,    // ��λ΢�� ��ƽ̨��λ�
	double tmpROLL, //��Ծ�ƽ̨��  ��X ������
	double tmpPITCH, //��Ծ�ƽ̨�� ��Y ������
	double tmpYAW,   //��Ծ�ƽ̨��   ��Z ������
	double tmp_XZhuan,//˳ʱ����ת ��λ�� ��
	unsigned char flagOpenFalg1,
	unsigned char flagOpenFalg2,
	int address //ʹ�õĵ�ַ
	);

////ͬʱ����6��ͨ��������
////20220119 ͨ��
//double B1;//ͨ��1 
//double B2;//ͨ��6
//double B3;//ͨ��5
//double B4;//ͨ��4
//double B5;//ͨ��3
//double B6;//ͨ��2
//unsigned char flagOpenFalg1;  //'O' ���� 'C' �ջ�
//unsigned char flagOpenFalg1;  //'O' ���� 'C' �ջ�
DLL_XMT_SER_API void send6zhou_TaoCi(double B1, double B2, double B3, unsigned char flagOpenFalg1, double B4, double B5, double B6, unsigned char flagOpenFalg2, int address_maUse);


//double tmpX;   // ��λ΢�� ��ƽ̨��λ�
//double tmpY;    // ��λ΢�� ��ƽ̨��λ�
//double tmpZ;    // ��λ΢�� ��ƽ̨��λ�
//double tmpROLL; //��Ծ�ƽ̨��  ��X ������
//double tmpPITCH; //��Ծ�ƽ̨�� ��Y ������
//double tmpYAW;   //��Ծ�ƽ̨��   ��Z ������
//double tmp_XZhuan;//˳ʱ����ת ��λ�� ��
//double SixControl[6];//���ص�6���մɵĳ��� ��λ ΢��
DLL_XMT_SER_API void PNSiXControlGetControl(double tmpX,    // ��λ΢�� ��ƽ̨��λ�
	double tmpY,    // ��λ΢�� ��ƽ̨��λ�
	double tmpZ,    // ��λ΢�� ��ƽ̨��λ�
	double tmpROLL, //��Ծ�ƽ̨��  ��X ������
	double tmpPITCH, //��Ծ�ƽ̨�� ��Y ������
	double tmpYAW,   //��Ծ�ƽ̨��   ��Z ������
	double tmp_XZhuan,//˳ʱ����ת ��λ�� ��
	double SixControl[6]
	);

//double XP,//��ƽ̨��Ծ�ƽ̨�ĳ�ʼλ�� Բ�ĵ�λ�� ��ƽ̨����ھ�ƽ̨
//double YP,
//double ZP,
//double X,//��ƽ̨��λ�
//double Y,
//double Z,
//double ROLL, //��Ծ�ƽ̨�� ��X
//double PITCH, //��Ծ�ƽ̨�� ��Y
//double YAW, //��Ծ�ƽ̨�� ��Z
//double R, //��ƽ̨�İ뾶
//double r,//��ƽ̨�İ뾶
//double up_angle0, //��ƽ̨ �µ�ĳ�ʼλ��
//double up_angle1,
//double up_angle2,
//
//double down_angle0,//��ƽ̨ �µ�ĳ�ʼλ��
//double down_angle1,
//double down_angle2
//20221027 P32P25 �ھ�����
DLL_XMT_SER_API void P32Control(double XP,//��ƽ̨��Ծ�ƽ̨�ĳ�ʼλ�� Բ�ĵ�λ�� ��ƽ̨����ھ�ƽ̨
	double YP,
	double ZP,
	double X,//��ƽ̨��λ�
	double Y,
	double Z,
	double ROLL, //��Ծ�ƽ̨�� ��X
	double PITCH, //��Ծ�ƽ̨�� ��Y
	double YAW, //��Ծ�ƽ̨�� ��Z
	double R, //��ƽ̨�İ뾶
	double r,//��ƽ̨�İ뾶
	double up_angle0, //��ƽ̨ �µ�ĳ�ʼλ��
	double up_angle1,
	double up_angle2,

	double down_angle0,//��ƽ̨ �µ�ĳ�ʼλ��
	double down_angle1,
	double down_angle2
	);

