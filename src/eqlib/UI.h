/*
 * MacroQuest: The extension platform for EverQuest
 * Copyright (C) 2002-present MacroQuest Authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

#include "ForwardDecls.h"
#include "ChatFilters.h"
#include "Common.h"
#include "Constants.h"
#include "Containers.h"
#include "CXStr.h"
#include "CXWnd.h"
#include "GameFace.h"
#include "GraphicsResources.h"
#include "Items.h"
#include "XMLData.h"
#include "UITemplates.h"
#include "EQData.h"

#include "base/Iterator.h"

#include <list>
#include <functional>

#undef FindWindow
#undef InsertMenuItem
#undef LoadMenu

struct IShellFolder;

namespace eqlib {

//----------------------------------------------------------------------------
// Forward Declarations

class CAtlas;
class CButtonWnd;
class CChatContainerWindow;
class CChatWindow;
class CContextMenu;
class CConfirmationDialog;
class CEditWnd;
class CParamLayoutStrategy;
class CLabel;
class CLayoutStrategyTemplate;
class CStmlWnd;
class CUnSerializeBuffer;
class CVerticalLayoutWnd;
class CVivoxObserver;

using D3DCOLOR = DWORD;
using CPopDialogWnd = CConfirmationDialog;

//----------------------------------------------------------------------------

class WndEventHandler
{
public:
	uint32_t lastUpdate;
};

//----------------------------------------------------------------------------

class CVivoxManager;
class CVivoxObserver
{
public:
	virtual void Unknown00(bool) {}

	CVivoxManager* VivoxManager;
};

//----------------------------------------------------------------------------

class CObservable;

class CNotification
{
public:
	int Type;
};

class IObserver
{
public:
	EQLIB_OBJECT virtual void Notify(CObservable* Src, const CNotification* const Notification) {}
};

class CObservable
{
public:
	EQLIB_OBJECT void NotifyObservers(CNotification* notification = 0);

private:
	// this will never work because of differences in stl between
	// mq2 and eq. Don't use it.
	std::list<IObserver*> ObserverList;
};

//============================================================================

struct [[offsetcomments]] PlayerBuffInfo
{
/*0x00*/ CButtonWnd*        BuffBtn;
/*0x08*/ CTextureAnimation* BuffIcon;
/*0x10*/ int                SpellID;    // Spell ID# of each buff
/*0x14*/ int                BuffTimer;  // milliseconds before buff will expire
/*0x18*/ SoeUtil::StringFixed<EQ_MAX_NAME> Caster;
/*0x78*/
};

struct [[offsetcomments]] BuffWindowPlayerBuffInfo : PlayerBuffInfo
{
/*0x78*/ CTextObjectInterface* TimeRemainingText;
/*0x80*/ CTextObjectInterface* CounterText;
/*0x88*/ CTextObjectInterface* LimitUseText;
/*0x90*/
};

// Convenience wrapper that smooths over the difference in buff interfaces between
// clients. Allows MQ code to share the same between test/live/emu/etc
class PlayerBuffInfoWrapper
{
protected:
	const PlayerBuffInfo* m_buffInfo;
	int m_index;

public:
	using BuffInfoType = PlayerBuffInfo;

	PlayerBuffInfoWrapper(int index, const PlayerBuffInfo* buffInfo) noexcept
		: m_index(index), m_buffInfo(buffInfo) {}

	PlayerBuffInfoWrapper(const PlayerBuffInfoWrapper&) = delete;
	PlayerBuffInfoWrapper& operator=(const PlayerBuffInfoWrapper&) = delete;

	PlayerBuffInfoWrapper(PlayerBuffInfoWrapper&& rhs) noexcept : m_index(rhs.m_index), m_buffInfo(rhs.m_buffInfo) {}
	PlayerBuffInfoWrapper& operator=(PlayerBuffInfoWrapper&& rhs) noexcept { m_index = rhs.m_index; m_buffInfo = rhs.m_buffInfo; }

	explicit operator bool() const { return m_buffInfo != nullptr; }

	int GetIndex() const { return m_index; }
	CButtonWnd* GetBuffButton() const { return m_buffInfo ? m_buffInfo->BuffBtn : nullptr; }
	CTextureAnimation* GetBuffIcon() const { return m_buffInfo ? m_buffInfo->BuffIcon : nullptr; }
	int GetSpellID() const { return m_buffInfo ? m_buffInfo->SpellID : 0; }
	int GetBuffTimer() const { return m_buffInfo ? m_buffInfo->BuffTimer : 0; }
	const char* GetCaster() const { return (m_buffInfo && m_buffInfo->SpellID > 0) ? m_buffInfo->Caster.c_str() : ""; }

	EQLIB_OBJECT EQ_Spell* GetSpell() const;

	template <typename T = PlayerBuffInfoWrapper>
	struct Iterator
	{
	public:
		using BuffInfoWrapperType = T;
		using BuffInfoType = typename T::BuffInfoType;
		using ContainerType = SoeUtil::Array<BuffInfoType>;

		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = BuffInfoWrapperType;

		Iterator(const ContainerType& container, int index)
			: m_container(container), m_index(index)
		{
			// Increment to first buff
			if (m_index < m_container.GetSize() && m_container[m_index].SpellID <= 0)
			{
				++(*this);
			}
		}

		value_type operator*() const { return BuffInfoWrapperType(m_index, &m_container[m_index]); }

		// Prefix increment - increment to the next valid buff
		Iterator operator++()
		{
			if (m_index < m_container.GetSize())
			{
				do {
					++m_index;
				} while (m_index < m_container.GetSize() && m_container[m_index].SpellID <= 0);
			}
			return *this;
		}
		
		// Postfix increment
		Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

		friend bool operator==(const Iterator& a, const Iterator& b) { return &a.m_container == &b.m_container && a.m_index == b.m_index; }
		friend bool operator!=(const Iterator& a, const Iterator& b) { return !(a == b); }

	private:
		const ContainerType& m_container;
		int m_index;
	};
};

class BuffWindowPlayerBuffInfoWrapper : public PlayerBuffInfoWrapper
{
	const BuffWindowPlayerBuffInfo* GetBuffInfo() const { return static_cast<const BuffWindowPlayerBuffInfo*>(m_buffInfo); }

public:
	using PlayerBuffInfoWrapper::PlayerBuffInfoWrapper;
	using BuffInfoType = BuffWindowPlayerBuffInfo;

	CTextObjectInterface* GetTimeRemainingText() const { return m_buffInfo ? GetBuffInfo()->TimeRemainingText : nullptr; }
	CTextObjectInterface* GetCounterText() const { return m_buffInfo ? GetBuffInfo()->CounterText : nullptr; }
	CTextObjectInterface* GetLimitUseText() const { return m_buffInfo ? GetBuffInfo()->LimitUseText : nullptr; }
};

//============================================================================
// CTextureFont
//============================================================================

class [[offsetcomments]] CTextureFont
{
public:
	virtual ~CTextureFont();

	EQLIB_OBJECT CXStr GetName() const;
	EQLIB_OBJECT int DrawWrappedText(const CXStr&, const CXRect&, const CXRect&, COLORREF, uint16_t, int) const;
	EQLIB_OBJECT int DrawWrappedText(const CXStr& Str, int x, int y, int Width, const CXRect& BoundRect, COLORREF Color, uint16_t Flags = 0, int StartX = 0) const;
	EQLIB_OBJECT int DrawWrappedText(CTextObjectInterface* Interface, const CXStr& Str, const CXRect& Rect, const CXRect& BoundRect, COLORREF Color, uint16_t Flags = 0, int StartX = 0) const;
	EQLIB_OBJECT int GetHeight() const;
	EQLIB_OBJECT int GetKerning(unsigned short, unsigned short) const;
	EQLIB_OBJECT int GetTextExtent(const CXStr&);
	EQLIB_OBJECT int GetWidth(unsigned short) const;

/*0x08*/ int FontStyle;
/*0x0c*/
};

//============================================================================
// CEQSuiteTextureLoader
//============================================================================

struct [[offsetcomments]] _SuiteTexture
{
/*0x00*/ bool               bUsed;
/*0x08*/ CXStr              Name;
/*0x10*/ enDir              Directory;
/*0x18*/ BMI*               pBMInfo;
/*0x20*/
};

struct SWadFile;

class [[offsetcomments]] CEQSuiteTextureLoader
{
public:
/*0x00*/ SWadFile*          pWadFile;
/*0x08*/ ArrayClass<_SuiteTexture> Textures;
/*0x20*/ CXStr              UIPath[cUIDirectoryCount];
/*0x40*/ CXStr              DefaultUIPath[cUIDirectoryCount];
/*0x60*/

	EQLIB_OBJECT CEQSuiteTextureLoader();
	EQLIB_OBJECT ~CEQSuiteTextureLoader();

	EQLIB_OBJECT BMI* GetTexture(const CUITextureInfo& ti);

	EQLIB_OBJECT unsigned int CreateTexture(const CUITextureInfo& ti);
	EQLIB_OBJECT void DestroyTexture(const CUITextureInfo& ti);
	EQLIB_OBJECT void UnloadAllTextures();

	EQLIB_OBJECT const CXStr& GetDefaultUIPath(int DirType) const;
};

//============================================================================
// CRadioGroup
//============================================================================

class [[offsetcomments]] CRadioGroup
{
public:
	EQLIB_OBJECT CRadioGroup(CXStr name = {});
	EQLIB_OBJECT virtual ~CRadioGroup();

	CButtonWnd* GetButton(int index) const
	{
		return index >= 0 && index < Buttons.GetLength() ? Buttons[index].get() : nullptr;
	}

/*0x08*/ CXStr                    Name;
/*0x10*/ ArrayClass<eqstd::shared_ptr<CButtonWnd>> Buttons;
/*0x28*/ int                      CurSel = -1;
/*0x2c*/ bool                     bAllowMultiSelect = false;
/*0x30*/ int                      nSelectionLimit = -1;
/*0x34*/ bool                     bAllowNullable = false;
/*0x38*/
};

//============================================================================
// CButtonWnd
//============================================================================

// @sizeof(CButtonWnd) == 0x348 :: 2024-07-22 (live) @ 0x1405b8390
constexpr size_t CButtonWnd_size = 0x348;

class [[offsetcomments]] CButtonWnd : public CXWnd
{
public:
	//----------------------------------------------------------------------------
	// constructor / destructor
	EQLIB_OBJECT CButtonWnd(CXWnd* parent, uint32_t id, const CXRect& rect,
		const CXPoint& decalOffset, const CXSize& decalSize,
		CTextureAnimation* normal, CTextureAnimation* pressed,
		CTextureAnimation* hover, CTextureAnimation* disabled,
		CTextureAnimation* pressedHover, CTextureAnimation* pressedDisabled,
		CTextureAnimation* normalDecal, CTextureAnimation* pressedDecal,
		CTextureAnimation* hoverDecal, CTextureAnimation* pressedDisabledDecal);
	//EQLIB_OBJECT CButtonWnd(CXWnd* parent, uint32_t id, const CXRect& rect);
	//EQLIB_OBJECT CBUttonWnd(CXWnd* parent, uint32_t id, const CXRect& rect, const CXStr& label);
	EQLIB_OBJECT virtual ~CButtonWnd();

	//----------------------------------------------------------------------------
	// virtual functions
	EQLIB_OBJECT virtual bool IsChecked() const;
	EQLIB_OBJECT virtual void SetRadioGroup(CRadioGroup* group);
	EQLIB_OBJECT virtual int DrawWndText(const CXRect& rect, const CXRect& clip);
	EQLIB_OBJECT virtual int DrawCooldown();
	EQLIB_OBJECT virtual void SetCheck(bool check, bool bNoSound = true);
	EQLIB_OBJECT virtual void SetCoolDownCompletionTimeDelta(uint32_t delta, uint32_t total);
	EQLIB_OBJECT virtual void SetCoolDownCompletionTime(uint32_t time, uint32_t total);
	EQLIB_OBJECT virtual void SetCoolDownCompletionTime(eqtime_t time, uint32_t total);
	EQLIB_OBJECT virtual void SetCoolDownBeginTime(uint32_t time, uint32_t total);
	EQLIB_OBJECT virtual void SetCoolDownUpdatedBeginTime(uint32_t time, uint32_t total);
	EQLIB_OBJECT virtual uint32_t GetCoolDownBeginTime() const;
	EQLIB_OBJECT virtual uint32_t GetCoolDownTotalDuration() const;
	EQLIB_OBJECT virtual uint32_t GetCoolDownTimeRemaining() const;
	EQLIB_OBJECT virtual void ClearCoolDownCompletionTime();

	//----------------------------------------------------------------------------
	// inherited virtuals
	EQLIB_OBJECT virtual int Draw() override;
	EQLIB_OBJECT virtual int DrawTooltipAtPoint(const CXPoint& pos, const CXStr& tooltip) const override;
	EQLIB_OBJECT virtual int HandleLButtonDown(const CXPoint&, uint32_t) override;
	EQLIB_OBJECT virtual int HandleLButtonUp(const CXPoint&, uint32_t) override;
	EQLIB_OBJECT virtual int HandleLButtonHeld(const CXPoint&, uint32_t) override;
	EQLIB_OBJECT virtual int HandleLButtonUpAfterHeld(const CXPoint&, uint32_t) override;
	EQLIB_OBJECT virtual int HandleRButtonDown(const CXPoint&, uint32_t) override;
	EQLIB_OBJECT virtual int HandleRButtonUp(const CXPoint&, uint32_t) override;
	EQLIB_OBJECT virtual int HandleRButtonHeld(const CXPoint&, uint32_t) override;
	EQLIB_OBJECT virtual int HandleRButtonUpAfterHeld(const CXPoint&, uint32_t) override;
	EQLIB_OBJECT virtual int HandleMouseMove(const CXPoint&, uint32_t) override;
	EQLIB_OBJECT virtual int OnProcessFrame() override;
	EQLIB_OBJECT virtual bool IsPointTransparent(const CXPoint& point) const override;
	EQLIB_OBJECT virtual void SetAttributesFromSidl(CParamScreenPiece*) override;

	//----------------------------------------------------------------------------
	void SetDecalTint(COLORREF cr) { DecalTint = cr; }
	void SetDecalTint(mq::MQColor color) { DecalTint = color.ToARGB(); }

	//----------------------------------------------------------------------------
	// data members
/*0x268*/ int                   MouseButtonState;
/*0x26c*/ bool                  bPicture;
/*0x270*/ CRadioGroup*          pGroup;
/*0x278*/ bool                  Checked;
/*0x279*/ bool                  bMouseOverLastFrame;
/*0x27c*/ CXPoint               DecalOffset;
/*0x284*/ CXSize                DecalSize;
/*0x28c*/ COLORREF              DecalTint;                // Color
/*0x290*/ CXRect                TextOffsets;
/*0x2a0*/ int                   TextModeBits;
/*0x2a4*/ COLORREF              Mouseover;
/*0x2a8*/ COLORREF              Pressed;
/*0x2ac*/ COLORREF              Disabled;
/*0x2b0*/ unsigned int          CoolDownBeginTime;
/*0x2b4*/ unsigned int          CoolDownDuration;
/*0x2b8*/ CXStr                 Indicator;
/*0x2c0*/ unsigned int          IndicatorVal;
/*0x2c8*/ CTextObjectInterface* pIndicatorTextObject;
/*0x2d0*/ unsigned int          Unknown0x248;
/*0x2d8*/ CButtonDrawTemplate   DrawTemplate;
/*0x340*/ bool                  bAllowButtonClickThrough;
/*0x341*/ bool                  bCoolDownDoDelayedStart;
/*0x342*/ bool                  bIsCheckbox;
/*0x343*/ bool                  bIsDrawLasso;
/*0x344*/ uint32_t              ButtonStyle;              // tbd
/*0x348*/

	ALT_MEMBER_ALIAS(bool, Checked, bChecked);

	struct [[offsetcomments]] VirtualFunctionTable : public CXWnd::VirtualFunctionTable
	{
	/*0x348*/ void* IsChecked;
	/*0x350*/ void* SetRadioGroup;
	/*0x358*/ void* DrawWndText;
	/*0x360*/ void* DrawCooldown;
	/*0x368*/ void* DrawCooldownRect;
	/*0x370*/ void* SetCheck;
	/*0x378*/ void* SetCoolDownCompletionTimeDelta;
	/*0x380*/ void* SetCoolDownCompletionTime;
	/*0x388*/ void* SetCoolDownCompletionTime2;
	/*0x390*/ void* SetCoolDownBeginTime;
	/*0x398*/ void* SetCoolDownUpdatedBeginTime;
	/*0x3a0*/ void* GetCoolDownBeginTime;
	/*0x3a8*/ void* GetCoolDownTotalDuration;
	/*0x3b0*/ void* GetCoolDownTimeRemaining;
	/*0x3b8*/ void* ClearCoolDownCompletionTime;
	/*0x3c0*/
	};

	// points to the eq instance of the virtual function table for this class
	EQLIB_OBJECT static VirtualFunctionTable* sm_vftable;
};

SIZE_CHECK(CButtonWnd, CButtonWnd_size);

inline namespace deprecated {
	using CBUTTONWND DEPRECATE("Use CButtonWnd instead of CBUTTONWND") = CButtonWnd;
	using PCBUTTONWND DEPRECATE("Use CButtonWnd* instead of PCBUTTONWND") = CButtonWnd*;
}

//============================================================================
// CCheckBoxWnd
//============================================================================

class [[offsetcomments]] CCheckBoxWnd : public CButtonWnd
{
public:
	CCheckBoxWnd(CXWnd*, uint32_t, const CXRect&, const CXPoint&, const CXSize&,
		CTextureAnimation*, CTextureAnimation*, CTextureAnimation*, CTextureAnimation*,
		CTextureAnimation*, CTextureAnimation*, CTextureAnimation*, CTextureAnimation*,
		CTextureAnimation*, CTextureAnimation*);
	virtual ~CCheckBoxWnd();

	// virtual
	int HandleLButtonDown(const CXPoint&, uint32_t) override;
	int HandleLButtonUp(const CXPoint&, uint32_t) override;
	int HandleMouseMove(const CXPoint&, uint32_t) override;
	void SetRadioGroup(CRadioGroup*) override;

	EQLIB_OBJECT void SetRadioLook();

	// protected
/*0x348*/ bool bOrgState;
/*0x34c*/
};

//============================================================================
// CComboWnd
//============================================================================

class [[offsetcomments]] CComboWnd : public CXWnd
{
public:
	CComboWnd(CXWnd*, uint32_t, const CXRect&, int height, const CButtonDrawTemplate&);
	virtual ~CComboWnd();

	//----------------------------------------------------------------------------
	// virtuals

	virtual CXSize GetMinSize() const;

	// overrides

	virtual int Draw() override;
	virtual int HandleLButtonDown(const CXPoint&, uint32_t) override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;
	virtual int OnMove(const CXRect&) override;
	virtual int OnResize(int, int) override;
	virtual int HitTest(const CXPoint&, int*) const override;
	virtual void SetDrawTemplate(CXWndDrawTemplate*) override;

	//----------------------------------------------------------------------------
	// methods

	EQLIB_OBJECT CXRect GetListRect(bool) const;
	EQLIB_OBJECT void SetColors(COLORREF norm, COLORREF highlight, COLORREF selected);
	void SetColors(mq::MQColor norm, mq::MQColor highlight, mq::MQColor selected) { SetColors(norm.ToARGB(), highlight.ToARGB(), selected.ToARGB()); }

	EQLIB_OBJECT int InsertChoice(const CXStr& text, uint32_t data = 0);
	EQLIB_OBJECT void SetChoice(int index);
	EQLIB_OBJECT int GetItemCount();
	EQLIB_OBJECT void DeleteAll();

	EQLIB_OBJECT CXRect GetTextRect() const;
	EQLIB_OBJECT CXRect GetButtonRect() const;

	EQLIB_OBJECT int GetCurChoice() const;
	EQLIB_OBJECT CXStr GetCurChoiceText() const;

	//----------------------------------------------------------------------------
	// data members
/*0x268*/ CListWnd*                    pListWnd;
/*0x270*/ int                          ListHeightMax;
/*0x274*/ int                          ListHeight;
/*0x278*/ CButtonDrawTemplate          ButtonDrawTemplate;
/*0x2e0*/
};

//============================================================================
// CEditWnd
//============================================================================

enum eTextAlign
{
	eta_Left,
	eta_Center,
	eta_Right,
};

constexpr int EDITWND_MAX_TAGS = 10;

enum EditWndStyle
{
	ewsMultiline    = 0x00010000,
	ewsPassword     = 0x00020000,
	ewsWantReturn   = 0x00040000,
	ewsWantArrows   = 0x00080000,
	ewsWantTabs     = 0x00100000,
	ewsReadOnly     = 0x00200000,
};

class [[offsetcomments]] CEditBaseWnd : public CXWnd
{
public:
	CEditBaseWnd(CXWnd* parent, uint32_t id, const CXRect& rect);
	virtual ~CEditBaseWnd();

	//----------------------------------------------------------------------------
	// virtuals

	virtual int GetHorzOffset() = 0;
	virtual CXStr GetDisplayString() const = 0;
	virtual CXPoint GetCaretPt() const = 0;
	virtual CXPoint PointFromPrintableChar(int charIndex) const = 0;
	virtual int ResetWnd() = 0;

	// overrides
	virtual int Draw() override = 0;
	virtual int DrawCaret() const override = 0;
	virtual int HandleKeyboardMsg(uint32_t message, uint32_t flags, bool down) override = 0;
	virtual int HandleLButtonDown(const CXPoint& pos, uint32_t flags) override = 0;
	virtual int OnKillFocus(CXWnd*) override;

	EQLIB_OBJECT void SetMaxChars(int);
	EQLIB_OBJECT void SetSel(int, int);

	//----------------------------------------------------------------------------
	// data members
/*0x268*/ eTextAlign   eAlign = eta_Left;
/*0x26c*/ int          StartPos = 0;
/*0x270*/ int          EndPos = 0;
/*0x274*/ int          MaxChars = -1;
/*0x278*/ int          MaxBytesUTF8 = -1;
/*0x280*/ CXStr        InputText;
/*0x288*/ int          TagPrintableStarts[EDITWND_MAX_TAGS];
/*0x2b0*/ int          TagPrintableEnds[EDITWND_MAX_TAGS];
/*0x2d8*/ int          TagOriginalStarts[EDITWND_MAX_TAGS];
/*0x300*/ int          TagOriginalEnds[EDITWND_MAX_TAGS];
/*0x328*/ int          TagDynamicSize[EDITWND_MAX_TAGS];
/*0x350*/ int          TagCodes[EDITWND_MAX_TAGS];
/*0x378*/ CXStr        TagStrings[EDITWND_MAX_TAGS];
/*0x3c8*/ int          TagCount;
/*0x3cc*/ uint32_t     EditStyle;
/*0x3d0*/
};

enum EditWndMode
{
	ewmNormal,
	ewmName,
	ewmAlphaOnly,
	ewmNumericOnly,
	ewmAlphaNumOnly,
	ewmCount
};

class [[offsetcomments]] CEditWnd : public CEditBaseWnd
{
public:
	CEditWnd(CXWnd* parent, uint32_t id, const CXRect& rect, uint32_t style = 0);
	virtual ~CEditWnd();

	//----------------------------------------------------------------------------
	// virtuals

	virtual int DrawMultiline();
	virtual CXStr GetWindowText() const;
	virtual CXStr GetWindowPrefixText();
	virtual CXStr GetWindowSuffixText();

	// CEditBaseWnd
	virtual int GetHorzOffset() override;
	virtual CXStr GetDisplayString() const override;
	virtual CXPoint GetCaretPt() const override;
	virtual CXPoint PointFromPrintableChar(int charIndex) const override;
	virtual int ResetWnd() override;

	// CXWnd
	virtual int Draw() override;
	virtual int DrawCaret() const override;
	virtual int HandleLButtonDown(const CXPoint&, uint32_t) override;
	virtual int HandleLButtonUp(const CXPoint&, uint32_t) override;
	virtual int HandleMouseMove(const CXPoint&, uint32_t) override;
	virtual void SetWindowText(const CXStr& text) override;
	virtual int OnMove(const CXRect& rect) override;
	virtual int OnResize(int, int) override;
	virtual int OnSetFocus(CXWnd*) override;
	virtual int OnKillFocus(CXWnd*) override;

	//----------------------------------------------------------------------------
	// methods

	EQLIB_OBJECT CXPoint GetCharIndexPt(int) const;
	EQLIB_OBJECT CXPoint GetSelEndPt() const;
	EQLIB_OBJECT CXPoint GetSelStartPt() const;
	EQLIB_OBJECT CXStr GetSTMLSafeText();
	EQLIB_OBJECT int ConvertIndexPrintableToTagged(int);
	EQLIB_OBJECT int ConvertIndexTaggedToPrintable(int);
	EQLIB_OBJECT int GetLineForPrintableChar(int) const;
	EQLIB_OBJECT int GetLineLength(int) const;
	EQLIB_OBJECT int SelectableCharFromPoint(CXPoint) const;
	EQLIB_OBJECT void AddItemTag(int, char*, int);
	EQLIB_OBJECT void CalculateScrollRange();
	EQLIB_OBJECT void EnsureCaretVisible();
	EQLIB_OBJECT void SetEditable(bool);

	EQLIB_OBJECT void FillIndexArray(CXStr) const;
	EQLIB_OBJECT void FilterInputStr(CXStr&);
	EQLIB_OBJECT void ProcessText();

	EQLIB_OBJECT bool ReplaceSelection(CXStr, bool bFilter = true);
	inline bool ReplaceSelection(char ch, bool bFilter = true)
	{
		CXStr str(1, ch);
		return ReplaceSelection(str, bFilter);
	}

	//----------------------------------------------------------------------------
	// data members

/*0x3d0*/ bool         bAnchorAtStart;
/*0x3d1*/ bool         bCaretAtEnd;
/*0x3d2*/ bool         bAutoVScrollCalc;
/*0x3d3*/ bool         bEditable;
/*0x3d8*/ CXStr        FilterChars;
/*0x3e0*/ int          EditMode;
/*0x3e4*/ wchar_t      PasswordChar;
/*0x3e8*/ ArrayClass2<uint32_t> LineIndices;
/*0x408*/
};

//============================================================================
// CEditLabelWnd
//============================================================================

class CEditLabelWnd : public CSidlScreenWnd, public WndEventHandler
{
public:
	CEditLabelWnd(CXWnd*);
	virtual ~CEditLabelWnd();

	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT CXStr GetLabelText();
};

//============================================================================
// CGaugeWnd
//============================================================================

class [[offsetcomments]] CGaugeWnd : public CXWnd
{
public:
	EQLIB_OBJECT CGaugeWnd(CXWnd*, uint32_t, const CXRect&, CTextureAnimation*, CTextureAnimation*, CTextureAnimation*, CTextureAnimation*, CTextureAnimation*, CTextureAnimation*, int, unsigned long, unsigned long, bool, int, int, int, int);
	EQLIB_OBJECT void SpecialToolTip();

	EQLIB_OBJECT CXRect CalcFillRect(CXRect rect, int value) const;
	EQLIB_OBJECT CXRect CalcLinesFillRect(CXRect rect, int value) const;

	// virtual
	EQLIB_OBJECT ~CGaugeWnd();
	EQLIB_OBJECT int Draw() const;
	EQLIB_OBJECT int HandleLButtonUp(const CXPoint&, uint32_t);
	EQLIB_OBJECT int OnProcessFrame();
	EQLIB_OBJECT void SetAttributesFromSidl(CParamScreenPiece*);

	//----------------------------------------------------------------------------
	// data members

/*0x268*/ int          EQType;
/*0x26c*/ D3DCOLOR     FillTint;
/*0x270*/ D3DCOLOR     LinesFillTint;
/*0x274*/ bool         bDrawLinesFill;
/*0x278*/ int          TextOffsetX;
/*0x27c*/ int          TextOffsetY;
/*0x280*/ int          GaugeOffsetX;
/*0x284*/ int          GaugeOffsetY;
/*0x288*/ float        LastFrameVal;
/*0x290*/ CXStr        LastFrameName;
/*0x298*/ int          LastFrameTime;
/*0x29c*/ int          LastFrameTarget;
/*0x2a0*/ CXStr        GaugeTooltip;
/*0x2a8*/ int          TooltipVal;
/*0x2ac*/ int          Unknown0x228;
/*0x2b0*/ CGaugeDrawTemplate DrawTemplate;
/*0x2e8*/ CTextObjectInterface* pTextObject;
/*0x2f0*/ CXStr        NextDrawStr;
/*0x2f8*/ bool         bSmooth;
/*0x2fc*/ int          TargetVal;
/*0x300*/ bool         bUseTargetVal;
/*0x304*/
};

//============================================================================
// CHotButton
//============================================================================

enum HotButtonTypes
{
	HotButtonType_None = 0,
	HotButtonType_WeaponSlot,
	HotButtonType_CombatSkill,   // deprecated: no longer used
	HotButtonType_Ability,       // deprecated: no longer used
	HotButtonType_Social,
	HotButtonType_InventorySlot,
	HotButtonType_MenuButton,
	HotButtonType_SpellGem,
	HotButtonType_PetCommand,
	HotButtonType_Skill,
	HotButtonType_MeleeAbility,
	HotButtonType_LeadershipAbility,
	HotButtonType_ItemLink,
	HotButtonType_KronoSlot,
	HotButtonType_Command,
	HotButtonType_CombatAbility,
	HotButtonType_MountLink,
	HotButtonType_IllusionLink,
	HotButtonType_FamiliarLink,
	HotButtonType_TeleportationLink,
};

EQLIB_OBJECT const char* HotButtonTypeToString(HotButtonTypes type);

class [[offsetcomments]] CHotButton : public CXWnd
{
public:
	CHotButton(CXWnd* parent, uint32_t id, const CXRect& rect, CButtonWnd* button,
		CInvSlotWnd* invSlot, CSpellGemWnd* gemSlot);
	virtual ~CHotButton();

	EQLIB_OBJECT void SetButtonSize(int percent, bool bUpdateParent = true);
	EQLIB_OBJECT void SetCheck(bool check);

	EQLIB_OBJECT const HotButtonData* GetHotButtonData() const;

	//----------------------------------------------------------------------------
	// data members

/*0x268*/ int                BarIndex;
/*0x26c*/ int                ButtonIndex;
/*0x270*/ uint32_t           Timer;
/*0x278*/ CTextureAnimation* DecalIcon;
/*0x280*/ HotButtonTypes     LastButtonType;
/*0x284*/ int                LastButtonSlot;
/*0x288*/ char               LastButtonPage;
/*0x289*/ EqItemGuid         LastItemGuid;
/*0x29c*/ int                LastItemId;
/*0x2a0*/ int                LastIconType;
/*0x2a4*/ int                LastIconSlot;
/*0x2a8*/ CXStr              LastLabel;
/*0x2b0*/ CXStr              DefaultLabel;
/*0x2b8*/ bool               bForceUpdate;
/*0x2c0*/ CTextObjectInterface* pKeyMapText;
/*0x2c8*/ int                Unknown0x228;
/*0x2d0*/ CButtonWnd*        pButtonWnd;
/*0x2d8*/ CInvSlotWnd*       pInvSlotWnd;
/*0x2e0*/ CSpellGemWnd*      pSpellGemWnd;
/*0x2e8*/ CXSize             BaseSize;
/*0x2f0*/ int                ButtonPercentSize;
/*0x2f4*/ CXSize             BaseButtonSize;
/*0x2fc*/ CXSize             BaseDecalSize;
/*0x304*/ CXSize             BaseInvButtonSize;
/*0x30c*/ CXSize             BaseSpellButtonSize;
/*0x314*/ int                Unknown0x264;
/*0x318*/
};
//============================================================================
// CLabelWnd
//============================================================================

class [[offsetcomments]] CLabelWnd : public CXWnd
{
public:
	// constructor / destructor
	CLabelWnd(CXWnd* parent, uint32_t id, const CXRect& rect);
	virtual ~CLabelWnd();

	// virtual functions
	virtual int Draw() override;
	virtual void SetWindowText(const CXStr& Text) override;

	virtual void UpdateText();
	virtual void ResizeHeightToText();

	// data members
/*0x268*/ bool         bNoWrap;
/*0x269*/ bool         bAlignRight;
/*0x26a*/ bool         bAlignCenter;
/*0x26c*/ int          xOffset;
/*0x270*/ bool         bResizeHeightToText;
/*0x278*/ CXStr        PrependText;
/*0x280*/ CXStr        Text;
/*0x288*/ CXStr        AppendText;
/*0x290*/ bool         bTextDirty;
/*0x294*/
};

//============================================================================
// CLabel
//============================================================================

class [[offsetcomments]] CLabel : public CLabelWnd
{
public:
	// constructor / destructor
	CLabel(CXWnd* parent, uint32_t id, CXRect rect, int EQType);
	virtual ~CLabel();

	// virtual functions
	virtual void UpdateText() override;

	// data members
/*0x298*/ int          EQType;
/*0x29c*/
};

inline namespace deprecated {
	using CLABEL DEPRECATE("Use CLabel instead of CLABEL") = CLabel;
	using PCLABEL DEPRECATE("Use CLabel* instead of PCLABEL") = CLabel*;
}

//============================================================================
// CListWnd
//============================================================================

struct [[offsetcomments]] STreeData
{
/*0x00*/ int                      Depth = 0;
/*0x04*/ bool                     bIsExpandable = false;
/*0x08*/
};

struct [[offsetcomments]] SListWndCell
{
/*0x00*/ const CTextureAnimation* pTA = nullptr;
/*0x08*/ CXStr                    Text;
/*0x10*/ COLORREF                 Color = RGB(255, 255, 255);
/*0x14*/ bool                     bOnlyDrawTexture = false;
/*0x18*/ CXWnd*                   pWnd = nullptr;
/*0x20*/ void*                    Unknown1 = nullptr;
/*0x28*/
};

struct [[offsetcomments]] SListWndLine
{
/*0x000*/ ArrayClass<SListWndCell> Cells;
/*0x018*/ uint64_t                Data = 0;
/*0x020*/ int                     Height = -1;
/*0x024*/ bool                    bSelected = false;
/*0x025*/ bool                    bEnabled = true;
/*0x028*/ STreeData               Treedata;
/*0x030*/ char                    TooltipText[256];
/*0x130*/ bool                    bVisible = true;
/*0x134*/ uint32_t                bIsSeparator = 0;
/*0x138*/
};

class [[offsetcomments]] SListWndSortInfo
{
public:
/*0x00*/ int                      SortCol;
/*0x08*/ const SListWndLine&      ListWndLine1;
/*0x10*/ CXStr                    StrLabel1;
/*0x18*/ uint64_t                 Data1;
/*0x20*/ const SListWndLine&      ListWndLine2;
/*0x28*/ CXStr                    StrLabel2;
/*0x30*/ uint64_t                 Data2;
/*0x38*/ int                      SortResult;
/*0x3c*/
};

enum ECellType
{
	CellTypeBasicText            = 1,
	CellTypeBasicIcon            = 2,
	CellTypeTextIcon             = 3
};

struct [[offsetcomments]] SListWndColumn
{
/*0x00*/ int                      Width = 0;
/*0x04*/ int                      MinWidth = 10;
/*0x08*/ CXSize                   TextureSize;
/*0x10*/ CXPoint                  TextureOffset;
/*0x18*/ CXStr                    StrLabel;
/*0x20*/ uint64_t                 Data = 0;
/*0x28*/ uint32_t                 Flags = 0;
/*0x2c*/ uint32_t                 Type = CellTypeTextIcon; // ECellType
/*0x30*/ CTextureAnimation*       pTextureAnim = nullptr;
/*0x38*/ CTextureAnimation*       pSelected = nullptr;
/*0x40*/ CTextureAnimation*       pMouseOver = nullptr;
/*0x48*/ CXStr                    Tooltip;
/*0x50*/ bool                     bResizable = false;
/*0x54*/

	SListWndColumn(CXStr strLabel = "",
		int width = 0,
		CTextureAnimation* pta = nullptr,
		uint32_t flags = 0,
		uint32_t type = CellTypeTextIcon,
		CTextureAnimation* pSelected = nullptr,
		CTextureAnimation* pMouseOver = nullptr,
		CXStr tooltip = "",
		bool bResizable = false,
		CXSize textureSize = {},
		CXPoint textureOffset = {})
		: StrLabel(strLabel)
		, Width(width)
		, pTextureAnim(pta)
		, Flags(flags)
		, Type(type)
		, pSelected(pSelected)
		, pMouseOver(pMouseOver)
		, Tooltip(tooltip)
		, bResizable(bResizable)
		, TextureSize(textureSize)
		, TextureOffset(textureOffset)
	{}
};

class IListItemDataHandler
{
public:
	virtual bool GetText(int index, int subItem, CXStr& text) const = 0;
};

class [[offsetcomments]] CListWnd : public CXWnd
{
public:
	EQLIB_OBJECT CListWnd(CXWnd*, uint32_t, const CXRect&);
	virtual ~CListWnd();

	//----------------------------------------------------------------------------
	// virtuals

	EQLIB_OBJECT virtual void OnColumnClick(int column);
	EQLIB_OBJECT virtual int OnHeaderClick(CXPoint);
	EQLIB_OBJECT virtual int DrawColumnSeparators() const;
	EQLIB_OBJECT virtual int DrawSeparator(int index) const;
	EQLIB_OBJECT virtual int DrawLine(int index) const;
	EQLIB_OBJECT virtual int DrawHeader() const;
	EQLIB_OBJECT virtual int DrawItem(int index, int, int) const;
	EQLIB_OBJECT virtual void DeleteAll();
	EQLIB_OBJECT virtual int Compare(const SListWndLine&, const SListWndLine&) const;
	EQLIB_OBJECT virtual int Unknown0x188(int a, int b) const;
	EQLIB_OBJECT virtual void Sort(bool unstable = true);

	// overrides
	EQLIB_OBJECT virtual int Draw() override;
	EQLIB_OBJECT virtual int DrawBackground() const override;
	EQLIB_OBJECT virtual int DrawTooltip(const CXWnd* wnd) const override;
	EQLIB_OBJECT virtual HCURSOR GetCursorToDisplay() const override;
	EQLIB_OBJECT virtual int HandleLButtonDown(const CXPoint& pos, uint32_t flags) override;
	EQLIB_OBJECT virtual int HandleLButtonUp(const CXPoint& pos, uint32_t flags) override;
	EQLIB_OBJECT virtual int HandleLButtonHeld(const CXPoint& pos, uint32_t flags) override;
	EQLIB_OBJECT virtual int HandleLButtonUpAfterHeld(const CXPoint& pos, uint32_t flags) override;
	EQLIB_OBJECT virtual int HandleRButtonDown(const CXPoint& pos, uint32_t flags) override;
	EQLIB_OBJECT virtual int HandleRButtonUp(const CXPoint& pos, uint32_t flags) override;
	EQLIB_OBJECT virtual int HandleRButtonHeld(const CXPoint& pos, uint32_t flags) override;
	EQLIB_OBJECT virtual int HandleRButtonUpAfterHeld(const CXPoint& pos, uint32_t flags) override;
	EQLIB_OBJECT virtual int HandleMouseMove(const CXPoint& pos, uint32_t flags) override;
	EQLIB_OBJECT virtual int WndNotification(CXWnd* sender, uint32_t message, void* data) override;
	EQLIB_OBJECT virtual void OnWndNotification() override;
	EQLIB_OBJECT virtual int OnMove(const CXRect& rect) override;
	EQLIB_OBJECT virtual int OnResize(int w, int h) override;
	EQLIB_OBJECT virtual int OnVScroll(EScrollCode code, int pos) override;
	EQLIB_OBJECT virtual int OnHScroll(EScrollCode code, int pos) override;
	EQLIB_OBJECT virtual CXRect GetHitTestRect(int code) const override;
	EQLIB_OBJECT virtual CXRect GetClientClipRect() const override;
	EQLIB_OBJECT virtual CXWnd* GetChildWndAt(const CXPoint& pos, bool, bool) const override;
	EQLIB_OBJECT virtual int SetVScrollPos(int pos) override;

