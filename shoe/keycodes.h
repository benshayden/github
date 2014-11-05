// Add these to KEY_*.
#define MOD_CONTROL 0x0100
#define MOD_SHIFT 0x0200
#define MOD_ALT 0x0400
#define MOD_GUI 0x0800

// http://www.freebsddiary.org/APC/usb_hid_usages.php
#define KEY_A                   4
#define KEY_B                   5
#define KEY_C                   6
#define KEY_D                   7
#define KEY_E                   8
#define KEY_F                   9
#define KEY_G                   10
#define KEY_H                   11
#define KEY_I                   12
#define KEY_J                   13
#define KEY_K                   14
#define KEY_L                   15
#define KEY_M                   16
#define KEY_N                   17
#define KEY_O                   18
#define KEY_P                   19
#define KEY_Q                   20
#define KEY_R                   21
#define KEY_S                   22
#define KEY_T                   23
#define KEY_U                   24
#define KEY_V                   25
#define KEY_W                   26
#define KEY_X                   27
#define KEY_Y                   28
#define KEY_Z                   29
#define KEY_1                   30
#define KEY_2                   31
#define KEY_3                   32
#define KEY_4                   33
#define KEY_5                   34
#define KEY_6                   35
#define KEY_7                   36
#define KEY_8                   37
#define KEY_9                   38
#define KEY_0                   39
#define KEY_ENTER               40
#define KEY_ESC                 41
#define KEY_BACKSPACE           42
#define KEY_TAB                 43
#define KEY_SPACE               44
#define KEY_MINUS               45
#define KEY_EQUAL               46
#define KEY_LEFT_BRACE          47
#define KEY_RIGHT_BRACE         48
#define KEY_BACKSLASH           49
#define KEY_NON_US_NUM          50
#define KEY_SEMICOLON           51
#define KEY_QUOTE               52
#define KEY_TILDE               53
#define KEY_COMMA               54
#define KEY_PERIOD              55
#define KEY_SLASH               56
#define KEY_CAPS_LOCK           57
#define KEY_F1                  58
#define KEY_F2                  59
#define KEY_F3                  60
#define KEY_F4                  61
#define KEY_F5                  62
#define KEY_F6                  63
#define KEY_F7                  64
#define KEY_F8                  65
#define KEY_F9                  66
#define KEY_F10                 67
#define KEY_F11                 68
#define KEY_F12                 69
#define KEY_PRINTSCREEN         70
#define KEY_SCROLL_LOCK         71
#define KEY_PAUSE               72
#define KEY_INSERT              73
#define KEY_HOME                74
#define KEY_PAGE_UP             75
#define KEY_DELETE              76
#define KEY_END                 77
#define KEY_PAGE_DOWN           78
#define KEY_RIGHT               79
#define KEY_LEFT                80
#define KEY_DOWN                81
#define KEY_UP                  82
#define KEY_NUM_LOCK            83
#define KEYPAD_SLASH            84
#define KEYPAD_ASTERIX          85
#define KEYPAD_MINUS            86
#define KEYPAD_PLUS             87
#define KEYPAD_ENTER            88
#define KEYPAD_1                89
#define KEYPAD_2                90
#define KEYPAD_3                91
#define KEYPAD_4                92
#define KEYPAD_5                93
#define KEYPAD_6                94
#define KEYPAD_7                95
#define KEYPAD_8                96
#define KEYPAD_9                97
#define KEYPAD_0                98
#define KEYPAD_PERIOD           99
#define KEY_NON_US_SLASH        100
#define KEY_APPLICATION         101
#define KEY_POWER               102
#define KEYPAD_EQUAL            103
#define KEY_F13                 104
#define KEY_F14                 105
#define KEY_F15                 106
#define KEY_F16                 107
#define KEY_F17                 108
#define KEY_F18                 109
#define KEY_F19                 110
#define KEY_F20                 111
#define KEY_F21                 112
#define KEY_F22                 113
#define KEY_F23                 114
#define KEY_F24                 115
#define KEY_EXECUTE             116
#define KEY_HELP                117
#define KEY_MENU                118
#define KEY_SELECT              119
#define KEY_STOP                120
#define KEY_AGAIN               121
#define KEY_UNDO                122
#define KEY_CUT                 123
#define KEY_COPY                124
#define KEY_PASTE               125
#define KEY_FIND                126
#define KEY_MUTE                127
#define KEY_VOLUME_UP           128
#define KEY_VOLUME_DOWN         129
#define KEY_LOCKING_CAPS_LOCK   130
#define KEY_LOCKING_NUM_LOCK    131
#define KEY_LOCKING_SCROLL_LOCK 132
#define KEYPAD_COMMA            133
#define KEYPAD_EQUAL_SIGN       134
#define KEYPAD_INT1             135
#define KEYPAD_INT2             136
#define KEYPAD_INT3             137
#define KEYPAD_INT4             138
#define KEYPAD_INT5             139
#define KEYPAD_INT6             140
#define KEYPAD_INT7             141
#define KEYPAD_INT8             142
#define KEYPAD_INT9             143
#define KEYPAD_LANG1            144
#define KEYPAD_LANG2            145
#define KEYPAD_LANG3            146
#define KEYPAD_LANG4            147
#define KEYPAD_LANG5            148
#define KEYPAD_LANG6            149
#define KEYPAD_LANG7            150
#define KEYPAD_LANG8            151
#define KEYPAD_LANG9            152
#define KEY_ALT_ERASE           153
#define KEY_SYS_REQ             154
#define KEY_CANCEL              155
#define KEY_CLEAR               156
#define KEY_PRIOR               157
#define KEY_RETURN              158
#define KEY_SEPARATOR           159
#define KEY_OUT                 160
#define KEY_OPER                161
#define KEY_CLEAR_AGAIN         162
#define KEY_CRSEL_PROPS         163
#define KEY_EXSEL               164
#define KEY_NEXT                165
#define KEY_PREV                166
#define KEY_EJECT               167

