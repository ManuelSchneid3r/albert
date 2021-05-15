// Copyright (C) 2014-2021 Manuel Schneider

#include <QAbstractEventDispatcher>
#include <QDebug>
#include <QKeySequence>
#include <map>
#include <set>
#include <QtX11Extras/QX11Info>
#include "X11/Xlib.h"
#include "X11/Xutil.h"
#include "X11/XKBlib.h"
#include "xcb/xcb.h"
#include "xcb/xproto.h"
#include "hotkeymanager_x11.h"
#include "X11/keysymdef.h"
#include "X11/XF86keysym.h"

namespace {

    // http://cep.xray.aps.anl.gov/software/qt4-x11-4.2.2-browser/dc/d02/qkeymapper__x11_8cpp-source.html
    static std::map<uint, std::set<uint>> QtToXSymsMap =
    {
        {Qt::Key_Escape,       { XK_Escape }}, // misc keys
        {Qt::Key_Tab,          { XK_Tab }},
        {Qt::Key_Backtab,      { XK_BackSpace }},
        {Qt::Key_Backspace,    { XK_BackSpace }},
        {Qt::Key_Return,       { XK_Return }},
        {Qt::Key_Enter,        { XK_KP_Enter }},
        {Qt::Key_Insert,       { XK_Insert, }},
        {Qt::Key_Delete,       { XK_Delete }},
        {Qt::Key_Pause,        { XK_Pause }},
        {Qt::Key_Print,        { XK_Print }},
        {Qt::Key_SysReq,       { XK_Sys_Req}},
        {Qt::Key_Clear,        { XK_Clear}},
        {Qt::Key_Home,         { XK_Home}},// cursor movement
        {Qt::Key_End,          { XK_End }},
        {Qt::Key_Left,         { XK_Left }},
        {Qt::Key_Up,           { XK_Up }},
        {Qt::Key_Right,        { XK_Right }},
        {Qt::Key_Down,         { XK_Down }},
        {Qt::Key_PageUp,       { XK_Prior }},
        {Qt::Key_PageDown,     { XK_Next }},
        {Qt::Key_Shift,        { XK_Shift_L, XK_Shift_R, XK_Shift_Lock  }},// modifiers
        {Qt::Key_Control,      { XK_Control_L, XK_Control_R }},
        {Qt::Key_Meta,         { XK_Meta_L, XK_Meta_R }},
        {Qt::Key_Alt,          { XK_Alt_L, XK_Alt_R }},
        {Qt::Key_AltGr,        { XK_ISO_Level3_Shift }},
        {Qt::Key_CapsLock,     { XK_Caps_Lock }},
        {Qt::Key_NumLock,      { XK_Num_Lock }},
        {Qt::Key_ScrollLock,   { XK_Scroll_Lock }},
        {Qt::Key_F1,           { XK_F1 }},          // function keys
        {Qt::Key_F2,           { XK_F2 }},
        {Qt::Key_F3,           { XK_F3 }},
        {Qt::Key_F4,           { XK_F4 }},
        {Qt::Key_F5,           { XK_F5 }},
        {Qt::Key_F6,           { XK_F6 }},
        {Qt::Key_F7,           { XK_F7 }},
        {Qt::Key_F8,           { XK_F8 }},
        {Qt::Key_F9,           { XK_F9 }},
        {Qt::Key_F10,          { XK_F10 }},
        {Qt::Key_F11,          { XK_F11 }},
        {Qt::Key_F12,          { XK_F12 }},
        {Qt::Key_F13,          { XK_F13 }},
        {Qt::Key_F14,          { XK_F14 }},
        {Qt::Key_F15,          { XK_F15 }},
        {Qt::Key_F16,          { XK_F16 }},
        {Qt::Key_F17,          { XK_F17 }},
        {Qt::Key_F18,          { XK_F18 }},
        {Qt::Key_F19,          { XK_F19 }},
        {Qt::Key_F20,          { XK_F20 }},
        {Qt::Key_F21,          { XK_F21 }},
        {Qt::Key_F22,          { XK_F22 }},
        {Qt::Key_F23,          { XK_F23 }},
        {Qt::Key_F24,          { XK_F24 }},
        {Qt::Key_F25,          { XK_F25 }},  // F25 .. F35 only on X11
        {Qt::Key_F26,          { XK_F26 }},
        {Qt::Key_F27,          { XK_F27 }},
        {Qt::Key_F28,          { XK_F28 }},
        {Qt::Key_F29,          { XK_F29 }},
        {Qt::Key_F30,          { XK_F30 }},
        {Qt::Key_F31,          { XK_F31 }},
        {Qt::Key_F32,          { XK_F32 }},
        {Qt::Key_F33,          { XK_F33 }},
        {Qt::Key_F34,          { XK_F34 }},
        {Qt::Key_F35,          { XK_F35 }},
        {Qt::Key_Super_L,      { XK_Super_L }},      // extra keys
        {Qt::Key_Super_R,      { XK_Super_R }},
        {Qt::Key_Menu,         { XK_Menu }},
        {Qt::Key_Hyper_L,      { XK_Hyper_L }},
        {Qt::Key_Hyper_R,      { XK_Hyper_R }},
        {Qt::Key_Help,         { XK_Help }},
//       {Qt::Key_Direction_L,  { XF86XK_D }},
//       {Qt::Key_Direction_R,         { XK_ }},
        {Qt::Key_Multi_key, {XK_Multi_key}},
        {Qt::Key_Codeinput, {XK_Codeinput}},
        {Qt::Key_SingleCandidate, {XK_SingleCandidate}},
        {Qt::Key_MultipleCandidate, {XK_MultipleCandidate}},
        {Qt::Key_PreviousCandidate, {XK_PreviousCandidate}},
        {Qt::Key_Mode_switch, {XK_Mode_switch}},
        {Qt::Key_Kanji, {XK_Kanji}},
        {Qt::Key_Muhenkan, {XK_Muhenkan}},
        {Qt::Key_Henkan, {XK_Henkan}},
        {Qt::Key_Romaji, {XK_Romaji}},
        {Qt::Key_Hiragana, {XK_Hiragana}},
        {Qt::Key_Katakana, {XK_Katakana}},
        {Qt::Key_Hiragana_Katakana, {XK_Hiragana_Katakana}},
        {Qt::Key_Zenkaku, {XK_Zenkaku}},
        {Qt::Key_Hankaku, {XK_Hankaku}},
        {Qt::Key_Zenkaku_Hankaku, {XK_Zenkaku_Hankaku}},
        {Qt::Key_Touroku, {XK_Touroku}},
        {Qt::Key_Massyo, {XK_Massyo}},
        {Qt::Key_Kana_Lock, {XK_Kana_Lock}},
        {Qt::Key_Kana_Shift, {XK_Kana_Shift}},
        {Qt::Key_Eisu_Shift, {XK_Eisu_Shift}},
        {Qt::Key_Eisu_toggle, {XK_Eisu_toggle}},
        {Qt::Key_Hangul, {XK_Hangul}},
        {Qt::Key_Hangul_Start, {XK_Hangul_Start}},
        {Qt::Key_Hangul_End, {XK_Hangul_End}},
        {Qt::Key_Hangul_Hanja, {XK_Hangul_Hanja}},
        {Qt::Key_Hangul_Jamo, {XK_Hangul_Jamo}},
        {Qt::Key_Hangul_Romaja, {XK_Hangul_Romaja}},
        {Qt::Key_Hangul_Jeonja, {XK_Hangul_Jeonja}},
        {Qt::Key_Hangul_Banja, {XK_Hangul_Banja}},
        {Qt::Key_Hangul_PreHanja, {XK_Hangul_PreHanja}},
        {Qt::Key_Hangul_PostHanja, {XK_Hangul_PostHanja}},
        {Qt::Key_Hangul_Special, {XK_Hangul_Special}},
        {Qt::Key_Dead_Grave, {XK_dead_grave}},
        {Qt::Key_Dead_Acute, {XK_dead_acute}},
        {Qt::Key_Dead_Circumflex, {XK_dead_circumflex}},
        {Qt::Key_Dead_Tilde, {XK_dead_tilde}},
        {Qt::Key_Dead_Macron, {XK_dead_macron}},
        {Qt::Key_Dead_Breve, {XK_dead_breve}},
        {Qt::Key_Dead_Abovedot, {XK_dead_abovedot}},
        {Qt::Key_Dead_Diaeresis, {XK_dead_diaeresis}},
        {Qt::Key_Dead_Abovering, {XK_dead_abovering}},
        {Qt::Key_Dead_Doubleacute, {XK_dead_doubleacute}},
        {Qt::Key_Dead_Caron, {XK_dead_caron}},
        {Qt::Key_Dead_Cedilla, {XK_dead_cedilla}},
        {Qt::Key_Dead_Ogonek, {XK_dead_ogonek}},
        {Qt::Key_Dead_Iota, {XK_dead_iota}},
        {Qt::Key_Dead_Voiced_Sound, {XK_dead_voiced_sound}},
        {Qt::Key_Dead_Semivoiced_Sound, {XK_dead_semivoiced_sound}},
        {Qt::Key_Dead_Belowdot, {XK_dead_belowdot}},
        {Qt::Key_Dead_Hook, {XK_dead_hook}},
        {Qt::Key_Dead_Horn, {XK_dead_horn}},
        {Qt::Key_Back, {XF86XK_Back}},
        {Qt::Key_Forward, {XF86XK_Forward}},
        {Qt::Key_Stop, {XF86XK_Stop}},
        {Qt::Key_Refresh, {XF86XK_Refresh}},
        {Qt::Key_VolumeDown, {XF86XK_AudioLowerVolume}},
        {Qt::Key_VolumeMute, {XF86XK_AudioMute}},
        {Qt::Key_VolumeUp, {XF86XK_AudioRaiseVolume}},
//        {Qt::Key_BassBoost, {}},
//        {Qt::Key_BassUp, {}},
//        {Qt::Key_BassDown, {}},
//        {Qt::Key_TrebleUp, {}},
//        {Qt::Key_TrebleDown, {}},
        {Qt::Key_MediaPlay, {XF86XK_AudioPlay}},
        {Qt::Key_MediaStop, {XF86XK_AudioStop}},
        {Qt::Key_MediaPrevious, {XF86XK_AudioPrev}},
        {Qt::Key_MediaNext, {XF86XK_AudioNext}},
        {Qt::Key_MediaRecord, {XF86XK_AudioRecord}},
        {Qt::Key_MediaPause, {XF86XK_AudioPause}},
//        {Qt::Key_MediaTogglePlayPause, {}},
        {Qt::Key_HomePage, {XF86XK_HomePage}},
        {Qt::Key_Favorites, {XF86XK_Favorites}},
        {Qt::Key_Search, {XF86XK_Search}},
        {Qt::Key_Standby, {XF86XK_Standby}},
        {Qt::Key_OpenUrl, {XF86XK_OpenURL}},
        {Qt::Key_LaunchMail, {XF86XK_Mail}},
//        {Qt::Key_LaunchMedia, {}},
        {Qt::Key_Launch0, {XF86XK_Launch0}},
        {Qt::Key_Launch1, {XF86XK_Launch1}},
        {Qt::Key_Launch2, {XF86XK_Launch2}},
        {Qt::Key_Launch3, {XF86XK_Launch3}},
        {Qt::Key_Launch4, {XF86XK_Launch4}},
        {Qt::Key_Launch5, {XF86XK_Launch5}},
        {Qt::Key_Launch6, {XF86XK_Launch6}},
        {Qt::Key_Launch7, {XF86XK_Launch7}},
        {Qt::Key_Launch8, {XF86XK_Launch8}},
        {Qt::Key_Launch9, {XF86XK_Launch9}},
        {Qt::Key_LaunchA, {XF86XK_LaunchA}},
        {Qt::Key_LaunchB, {XF86XK_LaunchB}},
        {Qt::Key_LaunchC, {XF86XK_LaunchC}},
        {Qt::Key_LaunchD, {XF86XK_LaunchD}},
        {Qt::Key_LaunchE, {XF86XK_LaunchE}},
        {Qt::Key_LaunchF, {XF86XK_LaunchF}},
//        {Qt::Key_LaunchG, {}},
//        {Qt::Key_LaunchH, {}},
        {Qt::Key_MonBrightnessUp, {XF86XK_MonBrightnessUp}},
        {Qt::Key_MonBrightnessDown, {XF86XK_MonBrightnessDown}},
        {Qt::Key_KeyboardLightOnOff, {XF86XK_KbdLightOnOff}},
        {Qt::Key_KeyboardBrightnessUp, {XF86XK_KbdBrightnessUp}},
        {Qt::Key_KeyboardBrightnessDown, {XF86XK_KbdBrightnessDown}},
        {Qt::Key_PowerOff, {XF86XK_PowerOff}},
        {Qt::Key_WakeUp, {XF86XK_WakeUp}},
        {Qt::Key_Eject, {XF86XK_Eject}},
        {Qt::Key_ScreenSaver, {XF86XK_ScreenSaver}},
        {Qt::Key_WWW, {XF86XK_WWW}},
        {Qt::Key_Memo, {XF86XK_Memo}},
        {Qt::Key_LightBulb, {XF86XK_LightBulb}},
        {Qt::Key_Shop, {XF86XK_Shop}},
        {Qt::Key_History, {XF86XK_History}},
        {Qt::Key_AddFavorite, {XF86XK_AddFavorite}},
        {Qt::Key_HotLinks, {XF86XK_HotLinks}},
        {Qt::Key_BrightnessAdjust, {XF86XK_BrightnessAdjust}},
        {Qt::Key_Finance, {XF86XK_Finance}},
        {Qt::Key_Community, {XF86XK_Community}},
        {Qt::Key_AudioRewind, {XF86XK_AudioRewind}},
        {Qt::Key_BackForward, {XF86XK_BackForward}},
        {Qt::Key_ApplicationLeft, {XF86XK_ApplicationLeft}},
        {Qt::Key_ApplicationRight, {XF86XK_ApplicationRight}},
        {Qt::Key_Book, {XF86XK_Book}},
        {Qt::Key_CD, {XF86XK_CD}},
        {Qt::Key_Calculator, {XF86XK_Calculator}},
        {Qt::Key_ToDoList, {XF86XK_ToDoList}},
        {Qt::Key_ClearGrab, {XF86XK_ClearGrab}},
        {Qt::Key_Close, {XF86XK_Close}},
        {Qt::Key_Copy, {XF86XK_Copy}},
        {Qt::Key_Cut, {XF86XK_Cut}},
        {Qt::Key_Display, {XF86XK_Display}},
        {Qt::Key_DOS, {XF86XK_DOS}},
        {Qt::Key_Documents, {XF86XK_Documents}},
        {Qt::Key_Excel, {XF86XK_Excel}},
        {Qt::Key_Explorer, {XF86XK_Explorer}},
        {Qt::Key_Game, {XF86XK_Game}},
        {Qt::Key_Go, {XF86XK_Go}},
        {Qt::Key_iTouch, {XF86XK_iTouch}},
        {Qt::Key_LogOff, {XF86XK_LogOff}},
        {Qt::Key_Market, {XF86XK_Market}},
        {Qt::Key_Meeting, {XF86XK_Meeting}},
        {Qt::Key_MenuKB, {XF86XK_MenuKB}},
        {Qt::Key_MenuPB, {XF86XK_MenuPB}},
        {Qt::Key_MySites, {XF86XK_MySites}},
        {Qt::Key_News, {XF86XK_News}},
        {Qt::Key_OfficeHome, {XF86XK_OfficeHome}},
        {Qt::Key_Option, {XF86XK_Option}},
        {Qt::Key_Paste, {XF86XK_Paste}},
        {Qt::Key_Phone, {XF86XK_Phone}},
        {Qt::Key_Calendar, {XF86XK_Calendar}},
        {Qt::Key_Reply, {XF86XK_Reply}},
        {Qt::Key_Reload, {XF86XK_Reload}},
        {Qt::Key_RotateWindows, {XF86XK_RotateWindows}},
        {Qt::Key_RotationPB, {XF86XK_RotationPB}},
        {Qt::Key_RotationKB, {XF86XK_RotationKB}},
        {Qt::Key_Save, {XF86XK_Save}},
        {Qt::Key_Send, {XF86XK_Send}},
        {Qt::Key_Spell, {XF86XK_Spell}},
        {Qt::Key_SplitScreen, {XF86XK_SplitScreen}},
        {Qt::Key_Support, {XF86XK_Support}},
        {Qt::Key_TaskPane, {XF86XK_TaskPane}},
        {Qt::Key_Terminal, {XF86XK_Terminal}},
        {Qt::Key_Tools, {XF86XK_Tools}},
        {Qt::Key_Travel, {XF86XK_Travel}},
        {Qt::Key_Video, {XF86XK_Video}},
        {Qt::Key_Word, {XF86XK_Word}},
        {Qt::Key_Xfer, {XF86XK_Xfer}},
        {Qt::Key_ZoomIn, {XF86XK_ZoomIn}},
        {Qt::Key_ZoomOut, {XF86XK_ZoomOut}},
        {Qt::Key_Away, {XF86XK_Away}},
        {Qt::Key_Messenger, {XF86XK_Messenger}},
        {Qt::Key_WebCam, {XF86XK_WebCam}},
        {Qt::Key_MailForward, {XF86XK_MailForward}},
        {Qt::Key_Pictures, {XF86XK_Pictures}},
        {Qt::Key_Music, {XF86XK_Music}},
        {Qt::Key_Battery, {XF86XK_Battery}},
        {Qt::Key_Bluetooth, {XF86XK_Bluetooth}},
        {Qt::Key_WLAN, {XF86XK_WLAN}},
        {Qt::Key_UWB, {XF86XK_UWB}},
        {Qt::Key_AudioForward, {XF86XK_AudioForward}},
        {Qt::Key_AudioRepeat, {XF86XK_AudioRepeat}},
        {Qt::Key_AudioRandomPlay, {XF86XK_AudioRandomPlay}},
        {Qt::Key_Subtitle, {XF86XK_Subtitle}},
        {Qt::Key_AudioCycleTrack, {XF86XK_AudioCycleTrack}},
        {Qt::Key_Time, {XF86XK_Time}},
        {Qt::Key_Hibernate, {XF86XK_Hibernate}},
        {Qt::Key_View, {XF86XK_View}},
        {Qt::Key_TopMenu, {XF86XK_TopMenu}},
        {Qt::Key_PowerDown, {XF86XK_PowerDown}},
        {Qt::Key_Suspend, {XF86XK_Suspend}},
        {Qt::Key_ContrastAdjust, {XF86XK_ContrastAdjust}},
        {Qt::Key_TouchpadToggle, {XF86XK_TouchpadToggle}},
        {Qt::Key_TouchpadOn, {XF86XK_TouchpadOn}},
        {Qt::Key_TouchpadOff, {XF86XK_TouchpadOff}},
        {Qt::Key_MicMute, {XF86XK_AudioMicMute}},
        {Qt::Key_Red, {XF86XK_Red}},
        {Qt::Key_Green, {XF86XK_Green}},
        {Qt::Key_Yellow, {XF86XK_Yellow}},
        {Qt::Key_Blue, {XF86XK_Blue}},
//        {Qt::Key_ChannelUp, {}},
//        {Qt::Key_ChannelDown, {}},
//        {Qt::Key_Guide, {}},
//        {Qt::Key_Info, {}},
//        {Qt::Key_Settings, {}},
//        {Qt::Key_MicVolumeUp, {}},
//        {Qt::Key_MicVolumeDown, {}},
        {Qt::Key_New, {XF86XK_New}},
        {Qt::Key_Open, {XF86XK_Open}},
        {Qt::Key_Find, {XK_Find}},
        {Qt::Key_Undo, {XK_Undo}},
        {Qt::Key_Redo, {XK_Redo}},
//        {Qt::Key_MediaLast, {}},
//        {Qt::Key_unknown, {}},
        {Qt::Key_Call, {XF86XK_Phone}},
        {Qt::Key_Camera, {XF86XK_WebCam}},
//        {Qt::Key_CameraFocus, {}},
//        {Qt::Key_Context1, {}},
//        {Qt::Key_Context2, {}},
//        {Qt::Key_Context3, {}},
//        {Qt::Key_Context4, {}},
//        {Qt::Key_Flip, {}},
//        {Qt::Key_Hangup, {}},
//        {Qt::Key_No, {}},
        {Qt::Key_Select, {XF86XK_Select}},
        {Qt::Key_Yes, {}},
        {Qt::Key_ToggleCallHangup, {}},
        {Qt::Key_VoiceDial, {}},
        {Qt::Key_LastNumberRedial, {}},
        {Qt::Key_Execute, {XK_Execute}},
//        {Qt::Key_Printer, {}},
//        {Qt::Key_Play, {}},
        {Qt::Key_Sleep, {XF86XK_Sleep}},
        {Qt::Key_Zoom, {XF86XK_ZoomIn}},
        {Qt::Key_Exit, {XK_Cancel}},
        {Qt::Key_Cancel, {XK_Cancel}}
    };