	//----------------------------------------------------------------------------
	// methods

	EQLIB_OBJECT bool IsLineEnabled(int) const;
	EQLIB_OBJECT const CTextureAnimation* GetColumnAnimation(int) const;
	EQLIB_OBJECT const CTextureAnimation* GetColumnAnimationMouseOver(int) const;
	EQLIB_OBJECT const CTextureAnimation* GetColumnAnimationSelected(int) const;
	EQLIB_OBJECT const CTextureAnimation* GetItemIcon(int, int) const;
	EQLIB_OBJECT CXRect GetHeaderRect(int) const;
	EQLIB_OBJECT CXRect GetItemRect(int, int) const;
	EQLIB_OBJECT CXRect GetSeparatorRect(int) const;
	EQLIB_OBJECT CXStr GetColumnLabel(int) const;
	EQLIB_OBJECT int AddColumn(const CXStr& Label, CTextureAnimation* pTA, int Width, uint32_t Flags, CXStr Tooltip = "",
		uint32_t Type = CellTypeTextIcon, CTextureAnimation* pTASelected = nullptr, CTextureAnimation* pTAMouseOver = nullptr,
		bool bResizeable = false, CXSize TextureSize = {}, CXPoint TextureOffset = {});
	EQLIB_OBJECT int AddColumn(const CXStr& Label, int Width, uint32_t Flags, uint32_t Type = CellTypeTextIcon);
	EQLIB_OBJECT int AddLine(SListWndLine*);
	EQLIB_OBJECT int AddString(const CXStr& Str, COLORREF Color, uint64_t Data = 0, const CTextureAnimation* pTa = nullptr, const char* TooltipStr = nullptr);
	int AddString(const CXStr& str, mq::MQColor Color, uint64_t Data = 0, const CTextureAnimation* pTA = nullptr, const char* TooltipStr = nullptr)
	{
		return AddString(str, Color.ToARGB(), Data, pTA, TooltipStr);
	}
	EQLIB_OBJECT int AddString(const char* Str, COLORREF Color, uint64_t Data, const CTextureAnimation* pTa, const char* TooltipStr = nullptr);
	EQLIB_OBJECT int GetColumnJustification(int) const;
	EQLIB_OBJECT int GetColumnMinWidth(int) const;
	EQLIB_OBJECT CXStr GetColumnTooltip(int) const;
	EQLIB_OBJECT int GetColumnWidth(int) const;
	EQLIB_OBJECT int GetCurCol() const;
	EQLIB_OBJECT int GetCurSel() const;
	EQLIB_OBJECT int GetItemHeight(int) const;
	EQLIB_OBJECT uint32_t GetColumnFlags(int) const;
	EQLIB_OBJECT uint64_t GetItemData(int) const;
	EQLIB_OBJECT COLORREF GetItemColor(int, int) const;
	EQLIB_OBJECT void CalculateFirstVisibleLine();
	EQLIB_OBJECT void CalculateLineHeights();
	EQLIB_OBJECT void CalculateVSBRange();
	EQLIB_OBJECT void ClearAllSel();
	EQLIB_OBJECT void ClearSel(int);
	EQLIB_OBJECT void CloseAndUpdateEditWindow();
	EQLIB_OBJECT void EnableLine(int Index, bool bVal);
	EQLIB_OBJECT void EnsureVisible(int);
	EQLIB_OBJECT void ExtendSel(int);
	EQLIB_OBJECT void InsertLine(int ID, SListWndLine* rEntry);
	EQLIB_OBJECT void RemoveLine(int);
	EQLIB_OBJECT void RemoveString(int);
	EQLIB_OBJECT void SetColumnJustification(int, int);
	EQLIB_OBJECT void SetColumnLabel(int, const CXStr&);
	EQLIB_OBJECT void SetColumnWidth(int, int);
	EQLIB_OBJECT void SetCurSel(int);
	EQLIB_OBJECT void SetItemColor(int, int, unsigned long);
	void SetItemColor(int row, int col, mq::MQColor color) { SetItemColor(row, col, color.ToARGB()); }
	EQLIB_OBJECT void SetItemData(int ID, uint64_t Data);
	EQLIB_OBJECT void SetItemText(int ID, int SubID, const CXStr& Text);
	EQLIB_OBJECT void ShiftColumnSeparator(int, int);
	EQLIB_OBJECT void ToggleSel(int);
	EQLIB_OBJECT void SetColumnsSizable(bool bColumnsSizable);
	EQLIB_OBJECT void GetWndPosition(CXWnd* pWnd, int& ItemID, int& SubItemID) const;
	EQLIB_OBJECT void SetItemWnd(int Index, int SubItem, CXWnd* pWnd);
	EQLIB_OBJECT CXWnd* GetItemWnd(int Index, int SubItem = 0) const;
	EQLIB_OBJECT void SetItemIcon(int Index, int SubItem, const CTextureAnimation* pTA);
	EQLIB_OBJECT void CalculateCustomWindowPositions();

	EQLIB_OBJECT void SetColors(COLORREF crNormal, COLORREF crHighlight, COLORREF crSelected);
	inline void SetColors(mq::MQColor normal, mq::MQColor highlight, mq::MQColor selected) { SetColors(normal.ToARGB(), highlight.ToARGB(), selected.ToARGB()); }


	EQLIB_OBJECT int GetItemAtPoint(const CXPoint& pt) const;
	EQLIB_OBJECT void GetItemAtPoint(const CXPoint& pt, int* ID, int* SubItem) const;


	inline int GetColumnCount() const { return Columns.GetLength(); }

	// Index of the first row in the list where the text in column matches predicate, or -1 if no row matches
	EQLIB_OBJECT int IndexOf(int column, const std::function<bool(const CXStr)>& predicate);

	// Index of the first row in the list where the text in the first column matches predicate, or -1 if no row matches
	EQLIB_OBJECT int IndexOf(const std::function<bool(const CXStr)>& predicate);

	// True if the list contains a row where the text in columns matches predicate
	EQLIB_OBJECT bool Contains(int column, const std::function<bool(const CXStr)>& predicate);

	// True if the list contains a row the text in the first column matches predicate
	EQLIB_OBJECT bool Contains(const std::function<bool(const CXStr)>& predicate);

	EQLIB_OBJECT CXStr GetItemText(int index, int subIndex = 0) const;

	DEPRECATE("GetItemText: Passing in a pointer to CXStr for GetItemText is deprecated. It should return CXStr instead.")
	void GetItemText(CXStr* pStr, int index, int subIndex = 0) const
	{
		*pStr = GetItemText(index, subIndex);
	}

	//----------------------------------------------------------------------------

	// Sets the sorting column. If this is the current column it will flip its order.
	inline void SetSortColumn(int column)
	{
		if (column < 0 || column >= Columns.GetCount())
			return;

		if (SortCol == column)
			bSortAsc = !bSortAsc;
		else
		{
			SortCol = column;
			bSortAsc = true;
		}

		Sort(false);
	}

	inline int GetItemCount() const { return ItemsArray.GetLength(); }

	//----------------------------------------------------------------------------
	// data members

/*0x268*/ int                 Unknown0x1f0;
/*0x270*/ ArrayClass<SListWndLine> ItemsArray;
/*0x288*/ ArrayClass<SListWndColumn> Columns;
/*0x2a0*/ int                 CurSel;
/*0x2a4*/ int                 CurCol;
/*0x2a8*/ int                 DownItem;
/*0x2ac*/ int                 ScrollOffsetY;
/*0x2b0*/ int                 HeaderHeight;
/*0x2b4*/ int                 FirstVisibleLine;
/*0x2b8*/ int                 SortCol;
/*0x2bc*/ bool                bSortAsc;
/*0x2bd*/ bool                bFixedHeight;
/*0x2be*/ bool                bOwnerDraw;
/*0x2bf*/ bool                bCalcHeights;
/*0x2c0*/ bool                bColumnSizable;
/*0x2c4*/ int                 LineHeight;
/*0x2c8*/ int                 ColumnSepDragged;
/*0x2cc*/ int                 ColumnSepMouseOver;
/*0x2d0*/ COLORREF            HeaderText;
/*0x2d4*/ COLORREF            Highlight;
/*0x2d8*/ COLORREF            Selected;
/*0x2e0*/ CUITextureInfo      BGHeader;
/*0x300*/ COLORREF            BGHeaderTint;
/*0x308*/ CTextureAnimation*  pRowSep;
/*0x310*/ CTextureAnimation*  pColumnSep;
/*0x318*/ CEditBaseWnd*       pEditCell;
/*0x320*/ IListItemDataHandler* pItemDataHandler;
/*0x328*/ bool                bHasItemTooltips;
/*0x32c*/ CXRect              PrevInsideRect;
/*0x33c*/ uint32_t            ListWndStyle;
/*0x340*/ eqtime_t            LastVisibleTime;
/*0x348*/

	struct [[offsetcomments]] VirtualFunctionTable : public CXWnd::VirtualFunctionTable
	{
	/*0x348*/ void* OnColumnClick;
	/*0x350*/ void* OnHeaderClick;
	/*0x358*/ void* DrawColumnSeparators;
	/*0x360*/ void* DrawSeparator;
	/*0x368*/ void* DrawLine;
	/*0x370*/ void* DrawHeader;
	/*0x378*/ void* DrawItem;
	/*0x380*/ void* DeleteAll;
	/*0x388*/ void* Compare;
	/*0x390*/ void* Unknown0x188;
	/*0x398*/ void* Sort;
	/*0x3a0*/
	};

	// points to the eq instance of the virtual function table for this class
	static VirtualFunctionTable* sm_vftable;
};

//============================================================================
// CPageWnd
//============================================================================

class [[offsetcomments]] CPageWnd : public CSidlScreenWnd
{
public:
	CPageWnd(CXWnd*, uint32_t, const CXRect&, const CXStr&, CPageTemplate*);
	virtual ~CPageWnd();

	CTextureAnimation* GetTabIcon() const { return pTATabIcon; }

	EQLIB_OBJECT CXStr GetTabText(bool bShowFlashing = false) const;
	EQLIB_OBJECT void SetTabText(CXStr&) const;
	EQLIB_OBJECT void FlashTab(bool flash, int msTime) const;

/*0x2d0*/ CXStr              TabText;
/*0x2d8*/ CXStr              OrigTabText;
/*0x2e0*/ COLORREF           CRTabText;
/*0x2e4*/ COLORREF           CRTabTextActive;
/*0x2e8*/ CTextureAnimation* pTATabIcon;
/*0x2f0*/ CTextureAnimation* pTATabIconActive;
/*0x2f8*/ int                Unknown0x250;
/*0x2fc*/ int                Unknown0x254;
/*0x300*/ int64_t            LastFlashTime;
/*0x308*/ COLORREF           CRHighlightFlashColor;
/*0x30c*/ bool               bHighlightOnNewMessages;
/*0x30d*/ bool               bFlashing;
/*0x310*/
};

//============================================================================
// CSliderWnd
//============================================================================

// size: 0x258 6/10/2019 (test)
class [[offsetcomments]] CSliderWnd : public CXWnd
{
public:
	CSliderWnd(CXWnd*, uint32_t, const CXRect&, CSliderTemplate*);
	virtual ~CSliderWnd();

	virtual int Draw() override;
	virtual int HandleKeyboardMsg(uint32_t, uint32_t, bool) override;
	virtual int HandleLButtonDown(const CXPoint&, uint32_t) override;
	virtual int HandleLButtonUp(const CXPoint&, uint32_t) override;
	virtual int HandleLButtonUpAfterHeld(const CXPoint&, uint32_t) override;
	virtual int HandleMouseMove(const CXPoint&, uint32_t) override;
	virtual int OnProcessFrame() override;

	EQLIB_OBJECT CXRect GetEndCapLeftRect() const;
	EQLIB_OBJECT CXRect GetEndCapRightRect() const;
	EQLIB_OBJECT CXRect GetMiddleRangeRect() const;
	EQLIB_OBJECT CXRect GetThumbRect() const;
	EQLIB_OBJECT int GetValue() const;
	EQLIB_OBJECT void SetNumTicks(int);
	EQLIB_OBJECT void SetValue(int);

	EQLIB_OBJECT int DrawEndCapLeft() const;
	EQLIB_OBJECT int DrawEndCapRight() const;
	EQLIB_OBJECT int DrawMiddleRange() const;
	EQLIB_OBJECT int DrawThumb() const;
	EQLIB_OBJECT void SetThumbToOffset(int);
	EQLIB_OBJECT void UpdateMiddleRange();
	EQLIB_OBJECT void UpdateThumb();

	//----------------------------------------------------------------------------
	// data members

/*0x268*/ CButtonDrawTemplate      bdtThumb;
/*0x2d0*/ CTextureAnimation*       ptaBackground;
/*0x2d8*/ CTextureAnimation*       ptaEndCapLeft;
/*0x2e0*/ CTextureAnimation*       ptaEndCapRight;
/*0x2e8*/
	// more ...
};

//============================================================================
// CStmlWnd
//============================================================================

enum ELinkType
{
	STML_LINK_FILE,
	STML_LINK_UNIT,
	STML_LINK_BUILDING,
	STML_LINK_MESSAGE,
	STML_LINK_CAMERA,
	STML_LINK_REPORT,
	STML_LINK_WNDNOTIFY,
	STML_LINK_EMPTY,
	STML_LINK_URL,
	STML_LINK_PLAYER,
	STML_LINK_SPAM,
	STML_LINK_ACHIEVEMENT,
	STML_LINK_HELP,
	STML_LINK_DIALOG_RESPONSE,
	STML_LINK_COMMAND,
	STML_NUM_LINK_TYPES,
};

enum EStmlTargetValue
{
	STML_TARGET_SELF,
	STML_TARGET_BLANK,
};

struct [[offsetcomments]] SFontTag
{
/*0x00*/ CTextureFont*      face;
/*0x08*/ COLORREF           color;
/*0x0c*/
};

using SStyleTag = uint32_t;

const uint32_t STML_TAG_BOLD = 0;
const uint32_t STML_TAG_UNDERLINE = 2;

struct [[offsetcomments]] SFormattedText
{
/*0x00*/ SFontTag           Font;
/*0x10*/ SStyleTag          Style;
/*0x14*/ int                Left;
/*0x18*/ int                Right;
/*0x20*/ CXStr              Text;
/*0x28*/ int32_t            LinkID;
/*0x30*/ CTextureAnimation* Texture;
/*0x38*/ int32_t            TableID;
/*0x3c*/ int32_t            IndexStart;
/*0x40*/
};

struct [[offsetcomments]] STextLine
{
/*0x00*/ ArrayClass<SFormattedText> TextPieces;
/*0x18*/ int                yBottom;
/*0x1c*/ int                yTop;
/*0x20*/ int32_t            IndexStart;
/*0x24*/
};

struct [[offsetcomments]] SLinkInfo
{
/*0x00*/ ELinkType          Type = STML_LINK_EMPTY;
/*0x04*/ uint32_t           MsgID = -1;
/*0x08*/ CXStr              Name;
/*0x10*/ EStmlTargetValue   TargetType = STML_TARGET_SELF;
/*0x14*/
};

struct SParseVariables
{
	// todo ...
};

struct [[offsetcomments]] SHistoryElement
{
/*0x00*/ SLinkInfo          sLink;
/*0x18*/ CXStr              STMLText;
/*0x20*/
};

enum ESTMLParseState
{
	STML_PARSE_STATE_BODY,
	STML_PARSE_STATE_HEAD,
};


class [[offsetcomments]] CStmlWnd : public CXWnd
{
public:
	CStmlWnd(CXWnd*, uint32_t, const CXRect&);
	virtual ~CStmlWnd();

	virtual bool IsPointTransparent(const CXPoint&) const override;
	virtual HCURSOR GetCursorToDisplay() const override;
	virtual int Draw() override;
	virtual int HandleKeyboardMsg(uint32_t, uint32_t, bool) override;
	virtual int HandleLButtonDown(const CXPoint&, uint32_t) override;
	virtual int HandleLButtonUp(const CXPoint&, uint32_t) override;
	virtual int HandleMouseMove(const CXPoint&, uint32_t) override;
	virtual int OnMove(const CXRect&) override;
	virtual int OnProcessFrame() override;
	virtual int OnResize(int, int) override;
	virtual int OnHScroll(EScrollCode, int) override;
	virtual int OnVScroll(EScrollCode, int) override;
	virtual void SetWindowText(const CXStr&) override;

	EQLIB_OBJECT bool CanGoBackward();
	EQLIB_OBJECT CXSize AppendSTML(CXStr);
	EQLIB_OBJECT CXStr GetVisibleText(CXStr, const CXRect&) const;
	EQLIB_OBJECT static CXStr MakeStmlColorTag(unsigned long);
	EQLIB_OBJECT static CXStr MakeWndNotificationTag(uint32_t, const CXStr&, const CXStr&);
	EQLIB_OBJECT void ActivateLink(SLinkInfo);
	EQLIB_OBJECT void ForceParseNow();
	EQLIB_OBJECT void GoToBackHistoryLink();
	// EQLIB_OBJECT void LoadPage(CXStr, enum ESTMLTargetValue, bool);
	EQLIB_OBJECT void SetSTMLText(CXStr, bool addToHistory = true, SLinkInfo* = nullptr);
	EQLIB_OBJECT void SetSTMLTextWithoutHistory(CXStr);

	// protected
	EQLIB_OBJECT bool IsLinkActive(SLinkInfo) const;
	EQLIB_OBJECT bool ParseAmpersand(CXStr, char*) const;
	EQLIB_OBJECT static bool CanBreakAtCharacter(unsigned short);
	EQLIB_OBJECT static bool IsCharacterNotASpaceAndNotNULL(unsigned short);
	EQLIB_OBJECT static bool IsCharacterNotEquals(unsigned short);
	EQLIB_OBJECT static bool IsCharacterNotQuotes(unsigned short);
	EQLIB_OBJECT unsigned short FastForwardPastQuotesAndGetNextTagPiece(const CXStr&, CXStr*, int*, bool);
	EQLIB_OBJECT unsigned short FastForwardPastSpaces(const CXStr&, int*);
	EQLIB_OBJECT unsigned short FastForwardPastSpacesAndQuotes(const CXStr&, int*);
	EQLIB_OBJECT unsigned short FastForwardToEndOfTag(const CXStr&, CXStr*, int*, char);
	EQLIB_OBJECT unsigned short GetNextChar(int*, const CXStr&);
	EQLIB_OBJECT unsigned short GetNextTagPiece(const CXStr&, CXStr*, int*, bool (*)(unsigned short), bool);
	EQLIB_OBJECT unsigned short GetThisChar(int, const CXStr&);
	EQLIB_OBJECT void ActivateLinkFile(SLinkInfo);
	EQLIB_OBJECT void ActivateLinkMessageId(SLinkInfo);
	EQLIB_OBJECT void ActivateLinkReport(SLinkInfo, bool);
	EQLIB_OBJECT void ActivateLinkWndNotify(SLinkInfo);
	EQLIB_OBJECT void AddLinkToHistory(SLinkInfo, const CXStr&);
	EQLIB_OBJECT void AddTextPieceToLine(SParseVariables*);
	EQLIB_OBJECT void CalculateHSBRange(int);
	EQLIB_OBJECT void CalculateVSBRange(int);
	EQLIB_OBJECT void CompleteParse();
	EQLIB_OBJECT void InitializeTempVariables(SParseVariables*, const CXRect&);
	EQLIB_OBJECT void InitializeTextLine(SParseVariables*, int);
	EQLIB_OBJECT void InitializeWindowVariables();
	// EQLIB_OBJECT void ParseLinkTarget(const CXStr&, enum ESTMLTargetValue*) const;
	EQLIB_OBJECT void ParseSTMLHead(const CXStr&);
	EQLIB_OBJECT void ParseSTMLTable(const CXStr&, int*, const CXStr&, SParseVariables*);
	EQLIB_OBJECT void ParseSTMLTableAttributes(const CXStr&, STable*);
	// EQLIB_OBJECT void ParseTagAlign(const CXStr&, enum ESTMLAlign*) const;
	EQLIB_OBJECT void ParseTagColor(const CXStr&, unsigned long*) const;
	EQLIB_OBJECT void ParseTagFace(const CXStr&, const CTextureFont**) const;
	EQLIB_OBJECT void ResetTempVariablesForNewLine(SParseVariables*);
	EQLIB_OBJECT void StripFirstSTMLLines(int);
	EQLIB_OBJECT void UpdateHistoryString(int32_t, const CXStr&);

	inline CXStr GetSTMLText() const { return STMLText; }

	//----------------------------------------------------------------------------
	// data members

	// todo: verify
/*0x268*/ CXStr                        STMLText;
/*0x270*/ CircularArrayClass2<STextLine> TextLines;
/*0x2a0*/ int32_t                      TextTotalHeight;
/*0x2a4*/ int32_t                      TextTotalWidth;
/*0x2a8*/ int32_t                      Unknown0x224;
/*0x2b0*/ ArrayClass2<SLinkInfo>       Links;
/*0x2d0*/ ArrayClass<STable>           Tables;
/*0x2e8*/ bool                         bReparseNow;
/*0x2e9*/ bool                         bResized;
/*0x2ea*/ bool                         bAlignCenter;
/*0x2ec*/ int                          LineSpacingAdjust;
/*0x2f0*/ int32_t                      CapturedLinkID;
/*0x2f4*/ int32_t                      MousedOverLinkID;
/*0x2f8*/ COLORREF                     BackGroundColor;
/*0x2fc*/ COLORREF                     TextColor;
/*0x300*/ COLORREF                     LinkColor;
/*0x304*/ COLORREF                     VLinkColor;
/*0x308*/ COLORREF                     ALinkColor;
/*0x30c*/ COLORREF                     MLinkColor;
/*0x310*/ ESTMLParseState              CurrentParseState;
/*0x318*/ ArrayClass2<SHistoryElement> HistoryArray;
/*0x338*/ int32_t                      HistoryIndex;
/*0x340*/ CStmlReport*                 pStmlReport;
/*0x348*/ int                          MaxLines;
/*0x34c*/ int                          PlayerContextMenuIndex;
/*0x350*/ int                          Unknown0x2ac;
/*0x354*/ int                          Unknown0x2b0;
/*0x358*/ int                          Unknown0x2b4;
/*0x35c*/
};

//============================================================================
// CTabWnd
//============================================================================

class [[offsetcomments]] CTabWnd : public CXWnd
{
public:
	CTabWnd(CXWnd* pParent, uint32_t uId, const CXRect& rect, CTabBoxTemplate* pTabContents);
	virtual ~CTabWnd();

	// functions we have offsets for
	EQLIB_OBJECT int DrawCurrentPage() const;
	EQLIB_OBJECT int DrawTab(int) const;
	EQLIB_OBJECT void UpdatePage();

	EQLIB_OBJECT void SetPageRect(const CXRect&);
	EQLIB_OBJECT bool SetPage(CPageWnd*, bool bNotifyParent = true, bool bBringToTop = true);
	EQLIB_OBJECT CPageWnd* GetPageFromTabPoint(CXPoint) const;

	EQLIB_OBJECT CXRect GetPageClientRect() const;
	EQLIB_OBJECT CXRect GetPageInnerRect() const;
	EQLIB_OBJECT CXRect GetTabInnerRect(int) const;
	EQLIB_OBJECT CXRect GetTabRect(int) const;

	int GetNumTabs() const { return PageArray.GetLength(); }
	int GetCurrentTabIndex() const { return CurTabIndex; }

	EQLIB_OBJECT CPageWnd* GetPageFromTabIndex(int tabIndex) const;
	EQLIB_OBJECT CPageWnd* GetCurrentPage() const;

	EQLIB_OBJECT void SetPage(int index, bool bNotifyParent = true, bool bBringToTop = true, bool bSomething = false);
	EQLIB_OBJECT void InsertPage(CPageWnd* pPageWnd, int position = -1); // defaults to the last tab
	EQLIB_OBJECT void RemovePage(CPageWnd* pPageWnd);

	inline bool IsValidIndex(int index) const
	{
		return index >= 0 && index < GetNumTabs();
	}


	//----------------------------------------------------------------------------
	// data members

/*0x268*/ CTAFrameDraw*           pTabBorder;
/*0x270*/ CTAFrameDraw*           pPageBorder;
/*0x278*/ ArrayClass<CPageWnd*>   PageArray;
/*0x290*/ int                     CurTabIndex;
/*0x294*/ int                     TabHeight;
/*0x298*/ CXRect                  PageRect;
/*0x2a8*/ bool                    bShowTabs;
/*0x2ac*/ int                     TabStyle;
/*0x2b0*/ int                     TabWidth;
/*0x2b4*/
};

//============================================================================
// CTreeViewWnd
//============================================================================

class [[offsetcomments]] CTreeViewWnd : public CListWnd
{
public:
	CTreeViewWnd(CXWnd*, uint32_t, CXRect, int);
	virtual ~CTreeViewWnd();
};

//============================================================================

class [[offsetcomments]] CLayoutWnd : public CSidlScreenWnd
{
public:
	CLayoutWnd(CXWnd*, uint32_t, const CXRect&, const CXStr&);
	virtual ~CLayoutWnd();

/*0x2d0*/ uint32_t             spacing;
/*0x2d4*/ bool                 expandLast;
/*0x2d8*/ int                  paddingLeft;
/*0x2dc*/ int                  paddingRight;
/*0x2e0*/ int                  paddingTop;
/*0x2e4*/ int                  paddingBottom;
/*0x2e8*/ };

// Horizontal and Vertical layouts are basically the same, but transposed.
class [[offsetcomments]] CHorizontalLayoutWnd : public CLayoutWnd
{
	FORCE_SYMBOLS

public:
/*0x2e8*/ ArrayClass<CXWnd*>         visibleChildren;
/*0x300*/ ArrayClass<CXRect>         dividerRects;
/*0x318*/ int                        activeDivider;
/*0x31c*/ bool                       dividerHover;
/*0x31d*/ bool                       dividerMoving;
/*0x320*/ int                        oldPosCoord;              // X for horizontal, Y for vertical
/*0x324*/ bool                       bStyle_Dividers;          // the Style_Dividers property from XML
/*0x328*/
};

class [[offsetcomments]] CVerticalLayoutWnd : public CLayoutWnd
{
public:
/*0x2e8*/ ArrayClass<CXWnd*>         visibleChildren;
/*0x300*/ ArrayClass<CXRect>         dividerRects;
/*0x318*/ int                        activeDivider;
/*0x31c*/ bool                       dividerHover;
/*0x31d*/ bool                       dividerMoving;
/*0x320*/ int                        oldPosCoord;              // X for horizontal, Y for vertical
/*0x324*/ bool                       bStyle_Dividers;          // the Style_Dividers property from XML
/*0x328*/
};

class [[offsetcomments]] CTileLayoutWnd : public CLayoutWnd
{
	FORCE_SYMBOLS

public:
/*0x2e8*/ bool                       horizontalFirst;
/*0x2e9*/ bool                       anchorToTop;
/*0x2ea*/ bool                       anchorToLeft;
/*0x2ec*/ int                        secondarySpacing;
/*0x2f0*/ bool                       firstPieceTemplate;
/*0x2f1*/ bool                       snapToChildren;
/*0x2f8*/ CXWnd*                     autoStretchWindow;
/*0x300*/
};

//============================================================================
//============================================================================
//============================================================================
//============================================================================

//============================================================================
// CAAWnd
//============================================================================

class CAAWnd : public CSidlScreenWnd, public PopDialogHandler, public WndEventHandler
{
public:
	CAAWnd(CXWnd*);
	virtual ~CAAWnd();

	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	// When confirmation for AA purchase is displayed, two message types are used
	// to identify which button was pressed
	// 100 = "Train" button
	// 101 = "Buy All" button
	virtual void DialogResponse(int msg, int, void*) {}

	EQLIB_OBJECT void CancelAASpend();
	EQLIB_OBJECT void ConfirmAASpend();
	EQLIB_OBJECT void SendNewPercent();
	EQLIB_OBJECT bool ShowAbility(int);
	EQLIB_OBJECT void Update(bool);
	EQLIB_OBJECT void UpdateTimer();

	// private
	EQLIB_OBJECT void Init();
	EQLIB_OBJECT void UpdateSelected();
};

//============================================================================
// CAchievementsWnd
//============================================================================

class CAchievementsWnd : public CSidlScreenWnd
{
public:
	CAchievementsWnd(CXWnd*);
	virtual ~CAchievementsWnd();
};

//============================================================================
// CActionsWnd
//============================================================================

class CActionsWnd : public CSidlScreenWnd, public WndEventHandler
{
public:
	CActionsWnd(CXWnd*);
	virtual ~CActionsWnd();

	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT CButtonWnd* GetAbilityBtn(int);
	EQLIB_OBJECT CButtonWnd* GetCombatBtn(int);
	EQLIB_OBJECT CButtonWnd* GetMenuBtn(int);
	EQLIB_OBJECT int GetCurrentPageEnum() const;
	EQLIB_OBJECT void Activate(int);
	EQLIB_OBJECT void KeyMapUpdated();
	EQLIB_OBJECT void SelectSkillForAbilityBtn(int);
	EQLIB_OBJECT void SelectSkillForCombatBtn(int);
	EQLIB_OBJECT void UpdateButtonText();

	// private
	EQLIB_OBJECT int AbilitiesPageActivate();
	EQLIB_OBJECT int AbilitiesPageDeactivate();
	EQLIB_OBJECT int AbilitiesPageOnProcessFrame();
	EQLIB_OBJECT int AbilitiesPageWndNotification(CXWnd*, uint32_t, void*);
	EQLIB_OBJECT int CombatPageActivate();
	EQLIB_OBJECT int CombatPageDeactivate();
	EQLIB_OBJECT int CombatPageOnProcessFrame();
	EQLIB_OBJECT int CombatPageWndNotification(CXWnd*, uint32_t, void*);
	EQLIB_OBJECT int MainPageActivate();
	EQLIB_OBJECT int MainPageDeactivate();
	EQLIB_OBJECT int MainPageOnProcessFrame();
	EQLIB_OBJECT int MainPageWndNotification(CXWnd*, uint32_t, void*);
	EQLIB_OBJECT int RedirectActivateTo(CPageWnd*);
	EQLIB_OBJECT int RedirectDeactivateTo(CPageWnd*);
	EQLIB_OBJECT int RedirectOnProcessFrameTo(CPageWnd*);
	EQLIB_OBJECT int RedirectWndNotificationTo(CPageWnd*, CXWnd*, uint32_t, void*);
	EQLIB_OBJECT int SocialsPageActivate();
	EQLIB_OBJECT int SocialsPageDeactivate();
	EQLIB_OBJECT int SocialsPageOnProcessFrame();
	EQLIB_OBJECT int SocialsPageWndNotification(CXWnd*, uint32_t, void*);
};

//============================================================================
// CAdvancedLootWnd
//============================================================================

struct [[offsetcomments]] AdvancedLootItemNPC
{
/*0x00*/ unsigned int  CorpseID;                 // spawnId of the corpse that has this lootitem
/*0x04*/ short         StackCount;
/*0x08*/ unsigned int  Expiration;
/*0x0c*/ bool          Locked;
/*0x0d*/ char          Name[0x40];
/*0x50*/ //more data here?
};

inline namespace deprecated {
	using LOOTDETAILS DEPRECATE("Use AdvancedLootItemNPC instead of LOOTDETAILS") = AdvancedLootItemNPC;
	using PLOOTDETAILS DEPRECATE("Use AdvancedLootItemNPC* instead of PLOOTDETAILS") = AdvancedLootItemNPC*;
}

enum eAdvLootState
{
	eAdvLootWaiting,
	eAdvLootAsk,
	eAdvLootAskAutoRoll,
	eAdvLootStop,
	eAdvLootAskCompleted,
	eAdvLootFreeGrab,
	eAdvLootFixedAskAutoRoll,
	eAdvLootFixedAskCompleted,
	eAdvLootRemoved
};

enum eAdvLootStatus
{
	eAdvLootStatusWaiting,
	eAdvLootStatusAsking,
	eAdvLootStatusRolling,
	eAdvLootStatusStopped,
	eAdvLootStatusClickRoll,
	eAdvLootStatusFreeGrab,
};

struct [[offsetcomments]] AdvancedLootItem
{
/*0x00*/ int64_t              ItemID;                   // EqGuid?
/*0x08*/ char                 Name[0x40];
/*0x48*/ int                  IconID;
/*0x4c*/ bool                 bStackable;
/*0x50*/ int                  MaxStack;
/*0x54*/ bool                 NoDrop;
/*0x58*/ int                  ComboID;
/*0x5c*/ unsigned int         LootID;
/*0x60*/ eAdvLootState        State;
/*0x64*/ eAdvLootStatus       Status;
/*0x68*/ bool                 bAutoRoll;
/*0x69*/ bool                 ActivelyManaged;          // User has the manage Window up
/*0x6a*/ bool                 ContextMenu;              // item has a context menu
/*0x6b*/ bool                 AskRandomMode;            // item is in AskRandom mode
/*0x6c*/ bool                 CLootInProgress;
/*0x6d*/ bool                 PLootInProgress;
/*0x70*/ SoeUtil::Array<AdvancedLootItemNPC> LootDetails;
/*0x88*/ unsigned int         AskTimer;
/*0x8c*/ bool                 AutoRoll;
/*0x8d*/ bool                 FG;
/*0x8e*/ bool                 Need;
/*0x8f*/ bool                 Greed;
/*0x90*/ bool                 No;
/*0x91*/ bool                 AlwaysNeed;
/*0x92*/ bool                 AlwaysGreed;
/*0x93*/ bool                 Never;
/*0x94*/
};

inline namespace deprecated {
	using LOOTITEM DEPRECATE("Use AdvancedLootItem instead of LOOTITEM") = AdvancedLootItem;
	using PLOOTITEM DEPRECATE("Use AdvancedLootItem* instead of PLOOTITEM") = AdvancedLootItem*;
}

struct [[offsetcomments]] AdvancedLootItemList
{
/*0x00*/ SoeUtil::Array<AdvancedLootItem> Items;
/*0x18*/ bool         bIsPersonalLoot;
/*0x20*/ CListWnd*    SharedLootList;
/*0x28*/ CComboWnd*   PersonalLootList;
/*0x30*/
};

inline namespace deprecated {
	using LOOTLIST DEPRECATE("Use AdvancedLootItemList instead of LOOTLIST") = AdvancedLootItemList;
	using PLOOTLIST DEPRECATE("Use AdvancedLootItemList* instead of PLOOTLIST") = AdvancedLootItemList*;
}

class CorpseData;
class RefreshCorpseItemChoices;
class CGroupMemberBase;
struct RaidMember;

class [[offsetcomments]] CAdvancedLootWnd : public CSidlScreenWnd, public PopDialogHandler, public WndEventHandler
{
public:
	CAdvancedLootWnd(CXWnd*);
	virtual ~CAdvancedLootWnd();

	// virtuals
	virtual void UpdateMasterLooter(const CXStr&, bool);
	virtual void ToggleGroupedByNpc();
	virtual void ToggleApplyFilters();
	virtual void AddCorpse(CorpseData*, RefreshCorpseItemChoices*);
	virtual void RemoveCorpse(uint32_t id);
	virtual void ReduceItemCount(bool, int, int, int);
	virtual void ClearCorpses();
	virtual void AddPlayerToList(CGroupMemberBase*);
	virtual void AddPlayerToList(const RaidMember*);
	virtual void ResetComboBox();
	virtual bool ConstructChange();
	virtual void AddPlayerAccess(uint32_t id);

	// GiveTo
	EQLIB_OBJECT void DoAdvLootAction(int listindex, const CXStr& Name, bool Action, int Quantity);
	// GiveTo
	EQLIB_OBJECT void DoSharedAdvLootAction(const AdvancedLootItem& pLootItem, const CXStr& Assignee, bool Action, int Quantity);

	//----------------------------------------------------------------------------
	// data members

/*0x2e0*/ CVerticalLayoutWnd*       pLayout;                 // ADLW_ScreenLayout
/*0x2e8*/ CListWnd*                 pPersonalLootList;       // ADLW_PLLList
/*0x2f0*/ CButtonWnd*               pPersonalSetBtn;         // ADLW_PLLSetBtn
/*0x2f8*/ CComboWnd*                pPersonalSetCombo;       // ADLW_PLLSetCmbo
/*0x300*/ CButtonWnd*               pLootAllBtn;             // ADLW_PLL_LootAllBtn
/*0x308*/ CButtonWnd*               pLeaveAllBtn;            // ADLW_PLL_LeaveAllBtn
/*0x310*/ CXWnd*                    pCLLWnd;                 // ADLW_CLLWnd
/*0x318*/ CListWnd*                 pConstructLootList;      // ADLW_CLLList
/*0x320*/ CButtonWnd*               pConstructSetBtn;        // ADLW_CLLSetBtn
/*0x328*/ CComboWnd*                pConstructSetCombo;      // ADLW_CLLSetCmbo
/*0x330*/ CButtonWnd*               pContructLeaveAllBtn;    // ADLW_CLL_LeaveAllBtn
/*0x338*/ CCheckBoxWnd*             pApplyFiltersChk;        // ADLW_ApplyFiltersCheck
/*0x340*/ CCheckBoxWnd*             pGroupedByNpcChk;        // ADLW_GroupedByNpcCheckbox
/*0x348*/ CButtonWnd*               pEditFiltersBtn;         // ADLW_EditFitlersBtn
/*0x350*/ CButtonWnd*               pLootSettingsBtn;        // ADLW_LootSettingsBtn
/*0x358*/ CLabelWnd*                pCalculatedMasterLooter; // ADLW_CalculatedMasterLooter
/*0x360*/ CButtonWnd*               pItemBtnTemplate;        // ADLW_ItemBtnTemplate
/*0x368*/ CButtonWnd*               pStatusTemplate;         // ADLW_StatusTemplate
/*0x370*/ CButtonWnd*               pNeverBtnTemplate;       // ADLW_NvrBtnTemplate
/*0x378*/ CButtonWnd*               pLootBtnTemplate;        // ADLW_LootBtnTemplate
/*0x380*/ CButtonWnd*               pLeaveBtnTemplate;       // ADLW_LeaveBtnTemplate
/*0x388*/ CButtonWnd*               pFreeGrabBtnTemplate;    // ADLW_FreeGrabBtnTemplate
/*0x390*/ CCheckBoxWnd*             pCheckBoxTemplate;       // ADLW_CheckBoxTemplate
/*0x398*/ CButtonWnd*               pTextLinkTemplate;       // ADLW_TextLinkTemplate
/*0x3a0*/ CButtonWnd*               pManageBtnTemplate;      // ADLW_ManageBtnTemplate
/*0x3a8*/ CButtonWnd*               pActionBtnTemplate;      // ADLW_ActionBtnTemplate
/*0x3b0*/ int                       PLLNPCColumnWidth;
/*0x3b4*/ int                       CLLNPCColumnWidth;
/*0x3b8*/ int                       CLLRNDColumnWidth;
/*0x3bc*/ int                       CLLManageColumnWidth;
/*0x3c0*/ int                       CLLActionColumnWidth;
/*0x3c4*/ int                       PLLANColumnWidth;
/*0x3c8*/ int                       PLLAGColumnWidth;
/*0x3cc*/ int                       PLLNeverColumnWidth;
/*0x3d0*/ int                       CLLANColumnWidth;
/*0x3d4*/ int                       CLLAGColumnWidth;
/*0x3d8*/ int                       CLLNeverColumnWidth;
/*0x3e0*/ AdvancedLootItemList*     pCLootList;
/*0x3e8*/ AdvancedLootItemList*     pPLootList;
/*0x3f0*/ uint32_t                  LastUpdateTime;
/*0x3f4*/ uint32_t                  LastTransactionCheck;
/*0x3f8*/ bool                      bPopupPending;
/*0x3fc*/ int                       Unknown1;
/*0x400*/ int                       Unknown2;
/*0x404*/ bool                      bFirstTimeShowingCLL;
/*0x408*/ int                       TotalLootCount;
/*0x40c*/ bool                      bAutoInventoryQuantity;
/*0x410*/ int                       CLLActionMenu;
/*0x414*/ int                       CLLActionMenuQty;
/*0x418*/ int                       PLLActionMenu;
/*0x41c*/ bool                      bUnknown3;
/*0x420*/
};

