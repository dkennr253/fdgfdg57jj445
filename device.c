#include "device.h"
#include "queue_default.h"
#include "queue_manual.h"

// 플래그 정의 추가
#define HID_INPUT_FLAG_SEPARATED 0x01

// IOCTL 코드 추가 - 분리된 스트림 모드 제어
#define IOCTL_CUSTOM_SET_SEPARATED_STREAM CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

HID_REPORT_DESCRIPTOR g_reportDescriptor[] = {
    // 상대 좌표 마우스 컬렉션
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xA1, 0x01,                    // COLLECTION (Application)
    0x85, REPORT_ID_MOUSE_INPUT,   // Report ID
    0x09, 0x01,                    // USAGE (Pointer)
    0xA1, 0x00,                    // COLLECTION (Physical)
    0x05, 0x09,                    // USAGE_PAGE (Buttons)
    0x19, 0x01,                    // USAGE_MINIMUM (1)
    0x29, 0x03,                    // USAGE_MAXIMUM (3)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x25, 0x01,                    // LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    // REPORT_COUNT (3)
    0x75, 0x01,                    // REPORT_SIZE (1)
    0x81, 0x02,                    // INPUT (Data, Variable, Absolute)
    0x95, 0x01,                    // REPORT_COUNT (1)
    0x75, 0x05,                    // REPORT_SIZE (5)
    0x81, 0x01,                    // INPUT (Constant)
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    // USAGE (X)
    0x09, 0x31,                    // USAGE (Y)
    0x15, 0x81,                    // LOGICAL_MINIMUM (-127)
    0x25, 0x7F,                    // LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    // REPORT_SIZE (8)
    0x95, 0x02,                    // REPORT_COUNT (2)
    0x81, 0x06,                    // Input (Data, Variable, Relative)
    0xC0,                          // END_COLLECTION (Physical)
    0xC0,                          // END_COLLECTION (Application)

    // 절대 좌표 마우스 컬렉션
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x80,                    // USAGE (System Control) - 실제 마우스와 구분
    0xA1, 0x01,                    // COLLECTION (Application)
    0x85, REPORT_ID_MOUSE_ABSOLUTE, // Report ID (0x5A)

    // 플래그 필드 - 분리된 스트림을 명확히 지정하는 설명 추가
    0x05, 0xFF,                    // USAGE_PAGE (Vendor Defined)
    0x09, 0x01,                    // USAGE (Vendor Usage 1 - Separated Stream Flag)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x25, 0x01,                    // LOGICAL_MAXIMUM (1)
    0x75, 0x08,                    // REPORT_SIZE (8)
    0x95, 0x01,                    // REPORT_COUNT (1)
    0x81, 0x02,                    // INPUT (Data,Var,Abs)

    // 버튼
    0x05, 0x09,                    // USAGE_PAGE (Button)
    0x19, 0x01,                    // USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    // USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x25, 0x01,                    // LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    // REPORT_SIZE (1)
    0x95, 0x03,                    // REPORT_COUNT (3)
    0x81, 0x02,                    // INPUT (Data,Var,Abs)

    // 패딩
    0x75, 0x05,                    // REPORT_SIZE (5)
    0x95, 0x01,                    // REPORT_COUNT (1)
    0x81, 0x03,                    // INPUT (Cnst,Var,Abs)

    // X, Y 좌표 (절대 좌표)
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    // USAGE (X)
    0x09, 0x31,                    // USAGE (Y)
    0x16, 0x00, 0x00,              // LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x7F,              // LOGICAL_MAXIMUM (32767)
    0x75, 0x10,                    // REPORT_SIZE (16)
    0x95, 0x02,                    // REPORT_COUNT (2)
    0x81, 0x02,                    // INPUT (Data,Var,Abs)
    0xC0,                          // END_COLLECTION (Application)

    // 키보드 디스크립터
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xA1, 0x01,                    // COLLECTION (Application)
    0x85, REPORT_ID_KEYBOARD_INPUT, // Report ID (0x03)

    // 키 상태 보고를 위한 Modifier 바이트
    0x05, 0x07,                    // USAGE_PAGE (Keyboard/Keypad)
    0x19, 0xE0,                    // USAGE_MINIMUM (Keyboard Left Control)
    0x29, 0xE7,                    // USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x25, 0x01,                    // LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    // REPORT_SIZE (1)
    0x95, 0x08,                    // REPORT_COUNT (8)
    0x81, 0x02,                    // INPUT (Data,Var,Abs)

    // 예약된 바이트
    0x95, 0x01,                    // REPORT_COUNT (1)
    0x75, 0x08,                    // REPORT_SIZE (8)
    0x81, 0x01,                    // INPUT (Const)

    // 6개의 동시 키 누름을 지원하는 키 배열
    0x95, 0x06,                    // REPORT_COUNT (6)
    0x75, 0x08,                    // REPORT_SIZE (8)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x25, 0x65,                    // LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    // USAGE_PAGE (Key Codes)
    0x19, 0x00,                    // USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    // USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    // INPUT (Data,Array)
    0xC0                          // END_COLLECTION (Application)
};

HID_DESCRIPTOR g_hidDescriptor = {
    0x09,        // length of HID descriptor
    0x21,        // descriptor type == HID 0x21
    0x0100,      // hid spec release
    0x00,        // country code == Not Specified
    0x01,        // number of HID class descriptors
    {            // DescriptorList[0]
        0x22,                             // report descriptor type 0x22
        sizeof(g_reportDescriptor)        // total length of report descriptor
    }
};