// These are in effect from when the button is pressed until the release of the
// next button that is pressed, unless KEY_LOCK_NEXT was pressed before this key.
// Pressing these keys twice in a row will do nothing.
#define KEY_MODE0_1             168
#define KEY_MODE1_1             169
#define KEY_CONTROL_1           170
#define KEY_SHIFT_1             171
#define KEY_ALT_1               172
#define KEY_GUI_1               173

// Pressing KEY_LOCK_NEXT before KEY_{CONTROL,SHIFT,ALT,GUI,MODE{0,1}}_1 or
// KEY_{LEFT,RIGHT}_{CONTROL,SHIFT,ALT,GUI} turns it into the locking version.
// Pressing KEY_LOCK_NEXT before KEY_MACRO* will record that macro until that
// macro key is pressed again. Pressing this key twice in a row will do nothing.
// Do not add this to MOD_*.
#define KEY_LOCK_NEXT           174

#define KEY_RESERVED            175

#define KEYPAD_00               176
#define KEYPAD_000              177
#define KEY_THOUSANDS           178
#define KEY_DECIMAL             179
#define KEY_CURRENCY            180
#define KEY_SUBCURRENCY         181
#define KEYPAD_OPEN_PAREN       182
#define KEYPAD_CLOSE_PAREN      183
#define KEYPAD_OPEN_BRACE       184
#define KEYPAD_CLOSE_BRACE      185
#define KEYPAD_TAB              186
#define KEYPAD_BACKSPACE        187
#define KEYPAD_A                188
#define KEYPAD_B                189
#define KEYPAD_C                190
#define KEYPAD_D                191
#define KEYPAD_E                192
#define KEYPAD_F                193
#define KEYPAD_XOR              194
#define KEYPAD_CARET            195
#define KEYPAD_PERCENT          196
#define KEYPAD_LESS_THAN        197
#define KEYPAD_GREATER_THAN     198
#define KEYPAD_AMPERSAND        199
#define KEYPAD_DOUBLE_AMPERSAND 200
#define KEYPAD_PIPE             201
#define KEYPAD_DOUBLE_PIPE      202
#define KEYPAD_COLON            203
#define KEYPAD_HASH             204
#define KEYPAD_SPACE            205
#define KEYPAD_AT               206
#define KEYPAD_BANG             207
#define KEYPAD_MEM_STORE        208
#define KEYPAD_MEM_RECALL       209
#define KEYPAD_MEM_CLEAR        210
#define KEYPAD_MEM_ADD          211
#define KEYPAD_MEM_SUBTRACT     212
#define KEYPAD_MEM_MULTIPLY     213
#define KEYPAD_MEM_DIVIDE       214
#define KEYPAD_PLUS_MINUS       215
#define KEYPAD_CLEAR            216
#define KEYPAD_CLEAR_ENTRY      217
#define KEYPAD_BINARY           218
#define KEYPAD_OCTAL            219
#define KEYPAD_DECIMAL          220
#define KEYPAD_HEX              221

// These are in effect from when the button is pressed until it is pressed
// again.
#define KEY_MODE0_LOCK          222
#define KEY_MODE1_LOCK          223

// These are in effect only while the button is still pressed.
#define KEY_LEFT_CONTROL        224
#define KEY_LEFT_SHIFT          225
#define KEY_LEFT_ALT            226
#define KEY_LEFT_GUI            227
#define KEY_RIGHT_CTRL_KEY      228/*TODO*/
#define KEY_RIGHT_SHIFT_KEY     229/*TODO*/
#define KEY_RIGHT_ALT_KEY       230/*TODO*/
#define KEY_RIGHT_GUI_KEY       231/*TODO*/
#define KEY_MODE0               232
#define KEY_MODE1               233

// These are in effect from when the button is pressed until it is pressed
// again. Pressing KEY_*_1 will toggle it off for one key press, and similarly
// for the while-held modifier keys.
#define KEY_LEFT_CONTROL_LOCK   234
#define KEY_LEFT_SHIFT_LOCK     235
#define KEY_LEFT_ALT_LOCK       236
#define KEY_LEFT_GUI_LOCK       237

// TODO record
// Record and replay key sequences. See SETTINGS in shoe.ino for the number and
// length of macros.
#define KEY_MACRO0              238
#define KEY_MACRO1              239
#define KEY_MACRO2              240
#define KEY_MACRO3              241
#define KEY_MACRO4              242
#define KEY_MACRO5              243
#define KEY_MACRO6              244
#define KEY_MACRO7              245
#define KEY_MACRO8              246
#define KEY_MACRO9              247
#define KEY_MACRO10             248
#define KEY_MACRO11             249
#define KEY_MACRO12             250
#define KEY_MACRO13             251
#define KEY_MACRO14             252
#define KEY_MACRO15             253
#define KEY_MACRO16             254
#define KEY_MACRO17             255