inline namespace deprecated {
	using EQADVLOOTWND DEPRECATE("Use CAdvancedLootWnd instead of EQADVLOOTWND") = CAdvancedLootWnd;
	using PEQADVLOOTWND DEPRECATE("Use CAdvancedLootWnd* instead of PEQADVLOOTWND") = CAdvancedLootWnd*;
}

//============================================================================
// CAlarmWnd
//============================================================================

class CAlarmWnd : public CSidlScreenWnd, public WndEventHandler
{
public:
	CAlarmWnd(CXWnd*);
	virtual ~CAlarmWnd();

	// virtual
	int OnProcessFrame() override;
	int WndNotification(CXWnd*, uint32_t, void*) override;

	// private
	EQLIB_OBJECT void DoNeverButton();
};

//============================================================================
// CAuraWnd
//============================================================================

class CAuraWnd : public CSidlScreenWnd
{
public:
	CAuraWnd(CXWnd*);
	virtual ~CAuraWnd();
};

//============================================================================
// CBandolierWnd
//============================================================================

class [[offsetcomments]] CBandolierWnd : public CSidlScreenWnd, public WndEventHandler
{
public:
	CBandolierWnd();
	virtual ~CBandolierWnd();

/*0x2d8*/ CButtonWnd*        pAddButton;
/*0x2e0*/ CButtonWnd*        pDeleteButton;
/*0x2e8*/ CButtonWnd*        pUseButton;
/*0x2f0*/ CButtonWnd*        pMkHotButton;
/*0x2f8*/ CButtonWnd*        pAutoSwapButton;
/*0x300*/ CButtonWnd*        pDisplayButtons[MAX_BANDOLIER_SLOTS];
/*0x320*/ CListWnd*          pWeaponSetList;
/*0x328*/
};

//============================================================================
// CBankWnd
//============================================================================

class [[offsetcomments]] CBankWnd : public CSidlScreenWnd, public WndEventHandler
{
public:
/*0x2d4*/ int          MoneyButtonIndex;
/*0x2d8*/ unsigned int NextRefreshTime;
/*0x2dc*/ bool         bInventoryWasActive;
/*0x2dd*/ bool         bRealEstateManagementWasActive;
/*0x2e0*/ CButtonWnd*  MoneyButtons[5];           // including shared plat
/*0x308*/ CLabel*      BankerNameLabel;
/*0x310*/ CInvSlotWnd* InvSlotWindows[NUM_BANK_SLOTS];
/*0x3d0*/ CLabel*      SharedBankLabel;
/*0x3d8*/ CInvSlotWnd* SharedSlotWindows[NUM_SHAREDBANK_SLOTS];
/*0x408*/ CButtonWnd*  DoneButton;                // DoneButton
/*0x410*/ CButtonWnd*  ChangeButton;              // ChangeButton
/*0x418*/ CButtonWnd*  AutoButton;                // AutoButton
/*0x420*/ CButtonWnd*  AltStorageButton;          // AltStorageButton
/*0x428*/ CButtonWnd*  FindItemButton;            // FindItemButton
/*0x430*/ CButtonWnd*  DragonHoardButton;         // DragonHoard
/*0x438*/ CButtonWnd*  TradeskillDepotButton;     // TradeskillDepot
/*0x440*/ int          BankSize;
/*0x444*/

	CBankWnd(CXWnd*, CXStr);
	virtual ~CBankWnd();

	virtual int OnProcessFrame() override;
	virtual int PostDraw() override;
	virtual int WndNotification(CXWnd* pWnd, unsigned int uiMessage, void* pData) override;

	int GetNumBankSlots() const { return BankSize; }

	// private
	EQLIB_OBJECT long GetBankQtyFromCoinType(int);
	EQLIB_OBJECT void ClickedMoneyButton(int, int);
	EQLIB_OBJECT void UpdateMoneyDisplay();
};

//============================================================================
// CBarterWnd
//============================================================================

struct BarterInventoryItem
{
	int  ItemID;
	int  Charges;
	int  Qty;
	int  ItemIcon;
	char Name[EQ_MAX_NAME];
};

struct CompensationItem
{
	int  ItemID;
	int  Quantity;
	int  ItemIcon;
	char ItemName[EQ_MAX_NAME];
};

struct [[offsetcomments]] BarterBuyLine
{
/*0x00*/ int      LineIndex;
/*0x04*/ bool     Filled;
/*0x08*/ int      ItemSought;
/*0x0c*/ char     ItemName[EQ_MAX_NAME];
/*0x4c*/ int      ItemIcon;
/*0x50*/ int      ItemQuantity;
/*0x54*/ bool     Active;
/*0x58*/ uint32_t CoinPrice;
/*0x60*/ ArrayClass<CompensationItem> ItemPrice;
/*0x78*/ uint32_t BuyerPlayerZoneID;
/*0x7c*/ uint32_t BuyerPlayerUniqueID;
/*0x80*/ int      BuyerZoneID;
/*0x84*/ char     BuyerName[EQ_MAX_NAME];
/*0xc4*/
};

struct BarterSearchedItem
{
	char itemName[EQ_MAX_NAME];
	int  itemID;
	int  itemIcon;
	int  searchIndex;
};

// sizeof(CBarterWnd) == 0x4c8 // test 2022-02-07

class [[offsetcomments]] CBarterWnd : public CSidlScreenWnd
{
public:
	enum BuyLineListColumns
	{
		Column_Active = 0,
		Column_Icon,
		Column_Name,
		Column_Count,
		Column_Offering,
	};

/*0x2d0*/ CStmlWnd*                    pstmlStatus;               // StatusText
/*0x2d8*/ CListWnd*                    plistItems;                // ItemSearchList
/*0x2e0*/ CListWnd*                    plistInventory;            // InventoryList
/*0x2e8*/ CListWnd*                    plistCompensation;         // CompensationList
/*0x2f0*/ CListWnd*                    plistBuyLines;             // BuyLineList
/*0x2f8*/ CEditWnd*                    peditSearch;               // ItemSearchEdit
/*0x300*/ CButtonWnd*                  pbtnSearch;                // ItemSearchButton
/*0x308*/ CButtonWnd*                  pbtnAddItem;               // InventoryAddButton
/*0x310*/ CButtonWnd*                  pbtnRemoveItem;            // InventoryRemoveButton
/*0x318*/ CButtonWnd*                  pbtnPlat;                  // CompensationPlat
/*0x320*/ CButtonWnd*                  pbtnGold;                  // CompensationGold
/*0x328*/ CButtonWnd*                  pbtnSilver;                // CompensationSilver
/*0x330*/ CButtonWnd*                  pbtnCopper;                // CompensationCopper
/*0x338*/ CButtonWnd*                  pbtnCreate;                // BuyLineAddButton
/*0x340*/ CEditWnd*                    peditCount;                // CountEdit
/*0x348*/ CButtonWnd*                  pbtnRemoveLine;            // BuyLineRemoveButton
/*0x350*/ CButtonWnd*                  pbtnUpdateLine;            // BuyLineModifyButton
/*0x358*/ CButtonWnd*                  pbtnActiveLine;            // BuyLineActiveButton
/*0x360*/ CButtonWnd*                  pbtnActivateAllLines;      // BuyLineActivateAllButton
/*0x368*/ CButtonWnd*                  pbtnListLines;             // ListLinesButton
/*0x370*/ CButtonWnd*                  pbtnClearStatus;           // ClearStatusButton
/*0x378*/ CButtonWnd*                  pbtnStartStop;             // StartStopButton
/*0x380*/ CButtonWnd*                  pbtnOfflineMode;           // OfflineModeButton
/*0x388*/ CEditWnd*                    peditWelcome;              // WelcomeEdit
/*0x390*/ CButtonWnd*                  pbtnUpdateWelcome;         // UpdateWelcomeButton
/*0x398*/ CButtonWnd*                  pbtnRefreshInventory;      // InventoryRefreshButton
/*0x3a0*/ CLayoutWnd*                  pLayout;                   // Layout
/*0x3a8*/ CLayoutWnd*                  pStatusLayout;             // StatusLayout
/*0x3b0*/ uint32_t                     lastUpdateTime;
/*0x3b8*/ eqtime_t                     lastSearchTime;
/*0x3c0*/ uint32_t                     quantityRequest;
/*0x3c8*/ ArrayClass2<BarterInventoryItem> InventoryItems;
/*0x3e8*/ ArrayClass2<BarterSearchedItem> SearchedItems;
/*0x408*/ BarterBuyLine                CurrentBuyLine;
/*0x4d0*/
};

//============================================================================
// CBarterSearchWnd
//============================================================================

struct [[offsetcomments]] BarterBuyerSearchData
{
/*0x00*/ int      ZoneID;
/*0x04*/ uint32_t UniquePlayerID;
/*0x08*/ uint32_t ZonePlayerID;
/*0x0c*/ char     Name[EQ_MAX_NAME];
/*0x4c*/ };

// Size: 3c8

class [[offsetcomments]] CBarterSearchWnd : public CSidlScreenWnd
{
public:
	enum InventoryListColumns
	{
		Column_Icon = 0,
		Column_Name,
		Column_Count,

		Column_Max,
	};

/*0x2d0*/ HashTable<BarterBuyerSearchData> Buyers;
/*0x2e8*/ CListWnd*                    plistInventory;                 // InventoryList
/*0x2f0*/ CListWnd*                    plistBuyLines;                  // BuyLineList
/*0x2f8*/ CListWnd*                    plistDetails;                   // DetailsList
/*0x300*/ CEditWnd*                    peditSearch;                    // ItemSearchEdit
/*0x308*/ CButtonWnd*                  pbtnRefreshInventory;           // RefreshInventoryButton
/*0x310*/ CButtonWnd*                  pbtnSearch;                     // SearchButton
/*0x318*/ CButtonWnd*                  pbtnWelcome;                    // WelcomeButon
/*0x320*/ CButtonWnd*                  pbtnGreeting;                   // GreetingButton
/*0x328*/ CButtonWnd*                  pbtnFind;                       // FindButton
/*0x330*/ CButtonWnd*                  pbtnHide;                       // HideButton
/*0x338*/ CButtonWnd*                  pbtnBuyLineInspectItem;         // BuyLineInspectButton
/*0x340*/ CButtonWnd*                  pbtnCompensationInspectItem;    // CompensationInspectButton
/*0x348*/ CButtonWnd*                  pbtnCompensationPreviewItem;    // CompensationPreviewButton
/*0x350*/ CButtonWnd*                  pbtnSellButton;                 // SellButton
/*0x358*/ CButtonWnd*                  pbtnResetButton;                // ResetButton
/*0x360*/ CComboWnd*                   pcomboPlayersCombo;             // PlayerCombo
/*0x368*/ CLayoutWnd*                  pLayout;                        // Layout
/*0x370*/ CLayoutWnd*                  pMatchLayout;                   // BuyLineLayout
/*0x378*/ bool                         NeedsUpdate;
/*0x380*/ ArrayClass<BarterBuyLine>    BuyLines;
/*0x398*/ ArrayClass2<BarterInventoryItem> InventoryItems;
/*0x3b8*/ // more members
};

//============================================================================
// CBazaarSearchWnd
//============================================================================

struct BazaarTraderData
{
/*0x00*/ int      ZoneID;
/*0x04*/ uint32_t UniquePlayerID;
/*0x08*/ uint32_t SpawnID;
/*0x0c*/ char     Name[0x40];
/*0x4c*/
};

constexpr int MAX_BAZAAR_SEARCH_RESULTS = 200;

// size: 0xBC
struct [[offsetcomments]] BazaarSearchResults
{
	FORCE_SYMBOLS

/*0x00*/ char       sellerName[EQ_MAX_NAME];
/*0x40*/ char       itemName[EQ_MAX_NAME];
/*0x80*/ uint32_t   uniqueSellerID;
/*0x84*/ uint32_t   sellerID;
/*0x88*/ int        zoneID = 0;
/*0x8c*/ int        price = 0;
/*0x90*/ int        position = -1;
/*0x94*/ int        iconID = 0;
/*0x98*/ int        itemID = 0;
/*0x9c*/ int        count = 0;
/*0xa0*/ int        statNum = 0;
/*0xa4*/ float      statNumFloat = 0.f;
/*0xa8*/ EqItemGuid itemGuid;
/*0xbc*/
};

// CBazaarSearchWnd_size: 0x9720
class [[offsetcomments]] CBazaarSearchWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CBazaarSearchWnd(CXWnd*);
	virtual ~CBazaarSearchWnd();

	virtual void Activate() override;
	virtual int Draw() override;
	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;
	virtual void Deactivate() override;

	EQLIB_OBJECT void HandleBazaarMsg(char*, int);

	EQLIB_OBJECT char* GetPriceString(unsigned long);
	EQLIB_OBJECT void UpdatePlayerCombo();
	EQLIB_OBJECT void UpdatePlayerUpdateButton(bool);
	EQLIB_OBJECT void AddItemtoList(char*, unsigned long, char*, int, int);
	EQLIB_OBJECT void doQuery();
	EQLIB_OBJECT void SortItemList(int);
	EQLIB_OBJECT void UpdateComboButtons();

	//----------------------------------------------------------------------------
	// data members

/*0x02d4*/ uint8_t                     UnknownPadding[0x34];
/*0x0308*/ BazaarSearchResults         searchResults[MAX_BAZAAR_SEARCH_RESULTS];
/*0x95e8*/ HashTable<BazaarTraderData> Traders;
/*0x9600*/ CListWnd*                   pItemList;                        // BZR_ItemList
/*0x9608*/ CButtonWnd*                 pQueryButton;
/*0x9610*/ CButtonWnd*                 pWelcomeButton;
/*0x9618*/ CButtonWnd*                 pUpdatePlayerButton;
/*0x9620*/ CButtonWnd*                 pRequestItemButton;
/*0x9628*/ CButtonWnd*                 pRequestPreviewButton;
/*0x9630*/ CButtonWnd*                 pFindTraderButton;
/*0x9638*/ CButtonWnd*                 pHideTradersButton;
/*0x9640*/ CButtonWnd*                 pDefaultButton;
/*0x9648*/ CButtonWnd*                 pBuyButton;
/*0x9650*/ CLabelWnd*                  pItemNameLabel;
/*0x9658*/ CLabelWnd*                  pPlayersLabel;
/*0x9660*/ CLabelWnd*                  pItemSlotLabel;
/*0x9668*/ CLabelWnd*                  pStatSlotLabel;
/*0x9670*/ CLabelWnd*                  pRaceSlotLabel;
/*0x9678*/ CLabelWnd*                  pClassSlotLabel;
/*0x9680*/ CLabelWnd*                  pItemTypeLabel;
/*0x9688*/ CLabelWnd*                  pSearchResultLabel;
/*0x9690*/ CLabelWnd*                  pMaxPriceLabel;
/*0x9698*/ CLabelWnd*                  pMinPriceLabel;
/*0x96a0*/ CLabelWnd*                  pItemPrestigeLabel;
/*0x96a8*/ CLabelWnd*                  pItemAugmentLabel;
/*0x96b0*/ CComboWnd*                  pItemSlotCombobox;
/*0x96b8*/ CComboWnd*                  pStatSlotCombobox;
/*0x96c0*/ CComboWnd*                  pRaceSlotCombobox;
/*0x96c8*/ CComboWnd*                  pClassSlotCombobox;
/*0x96d0*/ CComboWnd*                  pItemTypeCombobox;
/*0x96d8*/ CComboWnd*                  pPlayersCombobox;
/*0x96e0*/ CComboWnd*                  pItemPrestigeCombobox;
/*0x96e8*/ CComboWnd*                  pItemAugmentCombobox;
/*0x96f0*/ CEditWnd*                   pItemNameInput;
/*0x96f8*/ CEditWnd*                   pMaxPriceInput;
/*0x9700*/ CEditWnd*                   pMinPriceInput;
/*0x9708*/ CEditWnd*                   pMaxLevelInput;
/*0x9710*/ CEditWnd*                   pMinLevelInput;
/*0x9718*/ CEditWnd*                   pMaxResultsPerTraderInput;
/*0x9720*/ BYTE                        Unknown0x92C8[0x8];
/*0x9728*/
};

inline namespace deprecated {
	using BAZAARSEARCHWND DEPRECATE("Use CBazaarSearchWnd instead of BAZAARSEARCHWND") = CBazaarSearchWnd;
	using PBAZAARSEARCHWND DEPRECATE("Use CBazaarSearchWnd* instead of PBAZAARSEARCHWND") = CBazaarSearchWnd*;
}

//============================================================================
// CBazaarWnd
//============================================================================

constexpr int MAX_BAZAAR_ITEMS = 200;

// size: 0xdf0
class [[offsetcomments]] CBazaarWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	inline ItemContainer& GetBazaarItems() { return bazaarItems; }

/*0x2d4*/ int                selectedSlot;
/*0x2d8*/ int                curBazaarIndex;
/*0x2dc*/ int                curItemPrice;
/*0x2e0*/ int                moneyBtnIndex;
/*0x2e8*/ ItemPtr            selectedItem;
/*0x2f8*/ CButtonWnd*        startButton;
/*0x300*/ CButtonWnd*        endButton;
/*0x308*/ CButtonWnd*        clearButton;
/*0x310*/ CButtonWnd*        setPriceButton;                    // BZW_SetPrice_Button
/*0x318*/ CButtonWnd*        listButton;
/*0x320*/ uint32_t           unknown0x25c;
/*0x328*/ CButtonWnd*        addButton;
/*0x330*/ CButtonWnd*        removeButton;
/*0x338*/ CButtonWnd*        moneyButton[4];                    // NumMoneySlotTypes
/*0x358*/ CButtonWnd*        offlineModeButton;
/*0x360*/ CStmlWnd*          bazaarText;
/*0x368*/ CInvSlotWnd*       bazaarSlots[MAX_BAZAAR_ITEMS];     // BZR_BazaarSlot%d
/*0x9a8*/ ItemContainer      bazaarItems;
/*0x9d0*/ uint32_t           myPrice[MAX_BAZAAR_ITEMS];
/*0xcf0*/ bool               bItemActive[MAX_BAZAAR_ITEMS];
/*0xdb8*/ char               iniFilename[64];
/*0xdf8*/ bool               bIsMerchant;
/*0xdfc*/
};

//============================================================================
// CBodyTintWnd
//============================================================================

class [[offsetcomments]] CBodyTintWnd : public CSidlScreenWnd, public WndEventHandler
{
public:
	CBodyTintWnd(CXWnd*);
	virtual ~CBodyTintWnd();

	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT char* BuildRBGTooltip(unsigned long, char* const);
	EQLIB_OBJECT unsigned long GetButtonTint(int);
	EQLIB_OBJECT unsigned long GetItemTint(int);
	EQLIB_OBJECT void GetReagentCount();
	EQLIB_OBJECT void GetTintChangeCount();
	EQLIB_OBJECT void ResetToOrigBodyTints();
	EQLIB_OBJECT void SetSlotLabels(bool);
	EQLIB_OBJECT void StoreModifiedBodyTints();
	EQLIB_OBJECT void UpdateLabelsAndButtons();
	EQLIB_OBJECT void UpdateLocalTintColor(unsigned long);
	EQLIB_OBJECT void UpdateMaterialTintLocal(unsigned long);
};

//============================================================================
// CBookWnd
//============================================================================

class CBookWnd : public CSidlScreenWnd
{
public:
	CBookWnd(CXWnd*);
	virtual ~CBookWnd();

	virtual int HandleKeyboardMsg(uint32_t, uint32_t, bool) override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT bool CheckBook(EQ_Note*);
	EQLIB_OBJECT void SetBook(char*);
	EQLIB_OBJECT void CleanPages();
	EQLIB_OBJECT void DisplayText();
	EQLIB_OBJECT void ProcessText();
	EQLIB_OBJECT void ShowButtons();
};

//============================================================================
// CBuffWindow
//============================================================================

enum BuffWindowType
{
	BuffWindowStandard,
	BuffWindowShortDuration,
};

// @sizeof(CBuffWindow) == 0x350 :: 2024-07-22 (live) @ 0x14018ca1f
constexpr size_t CBuffWindow_size = 0x350;

class [[offsetcomments]] CBuffWindow : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CBuffWindow(CXWnd*, BuffWindowType);
	virtual ~CBuffWindow();

	virtual CXSize GetMinSize(bool) const override;
	virtual int OnProcessFrame() override;
	virtual int PostDraw() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	BuffWindowPlayerBuffInfoWrapper GetBuffInfo(int buffIndex) const
	{
		if (buffIndex >= 0 && buffIndex < GetMaxBuffs())
			return BuffWindowPlayerBuffInfoWrapper(buffIndex, &Buffs[buffIndex]);

		return BuffWindowPlayerBuffInfoWrapper(-1, nullptr);
	}

	BuffWindowPlayerBuffInfoWrapper GetBuffInfoBySpellID(int spellID) const
	{
		int buffIndex = 0;
		for (const PlayerBuffInfo& pbi : Buffs)
		{
			if (pbi.SpellID == spellID)
				return BuffWindowPlayerBuffInfoWrapper(buffIndex, &pbi);

			++buffIndex;
		}

		return BuffWindowPlayerBuffInfoWrapper(-1, nullptr);
	}

	int GetTotalBuffCount() const
	{
		int count = 0;
		for (const PlayerBuffInfo& pbi : Buffs)
		{
			if (pbi.SpellID > 0)
				++count;
		}

		return count;
	}

	int GetMaxBuffs() const { return std::min(lastEffectSlot - firstEffectSlot, Buffs.GetSize()); }

	int GetBuff(int buffIndex) const { return GetBuffInfo(buffIndex).GetSpellID(); }
	int GetBuffTimer(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffTimer(); }
	const char* GetCasterName(int buffIndex) const { return GetBuffInfo(buffIndex).GetCaster(); }
	CButtonWnd* GetBuffButton(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffButton(); }
	CTextureAnimation* GetBuffIcon(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffIcon(); }
	CTextObjectInterface* GetTimeRemainingText(int buffIndex) const { return GetBuffInfo(buffIndex).GetTimeRemainingText(); }
	CTextObjectInterface* GetCounterText(int buffIndex) const { return GetBuffInfo(buffIndex).GetCounterText(); }
	CTextObjectInterface* GetLimitUseText(int buffIndex) const { return GetBuffInfo(buffIndex).GetLimitUseText(); }

	using BuffIterator = PlayerBuffInfoWrapper::Iterator<BuffWindowPlayerBuffInfoWrapper>;
	eqlib::IteratorRange<BuffIterator> GetBuffRange() const { return eqlib::make_iterator_range(BuffIterator(Buffs, 0), BuffIterator(Buffs, Buffs.GetSize())); }

	#pragma region Deprecated Accessors
	//
	// Deprecated accessors
	//

	// spellIds -> GetBuff()
	DEPRECATE("Use GetBuff(index) or GetBuffInfo(index) insetad of spellIds[index]")
	int get_DeprecatedBuff(int buffIndex) const { return GetBuffInfo(buffIndex).GetSpellID(); }
	__declspec(property(get = get_DeprecatedBuff)) int spellIds[];

	// BuffId -> GetBuff()
	DEPRECATE("Use GetBuff(index) or GetBuffInfo(index) insetad of BuffId[index]")
		int get_DeprecatedBuff2(int buffIndex) const { return GetBuffInfo(buffIndex).GetSpellID(); }
	__declspec(property(get = get_DeprecatedBuff2)) int BuffId[];

	// buffTimers -> GetBuffTimer()
	DEPRECATE("Use GetBuffTimer(index) or GetBuffInfo(index) instead of buffTimers[index]")
	int get_DeprecatedBuffTimer(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffTimer(); }
	__declspec(property(get = get_DeprecatedBuffTimer)) int buffTimers[];

	// BuffTimer -> GetBuffTimer()
	DEPRECATE("Use GetBuffTimer(index) or GetBuffInfo(index) instead of BuffTimer[index]")
	int get_DeprecatedBuffTimer2(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffTimer(); }
	__declspec(property(get = get_DeprecatedBuffTimer2)) int BuffTimer[];

	// whoCastHash -> Removed
	DEPRECATE("whoCastHash was removed. Use GetBuffInfoBySpellID(spellID).GetCasterName() instead")
	HashTable<CXStr> get_DeprecatedWhoCastHash() const { return HashTable<CXStr>(); }
	__declspec(property(get = get_DeprecatedWhoCastHash)) HashTable<CXStr> whoCastHash;

	// pBuffButtons -> GetBuffButton()
	DEPRECATE("Use GetBuffButton(index) or GetBuffInfo(index) isntead of pBuffButtons[index]")
	CButtonWnd* get_DeprecatedBuffButton(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffButton(); }
	__declspec(property(get = get_DeprecatedBuffButton)) CButtonWnd* pBuffButtons[];

	// pBuff -> GetBuffButton()
	DEPRECATE("Use GetBuffButton(index) or GetBuffInfo(index) isntead of pBuff[index]")
	CButtonWnd* get_DeprecatedBuffButton2(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffButton(); }
	__declspec(property(get = get_DeprecatedBuffButton2)) CButtonWnd* pBuff[];

	ALT_VMEMBER_GETTER_DEPRECATED(int, lastEffectSlot, MaxLongBuffs, "CBuffWindow: Use lastEffectSlot instead of MaxLongBuffs");
	ALT_VMEMBER_GETTER_DEPRECATED(int, maxBuffButtons, MaxShortBuffs, "CBuffWindow: Use maxBuffButtons instead of MaxShortBuffs");
	#pragma endregion

	//----------------------------------------------------------------------------
	// data members

/*0x2d4*/ bool                  bOldIconArrangement;
/*0x2d8*/ CTextureAnimation*    ptaBlueIconBackground;
/*0x2e0*/ CTextureAnimation*    ptaRedIconBackground;
/*0x2e8*/ CTextureAnimation*    ptaYellowIconBackground;
/*0x2f0*/ SoeUtil::Array<BuffWindowPlayerBuffInfo> Buffs;
/*0x308*/ SoeUtil::Map<int, CButtonWnd*> ButtonMap;
/*0x320*/ uint32_t              nextRefreshTime;
/*0x324*/ int                   initWindowWidth;
/*0x328*/ int                   initWindowHeight;
/*0x32c*/ BuffWindowType        buffWindowType;
/*0x330*/ int                   firstEffectSlot;
/*0x334*/ int                   lastEffectSlot;                  // was: MaxLongBuffs 0x2a (NUM_LONG_BUFFS)
/*0x338*/ int                   maxBuffButtons;                  // was: MaxShortBuffs 0x37 (NUM_SHORT_BUFFS)
/*0x33c*/ int                   contextMenuId;
/*0x340*/ CXWnd*                selectedButtonWnd;               // this field doesn't always appear to be initialize
/*0x348*/ bool                  updatedMenuItems;
/*0x34c*/ int                   lastBuffRefreshTime;
/*0x350*/
};

SIZE_CHECK(CBuffWindow, CBuffWindow_size);

//============================================================================
// CCastingWnd
//============================================================================

class CCastingWnd : public CSidlScreenWnd
{
public:
	CCastingWnd(class CXWnd*);
	virtual ~CCastingWnd();
};

//============================================================================
// CCastSpellWnd
//============================================================================

class CSpellGemWnd;

// size: 0x3e0
class [[offsetcomments]] CCastSpellWnd : public CSidlScreenWnd, public PopDialogHandler, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CCastSpellWnd(CXWnd*);
	~CCastSpellWnd();

	virtual int OnProcessFrame() override;
	virtual int PostDraw() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	static EQLIB_OBJECT void Unmemorize(int);
	static EQLIB_OBJECT void UnmemorizeList(int*, int);
	EQLIB_OBJECT void ForgetMemorizedSpell(int);
	EQLIB_OBJECT void HandleSpellInfoDisplay(CXWnd*);
	EQLIB_OBJECT void HandleSpellLeftClick(int);
	EQLIB_OBJECT void HandleSpellRightClick(int);
	EQLIB_OBJECT void KeyMapUpdated();
	EQLIB_OBJECT void RecacheCategorizedSpells();
	EQLIB_OBJECT void RecacheLoadoutContextMenu();
	EQLIB_OBJECT void SpellBookDeactivating();
	EQLIB_OBJECT bool IsBardSongPlaying();
	EQLIB_OBJECT unsigned long GetSpellGemColor(EQ_Spell*);
	EQLIB_OBJECT void CategorizeSpell(int, int, int, int);
	EQLIB_OBJECT void ClearSpellCategories();
	EQLIB_OBJECT void Init();
	EQLIB_OBJECT void MakeSpellSelectMenu();
	EQLIB_OBJECT void RefreshSpellGemButtons();
	EQLIB_OBJECT void UpdateSpellGems(int);
	EQLIB_OBJECT void UpdateSpellGemTooltips(int);

	struct SpellCategoryData
	{
		int                             id;
		int                             level;
	};

	struct SpellCategoryMinor
	{
		int                             id;
		char                            name[64];
		ArrayClass<SpellCategoryData*>  spells;
	};

	struct SpellCategoryMajor
	{
		int                             id;
		char                            name[64];
		ArrayClass<SpellCategoryMinor*> subcategories;
	};

	//----------------------------------------------------------------------------
	// data members

/*0x2e0*/ CPopDialogWnd*       pCurrentPop;
/*0x2e8*/ uint32_t             lastRefresh;
/*0x2ec*/ bool                 bHorizontal;
/*0x2f0*/ CSpellGemWnd*        SpellSlots[NUM_SPELL_GEMS];     // CSPW_Spell%d
/*0x360*/ CButtonWnd*          pBtnSpellBook;                  // CSPW_SpellBook
/*0x368*/ CButtonWnd*          pBtnSpellBookH;                 // CSPW_SpellBookH
/*0x370*/ CButtonWnd*          pBtnSpellBookV;                 // CSPW_SpellBookV
/*0x378*/ CXWnd*               pHorizontalTemplate;            // CSPW_HorizontalOrientationTemplate
/*0x380*/ CXWnd*               pVerticalTemplate;              // CSPW_VerticalOrientationTemplate
/*0x388*/ CTileLayoutWnd*      pSpellGemLayout;                // CSPW_SpellGemLayout
/*0x390*/ CXWnd*               pHorizontalBook;                // CSPW_HorizontalOrientationBook
/*0x398*/ CXWnd*               pVerticalBook;                  // CSPW_VerticalOrientationBook
/*0x3a0*/ ArrayClass<SpellCategoryMajor*> CategorizedSpells;
/*0x3b8*/ int                  SpellSelectMenuID;
/*0x3bc*/ int                  GemMemming;
/*0x3c0*/ int                  LoadoutMenu;
/*0x3c4*/ int                  SaveLoadoutIndex;
/*0x3c8*/ CContextMenu*        pMainMenu;
/*0x3d0*/ CContextMenu*        pLoadMenu;
/*0x3d8*/ CContextMenu*        pSaveMenu;
/*0x3e0*/ CContextMenu*        pDeleteMenu;
/*0x3e8*/
};

inline namespace deprecated {
	using EQCASTSPELLWINDOW DEPRECATE("Use CCastSpellWnd instead of EQCASTSPELLWINDOW") = CCastSpellWnd;
	using PEQCASTSPELLWINDOW DEPRECATE("Use CCastSpellWnd* instead of PEQCASTSPELLWINDOW") = CCastSpellWnd*;
}

//============================================================================
// CCharacterCreation
//============================================================================

class CCharacterCreation : public CSidlScreenWnd
{
public:
	CCharacterCreation(CXWnd*);
	virtual ~CCharacterCreation();

	virtual int Draw() override;
	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT void ActivateScreen(int);
	EQLIB_OBJECT void DoBackButton();
	EQLIB_OBJECT void DoNextButton();
	EQLIB_OBJECT void HandleNameApprovalResponse(int);
	EQLIB_OBJECT void NameGenerated(char*);
	EQLIB_OBJECT bool IsLackingExpansion(bool, bool);
	EQLIB_OBJECT char* GetCharCreateText(char*, int, int, int, int);
	EQLIB_OBJECT int GetClassButtonIndex(int);
	EQLIB_OBJECT int GetRaceButtonIndex(int);
	EQLIB_OBJECT void DoStatButton(int);
	EQLIB_OBJECT void FinalizeNewPCAndSendToWorld();
	EQLIB_OBJECT void Init();
	EQLIB_OBJECT void InitNewPC();
	EQLIB_OBJECT void InitStartingCities();
	EQLIB_OBJECT void InitStats(bool);
	EQLIB_OBJECT void RandomizeCharacter(bool, bool);
	EQLIB_OBJECT void RandomizeFacialFeatures();
	EQLIB_OBJECT void SelectedClassButton(int);
	EQLIB_OBJECT void SelectedRaceButton(int);
	EQLIB_OBJECT void SelectStartingCity(int);
	EQLIB_OBJECT void SetNewPCDeityFromBtnIndex(int);
	EQLIB_OBJECT void SetPlayerAppearanceFromNewPC();
	EQLIB_OBJECT void SubmitNameToWorld();
	EQLIB_OBJECT void UpdatePlayerFromNewPC();
	EQLIB_OBJECT void UpdateScreenZeroButtons(bool);
};

//============================================================================
// CCharacterListWnd
//============================================================================

class CCharacterListWnd : public CSidlScreenWnd, public PopDialogHandler, public WndEventHandler
{
public:
	CCharacterListWnd(CXWnd*);
	virtual ~CCharacterListWnd();

	virtual int Draw() override;
	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT int IsEmptyCharacterSlot(int);
	EQLIB_OBJECT int IsValidCharacterSelected();
	EQLIB_OBJECT int NumberOfCharacters();
	EQLIB_OBJECT unsigned char IsEvil(int, int, int);
	EQLIB_OBJECT void DeleteCharacter();
	EQLIB_OBJECT void EnterExplorationMode();
	EQLIB_OBJECT void EnterWorld();
	EQLIB_OBJECT void LeaveExplorationMode();
	EQLIB_OBJECT void Quit();
	EQLIB_OBJECT void UpdateList(bool bForceUpdate = false);
	EQLIB_OBJECT void SelectCharacter(int charindex, bool bSwitchVisually = true, bool bForceUpdate = false /*dont mess with this*/);
	EQLIB_OBJECT void SetLoadZonePlayerLocation();
	EQLIB_OBJECT void SwitchModel(PlayerClient*, unsigned char, int, unsigned char, unsigned char);
	EQLIB_OBJECT void UpdateButtonNames();
	EQLIB_OBJECT void CreateExplorationModePlayers();
	EQLIB_OBJECT void EnableButtons(bool);
	EQLIB_OBJECT void EnableEqMovementCommands(bool);
	EQLIB_OBJECT void Init();
	EQLIB_OBJECT void RemoveExplorationModePlayers();
	EQLIB_OBJECT void SetLocationByClass(PlayerClient*, bool, int);
};

//============================================================================
// CChatWindowManager
//============================================================================

constexpr int MAX_CHAT_WINDOWS = 32;

// size: 0x700
class [[offsetcomments]] CChatWindowManager
{
public:
	//EQLIB_OBJECT CChatWindowManager();
	//EQLIB_OBJECT ~CChatWindowManager();

	EQLIB_OBJECT COLORREF GetRGBAFromIndex(int);
	EQLIB_OBJECT int InitContextMenu(CChatWindow*);
	EQLIB_OBJECT void FreeChatWindow(CChatWindow*);
	EQLIB_OBJECT CChatWindow* GetLockedActiveChatWindow() const;   // might be returning CChatContainerWindow now
	EQLIB_OBJECT void SetLockedActiveChatWindow(CChatWindow*);
	EQLIB_OBJECT void CreateChatWindow(CXWnd* pParentWnd, int ID, char* Name, int Language, int DefaultChannel,
		int ChatChannel, char* szTellTarget, int FontStyle, bool bScrollbar, bool bHighLight, COLORREF HighlightColor);

	//EQLIB_OBJECT CChatWindow* GetActiveChatWindow();
	//EQLIB_OBJECT CChatWindow* GetChannelMap(int);
	//EQLIB_OBJECT CXStr GetAllVisibleText(CXStr);
	//EQLIB_OBJECT int GetChannelFromColor(int);
	//EQLIB_OBJECT void Activate();
	//EQLIB_OBJECT void AddText(CXStr, int);
	//EQLIB_OBJECT void ClearChannelMap(int);
	//EQLIB_OBJECT void ClearChannelMaps(CChatWindow*);
	//EQLIB_OBJECT void CreateChatWindow();
	//EQLIB_OBJECT void Deactivate();
	//EQLIB_OBJECT void LoadChatInis();
	//EQLIB_OBJECT void Process();
	//EQLIB_OBJECT void SetActiveChatWindow(CChatWindow*);
	//EQLIB_OBJECT void SetChannelMap(int, CChatWindow*);
	//EQLIB_OBJECT void UpdateContextMenus(CChatWindow*);
	//EQLIB_OBJECT void UpdateTellMenus(CChatWindow*);

/*0x000*/ CChatContainerWindow* ChatContainerWindows[MAX_CHAT_WINDOWS];
/*0x100*/ SoeUtil::List<CChatWindow*> ChatWindowList;
/*0x120*/ CChatWindow*       ChatWindows[MAX_CHAT_WINDOWS];
/*0x220*/ int                Unknown7;
/*0x224*/ int                NumWindows;
/*0x228*/ int                LockedWindow;
/*0x22c*/ int                ActiveWindow;
/*0x230*/ int                CurrentActive;                      // CurrentActive... CChat::GetActiveChatWindow
/*0x234*/ int                LockedActive;                       // LockedActive... CChatManager__GetLockedActiveChatWindow_x
/*0x238*/ CChatWindow*       ChannelMap[NUM_CHAT_CHANNELS];      // channel map
/*0x580*/ CContextMenu*      pMainContextMenu;
/*0x588*/ int                ScrollbarIndex;
/*0x58c*/ int                UnknownContextMenuID1[6];
/*0x5a8*/ CContextMenu*      pLanguageContextMenu;
/*0x5b0*/ int                LanguageMenuIndex;
/*0x5b8*/ CContextMenu*      pFilterContextMenu;
/*0x5c0*/ int                FilterMenuIndex;
/*0x5c4*/ int                ChatChannelFilterMenuIndex;
/*0x5c8*/ int                MeleeFilterSubMenuIndex;
/*0x5cc*/ int                SpellsFilterSubMenuIndex;
/*0x5d0*/ CContextMenu*      pMeleeFilterContextMenu;
/*0x5d8*/ int                MeleeFilterMenuIndex;
/*0x5e0*/ CContextMenu*      pSpellsFilterContextMenu;
/*0x5e8*/ int                SpellsMenuIndex;
/*0x5f0*/ CContextMenu*      pChatChannelFilterContextMenu;
/*0x5f8*/ int                ChannelMenuIndex;
/*0x600*/ CContextMenu*      pDefaultChannelContextMenu;