_Use_decl_annotations_
NTSTATUS
DeviceCreate(
    _Inout_ PWDFDEVICE_INIT deviceInit)
{
    // 기존 코드 유지
    NTSTATUS                status = STATUS_SUCCESS;
    WDFDEVICE               device = NULL;
    PDEVICE_CONTEXT         deviceContext = NULL;
    WDF_OBJECT_ATTRIBUTES   deviceAttributes;

    WdfFdoInitSetFilter(deviceInit);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&deviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    deviceContext = DeviceGetContext(device);
    deviceContext->device = device;
    deviceContext->queueDefault = NULL;
    deviceContext->queueManual = NULL;
    deviceContext->hidDescriptor = &g_hidDescriptor;
    deviceContext->hidReportDescriptor = g_reportDescriptor;

    // 분리된 스트림 플래그 초기값을 FALSE로 변경
    deviceContext->isSeparatedStream = FALSE;

    RtlZeroMemory(&deviceContext->hidDeviceAttributes, sizeof(HID_DEVICE_ATTRIBUTES));
    deviceContext->hidDeviceAttributes.Size = sizeof(HID_DEVICE_ATTRIBUTES);
    deviceContext->hidDeviceAttributes.VendorID = VENDOR_ID;
    deviceContext->hidDeviceAttributes.ProductID = PRODUCT_ID;
    deviceContext->hidDeviceAttributes.VersionNumber = VERSION_NUMBER;

    status = QueueDefaultCreate(device, &deviceContext->queueDefault);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = QueueManualCreate(device, &deviceContext->queueManual);

    return status;
}

// 분리된 스트림 모드 설정 함수 추가
NTSTATUS
SetSeparatedStreamMode(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ BOOLEAN Enable
)
{
    if (DeviceContext == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    DeviceContext->isSeparatedStream = Enable;
    return STATUS_SUCCESS;
}

NTSTATUS
SendAbsoluteMouseInput(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ USHORT X,
    _In_ USHORT Y,
    _In_ BYTE Buttons,
    _In_ BOOLEAN IsSeparatedStream
)
{
    NTSTATUS status = STATUS_SUCCESS;
    
    // 분리 모드가 아닌 경우, 실제 디바이스와 충돌 가능성 확인
    if (!IsSeparatedStream) {
        if (ShouldBlockVirtualInput(DeviceContext)) {
            return STATUS_DEVICE_BUSY;
        }
    }

    HID_MOUSE_ABSOLUTE_REPORT report;
    WDFREQUEST request;
    WDFMEMORY memory;

    // 보고서 초기화
    RtlZeroMemory(&report, sizeof(report));
    report.reportId = REPORT_ID_MOUSE_ABSOLUTE;
    report.flags = IsSeparatedStream ? HID_INPUT_FLAG_SEPARATED : 0;
    report.buttons = Buttons;
    report.x = X;
    report.y = Y;

    // 수동 큐에서 요청 검색
    status = WdfIoQueueRetrieveNextRequest(DeviceContext->queueManual, &request);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // 요청에 메모리 할당
    status = WdfRequestRetrieveOutputMemory(request, &memory);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(request, status);
        return status;
    }

    // 메모리에 보고서 복사
    status = WdfMemoryCopyFromBuffer(memory, 0, &report, sizeof(report));
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(request, status);
        return status;
    }

    // 요청 완료 - 크기 정보 확실히 전달
    WdfRequestCompleteWithInformation(request, status, sizeof(report));
    return status;
}

// SendKeyboardInput 함수 추가
NTSTATUS
SendKeyboardInput(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ BYTE Modifiers,
    _In_ BYTE KeyCodes[6]
)
{
    NTSTATUS status = STATUS_SUCCESS;
    HID_KEYBOARD_INPUT_REPORT report;
    WDFREQUEST request;
    WDFMEMORY memory;

    // 보고서 초기화
    RtlZeroMemory(&report, sizeof(report));
    report.reportId = REPORT_ID_KEYBOARD_INPUT;
    report.modifiers = Modifiers;
    report._reserved = 0;

    if (KeyCodes) {
        RtlCopyMemory(report.keyCodes, KeyCodes, sizeof(report.keyCodes));
    }
    else {
        RtlZeroMemory(report.keyCodes, sizeof(report.keyCodes));
    }

    // 수동 큐에서 요청 검색
    status = WdfIoQueueRetrieveNextRequest(DeviceContext->queueManual, &request);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // 요청에 메모리 할당
    status = WdfRequestRetrieveOutputMemory(request, &memory);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(request, status);
        return status;
    }

    // 메모리에 보고서 복사
    status = WdfMemoryCopyFromBuffer(memory, 0, &report, sizeof(report));
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(request, status);
        return status;
    }

    // 요청 완료
    WdfRequestCompleteWithInformation(request, status, sizeof(report));
    return status;
}

BOOLEAN
ShouldBlockVirtualInput(
    _In_ PDEVICE_CONTEXT DeviceContext
)
{
    UNREFERENCED_PARAMETER(DeviceContext);
    // 실제 마우스 활동 감지 로직 구현
    // 이 예시에서는 항상 FALSE 반환 (차단하지 않음)
    return FALSE;
}