    bool lastGrabFailed;
    static int XGrabErrorHandler(Display *, XErrorEvent *e) {
        qWarning() << "XGrabError: "<< e->type;
        lastGrabFailed = true;
        return 0;
    }


//    /\*\* \*{20}\**//
//    class XLibManager
//    {
//    public:
//        XLibManager(){
//            display = XOpenDisplay(NULL); // Open $DISPLAY
//            if ( !display )
//                qFatal("Could not open DISPLAY");
//            defaultScreen = DefaultScreenOfDisplay(display);
//            defaultRootWindow = DefaultRootWindow(display);
//        }

//        ~XLibManager(){
//            XCloseDisplay(display);
//        }

//        Display *display;
//        Screen  *defaultScreen;
//        Window  defaultRootWindow;
//    };


//    /\*\* \*{20}\**//
//    class XCBManager
//    {
//    public:
//        XCBManager(){
//            connection = xcb_connect(NULL, &defaultScreenNumber); // Open $DISPLAY
//            defaultScreen = screen_of_display(connection, defaultScreenNumber);
//            if ( !defaultScreen )
//                qFatal("No default screen found!");
//            defaultRootWindow = defaultScreen->root;
//        }

//        ~XCBManager(){
//            xcb_disconnect(connection);
//        }

//        xcb_connection_t *connection;
//        int               defaultScreenNumber;
//        xcb_screen_t     *defaultScreen;
//        xcb_window_t      defaultRootWindow;
//    };



}


