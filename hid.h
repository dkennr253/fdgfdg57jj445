#pragma once

#include <ntddk.h>
#include <hidport.h>

// 기존 보고서 ID
#define REPORT_ID_MOUSE_INPUT 0x01
#define REPORT_ID_MOUSE_OUTPUT 0x02
#define REPORT_ID_KEYBOARD_INPUT 0x03
#define REPORT_ID_KEYBOARD_OUTPUT 0x04
#define REPORT_ID_MOUSE_ABSOLUTE 0x5A

#pragma pack(1)

// 상대 좌표 마우스 입력 보고서
typedef struct _HID_MOUSE_INPUT_REPORT {
    BYTE reportId;
    BYTE buttons;
    CHAR x;
    CHAR y;
} HID_MOUSE_INPUT_REPORT, * PHID_MOUSE_INPUT_REPORT;

// 절대 좌표 마우스 입력 보고서 추가 (분리된 입력 스트림용)
typedef struct _HID_MOUSE_ABSOLUTE_REPORT {
    BYTE reportId;     // REPORT_ID_MOUSE_ABSOLUTE (0x5A)
    BYTE flags;        // 특별 플래그 (분리된 스트림 여부 등)
    BYTE buttons;      // 버튼 상태
    USHORT x;          // 절대 X 좌표 (0-32767)
    USHORT y;          // 절대 Y 좌표 (0-32767)
} HID_MOUSE_ABSOLUTE_REPORT, * PHID_MOUSE_ABSOLUTE_REPORT;

typedef struct _HID_MOUSE_OUTPUT_REPORT {
    BYTE reportId;
    BYTE buttons;
    CHAR x;
    CHAR y;
} HID_MOUSE_OUTPUT_REPORT, * PHID_MOUSE_OUTPUT_REPORT;

typedef struct _HID_KEYBOARD_INPUT_REPORT {
    BYTE reportId;
    BYTE modifiers;
    BYTE _reserved;
    BYTE keyCodes[6];
} HID_KEYBOARD_INPUT_REPORT, * PHID_KEYBOARD_INPUT_REPORT;

typedef struct _HID_KEYBOARD_OUTPUT_REPORT {
    BYTE reportId;
    BYTE modifiers;
    BYTE _reserved;
    BYTE keyCodes[6];
} HID_KEYBOARD_OUTPUT_REPORT, * PHID_KEYBOARD_OUTPUT_REPORT;

#pragma pack()
