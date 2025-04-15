#include "queue_manual.h"
#include "memory.h"
#include "hid.h"
#include "device.h"

// 명시적 오류를 해결하기 위해 QueueManualCreate 함수 구현 추가
_Use_decl_annotations_
NTSTATUS
QueueManualCreate(
    _In_ WDFDEVICE Device,
    _Out_ WDFQUEUE* Queue
)
{
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDFQUEUE queue;

    // 수동 큐 초기화 (요청을 자동으로 처리하지 않음)
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

    status = WdfIoQueueCreate(
        Device,               // 상위 디바이스 객체
        &queueConfig,         // 큐 구성
        WDF_NO_OBJECT_ATTRIBUTES, // 객체 속성 없음
        &queue                // 결과 핸들
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    *Queue = queue;
    return status;
}

_Use_decl_annotations_
NTSTATUS
QueueManualSendReport(
    _In_ WDFREQUEST outputRequest,
    _In_ PDEVICE_CONTEXT deviceContext)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFQUEUE queue = deviceContext->queueManual;
    size_t inputReportRequiredSize = 0;
    WDFREQUEST inputRequest;
    WDF_REQUEST_PARAMETERS outputRequestParams;
    HID_XFER_PACKET hidXferPacket;

    WDF_REQUEST_PARAMETERS_INIT(&outputRequestParams);
    WdfRequestGetParameters(outputRequest, &outputRequestParams);

    if (outputRequestParams.Parameters.DeviceIoControl.InputBufferLength < sizeof(HID_XFER_PACKET)) {
        status = STATUS_BUFFER_TOO_SMALL;
        return status;
    }

    // 버퍼 경계 검사 추가
    if (WdfRequestWdmGetIrp(outputRequest)->UserBuffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlCopyMemory(&hidXferPacket, WdfRequestWdmGetIrp(outputRequest)->UserBuffer, sizeof(HID_XFER_PACKET));

    // 버퍼 크기 검증 강화
    if (hidXferPacket.reportBufferLen < 1) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    switch (hidXferPacket.reportId) {
    case REPORT_ID_MOUSE_OUTPUT:
        // 실제 마우스 동작 방지 - 분리된 입력 스트림으로 변환
        hidXferPacket.reportId = REPORT_ID_MOUSE_ABSOLUTE;

        // 버퍼 크기 검증 후 안전하게 복사
        if (hidXferPacket.reportBufferLen >= 1) {
            hidXferPacket.reportBuffer[0] = hidXferPacket.reportId;
        }
        else {
            status = STATUS_INVALID_BUFFER_SIZE;
            return status;
        }

        // 분리 플래그 설정 - deviceContext의 isSeparatedStream 값 사용
        if (hidXferPacket.reportBufferLen >= 2) {
            hidXferPacket.reportBuffer[1] = deviceContext->isSeparatedStream ?
                HID_INPUT_FLAG_SEPARATED : 0;
        }

        inputReportRequiredSize = sizeof(HID_MOUSE_ABSOLUTE_REPORT);
        break;

    case REPORT_ID_KEYBOARD_OUTPUT:
        hidXferPacket.reportId = REPORT_ID_KEYBOARD_INPUT;

        // 버퍼 크기 검증 후 안전하게 복사
        if (hidXferPacket.reportBufferLen >= 1) {
            hidXferPacket.reportBuffer[0] = hidXferPacket.reportId;
        }
        else {
            status = STATUS_INVALID_BUFFER_SIZE;
            return status;
        }

        inputReportRequiredSize = sizeof(HID_KEYBOARD_INPUT_REPORT);
        break;

    case REPORT_ID_MOUSE_ABSOLUTE:
        // 분리된 스트림 처리 로직
        if (hidXferPacket.reportBufferLen >= 2) {
            if (deviceContext->isSeparatedStream) {
                // 분리 플래그 설정
                hidXferPacket.reportBuffer[1] |= HID_INPUT_FLAG_SEPARATED;
            }
            else {
                // 실제 장치와의 충돌 여부 확인
                if (ShouldBlockVirtualInput(deviceContext)) {
                    status = STATUS_DEVICE_BUSY;
                    return status;
                }
                hidXferPacket.reportBuffer[1] &= ~HID_INPUT_FLAG_SEPARATED;
            }
        }

        inputReportRequiredSize = sizeof(HID_MOUSE_ABSOLUTE_REPORT);
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        return status;
    }

    if (hidXferPacket.reportBufferLen < inputReportRequiredSize) {
        status = STATUS_INVALID_BUFFER_SIZE;
        return status;
    }

    status = WdfIoQueueRetrieveNextRequest(queue, &inputRequest);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = CopyToRequestBuffer(inputRequest, hidXferPacket.reportBuffer, inputReportRequiredSize);
    WdfRequestComplete(inputRequest, status);
    return status;
}