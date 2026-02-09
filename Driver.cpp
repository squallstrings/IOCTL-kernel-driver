#include <ntifs.h>

extern "C"
{
    NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName, PDRIVER_INITIALIZE InitializationFunction);
}

void DebugPrint(PCSTR text)
{
#ifndef DEBUG
    UNREFERENCED_PARAMETER(text);
#endif
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, text));
}

namespace JadDriver
{
    namespace Commands
    {
        constexpr ULONG echo = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS);
        constexpr ULONG queryKeyboard = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS);
        constexpr ULONG queryMouse = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS);
    }

    struct EchoRequest
    {
        CHAR inputBuffer[256];
        CHAR outputBuffer[256];
        ULONG bufferLength;
    };

    struct DeviceInfo
    {
        BOOLEAN devicePresent;
        ULONG lastEventTime;
        USHORT vendorId;
        USHORT productId;
    };

    NTSTATUS HandleCreate(PDEVICE_OBJECT deviceObject, PIRP irp)
    {
        UNREFERENCED_PARAMETER(deviceObject);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return irp->IoStatus.Status;
    }

    NTSTATUS HandleClose(PDEVICE_OBJECT deviceObject, PIRP irp)
    {
        UNREFERENCED_PARAMETER(deviceObject);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return irp->IoStatus.Status;
    }

    NTSTATUS HandleDeviceControl(PDEVICE_OBJECT deviceObject, PIRP irp)
    {
        UNREFERENCED_PARAMETER(deviceObject);

        DebugPrint("[JadDriver] Device control received\n");

        NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
        PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(irp);

        if (irpStack == nullptr)
        {
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            return STATUS_INVALID_PARAMETER;
        }

        const ULONG controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
        const ULONG inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
        const ULONG outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

        switch (controlCode)
        {
        case Commands::echo:
        {
            if (inputBufferLength >= sizeof(EchoRequest) && outputBufferLength >= sizeof(EchoRequest))
            {
                auto request = static_cast<EchoRequest *>(irp->AssociatedIrp.SystemBuffer);
                if (request != nullptr)
                {
                    RtlCopyMemory(request->outputBuffer, request->inputBuffer, min(request->bufferLength, sizeof(request->outputBuffer) - 1));
                    request->outputBuffer[sizeof(request->outputBuffer) - 1] = '\0';
                    status = STATUS_SUCCESS;
                    irp->IoStatus.Information = sizeof(EchoRequest);
                }
            }
            break;
        }
        case Commands::queryKeyboard:
        {
            if (outputBufferLength >= sizeof(DeviceInfo))
            {
                auto deviceInfo = static_cast<DeviceInfo *>(irp->AssociatedIrp.SystemBuffer);
                if (deviceInfo != nullptr)
                {
                    deviceInfo->devicePresent = TRUE;
                    deviceInfo->lastEventTime = 0;
                    deviceInfo->vendorId = 0x046D;
                    deviceInfo->productId = 0xC52B;
                    status = STATUS_SUCCESS;
                    irp->IoStatus.Information = sizeof(DeviceInfo);
                }
            }
            break;
        }
        case Commands::queryMouse:
        {
            if (outputBufferLength >= sizeof(DeviceInfo))
            {
                auto deviceInfo = static_cast<DeviceInfo *>(irp->AssociatedIrp.SystemBuffer);
                if (deviceInfo != nullptr)
                {
                    deviceInfo->devicePresent = TRUE;
                    deviceInfo->lastEventTime = 0;
                    deviceInfo->vendorId = 0x046D;
                    deviceInfo->productId = 0xC52B;
                    status = STATUS_SUCCESS;
                    irp->IoStatus.Information = sizeof(DeviceInfo);
                }
            }
            break;
        }
        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return status;
    }
}

VOID UnloadDriver(PDRIVER_OBJECT driverObject)
{
    UNICODE_STRING symbolicLink = {};
    RtlInitUnicodeString(&symbolicLink, L"\\DosDevices\\JadDriver");
    IoDeleteSymbolicLink(&symbolicLink);
    if (driverObject->DeviceObject != nullptr)
    {
        IoDeleteDevice(driverObject->DeviceObject);
    }
    DebugPrint("[JadDriver] Driver unloaded\n");
}

NTSTATUS InitializeDriver(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath)
{
    UNREFERENCED_PARAMETER(registryPath);

    UNICODE_STRING deviceName = {};
    RtlInitUnicodeString(&deviceName, L"\\Device\\JadDriver");

    PDEVICE_OBJECT deviceObject = nullptr;
    NTSTATUS status = IoCreateDevice(driverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &deviceObject);

    if (!NT_SUCCESS(status))
    {
        DebugPrint("[JadDriver] Device creation failed\n");
        return status;
    }

    UNICODE_STRING symbolicLink = {};
    RtlInitUnicodeString(&symbolicLink, L"\\DosDevices\\JadDriver");

    status = IoCreateSymbolicLink(&symbolicLink, &deviceName);
    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(deviceObject);
        DebugPrint("[JadDriver] Symbolic link creation failed\n");
        return status;
    }

    SetFlag(deviceObject->Flags, DO_BUFFERED_IO);

    driverObject->MajorFunction[IRP_MJ_CREATE] = JadDriver::HandleCreate;
    driverObject->MajorFunction[IRP_MJ_CLOSE] = JadDriver::HandleClose;
    driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = JadDriver::HandleDeviceControl;
    driverObject->DriverUnload = UnloadDriver;

    ClearFlag(deviceObject->Flags, DO_DEVICE_INITIALIZING);

    DebugPrint("[JadDriver] Driver initialized successfully\n");

    return STATUS_SUCCESS;
}

extern "C" NTSTATUS DriverEntry()
{
    DebugPrint("[JadDriver] Entry point reached\n");

    UNICODE_STRING driverName = {};
    RtlInitUnicodeString(&driverName, L"\\Driver\\JadDriver");

    return IoCreateDriver(&driverName, &InitializeDriver);
}
