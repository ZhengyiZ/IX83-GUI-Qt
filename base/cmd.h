#ifndef CMD_H
#define CMD_H

#include <Windows.h>
#define	SZ_SMALL					8			//
#define	MAX_TAG_SIZE				32			// command tag size
#define	MAX_COMMAND_SIZE			256			// command buffer size
#define	MAX_RESPONSE_SIZE			256			// response buffer size

//-----------------------------------------------------------------------------
//	in/out
#define		IN
#define		OUT

#define CALLBACK    __stdcall
//-----------------------------------------------------------------------------
//	unit list
typedef	enum tag_eDeviceType {
    e_DTUnknown				= 0,			// unknown unit

    // motor unit
    e_DTNosepiece			= 20000,		// nose piece
    e_DTFilterWheel			= 20002,		// filter wheel
    e_DTLamp				= 20003,		// lamp
    e_DTApertureStop		= 20004,		// AS
    e_DTShutter				= 20005,		// shutter
    e_DTMagnificationDevice = 20006,
    e_DTMagnificationChanger= 20007,		//
    e_DTTopLens				= 20008,		// top lens
    e_DTLightPathSwitch		= 20009,		// light path switcher
    e_DTZoom				= 20010,		// zoom

    e_DTStageInsert			= 20050,

    e_DTSlideLoader			= 30000,

    e_DTManualControl		= 40000,

    e_DTFocus				= 25000,		// focus
    e_DTCondenser			= 25001,		// condenser
    e_DTMirror				= 25002,		// mirror
    e_DTHandSwitch			= 25003,		// hand switch
    e_DTAberLens			= 25004,		// aber lens
    e_DTFieldStop			= 25005,		// FS
    e_DTCorrectionRing		= 25007,		// correction ring

    e_DTStage				= 20051,		// stage
    e_DTSample				= 20052,		// sample, attached to stage ??

    e_DTMicroscope			= 60000,		// microscope system
    e_DTTiny				= 61000			// tiny board

}	eDeviceType;

//	command status
typedef	enum tag_eMDK_CmdStatus {
    e_Cmd_Init = 0,						// initial state
    e_Cmd_Queue,						// queued
    e_Cmd_Wait,							// waiting for response
    e_Cmd_Complete,						// completion
    e_Cmd_Timeout,						// timeout
    e_Cmd_Abort							// aborted
} eMDK_CmdStatus;

//-----------------------------------------------------------------------------
//	command block
typedef	struct	tag_MDK_MSL_CMD	{
    DWORD			m_Signature;		// command block signature

    //-------------------------------------------
    // basic fields
    eDeviceType		m_Type;				// unit type
    WORD			m_Sequence;			// command sequence
    char			m_From[SZ_SMALL];	// from
    char			m_To[SZ_SMALL];		// to
    eMDK_CmdStatus	m_Status;			// command statys
    DWORD			m_Result;			// result
    BOOL			m_Sync;				// sync or async?
    BOOL			m_Command;			// command or query? (refer command tag)
    BOOL			m_SendOnly;			// TRUE means NOT wait response.

    //-------------------------------------------
    // management field
    time_t			m_StartTime;		// start time
    time_t			m_FinishTime;		// end time
    DWORD			m_Timeout;			// time out (ms)

    void			*m_Callback;		// callback entry
    void			*m_Event;			// event info
    void			*m_Context;			// context
    UINT			m_TimerID;			// timer ID
    void			*m_PortContext;		// port info.

    //-------------------------------------------
    //	extend field (for indivisual command)
    ULONG			m_Ext1;				// extend info 1
    ULONG			m_Ext2;				// extend info 2
    ULONG			m_Ext3;				// extend info 3
    LONGLONG		m_lExt1;			// LONGLONG extend info 1
    LONGLONG		m_lExt2;			// LONGLONG extend info 2

    //-------------------------------------------
    // tag fields
    DWORD			m_TagSize;			// tag size
    BYTE			m_CmdTag[MAX_TAG_SIZE];

    //-------------------------------------------
    // command string
    DWORD			m_CmdSize;			// size of command string
    BYTE	    	m_Cmd[MAX_COMMAND_SIZE];

    //-------------------------------------------
    // response string
    DWORD			m_RspSize;			// size of response string
    BYTE			m_Rsp[MAX_RESPONSE_SIZE];

} MDK_MSL_CMD, *PMDK_MSL_CMD;

typedef	int	(__stdcall *GT_CALLBACK_ENTRY)(
        ULONG		MsgId,			// Callback ID.
        ULONG		wParam,			// Callback parameter, it depends on callback event.
        ULONG		lParam,			// Callback parameter, it depends on callback event.
        PVOID		pv,				// Callback parameter, it depends on callback event.
        PVOID		pContext,		// This contains information on this call back function.
        PVOID		pCaller			// This is the pointer specified by a user in the requirement function.
);

typedef	GT_CALLBACK_ENTRY	GT_MDK_CALLBACK;

#endif // CMD_H