	// data members beyond this point are not validated and likely incorrect
/*0x608*/ int                DefaultChannelMenu;
/*0x60c*/ int                DefaultChannelMenu2;
/*0x610*/ CContextMenu*      pCM_ChatChannelDefChan;
/*0x618*/ int                ChatChannelDefChanIndex;
/*0x620*/ CContextMenu*      pCM_YourHitsMenu;
/*0x628*/ int                YourHitsMenuIndex;
/*0x630*/ CContextMenu*      pCM_YourMissesMenu;
/*0x638*/ int                YourMissesMenuindex;
/*0x640*/ CContextMenu*      pCM_YouBeingHitMenu;
/*0x648*/ int                YouBeingHitMenuindex;
/*0x650*/ CContextMenu*      pCM_OthersHitsMenu;
/*0x658*/ int                OthersHitsMenuindex;
/*0x660*/ CContextMenu*      pCM_OthersMissesMenu;
/*0x668*/ int                OthersMissesMenuindex;
/*0x670*/ CContextMenu*      pCM_AllContextMenu;
/*0x678*/ int                AllContextMenuindex;
/*0x680*/ CContextMenu*      pCM_HitModesMenu;
/*0x688*/ int                HitModesMenuindex;
/*0x690*/ CContextMenu*      pCM_ReplyToMenu;
/*0x698*/ int                ReplyToMenuindex;
/*0x6a0*/ CContextMenu*      pCM_TellFriendMenu;
/*0x6a8*/ int                TellFriendMenuindex;
/*0x6b0*/ CContextMenu*      pCM_TellRaidmemberMenu;
/*0x6b8*/ int                TellRaidmemberMenuindex;
/*0x6bc*/ int                ReplyToSubIndex;
/*0x6c0*/ int                TellFriendSubIndex;
/*0x6c4*/ int                TellRaidmemberSubIndex;
/*0x6c8*/ int                HitModes[MAX_HITMODES];
/*0x6e8*/ int                DefaultChannel;
/*0x6f0*/ CContextMenu*      pRandomFilterContextMenu;              // 0x680
/*0x6f8*/ int                RandomFilterIndex;
/*0x6fc*/ int                RandomFilterSubIndex;
/*0x700*/ CContextMenu*      pEnvironmentalDamageFilterContextMenu; // 0x690
/*0x708*/ int                EnvironmentalDamageIndex;
/*0x70c*/ int                EnvironmentalDamageSubIndex;
/*0x710*/ CContextMenu*      pDamageShieldsFilterContextMenu;      // 0x6a0
/*0x718*/ int                DamageShieldsFilterIndex;
/*0x71c*/ int                DamageShieldsFilterSubIndex;
/*0x720*/ CContextMenu*      pCM_BeneficialSpellsFilterMenu;
/*0x728*/ int                BeneficialSpellsFilteIndex;
/*0x72c*/
};

//============================================================================
// CChatWindow
//============================================================================

class [[offsetcomments]] CChatContainerWindow : public CSidlScreenWnd
{
	FORCE_SYMBOLS

public:
	EQLIB_OBJECT CChatContainerWindow(CXWnd* parent);
	EQLIB_OBJECT virtual ~CChatContainerWindow();

};

//============================================================================
// CChatWindow
//============================================================================

class [[offsetcomments]] CChatWindow : public CSidlScreenWnd
{
	FORCE_SYMBOLS

public:
	EQLIB_OBJECT CChatWindow(CXWnd* parent);
	EQLIB_OBJECT virtual ~CChatWindow();

	//----------------------------------------------------------------------------
	// virtuals

	virtual int Draw() override;
	virtual int HandleRButtonDown(const CXPoint&, uint32_t) override;
	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd* sender, uint32_t message, void* data) override;
	virtual int OnSetFocus(CXWnd* wnd) override;
	virtual int OnKillFocus(CXWnd* wnd) override;
	virtual void Deactivate() override;

	EQLIB_OBJECT void Clear();
	EQLIB_OBJECT void AddHistory(CXStr Text);
	EQLIB_OBJECT CEditWnd* GetInputWnd() { return InputWnd; }
	EQLIB_OBJECT CStmlWnd* GetOutputWnd() { return OutputWnd; }

	//EQLIB_OBJECT CXStr GetInputText();
	//EQLIB_OBJECT void AddOutputText(CXStr, int);
	//EQLIB_OBJECT void HistoryBack();
	//EQLIB_OBJECT void HistoryForward();
	//EQLIB_OBJECT void PageDown();
	//EQLIB_OBJECT void PageUp();
	//EQLIB_OBJECT void SetChatFont(int);

/*0x2d0*/ CChatWindowManager* ChatManager;
/*0x2d8*/ CEditWnd*    InputWnd;
/*0x2e0*/ CStmlWnd*    OutputWnd;
/*0x2e8*/ int          ChatChannel;
/*0x2ec*/ int          ChatChannelIndex;
/*0x2f0*/ char         TellTarget[0x40];
/*0x330*/ int          Language;
/*0x334*/ bool         bIsMainChat;
/*0x335*/ bool         bIsTellWnd;
/*0x338*/ int          TimestampFormat;
/*0x33c*/ COLORREF     TimestampColor;
/*0x340*/ bool         bTimestampMatchChatColor;
/*0x348*/ CXStr        CommandHistory[0x28];
/*0x488*/ int          HistoryIndex;
/*0x48c*/ int          HistoryLastShown;
/*0x490*/ int          FontSize;                 // style
/*0x494*/ int          AlwaysChathereIndex;      // menu
/*0x498*/ int          NamesContextMenu;         // guess
/*0x49c*/ int          ContextMenuID;            // also a guess
/*0x4a0*/ int          ContextMenuSubID[0xa];    // this is not correct but ill fix it later.
/*0x4c8*/
};

inline namespace deprecated {
	using EQCHATWINDOW DEPRECATE("Use CChatWindow instead of EQCHATWINDOW") = CChatWindow;
	using PEQCHATWINDOW DEPRECATE("Use CChatWindow* instead of PEQCHATWINDOW") = CChatWindow*;
}

//============================================================================
// CColorPickerWnd
//============================================================================

class [[offsetcomments]] CColorPickerWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CColorPickerWnd(CXWnd* pwndParent);

	virtual bool AboutToHide() override;
	virtual int WndNotification(CXWnd* pwndSender, uint32_t Msg, void* pData) override;
	virtual void OnWndNotification() override;

	EQLIB_OBJECT void CheckMaxEditWnd();
	EQLIB_OBJECT void SetCurrentColor(unsigned long);
	EQLIB_OBJECT void SetTemplateColor(int, unsigned long);
	EQLIB_OBJECT void UpdateCurrentColor();
	EQLIB_OBJECT void UpdateEditWndFromSlider(CSliderWnd* pSlider, CEditWnd* pEdit, int* value);
	EQLIB_OBJECT void UpdateSliderFromEditWnd(CSliderWnd* pSlider, CEditWnd* pEdit, int* value);
	EQLIB_OBJECT int Open(CXWnd* pwndCaller, D3DCOLOR CurrentColor = 0x00FFFFFF);

	//----------------------------------------------------------------------------
	// data members

/*0x2d4*/ int                ContextMenuIndex;
/*0x2d8*/ int                MaxSliderValue;
/*0x2e0*/ CXWnd*             pwndCaller;
/*0x2e8*/ CButtonWnd*        ColorButton[16];
/*0x368*/ int                RedSliderValue;
/*0x370*/ CSliderWnd*        RedSlider;
/*0x378*/ CEditWnd*          RedSliderInputEdit;
/*0x380*/ int                GreenSliderValue;
/*0x388*/ CSliderWnd*        GreenSlider;
/*0x390*/ CEditWnd*          GreenSliderInputEdit;
/*0x398*/ int                BlueSliderValue;
/*0x3a0*/ CSliderWnd*        BlueSlider;
/*0x3a8*/ CEditWnd*          BlueSliderInputEdit;
/*0x3b0*/ CButtonWnd*        AcceptButton;
/*0x3b8*/
};

//============================================================================
// CCombatAbilityWnd
//============================================================================

class CCombatAbilityWnd : public CSidlScreenWnd
{
public:
	CCombatAbilityWnd(CXWnd*);
	virtual ~CCombatAbilityWnd();
};

//============================================================================
// CCombatSkillsSelectWnd
//============================================================================

class CCombatSkillsSelectWnd : public CSidlScreenWnd, public WndEventHandler
{
public:
	CCombatSkillsSelectWnd(CXWnd*);
	virtual ~CCombatSkillsSelectWnd();

	EQLIB_OBJECT bool ShouldDisplayThisSkill(int);
};

//============================================================================
// CCompassWnd
//============================================================================

// Size: 0x30 @ 0x575119 2021-05-14 (live)
struct [[offsetcomments]] CompassLineSource
{
/*0x00*/ uint32_t     Red;
/*0x04*/ uint32_t     Green;
/*0x08*/ uint32_t     Blue;
/*0x0c*/ float        Y;
/*0x10*/ float        X;
/*0x14*/ float        Z;
/*0x18*/ bool         ShowLine;
/*0x1c*/ DWORD        Unknown0x1C;
/*0x20*/ DWORD        Unknown0x20;
/*0x24*/ DWORD        Unknown0x24;
/*0x28*/ DWORD        Unknown0x28;
/*0x2c*/ DWORD        Unknown0x2C;
/*0x30*/
};

inline namespace deprecated {
	using COMPASSDATA DEPRECATE("Use CompassLineSource instead of COMPASSDATA") = CompassLineSource;
	using PCOMPASSDATAINFO DEPRECATE("Use CompassLineSource* instead of PCOMPASSDATAINFO") = CompassLineSource*;
}

// size 0x2a0 @ 0x56fda5 2021-05-14 (live)
class [[offsetcomments]] CCompassWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CCompassWnd(CXWnd*);
	virtual ~CCompassWnd();

	virtual int Draw() override;
	virtual int OnProcessFrame() override;

	EQLIB_OBJECT void SenseHeading();
	EQLIB_OBJECT void PickNewTarget();
	EQLIB_OBJECT void SetSpeed();

	//----------------------------------------------------------------------------
	// data members

/*0x2d8*/ CStaticAnimationTemplate*     pStrip1;
/*0x2e0*/ CStaticAnimationTemplate*     pStrip2;
/*0x2e8*/ int                           Unknown0x244;
/*0x2ec*/ int                           offset;
/*0x2f0*/ int                           timestamp;
/*0x2f8*/ double                        speed;
/*0x300*/ int                           timer;
/*0x304*/ CompassLineSource             lineAdventure;
/*0x338*/ ArrayClass<CompassLineSource*> lineData;
/*0x350*/
};

inline namespace deprecated {
	using EQCOMPASSWINDOW DEPRECATE("Use CCompassWnd instead of EQCOMPASSWINDOW") = CCompassWnd;
	using PEQCOMPASSWINDOW DEPRECATE("Use CCompassWnd* instead of PEQCOMPASSWINDOW") = CCompassWnd*;
}

//============================================================================
// CConfirmationDialog
//============================================================================

// This is actually CPopDialogWnd.
// TODO: Rename this
class [[offsetcomments]] CConfirmationDialog : public CSidlScreenWnd
{
	FORCE_SYMBOLS

public:
	CConfirmationDialog(CXWnd*);
	virtual ~CConfirmationDialog();

	virtual int Draw() override;
	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	// probably wrong
	virtual void Activate(PopDialogHandler* handler, int msg, const char* text,
		int, int, int, int);

	// this is all invalid
	//EQLIB_OBJECT void HandleButtonNoPressed();
	//EQLIB_OBJECT void HandleButtonOkPressed();
	//EQLIB_OBJECT void HandleButtonYesPressed();
	//EQLIB_OBJECT void ProcessNo(int);
	//EQLIB_OBJECT void ProcessYes();
	//EQLIB_OBJECT void SetNameApprovalData(char*, char*, int, int, char*);
	//EQLIB_OBJECT void ExpireCurrentDialog();
	//EQLIB_OBJECT void ResetFocusOnClose();

	// Data members
/*0x2d0*/ CStmlWnd*    OutputText;
/*0x2d8*/ CButtonWnd*  pYesButton;
/*0x2e0*/ CButtonWnd*  pNoButton;
/*0x2e8*/ CButtonWnd*  pCancelButton;
/*0x2f0*/ CButtonWnd*  pOKButton;
/*0x2f8*/ BYTE         Unknown0x170[0x18]; // fixme x64
/*0x310*/
};

//============================================================================
// CContainerWnd
//============================================================================

class [[offsetcomments]] CContainerWnd : public CSidlScreenWnd
{
	FORCE_SYMBOLS

public:
	CContainerWnd(CXWnd*);
	virtual ~CContainerWnd();

	virtual int OnProcessFrame() override;
	virtual int PostDraw() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT void CheckCloseable();
	EQLIB_OBJECT void SetContainer(const ItemPtr& pContainer, const ItemGlobalIndex& location);
	EQLIB_OBJECT bool ContainsNoDrop();
	EQLIB_OBJECT void HandleCombine();

	// TODO: Fix for inaccuracies
/*0x2d0*/ ItemPtr       Container;
/*0x2e0*/ ItemGlobalIndex Location;
/*0x2f0*/ VeArray<CInvSlotWnd*> InvSlotWnds;
/*0x308*/ CButtonWnd*   pCombineButton;
/*0x310*/ CButtonWnd*   pDoneButton;
/*0x318*/ CButtonWnd*   pStandardDoneButton;
/*0x320*/ CButtonWnd*   pCombineDoneButton;
/*0x328*/ CButtonWnd*   pContainerIcon;
/*0x330*/ CButtonWnd*   pContainerStandardIcon;
/*0x338*/ CButtonWnd*   pContainerCombineIcon;
/*0x340*/ CTextureAnimation* pIconAnimation;
/*0x348*/ CLabel*       ContainerLabel;
/*0x350*/ CInvSlotWnd*  pContainerSlotTemplate;
/*0x358*/ CXWnd*        pStandardItems;
/*0x360*/ CXWnd*        pStandardItemsWithDone;
/*0x368*/ CXWnd*        pCombineItems;
/*0x370*/ CStmlWnd*     pStandardSize;
/*0x378*/ CStmlWnd*     pCombineSize;
/*0x380*/ bool          bCombineValid;
/*0x381*/ bool          bUserCloseable;
/*0x384*/ int           ContainerType;            // classic = 0, standard = 1, combine = 2
/*0x388*/ int           IndexDoneButton;
/*0x390*/ CContextMenu* ContextMenu;
/*0x398*/

	DEPRECATE("CContainerWnd: Use Container instead of pContents")
	inline ItemClient* get_pContents() { return Container.get(); }
	__declspec(property(get = get_pContents)) ItemClient* pContents;
};

inline namespace deprecated {
	using EQCONTAINERWINDOW DEPRECATE("Use CContainerWnd instead of EQCONTAINERWINDOW") = CContainerWnd;
	using PEQCONTAINERWINDOW DEPRECATE("Use CContainerWnd* instead of PEQCONTAINERWINDOW") = CContainerWnd*;
}

constexpr int MAX_CONTAINERS = 43;

class [[offsetcomments]] CContainerMgr
{
	FORCE_SYMBOLS

public:
	CContainerMgr();
	virtual ~CContainerMgr();

	inline ItemPtr GetWorldContainerItem() const { return WorldContainer; };

	// Retrieves a window for the given item container, if it is active.
	EQLIB_OBJECT CContainerWnd* GetWindowForItem(const ItemPtr& pContainer) const;

	EQLIB_OBJECT bool CloseAllContainers();
	EQLIB_OBJECT void ClearWorldContainerItems();
	EQLIB_OBJECT void CloseContainer(const ItemPtr& pContainer, bool deleteWindow = true);
	EQLIB_OBJECT void OpenContainer(const ItemPtr& pContainer, const ItemGlobalIndex& location, bool allowTradeskill = true);
	EQLIB_OBJECT void OpenWorldContainer(const ItemPtr&, unsigned long);
	EQLIB_OBJECT void Process();
	EQLIB_OBJECT void SetWorldContainerItem(const ItemPtr&, int);
	EQLIB_OBJECT CContainerWnd* GetFreeContainerWnd();
	EQLIB_OBJECT void OpenExperimentContainer(const ItemPtr& pCont, const ItemGlobalIndex& Location);

	//----------------------------------------------------------------------------
	// data members

/*0x008*/ CContainerWnd*     pContainerWnds[MAX_CONTAINERS];
/*0x160*/ ArrayClass<CContainerWnd*> ContainerWndsToDelete;
/*0x178*/ ItemPtr            WorldContainer;
/*0x188*/ DWORD              WorldContainerSerialNumber;
/*0x18c*/ int                WorldContainerRealEstateID;
/*0x190*/ int                WorldContainerRealEstateItemID;
/*0x194*/ DWORD              Timer;
/*0x198*/ bool               bShowDone;
/*0x19c*/

	ItemClient* getter_pWorldContainer() { return WorldContainer.get(); }
	__declspec(property(get = getter_pWorldContainer)) ItemClient* pWorldContainer;
};

//============================================================================
// CContextMenuManager
//============================================================================

constexpr int MAX_CONTEXT_MENU_DEPTH = 8;
constexpr int MAX_CONTEXT_MENUS = 1024;

// combination of CContexTMenuManager and CContextMenuManagerBase
class [[offsetcomments]] CContextMenuManager : public CXWnd
{
	FORCE_SYMBOLS

public:
	CContextMenuManager(CXWnd*, uint32_t, const CXRect&);
	virtual ~CContextMenuManager();

	virtual int HandleLButtonDown(const CXPoint&, uint32_t) override;
	virtual int HandleRButtonDown(const CXPoint&, uint32_t) override;
	virtual int HandleWheelButtonDown(const CXPoint&, uint32_t) override;
	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	virtual void CreateDefaultMenu();
	virtual int HandleWindowMenuCommands(CXWnd*, int);

	CContextMenu* GetMenu(int index)
	{
		if (index >= 0 && index < NumMenus)
			return pMenus[index];

		return nullptr;
	}

	EQLIB_OBJECT int AddMenu(CContextMenu*);
	EQLIB_OBJECT int GetDefaultMenuIndex() { return DefaultMenuIndex; }
	EQLIB_OBJECT int PopupMenu(int, const CXPoint&, CXWnd*);
	EQLIB_OBJECT int RemoveMenu(int, bool);
	EQLIB_OBJECT void Flush();
	EQLIB_OBJECT void WarnDefaultMenu(CXWnd*);

	//----------------------------------------------------------------------------
	// data members

/*0x0268*/ CXWnd*             pParentMenuWnd;
/*0x0270*/ CContextMenu*      pCurrMenus[MAX_CONTEXT_MENU_DEPTH];
/*0x02b0*/ int                NumVisibleMenus;
/*0x02b4*/ int                CurrMenu;
/*0x02b8*/ CContextMenu*      pMenus[MAX_CONTEXT_MENUS];
/*0x22b8*/ int                NumMenus;
/*0x22c0*/ CXWnd*             pHandlerWnd;
/*0x22c8*/ int                HandlerCmd;
/*0x22cc*/ int                DefaultMenuIndex;
/*0x22d0*/ int                DefaultHelpItem;
/*0x22d4*/ int                DefaultBGItem;
/*0x22d8*/ int                DefaultMinItem;
/*0x22dc*/ int                DefaultCloseItem;
/*0x22e0*/ int                DefaultLockItem;
/*0x22e4*/ int                DefaultEscapeItem;
/*0x22e8*/
};

//============================================================================
// CContextMenu
//============================================================================

// Size: 0x348 @ 6/24/2024
class [[offsetcomments]] CContextMenu : public CListWnd
{
	FORCE_SYMBOLS

public:
	EQLIB_OBJECT CContextMenu(CXWnd* pParent, uint32_t MenuID, const CXRect& rect);
	EQLIB_OBJECT virtual ~CContextMenu();

	virtual int OnKillFocus(CXWnd*) override;

	// MenuID: Set HighPart as the ID for submenus and LowPart is then the subindex
	EQLIB_OBJECT int AddMenuItem(const CXStr& str, unsigned int MenuID, bool bChecked = false, COLORREF Color = 0xFFFFFFFF, bool bEnable = true);
	EQLIB_OBJECT int AddSeparator();
	EQLIB_OBJECT void Activate(CXPoint, int, int);
	EQLIB_OBJECT void CheckMenuItem(int ID, bool bVal = true, bool bUncheckAll = false);
	EQLIB_OBJECT void EnableMenuItem(int ID, bool bVal = true);
	EQLIB_OBJECT void RemoveAllMenuItems();
	EQLIB_OBJECT void RemoveMenuItem(int id);
	EQLIB_OBJECT void SetMenuItem(int ID, const CXStr& Str, bool bChecked = false, COLORREF Color = 0xFFFFFFFF, bool bEnable = true);

	EQLIB_OBJECT int InsertMenuItem(const CXStr& str, unsigned int position, unsigned int ItemID,
		bool bChecked, COLORREF Color, bool bEnable);

	//----------------------------------------------------------------------------
	// data members

/*0x348*/ int          NumItems;
/*0x34c*/
};

class CGFContextMenu : public CContextMenu
{
public:
	EQLIB_OBJECT CGFContextMenu(CXWnd* pParent, uint32_t MenuID, const CXRect& rect);
	EQLIB_OBJECT virtual ~CGFContextMenu();
};

//============================================================================
// CCursorAttachment
//============================================================================

enum ECursorAttachmentType
{
	eCursorAttachment_None              = -1,
	eCursorAttachment_MemorizeSpell     = 1,
	eCursorAttachment_Item,
	eCursorAttachment_Money,
	eCursorAttachment_Social,
	eCursorAttachment_MenuButton,
	eCursorAttachment_Ability,
	eCursorAttachment_Combat,
	eCursorAttachment_InvSlot,
	eCursorAttachment_SpellGem,
	eCursorAttachment_PetCommand,
	eCursorAttachment_SkillID,
	eCursorAttachment_MeleeAbility,
	eCursorAttachment_LeadershipAbility,
	eCursorAttachment_ItemLink, // also HeroForge
	eCursorAttachment_KronoSlot,
	eCursorAttachment_Command,
	eCursorAttachment_CombatAbility,
	eCursorAttachment_MountKeyRingLink,
	eCursorAttachment_IllusionKeyRingLink,
	eCursorAttachment_FamiliarKeyRingLink,
	eCursorAttachment_TeleportationKeyRingLink,
};

// @sizeof(CCursorAttachment) == 0x628 :: 2024-07-22 (live) @ 0x14018bdb8
constexpr size_t CCursorAttachment_size = 0x628;

class [[offsetcomments]] CCursorAttachment : public CGFScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CCursorAttachment(CXWnd*);
	virtual ~CCursorAttachment();

	//virtual int Draw() override;
	//virtual int OnProcessFrame() override;
	//virtual int WndNotification(CXWnd*, uint32_t, void*) override;
	//virtual void Deactivate() override;

	EQLIB_OBJECT bool IsOkToActivate(int);
	EQLIB_OBJECT bool RemoveAttachment();

	void AttachToCursor(CTextureAnimation* Overlay, CTextureAnimation* Background,
		ECursorAttachmentType Type, int Index, const char* AssignedName, const char* Name, int Qty = -1, int IconID = -1)
	{
		AttachToCursor(Overlay, Background, Type, Index, EqItemGuid(), 0, AssignedName, Name, Qty, IconID);
	}

	EQLIB_OBJECT void AttachToCursor(CTextureAnimation* Overlay, CTextureAnimation* Background,
		ECursorAttachmentType Type, int Index, const EqItemGuid& ItemGuid, int ItemID, const char* AssignedName,
		const char* Name, int Qty = -1, int IconID = -1);

	EQLIB_OBJECT bool AttachSpellToCursor(int SpellID);

	ECursorAttachmentType GetType() const { return static_cast<ECursorAttachmentType>(Type); }

	//----------------------------------------------------------------------------
	// data members

	// Shares the beginning of the other component class - need to unify
	struct CursorAttachmentComponent
	{
	/*0x00*/ eqstd::string name;
	/*0x20*/ eqstd::string fullName;
	/*0x40*/ eqstd::string modelPrefix;
	/*0x60*/ CCursorAttachment* target;
	/*0x68*/
	};

/*0x3c0*/ CursorAttachmentComponent Component;
/*0x428*/ UIStaticAnimationTemplate uiAnimComponent;
/*0x4c8*/ UIStaticAnimationTemplate uiAnimComponent2;
/*0x568*/ CStaticAnimationTemplate* pBGStaticAnim;                  // CA_Anim
/*0x570*/ CStaticAnimationTemplate* pOverlayStaticAnim;             // CA_Anim2
/*0x578*/ CTextObject*              pTextObject;
/*0x580*/ CTextObject*              pButtonTextObject;
/*0x588*/ int                       TextFontStyle;
/*0x58c*/ int                       Type;
/*0x590*/ int                       Index;
/*0x594*/ EqItemGuid                ItemGuid;
/*0x5a8*/ int                       ItemID;
/*0x5ac*/ int                       Quantity;
/*0x5b0*/ int                       IconID;
/*0x5b4*/ int                       ContextMenu;
/*0x5b8*/ SoeUtil::StringFixed<64>  AssignedName;
/*0x618*/ CXStr                     ButtonText;
/*0x620*/ CSpellGemWnd*             pSpellGem;                      // CA_SpellGem
/*0x628*/

	ALT_MEMBER_GETTER(int, Quantity, Qty);
};

SIZE_CHECK(CCursorAttachment, CCursorAttachment_size);

//============================================================================
// CDragonHoardWnd
//============================================================================

class [[offsetcomments]] CDragonHoardWnd : public CSidlScreenWnd, public WndEventHandler
{
public:
	FORCE_SYMBOLS;

/*0x2d8*/ CListWnd*     pItemList;          // DH_Item_List
/*0x2e0*/ CButtonWnd*   pInspectButton;     // DH_Inspect_Button
/*0x2e8*/ CButtonWnd*   pPreviewButton;     // DH_Preview_Button
/*0x2f0*/ CLabel*       pSelectedItemLabel; // DH_Selected_Item_Label
/*0x2f8*/ CButtonWnd*   pSelectedItemSlot;  // DH_Selected_Item_Slot
/*0x300*/ CButtonWnd*   pRetrieveButton;    // DH_Retrieve_Button
/*0x308*/ CButtonWnd*   pExitButton;        // DH_Exit_Button
/*0x310*/ CLabel*       pItemCountNumber;   // DH_Item_Count_Number
/*0x318*/ CEditWnd*     pSearchItemInput;   // DH_SearchItem_Input
/*0x320*/ CButtonWnd*   pSearchItemButton;  // DH_SearchItem_Button
/*0x328*/ ItemContainer Items;
/*0x350*/ bool          bNeedsUpdate;
/*0x358*/ ItemPtr       pSelectedItem;
/*0x368*/ uint64_t      SelectedItemId;
/*0x370*/
};

//============================================================================
// CExtendedTargetWnd
//============================================================================

constexpr int MAX_EXTENDED_TARGET_SIZE = 23; // total available extended targets


class CExtendedTargetWnd : public CSidlScreenWnd
{
public:
};

//============================================================================
// CFactionWnd
//============================================================================

class [[offsetcomments]] CFactionWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CFactionWnd(CXWnd*);
	virtual ~CFactionWnd();

	//----------------------------------------------------------------------------
	// data members

/*0x2d4*/ bool             Unknown0x22c;
/*0x2d8*/ int              Unknown0x230;
/*0x2dc*/ int              Unknown0x234;
/*0x2e0*/ int              Unknown0x238;
/*0x2e8*/ SoeUtil::String  Unknown0x23c;
/*0x300*/ SoeUtil::String  Unknown0x24c;
/*0x318*/ CGaugeWnd*       StandingGaugeTemplate;
/*0x320*/ CTreeViewWnd*    Categories;
/*0x328*/ CEditWnd*        SearchNameInput;
/*0x330*/ CButtonWnd*      SearchButton;
/*0x338*/ CListWnd*        FactionList;
/*0x340*/
};

inline namespace deprecated {
	using EQFACTIONWINDOW DEPRECATE("Use CFactionWnd instead of EQFACTIONWINDOW") = CFactionWnd;
	using PEQFACTIONWINDOW DEPRECATE("Use CFactionWnd* instead of PEQFACTIONWINDOW") = CFactionWnd*;
}

//============================================================================
// CPlayerCustomizationWnd
//============================================================================

class [[offsetcomments]] CPlayerCustomizationWnd : public CSidlScreenWnd
{
public:
	EQLIB_OBJECT CPlayerCustomizationWnd(CXWnd*);
	EQLIB_OBJECT ~CPlayerCustomizationWnd();

	EQLIB_OBJECT void SetFaceSelectionsFromPlayer();

	// virtual
	EQLIB_OBJECT int Draw() const;
	EQLIB_OBJECT int OnProcessFrame();
	EQLIB_OBJECT int WndNotification(CXWnd*, uint32_t, void*);

	// private
	EQLIB_OBJECT void CycleThroughFHEB(int, int);
	EQLIB_OBJECT void ShowButtonGroup(int, bool);
};

//============================================================================
// CFindItemWnd
//============================================================================

// @sizeof(CFindItemWnd) == 0x3f8 :: 2024-07-22 (live) @ 0x14018d444
constexpr size_t CFindItemWnd_size = 0x3f8;

class [[offsetcomments]] CFindItemWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CFindItemWnd(CXWnd*);
	virtual ~CFindItemWnd();

	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT void Update();
	EQLIB_OBJECT void PickupSelectedItem();

	struct ItemRecord
	{
		ItemGlobalIndex       itemIndex;
		EqItemGuid            guid;
		eqtime_t              unknown;
	};

	ItemGlobalIndex GetItemGlobalIndex(int index)
	{
		ItemRecord* itemRecord = Items.FindFirst(index);
		if (itemRecord)
		{
			return itemRecord->itemIndex;
		}

		return ItemGlobalIndex();
	}

/*0x2d4*/ bool                Unknown0;
/*0x2d5*/ bool                Unknown1;
/*0x2d8*/ uint32_t            Unknown2;
/*0x2dc*/ uint32_t            Timestamp;
/*0x2e0*/ uint32_t            Counter;
/*0x2e8*/ ItemContainer       Container;
/*0x310*/ HashTable<ItemRecord, int> Items;
/*0x328*/ HashTable<SoeUtil::String, int> ItemNames;     // just a guess, likely inaccurate.
/*0x340*/ CStaticAnimationTemplate* FIW_ClassAnim;
/*0x348*/ CSidlScreenWnd*     FIW_CharacterView;
/*0x350*/ CListWnd*           FIW_ItemList;
/*0x358*/ CButtonWnd*         FIW_QueryButton;
/*0x360*/ CButtonWnd*         FIW_RequestItemButton;
/*0x368*/ CButtonWnd*         FIW_RequestPreviewButton;
/*0x370*/ CButtonWnd*         FIW_Default;
/*0x378*/ CButtonWnd*         FIW_GrabButton;
/*0x380*/ CButtonWnd*         FIW_HighlightButton;
/*0x388*/ CButtonWnd*         FIW_DestroyItem;
/*0x390*/ CButtonWnd*         FIW_SearchDepotButton;
/*0x398*/ CComboWnd*          FIW_ItemLocationCombobox;
/*0x3a0*/ CComboWnd*          FIW_ItemSlotCombobox;
/*0x3a8*/ CComboWnd*          FIW_StatSlotCombobox;
/*0x3b0*/ CComboWnd*          FIW_RaceSlotCombobox;
/*0x3b8*/ CComboWnd*          FIW_ClassSlotCombobox;
/*0x3c0*/ CComboWnd*          FIW_ItemTypeCombobox;
/*0x3c8*/ CComboWnd*          FIW_ItemPrestigeCombobox;
/*0x3d0*/ CComboWnd*          FIW_ItemAugmentCombobox;
/*0x3d8*/ CEditWnd*           FIW_ItemNameInput;
/*0x3e0*/ CEditWnd*           FIW_MaxLevelInput;
/*0x3e8*/ CEditWnd*           FIW_MinLevelInput;
/*0x3f0*/ CLabelWnd*          FIW_DragonHoardLabel;
/*0x3f8*/
};

SIZE_CHECK(CFindItemWnd, CFindItemWnd_size);

//============================================================================
// CFindLocationWnd
//============================================================================

enum FindLocationType {
	FindLocation_Unknown,
	FindLocation_Player,
	FindLocation_POI,
	FindLocation_RealEstateItem,
	FindLocation_RealEstatePlot,
	FindLocation_MapPoint,
	FindLocation_Switch,
	FindLocation_Location
};
EQLIB_API const char* FindLocationTypeToString(FindLocationType type);

// @sizeof(CFindLocationWnd) == 0x3a0 :: 2024-07-22 (live) @ 0x14018dc02
constexpr size_t CFindLocationWnd_size = 0x3a0;

class [[offsetcomments]] CFindLocationWnd : public CSidlScreenWnd
{
public:
	// has virtuals, but we get those from CSidlScreenWnd
	EQLIB_OBJECT bool HandleFindBegin();
	EQLIB_OBJECT void HandleFindEnd();
	EQLIB_OBJECT void HandleRowClicked(int Index);
	EQLIB_OBJECT void HandleFindableZoneConnectionsMessage(CUnSerializeBuffer& buf);

public:
	struct FindPlayerData
	{
		uint32_t spawnId;
		CXStr    name;
		CXStr    description;
		int      race;
		int      Class;
	};
	using PlayerDataArray = ArrayClass<FindPlayerData>;

	struct FindPOIData
	{
		uint32_t id;
		CXStr    name;
		CXStr    description;
	};
	using POIDataArray = ArrayClass<FindPOIData>;

	struct FindZoneConnectionData
	{
		FindLocationType type = FindLocation_Location;
		int32_t          id = 0;
		int32_t          subId = -1;
		EQZoneIndex      zoneId;
		int              zoneIdentifier;
		CVector3         location;
	};
	using ZoneConnectionDataArray = ArrayClass<FindZoneConnectionData>;

	struct FindableReference
	{
		FindLocationType type;
		uint32_t         index;

		bool operator==(const FindableReference& other) const { return type == other.type && index == other.index; }
	};
	using FindableReferenceList = HashTable<FindableReference>;

public:
/*0x2d0*/ CListWnd*               findLocationList;        // FLW_FindLocationList
/*0x2d8*/ CCheckBoxWnd*           filterLocationsCheckBox; // FLW_FilterLocationsCheckbox
/*0x2e0*/ CCheckBoxWnd*           filterGroupCheckBox;     // FLW_FilterGroupCheckbox
/*0x2e8*/ CCheckBoxWnd*           filterRaidCheckBox;      // FLW_FilterRaidCheckbox
/*0x2f0*/ CLabelWnd*              noneLabel;               // FLW_FoundNoneLabel
/*0x2f8*/ uint32_t                lastUpdateTime;
/*0x2fc*/ uint32_t                lastFindRequestTime;
/*0x300*/ bool                    didFindRequest;
/*0x308*/ PlayerDataArray         unfilteredPlayerList;
/*0x320*/ PlayerDataArray         filteredGroupPlayerList;
/*0x338*/ PlayerDataArray         unfilteredRaidPlayerList;
/*0x350*/ POIDataArray            unfilteredPOIDataList;
/*0x368*/ ZoneConnectionDataArray unfilteredZoneConnectionList;
/*0x380*/ FindableReferenceList   referenceList;
/*0x398*/ uint32_t                lastId;
/*0x39c*/ bool                    dataRequested;
/*0x39d*/ bool                    playerListDirty;
/*0x39e*/ bool                    zoneConnectionsRcvd;
/*0x3a0*/
};

SIZE_CHECK(CFindLocationWnd, CFindLocationWnd_size);

//============================================================================
// CFileSelectionWnd
//============================================================================

class [[offsetcomments]] CFileSelectionWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS;

public:
	CFileSelectionWnd(CXWnd*);
	virtual ~CFileSelectionWnd();

	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	// would be LPITEMIDLIST from shtypes.h
	using LPITEMIDLIST = void*;

	EQLIB_OBJECT CXStr GetSelectedFile(int);
	EQLIB_OBJECT int GetSelectedFileCount();
	EQLIB_OBJECT void Callback(bool);
	EQLIB_OBJECT bool DirectoryEmpty(IShellFolder*, LPITEMIDLIST);
	EQLIB_OBJECT unsigned long GetPath(IShellFolder*, LPITEMIDLIST, bool, CXStr&);
	EQLIB_OBJECT void ClearFileList();
	EQLIB_OBJECT void GoSubdirectory(LPITEMIDLIST);
	EQLIB_OBJECT void MakeFilePath();
	EQLIB_OBJECT void UpdateButtons();
	EQLIB_OBJECT void UpdateFileList();

	EQLIB_OBJECT int Open(CXWnd* pWnd, int flags);

/*0x2d4*/ int             Unknown0x24c;
/*0x2d8*/ CXWnd*          pWndCaller;
/*0x2e0*/ CListWnd*       pListFiles;
/*0x2e8*/ CEditWnd*       pEditEntry;
/*0x2f0*/ CComboWnd*      pComboDirHistory;
/*0x2f8*/ CButtonWnd*     pUpButton;
/*0x300*/ CButtonWnd*     pOKButton;
/*0x308*/ CButtonWnd*     pCancelButton;
/*0x310*/ int             Flags;
/*0x318*/ CXStr           SelectedFile;
/*0x320*/ IShellFolder*   ShellFolder;
/*0x328*/ CXStr           CurrentDir;
/*0x330*/ // more
};

//============================================================================
// CFriendsWnd
//============================================================================

class CFriendsWnd : public CSidlScreenWnd
{
public:
	CFriendsWnd(CXWnd*);
	virtual ~CFriendsWnd();

	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT void UpdateFriendsList();
	EQLIB_OBJECT void UpdateIgnoreList();
	EQLIB_OBJECT void AddFriend();
	EQLIB_OBJECT void AddIgnore();
	EQLIB_OBJECT void UpdateButtons();
};

//============================================================================
// CGemsGameWnd
//============================================================================

struct GemsBlock;