GlobalShortcut::HotkeyManagerPrivate::HotkeyManagerPrivate(QObject *parent)
    : QObject(parent) {

    Display* dpy = QX11Info::display();
//        xcb_connection_t* conn = QX11Info::connection();
//        xcb_generic_error_t **e = nullptr ;

    XModifierKeymap* map = XGetModifierMapping(dpy); // Contains the keys being used as modifiers.
    // TODO
//        xcb_get_modifier_mapping_cookie_t mmc = xcb_get_modifier_mapping(conn);
//        xcb_get_modifier_mapping_reply_t *mmr = xcb_get_modifier_mapping_reply (conn, mmc, e);

    masks = {0,0,0,0,0,0};
    if (map) {
        /* The XDisplayKeycodes() function returns the min-keycodes and
        max-keycodes supported by the specified display. The minimum number
        of KeyCodes returned is never less than 8, and the maximum number
        of KeyCodes returned is never greater than 255. Not all KeyCodes in
        this range are required to have corresponding keys.*/
        int min_keycode, max_keycode;
        XDisplayKeycodes (dpy, &min_keycode, &max_keycode);
//          min_keycode = xcb_get_setup(QX11Info::connection())->min_keycode;
//          max_keycode = xcb_get_setup(QX11Info::connection())->max_keycode;

        /* The XGetKeyboardMapping() function returns the symbols for the
        specified number of KeyCodes starting with first_keycode.
        KeySym number N, counting from zero, for KeyCode K has the following
        index in the list, counting from zero:
                (K - first_code) * keysyms_per_code_return + N
        A special KeySym value of NoSymbol is used to fill in unused
        elements for individual KeyCodes. To free the storage returned by
        XGetKeyboardMapping(), use XFree(). */
        int keysyms_per_keycode;
        XFree(XGetKeyboardMapping (dpy,
                                   (KeyCode)min_keycode,
                                   (max_keycode- min_keycode + 1),
                                   &keysyms_per_keycode));
//            uint8_t keysyms_per_keycode;
//            xcb_get_keyboard_mapping_cookie_t kmc =
//            xcb_get_keyboard_mapping_unchecked(X11Info::connection(),
//                                               (xcb_keycode_t)min_keycode,
//                                               (max_keycode- min_keycode + 1));
//            xcb_get_keyboard_mapping_reply_t *kmr =
//            xcb_get_keyboard_mapping_reply (X11Info::connection(),kmc,NULL);
//            keysyms_per_keycode = kmr->keysyms_per_keycode;
//            free(kmr);

        KeyCode kc;
        for (int maskIndex = 0; maskIndex < 8; maskIndex++) {
            for (int i = 0; i < map->max_keypermod; i++) {
                kc = map->modifiermap[maskIndex*map->max_keypermod+i];
                if (kc) {
                    KeySym sym;
                    int symIndex = 0;
                    do {
                        sym = XkbKeycodeToKeysym(dpy, kc, 0, symIndex);
//                            qDebug() <<  XKeysymToString(sym);
                        symIndex++;
                    } while ( sym == NoSymbol && symIndex < keysyms_per_keycode);
                    if (masks.alt == 0 && (sym == XK_Alt_L || sym == XK_Alt_R))
                        masks.alt = 1 << maskIndex;
                    if (masks.meta == 0 && (sym == XK_Meta_L || sym == XK_Meta_R))
                        masks.meta = 1 << maskIndex;
                    if (masks.super == 0 && (sym == XK_Super_L || sym == XK_Super_R))
                        masks.super = 1 << maskIndex;
                    if (masks.hyper == 0 && (sym == XK_Hyper_L || sym == XK_Hyper_R))
                        masks.hyper = 1 << maskIndex;
                    if (masks.numlock == 0 && (sym == XK_Num_Lock))
                        masks.numlock = 1 << maskIndex;
//                        if (masks.iso_level3_shift == 0 && (sym == XK_ISO_Level3_Shift))
//                            masks.iso_level3_shift = 1 << maskIndex;
                }
            }
        }
        XFreeModifiermap(map);

        // logic from qt source see gui/kernel/qkeymapper_x11.cpp
        if (masks.meta == 0 || masks.meta == masks.alt) {
            // no meta keys... s,meta,super,
            masks.meta = masks.super;
            if (masks.meta == 0 || masks.meta == masks.alt) {
                // no super keys either? guess we'll use hyper then
                masks.meta = masks.hyper;
            }
        }
    }
    else {
        // assume defaults
        masks.alt     = Mod1Mask;
        masks.meta    = Mod1Mask;
        masks.numlock = Mod2Mask;
        masks.super   = Mod4Mask;
        masks.hyper   = Mod4Mask;
//            masks.iso_level3_shift = Mod5Mask;
    }

    offendingMasks = {0, LockMask, masks.numlock, (LockMask|masks.numlock)};

    QAbstractEventDispatcher::instance()->installNativeEventFilter(this);
}




