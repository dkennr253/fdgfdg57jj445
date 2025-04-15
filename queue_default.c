#include "queue_default.h"
#include "queue_manual.h"
#include "device.h"
#include "memory.h"
#include "hid.h"  // HID 구조체 정의가 포함된 헤더 추가

// HID_XFER_PACKET 구조체는 이미 다른 파일에 정의되어 있으므로 여기서 제거합니다

_Use_decl_annotations_
NTSTATUS
QueueDefaultCreate(
    _In_ WDFDEVICE device,
    _Out_ WDFQUEUE* queueOut)
{
    NTSTATUS                  status = STATUS_SUCCESS;
    WDFQUEUE                  queue = NULL;
    PQUEUE_DEFAULT_CONTEXT    context = NULL;
    WDF_IO_QUEUE_CONFIG       config;
    WDF_OBJECT_ATTRIBUTES     attributes;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&config, WdfIoQueueDispatchSequential);
    config.EvtIoInternalDeviceControl = QueueDefaultEvtIoDeviceControl;

    // 사용자 정의 IOCTL 처리를 위한 이벤트 핸들러 추가
    config.EvtIoDeviceControl = QueueDefaultEvtIoCustomControl;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, QUEUE_DEFAULT_CONTEXT);

    status = WdfIoQueueCreate(device, &config, &attributes, &queue);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    context = QueueDefaultGetContext(queue);
    context->device = device;
    context->queue = queue;
    *queueOut = queue;

    return status;
}

// 사용자 정의 IOCTL을 처리하기 위한 함수 추가
_Use_decl_annotations_
VOID
QueueDefaultEvtIoCustomControl(
    _In_ WDFQUEUE   queue,
    _In_ WDFREQUEST request,
    _In_ size_t     outputBufferLength,
    _In_ size_t     inputBufferLength,
    _In_ ULONG      ioControlCode)
{
    UNREFERENCED_PARAMETER(outputBufferLength);

    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_CONTEXT deviceContext = DeviceGetContext(QueueDefaultGetContext(queue)->device);
    PVOID inputBuffer = NULL;
    BOOLEAN streamMode = FALSE;

    switch (ioControlCode) {
    case IOCTL_CUSTOM_SET_SEPARATED_STREAM:
        if (inputBufferLength < sizeof(BOOLEAN)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveInputBuffer(request, sizeof(BOOLEAN), &inputBuffer, NULL);
        if (!NT_SUCCESS(status)) {
            break;
        }

        streamMode = *(BOOLEAN*)inputBuffer;
        status = SetSeparatedStreamMode(deviceContext, streamMode);
        break;

    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    WdfRequestComplete(request, status);
}

_Use_decl_annotations_
VOID
QueueDefaultEvtIoDeviceControl(
    _In_ WDFQUEUE   queue,
    _In_ WDFREQUEST request,
    _In_ size_t     outputBufferLength,
    _In_ size_t     inputBufferLength,
    _In_ ULONG      ioControlCode)
{
    UNREFERENCED_PARAMETER(outputBufferLength);
    UNREFERENCED_PARAMETER(inputBufferLength);

    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_CONTEXT         deviceContext = DeviceGetContext(QueueDefaultGetContext(queue)->device);

    switch (ioControlCode) {
    case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        status = CopyToRequestBuffer(
            request,
            deviceContext->hidDescriptor,
            deviceContext->hidDescriptor->bLength
        );
        WdfRequestComplete(request, status);
        break;
    case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        status = CopyToRequestBuffer(
            request,
            &deviceContext->hidDeviceAttributes,
            sizeof(HID_DEVICE_ATTRIBUTES)
        );
        WdfRequestComplete(request, status);
        break;
    case IOCTL_HID_GET_REPORT_DESCRIPTOR:
        status = CopyToRequestBuffer(
            request,
            deviceContext->hidReportDescriptor,
            deviceContext->hidDescriptor->DescriptorList[0].wReportLength
        );
        WdfRequestComplete(request, status);
        break;
    case IOCTL_HID_READ_REPORT:
    case IOCTL_HID_GET_INPUT_REPORT:
        status = WdfRequestForwardToIoQueue(
            request,
            deviceContext->queueManual
        );
        if (!NT_SUCCESS(status)) {
            WdfRequestComplete(request, status);
        }
        break;
    case IOCTL_HID_WRITE_REPORT:
    case IOCTL_HID_SET_OUTPUT_REPORT:
    {
        // 중괄호로 case 문 내부 변수 선언 영역 구분
        PIRP irp = WdfRequestWdmGetIrp(request);
        if (irp && irp->UserBuffer) {
            PHID_XFER_PACKET transferPacket = (PHID_XFER_PACKET)irp->UserBuffer;
            if (transferPacket && transferPacket->reportBuffer) {
                // reportId에 따라 적절한 함수 호출
                if (transferPacket->reportId == REPORT_ID_MOUSE_ABSOLUTE) {
                    // 절대 좌표 마우스 보고서 처리
                    PHID_MOUSE_ABSOLUTE_REPORT report = (PHID_MOUSE_ABSOLUTE_REPORT)transferPacket->reportBuffer;
                    status = SendAbsoluteMouseInput(
                        deviceContext,
                        report->x,
                        report->y,
                        report->buttons,
                        (report->flags & HID_INPUT_FLAG_SEPARATED) ? TRUE : FALSE
                    );
                }
                else if (transferPacket->reportId == REPORT_ID_KEYBOARD_INPUT) {
                    // 키보드 보고서 처리
                    PHID_KEYBOARD_INPUT_REPORT report = (PHID_KEYBOARD_INPUT_REPORT)transferPacket->reportBuffer;
                    status = SendKeyboardInput(
                        deviceContext,
                        report->modifiers,
                        report->keyCodes
                    );
                }
                else {
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        else {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    WdfRequestComplete(request, status);
    break;
    default:
        status = STATUS_NOT_IMPLEMENTED;
        WdfRequestComplete(request, status);
        break;
    }
}