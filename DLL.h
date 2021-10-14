#ifndef DLL_H
#define DLL_H
#include <cmd.h>

typedef	int	(*ptr_Initialize)();

typedef	int	(*ptr_EnumInterface)();

typedef	int	(*ptr_GetInterfaceInfo)(
    IN	int				iInterfaceIndex,	// interface index
    OUT	void			**pInterface		// interface class address location
);

typedef	bool (*ptr_OpenInterface)(
    IN	void			*pInterface			// interface class address
);

typedef	bool (*ptr_CloseInterface)(
    IN	void			*pInterface			// interface class address
);

typedef	bool (*ptr_ConfigInterface)(
    IN	void			*pInterface,		// interface class address
    IN  HWND			hWnd				// window handle for setting interface
);

typedef	bool (*ptr_SendCommand)(
    IN	void			*pInterface,		// interface class address
    IN  MDK_MSL_CMD		*cmd				// command structure.
);

typedef	bool (*ptr_RegisterCallback)(
    IN void				*pInterface,		// interface class address
    IN GT_MDK_CALLBACK	Cb_Complete,		// callback for command completion
    IN GT_MDK_CALLBACK	Cb_Notify,			// callback for notification
    IN GT_MDK_CALLBACK	Cb_Error,			// callback for error notification
    IN void				*pContext			// callback context.
);


#endif // DLL_H