GlobalShortcut::HotkeyManagerPrivate::~HotkeyManagerPrivate() {

}




bool GlobalShortcut::HotkeyManagerPrivate::registerNativeHotkey(uint hotkey) {

    // Convert to native format
    std::set<uint> keysX = nativeKeycodes(hotkey & ~Qt::KeyboardModifierMask);
    uint modsX = nativeModifiers(hotkey &  Qt::KeyboardModifierMask);
    if ( keysX.empty() )
        return false;

    // Set own errorhandler
    XErrorHandler savedErrorHandler = XSetErrorHandler(XGrabErrorHandler);

    // Grab the key combos (potenzmenge aus mods und keys)
    lastGrabFailed = false;
    std::set<std::pair<uint,uint>> tmpGrabbedKeys;
    for ( uint keySym : keysX ) {

        // Break if a grab failed
        if ( lastGrabFailed )
            break;

        KeyCode XKCode = XKeysymToKeycode(QX11Info::display(), keySym);

        // On X lock and numlock are modifiers. Just grab all of them.
        for ( uint mask : offendingMasks ) {
            XGrabKey(QX11Info::display(), XKCode, modsX|mask, QX11Info::appRootWindow(), true, GrabModeAsync, GrabModeAsync);
            if ( !lastGrabFailed )
                tmpGrabbedKeys.insert({modsX|mask, XKCode});
        }
    }

    // Reset errorhandler
    XSetErrorHandler(savedErrorHandler);

    if ( !lastGrabFailed )
        std::copy(tmpGrabbedKeys.begin(), tmpGrabbedKeys.end(),
                  std::inserter(grabbedKeys, grabbedKeys.begin()));
    else
        // In case of error unregister the partial registration
        for (const std::pair<uint,uint> &p : tmpGrabbedKeys)
            XUngrabKey(QX11Info::display(), p.second, p.first, QX11Info::appRootWindow());

    XSync(QX11Info::display(), False);
    return !lastGrabFailed;



    //    xcb_void_cookie_t ck = xcb_grab_key(
    //                QX11Info::connection(),
    //                true,
    //                QX11Info::appRootWindow(),
    //                uint16_t modifiers,
    //                xcb_keycode_t key,
    //                XCB_GRAB_MODE_ASYNC,
    //                XCB_GRAB_MODE_ASYNC
    //                );


    //    xcb_generic_error_t *err = xcb_request_check (QX11Info::connection(), ck);
    //    if (err != NULL) {
    //        qWarning("X11 error %d", err->error_code);
    //        free (err);
    //    }
}