class CGemsGameWnd : public CSidlScreenWnd
{
public:
	EQLIB_OBJECT CGemsGameWnd(CXWnd*);
	virtual ~CGemsGameWnd();

	virtual int HandleLButtonUp(const CXPoint&, uint32_t) override;
	virtual int OnProcessFrame() override;
	virtual int PostDraw() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT void MoveCurBlock(int);
	EQLIB_OBJECT bool BadSpecial(int) const;
	EQLIB_OBJECT bool LegalBlockMove(int, int, int, int, bool);
	EQLIB_OBJECT int GetRndBlockImage();
	EQLIB_OBJECT void ActivateSpecialMode(int);
	EQLIB_OBJECT void AdvanceToNextWave();
	EQLIB_OBJECT void CheckForMatches(int, int);
	EQLIB_OBJECT void CheckForNewHighScore();
	EQLIB_OBJECT void ClearBlock(GemsBlock*);
	EQLIB_OBJECT void ClearHighScores();
	EQLIB_OBJECT void DoMatchScore(int);
	EQLIB_OBJECT void DrawSpellGem(int, const CXRect&, int, bool) const;
	EQLIB_OBJECT void GetNextBlock();
	EQLIB_OBJECT void MakeBlockDrop(int, int, int);
	EQLIB_OBJECT void MarkHigherBlocksFalling(int, int);
	EQLIB_OBJECT void ProcessMatches(int);
	EQLIB_OBJECT void ProcessMoveCurBlock(int);
	EQLIB_OBJECT void ReadHighScores();
	EQLIB_OBJECT void Restart();
	EQLIB_OBJECT void SetNextUpdate();
	EQLIB_OBJECT void SetPause(bool);
	EQLIB_OBJECT void TogglePause();
	EQLIB_OBJECT void Update();
	EQLIB_OBJECT void UpdateDisplay();
	EQLIB_OBJECT void WriteHighScores();
};

//============================================================================
// CGiveWnd
//============================================================================

class [[offsetcomments]] CGiveWnd : public CSidlScreenWnd, public PopDialogHandler, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CGiveWnd(CXWnd*);
	virtual ~CGiveWnd();

	virtual int OnProcessFrame() override;
	virtual int PostDraw() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT void UpdateGiveDisplay();

	//----------------------------------------------------------------------------
	// data members

/*0x2e0*/ CButtonWnd*    pMoneyButton[4];
/*0x300*/ CButtonWnd*    TradeButton;
/*0x308*/ CButtonWnd*    CancelButton;
/*0x310*/ CLabel*        NPCNameLabel;
/*0x318*/ CInvSlotWnd*   pInvSlotWnd[MAX_GIVE_SLOTS];
/*0x338*/
};

//============================================================================
// CGroupSearchFiltersWnd
//============================================================================

// todo: not mapped or cleaned up
class CGroupSearchFiltersWnd : public CSidlScreenWnd
{
public:
	EQLIB_OBJECT CGroupSearchFiltersWnd(CXWnd*);
	EQLIB_OBJECT bool ShouldFilterPlayer(const char*, const char*) const;
	EQLIB_OBJECT bool UseExclusiveSearchMode() const;
	EQLIB_OBJECT void HandleAddDesiredGuild();
	EQLIB_OBJECT void HandleAddExcludedGuild();
	EQLIB_OBJECT void HandleDesiredGuildsSelected();
	EQLIB_OBJECT void HandleExcludeGuildsSelected();
	EQLIB_OBJECT void HandleRemoveAllDesiredGuilds();
	EQLIB_OBJECT void HandleRemoveAllExcludedGuilds();
	EQLIB_OBJECT void HandleRemoveDesiredGuild();
	EQLIB_OBJECT void HandleRemoveExcludedGuild();
	EQLIB_OBJECT void HandleUseFriendsListSelected();
	EQLIB_OBJECT void HandleUseIgnoreListSelected();

	// virtual
	EQLIB_OBJECT ~CGroupSearchFiltersWnd();
	EQLIB_OBJECT int OnProcessFrame();
	EQLIB_OBJECT int WndNotification(CXWnd*, uint32_t, void*);

	// private
	EQLIB_OBJECT bool GuildIsInDesiredGuildsList(const char*) const;
	EQLIB_OBJECT bool GuildIsInExcludedGuildsList(const char*) const;
	EQLIB_OBJECT bool Load();
	EQLIB_OBJECT bool NameIsInFriendsList(const char*) const;
	EQLIB_OBJECT bool NameIsInIgnoreList(const char*) const;
	EQLIB_OBJECT bool UseDesiredGuilds() const;
	EQLIB_OBJECT bool UseExcludedGuilds() const;
	EQLIB_OBJECT bool UseFriendsList() const;
	EQLIB_OBJECT bool UseIgnoreList() const;
	EQLIB_OBJECT void ClearUiPointers();
	EQLIB_OBJECT void FetchUiPointers();
	EQLIB_OBJECT void InitPieces();
	EQLIB_OBJECT void Save() const;
};

//============================================================================
// CGroupSearchWnd
//============================================================================

struct LfgGroupResult;
class SListWndSortInfo;
struct LfgPlayerData;
struct LfgPlayerResult;

// todo: not mapped or cleaned up
class CGroupSearchWnd : public CSidlScreenWnd
{
public:
	EQLIB_OBJECT CGroupSearchWnd(CXWnd*);
	EQLIB_OBJECT void AddGroupResult(const LfgGroupResult*);
	EQLIB_OBJECT void AddPlayerResult(const LfgPlayerResult*);
	EQLIB_OBJECT void HandleDeselectAllClasses();
	EQLIB_OBJECT void HandleDoubleClickedOnPlayer(const char*);
	EQLIB_OBJECT void HandleGroupInfoPost();
	EQLIB_OBJECT void HandleGroupInfoRemove();
	EQLIB_OBJECT void HandleGroupInfoUpdate();
	EQLIB_OBJECT void HandleGroupMakeupChanged();
	EQLIB_OBJECT void HandleGroupResultColSelected(int);
	EQLIB_OBJECT void HandleGroupResultRowSelected(int);
	EQLIB_OBJECT void HandleNumericSort(SListWndSortInfo*);
	EQLIB_OBJECT void HandleOpenFiltersWindow();
	EQLIB_OBJECT void HandlePlayerInfoPost();
	EQLIB_OBJECT void HandlePlayerInfoRemove();
	EQLIB_OBJECT void HandlePlayerInfoUpdate();
	EQLIB_OBJECT void HandlePlayerResultColSelected(int);
	EQLIB_OBJECT void HandleQueryingForGroups();
	EQLIB_OBJECT void HandleQueryingForPlayers();
	EQLIB_OBJECT void HandleSelectAllClasses();
	EQLIB_OBJECT void ResetGroupList();
	EQLIB_OBJECT void ResetPlayerList();
	EQLIB_OBJECT void SendServerLfgOff();
	EQLIB_OBJECT void SendServerLfgOn();
	EQLIB_OBJECT void SendServerLfpOff();
	EQLIB_OBJECT void SendServerLfpOn();

	// virtual
	EQLIB_OBJECT ~CGroupSearchWnd();
	EQLIB_OBJECT int OnProcessFrame();
	EQLIB_OBJECT int WndNotification(CXWnd*, uint32_t, void*);

	// private
	EQLIB_OBJECT bool IsLevelRangeValid(int, int) const;
	EQLIB_OBJECT bool PlayerShouldBeFiltered(const LfgPlayerData*) const;
	EQLIB_OBJECT int CheckIfCurrentLfgInfoIsValid() const;
	EQLIB_OBJECT int CheckIfCurrentLfpInfoIsValid() const;
	EQLIB_OBJECT int GetDesiredClassesFlag() const;
	EQLIB_OBJECT int GetDesiredGroupMaxLevel() const;
	EQLIB_OBJECT int GetDesiredGroupMinLevel() const;
	EQLIB_OBJECT int GetDesiredPlayerMaxLevel() const;
	EQLIB_OBJECT int GetDesiredPlayerMinLevel() const;
	EQLIB_OBJECT int GetEqClassType(int) const;
	EQLIB_OBJECT int GroupInfoPageOnProcessFrame();
	EQLIB_OBJECT int GroupInfoPageWndNotification(CXWnd*, uint32_t, void*);
	EQLIB_OBJECT int GroupListPageOnProcessFrame();
	EQLIB_OBJECT int GroupListPageWndNotification(CXWnd*, uint32_t, void*);
	EQLIB_OBJECT int PlayerInfoPageOnProcessFrame();
	EQLIB_OBJECT int PlayerInfoPageWndNotification(CXWnd*, uint32_t, void*);
	EQLIB_OBJECT int PlayerListPageOnProcessFrame();
	EQLIB_OBJECT int PlayerListPageWndNotification(CXWnd*, uint32_t, void*);
	EQLIB_OBJECT int RedirectOnProcessFrameTo(CPageWnd*);
	EQLIB_OBJECT int RedirectWndNotificationTo(CPageWnd*, CXWnd*, uint32_t, void*);
	EQLIB_OBJECT void ClearUiPointers();
	EQLIB_OBJECT void FetchUiPointers();
	EQLIB_OBJECT void FilterOutBadWords(const char*, char*) const;
	EQLIB_OBJECT void GetDefaultLfgLevelRange(const PcClient*, int*, int*) const;
	EQLIB_OBJECT void GetDefaultLfpLevelRange(const PcClient*, int*, int*) const;
	EQLIB_OBJECT void InitLfg();
	EQLIB_OBJECT void InitLfp();
	EQLIB_OBJECT void LockQueryButtonAtTime(long);
	EQLIB_OBJECT void PopUpErrorMessage(int) const;
	EQLIB_OBJECT void ShowGroupDetails(LfgGroupResult const*);
	EQLIB_OBJECT void UpdateGroupLabel();
	EQLIB_OBJECT void UpdateLfgPostingStatus();
	EQLIB_OBJECT void UpdateLfpPostingStatus();
	EQLIB_OBJECT void UpdatePlayerLabel();
	EQLIB_OBJECT void UpdateRemainingQueryLockedTime(long);
};

//============================================================================
// CGroupWnd
//============================================================================

// @sizeof(CGroupWnd) == 0x700 :: 2024-07-22 (live) @ 0x14018c537
constexpr size_t CGroupWnd_size = 0x700;

class [[offsetcomments]] CGroupWnd : public CGFScreenWnd
{
	FORCE_SYMBOLS

public:
	CGroupWnd(CXWnd*);
	virtual ~CGroupWnd();

	virtual int Draw() override;
	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;
	virtual void LoadIniInfo() override;
	virtual void StoreIniInfo() override;

	EQLIB_OBJECT void CreateLocalMenu();
	EQLIB_OBJECT void KeyMapUpdated();
	EQLIB_OBJECT void SetInvited(bool);
	EQLIB_OBJECT void UpdateContextMenu();

	EQLIB_OBJECT void UpdateButtons();
	EQLIB_OBJECT void UpdateDisplay(int Index, PlayerClient* groupmember, COLORREF NameColor, UINT RoleBits);

	//----------------------------------------------------------------------------
	// data members
/*0x3b8*/ CButtonWnd*        InviteButton;
/*0x3c0*/ CButtonWnd*        DisbandButton;
/*0x3c8*/ CButtonWnd*        FollowButton;
/*0x3d0*/ CButtonWnd*        DeclineButton;
/*0x3d8*/ CButtonWnd*        LFGButton;
/*0x3e0*/ CGaugeWnd*         HPGauge[6];
/*0x410*/ CGaugeWnd*         PetGauge[6];
/*0x440*/ CGaugeWnd*         ManaGauge[6];
/*0x470*/ CGaugeWnd*         EnduranceGauge[6];
/*0x4a0*/ CLabel*            HPLabel[6];
/*0x4d0*/ CLabel*            HPPercLabel[6];
/*0x500*/ CLabel*            ManaLabel[6];
/*0x530*/ CLabel*            ManaPercLabel[6];
/*0x560*/ CLabel*            EnduranceLabel[6];
/*0x590*/ CLabel*            EndurancePercLabel[6];
/*0x5c0*/ COLORREF           HPTextColor[6];
/*0x5d8*/ CButtonWnd*        GroupTankButton[6];
/*0x608*/ CButtonWnd*        GroupAssistButton[6];
/*0x638*/ CButtonWnd*        GroupPullerButton[6];
/*0x668*/ CButtonWnd*        GroupMarkNPCButton[6];
/*0x698*/ CLabel*            AggroPercLabel[6];
/*0x6c8*/ long               Timer;
/*0x6d0*/ CContextMenu*      GroupContextMenu;
/*0x6d8*/ bool               bPetbars;
/*0x6d9*/ bool               bManabars;
/*0x6da*/ bool               bEndurancebars;
/*0x6db*/ bool               bAggroPct;
/*0x6dc*/ int                PetBarIndex;
/*0x6e0*/ int                ManaBarIndex;
/*0x6e4*/ int                EnduranceBarIndex;
/*0x6e8*/ int                AggroPctIndex;
/*0x6ec*/ int                RoleSeparatorID;
/*0x6f0*/ int                RoleSelectMenu;
/*0x6f4*/ int                RoleSelectMenuID;
/*0x6f8*/ bool               bPlayerInvited;
/*0x6fc*/
};

SIZE_CHECK(CGroupWnd, CGroupWnd_size);

//============================================================================
// CGuildMgmtWnd
//============================================================================

// todo: update, some of this is obsolete
struct [[offsetcomments]] GuildMember
{
	// Start of GuildMember
/*0x000*/ GuildMember*       pNext;
/*0x008*/ bool               bOnline;
/*0x00a*/ WORD               ZoneID;
/*0x00c*/ WORD               Instance;
/*0x010*/ DWORD              PlayerSerial;
/*0x014*/ bool               bMainProfile;
/*0x015*/ bool               bOfflineMode;
/*0x016*/ char               Name[0x40];
/*0x058*/ int                Level;
/*0x05c*/ int                Flags; //1=banker, 2=alt
/*0x060*/ DWORD              Class;
/*0x064*/ DWORD              Rank; //0=member 1=officer 2=leader
/*0x068*/ char               PlayerHandle[0x20];
/*0x088*/ char               PlayerComments[0x100];
/*0x188*/ DWORD              LastLoginTime;
/*0x190*/ EqGuid             PlayerGuild;//size is 8
/*0x198*/ bool               bGuildShowSprite;
/*0x199*/ bool               bTributeStatus;//active on/off
/*0x19a*/ bool               bTrophyStatus;//active on/off
/*0x19c*/ int                TributeDonations;
/*0x1a0*/ DWORD              LastDonation;//timestamp
/*0x19c*/ // end of GuildMember
	// start of GuildMemberClient
/*0x1a4*/ char               PublicNote[0x100];
/*0x2a4*/ char               PersonalNote[0x100];
/*0x3a4*/ bool               bTributeOptIn;
/*0x3a5*/ bool               bTrophyTributeOptIn;
/*0x3a8*/
};

inline namespace deprecated {
	using GUILDMEMBERCLIENT DEPRECATE("Use GuildMember instead of GUILDMEMBERCLIENT") = GuildMember;
	using PGUILDMEMBERCLIENT DEPRECATE("Use GuildMember* instead of PGUILDMEMBERCLIENT") = GuildMember*;
	using GUILDMEMBERINFO DEPRECATE("Use GuildMember instead of GUILDMEMBERINFO") = GuildMember;
	using PGUILDMEMBERINFO DEPRECATE("Use GuildMember* instead of PGUILDMEMBERINFO") = GuildMember*;
}

// FIXME: Technically part of a HashMap of some kind
struct [[offsetcomments]] GuildNameEntry
{
/*0x00*/ EqGuid          Guild_Guid;
/*0x08*/ char            Name[0x40];
/*0x48*/ EqGuid          Guild_Guid_Copy;
/*0x50*/ GuildNameEntry* pNext;
/*0x58*/ GuildNameEntry* pPrev;
/*0x60*/ GuildNameEntry* pNode1;
/*0x68*/ GuildNameEntry* pANode2;
/*0x70*/
};

struct [[offsetcomments]] GuildBanner
{
/*0x00*/ int        Type;
/*0x04*/ int        PrimaryFlagPattern;
/*0x08*/ int        SecondaryFlagPattern;
/*0x0c*/ ARGBCOLOR  PrimaryTint;
/*0x10*/ ARGBCOLOR  SecondaryTint;
/*0x18*/ eqtime_t   TimeRemaining;
/*0x20*/ int        ZoneID;
/*0x24*/ CVector3   Loc;
/*0x30*/ int        Heading;
/*0x38*/ eqtime_t   GracePeriodRemaining;
/*0x40*/ int        ModificationCount;
/*0x44*/ int        TeleportPlantRestriction;
/*0x48*/ eqtime_t   ModifyDate;
/*0x50*/
};

enum GuildRankType
{
	GRT_None,
	GRT_Leader,
	GRT_SeniorOfficer,
	GRT_Officer,
	GRT_SeniorMember,
	GRT_Member,
	GRT_JuniorMember,
	GRT_Initiate,
	GRT_Recruit,

	NumGuildRanks,
};

struct [[offsetcomments]] GuildRank
{
/*0x00*/ GuildRankType Type;
/*0x08*/ CXStr         Name;
/*0x10*/ };


template <typename Group>
class GuildRankPermissions : public AccessGroupList<Group>
{
};

enum GuildPermissionType
{
	GPT_None,
	GPT_BannerChange,
	GPT_BannerPlant,
	GPT_BannerRemove,
	GPT_DisplayGuildName,
	GPT_ChangePermissions,
	GPT_ChangeRankNames,
	GPT_InviteMember,
	GPT_PromoteMember,
	GPT_DemoteMember,
	GPT_RemoveMember,
	GPT_EditRecruitingSettings,
	GPT_EditPublicNotes,
	GPT_BankDeposit,
	GPT_BankWithdraw,
	GPT_BankView,
	GPT_BankPromote,
	GPT_BankChangeItemPermission,
	GPT_ChangeMOTD,
	GPT_SeeGuildChat,
	GPT_SpeakGuildChat,
	GPT_SendGuildEMail,
	GPT_TributeChangeSettingsForOthers,
	GPT_TributeChangeActiveBenefit,
	GPT_TrophyTributeChangeSettingsForOthers,
	GPT_TrophyTributeChangeActiveBenefit,
	GPT_ChangeAltFlagForOthers,
	GPT_GuildPlotBuy,
	GPT_GuildPlotSell,
	GPT_ModifyTrophies,
	GPT_DemoteSelf,

	NumGuildPermissionTypes,
};

using GuildRankPermission = AccessGroup<GuildRankType, NumGuildRanks, GRT_None, GuildPermissionType, NumGuildPermissionTypes>;

class [[offsetcomments]] GuildBase
{
public:
	EQLIB_OBJECT GuildBase();
	EQLIB_OBJECT virtual ~GuildBase();

/*0x008*/ int                    NumGuildMembers;
/*0x010*/ GuildMember*           pFirstGuildMember;
/*0x018*/ EqGuid                 GuildGuid;
/*0x020*/ char                   Name[0x40];
/*0x060*/ eqtime_t               CreationDate;
/*0x068*/ int                    Active;
/*0x070*/ int64_t                GuildID;
/*0x078*/ bool                   bHasChanged;
/*0x079*/ char                   GuildMOTD[0x200];
/*0x280*/ SoeUtil::String        MOTD_Author;
/*0x298*/ int                    TributePoints;
/*0x29c*/ BenefitSelection       ActiveTributeBenefits[2];
/*0x2ac*/ bool                   bTributeBenefitsActive;
/*0x2b0*/ BenefitSelection       ActiveTrophyTributeBenefits[4];
/*0x2d0*/ bool                   bTrophyTributeBenefitsActive;
/*0x2d1*/ bool                   bRenameFlag;
/*0x2d2*/ bool                   bUnknownFlag;
/*0x2d8*/ GuildBanner            Banner;
/*0x328*/ TSafeArrayStatic<uint8_t, 0x320> BitFlags;
/*0x648*/ TSafeArrayStatic<uint8_t, 0x320> PrevBitFlags;
/*0x968*/ char                   GuildURL[0x200];
/*0xb68*/ char                   GuildChannel[0x80];
/*0xbe8*/ ArrayClass<GuildRank*> Ranks;
/*0xc00*/ GuildRankPermissions<GuildRankPermission>* RankPermissions;
/*0xc08*/

	ALT_MEMBER_GETTER(GuildMember*, pFirstGuildMember, pMember);
};

struct guildmotdSet;

class [[offsetcomments]] CGuild : public GuildBase
{
public:
/*0x0c08*/ eqtime_t              LastGuildNameRequest;
/*0x0c10*/ HashListSet<GuildNameEntry*, 250> GuildNamesTable;
/*0x1400*/ void* Map[3];     // this is a SoeUtil::Map
/*0x1418*/ int                   OnlineCount;      // 1418
/*0x141c*/ bool                  bOnlineOutofSync; // 141c
/*0x1420*/ int                   TributeTimer;
/*0x1424*/ int                   TributeCost;
/*0x1428*/ bool                  bTributeCostDirty;
/*0x1429*/ bool                  bTributeActive;
/*0x142a*/ bool                  bTributeBenefitsLocked;
/*0x1430*/ eqtime_t              TrophyTributeTimer;
/*0x1438*/ int                   TrophyTributeCost;
/*0x143c*/ bool                  bTrophyTributeCostOutofSync;
/*0x143d*/ bool                  bTrophyTributeActive;
/*0x143e*/ bool                  bTrophyBenefitsLocked;
/*0x1440*/

	EQLIB_OBJECT bool ValidGuildName(int);
	EQLIB_OBJECT char* GetGuildMotd();
	EQLIB_OBJECT char* GetGuildMotdAuthor();
	EQLIB_OBJECT const char* GetGuildName(int64_t, char* buffer, bool* found, bool) const;
	EQLIB_OBJECT GuildMember* FindMemberByName(const char*);
	EQLIB_OBJECT void DeleteAllMembers();
	EQLIB_OBJECT void DemoteMember(GuildMember*);
	EQLIB_OBJECT void HandleGuildMessage(UdpLibrary::UdpConnection*, uint32_t, char*, uint32_t);
	EQLIB_OBJECT void SendPublicCommentChange(char*, char*);
	EQLIB_OBJECT void SetGuildMotd(guildmotdSet*);

	EQLIB_OBJECT int64_t GetGuildIndex(const char*);

	inline const char* GetGuildName(int64_t guildId) const
	{
		char buffer[64] = { 0 };
		bool found = false;

		return GetGuildName(guildId, buffer, &found, true);
	}


	// private
	EQLIB_OBJECT void AddGuildMember(GuildMember*);
	EQLIB_OBJECT void ChangeGuildMemberName(char*);
	EQLIB_OBJECT void HandleAddGuildMember(char*, int);
	EQLIB_OBJECT void HandleDeleteGuildResponse(char*);
	EQLIB_OBJECT void HandleGuildInvite(UdpLibrary::UdpConnection*, uint32_t, char*, uint32_t);
	EQLIB_OBJECT void HandleMemberLevelUpdate(char*);
	EQLIB_OBJECT void HandleRemoveGuildMember(char*, int);
	EQLIB_OBJECT void InitializeFromDump(char*);
	EQLIB_OBJECT void UpdateGuildMemberOnWindow(GuildMember*);
	EQLIB_OBJECT void UpdateMemberStatus(char*);
	EQLIB_OBJECT void UpdatePublicComment(char*);
};


// Size: 0x3d0 (02/18/2004)
class [[offsetcomments]] CGuildMgmtWnd
	: public CSidlScreenWnd
	, public PopDialogHandler
	, public WndEventHandler
	, public CVivoxObserver
{
	FORCE_SYMBOLS

public:
	CGuildMgmtWnd(CXWnd*);
	virtual ~CGuildMgmtWnd();

	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT void AddMember(GuildMember*);
	EQLIB_OBJECT void Clean();
	EQLIB_OBJECT void DumpToFile(char*);
	EQLIB_OBJECT void RemoveMember(GuildMember*);
	EQLIB_OBJECT void RenameMember(char*, char*);
	EQLIB_OBJECT void SetMOTD(char*, char*);
	EQLIB_OBJECT void SetPlayerCount(int);
	EQLIB_OBJECT void SortList(int, bool);
	EQLIB_OBJECT void UpdateButtons();
	EQLIB_OBJECT void UpdateListMember(GuildMember*, int);
	EQLIB_OBJECT char* GetPersonalNote(char*);
	EQLIB_OBJECT int FindListMember(GuildMember*);
	EQLIB_OBJECT void CleanAndRefillListWnd(bool);
	EQLIB_OBJECT void CreatePersonalNotesFilename();
	EQLIB_OBJECT void LoadINI();
	EQLIB_OBJECT void LoadPersonalNotes();
	EQLIB_OBJECT void SavePersonalNotes();
	EQLIB_OBJECT void SetPersonalNote(char*, char*);

	//----------------------------------------------------------------------------
	// data members

	// this is all out of date and needs updating
/*0x250*/ // start

/*0x2ec*/ BYTE         Unknown0x148[0x3c];
/*0x328*/ BYTE         Unknown0x194;
/*0x329*/ BYTE         ShowOffline;              // 01 = show offline box is checked
/*0x32a*/ BYTE         Unknown0x196[0x2];

	// This is a pointer to the beginning of a list of pointers, each representing one player in
	// the guild. The number of  pointers depends upon TotalMemberCount.
/*0x330*/ GuildMember** pMember;
/*0x338*/ DWORD        TotalMemberCount;
/*0x33c*/ DWORD        Unknown0x1a0;             // 150?
/*0x340*/ DWORD        Unknown0x1a4;             // 1?
/*0x344*/ DWORD        TotalMemberCountAgain;
/*0x348*/ BYTE         Unknown0x1ac[0x1c];
/*0x364*/ char         PersonalNotesFilePath[0x40]; // path to personal notes file
/*0x3a4*/ BYTE         Unknown0x208[0x1c0];      // empty
/*0x564*/ DWORD        Unknown0x3c8;             // 1?
/*0x568*/ DWORD        Unknown0x3cc;             // some sort of bizaare pointer...
/*0x56c*/
};

inline namespace deprecated {
	using EQGUILDWINDOW DEPRECATE("Use CGuildMgmtWnd instead of EQGUILDWINDOW") = CGuildMgmtWnd;
	using PEQGUILDWINDOW DEPRECATE("Use CGuildMgmtWnd* instead of PEQGUILDWINDOW") = CGuildMgmtWnd*;
}

//============================================================================
// CHelpWnd
//============================================================================

class CHelpWnd : public CSidlScreenWnd
{
public:
	CHelpWnd(CXWnd*);
	virtual ~CHelpWnd();

	virtual int HandleKeyboardMsg(uint32_t, uint32_t, bool) override;
	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT void SetFile(CXStr);
};

//============================================================================
// CHotButtonWnd
//============================================================================

// size: 0x460 @ 6/24/2024
class [[offsetcomments]] CHotButtonWnd : public CSidlScreenWnd, public PopDialogHandler
{
public:
	CHotButtonWnd(CXWnd*);
	virtual ~CHotButtonWnd();

	virtual int Draw() override;
	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT void DoHotButton(int buttonIndex, BOOL bMouseClick, const KeyCombo* keyCombo);
	EQLIB_OBJECT void DoHotButtonRightClick(int);
	EQLIB_OBJECT void UpdatePage();
	EQLIB_OBJECT void SetCheck(bool checked);

	//----------------------------------------------------------------------------
	// data members

/*0x2d8*/ CXWnd*          NoSpinnerBarTemplate;            // HB_NoSpinnerBarTemplate
/*0x2e0*/ CXWnd*          HorizontalBarTemplate;           // HB_HorizontalBarTemplate
/*0x2e8*/ CXWnd*          VerticalBarTemplate;             // HB_VerticalBarTemplate
/*0x2f0*/ CTileLayoutWnd* HotButtonLayout;                 // HB_HotButtonLayout
/*0x2f8*/ CXWnd*          HorizontalBarPageButtons;        // HB_HorizontalBarPageButtons
/*0x300*/ CXWnd*          VerticalBarPageButtons;          // HB_VerticalBarPageButtons
/*0x308*/ CButtonWnd*     PageUpButton;                    // HB_PageUpButton
/*0x310*/ CLabel*         HorizontalCurrentPageLabel;      // HB_HorizontalCurrentPageLabel
/*0x318*/ CButtonWnd*     PageDownButton;                  // HB_PageDownButton
/*0x320*/ CButtonWnd*     PageLeftButton;                  // HB_PageLeftButton
/*0x328*/ CLabel*         VerticalCurrentPageLabel;        // HB_VerticalCurrentPageLabel
/*0x330*/ CButtonWnd*     PageRightButton;                 // HB_PageRightButton
/*0x338*/ int             Page;
/*0x340*/ CHotButton*     Buttons[HOTBUTTONS_PER_PAGE];    // HB_Button%d
/*0x3a0*/ int             LoadLoadoutContextIndex;
/*0x3a4*/ int             SaveLoadoutContextIndex;
/*0x3a8*/ int             DeleteLoadoutContextIndex;
/*0x3ac*/ int             SaveLoadoutIndex;
/*0x3b0*/ int             ShowKeyMapIndex;
/*0x3b4*/ int             ShowSpinnerIndex;
/*0x3b8*/ int             ButtonPercent;
/*0x3bc*/ int             OpenNewBarIndex;
/*0x3c0*/ bool            ShowKeyMap;
/*0x3c1*/ bool            ShowSpinner;
/*0x3c2*/ bool            LastShowSpinner;
/*0x3c4*/ FontStyles      TextFontStyle;
/*0x3c8*/ CXStr           KeyMapStrings[HOTBUTTONS_PER_PAGE];
/*0x428*/ CButtonWnd*     FileButton;                      // HB_FileButton
/*0x430*/ CContextMenu*   MainMenu;
/*0x438*/ CContextMenu*   LoadMenu;
/*0x440*/ CContextMenu*   SaveMenu;
/*0x448*/ CContextMenu*   DeleteMenu;
/*0x450*/ bool            HorizontalBar;
/*0x454*/ uint32_t        Timer;
/*0x458*/ int             HotWindowIndex;
/*0x45c*/ int             ConfirmId;
/*0x460*/ bool            KeepCurrentSize;
/*0x464*/
};

//============================================================================
// CInspectWnd
//============================================================================

// size: 0x318
class [[offsetcomments]] CInspectWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	ItemContainer& GetInspectItems() { return inspectItems; }

/*0x2d4*/ uint32_t           nextRefreshTime;
/*0x2d8*/ uint32_t           lastInspectTextSaveTime;
/*0x2e0*/ PlayerClient*      inspectPlayer;
/*0x2e8*/ ItemContainer      inspectItems;
/*0x310*/ CEditWnd*          inspectEdit;
/*0x318*/ CButtonWnd*        doneButton;
/*0x320*/
};

//============================================================================
// CInventoryWnd
//============================================================================

// size: 0x15e8
class [[offsetcomments]] CInventoryWnd : public CGFScreenWnd, public PopDialogHandler, public WndEventHandler, public IObserver
{
public:
	CInventoryWnd(CXWnd*);
	virtual ~CInventoryWnd();

	virtual int Draw() override;
	virtual int HandleLButtonUp(const CXPoint&, uint32_t) override;
	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT long GetInventoryQtyFromCoinType(int);
	EQLIB_OBJECT void ClickedMoneyButton(int, int);
	EQLIB_OBJECT void DestroyHeld();
	EQLIB_OBJECT void Init();
	EQLIB_OBJECT void UpdateMoneyDisplay();

/*0x03cc*/ uint8_t     Unknown0x03ac[0xdbc];
/*0x1188*/ int64_t     VitalityCap;
/*0x1190*/ int         AAVitalityCap;
/*0x1194*/
};

inline namespace deprecated {
	using INVENTORYWND DEPRECATE("Use CInventoryWnd instead of INVENTORYWND") = CInventoryWnd;
	using PINVENTORYWND DEPRECATE("Use CInventoryWnd* instead of PINVENTORYWND") = CInventoryWnd*;
}

//============================================================================
// CInvSlotWnd
//============================================================================

class CInvSlotWnd;

class [[offsetcomments]] CInvSlot
{
public:
	CInvSlot();
	virtual ~CInvSlot();

	EQLIB_OBJECT bool IllegalBigBank(int);
	EQLIB_OBJECT void HandleLButtonDown(const CXPoint&);
	EQLIB_OBJECT void HandleLButtonHeld(const CXPoint&);
	EQLIB_OBJECT void HandleLButtonUp(const CXPoint&, bool);
	EQLIB_OBJECT void HandleLButtonUpAfterHeld(const CXPoint&);
	EQLIB_OBJECT void HandleRButtonDown(const CXPoint&);
	EQLIB_OBJECT void HandleRButtonHeld(const CXPoint&);
	EQLIB_OBJECT void HandleRButtonUp(const CXPoint&);
	EQLIB_OBJECT void HandleRButtonUpAfterHeld(const CXPoint&);
	EQLIB_OBJECT void SetInvSlotWnd(CInvSlotWnd*);
	EQLIB_OBJECT void SetItem(const ItemPtr&);
	EQLIB_OBJECT void SliderComplete(int);
	EQLIB_OBJECT void UpdateItem();

	EQLIB_OBJECT ItemPtr GetItem();

	// Retrieves the global index of this item slot
	EQLIB_OBJECT ItemGlobalIndex GetItemLocation() const;

	//----------------------------------------------------------------------------
	// data members

/*0x08*/ CInvSlotWnd*       pInvSlotWnd;
/*0x10*/ CTextureAnimation* pInvSlotAnimation;
/*0x18*/ int                Index;                    // InvSlot
/*0x1c*/ bool               bEnabled;                 // Valid
/*0x20*/
};

inline namespace deprecated {
	using EQINVSLOT DEPRECATE("Use CInvSlot instead of EQINVSLOT") = CInvSlot;
	using PEQINVSLOT DEPRECATE("Use CInvSlot* instead PEQINVSLOT") = CInvSlot*;
}

const int MAX_INV_SLOTS = 2304;

//----------------------------------------------------------------------------

class [[offsetcomments]] CInvSlotMgr
{
public:
	CInvSlotMgr();
	virtual ~CInvSlotMgr();

	EQLIB_OBJECT CInvSlot* CreateInvSlot(CInvSlotWnd*);
	EQLIB_OBJECT CInvSlot* FindInvSlot(int TopSlot, int SubSlot = -1,
		ItemContainerInstance location = eItemContainerPossessions, bool includeLinks = true);

	inline CInvSlot* FindInvSlot(const ItemGlobalIndex& index, bool includeLinks = true)
	{
		return FindInvSlot(index.GetTopSlot(), index.GetIndex().GetSlot(1), index.GetLocation(), includeLinks);
	}

	EQLIB_OBJECT bool MoveItem(const ItemGlobalIndex& from, const ItemGlobalIndex& to, bool bDebugOut = true,
		bool CombineIsOk = true, bool MoveFromIntoToBag = false, bool MoveToIntoFromBag = false);
	EQLIB_OBJECT void Process();
	EQLIB_OBJECT void SelectSlot(CInvSlot*);
	EQLIB_OBJECT void UpdateSlots();

	//----------------------------------------------------------------------------
	// data members

/*0x0008*/ CInvSlot*    SlotArray[MAX_INV_SLOTS]; // size 0x2400 //see 72E00F in Nov 06 2018 Test
/*0x4808*/ int          TotalSlots;
/*0x480c*/ int          unknown;
/*0x4810*/ unsigned int LastUpdate;
/*0x4818*/ CInvSlot*    pSelectedItem;
/*0x4820*/ CInvSlot*    pFindSelectedItem;
/*0x4828*/ bool         bToggleBagsOpen;
/*0x4829*/ bool         bToggleBankBagsOpen;
/*0x482c*/
};

inline namespace deprecated {
	using EQINVSLOTMGR DEPRECATE("Use CInvSlotMgr instead of EQINVSLOTMGR") = CInvSlotMgr;
	using PEQINVSLOTMGR DEPRECATE("Use CInvSlotMgr* instead of PEQINVSLOTMGR") = CInvSlotMgr*;
}

//----------------------------------------------------------------------------

class [[offsetcomments]] CInvSlotWnd : public CButtonWnd
{
	FORCE_SYMBOLS

public:
	CInvSlotWnd(CXWnd* pParent, uint32_t ID, CXRect rect, CTextureAnimation* ptaBackground,
		const ItemGlobalIndex& itemLocation, int ItemOffsetX, int ItemOffsetY);
	virtual ~CInvSlotWnd();

	virtual int Draw() override;
	virtual int DrawTooltip(const CXWnd*) const override;
	virtual int HandleLButtonDown(const CXPoint&, uint32_t) override;
	virtual int HandleLButtonHeld(const CXPoint&, uint32_t) override;
	virtual int HandleLButtonUp(const CXPoint&, uint32_t) override;
	virtual int HandleLButtonUpAfterHeld(const CXPoint&, uint32_t) override;
	virtual int HandleRButtonDown(const CXPoint&, uint32_t) override;
	virtual int HandleRButtonHeld(const CXPoint&, uint32_t) override;
	virtual int HandleRButtonUp(const CXPoint&, uint32_t) override;
	virtual int HandleRButtonUpAfterHeld(const CXPoint&, uint32_t) override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;
	virtual void SetAttributesFromSidl(CParamScreenPiece*) override;

	EQLIB_OBJECT void SetInvSlot(CInvSlot*);

	//----------------------------------------------------------------------------
	// data members

	// size: 0xa8
	struct [[offsetcomments]] GameFaceComponent
	{
	/*0x00*/ eqstd::string   str_00;
	/*0x20*/ eqstd::string   str_20;
	/*0x40*/ eqstd::string   str_40;
	/*0x60*/ uint64_t        u64_60;
	/*0x68*/ uint32_t        u32_68;
	/*0x6c*/ uint32_t        u32_6c;
	/*0x70*/ uint32_t        u32_70;
	/*0x74*/ uint32_t        u32_74;
	/*0x78*/ uint32_t        u32_78;
	/*0x7c*/ uint32_t        u32_7c;
	/*0x80*/ uint32_t        u32_80;
	/*0x88*/ uint64_t        u64_88;
	/*0x90*/ uint64_t        u64_90;
	/*0x98*/ uint64_t        u64_98;
	/*0xa0*/ uint64_t        u64_a0;
	/*0xa8*/
	};

/*0x348*/ GameFaceComponent  component;
/*0x3f0*/ CTextureAnimation* pBackground;
/*0x3f8*/ ItemGlobalIndex    ItemLocation;            // WindowType = ItemLocation.Location, InvSlot = ItemLocation.GetTopSlot()
/*0x408*/ ItemPtr            LinkedItem;              // If the slot is linked to a specific item
/*0x418*/ int                ItemOffsetX;
/*0x41c*/ int                ItemOffsetY;
/*0x420*/ CTextureAnimation* ptItem;
/*0x428*/ int                Quantity;
/*0x42c*/ bool               bSelected;
/*0x42d*/ bool               bFindSelected;
/*0x430*/ int                RecastLeft;
/*0x434*/ bool               bHotButton;
/*0x435*/ bool               bInventorySlotLinked;
/*0x438*/ CInvSlot*          pInvSlot;
/*0x440*/ CTextObjectInterface* pTextObject;
/*0x448*/ int                TextFontStyle;
/*0x44c*/ int                Mode;
/*0x450*/ D3DCOLOR           BGTintRollover;
/*0x454*/ D3DCOLOR           BGTintNormal;
/*0x458*/ int                LastTime;
/*0x45c*/ int                Unknown0x2cc;
/*0x460*/

