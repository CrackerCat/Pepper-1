/****************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/                         *
* This is a Hex Control for MFC applications.                                           *
* Official git repository of the project: https://github.com/jovibor/HexCtrl/           *
* This software is available under the "MIT License".  *
* https://github.com/jovibor/HexCtrl/blob/master/LICENSE                                *
* For more information, or any questions, visit the project's official repository.      *
****************************************************************************************/
#pragma once
#include <Windows.h> //Standard Windows header.
#include <memory>    //std::shared/unique_ptr and related.

/**********************************************************************
* If HexCtrl is to be used as a .dll, then include this header,       *
* and uncomment the line below.                                       *
**********************************************************************/
//#define HEXCTRL_SHARED_DLL

/**********************************************************************
* For manually initialize MFC.                                        *
* This macro is used only for Win32 non MFC projects, built with      *
* Use MFC in a Shared DLL option.                                     *
**********************************************************************/
//#define HEXCTRL_MANUAL_MFC_INIT

namespace HEXCTRL
{
	/********************************************************************************************
	* HEXMODIFYSTRUCT - used to represent data modification parameters.                         *
	********************************************************************************************/
	struct HEXMODIFYSTRUCT
	{
		ULONGLONG ullIndex { };       //Index of the starting byte to modify.
		ULONGLONG ullSize { };        //Size to be modified.
		ULONGLONG ullDataSize { };    //Size of pData.
		PBYTE     pData { };          //Pointer to a data to be set.
		bool      fWhole { true };    //Is a whole byte or just a part of it to be modified.
		bool      fHighPart { true }; //Shows whether high or low part of the byte should be modified (if the fWhole flag is false).
		bool      fRepeat { false };  //If ullDataSize < ullSize, should data be replaced only one time or ullSize/ullDataSize times.
	};

	/********************************************************************************************
	* IHexVirtual - Pure abstract data handler class, that can be implemented by client,        *
	* to set its own data handler routines.	Works in EHexDataMode::DATA_VIRTUAL mode.           *
	* Pointer to this class can be set in IHexCtrl::SetData method.                             *
	* Its usage is very similar to DATA_MSG logic, where control sends WM_NOTIFY messages       *
	* to set window to get/set data. But in this case it's just a pointer to a custom           *
	* routine's implementation.                                                                 *
	* All virtual functions must be defined in client's derived class.                          *
	********************************************************************************************/
	class IHexVirtual
	{
	public:
		virtual ~IHexVirtual() = default;
		virtual BYTE GetByte(ULONGLONG ullIndex) = 0;            //Gets the byte data by index.
		virtual	void ModifyData(const HEXMODIFYSTRUCT& hms) = 0; //Routine to modify data, if HEXDATASTRUCT::fMutable == true.
		virtual void Undo() = 0;                                 //Undo command, through menu or hotkey.
		virtual void Redo() = 0;                                 //Redo command, through menu or hotkey.
	};

	/********************************************************************************************
	* HEXCOLORSTRUCT - All HexCtrl colors.                                                      *
	********************************************************************************************/
	struct HEXCOLORSTRUCT
	{
		COLORREF clrTextHex { GetSysColor(COLOR_WINDOWTEXT) };         //Hex chunks text color.
		COLORREF clrTextAscii { GetSysColor(COLOR_WINDOWTEXT) };       //Ascii text color.
		COLORREF clrTextSelected { GetSysColor(COLOR_HIGHLIGHTTEXT) }; //Selected text color.
		COLORREF clrTextCaption { RGB(0, 0, 180) };                    //Caption text color
		COLORREF clrTextInfoRect { GetSysColor(COLOR_WINDOWTEXT) };    //Text color of the bottom "Info" rect.
		COLORREF clrTextCursor { RGB(255, 255, 255) };                 //Cursor text color.
		COLORREF clrBk { GetSysColor(COLOR_WINDOW) };                  //Background color.
		COLORREF clrBkSelected { GetSysColor(COLOR_HIGHLIGHT) };       //Background color of the selected Hex/Ascii.
		COLORREF clrBkInfoRect { GetSysColor(COLOR_BTNFACE) };         //Background color of the bottom "Info" rect.
		COLORREF clrBkCursor { RGB(0, 0, 255) };                       //Cursor background color.
		COLORREF clrBkCursorSelected { RGB(0, 0, 200) };               //Cursor background color in selection.
	};