void GlobalShortcut::HotkeyManagerPrivate::unregisterNativeHotkey(uint hotkey) {

    // Convert to native format
    std::set<uint> keysX = nativeKeycodes(hotkey & ~Qt::KeyboardModifierMask);
    uint modsX = nativeModifiers(hotkey &  Qt::KeyboardModifierMask);
    if ( keysX.empty() )
        qCritical() << "keysX should not be empty";

    // Set own errorhandler
    XErrorHandler savedErrorHandler = XSetErrorHandler(XGrabErrorHandler);

    // UNgrab the key combos (potenzmenge aus mods und keys)
    for ( uint keySym : keysX ) {

        KeyCode XKCode = XKeysymToKeycode(QX11Info::display(), keySym);

        // On X lock and numlock are modifiers. Just ungrab all of them.
        for ( uint mask : offendingMasks ) {
            XUngrabKey(QX11Info::display(), XKCode, modsX|mask, QX11Info::appRootWindow());
            grabbedKeys.erase({modsX|mask, XKCode});
        }
    }

    // Reset errorhandler
    XSetErrorHandler(savedErrorHandler);

    XSync(QX11Info::display(), False);
}




std::set<uint> GlobalShortcut::HotkeyManagerPrivate::nativeKeycodes(uint qtKey) {
    /* Translate key symbol ( Qt -> X ) */
    // Use latin if possible
    if (qtKey >= 0x20 && qtKey <= 0xff)
        return std::set<uint>({qtKey});
    else {  // else check the handcrafted table for fancy keys
        auto it = QtToXSymsMap.find(qtKey);
        if ( it != QtToXSymsMap.end() )
            return it->second;
        else {
            qCritical() << "Could not translate key!"<< QKeySequence(qtKey).toString();
            return std::set<uint>();
        }
    }
}