	inline bool IsHotButton() const { return bHotButton; }

	// Backwards Compat Macros
	ALT_MEMBER_GETTER_COPY(ItemContainerInstance, ItemLocation.GetLocation(), WindowType);
	ALT_MEMBER_GETTER_COPY(ItemContainerInstance, ItemLocation.GetLocation(), Location);
	ALT_MEMBER_GETTER_COPY(short, ItemLocation.GetIndex().GetSlot(0), Slot1);
	ALT_MEMBER_GETTER_COPY(short, ItemLocation.GetIndex().GetSlot(0), InvSlot);
	ALT_MEMBER_GETTER_COPY(short, ItemLocation.GetIndex().GetSlot(1), Slot2);
	ALT_MEMBER_GETTER_COPY(short, ItemLocation.GetIndex().GetSlot(1), BagSlot);
	ALT_MEMBER_GETTER_COPY(short, ItemLocation.GetIndex().GetSlot(2), Slot3);
	ALT_MEMBER_GETTER_COPY(short, ItemLocation.GetIndex().GetSlot(2), GlobalSlot);
};

inline namespace deprecated {
	using EQINVSLOTWND DEPRECATE("Use CInvSlotWnd instead of EQINVSLOTWND") = CInvSlotWnd;
	using PEQINVSLOTWND DEPRECATE("Use CInvSlotWnd* instead of PEQINVSLOTWND") = CInvSlotWnd*;
}

//============================================================================
// CItemDisplayWnd
//============================================================================

enum ItemDisplayFlags
{
	PREVENT_LINK       = 0x00000001,
	RECYCLE_WINDOW     = 0x00000002,
	FROM_LINK          = 0x00000004,
	FROM_BAZAAR_SEARCH = 0x00000008,
	FROM_BARTER_SEARCH = 0x00000010
};

// @sizeof(CItemDisplayWnd) == 0xaa8 :: 2024-07-22 (live) @ 0x1404120c4
constexpr size_t CItemDisplayWnd_size = 0xaa8;

class [[offsetcomments]] CItemDisplayWnd : public CSidlScreenWnd
{
	FORCE_SYMBOLS

public:
	//EQLIB_OBJECT virtual int HandleKeyboardMsg(uint32_t, uint32_t, bool) override;
	//EQLIB_OBJECT virtual int OnProcessFrame() override;
	//EQLIB_OBJECT virtual int WndNotification(CXWnd* pWnd, uint32_t Message, void* pData) override;
	//EQLIB_OBJECT virtual bool AboutToShow() override;

	EQLIB_OBJECT CXStr CreateEquipmentStatusString(const ItemPtr&);
	EQLIB_OBJECT void SetItem(const ItemPtr& pItem, int flags);
	EQLIB_OBJECT void SetItemText(char*);
	EQLIB_OBJECT void UpdateStrings();

	EQLIB_OBJECT CXStr CreateClassString(EQ_Equipment*);
	EQLIB_OBJECT CXStr CreateMealSizeString(EQ_Equipment*);
	EQLIB_OBJECT CXStr CreateModString(EQ_Equipment*, int, int, int*);
	EQLIB_OBJECT CXStr CreateRaceString(EQ_Equipment*);
	EQLIB_OBJECT void GetSizeString(int, char*);
	EQLIB_OBJECT void InsertAugmentRequest(int AugSlot);
	EQLIB_OBJECT void RemoveAugmentRequest(int AugSlot);
	EQLIB_OBJECT void RequestConvertItem();

	//----------------------------------------------------------------------------
	// data members

	enum {
		NumStatRows = 26,
		NumStatCols = 3,
		NumHeroic = 13,
		NumItemInfo = 13,
	};

/*0x2d0*/ CStmlWnd*         Description;                   // ItemDescription
/*0x2d8*/ CButtonWnd*       IconButton;                    // IconButton
/*0x2e0*/ CStmlWnd*         ItemLore;                      // ItemLore
/*0x2e8*/ CTabWnd*          ItemDescriptionTabBox;         // ItemDescriptionTabBox
/*0x2f0*/ CPageWnd*         ItemDescriptionTab;            // ItemDescriptionTab
/*0x2f8*/ CPageWnd*         ItemLoreTab;                   // ItemLoreTab
/*0x300*/ CSidlScreenWnd*   pAppearanceSocketScreen;       // IDW_Appearance_Socket_Screen
/*0x308*/ CButtonWnd*       pAppearanceSocketItem;         // IDW_Appearance_Socket_Item
/*0x310*/ CButtonWnd*       pAppearanceSocketBuyButton;    // IDW_Appearance_Socket_Buy_Button
/*0x318*/ CStmlWnd*         pAppearanceSocketDescription;  // IDW_Appearance_Socket_Description
/*0x320*/ CSidlScreenWnd*   pItemSocketScreen[6];          // IDW_Socket_Slot_%d_Screen
/*0x350*/ CButtonWnd*       pItemSocketItemButton[6];      // IDW_Socket_Slot_%d_Item
/*0x380*/ CStmlWnd*         pItemSocketDescription[6];     // IDW_Socket_Slot_%d_Description
/*0x3b0*/ CXStr             ItemInfo;                      // "Error: Item description not set" -- "this item is placable in yards, guild yards...", "This item can be used in tradeskills", etc
/*0x3b8*/ CXStr             Unknown0x2ac;
/*0x3c0*/ CXStr             Unknown0x2b0;
/*0x3c8*/ CXStr             WindowTitle;                   // "Item Display"
/*0x3d0*/ CXStr             ItemAdvancedLoreText;          // "Error: Item lore not set"
/*0x3d8*/ CXStr             ItemMadeByText;
/*0x3e0*/ CXStr             SolventText;
/*0x3e8*/ CXStr             ItemInformationText;           // "Item Information: Placing this augment into...", "this armor can only be used in...", etc
/*0x3f0*/ ItemPtr           pItem;
/*0x400*/ bool              bActiveItem;
/*0x401*/ bool              bItemTextSet;
/*0x408*/ CTextureAnimation* DragIcons;                    // A_DragItem
/*0x410*/ bool              bTaggable;
/*0x411*/ bool              bFailed;
/*0x414*/ unsigned int      TabCount;
/*0x418*/ CLabel*           ModButtonLabel;                // IDW_ModButtonLabel // 3e8
/*0x420*/ CStmlWnd*         RewardButtonLabel;             // IDW_RewardButtonLabel
/*0x428*/ CStmlWnd*         ConvertStml;                   // IDW_ConvertButtonLabel
/*0x430*/ CLabel*           FuseButtonLabel;               // IDW_FuseButtonLabel
/*0x438*/ CLabel*           MadeByLabel;                   // IDW_MadeBy
/*0x440*/ CLabel*           CollectedLabel;                // IDW_CollectedLabel
/*0x448*/ CLabel*           ScribedLabel;                  // IDW_ScribedLabel
/*0x450*/ int               Row;
/*0x454*/ bool              bAntiTwink;
/*0x458*/ CButtonWnd*       ModButton;                     // IDW_ModButton
/*0x460*/ CButtonWnd*       RewardButton;                  // IDW_RewardButton
/*0x468*/ CButtonWnd*       PrintRealEstateItems;          // IDW_PrintRealEstateItems
/*0x470*/ CButtonWnd*       ConvertButton;                 // IDW_ConvertButton
/*0x478*/ CButtonWnd*       FuseButton;                    // IDW_FuseButton // 448
/*0x480*/ bool              bCollected;
/*0x481*/ bool              bCollectedReceived;
/*0x484*/ int               Unknown0x0314;
/*0x488*/ CXStr             ItemName;
/*0x490*/ bool              bScribed;
/*0x491*/ bool              bScribedReceived;
/*0x494*/ int               group[6];
/*0x4ac*/ int               showModified;
/*0x4b0*/ int               heroicCount;
/*0x4b4*/ int               itemInfoCount;
/*0x4b8*/ CStmlWnd*         lblItemInfo[NumItemInfo];      // IDW_ItemInfo%d
/*0x520*/ CLabel*           lblStat[NumStatRows][NumStatCols];      // IDW_Row%dCol%dStat
/*0x790*/ CLabel*           lblValue[NumStatRows][NumStatCols];     // IDW_Row%dCol%dValue
/*0xa00*/ CLabel*           lblHeroic[NumHeroic];          // IDW_Heroic%d 9d0
/*0xa68*/ int               rightClickMenuId;
/*0xa6c*/ int               rightClickMenuSocketId;
/*0xa70*/ CComboWnd*        ItemCompareList;               // IDW_ItemCompareList // a40
/*0xa78*/ bool              isComparing;                   // this and the next are a guess. Need to verify.
/*0xa80*/ ItemPtr           pCompareItem;
/*0xa90*/ CTAFrameDraw*     ptaPageBorderHighlight;        // FT_DefPageBorderHighlight // a60
/*0xa98*/ CTAFrameDraw*     ptaPageBorder;                 // FT_DefPageBorder // a68
/*0xaa0*/ uint32_t          ItemWndIndex;
/*0xaa4*/
};

SIZE_CHECK(CItemDisplayWnd, CItemDisplayWnd_size);

//============================================================================
// CJournalWnd
//============================================================================

class JournalNPC;

class CJournalCatWnd : public CSidlScreenWnd
{
public:
	CJournalCatWnd(CXWnd*);
	virtual ~CJournalCatWnd();

	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT void Clean();
	EQLIB_OBJECT void UpdateAll(bool);
	EQLIB_OBJECT void UpdateButtons();
	EQLIB_OBJECT void BuildList();
	EQLIB_OBJECT void LoadINI();
	EQLIB_OBJECT void SelectCategory(int);
	EQLIB_OBJECT void SortList(int, bool);
	EQLIB_OBJECT void StoreINI();
	EQLIB_OBJECT void UpdateListWnd(bool);
};

class CJournalNPCWnd : public CSidlScreenWnd
{
public:
	CJournalNPCWnd(CXWnd*);
	virtual ~CJournalNPCWnd();

	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT void EnterIntoJournal(char*, float, float, float, char*);
	EQLIB_OBJECT void LoadJournal(int);
	EQLIB_OBJECT void SaveJournal();
	EQLIB_OBJECT void UpdateAll(bool);
	EQLIB_OBJECT void UpdateCategories();
	EQLIB_OBJECT void BuildList();
	EQLIB_OBJECT void DoBackups(CXStr);
	EQLIB_OBJECT void GetLogState();
	EQLIB_OBJECT void SelectNPCIndex(int);
	EQLIB_OBJECT void SortList(int, bool);
	EQLIB_OBJECT void StoreLogState();
	EQLIB_OBJECT void UpdateButtons();
	EQLIB_OBJECT void UpdateListWnd(bool);
};

class CJournalTextWnd : public CSidlScreenWnd
{
public:
	CJournalTextWnd(CXWnd*);
	virtual ~CJournalTextWnd();

	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT void Clear();
	EQLIB_OBJECT void DisplayNPC(JournalNPC*);
	EQLIB_OBJECT void SetSearch(CXStr);
	EQLIB_OBJECT void UpdateAll(bool);
	EQLIB_OBJECT void UpdateCategories();
	EQLIB_OBJECT void BuildList();
	EQLIB_OBJECT void SelectEntryIndex(int);
	EQLIB_OBJECT void SortList(int, bool);
	EQLIB_OBJECT void UpdateButtons();
	EQLIB_OBJECT void UpdateListWnd(bool);
};

//============================================================================
// CKeyRingWnd
//============================================================================

// @sizeof(CKeyRingWnd) == 0x430 :: 2024-07-22 (live) @ 0x14018cea5
constexpr size_t CKeyRingWnd_size = 0x430;

class [[offsetcomments]] CKeyRingWnd : public CSidlScreenWnd, public WndEventHandler
{
public:
	EQLIB_OBJECT CKeyRingWnd(CXWnd* parent);
	EQLIB_OBJECT virtual ~CKeyRingWnd();

	EQLIB_OBJECT static int ExecuteRightClick(KeyRingType keyRingType, const ItemPtr& pItem, int index);

	EQLIB_OBJECT CListWnd* GetKeyRingList(KeyRingType type) const;

	enum KeyRingPages
	{
		ePageMountKeyRing = 0,
		ePageIllusionKeyRing = 1,
		ePageFamiliarKeyRing = 2,
		ePageHeroForgeKeyRing = 3,
		ePageTeleportationItemsKeyRing = 4,

		eNumPages,
		ePageFirst = ePageMountKeyRing,
		ePageLast = ePageTeleportationItemsKeyRing,
	};

/*0x2d8*/ CListWnd*     pList[eNumPages];        // KRW_Mounts_List, KRW_Illusions_List, KRW_Familiars_List, KRW_HeroForge_List
/*0x300*/ CLabel*       pStatLabel[eNumPages];   // KRW_Mounts_StatsNameLabel, KRW_Illusions_StatNameLabel, KRW_Familiars_StatNameLabel,
/*0x328*/ CLabel*       pCountLabel[eNumPages];  // KRW_Mounts_CountLabel, KRW_Illusions_CountLabel, KRW_Familiars_CountLabel, KRW_HeroForge_CountLabel
/*0x350*/ CButtonWnd*   pBtnStat[eNumPages];     // KRW_Mounts_Selected, KRW_Illusions_Selected, KRW_Familiars_Selected, KRW_HeroForge_SlotButton
/*0x378*/ CButtonWnd*   pBtnSlot[eNumPages];     // KRW_Mounts_SlotButton, KRW_Illusions_SlotButton, KRW_Familiars_SlotButton
/*0x3a0*/ uint8_t       Unknown0x28c[0x20];
/*0x3c0*/ CButtonWnd*   pItemBtnTemplate;                // KRW_ItemBtnTemplate
/*0x3c8*/ CButtonWnd*   pBtnFamiliarLeave;               // KRW_Familiars_LeaveButton
/*0x3d0*/ CButtonWnd*   pBtnFamiliarAutoLeave;           // KRW_Familiars_AutoLeaveButton
/*0x3d8*/ CListWnd*     pKeysList;                       // KRW_Keys_List
/*0x3e0*/ uint8_t       Unknown0x2b4[0x8];
/*0x3e8*/ int           LastUpdateTime;                  // timestamp of when the current tab was last updated.
/*0x3f0*/ CPageWnd*     pPageMounts;                     // KRW_Mounts_Page
/*0x3f8*/ CPageWnd*     pPageIllusions;                  // KRW_Illusions_Page
/*0x400*/ CPageWnd*     pPageFamiliars;                  // KRW_Familiars_Page
/*0x408*/ CPageWnd*     pPageKeys;                       // KRW_Keys_Page
/*0x410*/ CPageWnd*     pPageHeroForge;                  // KRW_HeroForge_Page
/*0x418*/ CPageWnd*     pPageTeleportation;              // KRW_TeleportationItems_Page
/*0x420*/ CTabWnd*      pTabBox;                         // KRW_Subwindows
/*0x428*/ KeyRingPages  CurrentPage;
/*0x42c*/
};

SIZE_CHECK(CKeyRingWnd, CKeyRingWnd_size);

//============================================================================
// ClaimWindow
//============================================================================

class [[offsetcomments]] ClaimWindow : public CSidlScreenWnd, public PopDialogHandler
{
public:
/*0x2d8*/ ClaimFeatureDataCollection claimFeatureData;
/*0x2f0*/ CListWnd*                  pListFeatures;           // ClaimFeatureList
/*0x2f8*/ CListWnd*                  pListItems;              // ClaimListItems
/*0x300*/ CButtonWnd*                pRefreshButton;          // ClaimRefreshButton
/*0x308*/ CButtonWnd*                pClaimButton;            // ClaimClaimButton
/*0x310*/ CButtonWnd*                pCancelButton;           // ClaimCancelButton
/*0x318*/ CStmlWnd*                  pItemDescription;        // ClaimItemDescription
/*0x320*/
};

//============================================================================
// CLargeDialogWnd
//============================================================================

class CLargeDialogWnd : public CSidlScreenWnd
{
public:
	EQLIB_OBJECT void Open(bool bYesNoEnabled, CXStr DialogText, unsigned long closeTimer /* 0 means never */,
		CXStr DialogTitle, bool bShowVolumeControls, CXStr YesText, CXStr NoText);
};

//============================================================================
// CLootWnd
//============================================================================

struct loot_msg;

// @sizeof(CLootWnd) == 0xcc0 :: 2024-07-22 (live) @ 0x14018d172
constexpr size_t CLootWnd_size = 0xcc0;

class [[offsetcomments]] CLootWnd : public CSidlScreenWnd, public PopDialogHandler, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CLootWnd(CXWnd*);
	virtual ~CLootWnd();

	virtual int OnProcessFrame() override;
	virtual int PostDraw() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;

	EQLIB_OBJECT void AddItemToLootArray(const ItemPtr&);
	EQLIB_OBJECT void Deactivate(bool);
	EQLIB_OBJECT void LootAll(bool close);
	EQLIB_OBJECT void RequestLootSlot(int Slot, bool bAutoInventory);
	EQLIB_OBJECT void SlotLooted(int);
	EQLIB_OBJECT void FinalizeLoot();

	enum { NumLootSlots = 200 };

	//----------------------------------------------------------------------------
	// data members

/*0x2dc*/ bool              bServerClose;
/*0x2dd*/ bool              bLootingActive;
/*0x2e0*/ int               LootIndex[NumLootSlots];
/*0x600*/ uint32_t          nNextRefreshTime;
/*0x604*/ bool              bInventoryWasActive;
/*0x608*/ ItemContainer     LootItems;
/*0x630*/ bool              bCloseOnComplete;
/*0x631*/ bool              bRightClickLoot;
/*0x632*/ bool              bLootAllReady;
/*0x638*/ CXWnd*            LootInvWnd;                       // LootInvWnd
/*0x640*/ CInvSlotWnd*      LootSlotWnd[NumLootSlots];        // created from LW_LootSlotTemplate
/*0xc80*/ CLabel*           CorpseNameLabel;                  // LW_CorpseName
/*0xc88*/ CButtonWnd*       DoneButton;                       // DoneButton
/*0xc90*/ CButtonWnd*       BroadcastButton;                  // BroadcastButton
/*0xc98*/ CButtonWnd*       LootAllButton;                    // LootAllButton
/*0xca0*/ bool              bPopupPending;
/*0xca8*/ void*             Unknown1;
/*0xcb0*/ void*             Unknown2;
/*0xcb8*/ uint32_t          Unknown3;
/*0xcbc*/

	inline ItemContainer& GetLootItems() { return LootItems; }
	inline ItemPtr GetLootItem(int slot) { return LootItems.GetItem(slot); }

	INVENTORYARRAY* get_pInventoryArray() { return reinterpret_cast<INVENTORYARRAY*>(&LootItems.Items[0]); }
	__declspec(property(get = get_pInventoryArray)) INVENTORYARRAY* pInventoryArray;

	int get_Size() { return LootItems.GetSize(); }
	__declspec(property(get = get_Size)) int Size;

	DEPRECATE("CLootWnd: Use Loots.GetSize() instead of NumOfSlots")
	__declspec(property(get = get_Size)) int NumOfSlots;
};

inline namespace deprecated {
	using EQLOOTWINDOW DEPRECATE("Use CLootWnd instead of EQLOOTWINDOW") = CLootWnd;
	using PEQLOOTWINDOW DEPRECATE("Use CLootWnd* instead of PEQLOOTWINDOW") = CLootWnd*;
}

SIZE_CHECK(CLootWnd, CLootWnd_size);

//============================================================================
// CMapViewWnd
//============================================================================

// Map Window sizeof() = 0x38
struct [[offsetcomments]] MapViewLabel
{
/*0x00*/ uint32_t      LabelId;
/*0x08*/ MapViewLabel* pNext;
/*0x10*/ MapViewLabel* pPrev;
/*0x18*/ CVector3      Location;
/*0x24*/ ARGBCOLOR     Color;
/*0x28*/ int           Size;                     // 1-3;
/*0x30*/ char*         Label;
/*0x38*/ int           Layer;                    // 0-3;
/*0x3c*/ int           Width;
/*0x40*/ int           Height;
/*0x44*/ int           OffsetX;
/*0x48*/ int           OffsetY;
/*0x4c*/
};

// If you see:  'eqlib::MAPLINE': redefinition; different basic types
// Then make sure to change your code to use MapViewLabel instead of MAPLABEL;
using MAPLABEL DEPRECATE("Use MapViewLabel instead of MAPLABEL") = MapViewLabel;
using PMAPLABEL DEPRECATE("Use MapViewLabel* instead of PMAPLABEL") = MapViewLabel*;

// sizeof() = 0x28
struct [[offsetcomments]] MapViewLine
{
/*0x00*/ MapViewLine*  pNext;
/*0x08*/ MapViewLine*  pPrev;
/*0x10*/ CVector3      Start;
/*0x1c*/ CVector3      End;
/*0x28*/ ARGBCOLOR     Color;
/*0x2c*/ int           Layer;                    // 0-3;
/*0x30*/
};

// If you see: 'eqlib::MAPLINE': redefinition; different basic types
// Then make sure to change your code to use MapViewLine instead of MAPLINE;
using MAPLINE DEPRECATE("Use MapViewLine instead of MAPLINE") = MapViewLine;
using PMAPLINE DEPRECATE("Use MapViewLine* instead of PMAPLINE") = MapViewLine*;

// pLines address = 0x254 + 0x035c = 0x05b0 (address of pMapViewMapVfTable)
// MapViewMap_size: 0x848 - 0x4b8 = 0x390

class [[offsetcomments]] MapViewMap : public CSidlScreenWnd
{
	FORCE_SYMBOLS

public:
	// constructor / destructor
	MapViewMap();
	virtual ~MapViewMap();

	// virtual functions
	EQLIB_OBJECT virtual int PostDraw() override;
	EQLIB_OBJECT virtual int HandleLButtonDown(const CXPoint& pos, uint32_t flags) override;
	EQLIB_OBJECT virtual int HandleLButtonUp(const CXPoint& pos, uint32_t flags) override;
	EQLIB_OBJECT virtual int HandleLButtonUpAfterHeld(const CXPoint& pos, uint32_t flags) override;
	EQLIB_OBJECT virtual int HandleRButtonDown(const CXPoint& pos, uint32_t flags) override;
	EQLIB_OBJECT virtual int HandleWheelMove(const CXPoint& pos, int scroll, uint32_t flags) override;

	// methods
	EQLIB_OBJECT void Clear();
	//EQLIB_OBJECT void SaveEx(char*, int);
	EQLIB_OBJECT void SetZoom(float);
	EQLIB_OBJECT void GetWorldCoordinates(CVector3&);

	//EQLIB_OBJECT bool DrawClippedLine(CVector3*, RGB, CXRect);
	//EQLIB_OBJECT bool IsLayerVisible(int);
	//EQLIB_OBJECT bool LoadEx(char*, int);
	//EQLIB_OBJECT bool PointInMapViewArea(CXPoint, CXRect);
	//EQLIB_OBJECT int GetMaxZ();
	//EQLIB_OBJECT int GetMinZ();
	//EQLIB_OBJECT unsigned long GetCurrentColor();
	//EQLIB_OBJECT unsigned long GetMarkedLineColor();
	//EQLIB_OBJECT void AddLabel(float, float, float, unsigned long, int, char*);
	//EQLIB_OBJECT void AddPoint(float, float, float);
	//EQLIB_OBJECT void CalcLabelRenderOffsets(CXRect);
	//EQLIB_OBJECT void ClearActiveLayer();
	//EQLIB_OBJECT void Draw(CXRect);
	//EQLIB_OBJECT void EndLine(float, float, float);
	//EQLIB_OBJECT void JoinLinesAtIntersect(bool);
	//EQLIB_OBJECT void Load(char*);
	//EQLIB_OBJECT void MoveLabel(MapViewLabel*, float, float, float);
	//EQLIB_OBJECT void PreCalcRenderValues();
	//EQLIB_OBJECT void RecalcLabelExtents(MapViewLabel*);
	//EQLIB_OBJECT void RemoveLabel(MapViewLabel*);
	//EQLIB_OBJECT void RemoveLabel();
	//EQLIB_OBJECT void RemoveLine(MapViewLabel*);
	//EQLIB_OBJECT void RemoveLine();
	//EQLIB_OBJECT void Save(char*);
	//EQLIB_OBJECT void SetCurrentColor(unsigned long);
	//EQLIB_OBJECT void SetMarkedLine(MapViewLabel*);
	//EQLIB_OBJECT void SetMarkedLineColor(unsigned long);
	//EQLIB_OBJECT void SetZoneExtents(int, int, int, int);
	//EQLIB_OBJECT void StartLine(float, float, float);
	//EQLIB_OBJECT void TransformPoint(CVector3*);

/*0x2d0*/ float              mapViewMinX;
/*0x2d4*/ float              mapViewMinY;
/*0x2d8*/ float              mapViewMaxX;
/*0x2dc*/ float              mapViewMaxY;
/*0x2e0*/ int                panOffsetX;
/*0x2e4*/ int                panOffsetY;
/*0x2e8*/ float              lineOffsetX;
/*0x2ec*/ float              lineOffsetY;
/*0x2f0*/ float              scaleDiffX;
/*0x2f4*/ float              scaleDiffY;
/*0x2f8*/ float              mapViewScaleX;
/*0x2fc*/ float              mapViewScaleY;
/*0x300*/ MapViewLine*       pLines;
/*0x308*/ MapViewLabel*      pLabels;
/*0x310*/ uint32_t           nextLabelId;
/*0x314*/ bool               lineActive;
/*0x318*/ CVector3           currentPoint;
/*0x324*/ RGB                currentColor;
/*0x328*/ RGB                myColor;
/*0x32c*/ RGB                groupColor;
/*0x330*/ RGB                findPathColor;
/*0x334*/ bool               failedFindActive;
/*0x338*/ CVector3           failedFindPoint;
/*0x344*/ int                failedFindType;
/*0x348*/ MapViewLine*       markedLines;
/*0x350*/ MapViewLabel*      markedLabel;
/*0x358*/ MapViewLabel*      selectedLabel;
/*0x360*/ int                layerFilter;              // bitmask of active layers
/*0x364*/ int                activeLayer;
/*0x368*/ int                minZ;
/*0x36c*/ int                maxZ;
/*0x370*/ bool               zFilterActive;
/*0x374*/ float              zoom;
/*0x378*/ float              recalcZoom;
/*0x37c*/ int                range;
/*0x380*/ bool               showGroup;
/*0x381*/ bool               showNames;
/*0x382*/ bool               findAutoZoom;
/*0x383*/ bool               showLabels;
/*0x384*/ bool               recalcLabels;
/*0x385*/ bool               canAddToMap;
/*0x388*/ EQZoneIndex        zoneId;
/*0x390*/ CXStr              customMapPath;
/*0x398*/

	// points to the eq instance of the virtual function table for this class
	static VirtualFunctionTable* sm_vftable;
};

// @sizeof(CMapViewWnd) == 0x858 :: 2024-07-22 (live) @ 0x14018c899
constexpr size_t CMapViewWnd_size = 0x858;

class [[offsetcomments]] CMapViewWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	// constructor / destructor
	EQLIB_OBJECT CMapViewWnd(CXWnd*);
	EQLIB_OBJECT virtual ~CMapViewWnd();

	// virtual functions
	virtual bool AboutToShow() override;
	virtual bool AboutToHide() override;
	virtual int OnShow() override;
	virtual int OnMove(const CXRect&) override;
	virtual int OnProcessFrame() override;
	virtual int WndNotification(CXWnd*, uint32_t, void*) override;
	virtual void OnWndNotification() override;
	virtual void LoadIniInfo() override;
	virtual void StoreIniInfo() override;

	// functions
	EQLIB_OBJECT void ActivateAutoMapping();
	EQLIB_OBJECT void DeactivateAutoMapping();
	EQLIB_OBJECT bool IsMappingEnabled();

	EQLIB_OBJECT void SetCurrentZone(EQZoneIndex, CVector3* v3Min, CVector3* v3Max, bool);

	// these are almost all the controls belonging to the CMapViewWnd
/*0x2d4*/ int         ZoneId;
/*0x2d8*/ bool        bAutoMapping;
/*0x2e0*/ CButtonWnd* btnPanLeft;
/*0x2e8*/ CButtonWnd* btnPanRight;
/*0x2f0*/ CButtonWnd* btnPanUp;
/*0x2f8*/ CButtonWnd* btnPanDown;
/*0x300*/ CButtonWnd* btnPanReset;
/*0x308*/ CButtonWnd* btnZoomIn;
/*0x310*/ CButtonWnd* btnZoomOut;
/*0x318*/ CButtonWnd* btnShowGroup;
/*0x320*/ CButtonWnd* btnShowNames;
/*0x328*/ CButtonWnd* btnFind;
/*0x330*/ CButtonWnd* btnEndFind;
/*0x338*/ CButtonWnd* btnFindAutoZoom;
/*0x340*/ CButtonWnd* btnMapZoneGuide;
/*0x348*/ CButtonWnd* btnToolbar;
/*0x350*/ char        szZoneName[0x80];
/*0x3d0*/ CButtonWnd* btntoggleLabels;
/*0x3d8*/ CEditWnd*   editMinZ;
/*0x3e0*/ CEditWnd*   editMaxZ;
/*0x3e8*/ CButtonWnd* btnZFilter;
/*0x3f0*/ CButtonWnd* btnLayer0;
/*0x3f8*/ CButtonWnd* btnLayer1;
/*0x400*/ CButtonWnd* btnLayer2;
/*0x408*/ CButtonWnd* btnLayer3;
/*0x410*/ CButtonWnd* btnLayer1Active;
/*0x418*/ CButtonWnd* btnLayer2Active;
/*0x420*/ CButtonWnd* btnLayer3Active;
/*0x428*/ CButtonWnd* btnLoadCurrent;
/*0x430*/ CPageWnd*   pageAtlas;
/*0x438*/ CPageWnd*   pageMap;
/*0x440*/ CTabWnd*    tabWnd;
/*0x448*/ CXWnd*      MapRenderArea;
/*0x450*/ CComboWnd*  MapsDirSelect;
/*0x458*/ CXWnd*      AtlasRenderArea;
/*0x460*/ CAtlas*     AtlasWnd;
/*0x468*/ CComboWnd*  AtlasDirSelect;
/*0x470*/ CButtonWnd* btnAtlasZoneGuide;
/*0x478*/ CButtonWnd* btnMapIcon;
/*0x480*/ CEditWnd*   editSearchBox;
/*0x488*/ CButtonWnd* btnSearch;
/*0x490*/ CButtonWnd* btnSearchClear;
/*0x498*/ CListWnd*   listSearch;
/*0x4a0*/ CXWnd*      wndSearchLayout;
/*0x4a8*/ CButtonWnd* btnSearchHide;
/*0x4b0*/ CXWnd*      wndSpacer;
/*0x4b8*/ bool        bEditing;
/*0x4c0*/ MapViewMap  MapView;                            // a window component owned by CMapViewWnd
/*0x858*/
	// alias the stupid
	__declspec(property(get = getLines)) MapViewLine* pLines;
	__declspec(property(get = getLabels)) MapViewLabel* pLabels;

	ALT_MEMBER_GETTER_ARRAY(char, 0x80, szZoneName, shortzonename);

	MapViewLine* getLines() { return MapView.pLines; }
	MapViewLabel* getLabels() { return MapView.pLabels; }
};

inline namespace deprecated {
	using EQMAPWINDOW DEPRECATE("Use CMapViewWnd instead of EQMAPWINDOW") = CMapViewWnd;
	using PEQMAPWINDOW DEPRECATE("Use CMapViewWnd* instead of PEQMAPWINDOW") = CMapViewWnd*;
}

SIZE_CHECK(CMapViewWnd, CMapViewWnd_size);

//============================================================================
// CMarketplaceWnd
//============================================================================

class [[offsetcomments]] CMarketplaceWnd : public CSidlScreenWnd
{
public:
};

//============================================================================
// CMerchantWnd
//============================================================================

enum eMerchantServices
{
	Regular,
	Recovery,
	Mail,
	ServiceCount
};

struct [[offsetcomments]] MerchantItemEntry
{
/*0x00*/ ItemPtr       pItem;
/*0x10*/ int           Unknown;
/*0x14*/

	ALT_MEMBER_GETTER_DEPRECATED(ItemClient*, pItem, pCont, "Use pItem instead of pCont");
};

// todo: finish mapping this and verify
class [[offsetcomments]] MerchantPageHandler
{
public:
/*0x08*/ CMerchantWnd*   pParent;
/*0x10*/ int             MaxItems;
/*0x14*/ int             LastIndex;
/*0x18*/ CListWnd*       ItemsList;
/*0x20*/ CPageWnd*       PurchasePage;
/*0x28*/ bool            bListNeedsRefresh;
/*0x30*/ SoeUtil::Array<MerchantItemEntry> ItemContainer;
/*0x48*/ int             Unknown0x40;
/*0x4c*/ int             Unknown0x44;
/*0x50*/ int             Unknown0x48;
/*0x54*/ int             Unknown0x4c;
/*0x58*/ int             Unknown0x50;
/*0x5c*/ int             Unknown0x54;
/*0x60*/ int             Unknown0x58;
/*0x64*/ int             Unknown0x5c;
/*0x68*/ int             Unknown0x60;
/*0x6c*/ int             Unknown0x64;
/*0x70*/ int             Unknown0x68;
/*0x74*/ int             Unknown0x6c;
/*0x78*/ int             Unknown0x70;
/*0x7c*/ int             Unknown0x74;
/*0x80*/ int             Unknown0x78;
/*0x84*/ int             Unknown0x7c;
/*0x88*/ int             Unknown0x80;
/*0x8c*/

	// offset comments indicate vtable offset
	/*0x08*/ EQLIB_OBJECT virtual void Unknownv0x08();
	/*0x0c*/ EQLIB_OBJECT virtual void Unknownv0x0c();
	/*0x10*/ EQLIB_OBJECT virtual void Unknownv0x10();
	/*0x14*/ EQLIB_OBJECT virtual void Unknownv0x14();
	/*0x18*/ EQLIB_OBJECT virtual void DestroyItemByUniqueId(int64_t UniqueID);
	/*0x1c*/ EQLIB_OBJECT virtual void DestroyItemByItemGuid(const EqItemGuid& ItemGuid);
	/*0x20*/ EQLIB_OBJECT virtual bool AddItemToArray(const ItemPtr& pSentItem);
	/*0x24*/ EQLIB_OBJECT virtual int Sort(SListWndSortInfo* SortInfo);
	/*0x28*/ EQLIB_OBJECT virtual void UpdateList();
	/*0x2c*/ EQLIB_OBJECT virtual int DisplayBuyOrSellPrice(const ItemPtr& pItem, bool bBuy) const;
	/*0x30*/ EQLIB_OBJECT virtual CXStr GetPriceString(int Price) const;
	/*0x34*/ EQLIB_OBJECT virtual void UpdateControls();
	/*0x38*/ EQLIB_OBJECT virtual bool RequestGetItem(int Qty);
	/*0x3c*/ EQLIB_OBJECT virtual void RequestPutItem(int Qty);
	/*0x40*/ EQLIB_OBJECT virtual bool CanSelectSlot(const ItemGlobalIndex& Location) const;
	/*0x44*/ EQLIB_OBJECT virtual void DisablePageSpecificButtons();
	/*0x48*/ EQLIB_OBJECT virtual eMerchantServices GetHandlerType() const;
	/*0x4c*/ EQLIB_OBJECT virtual void CXWnd__OnShowANDPostDraw() const;
	/*0x50*/ EQLIB_OBJECT virtual void Unknownv0x50() const;
	/*0x54*/ EQLIB_OBJECT virtual void Unknownv0x54() const;
	/*0x58*/ EQLIB_OBJECT virtual void Unknownv0x58() const;

	int GetItemCount() const
	{
		return ItemContainer.GetSize();
	}

	ItemPtr GetItem(int index) const
	{
		if (index >= 0 && index < ItemContainer.GetSize())
		{
			return ItemContainer[index].pItem;
		}

		return ItemPtr();
	}
};

// Size 0xa8 (0x7a331f) 2020-12-05
class [[offsetcomments]] PurchasePageHandler : public MerchantPageHandler
{
	FORCE_SYMBOLS;

public:
/*0x90*/ bool bShowAllItems;
/*0x94*/ int Unknown0x88;
/*0x98*/ int Unknown0x8c;
/*0x9c*/ int Unknown0x90;
/*0xa0*/ int Unknown0x94;
/*0xa4*/ int Unknown0x98;
/*0xa8*/ int Unknown0x9c;
/*0xac*/ int Unknown0xa0;
/*0xb0*/ int Unknown0xa4;
/*0xb4*/

	EQLIB_OBJECT bool RequestGetItem(int);
	EQLIB_OBJECT void RequestPutItem(int);
};

struct sell_msg;

enum MerchantPages
{
	RegularMerchantPage,
	BuybackMerchantPage,
	MailMerchantPage,

	MaxMerchantPages
};

// CMerchantWnd_size: 0x608 02-07-2022 test
class [[offsetcomments]] CMerchantWnd : public CSidlScreenWnd, public WndEventHandler, PopDialogHandler
{
	FORCE_SYMBOLS

public:
	using PageHandlerPtr = eqstd::shared_ptr<MerchantPageHandler>;
	using PageHandlerArray = VeArray<PageHandlerPtr>;

/*0x2dc*/ uint32_t           NextRefreshTime;
/*0x2e0*/ bool               bInventoryWasActive;
/*0x2e8*/ PageHandlerArray   PageHandlers;
/*0x300*/ float              MerchantGreed;
/*0x304*/ ItemGlobalIndex    ItemLocation;
/*0x310*/ BYTE               Unknown0x254[0x8];
/*0x318*/ ItemPtr            pSelectedItem;
/*0x328*/ eqtime_t           MailExpireTime;
/*0x330*/ bool               bAutoRetrieveingMail;
/*0x338*/ char*              Labels[14];                   // unknown - adjusted to align the next members
/*0x3a8*/ CEditWnd*          SearchEdit;                   // MW_ItemNameInput
/*0x3b0*/ CButtonWnd*        SearchButton;                 // MW_SearchItem_Butto
/*0x3b8*/ CLabel*            MerchantNameLabel;
/*0x3c0*/ CLabel*            SelectedItemLabel;
/*0x3c8*/ CLabel*            SelectedPriceLabel;
/*0x3d0*/ CButtonWnd*        InspectButton;
/*0x3d8*/ CButtonWnd*        PreviewButton;
/*0x3e0*/ CButtonWnd*        SelectedItemButton;
/*0x3e8*/ CButtonWnd*        BuyButton;
/*0x3f0*/ CButtonWnd*        BuyMarketPlaceButton;
/*0x3f8*/ CButtonWnd*        SellButton;
/*0x400*/ CButtonWnd*        RecoverButton;
/*0x408*/ CButtonWnd*        RetrieveButton;
/*0x410*/ CButtonWnd*        RetrieveAllButton;
/*0x418*/ CButtonWnd*        SendButton;
/*0x420*/ CButtonWnd*        AdventureButton;
/*0x428*/ CLabel*            SendToLabel;
/*0x430*/ CEditWnd*          SendToEdit;
/*0x438*/ CLabel*            NoteLabel;
/*0x440*/ CEditWnd*          NoteEdit;
/*0x448*/ CButtonWnd*        ClearNoteButton;
/*0x450*/ CListWnd*          ItemsList;
/*0x458*/ CListWnd*          ItemsRecoveryList;
/*0x460*/ CListWnd*          ItemsMailList;
/*0x468*/ CButtonWnd*        DoneButton;
/*0x470*/ CPageWnd*          PurchasePage;
/*0x478*/ CPageWnd*          RecoveryPage;
/*0x480*/ CPageWnd*          MailPage;
/*0x488*/ CTabWnd*           TabWindow;
/*0x490*/ CButtonWnd*        UsableButton;
/*0x498*/ CLabel*            CurrentCurrencyLabel;
/*0x4a0*/ int                Unknown0x32c;
/*0x4a4*/ int                Unknown0x330;
/*0x4a8*/ int                Unknown0x334;
/*0x4ac*/ int                Unknown0x338;
/*0x4b0*/ int                Unknown0x33C;
/*0x4b4*/ int                Guk_Currency;
/*0x4b8*/ uint8_t            Unknown0x340[0x100];
/*0x5b8*/

