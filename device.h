#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <hidport.h>

// 고유한 벤더 ID와 제품 ID로 변경
#define VENDOR_ID           0x5678  // 가상 장치임을 나타내는 고유 ID
#define PRODUCT_ID          0x1234  // 가상 장치임을 나타내는 고유 ID 
#define VERSION_NUMBER      0x0100  // 버전 1.0

// 분리된 입력 스트림을 위한 플래그 정의
#define HID_INPUT_FLAG_SEPARATED 0x01

// IOCTL 코드 - 분리된 스트림 모드 제어
#define IOCTL_CUSTOM_SET_SEPARATED_STREAM CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef UCHAR HID_REPORT_DESCRIPTOR, * PHID_REPORT_DESCRIPTOR;

typedef struct _DEVICE_CONTEXT {
    WDFDEVICE device;
    WDFQUEUE queueDefault;
    WDFQUEUE queueManual;
    HID_DEVICE_ATTRIBUTES hidDeviceAttributes;
    PHID_DESCRIPTOR hidDescriptor;
    PHID_REPORT_DESCRIPTOR hidReportDescriptor;

    // 가상 HID 상태를 추적하기 위한 플래그 추가
    BOOLEAN isSeparatedStream;
} DEVICE_CONTEXT, * PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext);

NTSTATUS
DeviceCreate(
    _Inout_ PWDFDEVICE_INIT deviceInit
);

// 분리된 스트림 모드 설정 함수 추가
NTSTATUS
SetSeparatedStreamMode(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ BOOLEAN Enable
);

NTSTATUS
SendAbsoluteMouseInput(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ USHORT X,
    _In_ USHORT Y,
    _In_ BYTE Buttons,
    _In_ BOOLEAN IsSeparatedStream
);

NTSTATUS
SendKeyboardInput(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ BYTE Modifiers,
    _In_ BYTE KeyCodes[6]
);

BOOLEAN
ShouldBlockVirtualInput(
    _In_ PDEVICE_CONTEXT DeviceContext
);