uint GlobalShortcut::HotkeyManagerPrivate::nativeModifiers(uint qtMods) {
    uint ret = 0;
    //    if (qtMods & Qt::ShiftModifier)       ret |= XCB_MOD_MASK_SHIFT;
    //    if (qtMods & Qt::ControlModifier)     ret |= XCB_MOD_MASK_CONTROL;
    if (qtMods & Qt::ShiftModifier)       ret |= ShiftMask;
    if (qtMods & Qt::ControlModifier)     ret |= ControlMask;
    if (qtMods & Qt::AltModifier)         ret |= masks.alt;
    if (qtMods & Qt::MetaModifier)        ret |= masks.super;
//    if (qtMods & Qt::KeypadModifier)      ret |= masks.meta;
//    if (qtMods & Qt::GroupSwitchModifier) ret |= masks.iso_level3_shift;
    return ret;
}




bool GlobalShortcut::HotkeyManagerPrivate::nativeEventFilter(const QByteArray &eventType, void *message, long *) {
    if ( eventType == "xcb_generic_event_t" ) {
        xcb_generic_event_t* ev = static_cast<xcb_generic_event_t *>(message);
        if ( (ev->response_type & 127) == XCB_KEY_PRESS ) {
            xcb_key_press_event_t *k = reinterpret_cast<xcb_key_press_event_t*>(ev);
            // Check if the key is one of the registered
            for ( const std::pair<uint,uint> &p: grabbedKeys ) {
                if (k->detail == p.second && k->state == p.first) {
                    emit hotKeyPressed();
                    return true;
                }
            }
        }
    }
    return false;
}