	CMerchantWnd(CXWnd*);
	virtual ~CMerchantWnd();

	EQLIB_OBJECT void AddContainerToMercArray(EQ_Container*);
	EQLIB_OBJECT void AddEquipmentToMercArray(EQ_Equipment*);
	EQLIB_OBJECT void AddNoteToMercArray(EQ_Note*);
	EQLIB_OBJECT void ClearMerchantSlot(int);
	EQLIB_OBJECT void FinishBuyingItem(sell_msg*);
	EQLIB_OBJECT void FinishSellingItem(sell_msg*);
	EQLIB_OBJECT int SelectBuySellSlot(const ItemGlobalIndex&, int ListIndex = -1);
	EQLIB_OBJECT void DisplayBuyOrSellPrice(const ItemPtr& item, bool buy);
	EQLIB_OBJECT void HandleBuy(int);
	EQLIB_OBJECT void HandleSell(int);
	EQLIB_OBJECT void UpdateBuySellButtons();
};

inline namespace deprecated {
	using EQMERCHWINDOW DEPRECATE("Use CMerchantWnd instead of EQMERCHWINDOW") = CMerchantWnd;
	using PEQMERCHWINDOW DEPRECATE("Use CMerchantWnd* instead of PEQMERCHWINDOW") = CMerchantWnd*;
}

//============================================================================
// CMusicPlayerWnd
//============================================================================

class CMusicPlayerWnd : public CSidlScreenWnd
{
public:
	CMusicPlayerWnd(CXWnd*);
	virtual ~CMusicPlayerWnd();

	EQLIB_OBJECT void Update();
	EQLIB_OBJECT void AutoStart();
	EQLIB_OBJECT void UpdateButtons();
};

//============================================================================
// CNoteWnd
//============================================================================

class CNoteWnd : public CSidlScreenWnd
{
public:
	EQLIB_OBJECT CNoteWnd(CXWnd*);
	EQLIB_OBJECT ~CNoteWnd();

	EQLIB_OBJECT bool CheckNote(EQ_Note*);
	EQLIB_OBJECT void SetNote(char*);
};

//============================================================================
// COptionsWnd
//============================================================================

struct [[offsetcomments]] KeyboardAssignmentData
{
/*0x00*/ CXStr Description;
/*0x08*/ int nAssignmentNumber;
/*0x0c*/
};

inline namespace deprecated {
	using EQKBASSIGN DEPRECATE("Use KeyboardAssignmentData instead of EQKBASSIGN") = KeyboardAssignmentData;
	using PEQKBASSIGN DEPRECATE("Use KeyboardAssignmentData* instead of PEQKBASSIGN") = KeyboardAssignmentData*;
}

// size 0x904 3-10-2004
class [[offsetcomments]] COptionsWnd : public CSidlScreenWnd, public PopDialogHandler, public WndEventHandler
{
public:
	COptionsWnd(CXWnd*);
	virtual ~COptionsWnd();

	EQLIB_OBJECT void RefreshCurrentKeyboardAssignmentList();
	EQLIB_OBJECT void ResetKeysToDefault();
	EQLIB_OBJECT void RestoreDefaultColors();
	EQLIB_OBJECT int ChatPageOnProcessFrame();
	EQLIB_OBJECT int ChatPageWndNotification(CXWnd*, uint32_t, void*);
	EQLIB_OBJECT int ColorPageOnProcessFrame();
	EQLIB_OBJECT int ColorPageWndNotification(CXWnd*, uint32_t, void*);
	EQLIB_OBJECT int DisplayPageOnProcessFrame();
	EQLIB_OBJECT int DisplayPageWndNotification(CXWnd*, uint32_t, void*);
	EQLIB_OBJECT int GeneralPageOnProcessFrame();
	EQLIB_OBJECT int GeneralPageWndNotification(CXWnd*, uint32_t, void*);
	EQLIB_OBJECT int KeyboardPageDeactivate();
	EQLIB_OBJECT int KeyboardPageHandleKeyboardMsg(uint32_t, uint32_t, bool);
	EQLIB_OBJECT int KeyboardPageOnProcessFrame();
	EQLIB_OBJECT int KeyboardPageWndNotification(CXWnd*, uint32_t, void*);
	EQLIB_OBJECT int MousePageOnProcessFrame();
	EQLIB_OBJECT int MousePageWndNotification(CXWnd*, uint32_t, void*);
	EQLIB_OBJECT int RedirectDeactivateTo(CPageWnd*);
	EQLIB_OBJECT int RedirectHandleKeyboardMsgTo(CPageWnd*, uint32_t, uint32_t, bool);
	EQLIB_OBJECT int RedirectOnProcessFrameTo(CPageWnd*);
	EQLIB_OBJECT int RedirectWndNotificationTo(CPageWnd*, CXWnd*, uint32_t, void*);
	EQLIB_OBJECT void AddKeyboardAssignment(int, int, int);
	EQLIB_OBJECT void InitKeyboardAssignments();
	EQLIB_OBJECT void InitKeyboardPage();
	EQLIB_OBJECT void KeyboardPageCancelKeypressAssignment();
	EQLIB_OBJECT void SetBagOptions(int, int);
	EQLIB_OBJECT void SyncChatPage();
	EQLIB_OBJECT void SyncColorPage();
	EQLIB_OBJECT void SyncDisplayPage();
	EQLIB_OBJECT void SyncGeneralPage();
	EQLIB_OBJECT void SyncMousePage();
	EQLIB_OBJECT void FillChatFilterList();

	//----------------------------------------------------------------------------
	// data members

	// this is incorrect and needs to be updated
/*0x2e0*/ KeyboardAssignmentData   Binds[0xA1];
/*0xcf0*/
/*0x904*/
};

inline namespace deprecated {
	using EQOPTIONSWINDOW DEPRECATE("Use COptionsWnd instead of EQOPTIONSWINDOW") = COptionsWnd;
	using PEQOPTIONSWINDOW DEPRECATE("Use COptionsWnd* instead of PEQOPTIONSWINDOW") = COptionsWnd*;
}

//============================================================================
// COverseerWnd
//============================================================================

class [[offsetcomments]] COverseerWnd : public CSidlScreenWnd
{
public:
};


//============================================================================
// CPetInfoWnd
//============================================================================

constexpr const int MAX_PET_BUTTONS = 14;

// @sizeof(CPetInfoWnd) == 0x3d0 :: 2024-07-22 (live) @ 0x14018c35f
constexpr size_t CPetInfoWnd_size = 0x3d0;

class [[offsetcomments]] CPetInfoWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CPetInfoWnd(CXWnd*);
	virtual ~CPetInfoWnd();

	EQLIB_OBJECT CButtonWnd* GetButton(int);
	EQLIB_OBJECT void SetShowOnSummon(bool);
	EQLIB_OBJECT void Update();

	PlayerBuffInfoWrapper GetBuffInfo(int buffIndex) const
	{
		if (buffIndex >= 0 && buffIndex < Buffs.GetSize())
			return PlayerBuffInfoWrapper(buffIndex, &Buffs[buffIndex]);

		return PlayerBuffInfoWrapper(buffIndex, nullptr);
	}

	PlayerBuffInfoWrapper GetBuffInfoBySpellID(int spellID) const
	{
		int buffIndex = 0;
		for (const PlayerBuffInfo& pbi : Buffs)
		{
			if (pbi.SpellID == spellID)
				return PlayerBuffInfoWrapper(buffIndex, &pbi);
			++buffIndex;
		}

		return PlayerBuffInfoWrapper(-1, nullptr);
	}

	int GetTotalBuffCount() const
	{
		int count = 0;
		for (const PlayerBuffInfo& pbi : Buffs)
		{
			if (pbi.SpellID > 0)
				++count;
		}
		
		return count;
	}

	int GetMaxBuffs() const { return Buffs.GetSize(); }

	int GetBuff(int buffIndex) const { return GetBuffInfo(buffIndex).GetSpellID(); }
	int GetBuffTimer(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffTimer(); }
	const char* GetCasterName(int buffIndex) const { return GetBuffInfo(buffIndex).GetCaster(); }
	CButtonWnd* GetBuffButton(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffButton(); }
	CTextureAnimation* GetBuffIcon(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffIcon(); }

	using BuffIterator = PlayerBuffInfoWrapper::Iterator<PlayerBuffInfoWrapper>;
	eqlib::IteratorRange<BuffIterator> GetBuffRange() const { return eqlib::make_iterator_range(BuffIterator(Buffs, 0), BuffIterator(Buffs, Buffs.GetSize())); }

	#pragma region Deprecated accessors
	//
	// Deprecated accessors
	//

	// Buff -> GetBuff()
	DEPRECATE("Use GetBuff(index) or GetBuffInfo(index) insetad of Buff[index]")
	int get_DeprecatedBuff(int buffIndex) const { return GetBuffInfo(buffIndex).GetSpellID(); }
	__declspec(property(get = get_DeprecatedBuff)) int Buff[];

	// PetBuffTimer -> GetBuffTimer()
	DEPRECATE("Use GetBuffTimer(index) or GetBuffInfo(index) instead of PetBuffTimer[index]")
	int get_DeprecatedBuffTimer(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffTimer(); }
	__declspec(property(get = get_DeprecatedBuffTimer)) int PetBuffTimer[];
	#pragma endregion

	//----------------------------------------------------------------------------
	// data members

/*0x2d4*/ uint32_t           LastUpdateTime;
/*0x2d8*/ CCheckBoxWnd*      pButton[MAX_PET_BUTTONS];          // Pet%d_Button
/*0x348*/ CXWnd*             pWndBuffs;                         // BuffWindow
/*0x350*/ CTextureAnimation* pGoodBuffBackground;               // PetBlueIconBackground
/*0x358*/ CTextureAnimation* pBadBuffBackground;                // PetRedIconBackground
/*0x360*/ CTextureAnimation* pBypassBuffBackground;             // YellowIconBackground 360
/*0x368*/ SoeUtil::Array<PlayerBuffInfo> Buffs;
/*0x380*/ SoeUtil::Map<int, CButtonWnd*> ButtonMap;
/*0x398*/ uint32_t           LastBuffDecTime;                   // last time buff counters were decremented
/*0x39c*/ bool               bHadPetLastTime;
/*0x39d*/ bool               bShowOnSummon;
/*0x3a0*/ uint32_t           ListContextMenuID;
/*0x3a8*/ CXWnd*             pRequestingWnd;
/*0x3b0*/ bool               bRemovedBuffBlockMenuItem;
/*0x3b4*/ int                PetCommandSelectContextMenu;
/*0x3b8*/ CXWnd*             LastClickedButton;
/*0x3c0*/ bool               bPetButtonsDirty;
/*0x3c1*/ bool               Sit;
/*0x3c2*/ bool               Stop;
/*0x3c3*/ bool               ReGroup;
/*0x3c4*/ bool               Follow;
/*0x3c5*/ bool               Guard;
/*0x3c6*/ bool               Taunt;
/*0x3c7*/ bool               Hold;
/*0x3c8*/ bool               GHold;
/*0x3c9*/ bool               Focus;
/*0x3cc*/
};

SIZE_CHECK(CPetInfoWnd, CPetInfoWnd_size);

//============================================================================
// CPlayerNotesWnd
//============================================================================

class CPlayerNotesWnd : public CSidlScreenWnd
{
public:
	CPlayerNotesWnd(CXWnd*);
	virtual ~CPlayerNotesWnd();

	EQLIB_OBJECT void AppendText(char*);
	EQLIB_OBJECT void SaveNotes();
};

//============================================================================
// CPlayerWnd
//============================================================================

enum ECombatState
{
	eCombatState_Combat,
	eCombatState_Debuff,
	eCombatState_Timer,
	eCombatState_Standing,
	eCombatState_Regen,
};


// @sizeof(CPlayerWnd) == 0x408 :: 2024-07-22 (live) @ 0x14018cccd
constexpr size_t CPlayerWnd_size = 0x408;

class [[offsetcomments]] CPlayerWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CPlayerWnd(CXWnd*);
	virtual ~CPlayerWnd();

	EQLIB_OBJECT void CreateLocalMenu();
	EQLIB_OBJECT void UpdateContextMenu();

	//----------------------------------------------------------------------------
	// data members

/*0x2cc*/ 
/*0x2d8*/ CGaugeWnd*         pPlayerHP;                         // PlayerHP
/*0x2e0*/ CGaugeWnd*         pPetHP;                            // PetHP
/*0x2e8*/ CGaugeWnd*         pPlayerMana;                       // PlayerMana
/*0x2f0*/ CGaugeWnd*         pPlayerEndurance;                  // PlayerFatigue
/*0x2f8*/ CLabel*            pPlayerHPLbl;                      // HPLabel
/*0x300*/ CLabel*            pPlayerManaLbl;                    // ManaLabel
/*0x308*/ CLabel*            pPlayerEnduranceLbl;               // FatigueLabel
/*0x310*/ CLabel*            pPlayerHPPercentLbl;               // HPPerLabel
/*0x318*/ CLabel*            pPlayerManaPercentLbl;             // ManaPercLabel
/*0x320*/ CLabel*            pPlayerEndurancePercentLbl;        // FatiguePercLabel
/*0x328*/ COLORREF           HPTextColor;
/*0x330*/ CButtonWnd*        pGroupRoleTankBtn;                 // GroupRoleTank
/*0x338*/ CButtonWnd*        pGroupRoleAssistBtn;               // GroupRoleAssist
/*0x340*/ CButtonWnd*        pGroupRolePullerBtn;               // GroupRolePuller
/*0x348*/ CButtonWnd*        pGroupRoleMarkNPCBtn;              // GroupRoleMarkNPC
/*0x350*/ CGaugeWnd*         pCombatTimerGauge;                 // Player_CombatTimer
/*0x358*/ CLabel*            pCombatTimerLbl;                   // Player_CombatTimerLabel
/*0x360*/ CButtonWnd*        pNewMailIconBtn;                   // PW_NewMailIcon
/*0x368*/ CButtonWnd*        pParcelsIconBtn;                   // PW_ParcelsIcon
/*0x370*/ CButtonWnd*        pParcelsOverLimitIconBtn;          // PW_ParcelsOverLimitIcon
/*0x378*/ CLabel*            pAggroPctPlayerLbl;                // PW_AggroPctPlayerLabel
/*0x380*/ CLabel*            pAggroPctSecondaryLbl;             // PW_AggroPctSecondaryLabel
/*0x388*/ CLabel*            pAggroNameSecondaryLbl;            // PW_AggroNameSecondaryLabel
/*0x390*/ bool               bDrawAutoAttackRect;
/*0x394*/ int                LastUpdateTimer;
/*0x398*/ int                LastFrameTimer;
/*0x3a0*/ CStaticAnimationTemplate* pAttackIndicatorAnimTop;    // A_AttackIndicatorAnimTop
/*0x3a8*/ CStaticAnimationTemplate* pAttackIndicatorAnimBottom; // A_AttackIndicatorAnimBottom
/*0x3b0*/ CStaticAnimationTemplate* pAttackIndicatorAnimLeft;   // A_AttackIndicatorAnimLeft
/*0x3b8*/ CStaticAnimationTemplate* pAttackIndicatorAnimRight;  // A_AttackIndicatorAnimRight
/*0x3c0*/ CStaticAnimationTemplate* pAttackIndicatorAnimFill;   // A_AttackIndicatorAnimFill
/*0x3c8*/ CButtonWnd*        pCombatStateAnimButton;            // PW_CombatStateAnim
/*0x3d0*/ int                AttackIndicatorPulseValue;
/*0x3d8*/ CContextMenu*      pContextMenu;
/*0x3e0*/ uint32_t           Unknowns[9];
/*0x404*/ ECombatState       CombatState;                       // right above "A_PWCSInCombat"
/*0x408*/
};

SIZE_CHECK(CPlayerWnd, CPlayerWnd_size);

inline namespace deprecated {
	using CPLAYERWND DEPRECATE("Use CPlayerWnd instead of CPLAYERWND") = CPlayerWnd;
	using PCPLAYERWND DEPRECATE("Use CPlayerWnd* instead of PCPLAYERWND") = CPlayerWnd*;
}

//============================================================================
// CPotionBeltWnd
//============================================================================

class CPotionBeltWnd : public CSidlScreenWnd
{
public:
	CPotionBeltWnd(CXWnd*);
	virtual ~CPotionBeltWnd();
};


//============================================================================
// CPointMerchantWnd
//============================================================================

struct [[offsetcomments]] PointMerchantItem
{
/*0x00*/ char          ItemName[0x40];
/*0x40*/ int           ItemID;
/*0x44*/ unsigned int  Price;
/*0x48*/ int           ThemeID;
/*0x4c*/ int           IsStackable;
/*0x50*/ int           IsLore;
/*0x54*/ int           RaceMask;
/*0x58*/ int           ClassMask;
/*0x5c*/ bool          bCanUse;
/*0x60*/
};

class PointMerchantInterface;

class [[offsetcomments]] PointMerchantWnd : public CSidlScreenWnd, public PopDialogHandler, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	PointMerchantWnd(CXWnd*);
	virtual ~PointMerchantWnd();

	//----------------------------------------------------------------------------
	// data members

	// todo: verify
/*0x2dc*/ int                 Unknown0x008;
/*0x2e0*/ int                 NumItems;
/*0x2e4*/ bool                HdrItemName;
/*0x2e5*/ bool                HdrTheme;
/*0x2e6*/ bool                HdrPrice;
/*0x2e7*/ char                OriginalPointsLabel[0x40];
/*0x328*/ CLabel*             MerchantNameLabel;
/*0x330*/ CListWnd*           ItemList;
/*0x338*/ CListWnd*           PointList;
/*0x340*/ CButtonWnd*         EquipButton;
/*0x348*/ CButtonWnd*         PurchaseButton;
/*0x350*/ CButtonWnd*         SellButton;
/*0x358*/ CButtonWnd*         DoneButton;
/*0x360*/ CLabel*             PointsAvailableValue;
/*0x368*/ CLabel*             PointsEverEarnedLabel;
/*0x370*/ CLabel*             PointsAvailableLabel;
/*0x378*/ unsigned int        NextRefreshTime;
/*0x380*/ PlayerClient*       ActiveMerchant;
/*0x388*/ PointMerchantItem** Items;
/*0x390*/ int                 MerchantThemeId;
/*0x394*/ int                 CurrentSelection;
/*0x398*/ int                 CurrentSort;
/*0x39c*/ bool                bCurrentAscending;
/*0x3a0*/ ItemGlobalIndex     ItemLocation;
/*0x3b0*/ ItemPtr             pSelectedItem;
/*0x3c0*/ bool                bInventoryWasActive;
/*0x3c4*/ int                 CurrentItem;
/*0x3c8*/ int                 CurrentQuantity;
/*0x3cc*/ int                 SliderType;
/*0x3d0*/ PointMerchantInterface* pHandler;
/*0x3d8*/
};

//============================================================================
// CPurchaseGroupWnd
//============================================================================

class [[offsetcomments]] CPurchaseGroupWnd : public CSidlScreenWnd
{
public:
};

//============================================================================
// CQuantityWnd
//============================================================================

class CQuantityWnd : public CSidlScreenWnd
{
public:
	CQuantityWnd(CXWnd*);
	virtual ~CQuantityWnd();

	EQLIB_OBJECT void Open(CXWnd*, int, int, int, int, int, int, bool);
	EQLIB_OBJECT void CheckMaxEditWnd();
	EQLIB_OBJECT void UpdateEditWndFromSlider();
	EQLIB_OBJECT void UpdateSliderFromEditWnd();
};

//============================================================================
// CRaidOptionsWnd
//============================================================================

class CRaidOptionsWnd : public CSidlScreenWnd
{
public:
	CRaidOptionsWnd(CXWnd*);
	virtual ~CRaidOptionsWnd();

	EQLIB_OBJECT void AddLooterToList(char*);
	EQLIB_OBJECT void ClearLooterList();
	EQLIB_OBJECT void UpdateComponents();
	EQLIB_OBJECT void InitializeClassColors();
};

//============================================================================
// CRaidWnd
//============================================================================

// size: 0x500
class [[offsetcomments]] CRaidWnd : public CSidlScreenWnd, public WndEventHandler, public CVivoxObserver
{
	FORCE_SYMBOLS

public:
	CRaidWnd(CXWnd*);
	virtual ~CRaidWnd();

	EQLIB_OBJECT void AddPlayertoList(char*, char*, char*, char*, int, int, bool);
	EQLIB_OBJECT void ChangePosition(char*, int, int, int, bool);
	EQLIB_OBJECT void ChangeRaidGroupLeader(char*, int, char*);
	EQLIB_OBJECT void ClearPlayerList();
	EQLIB_OBJECT void RemovePlayerFromList(char*, int);
	EQLIB_OBJECT void SetClassColor(int, unsigned long);
	EQLIB_OBJECT void SetPlayerClassColor(char*, int, int);
	EQLIB_OBJECT void SetPlayerClassColor(int, int, int);
	EQLIB_OBJECT void SetRaidCount(int);
	EQLIB_OBJECT void SetRaidMemberRank(char*, char*, int);
	EQLIB_OBJECT void SetRaidTarget(char*, int, char*, int);
	EQLIB_OBJECT void UpdateButtons();
	EQLIB_OBJECT void UpdateLevelAverage(int);
	EQLIB_OBJECT void UpdateMemberName(char*, char*, int);
	EQLIB_OBJECT int FindIndexNotInGroupList(char*);
	EQLIB_OBJECT int FindOpenIndexInGroup(int);
	EQLIB_OBJECT int FindPlayerIndexInGroup(char*, int);
	EQLIB_OBJECT void AddSeparator();
	EQLIB_OBJECT void InitializeClassColors();
	EQLIB_OBJECT void ResortRaidGroupList(int, int);

	//----------------------------------------------------------------------------
	// data members

/*0x2e4*/ uint8_t            Unknown0x244[0x154];
/*0x438*/ COLORREF           ClassColors[0x10];
/*0x478*/ uint8_t            Unknown0x330[0x80];
/*0x4f8*/
};

inline namespace deprecated {
	using EQRAIDWINDOW DEPRECATE("Use CRaidWnd instead of EQRAIDWINDOW") = CRaidWnd;
	using PEQRAIDWINDOW DEPRECATE("Use CRaidWnd* instead of PEQRAIDWINDOW") = CRaidWnd*;
}

//============================================================================
// CRealEstateItemsWnd
//============================================================================

class CRealEstateItemsWnd : public CSidlScreenWnd, public PopDialogHandler, public WndEventHandler
{
public:
	CRealEstateItemsWnd(CXWnd*);
	virtual ~CRealEstateItemsWnd();
};

//============================================================================
// CRespawnWnd
//============================================================================

class CRespawnWnd : public CSidlScreenWnd, public WndEventHandler
{
public:
	CRespawnWnd(CXWnd* pParent);
	virtual ~CRespawnWnd();
};

//============================================================================
// CRewardSelectionWnd
//============================================================================

class CRewardSelectionWnd : public CSidlScreenWnd
{
public:
	EQLIB_OBJECT CRewardSelectionWnd(CXWnd*);

	// virtual
	EQLIB_OBJECT ~CRewardSelectionWnd();
};

//============================================================================
// CSelectorWnd
//============================================================================

class CSelectorWnd : public CSidlScreenWnd
{
public:
	CSelectorWnd(CXWnd*);
	virtual ~CSelectorWnd();

	EQLIB_OBJECT void KeyMapUpdated();
};

//============================================================================
// CSkillsSelectWnd
//============================================================================

class CSkillsSelectWnd : public CSidlScreenWnd
{
public:
	CSkillsSelectWnd(CXWnd*);
	virtual ~CSkillsSelectWnd();

	EQLIB_OBJECT void Refresh();
	EQLIB_OBJECT void SetTypesToDisplay(int);
	EQLIB_OBJECT void UpdateAll();
	EQLIB_OBJECT void UpdateSkill(int);
};

//============================================================================
// CSkillsWnd
//============================================================================

class CSkillsWnd : public CSidlScreenWnd
{
public:
	CSkillsWnd(CXWnd*);
	virtual ~CSkillsWnd();

	EQLIB_OBJECT void SkillImproveOccurred(int);
	EQLIB_OBJECT void UpdateAll();
	EQLIB_OBJECT void UpdateSkill(int);
};

//============================================================================
// CSocialEditWnd
//============================================================================

class CSocialEditWnd : public CSidlScreenWnd
{
public:
	CSocialEditWnd(CXWnd*);
	virtual ~CSocialEditWnd();

	EQLIB_OBJECT unsigned long GetSocialTextColor(int);
	EQLIB_OBJECT void ClickedAccept();
	EQLIB_OBJECT void ClickedClear();
	EQLIB_OBJECT void ClickedTextColorButton(int);
	EQLIB_OBJECT void UpdateControlsFromSocial();
};

//============================================================================
// CSpellBookWnd
//============================================================================

class [[offsetcomments]] CSpellBookWnd : public CSidlScreenWnd,
	public PopDialogHandler, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CSpellBookWnd(CXWnd*);
	virtual ~CSpellBookWnd();

	EQLIB_OBJECT bool StartSpellMemorization(int, int, bool);
	EQLIB_OBJECT int GetSpellMemTicksLeft();
	EQLIB_OBJECT int GetSpellScribeTicksLeft();
	EQLIB_OBJECT void AutoMemSpell(int, int);
	EQLIB_OBJECT void DeleteSpellFromBook(int, int);
	EQLIB_OBJECT void FinishMemorizing(int, int);
	EQLIB_OBJECT void FinishScribing(int, int);
	EQLIB_OBJECT void MemorizeSet(int*, int);
	EQLIB_OBJECT void RequestSpellDeletion(int);
	EQLIB_OBJECT void SwapSpellBookSlots(int, int);
	EQLIB_OBJECT void TurnToPage(int);
	EQLIB_OBJECT bool CanStartMemming(int);
	EQLIB_OBJECT void ContinueSetMem();
	EQLIB_OBJECT void DelayedSpellMem(int, int, int);
	EQLIB_OBJECT void DisplaySpellInfo(int);
	EQLIB_OBJECT void EndSetMem();
	EQLIB_OBJECT void GetSpellDeletionConfirmation(int);
	EQLIB_OBJECT void HandleLeftClickOnSpell(int);
	EQLIB_OBJECT void HandleRightClickOnSpell(int);
	EQLIB_OBJECT void HandleSpellInfoDisplay(CXWnd*);
	EQLIB_OBJECT void StartSpellMemorizationDrag(int, CButtonWnd*);
	EQLIB_OBJECT void StartSpellScribe(int);
	EQLIB_OBJECT void StopSpellBookAction();
	EQLIB_OBJECT void UpdateSpellBookDisplay();

	EQLIB_OBJECT static int GetBookSlot(int);

/*0x2e0*/ CPopDialogWnd* m_currentPopup;
/*0x2e8*/ int            LeftPageNbr;
/*0x2ec*/ int            SelectedBookIndex;
/*0x2f0*/ int            MemSlotIndex;
/*0x2f4*/ int            SpellId;
/*0x2f8*/ int            MemTicksLeft;
/*0x2fc*/ int            ScribeBookSlot;
/*0x300*/ int            ScribeTicksLeft;
/*0x304*/
};

//============================================================================
// CSpellDisplayWnd
//============================================================================

enum ESpellDisplayType
{
	SpellDisplayType_None,
	SpellDisplayType_BuffWnd,
	SpellDisplayType_PetBuffWnd,
	SpellDisplayType_BlockedBuffWnd,
	SpellDisplayType_BlockedPetBuffWnd,
	SpellDisplayType_SpellBookWnd,
	SpellDisplayType_HotButtonWnd,
	SpellDisplayType_CastSpellWnd,
	SpellDisplayType_Unknown8,
	SpellDisplayType_Unknown9,
	SpellDisplayType_TargetBuff,
};

// @sizeof(CSpellDisplayWnd) == 0x3b8 :: 2024-07-22 (live) @ 0x1404e0fce
constexpr size_t CSpellDisplayWnd_size = 0x3b8;

class [[offsetcomments]] CSpellDisplayWnd : public CSidlScreenWnd
{
	FORCE_SYMBOLS

public:
	CSpellDisplayWnd(CXWnd* parent, ESpellDisplayType displayType);
	virtual ~CSpellDisplayWnd();

	EQLIB_OBJECT void SetSpell(int SpellID, int);
	EQLIB_OBJECT void UpdateStrings();

/*0x2d0*/ int                WindowID;
/*0x2d8*/ CLabelWnd*         pDuration;                 // SDW_SpellDurationLabel
/*0x2e0*/ CStmlWnd*          pDescription;              // SDW_SpellDescription
/*0x2e8*/ CStmlWnd*          pName;                     //
/*0x2f0*/ CButtonWnd*        pIcon;                     // SDW_IconButton
/*0x2f8*/ SoeUtil::String    DescriptionText;
/*0x310*/ SoeUtil::String    TitleText;
/*0x328*/ SoeUtil::StringFixed<EQ_MAX_NAME> CasterName;
/*0x388*/ bool               bActive;
/*0x390*/ CTextureAnimation* ptaBuffIcons;              // A_SpellIcons
/*0x398*/ CTextureAnimation* ptaDragIcons;
/*0x3a0*/ bool               bFailed;
/*0x3a4*/ ESpellDisplayType  SpellDisplayType;
/*0x3a8*/ int                SpellID;
/*0x3ac*/ int                Unknown;
/*0x3b0*/ int                LastUpdateTime;
/*0x3b4*/
};

SIZE_CHECK(CSpellDisplayWnd, CSpellDisplayWnd_size);

//============================================================================
// CSpellGemWnd
//============================================================================

// CSpellGemWnd_size: 0x3f8
class [[offsetcomments]] CSpellGemWnd : public CButtonWnd
{
	FORCE_SYMBOLS

public:
	CSpellGemWnd(CXWnd*, uint32_t, const CXRect&, CTextureAnimation*, CTextureAnimation*, CTextureAnimation*, int, int, unsigned long);
	virtual ~CSpellGemWnd();

	EQLIB_OBJECT void SetGemTintStage(int);
	EQLIB_OBJECT void SetSpellGemTint(unsigned long);
	EQLIB_OBJECT void SetSpellIconIndex(int);

	//----------------------------------------------------------------------------
	// data members

/*0x348*/ int                MouseButtonState;
/*0x34c*/ bool               bChecked;
/*0x350*/ unsigned int       LastRefresh;
/*0x354*/ D3DCOLOR           SpellGemTintArray[11];
/*0x380*/ int                SpellGemAlphaArray[11];
/*0x3ac*/ int                SpellIconOffsetX;
/*0x3b0*/ int                SpellIconOffsetY;
/*0x3b4*/ int                SpellIconWidth;
/*0x3b8*/ int                SpellIconHeight;
/*0x3bc*/ int                SpellIconIndex;           // if this is equal to -1 there is no spell memmed in this slot
/*0x3c0*/ int                spellstate;               // 1 = cast in progress or refreshtime not met 2 means we ducked or aborted cast, 0 means its ok to cast, should really rename this to tintstage
/*0x3c4*/ int                TintIndex;
/*0x3c8*/ CTextureAnimation* SpellIconTexture;         // CTextureAnimation
/*0x3d0*/ CTextureAnimation* CustomIconTexture;        // CTextureAnimation
/*0x3d8*/ int                SpellSlot;
/*0x3dc*/ int                Percent;
/*0x3e0*/ CSpellGemDrawTemplate DrawTemplate;
/*0x400*/

	ALT_MEMBER_GETTER(int, SpellIconIndex, spellicon);
};

inline namespace deprecated {
	using EQCASTSPELLGEM DEPRECATE("Use CSpellGemWnd instead of EQCASTSPELLGEM") = CSpellGemWnd;
	using PEQCASTSPELLGEM DEPRECATE("Use CSpellGemWnd* instead of PEQCASTSPELLGEM") = CSpellGemWnd*;
}

//============================================================================
// CStoryWnd
//============================================================================

class CStoryWnd : public CGFScreenWnd
{
public:
	CStoryWnd(CXWnd*);
	virtual ~CStoryWnd();

	EQLIB_OBJECT bool HasNew();
	EQLIB_OBJECT bool ShowAuto();
	EQLIB_OBJECT void SaveIni();
	EQLIB_OBJECT void SelectIndex(int);
	EQLIB_OBJECT void SelectOldestNew();
};

//============================================================================
// CTargetWnd
//============================================================================

// @sizeof(CTargetWnd) == 0x3c8 :: 2024-07-22 (live) @ 0x14018cb6e
constexpr size_t CTargetWnd_size = 0x3c8;

class [[offsetcomments]] CTargetWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CTargetWnd(CXWnd*);
	virtual ~CTargetWnd();

	EQLIB_OBJECT void RefreshTargetBuffs(CUnSerializeBuffer& buffer);
	EQLIB_OBJECT void HandleBuffRemoveRequest(CXWnd* pWnd);

	PlayerBuffInfoWrapper GetBuffInfo(int buffIndex) const
	{
		if (buffIndex >= 0 && buffIndex < Buffs.GetSize())
			return PlayerBuffInfoWrapper(buffIndex, &Buffs[buffIndex]);

		return PlayerBuffInfoWrapper(buffIndex, nullptr);
	}

	PlayerBuffInfoWrapper GetBuffInfoBySpellID(int spellID) const
	{
		int buffIndex = 0;
		for (const PlayerBuffInfo& pbi : Buffs)
		{
			if (pbi.SpellID == spellID)
				return PlayerBuffInfoWrapper(buffIndex, &pbi);

			++buffIndex;
		}

		return PlayerBuffInfoWrapper(-1, nullptr);
	}

	int GetTotalBuffCount() const
	{
		int count = 0;
		for (const PlayerBuffInfo& pbi : Buffs)
		{
			if (pbi.SpellID > 0)
				++count;
		}

		return count;
	}

	int GetMaxBuffs() const { return Buffs.GetSize(); }

	int GetBuff(int buffIndex) const { return GetBuffInfo(buffIndex).GetSpellID(); }
	int GetBuffTimer(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffTimer(); }
	const char* GetCasterName(int buffIndex) const { return GetBuffInfo(buffIndex).GetCaster(); }
	CButtonWnd* GetBuffButton(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffButton(); }
	CTextureAnimation* GetBuffIcon(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffIcon(); }

	using BuffIterator = PlayerBuffInfoWrapper::Iterator<PlayerBuffInfoWrapper>;
	eqlib::IteratorRange<BuffIterator> GetBuffRange() const { return eqlib::make_iterator_range(BuffIterator(Buffs, 0), BuffIterator(Buffs, Buffs.GetSize())); }

#pragma region Deprecated accessors
	//
	// Deprecated accessors
	//

	// BuffSpellID -> GetBuff()
	DEPRECATE("Use GetBuff(index) or GetBuffInfo(index) insetad of BuffSpellID[index]")
		int get_DeprecatedBuff(int buffIndex) const { return GetBuffInfo(buffIndex).GetSpellID(); }
	__declspec(property(get = get_DeprecatedBuff)) int BuffSpellID[];

	// BuffTimer -> GetBuffTimer()
	DEPRECATE("Use GetBuffTimer(index) or GetBuffInfo(index) instead of BuffTimer[index]")
		int get_DeprecatedBuffTimer(int buffIndex) const { return GetBuffInfo(buffIndex).GetBuffTimer(); }
	__declspec(property(get = get_DeprecatedBuffTimer)) int BuffTimer[];
#pragma endregion

	//----------------------------------------------------------------------------
	// data members

/*0x2d8*/ CXWnd*                    pWndBuffs;                  // BuffWindow
/*0x2e0*/ CTextureAnimation*        pBuffGoodBackground;        // BlueIconBackground
/*0x2e8*/ CTextureAnimation*        pBuffBadBackground;         // RedIconBackground
/*0x2f0*/ CTextureAnimation*        pBuffBypassBackground;      // ellowIconBackground
/*0x2f8*/ SoeUtil::Array<PlayerBuffInfo> Buffs;
/*0x310*/ SoeUtil::Map<int, CButtonWnd*> ButtonMap;
/*0x328*/ int                       LastBuffReceivedTime;
/*0x32c*/ int                       Timer;
/*0x330*/ int                       ContextMenuID;
/*0x338*/ CXWnd*                    RequestingWnd;
/*0x340*/ bool                      bRemovedBuffBlockMenuItem;
/*0x344*/ int                       Type;                       // 1 = self, 4 = group member, 5 = PC, 7 = NPC
/*0x348*/ bool                      bFirstUpdate;
/*0x350*/ CLabelWnd*                lblAggroPctPlayer;          // Target_AggroPctPlayerLabel
/*0x358*/ CLabelWnd*                lblAggroPctSecondary;       // Target_AggroPctSecondaryLabel
/*0x360*/ CLabelWnd*                lblAggroNameSecondary;      // Target_AggroNameSecondaryLabel
/*0x368*/ CContextMenu*             pContextMenu;
/*0x370*/ int                       togglePctPlayerMenu;
/*0x374*/ int                       togglePctSecondaryMenu;
/*0x378*/ int                       toggleNameSecondaryMenu;
/*0x37c*/ bool                      bShowPctPlayer;
/*0x37d*/ bool                      bShowPctSecondary;
/*0x37e*/ bool                      bShowNameSecondary;
/*0x380*/ CLabel*                   lblHPLabel;                 // HPLabel
/*0x388*/ CLabel*                   lblHPPercLabel;             // HPPercLabel
/*0x390*/ CStaticAnimationTemplate* pTargetBoxAnimTop;          // A_TargetBoxStaticAnimTop
/*0x398*/ CStaticAnimationTemplate* pTargetBoxAnimBottom;       // A_TargetBoxStaticAnimBottom
/*0x3a0*/ CStaticAnimationTemplate* pTargetBoxAnimLeft;         // A_TargetBoxStaticAnimLeft
/*0x3a8*/ CStaticAnimationTemplate* pTargetBoxAnimRight;        // A_TargetBoxStaticAnimRight
/*0x3b0*/ CStaticAnimationTemplate* pTargetBoxAnimFill;         // A_TargetBoxStaticAnimFill
/*0x3b8*/ PlayerClient*             previousTarget;
/*0x3c0*/ uint32_t                  conColorRgba;
/*0x3c4*/
};