	/********************************************************************************************
	* HEXCREATESTRUCT - for IHexCtrl::Create method.                                            *
	********************************************************************************************/
	struct HEXCREATESTRUCT
	{
		HEXCOLORSTRUCT  stColor { };           //All the control's colors.
		HWND            hwndParent { };        //Parent window pointer.
		const LOGFONTW* pLogFont { };          //Font to be used, nullptr for default. This font has to be monospaced.
		RECT            rect { };              //Initial rect. If null, the window is screen centered.
		UINT            uID { };               //Control ID.
		DWORD           dwStyle { };           //Window styles, 0 for default.
		DWORD           dwExStyle { };         //Extended window styles, 0 for default.
		bool            fFloat { false };      //Is float or child (incorporated into another window)?
		bool            fCustomCtrl { false }; //It's a custom dialog control.
	};

	/********************************************************************************************
	* EHexDataMode - Enum of the working data mode, used in HEXDATASTRUCT in SetData.           *
	* DATA_MEMORY: Default standard data mode.                                                  *
	* DATA_MSG: Data is handled through WM_NOTIFY messages in handler window.				    *
	* DATA_VIRTUAL: Data is handled through IHexVirtual interface by derived class.             *
	********************************************************************************************/
	enum class EHexDataMode : DWORD
	{
		DATA_MEMORY, DATA_MSG, DATA_VIRTUAL
	};

	/********************************************************************************************
	* HEXDATASTRUCT - for IHexCtrl::SetData method.                                             *
	********************************************************************************************/
	struct HEXDATASTRUCT
	{
		ULONGLONG    ullDataSize { };                       //Size of the data to display, in bytes.
		ULONGLONG    ullSelectionStart { };                 //Set selection at this position. Works only if ullSelectionSize > 0.
		ULONGLONG    ullSelectionSize { };                  //How many bytes to set as selected.
		HWND         hwndMsg { };                           //Window to send the control messages to. Parent window is used by default.
		IHexVirtual* pHexVirtual { };                       //Pointer to IHexVirtual data class for custom data handling.
		PBYTE        pData { };                             //Pointer to the data. Not used if it's virtual control.
		EHexDataMode enMode { EHexDataMode::DATA_MEMORY };  //Working data mode of the control.
		bool         fMutable { false };                    //Will data be mutable (editable) or just read mode.
	};

	/********************************************************************************************
	* HEXNOTIFYSTRUCT - used in notifications routine.                                          *
	********************************************************************************************/
	struct HEXNOTIFYSTRUCT
	{
		NMHDR     hdr { };      //Standard Windows header. For hdr.code values see HEXCTRL_MSG_* messages.
		UINT_PTR  uMenuId { };  //User defined custom menu id.
		ULONGLONG ullIndex { }; //Index of the start byte to get/send.
		ULONGLONG ullSize { };  //Size of the bytes to get/send.
		PBYTE     pData { };    //Pointer to a data to get/send.
	};
	using PHEXNOTIFYSTRUCT = HEXNOTIFYSTRUCT *;

	/********************************************************************************************
	* IHexCtrl - pure abstract base class.                                                      *
	********************************************************************************************/
	class IHexCtrl
	{
	public:
		virtual ~IHexCtrl() = default;
		virtual bool Create(const HEXCREATESTRUCT& hcs) = 0;   //Main initialization method.
		virtual bool CreateDialogCtrl(UINT uCtrlID, HWND hwndDlg) = 0; //Сreates custom dialog control.
		virtual void SetData(const HEXDATASTRUCT& hds) = 0;    //Main method for setting data to display (and edit).	
		virtual void ClearData() = 0;                          //Clears all data from HexCtrl's view (not touching data itself).
		virtual void SetEditMode(bool fEnable) = 0;            //Enable or disable edit mode.
		virtual void ShowOffset(ULONGLONG ullOffset, ULONGLONG ullSize = 1) = 0; //Shows (selects) given offset.
		virtual void SetFont(const LOGFONTW* pLogFontNew) = 0; //Sets the control's new font. This font has to be monospaced.
		virtual void SetFontSize(UINT uiSize) = 0;             //Sets the control's font size.
		virtual void SetColor(const HEXCOLORSTRUCT& clr) = 0;  //Sets all the control's colors.
		virtual void SetCapacity(DWORD dwCapacity) = 0;        //Sets the control's current capacity.
		virtual bool IsCreated()const = 0;                     //Shows whether control is created or not.
		virtual bool IsDataSet()const = 0;                     //Shows whether a data was set to the control or not.
		virtual bool IsMutable()const = 0;                     //Is edit mode enabled or not.
		virtual long GetFontSize()const = 0;                   //Current font size.
		virtual void GetSelection(ULONGLONG& ullOffset, ULONGLONG& ullSize)const = 0; //Current selection.
		virtual HWND GetWindowHandle()const = 0;               //Retrieves control's window handle.
		virtual HMENU GetMenuHandle()const = 0;                //Context menu handle.
		virtual void Destroy() = 0;                            //Deleter.
	};