SIZE_CHECK(CTargetWnd, CTargetWnd_size);

//============================================================================
// CTaskWnd
//============================================================================

class CTaskWnd : public CSidlScreenWnd, public PopDialogHandler, public WndEventHandler
{
public:
	CTaskWnd(CXWnd*);
	virtual ~CTaskWnd();

	EQLIB_OBJECT int UpdateTaskTimers(unsigned long fasttime);

	void ConfirmAbandonTask(int taskId)
	{
		DialogResponse(100 /* abandon task */, 4 /* yes */, (void*)(intptr_t)taskId);
	}
};

//============================================================================
// CTextEntryWnd
//============================================================================

class [[offsetcomments]] CTextEntryWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CTextEntryWnd(CXWnd*);
	virtual ~CTextEntryWnd();

	EQLIB_OBJECT void Open(CXWnd* pwnd, int EditMode, char* Prompt, int MinLength, int MaxLength, const char* InitialText);
	EQLIB_OBJECT CXStr GetEntryText() const { return m_text; };
	EQLIB_OBJECT void SetEntryText(const CXStr& text) { m_text = text; }
	EQLIB_OBJECT void Activate(CXWnd*, int, char*, int, int, char*);
	EQLIB_OBJECT void Callback(bool);
	EQLIB_OBJECT void UpdateButtons();

/*0x2d4*/ int          m_minLength;
/*0x2d8*/ int          m_maxLength;
/*0x2e0*/ CXStr        m_text;
/*0x2e8*/ CXWnd*       m_parent;
/*0x2f0*/ CEditWnd*    m_entry;
/*0x2f8*/ CLabel*      m_prompt;
/*0x300*/ CButtonWnd*  m_ok;
/*0x308*/ CButtonWnd*  m_cancel;
/*0x310*/
};

inline namespace deprecated {
	using CTEXTENTRYWND DEPRECATE("Use CTextEntryWnd instead of CTEXTENTRYWND") = CTextEntryWnd;
	using PCTEXTENTRYWND DEPRECATE("Use CTextEntryWnd* instead of PCTEXTENTRYWND") = CTextEntryWnd*;
}

//============================================================================
// CTipWnd
//============================================================================

class CTipWnd : public CSidlScreenWnd
{
public:
	CTipWnd(CXWnd*, int);
	virtual ~CTipWnd();

	EQLIB_OBJECT void InitializeTipSettings();
	EQLIB_OBJECT void UpdateButtons();
	EQLIB_OBJECT bool IsRecentTOTD(int);
	EQLIB_OBJECT int SelectRandomTip();
	EQLIB_OBJECT void AddContextTip(int);
	EQLIB_OBJECT void LoadINISettings();
	EQLIB_OBJECT void SetTOTD(int);
	EQLIB_OBJECT void ShowHistoryTip(int);
	EQLIB_OBJECT void CleanDayTips();
	EQLIB_OBJECT void LoadDayTips();
};


//============================================================================
// CTrackingWnd
//============================================================================

class CTrackingWnd : public CSidlScreenWnd
{
public:
	CTrackingWnd(CXWnd*);
	virtual ~CTrackingWnd();

	EQLIB_OBJECT void DoTrackFilter(bool);
	EQLIB_OBJECT void DoTrackSort(bool);
	EQLIB_OBJECT void RemovePlayerFromTracking(PlayerClient*);
	EQLIB_OBJECT void SetTrackingList(const char*, int);
	EQLIB_OBJECT int GetListIdByData(unsigned int);
	EQLIB_OBJECT unsigned long GetTrackColor(int);
	EQLIB_OBJECT void GenerateTrackingList();
	EQLIB_OBJECT void NotifyServerOfTrackingTarget(int);
	EQLIB_OBJECT void RemoveFromListWndByData(unsigned int);
	EQLIB_OBJECT void RemovePlayerFromTracking(int);
	EQLIB_OBJECT void UpdateTrackingControls();
	EQLIB_OBJECT void UpdateTrackingList(bool);
};

//============================================================================
// CTradeSkillDepotWnd
//============================================================================

class [[offsetcomments]] CTradeskillDepotWnd : public CSidlScreenWnd, public WndEventHandler
{
public:
	FORCE_SYMBOLS;

/*0x2d8*/ CListWnd*     pItemList;          // TD_Item_List
/*0x2e0*/ CButtonWnd*   pInspectButton;     // TD_Inspect_Button
/*0x2e8*/ CLabel*       pSelectedItemLabel; // TD_Selected_Item_Label
/*0x2f0*/ CLabel*       pNoDataLabel;       // TD_NoDataLabel
/*0x2f8*/ CXWnd*        pScreenHideList;    // TD_ScreenHideList
/*0x300*/ CButtonWnd*   pSelectedItemSlot;  // TD_Selected_Item_Slot
/*0x308*/ CButtonWnd*   pWithdrawButton;    // TD_Withdraw_Button
/*0x310*/ CButtonWnd*   pExitButton;        // TD_Exit_Button
/*0x318*/ CLabel*       pItemCountNumber;   // TD_Item_Count_Number
/*0x320*/ CEditWnd*     pSearchItemInput;   // TD_SearchItem_Input
/*0x328*/ CButtonWnd*   pSearchItemButton;  // TD_SearchItem_Button
/*0x330*/ ItemContainer Items;
/*0x358*/ bool          bNeedsUpdate;
/*0x360*/ ItemPtr       pSelectedItem;
/*0x370*/ uint32_t      SelectedItemID;
/*0x378*/ uint64_t      lastUpdateTime;
/*0x380*/ bool          bUnknown5;
/*0x384*/
};

//============================================================================
// CTradeSkillWnd
//============================================================================

constexpr int MAX_RECIPE_ITEMS = 10;

struct [[offsetcomments]] TradeskillRecipe
{
	FORCE_SYMBOLS;

/*0x00*/ int ID;
/*0x04*/ int Trivial;
/*0x08*/ int ContainerType;
/*0x0c*/ int ContainerID;
/*0x10*/ int RequiredSize;
/*0x14*/ char Name[0x40];
/*0x54*/ bool bUnknown;
/*0x58*/ int Ingredient[MAX_RECIPE_ITEMS];                 // ID# of ingredient(s)
/*0x80*/ int IngredientIcon[MAX_RECIPE_ITEMS];             // Icon# of ingredient(s) (Note: 499 is blank)
/*0xa8*/
};

inline namespace deprecated {
	using EQTRADESKILLRECIPE DEPRECATE("Use TradeskillRecipe instead of EQTRADESKILLRECIPE") = TradeskillRecipe;
	using PEQTRADESKILLRECIPE DEPRECATE("Use TradeskillRecipe* instead of PEQTRADESKILLRECIPE") = TradeskillRecipe*;
}

// size: 0x10e0 2022-11-22 (test)
class [[offsetcomments]] CTradeSkillWnd : public CSidlScreenWnd
{
	FORCE_SYMBOLS;

public:
	CTradeSkillWnd(CXWnd* parent);
	virtual ~CTradeSkillWnd();

	//----------------------------------------------------------------------------
	// data members

	// todo: check and update
/*0x2d0*/ DWORD        Unknown0x148[0x1b];
/*0x340*/ TradeskillRecipe* SearchResults[0x64];
/*0x660*/ DWORD        Unknown0x344;
/*0x664*/ DWORD        Unknown0x348;
/*0x668*/ DWORD        Unknown0x34c;
/*0x670*/ ItemPtr      Container;
/*0x680*/ DWORD        Unknown0x354;
/*0x688*/ TradeskillRecipe* SelectedRecipe;
/*0x690*/ DWORD        Unknown0x35c;
/*0x694*/ DWORD        SkillLevel;
/*0x698*/ DWORD        Unknown0x364;
/*0x69c*/ DWORD        Unknown0x368;
/*0x6a0*/ BYTE         Unknown0x36c[0x58];
/*0x6f8*/ DWORD        Unknown0x3c4;
/*0x6fc*/ DWORD        Unknown0x3c8;
/*0x700*/ BYTE         Unknonw0x3cc[0x8];
/*0x708*/ DWORD        Unknown0x3d4;
/*0x70c*/ DWORD        Unknown0x3d8;
/*0x710*/
};

inline namespace deprecated {
	using EQTRADESKILLWINDOW DEPRECATE("Use CTradeSkillWnd instead of EQTRADESKILLWINDOW") = CTradeSkillWnd;
	using PEQTRADESKILLWINDOW DEPRECATE("Use CTradeSkillWnd* instead of PEQTRADESKILLWINDOW") = CTradeSkillWnd*;
}

//============================================================================
// CTradeWnd
//============================================================================

// CTradeWnd__CTradeWnd aTradewnd
// CTradeWnd__size: 0x2e8 (see 550E08) in Dec 19 2019 Live
class [[offsetcomments]] CTradeWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
	CTradeWnd(CXWnd*);
	virtual ~CTradeWnd();

	EQLIB_OBJECT bool IsMyTradeSlot(int, bool*);
	EQLIB_OBJECT void Activate(PlayerClient*, bool);
	EQLIB_OBJECT void AddContainerToHisTradeArray(EQ_Container*);
	EQLIB_OBJECT void AddEquipmentToHisTradeArray(EQ_Equipment*);
	EQLIB_OBJECT void AddNoteToHisTradeArray(EQ_Note*);
	EQLIB_OBJECT void ClickedCancelButton();
	EQLIB_OBJECT void ClickedMoneyButton(int);
	EQLIB_OBJECT void ClickedTradeButton();
	EQLIB_OBJECT void CompleteTrade();
	EQLIB_OBJECT void DeleteItemFromHisTradeArray(int);
	EQLIB_OBJECT void DropItemIntoTrade();
	EQLIB_OBJECT void SetHisMoney(int, long);
	EQLIB_OBJECT void SetHisReadyTrade(bool);
	EQLIB_OBJECT void SetMyReadyTrade(bool);
	EQLIB_OBJECT void TradeItemChanged(int, int);
	EQLIB_OBJECT void UpdateTradeDisplay();

/*0x2d4*/ unsigned int       NextRefreshTime;
/*0x2d8*/ bool               bInventoryWasOpen;
/*0x2e0*/ CButtonWnd*        HisMoneyButton[4];
/*0x300*/ CButtonWnd*        MyMoneyButton[4];
/*0x320*/ CButtonWnd*        TradeButton;
/*0x328*/ CButtonWnd*        CancelButton;
/*0x330*/ CLabel*            HisNameLabel;
/*0x338*/ CLabel*            MyNameLabel;
/*0x340*/ CInvSlotWnd*       pInvSlotWnd[MAX_TRADE_SLOTS];
/*0x3c0*/ long               HisMoney[4];
/*0x3d0*/ long               MyMoney[4];
/*0x3e0*/ ItemContainer      TradeItems;
/*0x408*/ bool               bHisReadyTrade;           // was HisTradeReady
/*0x409*/ bool               bMyReadyTrade;            // was MyTradeReady
/*0x40a*/ bool               bIsTrading;               // was TradeWndOpen
/*0x40c*/

	inline ItemContainer& GetTradeItems() { return TradeItems; }
	// TODO: Add aliases

	ALT_MEMBER_GETTER(bool, bHisReadyTrade, HisTradeReady);
	ALT_MEMBER_GETTER(bool, bMyReadyTrade, MyTradeReady);
};

inline namespace deprecated {
	using EQTRADEWINDOW DEPRECATE("Use CTradeWnd instead of EQTRADEWINDOW") = CTradeWnd;
	using PEQTRADEWINDOW DEPRECATE("Use CTradeWnd* instead of PEQTRADEWINDOW") = CTradeWnd*;
}

//============================================================================
// CTrainWnd
//============================================================================

class CTrainWnd : public CSidlScreenWnd
{
public:
	CTrainWnd(CXWnd*);
	virtual ~CTrainWnd();

	EQLIB_OBJECT void SetGMData(int*, unsigned char*, float);
	EQLIB_OBJECT void SkillChanged(int);
	EQLIB_OBJECT char* SkillName(int);
	EQLIB_OBJECT int SkillValue(int);
	EQLIB_OBJECT void SortNames(int);
	EQLIB_OBJECT void SortSkill(int);
	EQLIB_OBJECT void SwapSkill(int, int);
	EQLIB_OBJECT void Train();
	EQLIB_OBJECT void UpdateAll(bool);
	EQLIB_OBJECT void UpdateRight();
	EQLIB_OBJECT void UpdateSkill(int);
};

//============================================================================
// CVideoModesWnd
//============================================================================

class CVideoModesWnd : public CSidlScreenWnd
{
public:
	CVideoModesWnd(CXWnd*);
	virtual ~CVideoModesWnd();

	EQLIB_OBJECT void RestoreOldMode();
	EQLIB_OBJECT void Update();
	EQLIB_OBJECT void UpdateSelection(unsigned int);
};

//============================================================================
// CZoneGuideWnd
//============================================================================

enum eZoneGuideConnectionsView
{
	eZGCV_None,
	eZGCV_Selected,
	eZGCV_PreviewPath,
	eZGCV_ActivePath,
	eZGCV_Disabled,
};

class [[offsetcomments]] ZoneGuideConnection
{
	FORCE_SYMBOLS;

public:
/*0x00*/ EQZoneIndex        destZoneId;
/*0x04*/ int                transferTypeIndex;
/*0x08*/ int                requiredExpansions;       // EQExpansionOwned
/*0x0c*/ bool               disabled;
/*0x10*/
};

class [[offsetcomments]] ZoneGuideContinent
{
	FORCE_SYMBOLS;

public:
/*0x00*/ int                id;
/*0x04*/ int                displaySequence;
/*0x08*/ CXStr              name;
/*0x10*/
};

class [[offsetcomments]] ZoneGuideZoneType
{
	FORCE_SYMBOLS;

public:
/*0x00*/ int                id;
/*0x04*/ int                displaySequence;
/*0x08*/ CXStr              name;
/*0x10*/
};

class [[offsetcomments]] ZoneGuideTransferType
{
	FORCE_SYMBOLS;

public:
/*0x00*/ int                id;
/*0x08*/ CXStr              description;
/*0x10*/
};

class [[offsetcomments]] ZoneGuideZone
{
	FORCE_SYMBOLS;

public:
/*0x00*/ EQZoneIndex   zoneId;
/*0x08*/ CXStr         name;
/*0x10*/ int           continentIndex;
/*0x14*/ int           minLevel;
/*0x18*/ int           maxLevel;
/*0x20*/ DynamicBitField<unsigned short, short> types;
/*0x30*/ ArrayClass<ZoneGuideConnection> zoneConnections;
/*0x48*/
};

struct [[offsetcomments]] ZonePathData
{
	FORCE_SYMBOLS;

/*0x00*/ EQZoneIndex        zoneId;
/*0x04*/ int                transferTypeIndex;
/*0x08*/

	ZonePathData(EQZoneIndex zoneId, int transferTypeIndex)
		: zoneId(zoneId), transferTypeIndex(transferTypeIndex) {}
	ZonePathData()
		: zoneId(0), transferTypeIndex(-1) {}
};

class [[offsetcomments]] ZoneGuideManagerBase
{
	FORCE_SYMBOLS;

public:
	ZoneGuideManagerBase();
	virtual ~ZoneGuideManagerBase();

	virtual void ResetAllData();
	//virtual void Serialize(CSerializeBuffer&);
	//virtual void Unserialize(CUnserializeBuffer&);

/*0x0008*/ ZoneGuideZone                     zones[ZONE_COUNT];
/*0xf6b0*/ ArrayClass<ZoneGuideContinent>    continents;
/*0xf6c8*/ ArrayClass<ZoneGuideZoneType>     zoneTypes;
/*0xf6e0*/ ArrayClass<ZoneGuideTransferType> transferTypes;
/*0xf6f8*/

	ZoneGuideZone* GetZone(EQZoneIndex zi)
	{
		if (zi > 0 && zi < ZONE_COUNT)
		{
			if (zones[zi].zoneId != 0)
				return &zones[zi];
		}

		return nullptr;
	}

	CXStr GetContinentNameByIndex(int continentIndex)
	{
		return continentIndex >= 0 && continentIndex < (int)continents.size() ? continents[continentIndex].name : CXStr();
	}

	CXStr GetZoneTypeNameByIndex(int zoneTypeIndex)
	{
		return zoneTypeIndex >= 0 && zoneTypeIndex < (int)zoneTypes.size() ? zoneTypes[zoneTypeIndex].name : CXStr();
	}

	CXStr GetZoneTransferTypeNameByIndex(int transferTypeIndex) const
	{
		return transferTypeIndex >= 0 && transferTypeIndex < (int)transferTypes.size() ? transferTypes[transferTypeIndex].description.c_str() : CXStr();
	}
};

using ZonePathArray = ArrayClass<ZonePathData>;

// @sizeof(ZoneGuideManagerClient) == 0xf738 :: 2024-07-22 (live) @ 0x14033b30f
constexpr size_t ZoneGuideManagerClient_size = 0xf738;

class [[offsetcomments]] ZoneGuideManagerClient : public ZoneGuideManagerBase
{
	FORCE_SYMBOLS;

public:
/*0xf6f8*/ ZonePathArray     activePath;
/*0xf710*/ ZonePathArray     previewPath;
/*0xf728*/ EQZoneIndex       currentZone;
/*0xf72c*/ int               heroesJourneyIndex;
/*0xf730*/ bool              zoneGuideDataSet;
/*0xf731*/ bool              includeBindZoneInPath;
/*0xf732*/ bool              autoFindActivePath;
/*0xf733*/ bool              findActivePath;
/*0xf734*/

	EQLIB_OBJECT static ZoneGuideManagerClient& Instance();
};

SIZE_CHECK(ZoneGuideManagerClient, ZoneGuideManagerClient_size)

class [[offsetcomments]] CZoneGuideWnd : public CSidlScreenWnd, public WndEventHandler
{
	FORCE_SYMBOLS

public:
/*0x2d8*/ CVerticalLayoutWnd*     VerticalLayout;
/*0x2e0*/ CButtonWnd*             FilterMyLevelButton;
/*0x2e8*/ CButtonWnd*             FilterAllLevelsButton;
/*0x2f0*/ CButtonWnd*             FilterZonesActiveButton;
/*0x2f8*/ CButtonWnd*             FilterZonesInactiveButton;
/*0x300*/ CButtonWnd*             ZoneRunSearchButton;
/*0x308*/ CButtonWnd*             ZoneClearSearchButton;
/*0x310*/ CButtonWnd*             SelectCurrentZoneButton;
/*0x318*/ CEditWnd*               LevelFilterEdit;
/*0x320*/ CEditWnd*               ZoneSearchEdit;
/*0x328*/ CComboWnd*              TypeFilterCombo;
/*0x330*/ CComboWnd*              ContinentFilterCombo;
/*0x338*/ CListWnd*               ZonesList;
/*0x340*/ CLabelWnd*              ViewZoneConnectionsSelectedZoneLabel;
/*0x348*/ CLabelWnd*              ViewZoneConnectionsPreviewPathLabel;
/*0x350*/ CLabelWnd*              ViewZoneConnectionsActivePathLabel;
/*0x358*/ CLabelWnd*              ViewZoneConnectionsDisabledLabel;
/*0x360*/ CButtonWnd*             ViewZoneConnectionsSelectedZoneButton;
/*0x368*/ CButtonWnd*             ViewZoneConnectionsPreviewPathButton;
/*0x370*/ CButtonWnd*             ViewZoneConnectionsActivePathButton;
/*0x378*/ CButtonWnd*             ViewZoneConnectionsDisabledButton;
/*0x380*/ CButtonWnd*             DisableConnectionTemplateButton;
/*0x388*/ CListWnd*               ZoneConnectionsList;
/*0x390*/ CButtonWnd*             ResetPathStartZoneButton;
/*0x398*/ CButtonWnd*             SetPathStartZoneButton;
/*0x3a0*/ CButtonWnd*             SetPathEndZoneButton;
/*0x3a8*/ CButtonWnd*             ShowPathWndButton;
/*0x3b0*/ CButtonWnd*             HidePathWndButton;
/*0x3b8*/ CButtonWnd*             FindPathButton;
/*0x3c0*/ CButtonWnd*             EndFindButton;
/*0x3c8*/ CButtonWnd*             ClearPathWndButton;
/*0x3d0*/ CButtonWnd*             ActivatePathButton;
/*0x3d8*/ CButtonWnd*             IncludeBindZoneInPathGenerationButton;
/*0x3e0*/ CButtonWnd*             ShowPathWndOnPathActivationButton;
/*0x3e8*/ CButtonWnd*             AutoFindActivePathButton;
/*0x3f0*/ CEditWnd*               PathStartZoneEdit;
/*0x3f8*/ CEditWnd*               PathEndZoneEdit;
/*0x400*/ uint32_t                NextButtonRefreshTime;
/*0x404*/ EQZoneIndex             CurrentZone;
/*0x408*/ bool                    bFilterActive;
/*0x40c*/ int                     FilterLevel;
/*0x410*/ int                     FilterContinentIndex;
/*0x414*/ int                     FilterZoneTypeIndex;
/*0x418*/ bool                    bSelectCurrentZone;
/*0x420*/ CXStr                   ZoneSearchString;
/*0x428*/ eZoneGuideConnectionsView eCurrConnectionsView;
/*0x42c*/ EQZoneIndex             CurrConnectionsViewSelectedZone;
/*0x430*/ bool                    bCurrentConnectionsViewPreviewPathChanged;
/*0x431*/ bool                    bCurrentConnectionsViewActivePathChanged;
/*0x432*/ bool                    bSetPathStartZoneToCurrentZone;
/*0x434*/ EQZoneIndex             StartZone;
/*0x438*/ EQZoneIndex             EndZone;
/*0x43c*/ bool                    bZoneGuideDataChanged;
/*0x43d*/ bool                    bZoneListChanged;
/*0x43e*/ bool                    bZoneConnectionsListChanged;
/*0x43f*/ bool                    bPathStartZoneChanged;
/*0x440*/ int                     RightClickMenuID;
/*0x444*/
};

class [[offsetcomments]] CZonePathWnd : public CSidlScreenWnd, public WndEventHandler
{
public:
/*0x2d4*/ EQZoneIndex             currentZone;
/*0x2d8*/ bool                    zonePathDirty;
/*0x2e0*/ CListWnd*               listZones;
/*0x2e8*/
};

//============================================================================
// CSidlManagerBase
//============================================================================

enum EStaticScreenPieceClasses
{
	StaticScreenPieceUnknown = -1,

	StaticScreenPieceHeader,
	StaticScreenPieceFrame,
	StaticScreenPieceText,
	StaticScreenPieceAnimation,
	StaticScreenPieceTintedBlendAnimation,

	StaticScreenPieceMax,
};

class [[offsetcomments]] CSidlManagerBase
{
	FORCE_SYMBOLS

public:
	CSidlManagerBase();
	virtual ~CSidlManagerBase();

	//----------------------------------------------------------------------------
	// virtuals

	virtual CXWnd* CreateXWnd(CXWnd* parent, CControlTemplate* pTemplate);

	//----------------------------------------------------------------------------
	// defined methods

	EQLIB_OBJECT CButtonDrawTemplate* FindButtonDrawTemplate(uint32_t id) const;
	EQLIB_OBJECT CButtonDrawTemplate* FindButtonDrawTemplate(std::string_view name) const;

	EQLIB_OBJECT CXMLParamManager* GetParamManager();

	// this "bSomething" is a parameter passed through to CreateButtonWnd and CreateScreenWnd, which is then passed back through
	// CreateXWndFromTemplate recursively... not sure where it is used yet. A couple non-xml source set it to 1.
	EQLIB_OBJECT CXWnd* CreateXWndFromTemplate(CXWnd* pParent, CControlTemplate* pTemplate, bool bUnknown = false);

	// same as above but looks up a template by name.
	EQLIB_OBJECT CXWnd* CreateXWndFromTemplate(CXWnd* pParent, const CXStr& name);

	//----------------------------------------------------------------------------
	// methods

	EQLIB_OBJECT void DeleteContents();
	EQLIB_OBJECT void LoadSidl(const CXStr& Path, const CXStr& DefaultPath, const CXStr& Filename, const CXStr& DefaultClientPath = "UIFiles\\default\\");
	EQLIB_OBJECT CXStr& GetParsingErrorMsg();

	EQLIB_OBJECT EStaticScreenPieceClasses GetScreenPieceEnum(const CScreenPieceTemplate*) const;
	EQLIB_OBJECT EStaticScreenPieceClasses GetScreenPieceEnum(const CParamScreenPiece*) const;

	EQLIB_OBJECT CScrollbarTemplate* FindScrollbarTemplate(const CXStr& name) const;
	EQLIB_OBJECT CScrollbarTemplate* FindScrollbarTemplate(uint32_t) const;
	EQLIB_OBJECT CSliderDrawTemplate* FindSliderDrawTemplate(const CXStr& Name) const;
	EQLIB_OBJECT CSliderDrawTemplate* FindSliderDrawTemplate(uint32_t) const;
	EQLIB_OBJECT CUITextureInfo* FindTexture(const CXStr&) const;
	EQLIB_OBJECT CUITextureInfo* FindTexture(uint32_t) const;
	EQLIB_OBJECT CTextureAnimation* FindAnimation(const CXStr&) const;
	EQLIB_OBJECT CTextureAnimation* FindAnimation(uint32_t) const;
	EQLIB_OBJECT CTAFrameDraw* FindFrameDraw(const CXStr&) const;
	EQLIB_OBJECT CTAFrameDraw* FindFrameDraw(uint32_t) const;
	EQLIB_OBJECT CXWndDrawTemplate* FindDrawTemplate(const CXStr&) const;
	EQLIB_OBJECT CXWndDrawTemplate* FindDrawTemplate(uint32_t) const;
	EQLIB_OBJECT CScreenPieceTemplate* FindScreenPieceTemplate(const CXStr& Name) const;
	EQLIB_OBJECT CScreenPieceTemplate* FindScreenPieceTemplate(uint32_t) const;
	EQLIB_OBJECT CLayoutStrategyTemplate* FindLayoutStrategyTEmplate(uint32_t id);

	EQLIB_OBJECT CParamScreenPiece* GetScreenPiece(CParamScreen* screen, const CXStr& screenId) const;
	EQLIB_OBJECT CTextureAnimation CreateTextureAnimationFromSidlAnimation(const CParamUi2DAnimation*) const;
	EQLIB_OBJECT CTAFrameDraw CreateTAFrameDrawFromSidlFrame(const CParamFrameTemplate*) const;
	EQLIB_OBJECT CXWndDrawTemplate CreateDrawTemplateFromParamWindowDrawTemplate(const CParamWindowDrawTemplate*) const;

	EQLIB_OBJECT void AddScreenPieceTemplateInOrder(CScreenPieceTemplate*);
	EQLIB_OBJECT void AddAnimationInOrder(CTextureAnimation*);
	EQLIB_OBJECT void AddTAFrameDrawInOrder(CTAFrameDraw*);
	EQLIB_OBJECT void AddDrawTemplateInOrder(CXWndDrawTemplate*);
	EQLIB_OBJECT void AddLayoutStrategyInOrder(CLayoutStrategyTemplate*);

	EQLIB_OBJECT CButtonDrawTemplate GetButtonDrawTemplateFromParamButtonDrawTemplate(const CParamButtonDrawTemplate&) const;
	EQLIB_OBJECT CGaugeDrawTemplate GetGaugeDrawTemplateFromParamGaugeDrawTemplate(const CParamGaugeDrawTemplate&) const;
	EQLIB_OBJECT CSpellGemDrawTemplate GetSpellGemDrawTemplateFromParamSpellGemDrawTemplate(const CParamSpellGemDrawTemplate&) const;
	EQLIB_OBJECT CScrollbarTemplate GetScrollbarTemplateFromParamScrollbarTemplate(const CParamScrollbarDrawTemplate&) const;
	EQLIB_OBJECT CSliderDrawTemplate GetSliderDrawTemplateFromParamSliderDrawTemplate(const CParamSliderDrawTemplate&) const;

	EQLIB_OBJECT static CXPoint GetPointFromParamPoint(const CParamPoint&);
	EQLIB_OBJECT static CXRect GetRectFromParamPointSize(const CParamPoint&, const CParamSize&);
	EQLIB_OBJECT static CXSize GetSizeFromParamSize(const CParamSize&);
	EQLIB_OBJECT static CXStr TranslateString(const CXStr&);
	EQLIB_OBJECT static D3DCOLOR GetD3DCOLOR(const CParamRGB&);

	EQLIB_OBJECT CLayoutStrategyTemplate* CreateLayoutStrategyTemplate(CParamLayoutStrategy*);
	EQLIB_OBJECT CScreenPieceTemplate* CreateScreenPieceTemplateFromParamScreenPiece(const CParamScreenPiece*) const;
	EQLIB_OBJECT CLayoutStrategy* CreateLayoutStrategy(CLayoutStrategyTemplate*);
	EQLIB_OBJECT CXWnd* CreateButtonWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateScreenWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreatePageWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateListWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateSliderWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateLabelWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateSTMLWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateTreeViewWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateComboWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateTabWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateLayoutWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateHorizontalLayoutWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateVerticalLayoutWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateFinderWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateTileLayoutWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateTemplateContainerWnd(CXWnd* parent, CControlTemplate* pTemplate);

	//----------------------------------------------------------------------------
	// data members

/*0x008*/ EStaticScreenPieceClasses              ScreenPieceClassIndex[StaticScreenPieceMax];
/*0x020*/ ArrayClass<CUITextureInfo*>            Textures;
/*0x038*/ ArrayClass<CButtonDrawTemplate*>       ButtonDrawTemplateArray;
/*0x050*/ ArrayClass<CScrollbarTemplate*>        ScrollbarTemplateArray;
/*0x068*/ ArrayClass<CSliderDrawTemplate*>       SliderDrawTemplateArray;
/*0x080*/ ArrayClass<CXStr>                      ScreenNameArray;
/*0x098*/ ArrayClass<CXWndDrawTemplate*>         DrawTemplateArray;
/*0x0b0*/ CHashCXStrInt32                        DrawTemplateHash;
/*0x0d0*/ ArrayClass<CTextureAnimation*>         AnimationArray;
/*0x0e8*/ CHashCXStrInt32                        AnimationsHash;
/*0x108*/ ArrayClass<CTAFrameDraw*>              TAFrameArray;
/*0x120*/ CHashCXStrInt32                        TAFrameHash;
/*0x140*/ ArrayClass<CScreenPieceTemplate*>      ScreenPieceArray;
/*0x158*/ CHashCXStrInt32                        ScreenPiecesHash;
/*0x178*/ ArrayClass<CLayoutStrategyTemplate*>   LayoutStrategyTemplateArray;
/*0x190*/ CHashCXStrInt32                        LayoutStrategyTemplatesHash;
/*0x1b0*/ CXMLParamManager                       XMLDataMgr;
/*0x288*/ bool                                   bLoadError;
/*0x290*/ CXStr                                  ErrorString;
/*0x298*/
};

class [[offsetcomments]] CSidlManager : public CSidlManagerBase
{
	FORCE_SYMBOLS

public:
	virtual ~CSidlManager();

	//----------------------------------------------------------------------------
	// virtuals
	virtual CXWnd* CreateXWnd(CXWnd* parent, CControlTemplate* pTemplate) override;

	//----------------------------------------------------------------------------
	// defined methods

	// don't need to use this -- can just use CreateXWnd.
	EQLIB_OBJECT CXWnd* CreateHotButtonWnd(CXWnd* pwndParent, CControlTemplate* pControl);

	//----------------------------------------------------------------------------
	// methods

	EQLIB_OBJECT CXWnd* CreateLabel(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateGuageWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateSpellGemWnd(CXWnd* parent, CControlTemplate* pTemplate);
	EQLIB_OBJECT CXWnd* CreateInvSlotWnd(CXWnd* parent, CControlTemplate* pTemplate);
};

inline namespace deprecated {
	using CSIDLMGR DEPRECATE("Use CSidlManager instead of CSIDLMGR") = CSidlManager;
	using PCSIDLMGR DEPRECATE("Use CSidlManager* instead of PCSIDLMGR") = CSidlManager*;
}

//----------------------------------------------------------------------------

class CascadeItemCommand;

// this is a base class for the cascade menu items defined in CascadeMenu.txt
class [[offsetcomments]] CascadeItemBase
{
public:
	enum Type
	{
		eTypeSubMenu = 0,
		eTypeCommand = 1,
		eTypeSeparator = 2,
	};

	CascadeItemBase(Type type)
		: m_type(type)
	{}

	virtual ~CascadeItemBase() {}

	Type GetType() const { return m_type; }
	bool IsChanged() const { return m_changed; }

	virtual CascadeItemCommand* GetAsCommand() { return nullptr; }

protected:
/*0x08*/ Type m_type;
/*0x0c*/ bool m_changed = true;
/*0x10*/
};

using CascadeItemArray = ArrayClass<CascadeItemBase*>;

// separator menu item (type 3 in CascadeMenu.txt)
class [[offsetcomments]] CascadeItemSeparator : public CascadeItemBase
{
public:
	CascadeItemSeparator() : CascadeItemBase(eTypeSeparator) {}
};

// a menu item that spawns another menu item
class [[offsetcomments]] CascadeItemSubMenu : public CascadeItemBase
{
public:
	CascadeItemSubMenu() : CascadeItemBase(eTypeSubMenu) {}
	CascadeItemSubMenu(int iconId, const char* text)
		: CascadeItemBase(eTypeSubMenu)
		, m_icon(iconId)
		, m_text(text)
	{}

	virtual ~CascadeItemSubMenu()
	{
		int count = m_items ? m_items->GetCount() : 0;
		for (int i = 0; i < count; ++i)
		{
			delete m_items->GetElementIdx(i);
		}
		m_items->Reset();
	}

	CascadeItemArray* GetItems() { return m_items; }

	CascadeItemArray* SetItems(CascadeItemArray* items)
	{
		m_changed = true;
		return std::exchange(m_items, items);
	}

	CascadeItemBase* GetItem(int index)
	{
		if (m_items)
		{
			return m_items->GetElementIdx(index);
		}

		return nullptr;
	}

	int GetIcon() const { return m_icon; }
	void SetIcon(int icon) { m_icon = icon; m_changed = true; }

	const CXStr& GetText() const { return m_text; }
	void SetText(const CXStr& text) { m_text = text; m_changed = true; }

protected:
/*0x10*/ int m_icon = -1;
/*0x18*/ CXStr m_text;
/*0x20*/ CascadeItemArray* m_items = nullptr;
/*0x28*/
};

// base class for items that execute a command of some kind when clicked
class [[offsetcomments]] CascadeItemCommandBase : public CascadeItemBase
{
public:
	CascadeItemCommandBase() : CascadeItemBase(CascadeItemBase::eTypeCommand) {}

	int GetIcon() const { return m_icon; }
	void SetIcon(int icon) { m_icon = icon; m_changed = true; }

	const CXStr& GetText() const { return m_text; }
	void SetText(const CXStr& text) { m_text = text; m_changed = true; }

	virtual void ExecuteCommand() = 0;
	virtual CXStr GetTooltip() const = 0;

protected:
/*0x10*/ int m_icon = -1;
/*0x18*/ CXStr m_text;
/*0x20*/
};

// A menu item that executes a command
class [[offsetcomments]] CascadeItemCommand : public CascadeItemCommandBase
{
public:
	CascadeItemCommand(int icon, const char* text, int command);

	virtual void ExecuteCommand() override;
	virtual CXStr GetTooltip() const override { return m_text; }

private:
/*0x20*/ int m_command = -1;
/*0x24*/
};

//----------------------------------------------------------------------------

// This is the frame the holds the EQ and store buttons
class [[offsetcomments]] CEQMainWnd : public CSidlScreenWnd
{
public:
	CEQMainWnd(CXWnd* pParent);
	virtual ~CEQMainWnd();

	virtual void UpdateCascadeMenuItems() {}

/*0x2d0*/ CButtonWnd*       EQButtonWnd;
/*0x2d8*/ CButtonWnd*       SCButtonWnd;
/*0x2e0*/ CButtonWnd*       FlashSaleButtonWnd;
/*0x2e8*/ CascadeItemArray* CascadeMenuItems;
/*0x2f0*/ int               MenuId;
/*0x2f4*/
};

//----------------------------------------------------------------------------

// there are four types of icon caches
enum eIconCacheType
{
	IconCacheType_Item = 0,
	IconCacheType_Spell = 1,
	IconCacheType_Menu = 2,
	IconCacheType_SpeakingIndicator = 3,
};
EQLIB_OBJECT const char* IconCacheTypeToString(eIconCacheType type);

class [[offsetcomments]] IconCache
{
public:
/*0x00*/ HashTable<CTextureAnimation*> IconTextures;
/*0x18*/ CXStr              pAnimationName;
/*0x20*/ int                Offset;
/*0x24*/ int                MinValue;
/*0x28*/ int                MaxValue;
/*0x2c*/

	EQLIB_OBJECT CTextureAnimation* GetIcon(int);
};

//----------------------------------------------------------------------------

constexpr int MAX_ITEMDISPLAY_WINDOWS = 6;
constexpr int MAX_SPELLDISPLAY_WINDOWS = 4;

class [[offsetcomments]] CWndDisplayManager
{
public:
	EQLIB_OBJECT int FindWindow(bool bNewWnd);
	//EQLIB_OBJECT bool CloseNewest();
	//EQLIB_OBJECT void CloseAll();

	EQLIB_OBJECT virtual int CreateWindowInstance();

	ALT_MEMBER_GETTER_DEPRECATED(ArrayClass2<CXWnd*>, m_windows, pWindows, "CWndDisplayManager: pWindows is deprecated. Use GetWindow instead.");
	ALT_MEMBER_GETTER_DEPRECATED(ArrayClass2<int>, m_times, pTimes, "CWndDisplayManager: pTimes is deprecated. Use GetLastUpdateTime instead.");

	inline CXWnd* GetWindow(int index) const
	{
		if (index >= 0 && index < m_windows.GetLength())
			return m_windows[index];
		return nullptr;
	}

	inline int GetLastUpdateTime(int index) const
	{
		if (index >= 0 && index < m_times.GetLength())
			return m_times[index];
		return 0;
	}

	inline int GetCount() const { return m_windows.GetLength(); }
	inline int GetMaxWindows() const { return MaxWindows; }

protected:
/*0x08*/ ArrayClass2<CXWnd*>      m_windows;
/*0x28*/ ArrayClass2<int>         m_times;
/*0x48*/ int                      MaxWindows;
/*0x4c*/
};

class CItemDisplayManager : public CWndDisplayManager
{
public:
	EQLIB_OBJECT virtual int CreateWindowInstance() override;
	EQLIB_OBJECT void ShowItem(const ItemPtr& pItem);

	inline CItemDisplayWnd* GetWindow(int index) const { return static_cast<CItemDisplayWnd*>(CWndDisplayManager::GetWindow(index)); }
};

class CSpellDisplayManager : public CWndDisplayManager
{
public:
	inline CSpellDisplayWnd* GetWindow(int index) const { return static_cast<CSpellDisplayWnd*>(CWndDisplayManager::GetWindow(index)); }
};

//----------------------------------------------------------------------------

void InitializeUI();

EQLIB_OBJECT void InitializeInGameUI();

} // namespace eqlib