	/********************************************************************************************
	* Factory function CreateHexCtrl returns IHexCtrlUnPtr - unique_ptr with custom deleter.    *
	* In client code you should use IHexCtrlPtr type which is an alias to either IHexCtrlUnPtr  *
	* - a unique_ptr, or IHexCtrlShPtr - a shared_ptr. Uncomment what serves best for you,      *
	* and comment out the other.                                                                *
	* If you, for some reason, need raw pointer, you can directly call CreateRawHexCtrl         *
	* function, which returns IHexCtrl interface pointer, but in this case you will need to     *
	* call IHexCtrl::Destroy method	afterwards - to manually delete HexCtrl object.             *
	********************************************************************************************/
#ifdef HEXCTRL_SHARED_DLL
#ifdef HEXCTRL_EXPORT
#define HEXCTRLAPI extern "C" IHexCtrl __declspec(dllexport)* __cdecl
#else
#define HEXCTRLAPI extern "C" IHexCtrl __declspec(dllimport)* __cdecl

#ifdef _WIN64
#ifdef _DEBUG
#define LIBNAME_PROPER(x) x"64d.lib"
#else
#define LIBNAME_PROPER(x) x"64.lib"
#endif
#else
#ifdef _DEBUG
#define LIBNAME_PROPER(x) x"d.lib"
#else
#define LIBNAME_PROPER(x) x".lib"
#endif
#endif

#pragma comment(lib, LIBNAME_PROPER("HexCtrl"))
#endif
#else
#define	HEXCTRLAPI IHexCtrl*
#endif

	HEXCTRLAPI CreateRawHexCtrl();
	using IHexCtrlUnPtr = std::unique_ptr<IHexCtrl, void(*)(IHexCtrl*)>;
	using IHexCtrlShPtr = std::shared_ptr<IHexCtrl>;

	inline IHexCtrlUnPtr CreateHexCtrl()
	{
		return IHexCtrlUnPtr(CreateRawHexCtrl(), [](IHexCtrl * p) { p->Destroy(); });
	};

	//using IHexCtrlPtr = IHexCtrlUnPtr;
	using IHexCtrlPtr = IHexCtrlShPtr;

	/********************************************************************************************
	* WM_NOTIFY message codes (NMHDR.code values).                                              *
	* These codes are used to notify m_hwndMsg window about control's states.                   *
	********************************************************************************************/

	constexpr auto HEXCTRL_MSG_DESTROY { 0xFFFF };       //Indicates that HexCtrl is being destroyed.
	constexpr auto HEXCTRL_MSG_GETDATA { 0x0100 };       //Used in Virtual mode to demand the next byte to display.
	constexpr auto HEXCTRL_MSG_MODIFYDATA { 0x0101 };    //Indicates that the data in memory has changed, used in Edit mode.
	constexpr auto HEXCTRL_MSG_SETSELECTION { 0x0102 };  //Selection has been made.
	constexpr auto HEXCTRL_MSG_MENUCLICK { 0x0103 };     //User defined custom menu clicked.
	constexpr auto HEXCTRL_MSG_ONCONTEXTMENU { 0x0104 }; //OnContextMenu triggered.
	constexpr auto HEXCTRL_MSG_UNDO { 0x0105 };          //Undo command triggered.
	constexpr auto HEXCTRL_MSG_REDO { 0x0106 };          //Redo command triggered.


/*******************Setting a manifest for ComCtl32.dll version 6.***********************/
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
}